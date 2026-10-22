#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stddef.h>

#include "../rto/rto.h"

#ifdef _WIN32

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

#else

#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>

#endif

#define PORT 6026
#define TOTAL_PACKETS 10
#define WINDOW_SIZE 4

#define INITIAL_RTO_MS 2000.0

#ifdef _WIN32

static double current_time_ms(void)
{
    static LARGE_INTEGER frequency;
    static int initialized = 0;

    LARGE_INTEGER counter;

    if (!initialized) {
        QueryPerformanceFrequency(&frequency);
        initialized = 1;
    }

    QueryPerformanceCounter(&counter);

    return ((double)counter.QuadPart * 1000.0) /
           (double)frequency.QuadPart;
}

#else

static double current_time_ms(void)
{
    struct timeval tv;

    gettimeofday(&tv, NULL);

    return (double)tv.tv_sec * 1000.0 +
           (double)tv.tv_usec / 1000.0;
}

#endif

static void close_socket_platform(int sockfd)
{
#ifdef _WIN32
    closesocket(sockfd);
#else
    close(sockfd);
#endif
}

static int send_packet(
    int sockfd,
    struct sockaddr_in *receiver,
#ifdef _WIN32
    int receiver_len,
#else
    socklen_t receiver_len,
#endif
    int seq,
    double sent_at[],
    int retransmission)
{
    char packet[128];

    snprintf(packet,
             sizeof(packet),
             "%d|Packet%d",
             seq,
             seq);

    int n = sendto(
        sockfd,
        packet,
        (int)strlen(packet),
        0,
        (struct sockaddr *)receiver,
        receiver_len);

    if (n < 0) {
#ifdef _WIN32
        printf("sendto failed with error: %d\n",
               WSAGetLastError());
#else
        perror("sendto");
#endif
        return -1;
    }

    sent_at[seq] = current_time_ms();

    if (retransmission) {
        printf("Retransmitting: SEQ=%d|DATA=Packet%d\n",
               seq,
               seq);
    } else {
        printf("Sending: SEQ=%d|DATA=Packet%d\n",
               seq,
               seq);
    }

    return 0;
}

static void print_rto(double rto_ms)
{
    printf("Current adaptive RTO: %.2f ms\n",
           rto_ms);
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

    struct sockaddr_in receiver;

#ifdef _WIN32
    int receiver_len = sizeof(receiver);
#else
    socklen_t receiver_len = sizeof(receiver);
#endif

    int acked[TOTAL_PACKETS] = {0};
    int retransmitted[TOTAL_PACKETS] = {0};

    double sent_at[TOTAL_PACKETS] = {0.0};

    int base = 0;
    int next_seq = 0;

    RTOEstimator rto;

    rto_init(&rto, INITIAL_RTO_MS);

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    if (sockfd < 0) {
#ifdef _WIN32
        printf("socket failed with error: %d\n",
               WSAGetLastError());
        WSACleanup();
#else
        perror("socket");
#endif
        return 1;
    }

    memset(&receiver, 0, sizeof(receiver));

    receiver.sin_family = AF_INET;
    receiver.sin_port = htons(PORT);
    receiver.sin_addr.s_addr = inet_addr("127.0.0.1");

    printf("=====================================\n");
    printf("    Selective Repeat Sender Started\n");
    printf("=====================================\n");
    printf("Window size: %d\n", WINDOW_SIZE);
    printf("Total packets: %d\n", TOTAL_PACKETS);
    printf("Initial RTO: %.2f ms\n",
           INITIAL_RTO_MS);
    printf("Adaptive RTO: ENABLED\n");
    printf("Receiver: 127.0.0.1:%d\n", PORT);
    printf("=====================================\n\n");

    while (base < TOTAL_PACKETS) {

        /*
         * Fill the sending window.
         */
        while (next_seq < TOTAL_PACKETS &&
               next_seq < base + WINDOW_SIZE) {

            if (send_packet(
                    sockfd,
                    &receiver,
                    receiver_len,
                    next_seq,
                    sent_at,
                    0) < 0) {

                close_socket_platform(sockfd);

#ifdef _WIN32
                WSACleanup();
#endif

                return 1;
            }

            next_seq++;
        }

        /*
         * Find the nearest timeout among all
         * currently unacknowledged packets.
         */
        double now = current_time_ms();

        double current_rto = rto_get(&rto);

        double nearest_deadline = -1.0;

        for (int i = base; i < next_seq; i++) {

            if (!acked[i]) {

                double deadline =
                    sent_at[i] + current_rto;

                if (nearest_deadline < 0.0 ||
                    deadline < nearest_deadline) {

                    nearest_deadline = deadline;
                }
            }
        }

        if (nearest_deadline < 0.0) {
            continue;
        }

        double remaining =
            nearest_deadline - now;

        if (remaining < 0.0) {
            remaining = 0.0;
        }

#ifdef _WIN32

        /*
         * Windows does not use select() with the same
         * timeout structure as POSIX sockets.
         *
         * We use SO_RCVTIMEO for the current adaptive RTO.
         */
        DWORD timeout_ms =
            (DWORD)remaining;

        setsockopt(
            sockfd,
            SOL_SOCKET,
            SO_RCVTIMEO,
            (const char *)&timeout_ms,
            sizeof(timeout_ms));

#else

        struct timeval timeout;

        timeout.tv_sec =
            (time_t)(remaining / 1000.0);

        timeout.tv_usec =
            (suseconds_t)(
                ((long long)remaining % 1000LL) * 1000LL);

        fd_set readfds;

        FD_ZERO(&readfds);
        FD_SET(sockfd, &readfds);

        int ready =
            select(
                sockfd + 1,
                &readfds,
                NULL,
                NULL,
                &timeout);

        if (ready < 0) {

            if (errno == EINTR) {
                continue;
            }

            perror("select");

            close_socket_platform(sockfd);
            return 1;
        }

        if (ready == 0) {

            /*
             * One or more individual packet timers
             * have expired.
             *
             * Selective Repeat retransmits ONLY the
             * packets whose own timers expired.
             */

            now = current_time_ms();

            current_rto = rto_get(&rto);

            printf("\n*** SELECTIVE REPEAT TIMEOUT ***\n");
            print_rto(current_rto);

            for (int i = base; i < next_seq; i++) {

                if (!acked[i] &&
                    now - sent_at[i] >= current_rto) {

                    printf(
                        "Timeout for SEQ=%d\n",
                        i);

                    if (send_packet(
                            sockfd,
                            &receiver,
                            receiver_len,
                            i,
                            sent_at,
                            1) < 0) {

                        close_socket_platform(sockfd);
                        return 1;
                    }

                    /*
                     * Karn's algorithm:
                     * this packet must not be used for
                     * an RTT sample after retransmission.
                     */
                    retransmitted[i] = 1;
                }
            }

            continue;
        }

#endif

        /*
         * Receive ACK.
         */
        char response[128];

        struct sockaddr_in ack_source;

#ifdef _WIN32
        int ack_source_len = sizeof(ack_source);
#else
        socklen_t ack_source_len = sizeof(ack_source);
#endif

        int n = recvfrom(
            sockfd,
            response,
            sizeof(response) - 1,
            0,
            (struct sockaddr *)&ack_source,
            &ack_source_len);

        if (n < 0) {

#ifdef _WIN32

            int error = WSAGetLastError();

            if (error == WSAETIMEDOUT) {

                /*
                 * Receive timeout means we need to check
                 * individual packet timers.
                 */
                now = current_time_ms();

                current_rto = rto_get(&rto);

                printf("\n*** SELECTIVE REPEAT TIMEOUT ***\n");
                print_rto(current_rto);

                for (int i = base; i < next_seq; i++) {

                    if (!acked[i] &&
                        now - sent_at[i] >= current_rto) {

                        printf(
                            "Timeout for SEQ=%d\n",
                            i);

                        if (send_packet(
                                sockfd,
                                &receiver,
                                receiver_len,
                                i,
                                sent_at,
                                1) < 0) {

                            close_socket_platform(sockfd);
                            WSACleanup();
                            return 1;
                        }

                        retransmitted[i] = 1;
                    }
                }

                continue;
            }

            printf(
                "recvfrom failed with error: %d\n",
                error);

            close_socket_platform(sockfd);
            WSACleanup();
            return 1;

#else

            if (errno == EINTR) {
                continue;
            }

            perror("recvfrom");

            close_socket_platform(sockfd);
            return 1;

#endif
        }

        response[n] = '\0';

        int ack;

        if (sscanf(response, "ACK=%d", &ack) == 1 &&
            ack >= 0 &&
            ack < TOTAL_PACKETS &&
            ack >= base &&
            ack < next_seq) {

            if (!acked[ack]) {

                acked[ack] = 1;

                printf(
                    "Received: ACK=%d\n",
                    ack);

                /*
                 * RTT measurement is valid only when
                 * the packet was never retransmitted.
                 */
                if (!retransmitted[ack]) {

                    double sample_rtt =
                        current_time_ms() -
                        sent_at[ack];

                    printf(
                        "Measured RTT for SEQ=%d: %.2f ms\n",
                        ack,
                        sample_rtt);

                    rto_update(
                        &rto,
                        sample_rtt);

                    printf(
                        "Updated RTO: %.2f ms\n",
                        rto_get(&rto));

                } else {

                    printf(
                        "RTT sample skipped for SEQ=%d "
                        "(packet was retransmitted).\n",
                        ack);

                    printf(
                        "Current RTO: %.2f ms\n",
                        rto_get(&rto));
                }

                /*
                 * Advance the base only across
                 * consecutive ACKed packets.
                 *
                 * This preserves Selective Repeat.
                 */
                while (base < TOTAL_PACKETS &&
                       acked[base]) {

                    base++;
                }

                printf(
                    "Updated base: %d\n\n",
                    base);

            } else {

                printf(
                    "Duplicate ACK=%d\n",
                    ack);
            }

        } else {

            printf(
                "Ignoring invalid or out-of-window "
                "response: %s\n",
                response);
        }
    }

    /*
     * All packets have been acknowledged.
     * Tell the receiver the transmission is complete.
     */
    if (sendto(
            sockfd,
            "END",
            3,
            0,
            (struct sockaddr *)&receiver,
            receiver_len) < 0) {

#ifdef _WIN32
        printf(
            "sendto END failed with error: %d\n",
            WSAGetLastError());
#else
        perror("sendto END");
#endif

        close_socket_platform(sockfd);

#ifdef _WIN32
        WSACleanup();
#endif

        return 1;
    }

    printf("=====================================\n");
    printf("     Transmission Complete\n");
    printf("=====================================\n");
    printf("Base: %d\n", base);
    printf("Next sequence number: %d\n", next_seq);
    printf("Final RTO: %.2f ms\n",
           rto_get(&rto));
    printf("All %d packets acknowledged.\n",
           TOTAL_PACKETS);

    close_socket_platform(sockfd);

#ifdef _WIN32
    WSACleanup();
#endif

    return 0;
}