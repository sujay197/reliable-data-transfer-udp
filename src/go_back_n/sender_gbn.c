int main()
{
    int sockfd;

    struct sockaddr_in sa;

    int base = 0;
    int next_seq_num = 0;
    int window_size = 4;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    if (sockfd < 0)
    {
        perror("socket");
        return 1;
    }

    sa.sin_family = AF_INET;
    sa.sin_addr.s_addr = inet_addr("127.0.0.1");
    sa.sin_port = htons(6025);

    printf("Go-Back-N Sender started.\n");
    printf("Window size: %d\n", window_size);
    printf("Base: %d\n", base);
    printf("Next sequence number: %d\n", next_seq_num);

    close(sockfd);

    return 0;
}