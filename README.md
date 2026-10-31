

# Reliable Data Transfer over UDP

A socket-level reliable data transfer system implemented in C using UDP. This project demonstrates and compares three reliable data transfer protocols: Stop-and-Wait ARQ, Go-Back-N ARQ, and Selective Repeat ARQ.

## Project Objectives

- Implement reliable data transmission over UDP.
- Demonstrate packet sequencing and acknowledgement mechanisms.
- Detect corrupted packets using checksums.
- Handle packet loss and acknowledgement loss through retransmission.
- Implement adaptive Retransmission Timeout (RTO) estimation.
- Apply Karn's algorithm to avoid inaccurate RTT measurements.
- Simulate network impairments using a channel emulator.
- Compare the behavior of different ARQ protocols experimentally.

## Protocols Implemented

### 1. Stop-and-Wait ARQ

- Transmits one packet at a time.
- Waits for the corresponding acknowledgement.
- Uses sequence numbers and checksums.
- Supports timeout-based retransmission.
- Detects duplicate packets and resends acknowledgements.

### 2. Go-Back-N ARQ

- Uses a sliding window with a configured window size of 4.
- Supports cumulative acknowledgements.
- Retransmits outstanding packets after timeout.
- Uses adaptive RTO and Karn's algorithm.

### 3. Selective Repeat ARQ

- Uses a sliding window with a configured window size of 4.
- Supports individual packet acknowledgements.
- Retransmits only packets whose acknowledgements are missing.
- Buffers out-of-order packets at the receiver.
- Delivers received data to the application in order.

## Features

- UDP socket communication
- Sequence numbers and acknowledgements
- Checksum-based error detection
- Timeout-based retransmission
- RTT measurement
- Adaptive retransmission timeout (RTO)
- Karn's algorithm
- Channel emulation
- Packet loss and ACK loss simulation
- Configurable channel delay
- Selective Repeat receiver buffering
- In-order data delivery
- Automated integration tests

## Technologies Used

- C
- UDP sockets
- Winsock2 (Windows)
- GCC / MinGW
- PowerShell
- Git and GitHub

## Project Structure

```text
reliable-data-transfer-udp/
├── src/
│   ├── stop_and_wait/
│   ├── go_back_n/
│   ├── selective_repeat/
│   ├── channel/
│   └── rto/
├── tests/
├── build/
├── docs/
├── experiments/
├── results/
├── graphs/
├── README.md
├── AI-USE.md
└── generate_graphs.ps1
```

## Prerequisites

- Windows operating system
- GCC compiler (MinGW-w64 recommended)
- Git
- PowerShell
- Visual Studio Code (recommended)

Verify GCC installation:

```powershell
gcc --version
```

## Getting Started

Clone the repository:

```powershell
git clone https://github.com/sujay197/reliable-data-transfer-udp.git
cd reliable-data-transfer-udp
```

Ensure the `build` directory exists before compiling:

```powershell
New-Item -ItemType Directory -Force build
```

Compile the protocol programs using GCC and the Winsock library.

### Stop-and-Wait

```powershell
gcc src\stop_and_wait\receiver_timeout.c src\channel\channel_emulator.c -o build\receiver_timeout.exe -lws2_32

gcc src\stop_and_wait\sender_timeout.c src\rto\rto.c src\channel\channel_emulator.c -o build\sender_timeout.exe -lws2_32
```

### Go-Back-N

```powershell
gcc src\go_back_n\receiver_gbn.c src\channel\channel_emulator.c -o build\receiver_gbn.exe -lws2_32

gcc src\go_back_n\sender_gbn.c src\rto\rto.c src\channel\channel_emulator.c -o build\sender_gbn.exe -lws2_32
```

### Selective Repeat

```powershell
gcc src\selective_repeat\receiver_sr.c src\channel\channel_emulator.c -o build\receiver_sr.exe -lws2_32

gcc src\selective_repeat\sender_sr.c src\rto\rto.c src\channel\channel_emulator.c -o build\sender_sr.exe -lws2_32
```

> Run the receiver before starting its corresponding sender. Use separate terminals for the sender and receiver.

## Testing

The project contains automated tests for packet validation and protocol behavior.

Run the automated test suite using PowerShell:

```powershell
.\tests\run_tests.ps1
```

The test suite compiles and executes the available integration tests, including invalid-ACK and duplicate-packet behavior.

## Experiments and Results

Experiment procedures and observations are documented in the `experiments/` directory.

| File | Description |
|---|---|
| `experiments/stop_and_wait_experiment.md` | Stop-and-Wait validation experiments |
| `experiments/gbn_experiment.md` | Go-Back-N experiments |
| `experiments/sr_experiment.md` | Selective Repeat experiments |
| `experiments/protocol_comparison.md` | Comparative protocol analysis |

Measured results are stored in CSV format in `results/`.

Important files:

- `results/stop_and_wait_results.csv`
- `results/gbn_results.csv`
- `results/sr_results.csv`
- `results/week5_results.csv`
- `results/protocol_comparison.csv`

## Graphs and Analysis

Generated graphs are stored in `graphs/`.

- `graphs/final_rto_comparison.svg`
- `graphs/packet_count_comparison.svg`

The graph generation script is:

```powershell
.\generate_graphs.ps1
```

The comparison report discusses reliability, retransmission behavior, timeout observations, and RTO measurements based on recorded experiments.

## Adaptive RTO and Channel Emulation

The adaptive RTO estimator maintains:

- Smoothed RTT (SRTT)
- RTT deviation (RTTVAR)
- Retransmission timeout (RTO)

The initial RTO used in the documented experiments is `2000 ms`. RTO values are bounded to avoid excessively small or large timeouts.

Karn's algorithm excludes retransmitted packets from RTT estimation.

The channel emulator supports controlled packet loss, ACK loss, configurable delay, and reproducible random seeds.

## Documentation

Additional project documentation is maintained in:

- `experiments/` — experiment procedures and observations
- `results/` — experimental measurements in CSV format
- `graphs/` — generated comparison graphs
- `docs/` — final report and demonstration instructions

## Team Members

- Sujay Pandit
- Archit Ishwar
- Abhishek Behera
- Suryansh Paul

## AI Usage

Refer to [`AI-USE.md`](AI-USE.md) for documentation of AI-assisted activities during the project.