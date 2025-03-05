**Cppev is a high performance C++ asyncio / multithreading / multiprocessing library.**

# Architecture

### IO

Support nonblocking-io of disk-file / pipe / fifo / socket.

Support socket protocol-type tcp / udp, protocol-family ipv4 / ipv6 / unix-domain.

Support io event listening by io-multiplexing, event-type readable / writable, event-mode level-trigger / edge-trigger / oneshot.

### Multithreading

Support subthread / threadpool / signal-handing / reactor.

### Interprocess Communication

Support semaphore / shared-memory.

Support mutex / condition-variable / read-write-lock shared among processes.

### Binary File Loading

Support executable-file loading by subprocess, dynamic-library loading in runtime.

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


### Build with bazelisk

Build

        $ bazel build  //...

Run Unittest

        $ bazel test //...

# Getting Started

Please see the examples along with a tutorial.
