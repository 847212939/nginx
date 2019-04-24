#pragma once
#include "ngx_cpp_log_message.h"

class CNgx_cpp_daemon
{
public:
    bool ngx_daemon();
public:
    CNgx_cpp_daemon():m_fd(-1){};
    ~CNgx_cpp_daemon()
    {
        
        if(m_fd)
            close(m_fd);
    };
private:
    int m_fd;
};

