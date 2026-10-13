
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <errno.h>

#define PORT 6026
#define TOTAL_PACKETS 10
#define WINDOW_SIZE 4
#define TIMEOUT_MS 2000

static long long current_time_ms(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);

    return (long long)tv.tv_sec * 1000LL +
           tv.tv_usec / 1000;
}

static int send_packet(int sockfd,
                       struct sockaddr_in *receiver,
                       socklen_t receiver_len,
                       int seq,
                       long long sent_at[],
                       int retransmission)
{
    char packet[128];

    snprintf(packet, sizeof(packet), "%d|Packet%d", seq, seq);

    ssize_t n = sendto(sockfd, packet, strlen(packet), 0,
                       (struct sockaddr *)receiver,
                       receiver_len);

    if (n < 0) {
        perror("sendto");
        return -1;
    }

    sent_at[seq] = current_time_ms();

    if (retransmission) {
        printf("Retransmitting SEQ=%d\n", seq);
    } else {
        printf("Sending: SEQ=%d DATA=Packet%d\n", seq, seq);
    }

    return 0;
}

int main(void)
{
    int sockfd;
    struct sockaddr_in receiver;
    socklen_t receiver_len = sizeof(receiver);

    int acked[TOTAL_PACKETS] = {0};
    long long sent_at[TOTAL_PACKETS] = {0};

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
    printf("Timeout: %d ms per packet\n", TIMEOUT_MS);
    printf("=====================================\n");

    while (base < TOTAL_PACKETS) {
        /* Fill the sending window. */
        while (next_seq < TOTAL_PACKETS &&
               next_seq < base + WINDOW_SIZE) {
            if (send_packet(sockfd, &receiver, receiver_len,
                            next_seq, sent_at, 0) < 0) {
                close(sockfd);
                return 1;
            }

            next_seq++;
        }

        /*
         * Calculate the nearest deadline among all outstanding
         * packets. Acknowledged packets have no active timer.
         */
        long long now = current_time_ms();
        long long nearest_deadline = -1;

        for (int i = base; i < next_seq; i++) {
            if (!acked[i]) {
                long long deadline = sent_at[i] + TIMEOUT_MS;

                if (nearest_deadline < 0 ||
                    deadline < nearest_deadline) {
                    nearest_deadline = deadline;
                }
            }
        }

        if (nearest_deadline < 0) {
            continue;
        }

        long long remaining = nearest_deadline - now;
        if (remaining < 0) {
            remaining = 0;
        }

        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(sockfd, &readfds);

        struct timeval timeout;
        timeout.tv_sec = (time_t)(remaining / 1000);
        timeout.tv_usec = (suseconds_t)((remaining % 1000) * 1000);

        int ready = select(sockfd + 1, &readfds, NULL, NULL, &timeout);

        if (ready < 0) {
            if (errno == EINTR) {
                continue;
            }

            perror("select");
            close(sockfd);
            return 1;
        }

        if (ready == 0) {
            /*
             * Retransmit only packets whose own timers expired.
             */
            now = current_time_ms();

            for (int i = base; i < next_seq; i++) {
                if (!acked[i] &&
                    now - sent_at[i] >= TIMEOUT_MS) {
                    printf("Timeout for SEQ=%d\n", i);

                    if (send_packet(sockfd, &receiver, receiver_len,
                                    i, sent_at, 1) < 0) {
                        close(sockfd);
                        return 1;
                    }
                }
            }

            continue;
        }

        char response[128];
        struct sockaddr_in ack_source;
        socklen_t ack_source_len = sizeof(ack_source);

        ssize_t n = recvfrom(sockfd, response,
                             sizeof(response) - 1, 0,
                             (struct sockaddr *)&ack_source,
                             &ack_source_len);

        if (n < 0) {
            if (errno == EINTR) {
                continue;
            }

            perror("recvfrom");
            close(sockfd);
            return 1;
        }

        response[n] = '\0';

        int ack;
        if (sscanf(response, "ACK=%d", &ack) == 1 &&
            ack >= 0 && ack < TOTAL_PACKETS &&
            ack >= base && ack < next_seq) {

            if (!acked[ack]) {
                acked[ack] = 1;
                printf("Received ACK=%d\n", ack);
            } else {
                printf("Duplicate ACK=%d\n", ack);
            }

            /*
             * Advance the window only across consecutive ACKs.
             */
            while (base < TOTAL_PACKETS && acked[base]) {
                base++;
            }

            printf("Updated base: %d\n", base);
        } else {
            printf("Ignoring invalid or out-of-window response: %s\n",
                   response);
        }
    }

    /*
     * Send END only after every data packet has been acknowledged.
     */
    if (sendto(sockfd, "END", 3, 0,
               (struct sockaddr *)&receiver,
               receiver_len) < 0) {
        perror("sendto END");
        close(sockfd);
        return 1;
    }

    printf("\nAll %d packets acknowledged!\n", TOTAL_PACKETS);

    close(sockfd);
    return 0;
}
