#include "ngx_cpp_socket.h"
#include "ngx_cpp_main.h"

bool CNgx_cpp_socket::ngx_open_listening_sockets()
{
    CNgx_cpp_config * pconfig = CNgx_cpp_config::get_m_pconfig();
    if(pconfig == nullptr)
    {
        CNgx_cpp_main::m_ngx_log.ngx_log_error_core(NGX_LOG_ERR,0,"CNgx_cpp_socket::ngx_open_listening_sockets()中pconfig为NULL");
        return false;
    }
    m_ListenCount = pconfig->get_def_int("ListenPortCount",m_ListenCount);
   
    sockaddr_in addr;
    memset(&addr,0,sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    int i=-1;
    while(++i < m_ListenCount)
    {
        int listenfd = socket(AF_INET,SOCK_STREAM,0);
        if(-1 == listenfd)
        {
            CNgx_cpp_main::m_ngx_log.ngx_log_error_core(NGX_LOG_ERR,errno,
                "CNgx_cpp_socket::ngx_open_listening_sockets()中socket()出错%d",i);
            return false;
        }
        int reuseaddr=1;
        /*WAIT_TIME不等待*/
        if(-1==setsockopt(listenfd,SOL_SOCKET,SO_REUSEADDR,&reuseaddr,sizeof(reuseaddr)))
        {
            CNgx_cpp_main::m_ngx_log.ngx_log_error_core(NGX_LOG_ERR,errno,
                "CNgx_cpp_socket::ngx_open_listening_sockets()中setsockopt()出错%d",i);
            return false;
        }
        if(!setnonblocking(listenfd))
        {
            CNgx_cpp_main::m_ngx_log.ngx_log_error_core(NGX_LOG_ERR,errno,
                "CNgx_cpp_socket::ngx_open_listening_sockets()中setnonblocking()出错%d",i);
            return false;
        }
        char buf[32]{0};
        sprintf(buf,"ListenPort%d",i);
        int nPort = pconfig->get_def_int(buf,0);
        addr.sin_port = htons(nPort);
        if(-1 == bind(listenfd,(sockaddr*)&addr,sizeof(addr)))
        {
            CNgx_cpp_main::m_ngx_log.ngx_log_error_core(NGX_LOG_ERR,errno,
                "CNgx_cpp_socket::ngx_open_listening_sockets()中bind()出错%d",i);
            return false;
        }
        if(-1 == listen(listenfd,NGX_LISTEN_BACKLOG))
        {
            CNgx_cpp_main::m_ngx_log.ngx_log_error_core(NGX_LOG_ERR,errno,
                "CNgx_cpp_socket::ngx_open_listening_sockets()中listen()出错%d",i);
            return false;
        }
        SNgx_listening * p = new SNgx_listening;
        p->fd = listenfd;
        p->port = nPort;
        CNgx_cpp_main::m_ngx_log.ngx_log_error_core(NGX_LOG_NOTICE,0,"监听端口%d成功",i);
        m_vect.push_back(p);

    }
    return true;
}

bool CNgx_cpp_socket::setnonblocking(int sockfd) 
{    
    int nb=1; //0：清除，1：设置  
    if(ioctl(sockfd, FIONBIO, &nb) == -1) //FIONBIO：设置/清除非阻塞I/O标记：0：清除，1：设置
    {
        return false;
    }
    return true;

    //如下也是一种写法，跟上边这种写法其实是一样的，但上边的写法更简单
    /* 
    //fcntl:file control【文件控制】相关函数，执行各种描述符控制操作
    //参数1：所要设置的描述符，这里是套接字【也是描述符的一种】
    int opts = fcntl(sockfd, F_GETFL);  //用F_GETFL先获取描述符的一些标志信息
    if(opts < 0) 
    {
        ngx_log_stderr(errno,"CSocekt::setnonblocking()中fcntl(F_GETFL)失败.");
        return false;
    }
    opts |= O_NONBLOCK; //把非阻塞标记加到原来的标记上，标记这是个非阻塞套接字【如何关闭非阻塞呢？opts &= ~O_NONBLOCK,然后再F_SETFL一下即可】
    if(fcntl(sockfd, F_SETFL, opts) < 0) 
    {
        ngx_log_stderr(errno,"CSocekt::setnonblocking()中fcntl(F_SETFL)失败.");
        return false;
    }
    return true;
    */
}
void CNgx_cpp_socket::ngx_close_listening_sockets()
{
    int i = -1;
    while(++i < m_ListenCount)
    {
        close(m_vect[i]->fd);
        CNgx_cpp_main::m_ngx_log.ngx_log_error_core(NGX_LOG_NOTICE,0,"关闭监听端口%d",i);
    }
}

CNgx_cpp_socket::CNgx_cpp_socket()
{
    m_ListenCount=1;
}

CNgx_cpp_socket::~CNgx_cpp_socket()
{
    for(SNgx_listening * p : m_vect)
    {
        delete(p);
    }
    m_vect.clear();
}