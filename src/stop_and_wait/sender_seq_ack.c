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
    int sockfd, length;
    char buf[100], buf1[100];
    char packet[200];
    struct sockaddr_in sa;

    int seq = 0;

    while (1)
    {
        sockfd = socket(AF_INET, SOCK_DGRAM, 0);

        sa.sin_family = AF_INET;
        sa.sin_addr.s_addr = inet_addr("127.0.0.1");
        sa.sin_port = htons(6025);

        printf("Enter a message: ");
        gets(buf);

        if (strcmp(buf, "exit") == 0)
        {
            sendto(sockfd, buf, strlen(buf), 0,
                   (struct sockaddr *)&sa, sizeof(sa));

            close(sockfd);
            break;
        }

        sprintf(packet, "SEQ=%d|%s", seq, buf);

        printf("Sending: %s\n", packet);

        sendto(sockfd, packet, strlen(packet), 0,
               (struct sockaddr *)&sa, sizeof(sa));

        length = sizeof(sa);

        int k = recvfrom(sockfd, buf1, 100, 0,
                         (struct sockaddr *)&sa, &length);

        buf1[k] = '\0';

        printf("Received: %s\n", buf1);

        close(sockfd);

        seq++;
    }

    printf("Client exited.\n");

    return 0;
}