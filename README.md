**Cppev is a high performance C++ asyncio / multithreading / multiprocessing library.**

# Architecture

### IO

Support io operations of disk-file / pipe / fifo / socket (socket protocol-type tcp / udp, protocol-family ipv4 / ipv6 / unix-domain).

Support io event listening by io-multiplexing, event-type readable / writable, event-mode level-trigger / edge-trigger / oneshot.

### Multithreading

Support subthread / threadpool.

### Interprocess Communication

Support semaphore / shared-memory.

### Thread and Process Synchronization

Support thread / process level signal-handing / mutex / condition-variable / read-write-lock.

### Binary File Loading

Support executable-file loading by subprocess, dynamic-library loading in runtime.

### Reactor

Support tcp server / client by multi-threading / nonblocking-io / level-trigger-event-listening.

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
