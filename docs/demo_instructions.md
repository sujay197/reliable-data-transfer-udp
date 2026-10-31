

# Demonstration Instructions
## Reliable Data Transfer over UDP

## 1. Overview

This document describes how to compile and demonstrate the Stop-and-Wait, Go-Back-N, and Selective Repeat ARQ implementations in the Reliable Data Transfer over UDP project.

The demonstrations use separate sender and receiver programs communicating through UDP sockets. The receiver should be started before the corresponding sender.

## 2. Prerequisites

Ensure the following are installed:

- Windows
- GCC (MinGW-w64)
- PowerShell
- Git

Verify GCC:

```powershell
gcc --version
```

Navigate to the project root:

```powershell
cd reliable-data-transfer-udp
```

Create the build directory if it does not exist:

```powershell
New-Item -ItemType Directory -Force build
```

## 3. Stop-and-Wait Demonstration

### Compile

```powershell
gcc src\stop_and_wait\receiver_timeout.c src\channel\channel_emulator.c -o build\receiver_timeout.exe -lws2_32

gcc src\stop_and_wait\sender_timeout.c src\rto\rto.c src\channel\channel_emulator.c -o build\sender_timeout.exe -lws2_32
```

### Run

**Terminal 1 — Receiver**

```powershell
.\build\receiver_timeout.exe
```

**Terminal 2 — Sender**

```powershell
.\build\sender_timeout.exe
```

### Observe

- Packet transmission.
- Receiver packet validation.
- Acknowledgement generation.
- Sender acknowledgement handling.
- Timeout and retransmission behavior, if triggered by the test conditions.

## 4. Go-Back-N Demonstration

### Compile

```powershell
gcc src\go_back_n\receiver_gbn.c src\channel\channel_emulator.c -o build\receiver_gbn.exe -lws2_32

gcc src\go_back_n\sender_gbn.c src\rto\rto.c src\channel\channel_emulator.c -o build\sender_gbn.exe -lws2_32
```

### Run

**Terminal 1 — Receiver**

```powershell
.\build\receiver_gbn.exe
```

**Terminal 2 — Sender**

```powershell
.\build\sender_gbn.exe
```

### Observe

- Sliding-window transmission.
- Multiple outstanding packets.
- Cumulative acknowledgements.
- Retransmissions when packet loss or timeout occurs.
- Completion of the transfer.

## 5. Selective Repeat Demonstration

### Compile

```powershell
gcc src\selective_repeat\receiver_sr.c src\channel\channel_emulator.c -o build\receiver_sr.exe -lws2_32

gcc src\selective_repeat\sender_sr.c src\rto\rto.c src\channel\channel_emulator.c -o build\sender_sr.exe -lws2_32
```

### Run

**Terminal 1 — Receiver**

```powershell
.\build\receiver_sr.exe
```

**Terminal 2 — Sender**

```powershell
.\build\sender_sr.exe
```

### Observe

- Sliding-window transmission.
- Individual acknowledgements.
- Selective retransmission of missing packets.
- Receiver buffering of out-of-order packets.
- In-order data delivery.

## 6. Automated Testing

Run the automated test suite from the project root:

```powershell
.\tests\run_tests.ps1
```

The test suite includes packet validation, invalid-ACK behavior, and duplicate-packet handling.

## 7. Experimental Results and Graphs

Experiment procedures are documented in the `experiments/` directory.

Recorded results are available in:

- `results/stop_and_wait_results.csv`
- `results/gbn_results.csv`
- `results/sr_results.csv`
- `results/week5_results.csv`
- `results/protocol_comparison.csv`

Generated graphs:

- `graphs/final_rto_comparison.svg`
- `graphs/packet_count_comparison.svg`

To regenerate the graphs:

```powershell
.\generate_graphs.ps1
```

## 8. Troubleshooting

### GCC is not recognized

Verify that GCC is installed and added to the Windows PATH.

```powershell
gcc --version
```

### Build directory does not exist

Create it:

```powershell
New-Item -ItemType Directory -Force build
```

### Receiver is not responding

- Start the receiver before the sender.
- Use separate terminals.
- Ensure the required UDP port is not occupied by another process.
- Check the terminal output for socket or initialization errors.

### Unexpected timeout or retransmission

- Check whether the receiver is running.
- Check the configured channel emulator conditions.
- Consider packet loss, ACK loss, and simulated delay.
- Review sender and receiver output.

