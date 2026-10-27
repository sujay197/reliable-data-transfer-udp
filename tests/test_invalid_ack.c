#include <stdio.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>

int main(void)
{
    WSADATA wsa;
    SOCKET sockfd;
    char buf[250];
    struct sockaddr_in addr, client;
    int len;
    int count = 0;

    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("FAIL: WSAStartup failed\n");
        return 1;
    }

    sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sockfd == INVALID_SOCKET) {
        printf("FAIL: socket creation failed\n");
        WSACleanup();
        return 1;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(6025);

    if (bind(sockfd, (struct sockaddr *)&addr, sizeof(addr))
        == SOCKET_ERROR) {
        printf("FAIL: bind error %d\n", WSAGetLastError());
        closesocket(sockfd);
        WSACleanup();
        return 1;
    }

    printf("Invalid-ACK test receiver running on port 6025.\n");
    printf("It will reply ACK=99 to five packets.\n");

    while (count < 5) {
        len = sizeof(client);

        int n = recvfrom(sockfd, buf, sizeof(buf) - 1, 0,
                         (struct sockaddr *)&client, &len);

        if (n == SOCKET_ERROR) {
            printf("FAIL: recvfrom error %d\n", WSAGetLastError());
            closesocket(sockfd);
            WSACleanup();
            return 1;
        }

        buf[n] = '\0';
        printf("Received packet %d: %s\n", count + 1, buf);

        const char *wrong_ack = "ACK=99";
        int sent = sendto(sockfd, wrong_ack, (int)strlen(wrong_ack), 0,
                          (struct sockaddr *)&client, len);

        if (sent == SOCKET_ERROR) {
            printf("FAIL: sendto error %d\n", WSAGetLastError());
            closesocket(sockfd);
            WSACleanup();
            return 1;
        }

        printf("Sent invalid ACK: %s\n", wrong_ack);
        count++;
    }

    printf("PASS: Sent invalid ACKs for all five packets received.\n");

    closesocket(sockfd);
    WSACleanup();
    return 0;
}
