# Exmaples Tutorial

### 1. Tcp Stress Test

In this example, tcp server initiates three listening threads to support ipc4 / ipv6 /unix protocol family, tcp client uses thread pool to connect and communicate with server.

* Usage

        $ cd examples/tcp_stress
        $ ./simple_server       # Shell-1
        $ ./simple_client       # Shell-2

### 2. File Transfer

In this example, tcp client send the name of the required file, tcp server caches and transfers the required file, then client receives and stores the file to disk.

* Usage

        $ cd example/file_transfer
        $ touch /tmp/test_cppev_file_transfer_6C0224787A17.file
        $ # Write to the file to make it large such as 20MB or more #
        $ ./file_transfer_server        # Shell-1
        $ ./file_transfer_client        # Shell-2
        $ ls -l /tmp/test_cppev_file_transfer_6C0224787A17.file*
        $ # md5sum to check the file content is all the same #

### 3. Nio Event Loop

In this example, use the original event loop (not reactor) to connect via tcp /udp.

* Usage

        $ cd examples/nio_evlp
        $ ./tcp_server      # Shell-1
        $ ./tcp_client      # Shell-2
        $ ./udp_server      # Shell-3
        $ ./udp_client      # Shell-4
