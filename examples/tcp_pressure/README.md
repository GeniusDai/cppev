# Tcp Pressure

This guide build a simple server and client step by step, while the client performs a cross-socket-family concurrency test for the server.

## Simple Server

The simple server just send a message to the client, then echo any message back to the client.

### Step 1. Define handler

On the socket accepted, put the message to the write buffer, then trying to send it. The async_write here means if the message is very long then would register writable event to do asynchrous write.

On the socket sys-buffer read completed, retrieve the message from read buffer, then put to the write buffer and trying to send it.

```
cppev::tcp_event_cb on_accept = [](std::shared_ptr<cppev::nsocktcp> iopt) -> void
{
    iopt->wbuf()->put("Cppev is a C++ event driven library");
    cppev::async_write(iopt);
    cppev::log::info << "write message to " << iopt->fd() << cppev::log::endl;
};

cppev::tcp_event_cb on_read_complete = [](std::shared_ptr<cppev::nsocktcp> iopt) -> void
{
    iopt->wbuf()->put(iopt->rbuf()->get());
    cppev::async_write(iopt);
};
```

### Step 2. Start Server

Use 32 io-threads to perform the handler, also implicitly there will one thread perform the accept operation. Set handlers to the server, start to listen in port with ipv6 network layer protocol.

```
int main()
{
    cppev::tcp_server server(32);
    server.set_on_accept(on_accept);
    server.set_on_read_complete(on_read_complete);
    server.listen(8888, cppev::family::ipv6);
    server.run();
    return 0;
}
```

## Simple Client

The simple client just echo any message back to the server.

### Step 1. Define Handler

Handler is similiar with the server, just we prompt a message to show our (may be) asynchrous write completed.

```
cppev::tcp_event_cb on_read_complete = [](std::shared_ptr<cppev::nsocktcp> iopt) -> void
{
    cppev::log::info << "receive message --> " << iopt->rbuf()->buf() << cppev::log::endl;
    iopt->wbuf()->put(iopt->rbuf()->get());
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    cppev::async_write(iopt);
};

cppev::tcp_event_cb on_write_complete = [](std::shared_ptr<cppev::nsocktcp> iopt) -> void
{
    cppev::log::info << "write message complete" << cppev::log::endl;
};
```

### Step 2. Start Client

Use 32 io-threads to perform the handler, also implicitly there will one thread perform the connect operation. Set handlers to the client, then use ipv4/ipv6 tcp sockets to perform the pressure test.

```
int main()
{
    cppev::tcp_client client(32);
    client.set_on_read_complete(on_read_complete);
    client.set_on_write_complete(on_write_complete);
    client.add("127.0.0.1", 8888, cppev::family::ipv4, 10000);
    client.add("::1", 8888, cppev::family::ipv6, 10000);
    client.run();
    return 0;
}
```

### Attention

Single or double stack is a concept involves DNS, here the server is seen as double stack since we use 127.0.0.1 / ::1 to connect, but the client is single stack since we specify the protocol family for the socket. So we cannot use ipv4 server since it doesn't support ipv6 client.

Don't forget to set the ulimit-params of your system since the socket number is much more than the default.
