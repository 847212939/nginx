#include "ngx_cpp_signal.h"
#include "ngx_cpp_main.h"

using namespace std;
CNgx_cpp_signal::SNgx_signal_t CNgx_cpp_signal::ngx_signals[]=
{
    {SIGHUP,    "SIGHUP",           ngx_signal_handler},
    {SIGINT,    "SIGINT",           ngx_signal_handler},
    {SIGTERM,   "SIGTERM",          ngx_signal_handler},
    {SIGCHLD,   "SIGCHLD",          ngx_signal_handler},
    {SIGQUIT,   "SIGQUIT",          ngx_signal_handler},
    {SIGIO,     "SIGIO",            ngx_signal_handler},
    //{SIGSEGV,   "SIGSEGV",          ngx_signal_handler},
    {SIGSYS,    "SIGSYS, SIG_IGN",  NULL              },
    {0,          NULL,              NULL              }
};

bool CNgx_cpp_signal::ngx_init_signals()
{
    int i=-1;
    struct sigaction sa;
    while(CNgx_cpp_signal::ngx_signals[++i].signo)
    {
        if(ngx_signals[i].handler)
        {
            sa.sa_sigaction=ngx_signals[i].handler;
            sa.sa_flags=SA_SIGINFO;
        }
        else 
        {
            sa.sa_handler=SIG_IGN;
        }
        /*不想阻塞任何信号，临时信号屏蔽集用过就失效了*/
        sigemptyset(&sa.sa_mask);
        if(-1==sigaction(ngx_signals[i].signo,&sa,nullptr))
        {
            CNgx_cpp_main::m_ngx_log.ngx_log_error_core(NGX_LOG_EMERG,errno,
                "sigaction(%s) failed",ngx_signals[i].signame);
            return false;
        }
    }
    return true;
}

void CNgx_cpp_signal::ngx_signal_handler(int signo, siginfo_t * siginfo, void * ucontext)
{
    int i=-1;
    char *action;
    while(ngx_signals[++i].signo)
    {
        if(signo==ngx_signals[i].signo)
        {
            break;
        }
    }
    action=(char *)"";
    if(CNgx_cpp_main::m_ngx_proctype==NGX_PROCESS_MASTER)
    {
        switch(signo)
        {
        case SIGCHLD:
            CNgx_cpp_main::m_ngx_reap=1;
            break;
        default:
            break;
        }
    }
    else if(CNgx_cpp_main::m_ngx_proctype==NGX_PROCESS_WORKER)
    {
        if(signo == SIGSEGV)
        {
            CNgx_cpp_main::m_ngx_log.ngx_log_error_core(0,errno,"111111111111111");
            return ;
        }
    }
    else
    {

    }
    if(siginfo && siginfo->si_pid)
    {
        CNgx_cpp_main::m_ngx_log.ngx_log_error_core(NGX_LOG_NOTICE,0,
            "signal %d (%s) received from %P%s", 
        signo, ngx_signals[i].signame, siginfo->si_pid, action); 
    }
    else
    {
        CNgx_cpp_main::m_ngx_log.ngx_log_error_core(NGX_LOG_NOTICE,0,
            "signal %d (%s) received %s",
        signo, ngx_signals[i].signame, action);
    }
    if(signo==SIGCHLD)
    {
        ngx_process_get_status();
    }

}

void CNgx_cpp_signal::ngx_process_get_status()
{
    pid_t            pid;
    int              status;
    int              err;
    int              one=0; /*抄自官方nginx，应该是标记信号正常处理过一次*/

    while(1) 
    {
        pid = waitpid(-1, &status, WNOHANG);
        if(pid == 0) //子进程没结束，会立即返回这个数字，但这里应该不是这个数字【因为一般是子进程退出时会执行到这个函数】
        {
            return;
        }
        if(pid == -1)//这表示这个waitpid调用有错误，有错误也理解返回出去，我们管不了这么多
        {
            //这里处理代码抄自官方nginx，主要目的是打印一些日志。考虑到这些代码也许比较成熟，所以，就基本保持原样照抄吧；
            err = errno;
            if(err == EINTR)           //调用被某个信号中断
            {
                continue;
            }

            if(err == ECHILD  && one)  //没有子进程
            {
                return;
            }

            if (err == ECHILD)         //没有子进程
            {
                CNgx_cpp_main::m_ngx_log.ngx_log_error_core(NGX_LOG_INFO,err,"waitpid() failed!");
                return;
            }
            CNgx_cpp_main::m_ngx_log.ngx_log_error_core(NGX_LOG_ALERT,err,"waitpid() failed!");
            return;
        }
        //-------------------------------
        //走到这里，表示  成功【返回进程id】 ，这里根据官方写法，打印一些日志来记录子进程的退出
        one = 1;  //标记waitpid()返回了正常的返回值
        if(WTERMSIG(status))  //获取使子进程终止的信号编号
        {
            CNgx_cpp_main::m_ngx_log.ngx_log_error_core(NGX_LOG_ALERT,0,
                "pid = %P exited on signal %d!",pid,WTERMSIG(status)); //获取使子进程终止的信号编号
        }
        else
        {
            CNgx_cpp_main::m_ngx_log.ngx_log_error_core(NGX_LOG_NOTICE,0,
                "pid = %P exited with code %d!",pid,WEXITSTATUS(status)); //WEXITSTATUS()获取子进程传递给exit或者_exit参数的低八位
        }
    }
    return;
}
