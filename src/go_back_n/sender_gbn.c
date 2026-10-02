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

    /*
     * Create UDP socket
     */
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    if (sockfd < 0)
    {
        perror("socket");
        return 1;
    }

    /*
     * Set receiver address
     */
    memset(&sa, 0, sizeof(sa));

    sa.sin_family = AF_INET;
    sa.sin_addr.s_addr = inet_addr("127.0.0.1");
    sa.sin_port = htons(PORT);

    /*
     * Set receive timeout.
     *
     * If an ACK does not arrive within TIMEOUT_SEC,
     * recvfrom() returns with an error.
     */
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

    printf("=====================================\n");
    printf("      Go-Back-N Sender Started\n");
    printf("=====================================\n");
    printf("Window size: %d\n", WINDOW_SIZE);
    printf("Total packets: %d\n", TOTAL_PACKETS);
    printf("Packet loss handling: ENABLED\n");
    printf("Timeout: %d seconds\n", TIMEOUT_SEC);
    printf("Receiver: 127.0.0.1:%d\n\n", PORT);

    /*
     * Main Go-Back-N transmission loop
     */
    while (base < TOTAL_PACKETS)
    {
        /*
         * ------------------------------------------------
         * STEP 1: Send packets while window has space
         * ------------------------------------------------
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
         * ------------------------------------------------
         * STEP 2: Wait for ACK
         * ------------------------------------------------
         */
        memset(ack, 0, sizeof(ack));

        ssize_t received = recvfrom(sockfd,
                                    ack,
                                    sizeof(ack) - 1,
                                    0,
                                    NULL,
                                    NULL);

        /*
         * ------------------------------------------------
         * STEP 3: Timeout occurred
         * ------------------------------------------------
         */
        if (received < 0)
        {
            printf("\n*** TIMEOUT ***\n");
            printf("No ACK received for %d seconds.\n", TIMEOUT_SEC);
            printf("Base = %d\n", base);
            printf("Retransmitting unacknowledged packets...\n\n");

            /*
             * Go-Back-N:
             * retransmit EVERY packet from base
             * up to next_seq_num - 1.
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
         * Make ACK a proper C string
         */
        ack[received] = '\0';

        printf("Received: %s\n", ack);

        /*
         * ------------------------------------------------
         * STEP 4: Parse ACK
         * ------------------------------------------------
         */
        if (sscanf(ack, "ACK=%d", &ack_num) == 1)
        {
            printf("ACK number: %d\n", ack_num);

            /*
             * ACK is valid if it acknowledges
             * something at or beyond the current base.
             *
             * Example:
             *
             * base = 2
             * ACK=3
             *
             * Then packets 2 and 3 are acknowledged.
             */
            if (ack_num >= base)
            {
                base = ack_num + 1;

                printf("ACK accepted: %d\n", ack_num);
                printf("Updated base: %d\n", base);
            }
            else
            {
                /*
                 * Duplicate/old ACK.
                 */
                printf("Duplicate/old ACK: %d\n", ack_num);
                printf("Current base remains: %d\n", base);
            }
        }
        else
        {
            printf("Invalid ACK received: %s\n", ack);
        }

        printf("\n");
    }

    /*
     * ------------------------------------------------
     * Transmission completed
     * ------------------------------------------------
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