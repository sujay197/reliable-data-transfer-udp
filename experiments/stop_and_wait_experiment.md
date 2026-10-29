# Stop-and-Wait ARQ Experiments

## Configuration

- Protocol: Stop-and-Wait ARQ
- Transport: UDP
- Receiver address: 127.0.0.1
- Window size: 1
- Packet format: Sequence number, checksum, and data
- Acknowledgement format: ACK with sequence number
- Timeout: 2 seconds for the timeout/retransmission test
- Maximum transmission attempts: 3

## Test Results

| Test | Purpose | Observed Result |
|---|---|---|
| Checksum Validation | Verify packet integrity and detect corrupted data | Checksum mismatch detected for invalid or corrupted packets |
| Invalid ACK Handling | Verify rejection of unexpected acknowledgements | Invalid ACK rejected; sender stopped after three attempts |
| Duplicate Packet Handling | Verify duplicate detection and acknowledgement recovery | Duplicate detected; ACK=0 resent |
| Normal Transmission | Verify successful data delivery and acknowledgement | `Hello` delivered; ACK=0 received |
| Timeout and Retransmission | Verify retry behavior when no ACK arrives | Sender timed out and failed after three attempts |

## Checksum Validation

The checksum tests evaluate whether the receiver validates incoming packet data before accepting it.

The corrupted-packet and wrong-checksum test screenshots show checksum mismatch detection. The malformed-packet test also shows rejection of an incorrectly formatted packet.

- Checksum validation: Tested
- Corrupted data: Detected
- Incorrect checksum: Rejected
- Malformed packet: Rejected

Detailed test evidence is associated with `results/checksum_test_results`.

## Invalid ACK and Duplicate Packet Handling

The invalid ACK test checks whether the sender accepts only the expected acknowledgement. The observed sender output shows that an invalid ACK is rejected and transmission stops after three attempts.

The duplicate-packet test sends the same sequence-numbered packet again. The receiver recognizes the duplicate and resends ACK=0 instead of treating the duplicate as new data.

- Invalid ACK: Rejected
- Retry limit: Three attempts
- Duplicate packet: Detected
- Duplicate acknowledgement: ACK=0 resent

Detailed test evidence is associated with `results/invalid_ack_duplicate_packet_test_results`.

## Timeout and Retransmission

The sender successfully transmitted `Hello` with sequence number 0. The receiver validated the checksum and sent ACK=0, and the sender reported successful transmission.

In the subsequent test, the sender transmitted `Hello` with sequence number 1 but did not receive an acknowledgement. It timed out, retried, and eventually reported `Transmission failed after 3 attempts`.

The receiver screenshot shows that the receiver was stopped with `Ctrl+C` after processing the first packet. Therefore, the second test demonstrates timeout and retry behavior, but does not establish that packet loss or ACK loss was deliberately simulated.

- Initial successful transmission: Observed
- ACK=0: Received
- Timeout: Observed
- Retry behavior: Observed
- Final status of second transmission: Failed after three attempts

Detailed test evidence is associated with `results/timeout_retransmission_test_results`.

## Conclusion

The Stop-and-Wait validation tests demonstrate normal packet acknowledgement, checksum validation, malformed-packet rejection, duplicate detection, invalid ACK handling, and timeout-based retry behavior.

The receiver rejects corrupted or incorrectly formatted packets and resends acknowledgements for duplicate packets. The sender rejects invalid ACKs and stops after its retry limit.

These tests do not establish measured packet-loss percentages, ACK-loss percentages, or adaptive RTO performance. Such measurements should be reported separately only if supported by experiment logs.