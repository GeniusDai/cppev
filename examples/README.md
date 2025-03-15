# Examples Tutorial

### 1. Tcp Stress Test

Tcp server initiates listening thread(s) to support ipc4 / ipv6 / unix protocol family.

Tcp client initiates connecting thread(s) to connect to server.

Both server and client initiate thread pool to handle tcp connection.

* Usage

        $ cd examples/tcp_stress
        $ ./simple_server       # Shell-1
        $ ./simple_client       # Shell-2

### 2. Large File Transfer

Tcp client sends the required file' name.

Tcp server caches and transfers the required file.

Tcp client receives the file and stores it to disk.

* Usage

        $ cd example/file_transfer
        $ touch /tmp/test_cppev_file_transfer_6C0224787A17.file
        $ # Write to the file to make it large such as 20MB or more #
        $ ./file_transfer_server        # Shell-1
        $ ./file_transfer_client        # Shell-2
        $ openssl md5 /tmp/test_cppev_file_transfer_6C0224787A17.file*

### 3. IO Event Loop

Use the original event loop to connect via tcp / udp.

* Usage

        $ cd examples/io_evlp
        $ ./tcp_server      # Shell-1
        $ ./tcp_client      # Shell-2
        $ ./udp_server      # Shell-3
        $ ./udp_client      # Shell-4
