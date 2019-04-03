#pragma once
#include "ngx_cpp_config.h"
#include "ngx_cpp_setproctitle.h"
#include "ngx_cpp_log_message.h"
#include "ngx_cpp_signal.h"
#include "ngx_cpp_process_cycle.h"
#include "ngx_cpp_daemon.h"

class CNgx_cpp_main
{
public:
    int     ngx_main();
    void    ngx_set_m_ngx_log();
public:
    CNgx_cpp_main(int argv_length,int argc,char **argv);
    ~CNgx_cpp_main(){}
public:
    static int                      m_ngx_proctype;
    static int                      m_ngx_reap;
    static CNgx_cpp_log_message     m_ngx_log;
private:
    CNgx_cpp_setproctitle           m_ngx_title;    
    CNgx_cpp_config *               m_ngx_pConfig;
    CNgx_cpp_signal                 m_ngx_signal;
    CNgx_cpp_process_cycle          m_ngx_process;
    CNgx_cpp_daemon                 m_ngx_daemon;
};

