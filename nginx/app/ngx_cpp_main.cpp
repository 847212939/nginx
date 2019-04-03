#include "ngx_cpp_main.h"


using namespace std;

int CNgx_cpp_main::m_ngx_proctype=-1;
int CNgx_cpp_main::m_ngx_reap=-1;
CNgx_cpp_log_message     CNgx_cpp_main::m_ngx_log;

CNgx_cpp_main::CNgx_cpp_main(int argv_length,int argc,char **argv)
:m_ngx_title(argv_length,argc,argv)
{
    m_ngx_pConfig = CNgx_cpp_config::get_m_pconfig();
}

int CNgx_cpp_main::ngx_main()
{
    if(!m_ngx_pConfig->config_load())
    {
        m_ngx_log.ngx_log_stderr(0,"CNgx_cpp_main::ngx_main()中的config_load()失败了");
        return -1;
    }
    m_ngx_title.ngx_init_setitle();
    m_ngx_log.ngx_log_init();
    ngx_set_m_ngx_log();
    if(!m_ngx_signal.ngx_init_signals())
        return -1;
    if(!m_ngx_daemon.ngx_daemon())
        return -1;
    m_ngx_process.ngx_master_process_cycle();
    
    return 0;
}

void CNgx_cpp_main::ngx_set_m_ngx_log()
{
    m_ngx_process.ngx_set_m_title(m_ngx_title);
}