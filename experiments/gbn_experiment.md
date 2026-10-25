# Go-Back-N ARQ Experiments

## Configuration

- Protocol: Go-Back-N ARQ
- Window size: 4
- Total packets: 12
- Transport: UDP
- Receiver: 127.0.0.1:6025
- Initial adaptive RTO: 2000 ms
- Channel emulator: packet loss, ACK loss, and delay supported
- Adaptive RTO: Enabled
- Karn's algorithm: Enabled

## Week 5 Tests

| Test | Packet Loss | ACK Loss | Delay | Result | Final RTO |
|---|---:|---:|---:|---|---:|
| No Loss | 0% | 0% | 0 ms | Success | 3359.64 ms |
| Packet Loss | 30% | 0% | 0 ms | Success | 4765.62 ms |
| ACK Loss | 0% | 30% | 0 ms | Success | 5032.69 ms |
| Combined Loss | 25% | 25% | 100 ms | Success | 4313.23 ms |

## No Loss

With packet loss and ACK loss disabled, all 12 packets were acknowledged successfully.

- Packets acknowledged: 12/12
- Retransmissions: None
- Initial RTO: 2000.00 ms
- Final RTO: 3359.64 ms
- Transfer completed: Yes

The adaptive RTO estimator updated its timeout value based on measured RTT samples.

## Packet Loss

With 30% simulated packet loss and no ACK loss, the receiver dropped packets during transmission.

The Go-Back-N sender detected missing acknowledgements through timeout and retransmitted all outstanding packets from the current base.

- Packet loss: 30%
- ACK loss: 0%
- Retransmissions: Yes
- Timeouts: Multiple
- Duplicate/old ACKs: Observed
- Karn's algorithm: Applied
- Packets acknowledged: 12/12
- Final RTO: 4765.62 ms
- Transfer completed: Yes

## ACK Loss

With 30% simulated ACK loss and no packet loss, some acknowledgements were not received by the sender.

Go-Back-N recovered using cumulative acknowledgements. When a timeout occurred, the sender retransmitted the outstanding packet(s), and the transfer eventually completed.

- Packet loss: 0%
- ACK loss: 30%
- Retransmissions: Yes
- Timeouts: Yes
- Cumulative ACK recovery: Yes
- Packets acknowledged: 12/12
- Final RTO: 5032.69 ms
- Transfer completed: Yes

## Combined Loss and Channel Delay

The channel emulator was also tested with packet loss, ACK loss, and 100 ms channel delay.

Multiple timeout and retransmission events occurred, but the sender eventually acknowledged all packets.

- Packet loss: 25%
- ACK loss: 25%
- Channel delay: 100 ms
- Retransmissions: Yes
- Timeouts: Multiple
- Karn's algorithm: Applied
- Packets acknowledged: 12/12
- Final RTO: 4313.23 ms
- Transfer completed: Yes

## Adaptive RTO

Adaptive RTO was integrated into the Go-Back-N sender.

The estimator updates the smoothed RTT and RTT deviation after valid RTT samples. Retransmitted packets are excluded from RTT measurement according to Karn's algorithm.

The RTO therefore changes according to observed network conditions rather than remaining fixed at the initial 2000 ms value.

## Conclusion

The Week 5 Go-Back-N implementation successfully combines cumulative acknowledgements, timeout-based retransmission, channel emulation, adaptive RTO, and Karn's algorithm.

All tested transfers completed successfully, including packet loss, ACK loss, combined loss, and delayed-channel conditions.
