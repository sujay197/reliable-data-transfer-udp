#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

int main()
{
    int sockfd, length, i;
    char buf[100];
    char ack[50];

    int seq;

    struct sockaddr_in sa, ca;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    sa.sin_family = AF_INET;
    sa.sin_addr.s_addr = inet_addr("127.0.0.1");
    sa.sin_port = htons(6025);

    i = bind(sockfd, (struct sockaddr *)&sa, sizeof(sa));

    printf("Server started.\n");

    while (1)
    {
        length = sizeof(ca);

        int k = recvfrom(sockfd, buf, 100, 0,
                         (struct sockaddr *)&ca, &length);

        buf[k] = '\0';

        if (strcmp(buf, "exit") == 0)
        {
            printf("Client requested exit.\n");
            break;
        }

        printf("Received: %s\n", buf);

        sscanf(buf, "SEQ=%d", &seq);

        printf("Received packet with sequence number: %d\n", seq);

        sprintf(ack, "ACK=%d", seq);

        printf("Sending: %s\n", ack);

        sendto(sockfd, ack, strlen(ack), 0,
               (struct sockaddr *)&ca, sizeof(ca));
    }

    close(sockfd);

    printf("Server exited.\n");

    return 0;
}