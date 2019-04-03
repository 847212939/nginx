#pragma once
#include "ngx_cpp_global.h"
#include "ngx_cpp_log_message.h"

class CNgx_cpp_signal
{

public:
    static void ngx_signal_handler(int signo, siginfo_t * siginfo, void * ucontext);
    static void ngx_process_get_status();
    bool ngx_init_signals();
public:

    /*信号处理结构*/
    struct SNgx_signal_t
    {
        int             signo;                                  /*信号*/
        const char *    signame;                                /*信号中文名*/
        void     (*handler)(int, siginfo_t *, void *);          /*信号回调函数*/
    };

    static SNgx_signal_t ngx_signals[8];

public:
    CNgx_cpp_signal(){};
    ~CNgx_cpp_signal(){};
};