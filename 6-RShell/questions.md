1. **How does the remote client determine when a command's output is fully received from the server, and what techniques can be used to handle partial reads or ensure complete message transmission?**  
**Answer:**  
   In our remote shell, we define a special end-of-message marker (like `RDSH_EOF_CHAR`) that the server appends to each response. The client reads from the TCP stream until it detects this marker, at which point it knows the command’s output is complete. Because TCP is a stream, we may receive data in partial chunks. To handle this, we typically:  
   - Accumulate data in a buffer until we see the EOF marker.  
   - Check for the marker at the end of each recv call.  
   - Reconstruct the complete message by concatenating chunks until the marker is found.  

2. **This week's lecture on TCP explains that it is a reliable stream protocol rather than a message-oriented one. Since TCP does not preserve message boundaries, how should a networked shell protocol define and detect the beginning and end of a command sent over a TCP connection? What challenges arise if this is not handled correctly?**  
**Answer:**  
   A networked shell protocol must explicitly define a way to mark the end of each command or output segment. It often uses a special character (e.g., `0x04`), a length prefix, or some delimiter-based strategy. If not handled correctly, partial data might arrive out of sync, leading to “message framing” issues. In practice, if we rely on `recv` returning the entire message in one call, we risk cutting messages in half or merging two commands into one. This can cause the shell to misinterpret or lose data.

3. **Describe the general differences between stateful and stateless protocols.**  
**Answer:**  
   - **Stateful Protocols** maintain some session or connection state between requests. Each request may depend on previous interactions (e.g., an FTP session keeps track of logged-in user state).  
   - **Stateless Protocols** treat each request independently with no memory of prior messages (e.g., HTTP/1.0 in its simplest form). Clients must include all necessary info (like session tokens) in each request if the server is to recognize them.

4. **Our lecture this week stated that UDP is "unreliable". If that is the case, why would we ever use it?**  
**Answer:**  
   Despite being “unreliable,” UDP is useful when:  
   - Speed is more prioritized than guaranteed delivery (e.g., real-time gaming, live streaming).  
   - Application-layer can tolerate or handle packet loss or reorder.  
   - Multicast/broadcast is needed (e.g., discovery protocols).  
   In these scenarios, the overhead of TCP’s reliability and congestion control might be too high, so UDP’s minimal protocol overhead is beneficial.

5. **What interface/abstraction is provided by the operating system to enable applications to use network communications?**  
**Answer:**  
   The OS provides the Berkeley sockets API or “sockets” in general. This abstraction lets applications create sockets, bind to addresses/ports, connect to remote hosts, send/receive data, and close connections. It is language independent and widely adopted across UNIX-like and other operating systems.