#include "ngx_cpp_memery.h"

CNgx_cpp_memery *CNgx_cpp_memery::m_ngx_memery = nullptr;

CNgx_cpp_memery *CNgx_cpp_memery::ngx_get_memery()
{
    if(nullptr == m_ngx_memery)
    {    
        /*双重锁定*/
        if(nullptr == m_ngx_memery)
        {
            static CNgx_recycle recycle;
            m_ngx_memery = new CNgx_cpp_memery;
            return m_ngx_memery;
        }
        /*解锁*/
    }
    return m_ngx_memery;
}

void *CNgx_cpp_memery::ngx_alloc_memery(int nMemCount,bool bIfmemset)
{
    char *p = nullptr;
    if(bIfmemset)
        p = new char[nMemCount]{0};
    else
        p = new char[nMemCount];
    return (void *)p;
}

void CNgx_cpp_memery::ngx_relase_memery(void * p)
{
    if(p)
    {
        delete []((char *)p);
    }
}