
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
    struct sockaddr_in server, client;
    int client_len;
    char buf[256], ack[50], data[200];
    int seq, expected_seq = 0;
    unsigned short received_checksum, calculated_checksum;

    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("WSAStartup failed\n");
        return 1;
    }

    sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sockfd == INVALID_SOCKET) {
        printf("Socket creation failed\n");
        WSACleanup();
        return 1;
    }

    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr("127.0.0.1");
    server.sin_port = htons(6025);

    if (bind(sockfd, (struct sockaddr *)&server, sizeof(server))
        == SOCKET_ERROR) {
        printf("Bind failed: %d\n", WSAGetLastError());
        closesocket(sockfd);
        WSACleanup();
        return 1;
    }

    printf("Windows duplicate receiver started on port 6025.\n");

    while (1) {
        client_len = sizeof(client);

        int received = recvfrom(sockfd, buf, sizeof(buf) - 1, 0,
                                (struct sockaddr *)&client, &client_len);

        if (received == SOCKET_ERROR) {
            printf("Receive failed: %d\n", WSAGetLastError());
            break;
        }

        buf[received] = '\0';

        if (strcmp(buf, "exit") == 0) {
            printf("Client requested exit.\n");
            break;
        }

        printf("Received: %s\n", buf);

        if (sscanf(buf, "SEQ=%d|CHK=%hx|DATA=%199[^\n]",
                   &seq, &received_checksum, data) != 3) {
            printf("Invalid packet format.\n");
            continue;
        }

        calculated_checksum = calculate_checksum(data, strlen(data));

        if (received_checksum != calculated_checksum) {
            printf("Checksum mismatch.\n");
            continue;
        }

        printf("Checksum valid.\n");

        if (seq == expected_seq) {
            printf("New packet accepted: SEQ=%d\n", seq);
            snprintf(ack, sizeof(ack), "ACK=%d", seq);

            if (sendto(sockfd, ack, (int)strlen(ack), 0,
                       (struct sockaddr *)&client, client_len)
                == SOCKET_ERROR) {
                printf("ACK send failed.\n");
                break;
            }

            printf("Sending: %s\n", ack);
            expected_seq++;
        }
        else if (seq >= 0 && seq < expected_seq) {
            printf("Duplicate packet received: SEQ=%d\n", seq);
            snprintf(ack, sizeof(ack), "ACK=%d", seq);

            if (sendto(sockfd, ack, (int)strlen(ack), 0,
                       (struct sockaddr *)&client, client_len)
                == SOCKET_ERROR) {
                printf("ACK resend failed.\n");
                break;
            }

            printf("Resending: %s\n", ack);
        }
        else {
            printf("Unexpected sequence number. Expected SEQ=%d\n",
                   expected_seq);
        }
    }

    closesocket(sockfd);
    WSACleanup();
    printf("Receiver stopped.\n");
    return 0;
}
