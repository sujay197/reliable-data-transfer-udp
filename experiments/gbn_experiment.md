# Go-Back-N ARQ Experiments

## Configuration

- Protocol: Go-Back-N ARQ
- Window size: 4
- Total packets: 12
- Transport: UDP
- Receiver: 127.0.0.1:6025
- Timeout: 2 seconds

## Experiments

| Experiment | Packet Loss | ACK Loss | Timeouts | Result |
|---|---:|---:|---:|---|
| E1 | 0% | 0% | 0 | Success |
| E2 | 25% | 0% | 1 | Success |
| E3 | 0% | 25% | 0 | Success |
| E4 | 25% | 25% | 3 | Success |

## E1 - No Loss

Packet loss was set to 0% and ACK loss was set to 0%.

All 12 packets were acknowledged without timeout or retransmission.

Final sender state:

- Base: 12
- Next sequence number: 12
- Result: Successful

## E2 - Packet Loss Only

Packet loss was set to 25% and ACK loss was set to 0%.

Packet 10 was simulated as lost. Packet 11 subsequently arrived out of order and the receiver sent the previous cumulative ACK.

The sender timed out and retransmitted packets 10 and 11.

Final sender state:

- Base: 12
- Next sequence number: 12
- Timeouts: 1
- Result: Successful

## E3 - ACK Loss Only

Packet loss was set to 0% and ACK loss was set to 25%.

ACK=1 was deliberately lost by the receiver. A later cumulative ACK=2 allowed the sender to advance its base beyond packet 1 without requiring a timeout.

Final sender state:

- Base: 12
- Next sequence number: 12
- Timeouts: 0
- Result: Successful

## E4 - Combined Packet and ACK Loss

Packet loss was set to 25% and ACK loss was set to 25%.

Both packet loss and ACK loss occurred. The receiver rejected out-of-order packets and returned previous cumulative ACKs. The sender performed Go-Back-N retransmissions after timeouts.

Three timeout events occurred at bases 3, 4, and 8.

Final sender state:

- Base: 12
- Next sequence number: 12
- Timeouts: 3
- Result: Successful

## Conclusion

All four experiments completed successfully.

The experiments demonstrate that the Go-Back-N implementation can recover from packet loss and ACK loss using cumulative acknowledgements, timeout detection, and retransmission of outstanding packets.

Higher combined loss resulted in more timeout and retransmission activity, while the final transfer remained reliable.
