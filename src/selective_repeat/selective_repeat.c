
#include <stdio.h>

#define N 5
#define WINDOW_SIZE 3

int main(void) {
    int frames[N] = {0, 1, 2, 3, 4};

    // 1 = frame has been acknowledged
    int acked[N] = {0};

    // 1 = frame is stored in receiver buffer
    int buffer[N] = {0};

    // Simulate first-transmission losses
    int lost[N] = {0, 1, 0, 1, 0};

    int base = 0;
    int i, end;

    printf("====================================\n");
    printf(" SELECTIVE REPEAT ARQ WITH BUFFERING\n");
    printf("====================================\n\n");

    while (base < N) {
        end = base + WINDOW_SIZE;

        if (end > N)
            end = N;

        printf("Sender window: Frames %d to %d\n\n",
               frames[base], frames[end - 1]);

        // Step 1: Transmit unacknowledged frames
        for (i = base; i < end; i++) {
            if (acked[i])
                continue;

            printf("Sender sends Frame %d\n", frames[i]);

            if (lost[i]) {
                printf("Frame %d is lost!\n", frames[i]);

                // Loss happens only on first transmission
                lost[i] = 0;
                continue;
            }

            // Receiver receives and acknowledges the frame
            buffer[i] = 1;
            acked[i] = 1;

            printf("Receiver sends ACK for Frame %d\n",
                   frames[i]);

            if (i > base) {
                printf("Frame %d stored in receiver buffer\n",
                       frames[i]);
            }
        }

        // Step 2: Retransmit only missing frames
        for (i = base; i < end; i++) {
            if (!acked[i]) {
                printf("\nTimeout: Frame %d not acknowledged\n",
                       frames[i]);

                printf("Sender retransmits Frame %d\n",
                       frames[i]);

                buffer[i] = 1;
                acked[i] = 1;

                printf("Receiver sends ACK for Frame %d\n",
                       frames[i]);
            }
        }

        // Step 3: Deliver buffered frames in sequence
        printf("\nReceiver delivers:\n");

        while (base < N && buffer[base]) {
            printf("Frame %d\n", frames[base]);
            base++;
        }

        printf("\n------------------------------------\n\n");
    }

    printf("All frames delivered successfully!\n");

    return 0;
}
