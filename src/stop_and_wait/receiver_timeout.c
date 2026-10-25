#include "../channel/channel_emulator.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#ifdef _WIN32

#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>

#else

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#endif

#define PORT 6025
#define BUFFER_SIZE 1024

#define PACKET_LOSS_PERCENT 0
#define ACK_LOSS_PERCENT 0
#define CHANNEL_DELAY_MS 0
#define CHANNEL_SEED 12345

static unsigned short calculate_checksum(const char *data)
{
    unsigned long sum = 0;

    while (*data) {
        sum += (unsigned char)*data;
        data++;
    }

    return (unsigned short)(sum & 0xFFFF);
}

int main(void)
{
#ifdef _WIN32

    WSADATA wsa;

    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("WSAStartup failed\n");
        return 1;
    }

#endif

#ifdef _WIN32

    SOCKET sock = socket(
        AF_INET,
        SOCK_DGRAM,
        IPPROTO_UDP
    );

    if (sock == INVALID_SOCKET) {
        printf(
            "Socket creation failed: %d\n",
            WSAGetLastError()
        );

        WSACleanup();
        return 1;
    }

#else

    int sock = socket(
        AF_INET,
        SOCK_DGRAM,
        0
    );

    if (sock < 0) {
        perror("Socket creation failed");
        return 1;
    }

#endif

    struct sockaddr_in server_addr;

    memset(
        &server_addr,
        0,
        sizeof(server_addr)
    );

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(PORT);

    if (bind(
            sock,
            (struct sockaddr *)&server_addr,
            sizeof(server_addr)) < 0) {

#ifdef _WIN32

        printf(
            "Bind failed: %d\n",
            WSAGetLastError()
        );

        closesocket(sock);
        WSACleanup();

#else

        perror("Bind failed");
        close(sock);

#endif

        return 1;
    }

    ChannelConfig channel;

    channel_init(
        &channel,
        PACKET_LOSS_PERCENT,
        ACK_LOSS_PERCENT,
        CHANNEL_DELAY_MS,
        CHANNEL_SEED
    );

    printf("Stop-and-Wait Receiver Started\n");
    printf("Listening on port: %d\n", PORT);
    printf("Adaptive RTO is handled by the sender.\n");
    printf("Channel Emulator: ENABLED\n");
    printf(
        "Packet loss: %d%%\n",
        PACKET_LOSS_PERCENT
    );
    printf(
        "ACK loss: %d%%\n",
        ACK_LOSS_PERCENT
    );
    printf(
        "Channel delay: %d ms\n",
        CHANNEL_DELAY_MS
    );
    printf("Waiting for packets...\n\n");

    int expected_sequence = 0;

    while (1) {

        char buffer[BUFFER_SIZE];

        struct sockaddr_in client_addr;

#ifdef _WIN32

        int client_len = sizeof(client_addr);

#else

        socklen_t client_len =
            sizeof(client_addr);

#endif

        int received = recvfrom(
            sock,
            buffer,
            sizeof(buffer) - 1,
            0,
            (struct sockaddr *)&client_addr,
            &client_len
        );

        if (received < 0) {

#ifdef _WIN32

            printf(
                "recvfrom failed: %d\n",
                WSAGetLastError()
            );

#else

            perror("recvfrom failed");

#endif

            continue;
        }

        buffer[received] = '\0';

        printf(
            "Received: %s\n",
            buffer
        );

        /*
         * Simulate packet loss at the receiver.
         *
         * The sender may retransmit the same packet
         * when this happens.
         */
        if (channel_should_drop_packet(
                &channel)) {

            printf(
                "CHANNEL EMULATOR: "
                "Dropping received packet\n\n"
            );

            continue;
        }

        /*
         * Apply configurable channel delay.
         */
        channel_delay(&channel);

        int sequence_number = -1;
        unsigned int received_checksum = 0;
        char data[BUFFER_SIZE];

        memset(
            data,
            0,
            sizeof(data)
        );

        int parsed = sscanf(
            buffer,
            "SEQ=%d|CHK=%x|DATA=%[^\n]",
            &sequence_number,
            &received_checksum,
            data
        );

        if (parsed != 3) {

            printf(
                "Invalid packet format.\n\n"
            );

            continue;
        }

        printf(
            "Received packet with sequence number: %d\n",
            sequence_number
        );

        unsigned short calculated_checksum =
            calculate_checksum(data);

        if (calculated_checksum !=
            (unsigned short)received_checksum) {

            printf(
                "Checksum mismatch. "
                "Packet corrupted.\n\n"
            );

            continue;
        }

        printf("Checksum valid.\n");

        if (sequence_number == expected_sequence) {

            printf(
                "Accepted SEQ=%d\n",
                sequence_number
            );

            printf(
                "Delivering DATA: %s\n",
                data
            );

            char ack[64];

            snprintf(
                ack,
                sizeof(ack),
                "ACK=%d",
                sequence_number
            );

            /*
             * Simulate ACK loss before sending.
             */
            if (channel_should_drop_ack(
                    &channel)) {

                printf(
                    "CHANNEL EMULATOR: "
                    "Dropping ACK=%d\n\n",
                    sequence_number
                );

                /*
                 * The packet was accepted and delivered,
                 * so advance the expected sequence.
                 *
                 * The sender will timeout and retransmit.
                 * The duplicate handling below will then
                 * resend the ACK.
                 */
                expected_sequence++;

                continue;
            }

            channel_delay(&channel);

            sendto(
                sock,
                ack,
                (int)strlen(ack),
                0,
                (struct sockaddr *)&client_addr,
                client_len
            );

            printf(
                "Sent: %s\n",
                ack
            );

            expected_sequence++;

        } else if (sequence_number < expected_sequence) {

            /*
             * Duplicate packet.
             *
             * This happens when the original ACK was lost.
             * Re-send the ACK so the sender can stop
             * retransmitting.
             */

            printf(
                "Duplicate SEQ=%d received.\n",
                sequence_number
            );

            char ack[64];

            snprintf(
                ack,
                sizeof(ack),
                "ACK=%d",
                sequence_number
            );

            /*
             * Apply ACK-loss simulation to duplicate ACKs too.
             */
            if (channel_should_drop_ack(
                    &channel)) {

                printf(
                    "CHANNEL EMULATOR: "
                    "Dropping duplicate ACK=%d\n\n",
                    sequence_number
                );

                continue;
            }

            channel_delay(&channel);

            sendto(
                sock,
                ack,
                (int)strlen(ack),
                0,
                (struct sockaddr *)&client_addr,
                client_len
            );

            printf(
                "Re-sent: %s\n",
                ack
            );

        } else {

            printf(
                "Unexpected SEQ=%d. "
                "Expected SEQ=%d.\n",
                sequence_number,
                expected_sequence
            );

            /*
             * Do not acknowledge packets that arrive
             * ahead of the expected sequence number.
             */

        }

        printf("\n");
    }

#ifdef _WIN32

    closesocket(sock);
    WSACleanup();

#else

    close(sock);

#endif

    return 0;
}
