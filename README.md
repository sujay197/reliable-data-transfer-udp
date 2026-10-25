# Reliable Data Transfer over UDP

A socket-level reliable data transfer system implemented over UDP.

## Protocols

- Stop-and-Wait ARQ
- Go-Back-N ARQ
- Selective Repeat ARQ

## Features

- Sequence numbers
- Acknowledgements
- Checksums
- Retransmission
- Adaptive retransmission timeout (RTO)
- RTT measurement
- Karn's algorithm
- Channel emulation
- Packet loss simulation
- ACK loss simulation
- Configurable channel delay
- File integrity verification
- Selective Repeat receiver buffering
- In-order delivery

## Project Structure

- `src/` — Protocol implementations
- `build/` — Compiled executables
- `data/` — Input and output data
- `docs/` — Project documentation
- `experiments/` — Experiment procedures and observations
- `results/` — Experiment results in CSV format
- `tests/` — Test files

## Week 5 Enhancements

Week 5 integrates adaptive retransmission timeout estimation and channel emulation into all three reliable UDP protocols.

### Adaptive RTO

The sender measures valid RTT samples and updates:

- Smoothed RTT
- RTT deviation
- Retransmission timeout (RTO)

The initial RTO is `2000 ms`. RTO values are bounded to prevent excessively small or large timeout values.

Karn's algorithm is used so that retransmitted packets are not used for RTT estimation.

### Channel Emulator

The channel emulator supports:

- Packet loss
- ACK loss
- Configurable delay
- Reproducible random seeds

This allows the reliability mechanisms of each protocol to be tested under controlled network conditions.

## Stop-and-Wait ARQ

The Stop-and-Wait implementation uses adaptive RTO, RTT measurement, Karn's algorithm, retransmission, checksums, and channel emulation.

Week 5 testing covered:

- No packet or ACK loss
- 30% packet loss
- 30% ACK loss
- Retransmission and timeout behavior
- Adaptive RTO updates

## Go-Back-N ARQ Experiment

The Go-Back-N implementation uses UDP port `6025`, a window size of `4`, and `12` packets.

The sender uses cumulative acknowledgements and retransmits outstanding packets after timeout. Adaptive RTO and Karn's algorithm are integrated with the retransmission mechanism.

Week 5 testing covered:

- No loss
- 30% packet loss
- 30% ACK loss
- Combined packet/ACK loss
- Channel delay
- Adaptive RTO

See [`experiments/gbn_experiment.md`](experiments/gbn_experiment.md) for the experiment procedure and observations.

## Selective Repeat ARQ Experiment

The Selective Repeat implementation uses UDP port `6026`, a window size of `4`, and `10` packets.

The receiver supports individual acknowledgements and buffering of out-of-order packets. The sender retransmits only packets whose acknowledgements are missing.

The receiver deliberately drops the first copy of packet `2` during the standard Selective Repeat experiment.

Week 5 testing covered:

- No loss
- 30% packet loss
- 30% ACK loss
- Adaptive RTO under 500 ms receiver-side delay
- Retransmissions under delay
- In-order delivery with out-of-order buffering

See [`experiments/sr_experiment.md`](experiments/sr_experiment.md) for the experiment procedure and observations.

## Week 5 Results

Detailed Week 5 measurements are recorded in:

- [`results/week5_results.csv`](results/week5_results.csv)

The results include packet loss, ACK loss, delay, retransmissions, timeouts, initial RTO, final RTO, in-order delivery, and transfer completion.

All recorded Week 5 protocol tests completed successfully.

## Building and Running

Build and execution commands depend on the protocol implementation and your environment. Refer to the source files and experiment documentation for the applicable instructions.

For MinGW/Windows, the protocol executables can be built with commands such as:

    gcc src\stop_and_wait\receiver_timeout.c src\channel\channel_emulator.c -o build\receiver_timeout.exe -lws2_32
    gcc src\stop_and_wait\sender_timeout.c src\rto\rto.c src\channel\channel_emulator.c -o build\sender_timeout.exe -lws2_32

    gcc src\go_back_n\receiver_gbn.c src\channel\channel_emulator.c -o build\receiver_gbn.exe -lws2_32
    gcc src\go_back_n\sender_gbn.c src\rto\rto.c src\channel\channel_emulator.c -o build\sender_gbn.exe -lws2_32

    gcc src\selective_repeat\receiver_sr.c src\channel\channel_emulator.c -o build\receiver_sr.exe -lws2_32
    gcc src\selective_repeat\sender_sr.c src\rto\rto.c src\channel\channel_emulator.c -o build\sender_sr.exe -lws2_32

## Team

- Sujay Pandit
- Archit Ishwar
- Abhishek Behera
- Suryansh Paul