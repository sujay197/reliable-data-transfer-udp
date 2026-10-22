#ifdef _WIN32
#define _WIN32_WINNT 0x0601
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#else
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../rto/rto.h"

#ifdef _WIN32
#pragma comment(lib, "ws2_32.lib")
#endif

#define PORT 6025
#define WINDOW_SIZE 4
#define TOTAL_PACKETS 12
#define PACKET_SIZE 250
#define ACK_SIZE 250

typedef struct
{
    double send_time_ms;
    int sent;
    int retransmitted;
} PacketInfo;

/*
 * Return the current time in milliseconds.
 */
static double get_time_ms(void)
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

/*
 * Apply the current RTO value to the UDP socket.
 */
static void apply_rto_timeout(int sockfd, double rto_ms)
{
    struct timeval timeout;

    timeout.tv_sec = (long)(rto_ms / 1000.0);

    timeout.tv_usec =
        (long)((rto_ms -
                timeout.tv_sec * 1000.0) *
               1000.0);

    setsockopt(sockfd,
               SOL_SOCKET,
               SO_RCVTIMEO,
               (const char *)&timeout,
               sizeof(timeout));
}

int main(void)
{
    int sockfd;
    struct sockaddr_in sa;

    int base = 0;
    int next_seq_num = 0;

    char packet[PACKET_SIZE];
    char ack[ACK_SIZE];

    int ack_num;

    RTOEstimator rto;

    PacketInfo packets[TOTAL_PACKETS];

    /*
     * Initialize packet information.
     */
    for (int i = 0; i < TOTAL_PACKETS; i++)
    {
        packets[i].send_time_ms = 0.0;
        packets[i].sent = 0;
        packets[i].retransmitted = 0;
    }

    /*
     * Initial adaptive RTO.
     */
    rto_init(&rto, 2000.0);

#ifdef _WIN32

    /*
     * Initialize Windows Sockets.
     */
    WSADATA wsa_data;

    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0)
    {
        printf("WSAStartup failed.\n");
        return 1;
    }

#endif

    /*
     * Create UDP socket.
     */
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

#ifdef _WIN32
    if (sockfd == INVALID_SOCKET)
#else
    if (sockfd < 0)
#endif
    {
        perror("socket");

#ifdef _WIN32
        WSACleanup();
#endif

        return 1;
    }

    /*
     * Configure receiver address.
     */
    memset(&sa, 0, sizeof(sa));

    sa.sin_family = AF_INET;
    sa.sin_addr.s_addr = inet_addr("127.0.0.1");
    sa.sin_port = htons(PORT);

    /*
     * Apply initial RTO as socket receive timeout.
     */
    apply_rto_timeout(sockfd, rto_get(&rto));

    /*
     * Startup information.
     */
    printf("=====================================\n");
    printf("      Go-Back-N Sender Started\n");
    printf("=====================================\n");
    printf("Window size: %d\n", WINDOW_SIZE);
    printf("Total packets: %d\n", TOTAL_PACKETS);
    printf("Packet loss handling: ENABLED\n");
    printf("Initial RTO: %.2f ms\n", rto_get(&rto));
    printf("Receiver: 127.0.0.1:%d\n\n", PORT);

    /*
     * Continue until every packet has been acknowledged.
     */
    while (base < TOTAL_PACKETS)
    {
        /*
         * Send new packets while there is room
         * in the Go-Back-N sliding window.
         */
        while (next_seq_num < base + WINDOW_SIZE &&
               next_seq_num < TOTAL_PACKETS)
        {
            int seq = next_seq_num;

            snprintf(packet,
                     sizeof(packet),
                     "SEQ=%d|DATA=Packet%d",
                     seq,
                     seq);

            printf("Sending: %s\n", packet);

            if (sendto(sockfd,
                       packet,
                       (int)strlen(packet),
                       0,
                       (struct sockaddr *)&sa,
                       sizeof(sa)) < 0)
            {
                perror("sendto");

#ifdef _WIN32
                closesocket(sockfd);
                WSACleanup();
#else
                close(sockfd);
#endif

                return 1;
            }

            /*
             * Record the exact send time for this packet.
             */
            packets[seq].send_time_ms = get_time_ms();
            packets[seq].sent = 1;
            packets[seq].retransmitted = 0;

            next_seq_num++;
        }

        /*
         * Wait for an ACK.
         */
        memset(ack, 0, sizeof(ack));

#ifdef _WIN32
        int received =
            recvfrom(sockfd,
                     ack,
                     sizeof(ack) - 1,
                     0,
                     NULL,
                     NULL);
#else
        ssize_t received =
            recvfrom(sockfd,
                     ack,
                     sizeof(ack) - 1,
                     0,
                     NULL,
                     NULL);
#endif

        /*
         * No ACK received before the current RTO.
         */
        if (received < 0)
        {
            printf("\n*** TIMEOUT ***\n");
            printf("Current adaptive RTO: %.2f ms\n",
                   rto_get(&rto));
            printf("Base = %d\n", base);
            printf("Retransmitting unacknowledged packets...\n\n");

            /*
             * Go-Back-N:
             * retransmit every outstanding packet
             * starting from base.
             */
            for (int i = base; i < next_seq_num; i++)
            {
                snprintf(packet,
                         sizeof(packet),
                         "SEQ=%d|DATA=Packet%d",
                         i,
                         i);

                printf("Retransmitting: %s\n", packet);

                if (sendto(sockfd,
                           packet,
                           (int)strlen(packet),
                           0,
                           (struct sockaddr *)&sa,
                           sizeof(sa)) < 0)
                {
                    perror("sendto");

#ifdef _WIN32
                    closesocket(sockfd);
                    WSACleanup();
#else
                    close(sockfd);
#endif

                    return 1;
                }

                /*
                 * Mark packet as retransmitted.
                 *
                 * Karn's algorithm:
                 * an RTT sample from a retransmitted packet
                 * must not be used to update the RTO.
                 */
                packets[i].retransmitted = 1;

                /*
                 * Record the new transmission time.
                 * This is useful for timeout tracking, but
                 * the ACK will not be used as an RTT sample.
                 */
                packets[i].send_time_ms = get_time_ms();
            }

            printf("\n");

            continue;
        }

        /*
         * Convert received ACK into a C string.
         */
        ack[received] = '\0';

        printf("Received: %s\n", ack);

        /*
         * Parse ACK.
         */
        if (sscanf(ack, "ACK=%d", &ack_num) == 1)
        {
            printf("ACK number: %d\n", ack_num);

            /*
             * VALID ACK
             */
            if (ack_num >= base &&
                ack_num < next_seq_num)
            {
                /*
                 * Measure RTT only if this packet
                 * was not retransmitted.
                 */
                if (packets[ack_num].sent &&
                    !packets[ack_num].retransmitted)
                {
                    double ack_time_ms =
                        get_time_ms();

                    double measured_rtt_ms =
                        ack_time_ms -
                        packets[ack_num].send_time_ms;

                    /*
                     * Avoid zero/negative RTT.
                     */
                    if (measured_rtt_ms <= 0.0)
                    {
                        measured_rtt_ms = 1.0;
                    }

                    /*
                     * Update adaptive RTO.
                     */
                    rto_update(&rto,
                               measured_rtt_ms);

                    /*
                     * Apply updated RTO.
                     */
                    apply_rto_timeout(sockfd,
                                      rto_get(&rto));

                    printf("Measured RTT: %.2f ms\n",
                           measured_rtt_ms);

                    printf("Updated RTO: %.2f ms\n",
                           rto_get(&rto));
                }
                else
                {
                    /*
                     * ACK corresponds to a retransmitted
                     * packet. Do not use it for RTT/RTO.
                     */
                    printf("RTT sample skipped "
                           "(packet was retransmitted).\n");

                    printf("Current RTO: %.2f ms\n",
                           rto_get(&rto));
                }

                /*
                 * Advance the Go-Back-N base.
                 */
                base = ack_num + 1;

                printf("ACK accepted: %d\n", ack_num);
                printf("Updated base: %d\n", base);
            }

            /*
             * OLD / DUPLICATE ACK
             */
            else if (ack_num < base)
            {
                printf("Duplicate/old ACK: %d\n",
                       ack_num);

                printf("Current base remains: %d\n",
                       base);
            }

            /*
             * INVALID / FUTURE ACK
             */
            else
            {
                printf("Invalid/future ACK: %d\n",
                       ack_num);

                printf("Current base remains: %d\n",
                       base);
            }
        }
        else
        {
            printf("Invalid ACK received: %s\n",
                   ack);
        }

        printf("\n");
    }

    /*
     * Transmission completed successfully.
     */
    printf("=====================================\n");
    printf("     Transmission Complete\n");
    printf("=====================================\n");
    printf("Base: %d\n", base);
    printf("Next sequence number: %d\n",
           next_seq_num);
    printf("Final RTO: %.2f ms\n",
           rto_get(&rto));
    printf("All %d packets acknowledged.\n",
           TOTAL_PACKETS);

#ifdef _WIN32
    closesocket(sockfd);
    WSACleanup();
#else
    close(sockfd);
#endif

    return 0;
}