
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 6026
#define BUFFER_SIZE 1024

int main(void) {
    int sockfd;
    struct sockaddr_in server, client;
    socklen_t client_len = sizeof(client);
    char buffer[BUFFER_SIZE];
    int expected = 0;
    int received[100] = {0};

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return 1;
    }

    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(PORT);

    if (bind(sockfd, (struct sockaddr *)&server,
             sizeof(server)) < 0) {
        perror("bind");
        close(sockfd);
        return 1;
    }

    printf("=====================================\n");
    printf("   Selective Repeat Receiver Started\n");
    printf("Listening on port %d\n", PORT);
    printf("=====================================\n");

    while (1) {
        ssize_t n = recvfrom(sockfd, buffer, sizeof(buffer) - 1, 0,
                            (struct sockaddr *)&client, &client_len);

        if (n < 0) {
            perror("recvfrom");
            continue;
        }

        buffer[n] = '\0';

        if (strcmp(buffer, "END") == 0) {
            printf("Transmission finished.\n");
            break;
        }

        int seq;
        char data[BUFFER_SIZE];

        if (sscanf(buffer, "%d|%1023[^\n]", &seq, data) != 2) {
            printf("Invalid packet received: %s\n", buffer);
            continue;
        }

        printf("Received: SEQ=%d DATA=%s\n", seq, data);

        if (seq >= 0 && seq < 100) {
            if (!received[seq]) {
                received[seq] = 1;
                printf("Packet %d accepted and buffered.\n", seq);
            } else {
                printf("Duplicate packet %d received.\n", seq);
            }

            char ack[32];
            snprintf(ack, sizeof(ack), "ACK=%d", seq);

            sendto(sockfd, ack, strlen(ack), 0,
                   (struct sockaddr *)&client, client_len);
            printf("Sent %s\n", ack);

            while (expected < 100 && received[expected]) {
                printf("Delivering packet %d in order.\n", expected);
                expected++;
            }
        } else {
            printf("Sequence number out of range.\n");
        }
    }

    close(sockfd);
    return 0;
}

