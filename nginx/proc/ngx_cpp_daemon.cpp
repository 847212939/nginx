#include "ngx_cpp_daemon.h"
#include "ngx_cpp_global.h"
#include "ngx_cpp_config.h"
#include "ngx_cpp_main.h"

using namespace std;

bool CNgx_cpp_daemon::ngx_daemon()
{
    CNgx_cpp_config * pconfig=CNgx_cpp_config::get_m_pconfig();
    if(nullptr==pconfig)
    {
        CNgx_cpp_main::m_ngx_log.ngx_log_error_core(NGX_LOG_EMERG,0,"CNgx_cpp_daemon::ngx_daemon()中的CNgx_cpp_config::get_m_pconfig()失败");
        return false;
    }
    int nDaemon=pconfig->get_def_int("Daemon",0);
    if(nDaemon)
    {
        pid_t pid=0;
        pid=fork();
        while(1)
        {
            switch(pid)
            {
            case -1:
                CNgx_cpp_main::m_ngx_log.ngx_log_error_core(NGX_LOG_EMERG,errno,"CNgx_cpp_daemon::ngx_daemon()中的fork()失败");
                return false;
            case  0:
                CNgx_cpp_main::m_ngx_log.ngx_log_error_core(NGX_LOG_NOTICE,0,"守护进程[%P]创建",getpid());
                break;
            default:
                CNgx_cpp_main::m_ngx_log.ngx_log_error_core(NGX_LOG_NOTICE,0,"守护进程中父进程[%P]退出",getpid());
                exit(-1);
            }
            break;
        }
        if(-1 == setsid())
        {
            CNgx_cpp_main::m_ngx_log.ngx_log_error_core(NGX_LOG_WARN,errno,"CNgx_cpp_daemon::ngx_daemon()中的setsid()失败");
            return false;
        }
        m_fd=open("/dev/null",O_RDWR);
        if(-1 == m_fd)
        {
            CNgx_cpp_main::m_ngx_log.ngx_log_error_core(NGX_LOG_WARN,errno,"CNgx_cpp_daemon::ngx_daemon()中的open()失败");
            return false;
        }
        if(-1 == dup2(m_fd,STDIN_FILENO))
        {
            CNgx_cpp_main::m_ngx_log.ngx_log_error_core(NGX_LOG_WARN,errno,"CNgx_cpp_daemon::ngx_daemon()中的dup2()失败");
            return false;
        }
        if(-1 == dup2(m_fd,STDOUT_FILENO))
        {
            CNgx_cpp_main::m_ngx_log.ngx_log_error_core(NGX_LOG_WARN,errno,"CNgx_cpp_daemon::ngx_daemon()中的dup2()失败");
            return false;
        }
    }
    return true;
}