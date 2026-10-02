#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>

#define PORT 6025
#define BUFFER_SIZE 250
#define PACKET_LOSS_PERCENT 25

int main()
{
    int sockfd;

    struct sockaddr_in sa;
    struct sockaddr_in ca;

    socklen_t ca_len;

    char buffer[BUFFER_SIZE];
    char ack[BUFFER_SIZE];

    int expected_seq = 0;

    /*
     * Used to make sure ACK=5 is lost only once.
     */
    int ack_loss_done = 0;

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
     * Allow the port to be reused quickly.
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
     * Receiver address.
     */
    memset(&sa, 0, sizeof(sa));

    sa.sin_family = AF_INET;
    sa.sin_addr.s_addr = INADDR_ANY;
    sa.sin_port = htons(PORT);

    /*
     * Bind receiver to port 6025.
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
     * Seed random number generator.
     */
    srand((unsigned int)time(NULL));

    printf("========================================\n");
    printf("       Go-Back-N Receiver Started\n");
    printf("========================================\n");

    printf("Port: %d\n", PORT);
    printf("Packet loss probability: %d%%\n", PACKET_LOSS_PERCENT);
    printf("ACK loss test: ACK=5 will be lost once.\n");
    printf("Expected sequence number: %d\n", expected_seq);
    printf("========================================\n\n");

    while (1)
    {
        int n;

        memset(buffer, 0, sizeof(buffer));
        memset(ack, 0, sizeof(ack));

        ca_len = sizeof(ca);

        /*
         * Wait for a packet.
         */
        n = recvfrom(
            sockfd,
            buffer,
            sizeof(buffer) - 1,
            0,
            (struct sockaddr *)&ca,
            &ca_len
        );

        if (n < 0)
        {
            perror("recvfrom");
            continue;
        }

        buffer[n] = '\0';

        printf("Received: %s\n", buffer);

        /*
         * Extract sequence number.
         *
         * Expected packet format:
         *
         * SEQ=0|DATA=Packet0
         */
        int seq;

        if (sscanf(buffer, "SEQ=%d", &seq) != 1)
        {
            printf("Invalid packet format.\n\n");
            continue;
        }

        printf("Packet sequence number: %d\n", seq);
        printf("Expected sequence number: %d\n",
               expected_seq);

        /*
         * ====================================================
         * PACKET LOSS SIMULATION
         * ====================================================
         *
         * Randomly discard approximately 25% of packets.
         *
         * If a packet is discarded:
         * - It is not processed.
         * - No ACK is sent.
         * - Sender must eventually timeout and retransmit.
         */
        int loss = rand() % 100;

        if (loss < PACKET_LOSS_PERCENT)
        {
            printf("\n*** SIMULATED PACKET LOSS ***\n");
            printf("Packet discarded: %s\n", buffer);
            printf("No ACK sent.\n\n");

            continue;
        }

        /*
         * ====================================================
         * EXPECTED PACKET
         * ====================================================
         */
        if (seq == expected_seq)
        {
            printf("Packet accepted: SEQ=%d\n", seq);

            /*
             * The packet was successfully received.
             *
             * Move expected sequence number forward.
             */
            expected_seq++;

            /*
             * ====================================================
             * ACK LOSS SIMULATION
             * ====================================================
             *
             * Deliberately lose ACK=5 once.
             *
             * The receiver has already accepted the packet and
             * advanced expected_seq.
             *
             * The sender will therefore timeout and retransmit
             * SEQ=5.
             */
            if (seq == 5 && ack_loss_done == 0)
            {
                printf("\n");
                printf("*** SIMULATED ACK LOSS ***\n");
                printf("ACK=5 discarded.\n");
                printf("No ACK sent.\n\n");

                ack_loss_done = 1;

                continue;
            }

            /*
             * Create cumulative ACK.
             */
            sprintf(ack, "ACK=%d", seq);

            printf("Sending cumulative ACK: %s\n",
                   ack);

            /*
             * Send ACK to sender.
             */
            sendto(
                sockfd,
                ack,
                strlen(ack),
                0,
                (struct sockaddr *)&ca,
                ca_len
            );

            printf("\n");
        }

        /*
         * ====================================================
         * DUPLICATE / OLD PACKET
         * ====================================================
         *
         * This happens when:
         *
         * 1. Sender's ACK was lost.
         * 2. Sender times out.
         * 3. Sender retransmits the packet.
         *
         * The receiver has already accepted it, so seq will be
         * smaller than expected_seq.
         */
        else if (seq < expected_seq)
        {
            printf("Duplicate packet: SEQ=%d\n", seq);

            /*
             * Send ACK for the most recently accepted packet.
             */
            sprintf(ack, "ACK=%d", expected_seq - 1);

            printf("Resending ACK: %s\n", ack);

            sendto(
                sockfd,
                ack,
                strlen(ack),
                0,
                (struct sockaddr *)&ca,
                ca_len
            );

            printf("\n");
        }

        /*
         * ====================================================
         * OUT-OF-ORDER PACKET
         * ====================================================
         *
         * Go-Back-N receiver accepts only the expected packet.
         *
         * If a packet arrives with a sequence number greater
         * than expected_seq, it is out of order.
         */
        else
        {
            printf("Out-of-order packet: SEQ=%d\n",
                   seq);

            /*
             * Send ACK for the previous correctly received
             * packet.
             */
            sprintf(ack, "ACK=%d", expected_seq - 1);

            printf("Sending previous ACK: %s\n",
                   ack);

            sendto(
                sockfd,
                ack,
                strlen(ack),
                0,
                (struct sockaddr *)&ca,
                ca_len
            );

            printf("\n");
        }
    }

    close(sockfd);

    return 0;
}