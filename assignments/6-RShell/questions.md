1. How does the remote client determine when a command's output is fully received from the server, and what techniques can be used to handle partial reads or ensure complete message transmission?

The remote client knows a command's output is complete when it receives a termination marker like EOF or by using length-prefixed messages that specify exactly how many bytes to expect, and it can handle partial reads by buffering data until the complete message arrives

2. This week's lecture on TCP explains that it is a reliable stream protocol rather than a message-oriented one. Since TCP does not preserve message boundaries, how should a networked shell protocol define and detect the beginning and end of a command sent over a TCP connection? What challenges arise if this is not handled correctly?

A networked shell protocol over TCP should define message boundaries using either length prefixing or special delimiter sequences, and without proper boundary handling you'll face issues like command fragmentation where partial commands get executed or responses get mixed up

3. Describe the general differences between stateful and stateless protocols.

Stateful protocols maintain session info between requests while stateless ones treat each request as independent, kinda like the difference between having a conversation versus just shouting individual messages

4. Our lecture this week stated that UDP is "unreliable". If that is the case, why would we ever use it?

We use UDP despite being "unreliable" because it's way faster with less overhead, which makes it perfect for stuff like gaming, streaming, or VoIP where occasional packet loss isn't a big deal

5. What interface/abstraction is provided by the operating system to enable applications to use network communications?

Operating systems provide the socket interface as an abstraction for network communication, letting applications create and manage network connections without dealing with all the low-level details