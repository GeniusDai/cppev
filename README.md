**Cppev is a C++ event driven library.**

# Architecture

## Nonblock IO

* Support disk-file / pipe / fifo / inotify / socket.

* Support socket protocol-type tcp / udp, protocol-family ipv4 / ipv6 / unix-domain.

## Thread Pool

Two kinds of thread pool provided:

* 1) Custom type runnable by variadic templates.

* 2) Task queue for thread pool with stop function.

## Event Loop

* Support readable / writable event for fd, use io-multiplexing.

* Support event priority and multi-thread shared data.

## Tcp Socket Handle

* Server: acceptor-thread and io-thread-pool

* Client: connector-thread and io-thread-pool

## Others

* Async Logger: Support multi-thread write concurrently by rwlock, buffer outdate, backend-thread sleep, elegant destruction.

* Thread and Synchronization: Encapsulation of POSIX thread lib, runnable is a thread base class similiar to Java, provide rwlock and its RAII, spinlock.

# Prerequirement

* Linux: Support epoll/inotify/spinlock, and no bug in glibc.

* g++: Support C++11.

* others: googletest / cmake / make.
