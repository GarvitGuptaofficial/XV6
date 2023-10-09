# Question1:

This is different from traditional TCP.
1. Protocol Type:
    1. TCP includes checksums and extensive error handling mechanisms. The provided code does not show evidence of incorporating these mechanisms.
    2. The actual TCP uses sliding window whereas we have used hash table for ACK management.
    3. TCP has sophisticated congestion control algorithms to adapt to network conditions and prevent network congestion. Our code does not include any congestion control mechanisms.
    4. TCP has a three way handshake SYN,SYN - ACK , ACK whereas our implementation has only SYN ACK hankshake.

# Question2:

1. Sender Side:
    1. Maintain a sender's window size, which determines how many packets can be in-flight (i.e., sent but not yet acknowledged) at a given time.
    2. Initialize the sender's window size to a reasonable value .
    3. Before sending a new packet, check if the number of unacknowledged packets in-flight is less than the sender's window size.
    4. If the condition is met, send the packet. Otherwise, wait until acknowledgments are received for some of the sent packets before sending more.

2. Receiver Side:
    1. Maintain a receiver's window size, which determines how many packets the receiver can buffer and process at a time.
    2. Initialize the receiver's window size to a reasonable value.
    3. Keep track of the expected sequence number of the next packet to be received.
    4. When a packet is received, check if its sequence number is equal to the expected sequence number of the next packet to be received.
    5. f the sequence number matches, process the packet, send an acknowledgment, and update expected sequence number. Otherwise, discard the packet (out of order).


