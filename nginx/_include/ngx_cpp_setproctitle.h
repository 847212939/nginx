#pragma once
#include "ngx_cpp_global.h"
#include "ngx_cpp_log_message.h"

class CNgx_cpp_setproctitle
{

public:
    void ngx_init_setitle();
    void ngx_setitile(const char * p_title);
public:
    CNgx_cpp_setproctitle(int argv_length,int argc,char **argv);
    CNgx_cpp_setproctitle(){}
    ~CNgx_cpp_setproctitle()
    {
        if(m_p_environ)
        delete []m_p_environ;
    }
public:
    int     m_argvcount;
    int     m_argc;
    char ** m_argv;
private:
    char *  m_p_environ;
    size_t  m_environ_length;
};


