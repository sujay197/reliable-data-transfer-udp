
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>
#endif

#include "../channel/channel_emulator.h"

#define PORT 6026
#define TOTAL_PACKETS 10
#define WINDOW_SIZE 4

#define PACKET_LOSS_PERCENT 0
#define ACK_LOSS_PERCENT 0
#define CHANNEL_DELAY_MS 0
#define CHANNEL_SEED 12345

#ifdef _WIN32
#define close_socket_platform closesocket
#else
#define close_socket_platform close
#endif

int main(void)
{
#ifdef _WIN32
    WSADATA wsa_data;

    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
        printf("WSAStartup failed.\n");
        return 1;
    }
#endif

    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    if (sockfd < 0) {
        printf("Socket creation failed.\n");

#ifdef _WIN32
        WSACleanup();
#endif

        return 1;
    }

    struct sockaddr_in server;
    struct sockaddr_in client;

    memset(&server, 0, sizeof(server));
    memset(&client, 0, sizeof(client));

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(PORT);

    if (bind(
            sockfd,
            (struct sockaddr *)&server,
            sizeof(server)) < 0) {

#ifdef _WIN32
        printf(
            "bind failed with error: %d\n",
            WSAGetLastError()
        );
        close_socket_platform(sockfd);
        WSACleanup();
#else
        perror("bind");
        close_socket_platform(sockfd);
#endif

        return 1;
    }

#ifdef _WIN32
    int client_len = sizeof(client);
#else
    socklen_t client_len = sizeof(client);
#endif

    /*
     * Channel Emulator.
     */
    ChannelConfig channel;

    channel_init(
        &channel,
        PACKET_LOSS_PERCENT,
        ACK_LOSS_PERCENT,
        CHANNEL_DELAY_MS,
        CHANNEL_SEED
    );

    /*
     * Selective Repeat receiver state.
     */
    int received[TOTAL_PACKETS] = {0};

    int next_expected = 0;

    /*
     * Used to deliberately drop the first copy of
     * SEQ=2 for the Selective Repeat demonstration.
     */
    int dropped_seq2 = 0;

    printf("=====================================\n");
    printf("   Selective Repeat Receiver Started\n");
    printf("=====================================\n");
    printf("Listening on port: %d\n", PORT);
    printf("Window size: %d\n", WINDOW_SIZE);
    printf("Channel Emulator: ENABLED\n");
    printf("Packet loss probability: %d%%\n",
           PACKET_LOSS_PERCENT);
    printf("ACK loss probability: %d%%\n",
           ACK_LOSS_PERCENT);
    printf("Channel delay: %d ms\n",
           CHANNEL_DELAY_MS);
    printf("First copy of SEQ=2 will be dropped.\n");
    printf("=====================================\n\n");

    while (1) {

        char buffer[256];

#ifdef _WIN32
        int n = recvfrom(
            sockfd,
            buffer,
            sizeof(buffer) - 1,
            0,
            (struct sockaddr *)&client,
            &client_len
        );
#else
        int n = recvfrom(
            sockfd,
            buffer,
            sizeof(buffer) - 1,
            0,
            (struct sockaddr *)&client,
            &client_len
        );
#endif

        if (n < 0) {

#ifdef _WIN32
            printf(
                "recvfrom failed: %d\n",
                WSAGetLastError()
            );
#else
            perror("recvfrom");
#endif

            continue;
        }

        buffer[n] = '\0';

        /*
         * Transmission completion message.
         */
        if (strcmp(buffer, "END") == 0) {

            printf("\n=====================================\n");
            printf("     Transmission Complete\n");
            printf("=====================================\n");
            printf(
                "Packets delivered in order: %d/%d\n",
                next_expected,
                TOTAL_PACKETS
            );

            break;
        }

        /*
         * Apply simulated packet loss BEFORE
         * processing the packet.
         */
        if (channel_should_drop_packet(&channel)) {

            printf(
                "CHANNEL: Simulated packet loss. "
                "Packet discarded.\n\n"
            );

            continue;
        }

        /*
         * Simulated propagation delay.
         */
        channel_delay(&channel);

        /*
         * Sender format:
         *
         * SEQ=0|DATA=Packet0
         *
         * Parse exactly that format.
         */
        int seq = -1;
        char data[128];

        memset(data, 0, sizeof(data));

        int parsed = sscanf(
            buffer,
            "SEQ=%d|DATA=%127[^\n]",
            &seq,
            data
        );

        if (parsed != 2 ||
            seq < 0 ||
            seq >= TOTAL_PACKETS) {

            printf(
                "Invalid packet received: %s\n\n",
                buffer
            );

            continue;
        }

        printf(
            "Received: SEQ=%d DATA=%s\n",
            seq,
            data
        );

        /*
         * Deliberately drop the first SEQ=2.
         */
        if (seq == 2 && !dropped_seq2) {

            dropped_seq2 = 1;

            printf(
                "SIMULATED LOSS: Dropping first copy "
                "of SEQ=2; no ACK sent.\n\n"
            );

            continue;
        }

        /*
         * Check whether the packet is inside the
         * current Selective Repeat receive window.
         */
        if (
            seq < next_expected ||
            seq >= next_expected + WINDOW_SIZE) {

            /*
             * A previously received packet may be
             * retransmitted because its ACK was lost.
             *
             * Re-send its ACK when appropriate.
             */
            if (
                seq >= 0 &&
                seq < TOTAL_PACKETS &&
                received[seq]) {

                printf(
                    "Duplicate packet SEQ=%d.\n",
                    seq
                );

            } else {

                printf(
                    "Packet SEQ=%d is outside the "
                    "current receive window.\n",
                    seq
                );
            }

        } else {

            if (!received[seq]) {

                received[seq] = 1;

                printf(
                    "Packet %d accepted and buffered.\n",
                    seq
                );

            } else {

                printf(
                    "Duplicate packet SEQ=%d already "
                    "buffered.\n",
                    seq
                );
            }
        }

        /*
         * Send individual ACK.
         */
        char ack[64];

        snprintf(
            ack,
            sizeof(ack),
            "ACK=%d",
            seq
        );

        /*
         * Simulate ACK loss.
         */
        if (channel_should_drop_ack(&channel)) {

            printf(
                "CHANNEL: Simulated ACK loss for ACK=%d.\n\n",
                seq
            );

        } else {

            channel_delay(&channel);

            int sent = sendto(
                sockfd,
                ack,
                (int)strlen(ack),
                0,
                (struct sockaddr *)&client,
                client_len
            );

            if (sent < 0) {

#ifdef _WIN32
                printf(
                    "sendto ACK failed: %d\n",
                    WSAGetLastError()
                );
#else
                perror("sendto ACK");
#endif

            } else {

                printf(
                    "Sent ACK=%d\n",
                    seq
                );
            }
        }

        /*
         * Deliver all consecutive buffered packets
         * starting from next_expected.
         */
        while (
            next_expected < TOTAL_PACKETS &&
            received[next_expected]) {

            printf(
                "Delivering packet %d in order.\n",
                next_expected
            );

            next_expected++;
        }

        printf(
            "Next expected packet: %d\n\n",
            next_expected
        );

        /*
         * Once every packet has been delivered,
         * wait for END from the sender.
         */
        if (next_expected >= TOTAL_PACKETS) {

            printf(
                "All %d packets received and delivered "
                "in order.\n",
                TOTAL_PACKETS
            );

            printf(
                "Waiting for END message...\n\n"
            );
        }
    }

    close_socket_platform(sockfd);

#ifdef _WIN32
    WSACleanup();
#endif

    return 0;
}
