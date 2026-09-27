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
#include <sys/select.h>

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
    int sockfd, length;
    char buf[100], buf1[100];
    char packet[250];
    struct sockaddr_in sa;

    unsigned short checksum;
    int seq = 0;

    while (1)
    {
        int attempts = 0;
        int acknowledgment_not_received = 1;

        sockfd = socket(AF_INET, SOCK_DGRAM, 0);

        sa.sin_family = AF_INET;
        sa.sin_addr.s_addr = inet_addr("127.0.0.1");
        sa.sin_port = htons(6025);

        printf("Enter a message: ");
        gets(buf);

        if (strcmp(buf, "exit") == 0)
        {
            sendto(sockfd, buf, strlen(buf), 0,
                   (struct sockaddr *)&sa, sizeof(sa));

            close(sockfd);
            break;
        }

        checksum = calculate_checksum(buf, strlen(buf));

        sprintf(packet, "SEQ=%d|CHK=%04X|DATA=%s", seq, checksum, buf);

        while (attempts < 3 && acknowledgment_not_received)
        {
            printf("Sending: %s\n", packet);

            sendto(sockfd, packet, strlen(packet), 0,
                   (struct sockaddr *)&sa, sizeof(sa));

            length = sizeof(sa);

            fd_set readfds;
            struct timeval timeout;

            FD_ZERO(&readfds);
            FD_SET(sockfd, &readfds);

            timeout.tv_sec = 2;
            timeout.tv_usec = 0;

            int ready = select(sockfd + 1, &readfds, NULL, NULL, &timeout);

            if (ready > 0)
            {
                int k = recvfrom(sockfd, buf1, sizeof(buf1)-1, 0, (struct sockaddr *)&sa, &length);

                if (k >= 0)
                {
                    buf1[k] = '\0';

                    char expected_ack[50];
                    snprintf(expected_ack, sizeof(expected_ack), "ACK=%d", seq);

                    printf("Received: %s\n", buf1);
                    
                    if (strcmp(buf1, expected_ack) == 0)
                    {
                        printf("Valid ACK received.\n");
                        acknowledgment_not_received = 0;
                        break;
                    }
                    else
                    { 
                        printf("Invalid ACK! Expected %s\n", expected_ack);
                    }
                }
            }
            else if (ready == 0)
            {
                printf("Timeout! No ACK received. Retrying...\n");
            }
            else
            {
                perror("select");
                break;
            }
            attempts++;
        }
        
        if (acknowledgment_not_received)
        {
            printf("Transmission failed after 3 attempts.\n");
        }
        else
        {
            printf("Transmission successful.\n");
            seq++;
        }

        close(sockfd);
    }

    printf("Client exited.\n");

    return 0;
}