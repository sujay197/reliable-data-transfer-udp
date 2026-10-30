\# Comparative Analysis of Reliable Data Transfer Protocols



\## 1. Objective



The objective of this experiment is to compare three reliable data transfer protocols implemented over UDP:



\- Stop-and-Wait ARQ

\- Go-Back-N ARQ

\- Selective Repeat ARQ



The comparison considers packet loss, acknowledgement loss, adaptive retransmission timeout (RTO), retransmission behavior, and successful in-order delivery.



\## 2. Experimental Configuration



| Parameter | Stop-and-Wait | Go-Back-N | Selective Repeat |

|---|---:|---:|---:|

| Transport | UDP | UDP | UDP |

| Total packets | 4 | 12 | 10 |

| Initial RTO | 2000 ms | 2000 ms | 2000 ms |

| Packet loss test | 30% | 30% | 30% |

| ACK loss test | 30% | 30% | 30% |

| In-order delivery | Yes | Yes | Yes |

| Transfer completed | Yes | Yes | Yes |



The experiments use different packet counts. Therefore, the results should be interpreted as observations from the tested configurations rather than a controlled throughput comparison.



\## 3. Results



\### 3.1 No-Loss Scenario



| Protocol | Final RTO (ms) | Transfer |

|---|---:|---|

| Stop-and-Wait | 3749.34 | Successful |

| Go-Back-N | 3359.64 | Successful |

| Selective Repeat | 4225.40 | Successful |



All three protocols completed their transfers without simulated packet or ACK loss.



\### 3.2 Packet-Loss Scenario



| Protocol | Packet Loss | Final RTO (ms) | Transfer |

|---|---:|---:|---|

| Stop-and-Wait | 30% | 5307.76 | Successful |

| Go-Back-N | 30% | 4765.62 | Successful |

| Selective Repeat | 30% | 5052.19 | Successful |



All three protocols recovered from the simulated packet-loss condition.



Go-Back-N retransmits outstanding packets from the current sender window after detecting missing acknowledgements. Selective Repeat retransmits individual packets whose acknowledgements are missing.



\### 3.3 ACK-Loss Scenario



| Protocol | ACK Loss | Final RTO (ms) | Transfer |

|---|---:|---:|---|

| Stop-and-Wait | 30% | 5307.72 | Successful |

| Go-Back-N | 30% | 5032.69 | Successful |

| Selective Repeat | 30% | 5299.35 | Successful |



All three protocols completed their transfers under simulated ACK loss.



Go-Back-N uses cumulative acknowledgements, whereas Selective Repeat uses individual acknowledgements and packet-specific retransmission behavior.



\## 4. Graphical Analysis



\### 4.1 Final Adaptive RTO Comparison



Refer to `graphs/final\_rto\_comparison.svg`.



The graph compares final adaptive RTO values under no-loss, packet-loss, and ACK-loss conditions.



The recorded final RTO values vary between scenarios and protocols. This demonstrates that the timeout estimator changed its value during the experiments.



A higher final RTO does not necessarily indicate worse protocol performance. It reflects the estimator's response to measured RTT samples and network conditions.



\### 4.2 Packet Count Comparison



Refer to `graphs/packet\_count\_comparison.svg`.



The graph compares the number of packets configured in the Week 5 experiments:



\- Stop-and-Wait: 4 packets

\- Go-Back-N: 12 packets

\- Selective Repeat: 10 packets



These are experiment configuration values, not throughput measurements.



\## 5. Protocol Comparison



\### Stop-and-Wait ARQ



Stop-and-Wait transmits one packet and waits for its acknowledgement before proceeding.



The experiments demonstrated normal transmission, checksum validation, invalid ACK rejection, duplicate packet detection, and timeout-based retry behavior.



Its window size of one limits the number of outstanding packets.



\### Go-Back-N ARQ



Go-Back-N allows multiple outstanding packets within a sender window.



It uses cumulative acknowledgements. When a packet is lost, the sender retransmits outstanding packets beginning from the current base.



The experiments demonstrated successful recovery under packet loss, ACK loss, and combined loss conditions.



\### Selective Repeat ARQ



Selective Repeat uses individual acknowledgements and buffers correctly received out-of-order packets.



It retransmits only packets whose acknowledgements are missing instead of retransmitting the entire outstanding window.



The experiments demonstrated selective retransmission, receiver buffering, adaptive RTO, and in-order delivery.



\## 6. Adaptive RTO and Karn's Algorithm



All three protocols have Week 5 adaptive RTO measurements recorded in the comparison dataset.



The implementation uses smoothed RTT and RTT deviation to calculate the retransmission timeout.



Karn's algorithm excludes retransmitted packets from RTT sampling to avoid ambiguous measurements.



The initial RTO for the recorded experiments was 2000 ms.



\## 7. Limitations



1\. The protocols were tested with different total packet counts.

2\. Exact retransmission counts are not available for every scenario.

3\. Some results record retransmission behavior qualitatively rather than numerically.

4\. The Stop-and-Wait validation experiments do not independently establish controlled packet-loss and ACK-loss measurements; those scenarios are represented by the Week 5 results.

5\. The packet-count graph represents configured experiment sizes, not throughput or efficiency.

6\. No controlled end-to-end throughput or CPU utilization comparison is included.



\## 8. Conclusion



All three reliable data transfer protocols successfully completed the recorded Week 5 no-loss, packet-loss, and ACK-loss experiments.



Stop-and-Wait provides a simple acknowledgement-based transfer mechanism. Go-Back-N improves transmission pipelining through cumulative acknowledgements. Selective Repeat provides individual acknowledgements and selective retransmission, with receiver buffering for out-of-order packets.



The experimental results demonstrate the operation of all three protocols and their recovery mechanisms under the tested network conditions.



A more rigorous performance comparison would require identical packet counts, payload sizes, network conditions, and measured throughput and retransmission counts across all protocols.

