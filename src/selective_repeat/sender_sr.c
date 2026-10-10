
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>

#define PORT 6026
#define TOTAL_PACKETS 10
#define WINDOW_SIZE 4
#define TIMEOUT_SECONDS 2

int main(void) {
    int sockfd;
    struct sockaddr_in receiver;
    socklen_t receiver_len = sizeof(receiver);
    int acked[TOTAL_PACKETS] = {0};
    int base = 0;
    int next_seq = 0;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return 1;
    }

    memset(&receiver, 0, sizeof(receiver));
    receiver.sin_family = AF_INET;
    receiver.sin_port = htons(PORT);
    receiver.sin_addr.s_addr = inet_addr("127.0.0.1");

    printf("=====================================\n");
    printf("    Selective Repeat Sender Started\n");
    printf("Window size: %d\n", WINDOW_SIZE);
    printf("Total packets: %d\n", TOTAL_PACKETS);
    printf("=====================================\n");

    while (base < TOTAL_PACKETS) {
        while (next_seq < TOTAL_PACKETS &&
               next_seq < base + WINDOW_SIZE) {
            char packet[128];
            snprintf(packet, sizeof(packet),
                     "%d|Packet%d", next_seq, next_seq);

            if (sendto(sockfd, packet, strlen(packet), 0,
                       (struct sockaddr *)&receiver,
                       receiver_len) < 0) {
                perror("sendto");
                close(sockfd);
                return 1;
            }

            printf("Sending: SEQ=%d DATA=Packet%d\n",
                   next_seq, next_seq);
            next_seq++;
        }

        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(sockfd, &readfds);

        struct timeval timeout;
        timeout.tv_sec = TIMEOUT_SECONDS;
        timeout.tv_usec = 0;

        int ready = select(sockfd + 1, &readfds, NULL, NULL, &timeout);

        if (ready < 0) {
            perror("select");
            close(sockfd);
            return 1;
        }

        if (ready == 0) {
            printf("Timeout: checking unacknowledged packets.\n");

            for (int i = base; i < next_seq; i++) {
                if (!acked[i]) {
                    char packet[128];
                    snprintf(packet, sizeof(packet),
                             "%d|Packet%d", i, i);

                    sendto(sockfd, packet, strlen(packet), 0,
                           (struct sockaddr *)&receiver,
                           receiver_len);

                    printf("Retransmitting SEQ=%d\n", i);
                }
            }
            continue;
        }

        char response[128];
        ssize_t n = recvfrom(sockfd, response,
                             sizeof(response) - 1, 0,
                             (struct sockaddr *)&receiver,
                             &receiver_len);

        if (n <= 0) {
            continue;
        }

        response[n] = '\0';

        int ack;
        if (sscanf(response, "ACK=%d", &ack) == 1 &&
            ack >= base && ack < next_seq) {
            acked[ack] = 1;
            printf("Received ACK=%d\n", ack);

            while (base < TOTAL_PACKETS && acked[base]) {
                base++;
            }

            printf("Updated base: %d\n", base);
        }
    }

    sendto(sockfd, "END", 3, 0,
           (struct sockaddr *)&receiver, receiver_len);

    printf("\nAll %d packets acknowledged!\n", TOTAL_PACKETS);
    close(sockfd);
    return 0;
}
