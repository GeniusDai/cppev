**Cppev is a high performance C++ asyncio / multithreading / multiprocessing library.**

# Architecture

### IO Handling

Support disk-file / pipe / fifo / socket.

Support socket protocol-type tcp / udp, protocol-family ipv4 / ipv6 / unix-domain.

Support readable / writable event handling by io-multiplexing.

### Concurrency Handling

Support thread-pool / subprocess / shared-memory / semaphore.

### Massive Tcp Connection Handling

Support reactor:

* Server: multi-acceptor-thread and io-thread-pool.

* Client: multi-connector-thread and io-thread-pool.

Support registering callbacks:

* on_accept: server accept connection (works only for server).

* on_connect: client establish connection (works only for client).

* on_read_complete: data read into rbuffer complete.

* on_write_complete: data write from wbuffer complete.

* on_closed: socket closed by opposite host.

# Usage

### Prerequirement

        OS           :  Linux / macOS
        Dependency   :  googletest

### Build with cmake

Build

        $ mkdir build && cd build
        $ cmake .. && make

Install

        $ make install

Run Unittest

        $ cd unittest && ctest


### Build with bazel

Build

        $ bazel build  //...

Run Unittest

        $ bazel test //...

### Validated platform

Ubuntu-22.04 / CentOS-8 / macOS-Monterey.

# Getting Started

Please see the examples along with a tutorial.

# Issues and Maintenance

Repo will be long-term maintained, author will keep working on the todo-list. If you found any shortage or any new feature nice to have, feel free to raise an issue.
