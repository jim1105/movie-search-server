## CLIENT/SERVER PROTOCOL
Whenever we have a client and server talking to each other, we define a protocol so both the server and client know what to expect from each other and therefore communicate effectively.

### Timeline/process:

* Server accepts connection
* Server sends ACK to client (using SendAck)
* Server recieves query term (as a string)
* Server sends client number of responses (as an int)
* Server receives ACK from client (sent using SendAck)
* For each response:
  * Server sends response to client (as a string)
  * Server receives ACK from client (sent using SendAck)
  * Server sends GOODBYE (sent using SendGoodbye)
  * Server and Client close the connection.
  
### This is represented in the diagram below:
![alt text](https://course.ccs.neu.edu/cs5007su19-seattle/assignments/queryprotocol.png)


### The above timeline was written in terms of the server. We can re-write it from the perspective of the client:

* Client calls connect
* Client recieves ACK
* Client sends query (as string)
* Client reads int describing number of results
* Client sends ACK (sent using SendAck)
* For each response:
  * Client reads response from server (as a string)
  * Client sends ACK to server (using SendAck)
  * Client reads GOODBYE from server
  * Client closes the connection and waits for another query
