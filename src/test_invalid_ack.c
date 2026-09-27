#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

int main()
{
    int sockfd;
    char buf[250];
    struct sockaddr_in addr, client;
    socklen_t len = sizeof(client);

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(6025);

    bind(sockfd, (struct sockaddr *)&addr, sizeof(addr));

    int n = recvfrom(sockfd, buf, sizeof(buf) - 1, 0,
                     (struct sockaddr *)&client, &len);

    if (n > 0)
    {
        buf[n] = '\0';
        printf("Received: %s\n", buf);

        char *wrong_ack = "ACK=99";
        sendto(sockfd, wrong_ack, strlen(wrong_ack), 0,
               (struct sockaddr *)&client, len);

        printf("Sent: ACK=99\n");
    }

    close(sockfd);
    return 0;
}