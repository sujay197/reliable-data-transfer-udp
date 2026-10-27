
#include <stdio.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>

unsigned short calculate_checksum(const char *data, size_t length)
{
    unsigned long sum = 0;
    const unsigned char *ptr = (const unsigned char *)data;

    while (length > 1) {
        sum += ((unsigned short)ptr[0] << 8) | ptr[1];
        ptr += 2;
        length -= 2;
    }

    if (length > 0)
        sum += (unsigned short)ptr[0] << 8;

    while (sum >> 16)
        sum = (sum & 0xFFFF) + (sum >> 16);

    return (unsigned short)(~sum);
}

int main(void)
{
    WSADATA wsa;
    SOCKET sockfd;
    struct sockaddr_in server;
    int server_len = sizeof(server);
    char packet[256], response[100];
    const char data[] = "Hello";
    int passed = 1;

    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("FAIL: WSAStartup\n");
        return 1;
    }

    sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sockfd == INVALID_SOCKET) {
        printf("FAIL: socket creation\n");
        WSACleanup();
        return 1;
    }

    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(6025);
    server.sin_addr.s_addr = inet_addr("127.0.0.1");

    DWORD timeout_ms = 3000;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO,
               (const char *)&timeout_ms, sizeof(timeout_ms));

    unsigned short checksum = calculate_checksum(data, strlen(data));
    snprintf(packet, sizeof(packet), "SEQ=0|CHK=%04X|DATA=%s",
             checksum, data);

    for (int i = 1; i <= 2; i++) {
        if (sendto(sockfd, packet, (int)strlen(packet), 0,
                   (struct sockaddr *)&server, sizeof(server)) == SOCKET_ERROR) {
            printf("FAIL: send packet %d\n", i);
            passed = 0;
            break;
        }

        printf("Sent packet %d: %s\n", i, packet);

        int received = recvfrom(sockfd, response, sizeof(response) - 1, 0,
                                (struct sockaddr *)&server, &server_len);

        if (received == SOCKET_ERROR) {
            printf("FAIL: no ACK for packet %d (error %d)\n",
                   i, WSAGetLastError());
            passed = 0;
            break;
        }

        response[received] = '\0';
        printf("Received: %s\n", response);

        if (strcmp(response, "ACK=0") != 0) {
            printf("FAIL: expected ACK=0 for packet %d\n", i);
            passed = 0;
            break;
        }

        printf("PASS: ACK=0 received for packet %d\n", i);
    }

    closesocket(sockfd);
    WSACleanup();

    if (passed) {
        printf("\nPASS: Receiver acknowledged both original and duplicate packets.\n");
        return 0;
    }

    printf("\nFAIL: Duplicate packet behavior test failed.\n");
    return 1;
}
