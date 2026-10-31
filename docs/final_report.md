

# Reliable Data Transfer over UDP
## Final Project Report

## Abstract

This project implements and evaluates reliable data transfer mechanisms over the User Datagram Protocol (UDP) using the C programming language and Windows socket programming. Since UDP does not guarantee packet delivery, ordering, or duplicate protection, reliable communication mechanisms are implemented at the application level.

Three Automatic Repeat reQuest (ARQ) protocols are developed and studied: Stop-and-Wait, Go-Back-N, and Selective Repeat. The system incorporates sequence numbers, acknowledgements, checksum-based error detection, timeout-based retransmission, adaptive Retransmission Timeout (RTO) estimation, Karn's algorithm, and a channel emulator for simulating network impairments.

Experiments are conducted under normal transmission, packet loss, acknowledgement loss, and delayed network conditions. The recorded results are used to compare protocol behavior, retransmission patterns, timeout observations, and reliable in-order delivery.

---

## 1. Introduction

Reliable data transfer is an essential requirement in computer networks. Applications often need data to arrive correctly, completely, and in the proper order, even when the underlying network experiences packet loss, corruption, or delays.

UDP provides a connectionless transport service with low overhead, but it does not provide built-in reliability. Unlike TCP, UDP does not automatically retransmit lost packets, acknowledge received data, or ensure in-order delivery.

This project explores how reliability can be implemented on top of UDP through different ARQ protocols. Each protocol uses a different strategy to handle lost packets and acknowledgements.

The project also investigates how adaptive timeout estimation and simulated network conditions affect protocol performance.

## 2. Objectives

The main objectives of this project are:

- To implement reliable data transfer over UDP using C.
- To implement Stop-and-Wait, Go-Back-N, and Selective Repeat ARQ.
- To implement sequence numbers and acknowledgement mechanisms.
- To detect packet corruption using checksums.
- To handle packet loss and acknowledgement loss through retransmission.
- To implement adaptive RTO estimation using RTT measurements.
- To apply Karn's algorithm during retransmission.
- To simulate packet loss, ACK loss, and network delay.
- To evaluate and compare the behavior of the implemented protocols.
- To perform automated tests for packet validation and protocol behavior.

## 3. Technologies Used

| Technology | Purpose |
|---|---|
| C | Protocol implementation |
| UDP sockets | Network communication |
| Winsock2 | Windows socket programming |
| GCC / MinGW | Compilation |
| PowerShell | Build and test execution |
| Git | Version control |
| GitHub | Repository hosting and collaboration |

## 4. System Design and Architecture

The system is organized into separate protocol implementations and shared networking components.

### 4.1 Sender

The sender is responsible for:

- Creating data packets.
- Assigning sequence numbers.
- Transmitting packets over UDP.
- Receiving acknowledgements.
- Detecting missing or invalid acknowledgements.
- Starting and monitoring retransmission timers.
- Retransmitting packets when required.
- Updating RTO estimates based on valid RTT measurements.

### 4.2 Receiver

The receiver is responsible for:

- Receiving UDP packets.
- Validating packet structure and checksums.
- Checking sequence numbers.
- Generating acknowledgements.
- Detecting duplicate packets.
- Buffering out-of-order packets in Selective Repeat.
- Delivering data in the correct order where supported.

### 4.3 Channel Emulator

A channel emulator is used to introduce simulated network impairments into the communication process.

The implemented channel emulation supports:

- Packet loss.
- Acknowledgement loss.
- Configurable network delay.
- Reproducible behavior using random seeds.

This allows protocol behavior to be evaluated under controlled conditions without depending entirely on real network failures.

### 4.4 Adaptive RTO Module

The shared RTO module estimates the retransmission timeout using RTT measurements.

The estimator maintains:

- Smoothed Round-Trip Time (SRTT).
- Round-Trip Time Variation (RTTVAR).
- Retransmission Timeout (RTO).

Karn's algorithm is applied to avoid using ambiguous RTT measurements from retransmitted packets.

## 5. Protocol Implementations

### 5.1 Stop-and-Wait ARQ

Stop-and-Wait is the simplest ARQ protocol implemented in this project.

The sender transmits one packet and waits for its corresponding acknowledgement before sending the next packet.

**Operation:**

1. The sender transmits a packet with a sequence number.
2. The receiver validates the packet.
3. The receiver sends an acknowledgement.
4. The sender receives the acknowledgement and proceeds.
5. If the acknowledgement is not received within the timeout period, the sender retransmits.

**Characteristics:**

- Only one outstanding packet at a time.
- Simple sender and receiver logic.
- Straightforward duplicate detection.
- Lower transmission concurrency.
- Performance can be affected by RTT and timeout duration.

### 5.2 Go-Back-N ARQ

Go-Back-N uses a sliding window to allow multiple packets to be transmitted before acknowledgements are received.

The implemented experiment uses a configured window size of 4 and a 12-packet transfer.

**Operation:**

1. The sender transmits packets within its current window.
2. The receiver accepts packets in sequence.
3. The receiver sends cumulative acknowledgements.
4. The sender advances its window when acknowledgements arrive.
5. When a timeout occurs, the sender retransmits outstanding packets according to Go-Back-N behavior.

**Characteristics:**

- Sliding-window transmission.
- Cumulative acknowledgements.
- Multiple packets can remain outstanding.
- Lost packets can cause retransmission of subsequent outstanding packets.
- Adaptive RTO estimation is used.

### 5.3 Selective Repeat ARQ

Selective Repeat also uses a sliding window, but it acknowledges packets individually and retransmits only packets whose acknowledgements are missing.

The implemented experiment uses a window size of 4 and a 10-packet transfer.

**Operation:**

1. The sender transmits packets within its window.
2. The receiver individually acknowledges valid packets.
3. Out-of-order packets are buffered.
4. The sender retransmits only packets that time out.
5. The receiver delivers buffered data in order when missing earlier packets arrive.

**Characteristics:**

- Individual acknowledgements.
- Selective retransmission.
- Receiver-side buffering.
- In-order application delivery.
- Reduced unnecessary retransmissions compared with Go-Back-N in suitable loss conditions.

## 6. Adaptive Retransmission Timeout

A fixed timeout may be inefficient because network round-trip times can vary.

The project implements adaptive RTO estimation using RTT measurements.

The estimator tracks the smoothed RTT and RTT variation and uses these values to calculate a retransmission timeout.

The initial RTO used in the documented experiments is **2000 ms**.

### 6.1 Karn's Algorithm

When a packet is retransmitted, it becomes difficult to determine whether an acknowledgement corresponds to the original transmission or the retransmission.

Karn's algorithm avoids using RTT samples from retransmitted packets when updating the RTT estimator.

This prevents ambiguous measurements from producing inaccurate timeout estimates.

### 6.2 Purpose

Adaptive RTO helps the sender respond to observed network conditions by adjusting timeout values instead of relying exclusively on a fixed timeout.

The experiment results record initial and final RTO values for the evaluated scenarios.

## 7. Testing and Validation

Testing was performed to evaluate packet validation, acknowledgement handling, duplicate-packet behavior, and protocol operation.

### 7.1 Automated Tests

The automated test suite includes:

- Compilation of the duplicate-packet test.
- Compilation of the invalid-ACK test.
- Invalid-ACK behavior.
- Duplicate-packet behavior.

The recorded automated test execution reported:

| Test | Result |
|---|---|
| Duplicate-packet test compilation | PASS |
| Invalid-ACK test compilation | PASS |
| Invalid-ACK handling | PASS |
| Duplicate-packet handling | PASS |
| **Total** | **4 passed, 0 failed** |

### 7.2 Stop-and-Wait Validation

Additional Stop-and-Wait validation scenarios were documented.

| Test scenario | Observed behavior | Status |
|---|---|---|
| Normal transmission | Data delivered and ACK observed | Pass |
| Corrupted packet | Checksum mismatch detected | Pass |
| Wrong checksum | Invalid checksum rejected | Pass |
| Malformed packet | Invalid packet format detected | Pass |
| Invalid ACK | Sender rejected unexpected ACK | Pass |
| Duplicate packet | Duplicate detected and ACK resent | Pass |
| Timeout and retransmission | Sender failed after retry attempts when receiver was stopped | Observed failure |

The timeout/retransmission scenario was not conclusively isolated because the receiver was stopped during the test. Therefore, this observation is recorded as a limitation rather than treated as a successful timeout validation.

## 8. Experimental Methodology

Experiments were conducted using the implemented protocol programs and channel emulator.

The evaluated conditions include:

1. Normal transmission without packet loss.
2. Simulated packet loss.
3. Simulated acknowledgement loss.
4. Combined packet loss and acknowledgement loss.
5. Delayed transmission conditions.
6. Out-of-order packet arrival and in-order delivery in Selective Repeat.

The experimental observations were recorded in CSV files and documented in the `experiments/` directory.

The principal comparison evaluates Stop-and-Wait, Go-Back-N, and Selective Repeat under normal transmission, packet loss, and acknowledgement loss.

## 9. Experimental Results

### 9.1 Protocol Comparison

The following table summarizes the recorded results from `results/protocol_comparison.csv`.

| Protocol | Scenario | Packets | Retransmissions | Final RTO (ms) | Transfer |
|---|---|---:|---:|---|
| Stop-and-Wait | No loss | 4 | 0 | Completed |
| Stop-and-Wait | Packet loss (30%) | 4 | Yes | Completed |
| Stop-and-Wait | ACK loss (30%) | 4 | Yes | Completed |
| Go-Back-N | No loss | 12 | 0 | Completed |
| Go-Back-N | Packet loss (30%) | 12 | Yes | Completed |
| Go-Back-N | ACK loss (30%) | 12 | Yes | Completed |
| Selective Repeat | No loss | 10 | 1 | Completed |
| Selective Repeat | Packet loss (30%) | 10 | Yes | Completed |
| Selective Repeat | ACK loss (30%) | 10 | Yes | Completed |

### 9.2 Final RTO Measurements

The recorded final RTO values under the principal comparison conditions are:

| Protocol | No loss (ms) | Packet loss (ms) | ACK loss (ms) |
|---|---:|---:|---:|
| Stop-and-Wait | 3749.34 | 5307.76 | 5307.72 |
| Go-Back-N | 3359.64 | 4765.62 | 5032.69 |
| Selective Repeat | 4225.40 | 5052.19 | 5299.35 |

These values are measurements from the documented experimental runs. They represent the final estimated RTO for each run and should not be interpreted as universal or constant performance values for the protocols.

### 9.3 Go-Back-N Experiments

Go-Back-N was evaluated using a 4-packet sliding window and a 12-packet transfer.

The experiments included:

- No packet loss or acknowledgement loss.
- Packet loss.
- Acknowledgement loss.
- Combined packet and acknowledgement loss.

The recorded results show successful completion under the documented loss scenarios, with retransmissions and timeouts observed when network impairments were introduced.

### 9.4 Selective Repeat Experiments

Selective Repeat was evaluated using a 4-packet sliding window and a 10-packet transfer.

The experiments investigated:

- Normal transmission.
- Packet loss.
- Acknowledgement loss.
- Adaptive RTO under 500 ms delay.
- Retransmissions under delayed conditions.
- In-order delivery when an earlier packet was delayed.

In the in-order delivery experiment, later packets were received and acknowledged before an earlier delayed packet. The receiver buffered the out-of-order packets and delivered the data in sequence when the missing earlier packet arrived.

The recorded experiments demonstrate the selective retransmission and buffering behavior of the implementation.

## 10. Graphical Analysis

The project includes generated graphs to visualize selected experimental measurements.

### 10.1 Final RTO Comparison

File: `graphs/final_rto_comparison.svg`

This graph compares final RTO measurements across the three protocols under selected transmission conditions.

It helps visualize how timeout estimates differed between the documented experimental runs.

### 10.2 Packet Count Comparison

File: `graphs/packet_count_comparison.svg`

This graph compares the configured number of packets used in the protocol experiments:

- Stop-and-Wait: 4 packets.
- Go-Back-N: 12 packets.
- Selective Repeat: 10 packets.

This represents the experiment configurations and is **not a direct throughput measurement**.

## 11. Discussion

### 11.1 Reliability

The recorded experiments demonstrate successful transfer completion under the documented packet-loss and acknowledgement-loss conditions for the three protocols.

Reliability is achieved through sequence numbers, acknowledgements, retransmission timers, and retransmission mechanisms.

### 11.2 Retransmission Behavior

The protocols differ in how they respond to packet loss.

- **Stop-and-Wait** retransmits the outstanding packet when its acknowledgement is not received.
- **Go-Back-N** may retransmit multiple outstanding packets after a timeout.
- **Selective Repeat** retransmits individual packets whose acknowledgements are missing.

Selective Repeat can avoid retransmitting correctly received subsequent packets, while Go-Back-N uses a simpler cumulative acknowledgement and retransmission approach.

### 11.3 Sliding Windows

Go-Back-N and Selective Repeat use sliding windows to permit multiple outstanding packets.

This allows them to transmit more than one packet before waiting for acknowledgements, unlike Stop-and-Wait.

### 11.4 Adaptive Timeout

The recorded final RTO measurements vary across experimental scenarios.

This demonstrates that the adaptive timeout estimator responds to measured RTT conditions. However, the results are specific to the test configuration, packet count, and simulated network behavior.

## 12. Limitations

The project has several limitations:

- Experiments were conducted in a controlled environment using Windows and UDP sockets.
- The simulated channel does not represent every behavior of a real-world network.
- Experimental packet counts differ across protocols.
- The reported final RTO values depend on individual experimental runs.
- The packet-count graph shows configured packet counts rather than throughput.
- The Stop-and-Wait timeout/retransmission validation did not isolate the failure cause because the receiver was stopped.
- The experiments do not constitute a comprehensive performance evaluation across all possible network conditions.
- Further testing with varied window sizes, packet counts, network delays, and loss rates would provide a broader performance comparison.

## 13. Future Work

Possible improvements include:

- Conducting more extensive experiments with varying packet counts.
- Evaluating different sliding-window sizes.
- Testing a broader range of packet-loss and ACK-loss rates.
- Measuring throughput and end-to-end transfer time.
- Performing additional timeout and retransmission validation.
- Evaluating protocol behavior under higher network delays.
- Improving experiment automation and result collection.
- Extending testing to other operating systems and network environments.

## 14. Conclusion

This project demonstrates the implementation of reliable data transfer mechanisms over UDP using three ARQ protocols: Stop-and-Wait, Go-Back-N, and Selective Repeat.

The implementations incorporate packet sequencing, acknowledgements, checksum-based error detection, retransmission timers, adaptive RTO estimation, and channel emulation.

The experimental observations illustrate the different retransmission strategies used by the protocols. Stop-and-Wait provides a simple one-packet-at-a-time approach, Go-Back-N uses cumulative acknowledgements and window-based transmission, and Selective Repeat supports individual acknowledgements, selective retransmission, receiver buffering, and in-order delivery.

The project also demonstrates how simulated packet loss, acknowledgement loss, and delay affect protocol behavior and timeout estimation.

Overall, the project provides a practical demonstration of reliable transport mechanisms and the fundamental principles underlying reliable data transfer over an unreliable network service.

## 15. References

1. RFC 768 — User Datagram Protocol (UDP).
2. Computer Networks course material on reliable data transfer and ARQ protocols.
3. Project source code, experiment documentation, and recorded results maintained in this repository.

