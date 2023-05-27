### CINNOX_HW

# File
/Server/listener.c: Listen to UDP messages and reply a same message back to the client.

/Client/sender.c: Send a UDP message to a server and display the echo message.

/Client/conf/conf.txt: A text file where can set server IP, port, message and maximum retry count. (The default location should be /conf/conf.txt under the sender)

# Build
Server: gcc -o \<listener name\> listener.c

Client: gcc -pthread -o \<sender name\> sender.c

# Run
Server: ./listener -ip \<target ip\> -port \<target port\>

Client: ./sender
