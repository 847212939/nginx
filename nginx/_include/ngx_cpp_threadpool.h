#pragma once
#include "ngx_cpp_global.h"


class CNgx_cpp_threadpool
{
public:
    bool ngx_create_threadpool(int nThread);
    void ngx_in_recv_and_signal(char *pBuf);
    void ngx_thread_pool_exit();
public:
    CNgx_cpp_threadpool();
    ~CNgx_cpp_threadpool();
public:
    static pthread_mutex_t      m_mutex;
    static pthread_cond_t       m_cond;
private:
    static void *ngx_threadProc(void * pArg);
private:
    void ngx_call();
private:
    struct SNgx_thread
    {
        pthread_t               st_handle;
        CNgx_cpp_threadpool    *st_this;
        bool                    st_bRuning;

        SNgx_thread(CNgx_cpp_threadpool * p):st_this(p),st_bRuning(false){}
        ~SNgx_thread(){}
    };
private:
    CNgx_cpp_log_message        m_log;
    std::list<SNgx_thread *>    m_listSt;
    std::list<char *>           m_listBuf;
    std::atomic<int>            m_nListBuf;
    time_t                      m_nRunTime;
    int                         m_nThread;
    int                         m_nThreadPool;
};
