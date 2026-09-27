#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

int main()
{
    int sockfd;
    struct sockaddr_in server;
    char packet[] = "SEQ=0|CHK=DC2D|DATA=Hello";

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(6025);
    server.sin_addr.s_addr = inet_addr("127.0.0.1");

    for (int i = 0; i < 2; i++) {
        sendto(sockfd, packet, strlen(packet), 0,
               (struct sockaddr *)&server, sizeof(server));

        printf("Sent packet %d: %s\n", i + 1, packet);
        sleep(1);
    }

    close(sockfd);
    return 0;
}