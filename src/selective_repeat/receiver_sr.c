
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 6026
#define TOTAL_PACKETS 10
#define WINDOW_SIZE 4
#define BUFFER_SIZE 1024

int main(void)
{
    int sockfd;
    struct sockaddr_in server, client;
    socklen_t client_len;

    char buffer[BUFFER_SIZE];
    int expected = 0;
    int received[TOTAL_PACKETS] = {0};

    /* Test configuration: drop the first arrival of frame 2. */
    int dropped_packet_2 = 0;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return 1;
    }

    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
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
    printf("Window size: %d\n", WINDOW_SIZE);
    printf("First copy of SEQ=2 will be dropped.\n");
    printf("=====================================\n");

    while (1) {
        client_len = sizeof(client);

        ssize_t n = recvfrom(sockfd, buffer,
                             sizeof(buffer) - 1, 0,
                             (struct sockaddr *)&client,
                             &client_len);

        if (n < 0) {
            perror("recvfrom");
            continue;
        }

        buffer[n] = '\0';

        if (strcmp(buffer, "END") == 0) {
            if (expected == TOTAL_PACKETS) {
                printf("All packets delivered. Transmission finished.\n");
            } else {
                printf("END received, but delivery is incomplete: "
                       "%d of %d packets delivered.\n",
                       expected, TOTAL_PACKETS);
            }
            break;
        }

        int seq;
        char data[BUFFER_SIZE];

        if (sscanf(buffer, "%d|%1023[^\n]", &seq, data) != 2) {
            printf("Invalid packet received: %s\n", buffer);
            continue;
        }

        printf("Received: SEQ=%d DATA=%s\n", seq, data);

        if (seq < 0 || seq >= TOTAL_PACKETS) {
            printf("Sequence number out of range.\n");
            continue;
        }

        /*
         * Simulate one lost data packet. No ACK is sent for this
         * attempt, so the sender's timer for SEQ=2 will expire.
         */
        if (seq == 2 && !dropped_packet_2) {
            dropped_packet_2 = 1;
            printf("SIMULATED LOSS: Dropping first copy of SEQ=2; "
                   "no ACK sent.\n");
            continue;
        }

        /*
         * Accept packets only within the current receiver window.
         * Already-delivered packets are re-ACKed to support recovery
         * if their ACKs were lost.
         */
        if (seq < expected) {
            printf("Duplicate of previously delivered SEQ=%d.\n", seq);
        } else if (seq >= expected + WINDOW_SIZE) {
            printf("SEQ=%d outside receiver window [%d, %d].\n",
                   seq, expected, expected + WINDOW_SIZE - 1);
            continue;
        } else if (!received[seq]) {
            received[seq] = 1;
            printf("Packet %d accepted and buffered.\n", seq);
        } else {
            printf("Duplicate packet %d received.\n", seq);
        }

        char ack[32];
        snprintf(ack, sizeof(ack), "ACK=%d", seq);

        if (sendto(sockfd, ack, strlen(ack), 0,
                   (struct sockaddr *)&client, client_len) < 0) {
            perror("sendto ACK");
            continue;
        }

        printf("Sent %s\n", ack);

        /*
         * Deliver only consecutive packets starting at expected.
         * Out-of-order packets remain buffered until the gap closes.
         */
        while (expected < TOTAL_PACKETS && received[expected]) {
            printf("Delivering packet %d in order.\n", expected);
            expected++;
        }
    }

    close(sockfd);
    return 0;
}
