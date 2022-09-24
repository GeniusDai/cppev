**Cppev is a high performance C++ asyncio / multithreading / multiprocessing library.**

# Architecture

### IO Handling

Support disk-file / pipe / fifo / socket.

Support socket protocol-type tcp / udp, protocol-family ipv4 / ipv6 / unix-domain.

Support readable / writable event handling by io-multiplexing.

### Concurrency Handling

Support thread-pool / shared-memory / semaphore / subprocess.

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

        OS           :  Linux / macOS.
        Compiler     :  g++ support c++17.
        Dependency   :  googletest.

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

        $ bazel build ...

Run Unittest

        $ bazel test ...

### About the compilation

In Ubuntu-22.04 / macOS-Monterey, build passed.

In CentOS-8, "-lrt" is needed and "std::filesystem" need to be replaced.

# Getting Started

Please see the examples along with a tutorial.

# Issues and Maintenance

Repo will be long-term maintained, author will keep working on the todo-list. If you found any shortage or any new feature nice to have, feel free to raise an issue.

First release may be in 2024 due to author need some time to make sure the architecture is well designed. So before this day API may be changed.
