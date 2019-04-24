#pragma once 
#include "ngx_cpp_log_message.h"
#include "ngx_cpp_setproctitle.h"
#include "ngx_cpp_config.h"

class CNgx_cpp_process_cycle
{
public:
    void ngx_master_process_cycle();
    void ngx_set_m_title(CNgx_cpp_setproctitle & title);
public:
    CNgx_cpp_process_cycle():m_parentid(0),m_childid(0){}
    ~CNgx_cpp_process_cycle(){}
private:
    void ngx_process_events_and_timers();
    void ngx_start_worker_processes(int threadnums);
    int  ngx_spawn_process(int threadnums,const char *pprocname);
    void ngx_worker_process_cycle(int inum,const char *pprocname);
    void ngx_worker_process_init(int inum);
private:
    static char             m_mastername[];
    CNgx_cpp_setproctitle   m_title;
    pid_t                   m_parentid;
    pid_t                   m_childid;
};