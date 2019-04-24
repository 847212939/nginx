#include "ngx_cpp_process_cycle.h"
#include "ngx_cpp_main.h"
#include "ngx_cpp_main.h"

using namespace std;

char CNgx_cpp_process_cycle::m_mastername[]="nginx master";

void CNgx_cpp_process_cycle::ngx_master_process_cycle()
{
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set,SIGHUP),     sigaddset(&set,SIGINT);
    sigaddset(&set,SIGTERM),    sigaddset(&set,SIGCHLD);
    sigaddset(&set,SIGQUIT),    sigaddset(&set,SIGIO);
    sigaddset(&set,SIGUSR1),    sigaddset(&set,SIGUSR2);
    sigaddset(&set, SIGALRM),   sigaddset(&set, SIGWINCH);
    if(-1==sigprocmask(SIG_BLOCK,&set,nullptr))
    {
        CNgx_cpp_main::m_ngx_log.ngx_log_error_core(NGX_LOG_EMERG,errno,
            "ngx_master_process_cycle()函数中的sigprocmask()错误");
        return ;
    }
    int     nLen=0;
    int     i=-1;
    char    buf[1000]{0};
    nLen=sizeof(m_mastername)+m_title.m_argc+m_title.m_argvcount;
    /*大于1000不给设置标题*/
    if(nLen<1000)
    {
        strcpy(buf,m_mastername);
        while(m_title.m_argv[++i])
        {
            strcat(buf," ");
            strcat(buf,m_title.m_argv[i]);
        }
    }
    m_title.ngx_setitile(buf);
    CNgx_cpp_config * pconfig=CNgx_cpp_config::get_m_pconfig();
    int nprocnum=pconfig->get_def_int("WorkerProcesses",1);
    CNgx_cpp_main::m_ngx_proctype=NGX_PROCESS_MASTER;
    ngx_start_worker_processes(nprocnum);
    sigemptyset(&set);
    while(1)
    {
        /*原子操作 阻塞到直到有信号过来*/
        sigsuspend(&set);
        if(CNgx_cpp_main::m_ngx_exit)
            break;
    }

}

void CNgx_cpp_process_cycle::ngx_start_worker_processes(int threadnums)
{
    int i=0;
    while(i<threadnums)
    {
        ngx_spawn_process(++i,"nginx worker");
    }
}

void CNgx_cpp_process_cycle::ngx_worker_process_cycle(int inum,const char *pprocname)
{
    ngx_worker_process_init(inum);
    m_title.ngx_setitile(pprocname);
    CNgx_cpp_main::m_ngx_log.ngx_log_error_core(NGX_LOG_NOTICE,0,"子进程[%P]创建",m_childid);
    CNgx_cpp_main::m_ngx_proctype=NGX_PROCESS_WORKER;
    int i = 0;
    while(1)
    {
        ngx_process_events_and_timers();
        if(CNgx_cpp_main::m_ngx_exit)
            break;
    }
}

int  CNgx_cpp_process_cycle::ngx_spawn_process(int threadnums,const char *pprocname)
{
    pid_t pid=fork();  
    switch(pid)
    {
    case 0:
        m_childid=getpid();
        ngx_worker_process_cycle(threadnums,pprocname);
        return m_childid;
    case -1:
        CNgx_cpp_main::m_ngx_log.ngx_log_error_core(NGX_LOG_EMERG,errno,
            "CNgx_cpp_process_cycle::ngx_worker_process_cycle()中的fork()出错");
        return -1;
    default:
        m_parentid=getpid();
        break;
    }
    return pid;
}

void CNgx_cpp_process_cycle::ngx_worker_process_init(int inum)
{
    sigset_t set;
    sigemptyset(&set);
    if(-1==sigprocmask(SIG_SETMASK,&set,nullptr))
    {
        CNgx_cpp_main::m_ngx_log.ngx_log_error_core(NGX_LOG_EMERG,errno,
            "CNgx_cpp_process_cycle::ngx_worker_process_init()sigprocmask()出错");
        return ;
    }
    CNgx_cpp_config * pconfig=CNgx_cpp_config::get_m_pconfig();
    int nThread = pconfig->get_def_int("ThreadPoolCount",1);
    CNgx_cpp_main::m_ngx_thread.ngx_create_threadpool(nThread);
    sleep(1);
    if(!CNgx_cpp_main::m_ngx_logic.ngx_initialize())
    {
        return ;
    }
    if(!CNgx_cpp_main::m_ngx_logic.ngx_epoll_init())
    {
        CNgx_cpp_main::m_ngx_log.ngx_log_error_core(NGX_LOG_EMERG,errno,
            "CNgx_cpp_process_cycle::ngx_worker_process_init()出错");
        return ;
    }
}

void CNgx_cpp_process_cycle::ngx_process_events_and_timers()
{
    CNgx_cpp_main::m_ngx_logic.ngx_epoll_process_events(-1);
}

void CNgx_cpp_process_cycle::ngx_set_m_title(CNgx_cpp_setproctitle & title)
{
    m_title=title;
}
