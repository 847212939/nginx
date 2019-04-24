#pragma once

class CNgx_cpp_memery
{
public:
    static CNgx_cpp_memery *m_ngx_memery;
    void  *ngx_alloc_memery(int nMemCount,bool bIfmemset = false);
    void   ngx_relase_memery(void * p);
    static CNgx_cpp_memery *ngx_get_memery();
public:
    ~CNgx_cpp_memery(){};
public:
    class CNgx_recycle
    {
    public:
        CNgx_recycle(){};
        ~CNgx_recycle()
        {
            if(m_ngx_memery)
            {
                delete m_ngx_memery;
                m_ngx_memery = nullptr;
            }
        }
    };
private:
    CNgx_cpp_memery(){};
};
