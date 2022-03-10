**Cppev is a C++ event driven library.**

# Architecture

### Nonblock IO

* Support disk-file / pipe / fifo / socket.

* Support socket protocol-type tcp / udp, protocol-family ipv4 / ipv6 / unix-domain.

### Event Loop

* Support readable / writable event for fd, using io-multiplexing.

* Support event priority and multi-thread shared data.

### Thread Pool

Two kinds of thread pool provided:

* Custom type runnable by variadic template.

* Task queue for thread pool with stop function.

### Tcp Socket Handle

Reactor similiar to netty:

* Server: acceptor-thread and io-thread-pool

* Client: connector-thread and io-thread-pool

Five callbacks could be registered:

* on_accept: server accept connection (works only for server)

* on_connect: client establish connection (works only for client)

* on_read_complete: data read into rbuffer complete

* on_write_complete: data write from wbuffer complete

* on_closed: peer closed socket

### Others

* Async Logger: Two kinds of logger both using backend thread.

* Thread and Synchronization: Encapsulation of pthread, rwlock and its RAII.

* Subprocess: Encapsulation of fork + execve + waitpid.

* Linux Peculiar: Support inotify and spinlock.

# Prerequirement

* Unix: Linux / macOS

* g++: Support C++11.

* others: googletest / cmake / make.

# Build and Run

    $ mkdir build && cd build
    $ cmake .. && make
    $ cd unittest && ctest

# Examples

* Tcp Pressure: Perform cross socket family pressure test for ipv6 server.

* Event Driven Nio: Demo how to use event loop for the nio classes.

# Issues and Maintenance

* Due to rapid development, regression issue may occur intermittently in the dev branch. Try git pull when you're blocked by compile/runtime issue.

* Repo will be long-term maintained, author will keep working on the todo-list. If you found any shortage or any new feature nice to have, feel free to raise an issue.
