#include "ngx_cpp_setproctitle.h"

using namespace std;

CNgx_cpp_setproctitle::CNgx_cpp_setproctitle(int argv_length,int argc,char **argv)
 :m_argvcount(argv_length),m_argv(argv),m_argc(argc)
{
    m_p_environ=nullptr;
    m_environ_length=0;
}

void CNgx_cpp_setproctitle::ngx_init_setitle()
{
    int i{0};
    while(environ[i])
    {
        m_environ_length += strlen(environ[i++])+1;    /*+1是换行符*/
    }
    char * p_environ = new char [m_environ_length+1]{0};
    m_p_environ=p_environ;
    i=0;
    while(environ[i])
    {
        size_t size = strlen(environ[i])+1;
        strcpy(p_environ,environ[i]);
        environ[i++]=p_environ;
        p_environ+=size;
    }
    p_environ=nullptr;
}

void CNgx_cpp_setproctitle::ngx_setitile(const char * p_title)
{
    int i{0};
    int n=m_argvcount+m_environ_length;
    if(strlen(p_title) > n)
        return ;
    m_argv[1] = nullptr;
    strcpy(m_argv[0],p_title);
}
