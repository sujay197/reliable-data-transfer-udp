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

#define DEFAULT_PACKET_LOSS_PROBABILITY 25
#define DEFAULT_ACK_LOSS_PROBABILITY 25

int main(int argc, char *argv[])
{
    int sockfd;
    struct sockaddr_in sa;
    struct sockaddr_in ca;
    socklen_t ca_len;

    char packet[PACKET_SIZE];
    char ack[ACK_SIZE];

    int expected_seq = 0;
    int seq;

    int packet_loss_probability =
        DEFAULT_PACKET_LOSS_PROBABILITY;

    int ack_loss_probability =
        DEFAULT_ACK_LOSS_PROBABILITY;

    /*
     * Optional command-line arguments:
     *
     * ./receiver_gbn <packet_loss_%> <ack_loss_%>
     *
     * Example:
     *
     * ./receiver_gbn 0 0
     * ./receiver_gbn 25 0
     * ./receiver_gbn 0 25
     * ./receiver_gbn 25 25
     */
    if (argc >= 2)
    {
        packet_loss_probability = atoi(argv[1]);
    }

    if (argc >= 3)
    {
        ack_loss_probability = atoi(argv[2]);
    }

    /*
     * Validate loss probabilities.
     */
    if (packet_loss_probability < 0 ||
        packet_loss_probability > 100 ||
        ack_loss_probability < 0 ||
        ack_loss_probability > 100)
    {
        printf("Error: loss probabilities must be between 0 and 100.\n");
        return 1;
    }

    /*
     * Seed random number generator.
     */
    srand((unsigned int)time(NULL));

    /*
     * Create UDP socket.
     */
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    if (sockfd < 0)
    {
        perror("socket");
        return 1;
    }

    /*
     * Allow immediate reuse of the port.
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
     * Configure server address.
     */
    memset(&sa, 0, sizeof(sa));

    sa.sin_family = AF_INET;
    sa.sin_addr.s_addr = htonl(INADDR_ANY);
    sa.sin_port = htons(PORT);

    /*
     * Bind socket to port 6025.
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
     * Startup information.
     */
    printf("=====================================\n");
    printf("      Go-Back-N Receiver Started\n");
    printf("=====================================\n");
    printf("Listening on port: %d\n", PORT);
    printf("Packet loss probability: %d%%\n",
           packet_loss_probability);
    printf("ACK loss probability: %d%%\n\n",
           ack_loss_probability);

    /*
     * Receive packets continuously.
     */
    while (1)
    {
        memset(packet, 0, sizeof(packet));
        memset(&ca, 0, sizeof(ca));

        ca_len = sizeof(ca);

        ssize_t received =
            recvfrom(sockfd,
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
         * Extract sequence number.
         */
        if (sscanf(packet, "SEQ=%d", &seq) != 1)
        {
            printf("Invalid packet received: %s\n\n",
                   packet);
            continue;
        }

        printf("Packet sequence number: %d\n", seq);
        printf("Expected sequence number: %d\n",
               expected_seq);

        /*
         * Simulate packet loss.
         */
        int packet_loss = rand() % 100;

        if (packet_loss < packet_loss_probability)
        {
            printf("\n*** SIMULATED PACKET LOSS ***\n");
            printf("Packet discarded: %s\n", packet);
            printf("No ACK sent.\n\n");

            continue;
        }

        /*
         * Correct packet.
         */
        if (seq == expected_seq)
        {
            printf("Packet accepted: SEQ=%d\n", seq);

            /*
             * Advance receiver's expected sequence number.
             */
            expected_seq++;

            /*
             * Generate cumulative ACK.
             */
            snprintf(ack,
                     sizeof(ack),
                     "ACK=%d",
                     seq);

            printf("Generated cumulative ACK: %s\n",
                   ack);

            /*
             * Simulate ACK loss.
             */
            int ack_loss = rand() % 100;

            if (ack_loss < ack_loss_probability)
            {
                printf("\n*** SIMULATED ACK LOSS ***\n");
                printf("ACK discarded: %s\n", ack);
                printf("No ACK sent.\n\n");

                continue;
            }

            /*
             * Send ACK.
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
         * Duplicate packet.
         */
        else if (seq < expected_seq)
        {
            printf("Duplicate packet: SEQ=%d\n", seq);

            /*
             * The previous correctly received packet
             * is the latest cumulative ACK.
             */
            int previous_ack = expected_seq - 1;

            snprintf(ack,
                     sizeof(ack),
                     "ACK=%d",
                     previous_ack);

            printf("Previous cumulative ACK: %s\n",
                   ack);

            /*
             * Simulate loss of duplicate ACK.
             */
            int ack_loss = rand() % 100;

            if (ack_loss < ack_loss_probability)
            {
                printf("\n*** SIMULATED ACK LOSS ***\n");
                printf("Duplicate ACK discarded: %s\n",
                       ack);
                printf("No ACK sent.\n\n");

                continue;
            }

            /*
             * Resend previous cumulative ACK.
             */
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
         * Out-of-order packet.
         */
        else
        {
            printf("Out-of-order packet: SEQ=%d\n",
                   seq);

            printf("Expected sequence number: %d\n",
                   expected_seq);

            /*
             * In GBN, receiver does not accept
             * out-of-order packets.
             *
             * Send the most recent cumulative ACK
             * if one exists.
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
                 * Simulate ACK loss.
                 */
                int ack_loss = rand() % 100;

                if (ack_loss < ack_loss_probability)
                {
                    printf("\n*** SIMULATED ACK LOSS ***\n");
                    printf("ACK discarded: %s\n", ack);
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
                printf("No previous ACK available.\n");
            }

            printf("\n");
        }
    }

    close(sockfd);

    return 0;
}