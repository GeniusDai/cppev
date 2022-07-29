#ifndef _tcp_pressure_config_h_6C0224787A17_
#define _tcp_pressure_config_h_6C0224787A17_

/*
 * Concurrency Number
 */
#ifdef __APPLE__
    const int IPV4_CONCURRENCY = 100;
    const int IPV6_CONCURRENCY = 100;
    const int UNIX_CONCURRENCY = 100;
#else
    const int IPV4_CONCURRENCY = 8000;
    const int IPV6_CONCURRENCY = 8000;
    const int UNIX_CONCURRENCY = 2000;
#endif

const int SERVER_WORKER_NUM = 32;
const int CLIENT_WORKER_NUM = 32;
const int CONTOR_NUM = 3;

const int   IPV4_PORT = 8884;
const int   IPV6_PORT = 8886;
const char* UNIX_PATH = "/tmp/cppev_test_6C0224787A17.sock";

#endif  // config.h
