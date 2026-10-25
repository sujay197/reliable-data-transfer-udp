
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
#include <errno.h>
#endif

#include "../rto/rto.h"
#include "../channel/channel_emulator.h"

#define SERVER_IP "127.0.0.1"
#define PORT 6026

#define TOTAL_PACKETS 10
#define WINDOW_SIZE 4

#define INITIAL_RTO_MS 2000.0

/*
 * The receiver handles packet loss and ACK loss.
 * The sender only applies outgoing channel delay.
 */
#define PACKET_LOSS_PERCENT 0
#define ACK_LOSS_PERCENT 0
#define CHANNEL_DELAY_MS 100
#define CHANNEL_SEED 12345

#ifdef _WIN32
#define close_socket_platform closesocket
#else
#define close_socket_platform close
#endif

typedef struct {
    int seq;
    double sent_at;
    int retransmitted;
    int acked;
} PacketState;

static double current_time_ms(void)
{
#ifdef _WIN32
    static LARGE_INTEGER frequency;
    static int initialized = 0;
    LARGE_INTEGER counter;

    if (!initialized) {
        QueryPerformanceFrequency(&frequency);
        initialized = 1;
    }

    QueryPerformanceCounter(&counter);

    return ((double)counter.QuadPart * 1000.0) /
           (double)frequency.QuadPart;
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);

    return (double)tv.tv_sec * 1000.0 +
           (double)tv.tv_usec / 1000.0;
#endif
}

static void set_socket_timeout(int sockfd, double timeout_ms)
{
#ifdef _WIN32
    DWORD timeout = (DWORD)(timeout_ms < 1.0 ? 1.0 : timeout_ms);

    setsockopt(
        sockfd,
        SOL_SOCKET,
        SO_RCVTIMEO,
        (const char *)&timeout,
        sizeof(timeout)
    );
#else
    struct timeval tv;

    tv.tv_sec = (int)(timeout_ms / 1000.0);
    tv.tv_usec =
        (int)((timeout_ms - tv.tv_sec * 1000.0) * 1000.0);

    setsockopt(
        sockfd,
        SOL_SOCKET,
        SO_RCVTIMEO,
        &tv,
        sizeof(tv)
    );
#endif
}

static int timeout_error(void)
{
#ifdef _WIN32
    int error = WSAGetLastError();

    return error == WSAETIMEDOUT;
#else
    return errno == EAGAIN || errno == EWOULDBLOCK;
#endif
}

int main(void)
{
#ifdef _WIN32
    WSADATA wsa;

    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
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

    struct sockaddr_in receiver;

    memset(&receiver, 0, sizeof(receiver));

    receiver.sin_family = AF_INET;
    receiver.sin_port = htons(PORT);

#ifdef _WIN32
    receiver.sin_addr.s_addr = inet_addr(SERVER_IP);
#else
    inet_pton(AF_INET, SERVER_IP, &receiver.sin_addr);
#endif

    socklen_t receiver_len = sizeof(receiver);

    /*
     * Channel emulator configuration.
     *
     * Packet and ACK loss are handled by the receiver.
     * The sender applies only outgoing delay.
     */
    ChannelConfig channel;

    channel_init(
        &channel,
        PACKET_LOSS_PERCENT,
        ACK_LOSS_PERCENT,
        CHANNEL_DELAY_MS,
        CHANNEL_SEED
    );

    RTOEstimator rto;

    rto_init(&rto, INITIAL_RTO_MS);

    PacketState packets[TOTAL_PACKETS];

    memset(packets, 0, sizeof(packets));

    for (int i = 0; i < TOTAL_PACKETS; i++) {
        packets[i].seq = i;
        packets[i].sent_at = 0.0;
        packets[i].retransmitted = 0;
        packets[i].acked = 0;
    }

    int base = 0;
    int next_seq = 0;

    printf("=====================================\n");
    printf("   Selective Repeat Sender Started\n");
    printf("=====================================\n");
    printf("Window size: %d\n", WINDOW_SIZE);
    printf("Total packets: %d\n", TOTAL_PACKETS);
    printf("Initial RTO: %.2f ms\n", INITIAL_RTO_MS);
    printf("Adaptive RTO: ENABLED\n");
    printf("Channel Emulator: ENABLED\n");
    printf("Outgoing channel delay: %d ms\n",
           CHANNEL_DELAY_MS);
    printf("Packet loss: handled by receiver\n");
    printf("ACK loss: handled by receiver\n");
    printf("Receiver: %s:%d\n", SERVER_IP, PORT);
    printf("=====================================\n\n");

    while (base < TOTAL_PACKETS) {

        /*
         * Fill the sending window.
         */
        while (
            next_seq < TOTAL_PACKETS &&
            next_seq < base + WINDOW_SIZE) {

            if (!packets[next_seq].acked) {

                char message[128];

                snprintf(
                    message,
                    sizeof(message),
                    "SEQ=%d|DATA=Packet%d",
                    next_seq,
                    next_seq
                );

                printf("Sending: %s\n", message);

                /*
                 * Sender-side channel delay.
                 */
                channel_delay(&channel);

                int sent = sendto(
                    sockfd,
                    message,
                    (int)strlen(message),
                    0,
                    (struct sockaddr *)&receiver,
                    receiver_len
                );

                if (sent < 0) {

#ifdef _WIN32
                    printf(
                        "sendto failed: %d\n",
                        WSAGetLastError()
                    );
#else
                    perror("sendto");
#endif

                    close_socket_platform(sockfd);

#ifdef _WIN32
                    WSACleanup();
#endif

                    return 1;
                }

                packets[next_seq].sent_at =
                    current_time_ms();

                printf(
                    "Sent SEQ=%d\n",
                    next_seq
                );
            }

            next_seq++;
        }

        /*
         * Calculate the earliest timeout among
         * unacknowledged packets.
         */
        double now = current_time_ms();
        double earliest_timeout = rto_get(&rto);

        int timeout_seq = -1;

        for (int i = base; i < next_seq; i++) {

            if (packets[i].acked)
                continue;

            double elapsed =
                now - packets[i].sent_at;

            double remaining =
                rto_get(&rto) - elapsed;

            if (remaining < earliest_timeout) {
                earliest_timeout = remaining;
                timeout_seq = i;
            }
        }

        if (earliest_timeout < 1.0)
            earliest_timeout = 1.0;

        set_socket_timeout(
            sockfd,
            earliest_timeout
        );

        char response[128];

#ifdef _WIN32
        int received = recvfrom(
            sockfd,
            response,
            sizeof(response) - 1,
            0,
            (struct sockaddr *)&receiver,
            &receiver_len
        );
#else
        int received = recvfrom(
            sockfd,
            response,
            sizeof(response) - 1,
            0,
            (struct sockaddr *)&receiver,
            &receiver_len
        );
#endif

        if (received < 0) {

            if (timeout_error()) {

                /*
                 * Selective Repeat timeout.
                 *
                 * Only the timed-out packet is
                 * retransmitted.
                 */
                if (timeout_seq >= 0) {

                    printf(
                        "\n*** SELECTIVE REPEAT TIMEOUT ***\n"
                    );

                    printf(
                        "Current adaptive RTO: %.2f ms\n",
                        rto_get(&rto)
                    );

                    printf(
                        "Timeout for SEQ=%d\n",
                        timeout_seq
                    );

                    char message[128];

                    snprintf(
                        message,
                        sizeof(message),
                        "SEQ=%d|DATA=Packet%d",
                        timeout_seq,
                        timeout_seq
                    );

                    printf(
                        "Retransmitting: %s\n",
                        message
                    );

                    channel_delay(&channel);

                    int sent = sendto(
                        sockfd,
                        message,
                        (int)strlen(message),
                        0,
                        (struct sockaddr *)&receiver,
                        receiver_len
                    );

                    if (sent < 0) {

#ifdef _WIN32
                        printf(
                            "Retransmission failed: %d\n",
                            WSAGetLastError()
                        );
#else
                        perror("Retransmission failed");
#endif

                        close_socket_platform(sockfd);

#ifdef _WIN32
                        WSACleanup();
#endif

                        return 1;
                    }

                    packets[timeout_seq].sent_at =
                        current_time_ms();

                    packets[timeout_seq].retransmitted = 1;

                    printf(
                        "SEQ=%d retransmitted.\n\n",
                        timeout_seq
                    );
                }

                continue;
            }

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

        response[received] = '\0';

        printf(
            "Received: %s\n",
            response
        );

        /*
         * Parse ACK.
         */
        int ack = -1;

        if (sscanf(response, "ACK=%d", &ack) == 1) {

            if (
                ack >= 0 &&
                ack < TOTAL_PACKETS &&
                ack >= base &&
                ack < next_seq) {

                if (!packets[ack].acked) {

                    packets[ack].acked = 1;

                    printf(
                        "ACK=%d accepted.\n",
                        ack
                    );

                    /*
                     * Karn's algorithm:
                     *
                     * Only use RTT samples from
                     * packets that were never
                     * retransmitted.
                     */
                    if (!packets[ack].retransmitted) {

                        double sample_rtt =
                            current_time_ms() -
                            packets[ack].sent_at;

                        printf(
                            "Measured RTT for SEQ=%d: %.2f ms\n",
                            ack,
                            sample_rtt
                        );

                        rto_update(
                            &rto,
                            sample_rtt
                        );

                        printf(
                            "Updated RTO: %.2f ms\n",
                            rto_get(&rto)
                        );

                    } else {

                        printf(
                            "RTT sample skipped for SEQ=%d "
                            "(packet was retransmitted).\n",
                            ack
                        );

                        printf(
                            "Current RTO: %.2f ms\n",
                            rto_get(&rto)
                        );
                    }

                    /*
                     * Advance the base only across
                     * consecutively acknowledged packets.
                     *
                     * This preserves Selective Repeat.
                     */
                    while (
                        base < TOTAL_PACKETS &&
                        packets[base].acked) {

                        base++;
                    }

                    printf(
                        "Updated base: %d\n\n",
                        base
                    );

                } else {

                    printf(
                        "Duplicate ACK=%d\n",
                        ack
                    );
                }

            } else {

                printf(
                    "Ignoring invalid or out-of-window "
                    "ACK=%d\n",
                    ack
                );
            }

        } else {

            printf(
                "Ignoring invalid response: %s\n",
                response
            );
        }
    }

    /*
     * All packets have been acknowledged.
     * Tell the receiver transmission is complete.
     */
    channel_delay(&channel);

    if (sendto(
            sockfd,
            "END",
            3,
            0,
            (struct sockaddr *)&receiver,
            receiver_len) < 0) {

#ifdef _WIN32
        printf(
            "sendto END failed with error: %d\n",
            WSAGetLastError()
        );
#else
        perror("sendto END");
#endif

        close_socket_platform(sockfd);

#ifdef _WIN32
        WSACleanup();
#endif

        return 1;
    }

    printf("=====================================\n");
    printf("     Transmission Complete\n");
    printf("=====================================\n");
    printf("Base: %d\n", base);
    printf("Next sequence number: %d\n", next_seq);
    printf(
        "Final RTO: %.2f ms\n",
        rto_get(&rto)
    );
    printf(
        "All %d packets acknowledged.\n",
        TOTAL_PACKETS
    );

    close_socket_platform(sockfd);

#ifdef _WIN32
    WSACleanup();
#endif

    return 0;
}
