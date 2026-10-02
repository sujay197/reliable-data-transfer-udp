#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <string.h>

int main()
{
    int sockfd;

    struct sockaddr_in sa;

    int base = 0;
    int next_seq_num = 0;
    int window_size = 4;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    if (sockfd < 0)
    {
        perror("socket");
        return 1;
    }

    sa.sin_family = AF_INET;
    sa.sin_addr.s_addr = inet_addr("127.0.0.1");
    sa.sin_port = htons(6025);

    printf("Go-Back-N Sender started.\n");
    printf("Window size: %d\n", window_size);
    printf("Base: %d\n", base);
    printf("Next sequence number: %d\n", next_seq_num);

    /*
     * Send packets while there is space
     * in the Go-Back-N window.
     */
    while (next_seq_num < base + window_size)
    {
        char packet[250];

        sprintf(packet,
                "SEQ=%d|DATA=Packet%d",
                next_seq_num,
                next_seq_num);

        printf("Sending: %s\n", packet);

        sendto(sockfd,
               packet,
               strlen(packet),
               0,
               (struct sockaddr *)&sa,
               sizeof(sa));

        next_seq_num++;
    }

    /*
     * Wait for ACKs.
     */
    while (base < next_seq_num)
    {
        char ack[100];
        int ack_num;

        fd_set readfds;
        struct timeval timeout;

        FD_ZERO(&readfds);
        FD_SET(sockfd, &readfds);

        /*
         * Wait for an ACK for a maximum
         * of 2 seconds.
         */
        timeout.tv_sec = 2;
        timeout.tv_usec = 0;

        int ready = select(sockfd + 1,
                           &readfds,
                           NULL,
                           NULL,
                           &timeout);

        /*
         * No ACK received within 2 seconds.
         */
        if (ready == 0)
        {
            printf("Timeout! No ACK received.\n");
            break;
        }

        /*
         * select() error.
         */
        if (ready < 0)
        {
            perror("select");
            break;
        }

        /*
         * ACK is available.
         */
        int n = recvfrom(sockfd,
                         ack,
                         sizeof(ack) - 1,
                         0,
                         NULL,
                         NULL);

        if (n < 0)
        {
            perror("recvfrom");
            break;
        }

        ack[n] = '\0';

        printf("Received: %s\n", ack);

        /*
         * Check whether the received message
         * has the format ACK=<number>.
         */
        if (sscanf(ack, "ACK=%d", &ack_num) == 1)
        {
            /*
             * Valid new ACK.
             */
            if (ack_num >= base)
            {
                base = ack_num + 1;

                printf("ACK accepted: %d\n", ack_num);
                printf("Updated base: %d\n", base);
            }
            /*
             * Old or duplicate ACK.
             */
            else
            {
                printf("Duplicate/old ACK: %d\n", ack_num);
            }
        }
        else
        {
            printf("Invalid ACK received.\n");
        }
    }

    printf("\nWindow transmission complete.\n");
    printf("Base: %d\n", base);
    printf("Next sequence number: %d\n", next_seq_num);

    close(sockfd);

    return 0;
}