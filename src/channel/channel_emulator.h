#ifndef CHANNEL_EMULATOR_H
#define CHANNEL_EMULATOR_H

typedef struct {
    int packet_loss_percent;
    int ack_loss_percent;
    int delay_ms;
    unsigned int seed;
} ChannelConfig;

void channel_init(ChannelConfig *config,
                  int packet_loss_percent,
                  int ack_loss_percent,
                  int delay_ms,
                  unsigned int seed);

int channel_should_drop_packet(const ChannelConfig *config);

int channel_should_drop_ack(const ChannelConfig *config);

void channel_delay(const ChannelConfig *config);

#endif