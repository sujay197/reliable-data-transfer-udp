#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <stddef.h>

unsigned short calculate_checksum(const char *data, size_t length)
{
    unsigned long sum = 0;
    const unsigned char *ptr = (const unsigned char *)data;

    while (length > 1)
    {
        unsigned short word = ((unsigned short)ptr[0] << 8) | ptr[1];
        sum += word;

        ptr += 2;
        length -= 2;
    }

    if (length > 0)
    {
        sum += (unsigned short)ptr[0] << 8;
    }

    while (sum >> 16)
    {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }

    return (unsigned short)(~sum);
}

int main()
{
    int sockfd, length, i;
    char buf[100];
    char ack[50];
    char data[100];
    unsigned short received_checksum, calculated_checksum;

    int seq;

    struct sockaddr_in sa, ca;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    sa.sin_family = AF_INET;
    sa.sin_addr.s_addr = inet_addr("127.0.0.1");
    sa.sin_port = htons(6025);

    i = bind(sockfd, (struct sockaddr *)&sa, sizeof(sa));

    printf("Server started.\n");

    while (1)
    {
        length = sizeof(ca);

        int k = recvfrom(sockfd, buf, 100, 0,
                         (struct sockaddr *)&ca, &length);

        buf[k] = '\0';

        if (strcmp(buf, "exit") == 0)
        {
            printf("Client requested exit.\n");
            break;
        }

        printf("Received: %s\n", buf);

        int fields = sscanf(buf, "SEQ=%d|CHK=%hx|DATA=%99[^\n]",
                    &seq, &received_checksum, data);

        if (fields != 3) {
            printf("Invalid packet format\n");
            continue;
        }
        
        printf("Received packet with sequence number: %d\n", seq);
        
        calculated_checksum = calculate_checksum(data, strlen(data));

        if (received_checksum == calculated_checksum)
        {
            printf("Checksum valid\n");

            sprintf(ack, "ACK=%d", seq);

            printf("Sending: %s\n", ack);

            sendto(sockfd, ack, strlen(ack), 0, (struct sockaddr *)&ca, sizeof(ca));
        }
        else
        {
            printf("Checksum mismatch. Packet corrupted.\n");
        }

    }

    close(sockfd);

    printf("Server exited.\n");

    return 0;
}