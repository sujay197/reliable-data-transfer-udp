
#include "../rto/rto.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#ifdef _WIN32

#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>

#else

#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
#include <errno.h>

#endif

#define SERVER_IP "127.0.0.1"
#define PORT 6025

#define INITIAL_RTO_MS 2000.0
#define MAX_ATTEMPTS 5
#define BUFFER_SIZE 1024

static unsigned short calculate_checksum(const char *data)
{
    unsigned long sum = 0;

    while (*data) {
        sum += (unsigned char)*data;
        data++;
    }

    return (unsigned short)(sum & 0xFFFF);
}

static double current_time_ms(void)
{
#ifdef _WIN32

    LARGE_INTEGER frequency;
    LARGE_INTEGER counter;

    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&counter);

    return ((double)counter.QuadPart * 1000.0) /
           (double)frequency.QuadPart;

#else

    struct timeval tv;

    gettimeofday(&tv, NULL);

    return (double)tv.tv_sec * 1000.0 +
           (double)tv.tv_usec / 1000.0;

#endif
}

static void set_timeout(
#ifdef _WIN32
    SOCKET sock,
#else
    int sock,
#endif
    double timeout_ms)
{
#ifdef _WIN32

    DWORD timeout = (DWORD)timeout_ms;

    setsockopt(
        sock,
        SOL_SOCKET,
        SO_RCVTIMEO,
        (const char *)&timeout,
        sizeof(timeout)
    );

#else

    struct timeval tv;

    tv.tv_sec = (long)(timeout_ms / 1000.0);
    tv.tv_usec = (long)((timeout_ms -
                         (double)tv.tv_sec * 1000.0) * 1000.0);

    setsockopt(
        sock,
        SOL_SOCKET,
        SO_RCVTIMEO,
        &tv,
        sizeof(tv)
    );

#endif
}

static int receive_ack(
#ifdef _WIN32
    SOCKET sock,
#else
    int sock,
#endif
    int expected_seq,
    double timeout_ms,
    double *sample_rtt)
{
    char buffer[BUFFER_SIZE];

    set_timeout(sock, timeout_ms);

    double start_time = current_time_ms();

    int received = recvfrom(
        sock,
        buffer,
        sizeof(buffer) - 1,
        0,
        NULL,
        NULL
    );

    if (received < 0) {

#ifdef _WIN32
        int error_code = WSAGetLastError();

        if (error_code == WSAETIMEDOUT) {
            printf("Timeout waiting for ACK=%d\n", expected_seq);
            return 0;
        }

        /*
         * Windows can report WSAECONNRESET (10054) when a UDP
         * peer is no longer available.
         *
         * Treat it as a failed attempt rather than retrying
         * forever.
         */
        if (error_code == WSAECONNRESET) {
            printf("recvfrom failed: %d\n", error_code);
            printf("Treating receive error as failed attempt.\n");
            return 0;
        }

        printf("recvfrom failed: %d\n", error_code);
        return 0;

#else

        if (errno == EAGAIN ||
            errno == EWOULDBLOCK ||
            errno == ETIMEDOUT) {

            printf("Timeout waiting for ACK=%d\n", expected_seq);
            return 0;
        }

        perror("recvfrom");
        return 0;

#endif
    }

    buffer[received] = '\0';

    double end_time = current_time_ms();

    if (sample_rtt != NULL) {
        *sample_rtt = end_time - start_time;
    }

    int ack_seq = -1;

    if (sscanf(buffer, "ACK=%d", &ack_seq) != 1) {
        printf("Invalid ACK received: %s\n", buffer);
        return 0;
    }

    printf("Received: %s\n", buffer);

    if (ack_seq != expected_seq) {
        printf(
            "Unexpected ACK=%d, expected ACK=%d\n",
            ack_seq,
            expected_seq
        );

        return 0;
    }

    printf(
        "Valid ACK received for SEQ=%d\n",
        expected_seq
    );

    return 1;
}

int main(void)
{
#ifdef _WIN32

    WSADATA wsa;

    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("WSAStartup failed\n");
        return 1;
    }

#endif

    printf("Stop-and-Wait Sender Started\n");
    printf("Adaptive RTO: ENABLED\n");
    printf("Initial RTO: %.2f ms\n", INITIAL_RTO_MS);
    printf("Server: %s:%d\n", SERVER_IP, PORT);
    printf("Maximum attempts per packet: %d\n\n", MAX_ATTEMPTS);

#ifdef _WIN32

    SOCKET sock = socket(
        AF_INET,
        SOCK_DGRAM,
        IPPROTO_UDP
    );

    if (sock == INVALID_SOCKET) {
        printf("Socket creation failed: %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

#else

    int sock = socket(
        AF_INET,
        SOCK_DGRAM,
        0
    );

    if (sock < 0) {
        perror("Socket creation failed");
        return 1;
    }

#endif

    struct sockaddr_in server_addr;

    memset(
        &server_addr,
        0,
        sizeof(server_addr)
    );

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);

    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);

if (server_addr.sin_addr.s_addr == INADDR_NONE) {
    printf("Invalid server address\n");

#ifdef _WIN32
    closesocket(sock);
    WSACleanup();
#else
    close(sock);
#endif

    return 1;
}

    RTOEstimator rto;

    rto_init(
        &rto,
        INITIAL_RTO_MS
    );

    int sequence_number = 0;

    char message[BUFFER_SIZE];

    while (1) {

        printf("Enter a message: ");

        if (fgets(
                message,
                sizeof(message),
                stdin) == NULL) {

            break;
        }

        message[strcspn(message, "\r\n")] = '\0';

        if (strlen(message) == 0) {
            printf("Message cannot be empty.\n\n");
            continue;
        }

        char packet[BUFFER_SIZE];

        unsigned short checksum =
            calculate_checksum(message);

        snprintf(
            packet,
            sizeof(packet),
            "SEQ=%d|CHK=%04X|DATA=%s",
            sequence_number,
            checksum,
            message
        );

        int acknowledged = 0;
        int attempt = 0;

        int retransmitted = 0;

        while (!acknowledged &&
               attempt < MAX_ATTEMPTS) {

            attempt++;

            printf("\nSending SEQ=%d\n", sequence_number);
            printf("Attempt: %d\n", attempt);
            printf(
                "Current RTO: %.2f ms\n",
                rto_get(&rto)
            );

            int sent = sendto(
                sock,
                packet,
                (int)strlen(packet),
                0,
                (struct sockaddr *)&server_addr,
                sizeof(server_addr)
            );

            if (sent < 0) {

#ifdef _WIN32
                printf(
                    "sendto failed: %d\n",
                    WSAGetLastError()
                );
#else
                perror("sendto failed");
#endif

                continue;
            }

            double sample_rtt = 0.0;

            int result = receive_ack(
                sock,
                sequence_number,
                rto_get(&rto),
                &sample_rtt
            );

            if (result) {

                acknowledged = 1;

                if (!retransmitted) {

                    printf(
                        "RTT sample: %.2f ms\n",
                        sample_rtt
                    );

                    rto_update(
                        &rto,
                        sample_rtt
                    );

                    printf(
                        "Updated RTO: %.2f ms\n",
                        rto_get(&rto)
                    );

                } else {

                    printf(
                        "RTT sample skipped for SEQ=%d "
                        "(packet was retransmitted).\n",
                        sequence_number
                    );

                    printf(
                        "Current RTO: %.2f ms\n",
                        rto_get(&rto)
                    );
                }

                printf(
                    "Transmission successful for SEQ=%d\n",
                    sequence_number
                );

                printf(
                    "Final RTO after packet: %.2f ms\n\n",
                    rto_get(&rto)
                );

            } else {

                if (attempt < MAX_ATTEMPTS) {

                    retransmitted = 1;

                    printf(
                        "Retransmitting SEQ=%d...\n",
                        sequence_number
                    );

                } else {

                    printf(
                        "Maximum attempts reached for "
                        "SEQ=%d.\n",
                        sequence_number
                    );
                }
            }
        }

        if (!acknowledged) {

            printf(
                "Transmission failed for SEQ=%d "
                "after %d attempts.\n",
                sequence_number,
                MAX_ATTEMPTS
            );

            printf(
                "Stopping sender to avoid infinite retries.\n"
            );

            break;
        }

        sequence_number++;
    }

#ifdef _WIN32

    closesocket(sock);
    WSACleanup();

#else

    close(sock);

#endif

    printf("\nStop-and-Wait Sender Stopped.\n");

    return 0;
}