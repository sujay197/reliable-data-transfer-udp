# Selective Repeat ARQ Experiment

## Objective

To implement and test Selective Repeat ARQ over UDP with individual acknowledgements, receiver buffering, selective retransmission, adaptive RTO, channel emulation, and in-order delivery.

## Configuration

- Transport protocol: UDP
- Receiver port: 6026
- Total packets: 10
- Sender and receiver window size: 4
- Initial adaptive RTO: 2000 ms
- Channel emulator: packet loss, ACK loss, and delay supported
- Adaptive RTO: Enabled
- Karn's algorithm: Enabled
- Deliberate first-copy drop: SEQ=2

## Week 5 Tests

| Test | Packet Loss | ACK Loss | Delay | Result | Final RTO |
|---|---:|---:|---:|---|---:|
| No Loss | 0% | 0% | 0 ms | Success | 4225.40 ms |
| Packet Loss | 30% | 0% | 0 ms | Success | 5052.19 ms |
| ACK Loss | 0% | 30% | 0 ms | Success | 5299.35 ms |
| Adaptive RTO Delay | 0% | 0% | 500 ms | Success | 2655.53 ms |
| In-Order Delivery | 0% | 0% | 0 ms | Success | 4225.98 ms |

## No Loss

With channel packet and ACK loss disabled, the transfer completed successfully.

The receiver still deliberately discarded the first copy of SEQ=2 as part of the Selective Repeat test.

- Packets sent: 10
- Intentional SEQ=2 drop: Yes
- Selective retransmission: SEQ=2
- Packets acknowledged: 10/10
- In-order delivery: Yes
- Final RTO: 4225.40 ms
- Transfer completed: Yes

## Packet Loss

With 30% simulated packet loss, several packets required individual retransmission.

Observed retransmissions included SEQ=2, 3, 4, 5, 6, 7, and 8.

Unlike Go-Back-N, Selective Repeat retransmitted only packets whose acknowledgements were missing.

- Packet loss: 30%
- ACK loss: 0%
- Selective retransmissions: Yes
- Timeouts: Yes
- Retransmitted packets: SEQ=2,3,4,5,6,7,8
- Karn's algorithm: Applied
- Packets acknowledged: 10/10
- Final RTO: 5052.19 ms
- Transfer completed: Yes

## ACK Loss

With 30% simulated ACK loss and no packet loss, missing acknowledgements caused individual packet timers to expire.

The sender retransmitted only the affected packets and eventually received acknowledgements for all packets.

- Packet loss: 0%
- ACK loss: 30%
- Selective retransmissions: Yes
- Timeouts: Yes
- Packets acknowledged: 10/10
- Final RTO: 5299.35 ms
- Transfer completed: Yes

## Adaptive RTO Under Delay

The channel emulator was configured with 500 ms receiver-side delay and no packet or ACK loss.

Measured RTT samples included approximately 1027.56 ms and 1920.90 ms. The adaptive RTO changed from the initial 2000 ms to 2850.88 ms and then to 2655.53 ms.

- Packet loss: 0%
- ACK loss: 0%
- Configured delay: 500 ms
- Initial RTO: 2000.00 ms
- Measured RTT: 1027.56 ms, 1920.90 ms
- RTO updates: 2850.88 ms -> 2655.53 ms
- Packets acknowledged: 10/10
- Final RTO: 2655.53 ms
- Transfer completed: Yes

## In-Order Delivery

During testing, packets 3, 4, and 5 were acknowledged before SEQ=2.

The receiver buffered these out-of-order packets rather than delivering them immediately. After SEQ=2 was retransmitted and received, the buffered packets were delivered in order.

- Packets acknowledged: 10/10
- Out-of-order buffering: Yes
- Retransmitted packet: SEQ=2
- In-order delivery: Yes
- Final RTO: 4225.98 ms
- Transfer completed: Yes

## Selective Repeat vs Go-Back-N

Selective Repeat retransmits individual missing packets, while Go-Back-N retransmits the outstanding sequence starting from the current base.

The Week 5 tests demonstrated that Selective Repeat can retain correctly received out-of-order packets in the receiver buffer and deliver them in order after the missing packet arrives.

## Adaptive RTO and Karn's Algorithm

Adaptive RTO was integrated into the Selective Repeat sender.

Valid RTT samples update the smoothed RTT, RTT deviation, and resulting RTO. Packets retransmitted after timeout are excluded from RTT measurement using Karn's algorithm.

This allows the timeout value to adapt to changing channel delay while avoiding ambiguous RTT measurements from retransmitted packets.

## Conclusion

The Week 5 Selective Repeat implementation successfully combines individual acknowledgements, receiver buffering, selective retransmission, adaptive RTO, Karn's algorithm, channel emulation, and in-order delivery.

All tested transfers completed successfully under no-loss, packet-loss, ACK-loss, and delayed-channel conditions.
