#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 6025
#define WINDOW_SIZE 4
#define TOTAL_PACKETS 12
#define PACKET_SIZE 250
#define ACK_SIZE 250
#define TIMEOUT_SEC 2

int main()
{
    int sockfd;
    struct sockaddr_in sa;

    int base = 0;
    int next_seq_num = 0;

    char packet[PACKET_SIZE];
    char ack[ACK_SIZE];

    int ack_num;

    /* Create UDP socket */
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    if (sockfd < 0)
    {
        perror("socket");
        return 1;
    }

    /* Configure receiver address */
    memset(&sa, 0, sizeof(sa));

    sa.sin_family = AF_INET;
    sa.sin_addr.s_addr = inet_addr("127.0.0.1");
    sa.sin_port = htons(PORT);

    /* Set receive timeout */
    struct timeval timeout;

    timeout.tv_sec = TIMEOUT_SEC;
    timeout.tv_usec = 0;

    if (setsockopt(sockfd,
                   SOL_SOCKET,
                   SO_RCVTIMEO,
                   &timeout,
                   sizeof(timeout)) < 0)
    {
        perror("setsockopt");
        close(sockfd);
        return 1;
    }

    /* Startup information */
    printf("=====================================\n");
    printf("      Go-Back-N Sender Started\n");
    printf("=====================================\n");
    printf("Window size: %d\n", WINDOW_SIZE);
    printf("Total packets: %d\n", TOTAL_PACKETS);
    printf("Packet loss handling: ENABLED\n");
    printf("Timeout: %d seconds\n", TIMEOUT_SEC);
    printf("Receiver: 127.0.0.1:%d\n\n", PORT);

    /*
     * Continue until every packet has been acknowledged.
     *
     * base:
     *   Oldest unacknowledged packet.
     *
     * next_seq_num:
     *   Next new packet that can be transmitted.
     */
    while (base < TOTAL_PACKETS)
    {
        /*
         * Send new packets while there is space
         * in the Go-Back-N sliding window.
         */
        while (next_seq_num < base + WINDOW_SIZE &&
               next_seq_num < TOTAL_PACKETS)
        {
            snprintf(packet,
                     sizeof(packet),
                     "SEQ=%d|DATA=Packet%d",
                     next_seq_num,
                     next_seq_num);

            printf("Sending: %s\n", packet);

            if (sendto(sockfd,
                       packet,
                       strlen(packet),
                       0,
                       (struct sockaddr *)&sa,
                       sizeof(sa)) < 0)
            {
                perror("sendto");
                close(sockfd);
                return 1;
            }

            next_seq_num++;
        }

        /*
         * Wait for an ACK.
         */
        memset(ack, 0, sizeof(ack));

        ssize_t received = recvfrom(sockfd,
                                    ack,
                                    sizeof(ack) - 1,
                                    0,
                                    NULL,
                                    NULL);

        /*
         * No ACK received before timeout.
         */
        if (received < 0)
        {
            printf("\n*** TIMEOUT ***\n");
            printf("No ACK received for %d seconds.\n", TIMEOUT_SEC);
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
                           strlen(packet),
                           0,
                           (struct sockaddr *)&sa,
                           sizeof(sa)) < 0)
                {
                    perror("sendto");
                    close(sockfd);
                    return 1;
                }
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
             *
             * ACK must:
             *
             * 1. Be at or after the current base.
             * 2. Refer to a packet that has actually
             *    been sent.
             *
             * next_seq_num is one greater than the
             * highest packet sent.
             *
             * Therefore valid ACK numbers are:
             *
             *     base <= ACK < next_seq_num
             */
            if (ack_num >= base &&
                ack_num < next_seq_num)
            {
                base = ack_num + 1;

                printf("ACK accepted: %d\n", ack_num);
                printf("Updated base: %d\n", base);
            }

            /*
             * OLD / DUPLICATE ACK
             */
            else if (ack_num < base)
            {
                printf("Duplicate/old ACK: %d\n", ack_num);
                printf("Current base remains: %d\n", base);
            }

            /*
             * INVALID / FUTURE ACK
             *
             * Example:
             *
             * base = 0
             * next_seq_num = 4
             *
             * ACK=99 is invalid because packet 99
             * has never been sent.
             */
            else
            {
                printf("Invalid/future ACK: %d\n", ack_num);
                printf("Current base remains: %d\n", base);
            }
        }
        else
        {
            /*
             * ACK could not be parsed.
             */
            printf("Invalid ACK received: %s\n", ack);
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
    printf("Next sequence number: %d\n", next_seq_num);
    printf("All %d packets acknowledged.\n", TOTAL_PACKETS);

    close(sockfd);

    return 0;
}