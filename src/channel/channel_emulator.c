#include "channel_emulator.h"

#include <stdlib.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

void channel_init(ChannelConfig *config,
                  int packet_loss_percent,
                  int ack_loss_percent,
                  int delay_ms,
                  unsigned int seed)
{
    if (config == NULL) {
        return;
    }

    config->packet_loss_percent = packet_loss_percent;
    config->ack_loss_percent = ack_loss_percent;
    config->delay_ms = delay_ms;
    config->seed = seed;

    srand(seed);
}

int channel_should_drop_packet(const ChannelConfig *config)
{
    if (config == NULL) {
        return 0;
    }

    if (config->packet_loss_percent <= 0) {
        return 0;
    }

    if (config->packet_loss_percent >= 100) {
        return 1;
    }

    return (rand() % 100) < config->packet_loss_percent;
}

int channel_should_drop_ack(const ChannelConfig *config)
{
    if (config == NULL) {
        return 0;
    }

    if (config->ack_loss_percent <= 0) {
        return 0;
    }

    if (config->ack_loss_percent >= 100) {
        return 1;
    }

    return (rand() % 100) < config->ack_loss_percent;
}

void channel_delay(const ChannelConfig *config)
{
    if (config == NULL || config->delay_ms <= 0) {
        return;
    }

#ifdef _WIN32
    Sleep((DWORD)config->delay_ms);
#else
    usleep((useconds_t)config->delay_ms * 1000U);
#endif
}
