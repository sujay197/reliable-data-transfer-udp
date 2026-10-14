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
- Channel emulation
- File integrity verification
- Selective Repeat packet-loss simulation

## Project Structure

- `src/` — Protocol implementations
- `build/` — Compiled executables
- `data/` — Input and output data
- `docs/` — Project documentation
- `experiments/` — Experiment procedures and observations
- `results/` — Experiment results in CSV format
- `tests/` — Test files

## Selective Repeat ARQ Experiment

The Selective Repeat UDP implementation uses port `6026`, a window size of `4`, and `10` packets. The receiver simulates packet loss by dropping the first copy of packet `2`, allowing the sender's timeout and selective retransmission behavior to be tested.

See [`experiments/sr_experiment.md`](experiments/sr_experiment.md) for the experiment procedure and [`results/sr_results.csv`](results/sr_results.csv) for the results template.

## Go-Back-N ARQ Experiment

See [`experiments/gbn_experiment.md`](experiments/gbn_experiment.md) for the experiment procedure and [`results/gbn_results.csv`](results/gbn_results.csv) for recorded results.

## Building and Running

Build and execution commands depend on the protocol implementation and your environment. Refer to the source files and experiment documentation for the applicable instructions.

## Team

- Sujay Pandit
- Archit Ishwar
- Abhishek Behera
- Suryansh Paul