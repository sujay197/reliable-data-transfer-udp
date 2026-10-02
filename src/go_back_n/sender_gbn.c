#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
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
    char packet[250];

while (next_seq_num < base + window_size)
{
    sprintf(packet, "SEQ=%d|DATA=Packet%d",
            next_seq_num, next_seq_num);

    printf("Sending: %s\n", packet);

    sendto(sockfd, packet, strlen(packet), 0,
           (struct sockaddr *)&sa, sizeof(sa));

    next_seq_num++;
}
/* Receive ACKs */
while (base < next_seq_num)
{
    char ack[100];
    int ack_num;

    int n = recvfrom(sockfd, ack, sizeof(ack) - 1, 0,
                     NULL, NULL);

    if (n < 0)
    {
        perror("recvfrom");
        break;
    }

    ack[n] = '\0';

    printf("Received: %s\n", ack);

    if (sscanf(ack, "ACK=%d", &ack_num) == 1)
    {
        if (ack_num >= base)
        {
            base = ack_num + 1;

            printf("ACK accepted: %d\n", ack_num);
            printf("Updated base: %d\n", base);
        }
        else
        {
            printf("Duplicate/old ACK: %d\n", ack_num);
        }
    }
}

printf("Window transmission complete.\n");
printf("Base: %d\n", base);
printf("Next sequence number: %d\n", next_seq_num);

    close(sockfd);

    return 0;
}