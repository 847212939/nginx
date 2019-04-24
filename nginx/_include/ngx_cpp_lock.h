#pragma once
#include "ngx_cpp_global.h"

class CNgx_cpp_lock
{
public:
    CNgx_cpp_lock(pthread_mutex_t *mutex);
    ~CNgx_cpp_lock();
private:
    pthread_mutex_t *m_mutex;
};

