#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32

#include <winsock2.h>
#include <ws2tcpip.h>

#else

#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#endif

#define PORT 6026
#define TOTAL_PACKETS 10
#define WINDOW_SIZE 4
#define BUFFER_SIZE 1024

static void close_socket_platform(int sockfd)
{
#ifdef _WIN32
    closesocket(sockfd);
#else
    close(sockfd);
#endif
}

int main(void)
{
#ifdef _WIN32

    WSADATA wsa_data;

    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
        printf("WSAStartup failed.\n");
        return 1;
    }

#endif

    int sockfd;

    struct sockaddr_in server;
    struct sockaddr_in client;

#ifdef _WIN32
    int client_len;
#else
    socklen_t client_len;
#endif

    char buffer[BUFFER_SIZE];

    int expected = 0;

    int received[TOTAL_PACKETS] = {0};

    /*
     * Test configuration:
     * Drop the first arrival of SEQ=2.
     */
    int dropped_packet_2 = 0;

    sockfd = socket(
        AF_INET,
        SOCK_DGRAM,
        0);

    if (sockfd < 0) {

#ifdef _WIN32
        printf(
            "socket failed with error: %d\n",
            WSAGetLastError());

        WSACleanup();

#else
        perror("socket");
#endif

        return 1;
    }

    memset(
        &server,
        0,
        sizeof(server));

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(PORT);

    if (bind(
            sockfd,
            (struct sockaddr *)&server,
            sizeof(server)) < 0) {

#ifdef _WIN32
        printf(
            "bind failed with error: %d\n",
            WSAGetLastError());
#else
        perror("bind");
#endif

        close_socket_platform(sockfd);

#ifdef _WIN32
        WSACleanup();
#endif

        return 1;
    }

    printf("=====================================\n");
    printf("   Selective Repeat Receiver Started\n");
    printf("=====================================\n");
    printf("Listening on port: %d\n", PORT);
    printf("Window size: %d\n", WINDOW_SIZE);
    printf("First copy of SEQ=2 will be dropped.\n");
    printf("=====================================\n\n");

    while (1) {

        client_len = sizeof(client);

        int n = recvfrom(
            sockfd,
            buffer,
            sizeof(buffer) - 1,
            0,
            (struct sockaddr *)&client,
            &client_len);

        if (n < 0) {

#ifdef _WIN32
            printf(
                "recvfrom failed with error: %d\n",
                WSAGetLastError());
#else
            perror("recvfrom");
#endif

            continue;
        }

        buffer[n] = '\0';

        /*
         * END is sent after all packets have been acknowledged.
         */
        if (strcmp(buffer, "END") == 0) {

            if (expected == TOTAL_PACKETS) {

                printf(
                    "\nAll packets delivered. "
                    "Transmission finished.\n");

            } else {

                printf(
                    "\nEND received, but delivery is "
                    "incomplete: %d of %d packets delivered.\n",
                    expected,
                    TOTAL_PACKETS);
            }

            break;
        }

        int seq;
        char data[BUFFER_SIZE];

        if (sscanf(
                buffer,
                "%d|%1023[^\n]",
                &seq,
                data) != 2) {

            printf(
                "Invalid packet received: %s\n",
                buffer);

            continue;
        }

        printf(
            "Received: SEQ=%d DATA=%s\n",
            seq,
            data);

        if (seq < 0 || seq >= TOTAL_PACKETS) {

            printf(
                "Sequence number out of range.\n");

            continue;
        }

        /*
         * Simulate one lost data packet.
         *
         * The first copy of SEQ=2 is deliberately dropped.
         * No ACK is sent.
         *
         * The sender's individual timer for SEQ=2
         * should therefore expire.
         */
        if (seq == 2 && !dropped_packet_2) {

            dropped_packet_2 = 1;

            printf(
                "SIMULATED LOSS: Dropping first copy "
                "of SEQ=2; no ACK sent.\n\n");

            continue;
        }

        /*
         * Packet is outside the current receiver window.
         */
        if (seq >= expected + WINDOW_SIZE) {

            printf(
                "SEQ=%d outside receiver window "
                "[%d, %d].\n",
                seq,
                expected,
                expected + WINDOW_SIZE - 1);

            continue;
        }

        /*
         * Packet was already delivered.
         *
         * Re-send its ACK so the sender can recover
         * from a lost ACK.
         */
        if (seq < expected) {

            printf(
                "Duplicate of previously delivered "
                "SEQ=%d.\n",
                seq);

        } else if (!received[seq]) {

            /*
             * First time receiving this packet.
             * Store it in the receiver buffer.
             */
            received[seq] = 1;

            printf(
                "Packet %d accepted and buffered.\n",
                seq);

        } else {

            printf(
                "Duplicate packet %d received.\n",
                seq);
        }

        /*
         * Selective Repeat sends an individual ACK
         * for the received packet.
         */
        char ack[32];

        snprintf(
            ack,
            sizeof(ack),
            "ACK=%d",
            seq);

        if (sendto(
                sockfd,
                ack,
                (int)strlen(ack),
                0,
                (struct sockaddr *)&client,
                client_len) < 0) {

#ifdef _WIN32
            printf(
                "sendto ACK failed with error: %d\n",
                WSAGetLastError());
#else
            perror("sendto ACK");
#endif

            continue;
        }

        printf(
            "Sent %s\n",
            ack);

        /*
         * Deliver only consecutive packets beginning
         * at 'expected'.
         *
         * Out-of-order packets remain buffered.
         */
        while (
            expected < TOTAL_PACKETS &&
            received[expected]) {

            printf(
                "Delivering packet %d in order.\n",
                expected);

            expected++;
        }

        printf(
            "Next expected packet: %d\n\n",
            expected);
    }

    close_socket_platform(sockfd);

#ifdef _WIN32
    WSACleanup();
#endif

    return 0;
}