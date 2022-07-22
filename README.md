**Cppev is a C++ multithreading / multiprocessing / network / ipc library with high performance.**

# Architecture

### IO Handling

* Support disk-file / pipe / fifo / socket.

* Support socket protocol-type tcp / udp, protocol-family ipv4 / ipv6 / unix-domain.

* Support readable / writable event listening by using io-multiplexing.

### Concurrency

* Two kinds of thread pool : task-queue / variadic-template

* POSIX ipc : shared_memory / semaphore

* Subprocess similiar to Python

### Massive Tcp Connection Handling

Reactor similiar to Netty:

* Server: multi-acceptor-thread and io-thread-pool.

* Client: multi-connector-thread and io-thread-pool.

Five callbacks could be registered:

* on_accept: server accept connection (works only for server).

* on_connect: client establish connection (works only for client).

* on_read_complete: data read into rbuffer complete.

* on_write_complete: data write from wbuffer complete.

* on_closed: socket closed by opposite host.

# Prerequirement

* Unix: Linux / macOS.

* g++: Support C++11.

* others: googletest / cmake / make.

# Build and Run

    $ mkdir build && cd build
    $ cmake .. && make
    $ cd unittest && ctest

# Examples

* Tcp Pressure : Perform tcp/ip stress testing.

* Event Driven Nio : Demo how to use event loop for the nio classes.

* File Transfer : Transfer file with large size by tcp/ip.

# Issues and Maintenance

* Due to rapid development, regression issue may occur intermittently in the dev branch. Try git pull when you're blocked by compile/runtime issue.

* Repo will be long-term maintained, author will keep working on the todo-list. If you found any shortage or any new feature nice to have, feel free to raise an issue.

* First release may be in 2025 due to author need some time to make sure the architecture is well designed. So before this day API may be changed.
