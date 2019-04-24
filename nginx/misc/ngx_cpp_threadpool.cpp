#include "ngx_cpp_threadpool.h"
#include "ngx_cpp_main.h"
#include "ngx_cpp_memery.h"

pthread_mutex_t CNgx_cpp_threadpool::m_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  CNgx_cpp_threadpool::m_cond = PTHREAD_COND_INITIALIZER;

bool CNgx_cpp_threadpool::ngx_create_threadpool(int nThread)
{
    SNgx_thread *pThread = nullptr;
    int i = -1;
    int nErrno = -1;
    m_nThreadPool = nThread;
    while(++i < nThread)
    {
        m_listSt.push_back(pThread = new SNgx_thread(this));
        if((nErrno = pthread_create(&pThread->st_handle,nullptr,ngx_threadProc,pThread)))
        {
            CNgx_cpp_main::m_ngx_log.ngx_log_error_core(NGX_LOG_ERR,nErrno,
                "CNgx_cpp_threadpool::ngx_create_threadpool()中的pthread_create()出错");
            return false;
        }
    }
/*goto 语句是要是在来一次 就是到m_lis在循环一次去判断是否有false的存在要是这样简直是妙用呀*/
loopagain:

    for(SNgx_thread *p : m_listSt)
    {
        /*睡100毫秒 让线程都能到条件变量哪卡着*/
        if(!p->st_bRuning)
        {
            usleep(100*1000);
            goto loopagain;
        }
    }
    return true;
}

void *CNgx_cpp_threadpool::ngx_threadProc(void * pArg)
{
    int nErrno = -1;
    SNgx_thread *pThread = (SNgx_thread *)pArg;
    CNgx_cpp_threadpool *pt = pThread->st_this;
    while(true)
    {
        if(nErrno = pthread_mutex_lock(&m_mutex))
        {
            CNgx_cpp_main::m_ngx_log.ngx_log_error_core(NGX_LOG_ERR,nErrno,
                "CNgx_cpp_threadpool::ngx_create_threadpool()中的pthread_mutex_lock()出错");
        }
        while(false == CNgx_cpp_main::m_ngx_exit && 0 == pt->m_listBuf.size())
        {
            if(!pThread->st_bRuning)
                pThread->st_bRuning = true;
            if(nErrno = pthread_cond_wait(&m_cond,&m_mutex))
            {
                CNgx_cpp_main::m_ngx_log.ngx_log_error_core(NGX_LOG_ERR,nErrno,
                    "CNgx_cpp_threadpool::ngx_create_threadpool()中的pthread_cond_wait()出错");
            }
        }
        ++pt->m_nThread;
        pt->m_nRunTime = time(nullptr);
        if(CNgx_cpp_main::m_ngx_exit)
        {
            pthread_mutex_unlock(&m_mutex);
            break;
        }
        char *pBuf = pt->m_listBuf.front();
        pt->m_listBuf.pop_front();
        pt->m_nListBuf--;
        if(nErrno = pthread_mutex_unlock(&m_mutex))
        {
            CNgx_cpp_main::m_ngx_log.ngx_log_error_core(NGX_LOG_ERR,nErrno,
                "CNgx_cpp_threadpool::ngx_create_threadpool()pthread_mutex_unlock()出错");
        }
        /*线程中业务逻辑代码*/
        /*抛到CNgx_cpp_logic类中 对包解包数据的处理*/
        CNgx_cpp_main::m_ngx_logic.ngx_process_data(pBuf);
        /*用过了给释放了*/
        CNgx_cpp_memery *pMem = CNgx_cpp_memery::ngx_get_memery();
        pMem->ngx_relase_memery(pBuf);

    }
    
    return (void *)0;
}

void CNgx_cpp_threadpool::ngx_in_recv_and_signal(char *pBuf)
{
    int nErrno = -1;
    if(nErrno = pthread_mutex_lock(&m_mutex))
    {
        CNgx_cpp_main::m_ngx_log.ngx_log_error_core(NGX_LOG_ERR,nErrno,
            "CNgx_cpp_threadpool::ngx_in_recv_and_signal()中的pthread_mutex_lock()出错");
    }
    m_listBuf.push_back(pBuf);
    m_nListBuf++;
    if(nErrno = pthread_mutex_unlock(&m_mutex))
    {
        CNgx_cpp_main::m_ngx_log.ngx_log_error_core(NGX_LOG_ERR,nErrno,
            "CNgx_cpp_threadpool::ngx_in_recv_and_signal()pthread_mutex_unlock()出错");
    }
    ngx_call();
}

void CNgx_cpp_threadpool::ngx_call()
{
    int nErrno = -1;
    if(m_nListBuf <= 0)
        return ;
    if(nErrno = pthread_cond_signal(&m_cond))
    {
        CNgx_cpp_main::m_ngx_log.ngx_log_error_core(NGX_LOG_ERR,nErrno,
            "CNgx_cpp_threadpool::ngx_in_recv_and_signal()pthread_mutex_unlock()出错");
    }
    /*检测线程池中的数量够用吗 不够用提醒一下*/
    if(m_nThreadPool == m_nThread)
    {
        time_t nCurTimme = time(nullptr);
        if(m_nRunTime - nCurTimme > 10)
        {
            CNgx_cpp_main::m_ngx_log.ngx_log_error_core(NGX_LOG_INFO,0,"线程池中线程不够用了");
        }
    }
}

void CNgx_cpp_threadpool::ngx_thread_pool_exit()
{
    int nErrno = -1;
    if(nErrno = pthread_cond_broadcast(&m_cond))
    {
        CNgx_cpp_main::m_ngx_log.ngx_log_error_core(NGX_LOG_ERR,nErrno,
            "CNgx_cpp_threadpool::ngx_thread_pool_exit()中pthread_cond_broadcast()出错");
        return ;
    }
tryagain:
    for(SNgx_thread * pThread : m_listSt)
    {
        if(nErrno = pthread_join(pThread->st_handle,nullptr))
        {
            CNgx_cpp_main::m_ngx_log.ngx_log_error_core(NGX_LOG_ERR,nErrno,
                "CNgx_cpp_threadpool::ngx_thread_pool_exit()中pthread_join()出错");
            goto tryagain;
        }
    }
    for(SNgx_thread * pThread : m_listSt)
    {
        if(pThread)
            delete pThread;
    }
    CNgx_cpp_main::m_ngx_log.ngx_log_error_core(NGX_LOG_NOTICE,0,
            "CNgx_cpp_threadpool::ngx_thread_pool_exit()进程中【%P】线程池被清理了",getpid());
    m_listBuf.clear();
}

CNgx_cpp_threadpool::CNgx_cpp_threadpool()
{
    m_nThread = 0;
    m_nThreadPool = 0;
    m_nListBuf = 0;
}

CNgx_cpp_threadpool::~CNgx_cpp_threadpool()
{
    CNgx_cpp_memery *pMem = CNgx_cpp_memery::ngx_get_memery();
    if(CNgx_cpp_main::m_ngx_exit)
        ngx_thread_pool_exit();
    for(char *pBuf : m_listBuf)
    {
        if(pBuf)
            pMem->ngx_relase_memery(pBuf);
    }
    pthread_mutex_destroy(&m_mutex);
    pthread_cond_destroy(&m_cond);
}
