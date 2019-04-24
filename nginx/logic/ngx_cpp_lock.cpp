#include "ngx_cpp_lock.h"
#include "ngx_cpp_main.h"

CNgx_cpp_lock::CNgx_cpp_lock(pthread_mutex_t *mutex)
{
    int nErrno = -1;
    m_mutex = mutex;
    if(nErrno = pthread_mutex_lock(m_mutex))
    {
        CNgx_cpp_main::m_ngx_log.ngx_log_error_core(NGX_LOG_DEBUG,nErrno,
        "CNgx_cpp_lock::CNgx_cpp_lock()中的pthread_mutex_lock()错了");
    }
}

CNgx_cpp_lock::~CNgx_cpp_lock()
{
    int nErrno = -1;
    if(nErrno = pthread_mutex_unlock(m_mutex))
    {
        CNgx_cpp_main::m_ngx_log.ngx_log_error_core(NGX_LOG_DEBUG,nErrno,
        "CNgx_cpp_lock::CNgx_cpp_lock()中的pthread_mutex_unlock()错了");
    }
}