**Cppev is a high performance C++ asyncio / multithreading / multiprocessing library.**

# Architecture

### IO

Support nonblocking-io of disk-file / pipe / fifo / socket.

Support socket protocol-type tcp / udp, protocol-family ipv4 / ipv6 / unix-domain.

Support readable / writable event handling by io-multiplexing.

### Concurrency

Support thread / thread-pool.

Support subprocess / semaphore / shared-memory / pshared-locks.

### Others

Support reactor / dynamic-loader / task-scheduler / signal-handing / async-logger.

# Usage

### Prerequisite

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

### Validated platforms

Ubuntu-20.04 / CentOS-8 / macOS-Monterey.

# Getting Started

Please see the examples along with a tutorial.

# Issues and Maintenance

Repo will be long-term maintained, author will keep working on the todo-list. If you found any shortage or any new feature nice to have, feel free to raise an issue.
