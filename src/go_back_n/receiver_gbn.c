#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include <sys/types.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 6025

#define PACKET_SIZE 250
#define ACK_SIZE 250

#define PACKET_LOSS_PROBABILITY 25
#define ACK_LOSS_PROBABILITY 25

int main()
{
    int sockfd;

    struct sockaddr_in sa;
    struct sockaddr_in ca;

    socklen_t ca_len;

    char packet[PACKET_SIZE];
    char ack[ACK_SIZE];

    int expected_seq = 0;
    int seq;

    /*
     * Seed random number generator.
     *
     * This makes packet/ACK loss different on
     * different executions.
     */
    srand((unsigned int)time(NULL));

    /*
     * ------------------------------------------------
     * Create UDP socket
     * ------------------------------------------------
     */
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    if (sockfd < 0)
    {
        perror("socket");
        return 1;
    }

    /*
     * Allow quick reuse of the port after
     * stopping and restarting the receiver.
     */
    int reuse = 1;

    if (setsockopt(sockfd,
                   SOL_SOCKET,
                   SO_REUSEADDR,
                   &reuse,
                   sizeof(reuse)) < 0)
    {
        perror("setsockopt");
        close(sockfd);
        return 1;
    }

    /*
     * ------------------------------------------------
     * Configure receiver address
     * ------------------------------------------------
     */
    memset(&sa, 0, sizeof(sa));

    sa.sin_family = AF_INET;
    sa.sin_addr.s_addr = htonl(INADDR_ANY);
    sa.sin_port = htons(PORT);

    /*
     * ------------------------------------------------
     * Bind socket
     * ------------------------------------------------
     */
    if (bind(sockfd,
             (struct sockaddr *)&sa,
             sizeof(sa)) < 0)
    {
        perror("bind");
        close(sockfd);
        return 1;
    }

    /*
     * ------------------------------------------------
     * Startup information
     * ------------------------------------------------
     */
    printf("=====================================\n");
    printf("      Go-Back-N Receiver Started\n");
    printf("=====================================\n");
    printf("Listening on port: %d\n", PORT);
    printf("Packet loss probability: %d%%\n",
           PACKET_LOSS_PROBABILITY);
    printf("ACK loss probability: %d%%\n\n",
           ACK_LOSS_PROBABILITY);

    /*
     * ------------------------------------------------
     * Main receive loop
     * ------------------------------------------------
     */
    while (1)
    {
        memset(packet, 0, sizeof(packet));
        memset(&ca, 0, sizeof(ca));

        ca_len = sizeof(ca);

        /*
         * ------------------------------------------------
         * Receive packet
         * ------------------------------------------------
         */
        ssize_t received = recvfrom(sockfd,
                                    packet,
                                    sizeof(packet) - 1,
                                    0,
                                    (struct sockaddr *)&ca,
                                    &ca_len);

        if (received < 0)
        {
            perror("recvfrom");
            continue;
        }

        packet[received] = '\0';

        printf("Received: %s\n", packet);

        /*
         * ------------------------------------------------
         * Parse sequence number
         * ------------------------------------------------
         */
        if (sscanf(packet, "SEQ=%d", &seq) != 1)
        {
            printf("Invalid packet received: %s\n\n", packet);
            continue;
        }

        printf("Packet sequence number: %d\n", seq);
        printf("Expected sequence number: %d\n",
               expected_seq);

        /*
         * =================================================
         * SIMULATED PACKET LOSS
         * =================================================
         *
         * Randomly discard some incoming packets.
         *
         * No ACK is sent when a packet is discarded.
         */
        int packet_loss = rand() % 100;

        if (packet_loss < PACKET_LOSS_PROBABILITY)
        {
            printf("\n*** SIMULATED PACKET LOSS ***\n");
            printf("Packet discarded: %s\n", packet);
            printf("No ACK sent.\n\n");

            continue;
        }

        /*
         * =================================================
         * EXPECTED PACKET
         * =================================================
         */
        if (seq == expected_seq)
        {
            printf("Packet accepted: SEQ=%d\n", seq);

            /*
             * Move receiver's expected sequence number.
             */
            expected_seq++;

            /*
             * Cumulative ACK acknowledges the
             * packet that was just correctly received.
             */
            snprintf(ack,
                     sizeof(ack),
                     "ACK=%d",
                     seq);

            printf("Generated cumulative ACK: %s\n",
                   ack);

            /*
             * =================================================
             * SIMULATED ACK LOSS
             * =================================================
             *
             * Randomly discard the ACK.
             *
             * The sender will not receive this ACK and
             * will eventually trigger its timeout.
             */
            int ack_loss = rand() % 100;

            if (ack_loss < ACK_LOSS_PROBABILITY)
            {
                printf("\n*** SIMULATED ACK LOSS ***\n");
                printf("ACK discarded: %s\n", ack);
                printf("No ACK sent.\n\n");

                continue;
            }

            /*
             * ------------------------------------------------
             * Send ACK
             * ------------------------------------------------
             */
            printf("Sending cumulative ACK: %s\n",
                   ack);

            if (sendto(sockfd,
                        ack,
                        strlen(ack),
                        0,
                        (struct sockaddr *)&ca,
                        ca_len) < 0)
            {
                perror("sendto");
                continue;
            }

            printf("\n");
        }

        /*
         * =================================================
         * DUPLICATE PACKET
         * =================================================
         *
         * Example:
         *
         * expected_seq = 5
         * received SEQ=4
         *
         * Packet 4 was already accepted.
         *
         * We resend ACK=4.
         */
        else if (seq < expected_seq)
        {
            printf("Duplicate packet: SEQ=%d\n",
                   seq);

            /*
             * The latest correctly received packet is:
             */
            int previous_ack = expected_seq - 1;

            snprintf(ack,
                     sizeof(ack),
                     "ACK=%d",
                     previous_ack);

            printf("Previous cumulative ACK: %s\n",
                   ack);

            /*
             * Apply ACK loss simulation to duplicate ACKs too.
             */
            int ack_loss = rand() % 100;

            if (ack_loss < ACK_LOSS_PROBABILITY)
            {
                printf("\n*** SIMULATED ACK LOSS ***\n");
                printf("Duplicate ACK discarded: %s\n",
                       ack);
                printf("No ACK sent.\n\n");

                continue;
            }

            printf("Resending previous ACK: %s\n",
                   ack);

            if (sendto(sockfd,
                        ack,
                        strlen(ack),
                        0,
                        (struct sockaddr *)&ca,
                        ca_len) < 0)
            {
                perror("sendto");
                continue;
            }

            printf("\n");
        }

        /*
         * =================================================
         * OUT-OF-ORDER PACKET
         * =================================================
         *
         * Example:
         *
         * expected_seq = 5
         * received SEQ=7
         *
         * Packet 5 or 6 is missing.
         *
         * Go-Back-N receiver does NOT buffer packet 7.
         * It sends the ACK for the last correctly
         * received packet.
         */
        else
        {
            printf("Out-of-order packet: SEQ=%d\n",
                   seq);

            printf("Expected sequence number: %d\n",
                   expected_seq);

            /*
             * Send ACK for the last correctly
             * received packet.
             */
            if (expected_seq > 0)
            {
                int previous_ack = expected_seq - 1;

                snprintf(ack,
                         sizeof(ack),
                         "ACK=%d",
                         previous_ack);

                printf("Sending previous cumulative ACK: %s\n",
                       ack);

                /*
                 * Apply ACK loss simulation.
                 */
                int ack_loss = rand() % 100;

                if (ack_loss < ACK_LOSS_PROBABILITY)
                {
                    printf("\n*** SIMULATED ACK LOSS ***\n");
                    printf("ACK discarded: %s\n",
                           ack);
                    printf("No ACK sent.\n\n");

                    continue;
                }

                if (sendto(sockfd,
                            ack,
                            strlen(ack),
                            0,
                            (struct sockaddr *)&ca,
                            ca_len) < 0)
                {
                    perror("sendto");
                    continue;
                }
            }
            else
            {
                /*
                 * No packet has been correctly received yet,
                 * so there is no previous ACK to send.
                 */
                printf("No previous ACK available.\n");
            }

            printf("\n");
        }
    }

    /*
     * This is normally unreachable because the receiver
     * continuously waits for packets.
     */
    close(sockfd);

    return 0;
}