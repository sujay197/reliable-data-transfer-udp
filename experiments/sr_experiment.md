# Selective Repeat ARQ Experiment

## Objective

To implement and test Selective Repeat Automatic Repeat reQuest (ARQ) over UDP, demonstrating individual packet acknowledgements, receiver buffering, timeout-based retransmission, and in-order delivery.

## Configuration

- Transport protocol: UDP
- Receiver port: 6026
- Total packets: 10
- Sender and receiver window size: 4
- Retransmission timeout: 2000 ms
- Packet-loss simulation: The receiver deliberately drops the first copy of packet 2 without sending an acknowledgement.

## Procedure

1. Start the Selective Repeat receiver.
2. Start the sender.
3. Observe the individual acknowledgements for received packets.
4. Verify that the receiver buffers packets received out of order.
5. Observe the sender retransmitting packet 2 after its acknowledgement is not received before the timeout.
6. Verify that the receiver delivers buffered packets in order after the missing packet arrives.
7. Confirm that the sender finishes after receiving acknowledgements for all packets.

## Expected Observations

- Packets are acknowledged individually rather than using cumulative acknowledgements.
- Out-of-order packets within the receive window can be buffered.
- The first copy of packet 2 is deliberately discarded.
- Only the unacknowledged packet should need retransmission when its timer expires.
- Buffered packets are delivered in order when the missing packet arrives.
- The transfer should complete after all packets are acknowledged.

## Results

Record the actual output of the sender and receiver to confirm successful completion, selective retransmission, and in-order delivery.

## Conclusion

The experiment demonstrates how Selective Repeat ARQ can recover from packet loss by retransmitting missing packets individually while retaining correctly received out-of-order packets at the receiver.
