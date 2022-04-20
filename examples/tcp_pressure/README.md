# Tcp Pressure

Build a simple server and client step by step, while the client performs a cross-socket-family concurrency test for the server.

### Attention

Single or double stack is a concept involves DNS, here the server is seen as double stack since we use 127.0.0.1 / ::1 to connect, but the client is single stack since we specify the protocol family for the socket. So we cannot use ipv4 server since it doesn't support ipv6 client.

Don't forget to set the ulimit-params of your system since the socket number is much more than the default.
