#include "ngx_cpp_socket.h"
#include "ngx_cpp_main.h"
#include "ngx_cpp_lock.h"

SNgx_connection::SNgx_connection()
{
    sc_nOvertime = 0;
    pthread_mutex_init(&sc_mutex,nullptr);
}

SNgx_connection::~SNgx_connection()
{
    pthread_mutex_destroy(&sc_mutex);
}

void SNgx_connection::sc_init_connection()
{
    sc_nOvertime++;
    sc_nFd = -1;
    sc_nStat = PKG_HD_INIT;
    sc_pPointer = sc_pHeadBuf;
    sc_nBufLen = sizeof(SNgx_pkgHead);
    sc_pPkg = nullptr;
    sc_nSend = 0;
    sc_nEvents =0 ;
}

void SNgx_connection::sc_put_one_connection()
{
    sc_nOvertime++;   
    CNgx_cpp_memery *pMem = CNgx_cpp_memery::ngx_get_memery();
    if(sc_pPkg)
    {        
        pMem->ngx_relase_memery(sc_pPkg);
        sc_pPkg = nullptr;
    }
    if(sc_pMsgPkg)
    {
        pMem->ngx_relase_memery(sc_pMsgPkg);
        sc_pMsgPkg = nullptr;
    }
}

/*listen 侦听创建 在main中调用*/
bool CNgx_cpp_socket::ngx_open_listening_sockets()
{
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
            close(listenfd);
            return false;
        }
        if(!ngx_setnonblocking(listenfd))
        {
            CNgx_cpp_main::m_ngx_log.ngx_log_error_core(NGX_LOG_ERR,errno,
                "CNgx_cpp_socket::ngx_open_listening_sockets()中setnonblocking()出错%d",i);
            close(listenfd);
            return false;
        }
        char buf[32]{0};
        sprintf(buf,"ListenPort%d",i);
        int nPort = m_pconfig->get_def_int(buf,10000);
        addr.sin_port = htons(nPort);
        if(-1 == bind(listenfd,(sockaddr*)&addr,sizeof(addr)))
        {
            CNgx_cpp_main::m_ngx_log.ngx_log_error_core(NGX_LOG_ERR,errno,
                "CNgx_cpp_socket::ngx_open_listening_sockets()中bind()出错%d",i);
            close(listenfd);
            return false;
        }
        if(-1 == listen(listenfd,NGX_LISTEN_BACKLOG))
        {
            CNgx_cpp_main::m_ngx_log.ngx_log_error_core(NGX_LOG_ERR,errno,
                "CNgx_cpp_socket::ngx_open_listening_sockets()中listen()出错%d",i);
            close(listenfd);
            return false;
        }
        SNgx_listening * p = new SNgx_listening;
        p->sl_nFd = listenfd;
        p->sl_nPort = nPort;
        CNgx_cpp_main::m_ngx_log.ngx_log_error_core(NGX_LOG_NOTICE,0,"监听端口%d成功",nPort);
        m_vect.push_back(p);
    }
    return true;
}

/*epoll初始化 在子进程初始化时调用*/
bool CNgx_cpp_socket::ngx_epoll_init()
{
    m_epollfd = epoll_create(m_nConnections);
    if(-1 == m_epollfd)
    {
        CNgx_cpp_main::m_ngx_log.ngx_log_error_core(NGX_LOG_EMERG,errno,
                "CNgx_cpp_socket::ngx_epoll_init()中epoll_create()出错");
        exit(2);
    }  
    /*初始化一个连接池 */
    ngx_init_connection();

    /*每一个监听端口套接字都与连接池内存绑定*/
    SNgx_connection *pc = nullptr;
    for(SNgx_listening * pl : m_vect)
    {
        pc = ngx_get_connection(pl->sl_nFd);
        pl->sl_sConnection = pc;
        pc->sc_sListening = pl;
        pc->sc_rdhandler = &CNgx_cpp_socket::ngx_event_accept;
        if(!ngx_epoll_add_event(pc->sc_nFd,EPOLL_CTL_ADD,EPOLLIN|EPOLLRDHUP,0,pc))
        {
            CNgx_cpp_main::m_ngx_log.ngx_log_error_core(NGX_LOG_NOTICE,0,
                "CNgx_cpp_socket::ngx_epoll_init()中nngx_epoll_add_event()错误");
            return false;
        }
    }
    return true;
}

void CNgx_cpp_socket::ngx_init_connection()
{
    SNgx_connection *pc = nullptr;
    CNgx_cpp_memery *pm = CNgx_cpp_memery::ngx_get_memery();
    int nSize = sizeof(SNgx_connection);
    int i = m_nConnections;
    while(i--)
    {
        pc = (SNgx_connection *)pm->ngx_alloc_memery(nSize,true);
        /*定位new 执行构造函数SNgx_connection()*/
        pc = new(pc) SNgx_connection();
        m_listLeisure.push_back(pc);
        m_listTotal.push_back(pc);
    }
    m_nListLeisure = m_listTotal.size();
    m_nListTotal = m_listTotal.size();
}

/*取出连接池里的内存与套接字绑定*/
SNgx_connection *CNgx_cpp_socket::ngx_get_connection(int fd)
{
    SNgx_connection *pc = nullptr;
    CNgx_cpp_lock CLock(&m_listMtx);
    if(m_nListLeisure > 0)
    {
        pc = m_listLeisure.front();
        m_listLeisure.pop_front();
        --m_nListLeisure;
        pc->sc_init_connection();
        pc->sc_nFd = fd;
        return pc;
    }
    /*连接池中没有了 考虑申请一个并加到链表上*/
    CNgx_cpp_memery *pm = CNgx_cpp_memery::ngx_get_memery();
    pc = (SNgx_connection *)pm->ngx_alloc_memery(sizeof(SNgx_connection),true);
    /*定位new 执行构造函数SNgx_connection()*/
    pc = new(pc) SNgx_connection();
    pc->sc_init_connection();
    pc->sc_nFd = fd;
    m_listTotal.push_back(pc);
    ++m_nListTotal;
    return pc;
}

/*调用epoll_ctl()和套接字加入红黑树中让系统给我们提供连接出来的数量*/
/*int bcaction  0 添加 1 去掉 2 完全覆盖*/
bool CNgx_cpp_socket::ngx_epoll_add_event(int fd,uint32_t eventtype,uint32_t flag,int bcaction,SNgx_connection * c)
{
    struct epoll_event ev;
    memset(&ev,0,sizeof(ev));
    if(eventtype == EPOLL_CTL_ADD)
    {
        ev.events = flag;
        c->sc_nEvents = flag;
    }
    else if(eventtype == EPOLL_CTL_MOD)
    {
        /*节点已经在红黑树中，修改节点的事件信息*/
        ev.events = c->sc_nEvents;  //先把标记恢复回来
        if(bcaction == 0)
        {
            //增加某个标记            
            ev.events |= flag;
        }
        else if(bcaction == 1)
        {
            //去掉某个标记
            ev.events &= ~flag;
        }
        else
        {
            //完全覆盖某个标记            
            ev.events = flag;      //完全覆盖            
        }
        c->sc_nEvents = ev.events; //记录该标记
    }
    else
    {
        /*删除红黑树中节点，目前没这个需求，所以将来再扩展*/
        return  true;
    } 
    ev.data.ptr = (void *)c;
    /*指针的最后一位二进制都是零*/
    if(-1 == epoll_ctl(m_epollfd,eventtype,fd,&ev))
    {
        CNgx_cpp_main::m_ngx_log.ngx_log_error_core(NGX_LOG_EMERG,errno,
            "epoll_ctl(%d,%ud,%ud,%d)失败.",fd,eventtype,flag,bcaction);
        return false;
    }
    return true;
}

/*在子进程中循环中调用有个超市处理可以阻塞*/
int  CNgx_cpp_socket::ngx_epoll_process_events(int timer)
{
    int events = epoll_wait(m_epollfd,m_events,NGX_MAX_EPOLL_EVENTS,timer);
    if(events < 0)
    {
        if(errno == EINTR)
        {
            /*这个可能是信号来了导致系统调用属于正常现象*/
            CNgx_cpp_main::m_ngx_log.ngx_log_stderr(0,
                "CNgx_cpp_socket::ngx_epoll_process_events()系统调用产生的错误属于正常直接返回");
            return 1;
        }
        else
        {
            CNgx_cpp_main::m_ngx_log.ngx_log_error_core(NGX_LOG_EMERG,errno,
                "CNgx_cpp_socket::ngx_epoll_process_events()中epoll_wait(%d)错误",m_epollfd);
            return -1;
        }
    }
    if(0 == events)
    {
        if(timer != -1)
        {
            /*时间到了正常返回*/
            return 1;
        }
        CNgx_cpp_main::m_ngx_log.ngx_log_error_core(NGX_LOG_EMERG,errno,
                "CNgx_cpp_socket::ngx_epoll_process_events()中epoll_wait(%d)错误",m_epollfd);
        return -1;
    }
    SNgx_connection *c =nullptr;
    uint32_t revents = 0;
    for(int i = 0;i < events;++i)
    {
        c = (SNgx_connection *)m_events[i].data.ptr;
        SNgx_connection *p = (SNgx_connection *)c;
        if(-1 == p->sc_nFd)
        {
            CNgx_cpp_main::m_ngx_log.ngx_log_error_core(NGX_LOG_DEBUG,0,
                "CNgx_cpp_socket::ngx_epoll_process_events()过期事件继续运行%d  events %d",i,events);
            continue;
        }
        revents = m_events[i].events;
        if(revents & (EPOLLERR | EPOLLHUP))
        {
            /*加上读写标记方便日后处理*/
            revents |= EPOLLIN | EPOLLOUT;
        }
        if(revents & EPOLLIN)
        {
            (this->*(p->sc_rdhandler))(p);
        }
        if(revents & EPOLLOUT)
        {
            if(revents & (EPOLLERR | EPOLLHUP | EPOLLRDHUP))
            {
                p->sc_nSend--;
            }
            else
            {
                (this->*(p->sc_wthandler))(p);
            }
        }

    }
    return 1;
}

/*结构体中的读事件回调函数*/
void CNgx_cpp_socket::ngx_event_accept(SNgx_connection * c)
{
    static int nUsr_accept4 = 1;
    struct sockaddr myAddr;
    memset(&myAddr,0,sizeof(myAddr));
    socklen_t nLen = sizeof(myAddr);
    int nAcceptFd = -1;
    while(1)
    {
        if(nUsr_accept4)
        {
            nAcceptFd = accept4(c->sc_nFd,&myAddr,&nLen,SOCK_NONBLOCK);
        }
        else
        {
            nAcceptFd = accept(c->sc_nFd,&myAddr,&nLen);
        }
        int nLevel = -1;
        if(-1 == nAcceptFd)
        {
            if(errno == EAGAIN)
            {
                return ;
            }
            nLevel = NGX_LOG_ALERT;
            if(errno == ECONNABORTED)
            {
                nLevel = NGX_LOG_ERR;
            }
            else if(errno == EMFILE || errno == ENFILE)
            {
                nLevel = NGX_LOG_CRIT;
            }
            CNgx_cpp_main::m_ngx_log.ngx_log_error_core(nLevel,errno,
                "CNgx_cpp_socket::ngx_event_accept()中accept()失败");
            if(nUsr_accept4 && errno == ENOSYS)
            {
                nUsr_accept4 = 0;
                continue;
            }
            return ;
        }
        SNgx_connection *pc = ngx_get_connection(nAcceptFd);
        if(!nUsr_accept4)
        {
            if(!ngx_setnonblocking(nAcceptFd))
            {
                CNgx_cpp_main::m_ngx_log.ngx_log_error_core(NGX_LOG_WARN,0,
                    "CNgx_cpp_socket::ngx_event_accept()中的ngx_setnonblocking()为空");
                ngx_close_now_connection(pc);
                return ;
            }
        }
        memcpy(&pc->sc_sMyaddr,&myAddr,sizeof(myAddr));
        pc->sc_rdhandler = &CNgx_cpp_socket::ngx_wait_read_handler;
        pc->sc_wthandler = &CNgx_cpp_socket::ngx_wait_write_handler;
        pc->sc_sListening = c->sc_sListening;
        pc->sc_nWrite = 1;
        /*使用 低速模式 LT*/
        if(!ngx_epoll_add_event(nAcceptFd,EPOLL_CTL_ADD,EPOLLIN|EPOLLRDHUP,0,pc))
        {
            ngx_close_now_connection(pc);
            return ;
        }
        break;
    } 
}

/*结构体中的写事件回调函数  里面完美解决了粘包和缺包问题*/
void CNgx_cpp_socket::ngx_wait_read_handler(SNgx_connection * c)
{
    ssize_t n = ngx_recv(c,c->sc_pPointer,c->sc_nBufLen);
    if( n <= 0)
        return ;
    if(c->sc_nStat == PKG_HD_INIT)
    {
        if(n == c->sc_nBufLen)
        {
            ngx_wait_request_handler_proc_p1(c);
        }
        else
        {
            c->sc_nStat = PKG_HD_RECVING;
            c->sc_pPointer = c->sc_pPointer +n;
            c->sc_nBufLen = c->sc_nBufLen -n;
        }
    }
    else if(c->sc_nStat == PKG_HD_RECVING)
    {
        if(n == c->sc_nBufLen)
        {
            ngx_wait_request_handler_proc_p1(c);
        }
        else
        {
            c->sc_pPointer = c->sc_pPointer +n;
            c->sc_nBufLen = c->sc_nBufLen -n;
        }
    }
    else if(c->sc_nStat == PKG_BD_INIT)
    {
        if(n == c->sc_nBufLen)
        {
            ngx_wait_request_handler_proc_pLast(c);
        }
        else
        {
            c->sc_nStat = PKG_BD_RECVING;
            c->sc_pPointer = c->sc_pPointer +n;
            c->sc_nBufLen = c->sc_nBufLen -n;
        }
    }
    else if(c->sc_nStat == PKG_BD_RECVING)
    {
        if(n == c->sc_nBufLen)
        {
            ngx_wait_request_handler_proc_pLast(c);
        }
        else
        {
            c->sc_pPointer = c->sc_pPointer +n;
            c->sc_nBufLen = c->sc_nBufLen -n;
        }
    }
    
}

void CNgx_cpp_socket::ngx_wait_write_handler(SNgx_connection * c)
{
    ssize_t n = ngx_send(c,c->sc_pMsgPointer,c->sc_nMsgLen);
    if(n > 0 && n != c->sc_nMsgLen)
    {
        c->sc_pMsgPointer = c->sc_pMsgPointer + n;
        c->sc_nMsgLen = c->sc_nMsgLen - n;
        CNgx_cpp_main::m_ngx_log.ngx_log_stderr(0,"ngx_wait_write_handler：：发送了但只发送了一部分");

        return ;
    }
    if(-1 == n)
    {
        /*这不太可能，可以发送数据时通知我发送数据，我发送时你却通知我发送缓冲区满？*/
        CNgx_cpp_main::m_ngx_log.ngx_log_error_core(NGX_LOG_ERR,0,
            "缓冲区满了还通知我发送数据 很难受 很诡异");
        return ;
    }
    if(n > 0 && n == c->sc_nMsgLen)
    {
         if(!ngx_epoll_add_event(c->sc_nFd,EPOLL_CTL_MOD,EPOLLOUT,1,c))
        {
            CNgx_cpp_main::m_ngx_log.ngx_log_error_core(NGX_LOG_ERR,0,
            "CNgx_cpp_socket::ngx_wait_write_handler中ngx_epoll_add_event()错了");
        }
        CNgx_cpp_main::m_ngx_log.ngx_log_stderr(0,"ngx_wait_write_handler：：成功的发送了一个包");

    }
    m_pMem->ngx_relase_memery(c->sc_pMsgPkg);
    c->sc_pMsgPkg = nullptr;
    c->sc_pMsgPointer = nullptr;
    c->sc_nMsgLen = 0;
    c->sc_nSend--;
    if(-1 == sem_post(&m_sem))
    {
        CNgx_cpp_main::m_ngx_log.ngx_log_error_core(NGX_LOG_ERR,0,
                "CNgx_cpp_socket::ngx_wait_write_handler()中sem_post()错了");
    }


}

void CNgx_cpp_socket::ngx_wait_request_handler_proc_p1(SNgx_connection * c)
{
    SNgx_pkgHead *pPkgHead =(SNgx_pkgHead *)c->sc_pHeadBuf;
    unsigned short nPkgLen = ntohs(pPkgHead->sp_nPkgLen);
    CNgx_cpp_main::m_ngx_log.ngx_log_stderr(0,"CNgx_cpp_socket::ngx_wait_request_handler_proc_p1()中nPkgLen = %d",nPkgLen);
    /*畸形包检测*/
    if(nPkgLen < m_nPkgHeadLen)
    {
        c->sc_nStat = PKG_HD_INIT;
        c->sc_pPointer = c->sc_pHeadBuf;
        c->sc_nBufLen = m_nPkgHeadLen;
    }
    else if(nPkgLen > PKG_MAX_LENTH - 1000)
    {
        c->sc_nStat = PKG_HD_INIT;
        c->sc_pPointer = c->sc_pHeadBuf;
        c->sc_nBufLen = m_nPkgHeadLen;
    }
    else
    {
        /*客户端格式 包头+包体 服务器加一个消息头作为判断依据*/
        char *pBuf = (char *)m_pMem->ngx_alloc_memery(m_nMsgHeadLen + nPkgLen);
        /*防止恶意用户在你申请过内存 就关闭连接 不给你发送数据了 自己来回收内存*/
        c->sc_pPkg = pBuf;
        /*填写消息头的内容*/
        SNgx_message *pMsgHead = (SNgx_message *)pBuf;
        pMsgHead->sm_c = c;
        pMsgHead->sm_nOvertime = c->sc_nOvertime;
        /*填写包头的内容*/
        pBuf = pBuf +m_nMsgHeadLen;
        memcpy(pBuf,c->sc_pHeadBuf,m_nPkgHeadLen);
        if(m_nPkgHeadLen == nPkgLen)
        {
            /*允许只有包头的存在*/
            ngx_wait_request_handler_proc_pLast(c);
        }
        else
        {
            pBuf += m_nPkgHeadLen;
            c->sc_nStat = PKG_BD_INIT;
            c->sc_pPointer = pBuf;
            c->sc_nBufLen = nPkgLen - m_nPkgHeadLen;
        }
        
    }

}

void CNgx_cpp_socket::ngx_wait_request_handler_proc_pLast(SNgx_connection * c)
{
    /*转到CNgx_cpp_thread中来了消息队列*/
    CNgx_cpp_main::m_ngx_thread.ngx_in_recv_and_signal(c->sc_pPkg);
    c->sc_pPkg = nullptr;
    c->sc_nStat = PKG_HD_INIT;
    c->sc_pPointer = c->sc_pHeadBuf;
    c->sc_nBufLen = sizeof(SNgx_pkgHead);
}

bool CNgx_cpp_socket::ngx_setnonblocking(int sockfd) 
{    
    /*0：清除，1：设置  */
    int nb=1; 
    /*FIONBIO：设置/清除非阻塞I/O标记：0：清除，1：设置*/
    if(ioctl(sockfd, FIONBIO, &nb) == -1) 
    {
        return false;
    }
    return true;
}

void CNgx_cpp_socket::ngx_close_listening_sockets()
{
    int i = -1;
    while(++i < m_ListenCount)
    {
        close(m_vect[i]->sl_nFd);
        CNgx_cpp_main::m_ngx_log.ngx_log_error_core(NGX_LOG_NOTICE,0,"关闭监听端口%d",i);
    }
}
bool CNgx_cpp_socket::ngx_initialize()
{
    int nErrno = -1;
    m_pSthread = new SNgx_thread(this);
    if(nErrno = pthread_create(&m_pSthread->st_handle,nullptr,ngx_server_recycle_proc,m_pSthread))
    {
        CNgx_cpp_main::m_ngx_log.ngx_log_error_core(NGX_LOG_ERR,nErrno,
            "CNgx_cpp_socket::ngx_listening_initialize()中thread_create()失败了");
        return false;
    }
    m_pMsgthread = new SNgx_thread(this);
    if(nErrno = pthread_create(&m_pMsgthread->st_handle,nullptr,ngx_server_msg_proc,m_pMsgthread))
    {
        CNgx_cpp_main::m_ngx_log.ngx_log_error_core(NGX_LOG_ERR,nErrno,
            "CNgx_cpp_socket::ngx_listening_initialize()中ngx_threadMsgProc()失败了");
        return false;
    }
    return true;
}

bool CNgx_cpp_socket::ngx_listening_initialize()
{
    m_ListenCount = m_pconfig->get_def_int("ListenPortCount",m_ListenCount);
    m_nConnections = m_pconfig->get_def_int("worker_connections",m_ListenCount);
    if(!ngx_open_listening_sockets())
        return false;
    int nErrno = -1;
    if(nErrno = pthread_mutex_init(&m_listMtx,nullptr))
    {
        CNgx_cpp_main::m_ngx_log.ngx_log_error_core(NGX_LOG_ERR,nErrno,
        "CNgx_cpp_socket::ngx_listening_initialize()中pthread_mutex_init(&m_listMtx,nullptr))失败");
        return false;
    }
    if(nErrno = pthread_mutex_init(&m_listSendQueueMtx,nullptr))
    {
        CNgx_cpp_main::m_ngx_log.ngx_log_error_core(NGX_LOG_ERR,nErrno,
        "CNgx_cpp_socket::ngx_listening_initialize()中pthread_mutex_init(&m_listSendQueueMtx,nullptr)失败");
        return false;
    }
    /*第二个参数=0，表示信号量在线程之间共享，确实如此 ，如果非0，表示在进程之间共享*/
    /*第三个参数=0，表示信号量的初始值，为0时，调用sem_wait()就会卡在那里卡着*/
    if(-1 == sem_init(&m_sem,0,0))
    {
        CNgx_cpp_main::m_ngx_log.ngx_log_error_core(NGX_LOG_ERR,0,
        "CNgx_cpp_socket::ngx_listening_initialize()中sem_init()失败");
        return false;
    }
    
    return true;
}

CNgx_cpp_socket::CNgx_cpp_socket()
{
    m_ListenCount = 1;
    m_nConnections = 1;
    m_nListSendQueue = 0;
    m_epollfd = -1;
    m_nRecvQueue = 0;
    m_pMsgthread = nullptr;
    m_pSthread = nullptr;
    m_nPkgHeadLen = sizeof(SNgx_pkgHead);
    m_nMsgHeadLen = sizeof(SNgx_message);
    m_pconfig = CNgx_cpp_config::get_m_pconfig();
    m_pMem = CNgx_cpp_memery::ngx_get_memery();
}

CNgx_cpp_socket::~CNgx_cpp_socket()
{
    ngx_free_new_and_thread();
}

void CNgx_cpp_socket::ngx_free_new_and_thread()
{
    if(CNgx_cpp_main::m_ngx_exit)
    {
        for(SNgx_listening * p : m_vect)
        {
            if(p)
                delete p;
        }
        m_vect.clear();
        pthread_join(m_pSthread->st_handle,nullptr);
        delete m_pSthread;
        pthread_join(m_pMsgthread->st_handle,nullptr);
        delete m_pMsgthread;
        for(SNgx_connection *pc : m_listTotal)
        {
            if(pc)
            {
                if(pc->sc_nFd > 0)
                    close(pc->sc_nFd);
                pc->sc_put_one_connection();
                pc->~SNgx_connection();
                m_pMem->ngx_relase_memery(pc);
            }
        }
        m_listTotal.clear();
        m_listRecycle.clear();
        m_listLeisure.clear();
        for(char *pSendBuf : m_listSendQueue)
        {
            if(pSendBuf)
                m_pMem->ngx_relase_memery(pSendBuf);
        }
        m_listSendQueue.clear();
        /*都推出了 也没必要知道退出是出错了不行操作系统给我回收*/
        pthread_mutex_destroy(&m_listMtx);
        pthread_mutex_destroy(&m_listSendQueueMtx);
        sem_destroy(&m_sem);
    }
}

void CNgx_cpp_socket::ngx_free_connection(SNgx_connection *c)
{
    CNgx_cpp_lock CLock(&m_listMtx);
    c->sc_nOvertime++;
    c->sc_put_one_connection();
    c->sc_nOldTime = time(nullptr);
    m_listRecycle.push_back(c);

}

void CNgx_cpp_socket::ngx_close_connection(SNgx_connection *c)
{
    int fd =c->sc_nFd;
    ngx_free_connection(c);
    c->sc_nFd = -1;
    if(-1 == close(fd))
    {
        CNgx_cpp_main::m_ngx_log.ngx_log_error_core(NGX_LOG_NOTICE,0,
            "CNgx_cpp_socket::ngx_ngx_close_connection中的close()出错");
        return ;
    }
}

void CNgx_cpp_socket::ngx_close_now_connection(SNgx_connection *c)
{
    int fd =c->sc_nFd;
    c->sc_nFd = -1;
    c->sc_nOvertime++;
    CNgx_cpp_lock CLock(&m_listMtx);
    m_listLeisure.push_back(c);
    m_nListLeisure++;
    if(-1 == close(fd))
    {
        CNgx_cpp_main::m_ngx_log.ngx_log_error_core(NGX_LOG_NOTICE,0,
            "CNgx_cpp_socket::ngx__connection()中的close()出错");
        return ;
    }
}

size_t CNgx_cpp_socket::ngx_sock_ntop(struct sockaddr *sa,int port,u_char *text,size_t len)
{
    struct sockaddr_in   *sin;
    u_char               *p;

    switch (sa->sa_family)
    {
    case AF_INET:
        sin = (struct sockaddr_in *) sa;
        p = (u_char *) &sin->sin_addr;
        if(port)  //端口信息也组合到字符串里
        {
            p = CNgx_cpp_main::m_ngx_log.ngx_snprintf(text, len, "%ud.%ud.%ud.%ud:%d",
                p[0], p[1], p[2], p[3], ntohs(sin->sin_port)); //返回的是新的可写地址
        }
        else //不需要组合端口信息到字符串中
        {
            p = CNgx_cpp_main::m_ngx_log.ngx_snprintf(text, len, "%ud.%ud.%ud.%ud",
                p[0], p[1], p[2], p[3]);            
        }
        return (p - text);
        break;
    default:
        return 0;
        break;
    }
    return 0;
}

void *CNgx_cpp_socket::ngx_server_recycle_proc(void *pArg)
{
    SNgx_thread *pt = (SNgx_thread *)pArg;
    CNgx_cpp_socket *ps = pt->st_this;
    int nTime = ps->m_pconfig->get_def_int("RecoveryTime",60);
    int nErrno = -1;
    while(true)
    {
        usleep(200*1000);
        if(nErrno = pthread_mutex_lock(&ps->m_listMtx))
        {
            CNgx_cpp_main::m_ngx_log.ngx_log_error_core(NGX_LOG_ERR,nErrno,
                "CNgx_cpp_socket::ngx_server_recycle_proc()中pthread_mutex_lock()错了");
        }
        if(ps->m_listRecycle.size() > 0)
        {
            std::list<SNgx_connection *>::iterator ite = ps->m_listRecycle.begin();
            while(ite != ps->m_listRecycle.end())
            {
                time_t nNewTime = time(nullptr);
                if((nNewTime - (*ite)->sc_nOldTime) >= nTime)
                {
                    ps->m_listLeisure.push_back(*ite);
                    ps->m_nListLeisure++;
                    ps->m_listRecycle.erase(ite);
                }
                ite++;
            }

        }
        if(nErrno = pthread_mutex_unlock(&ps->m_listMtx))
        {
            CNgx_cpp_main::m_ngx_log.ngx_log_error_core(NGX_LOG_ERR,nErrno,
                "CNgx_cpp_socket::ngx_server_recycle_proc()中pthread_mutex_unlock()错了");
        }
        if(CNgx_cpp_main::m_ngx_exit)
        {
            break;
        }

    }
    return (void *)0;
}

void CNgx_cpp_socket::ngx_send_queun_and_post(char *pSendQueue)
{
    CNgx_cpp_lock CLock(&m_listSendQueueMtx);
    m_listSendQueue.push_back(pSendQueue);
    /*记录下队列中的数目 原子操作*/
    m_nListSendQueue++;
    CLock.~CNgx_cpp_lock();
    if(-1 == sem_post(&m_sem))
    {
        CNgx_cpp_main::m_ngx_log.ngx_log_error_core(NGX_LOG_ERR,0,
                "CNgx_cpp_socket::ngx_send_queun_and_post中sem_post()错了");
    }
}

void *CNgx_cpp_socket::ngx_server_msg_proc(void *pArg)
{
    SNgx_thread     *pt = (SNgx_thread *)pArg;
    CNgx_cpp_socket *ps = pt->st_this;
    int              nErrno = -1;
    SNgx_connection *pc = nullptr;
    SNgx_message    *pMsg = nullptr;
    SNgx_pkgHead    *pHead = nullptr;
    char            *pMsgBuf = nullptr;

    while(true)
    {
        /*接到一个信号就减一 到零位置变成阻塞状态*/
        if(-1 == sem_wait(&ps->m_sem))
        {
            CNgx_cpp_main::m_ngx_log.ngx_log_error_core(NGX_LOG_ERR,errno,
                "CNgx_cpp_socket::ngx_server_msg_proc()中sem_wait()错了");
        }
        if(ps->m_nListSendQueue > 0)
        {
            if(nErrno = pthread_mutex_lock(&ps->m_listSendQueueMtx))
            {
                CNgx_cpp_main::m_ngx_log.ngx_log_error_core(NGX_LOG_ERR,nErrno,
                    "CNgx_cpp_socket::ngx_server_msg_proc()中pthread_mutex_lock()错了");
            }
            std::list<char *>::iterator itbegin = ps->m_listSendQueue.begin();
            std::list<char *>::iterator itend = ps->m_listSendQueue.end();
            while(itbegin != itend)
            {
                pMsgBuf = *itbegin;
                pMsg = (SNgx_message *)pMsgBuf;
                pHead = (SNgx_pkgHead *)(pMsgBuf + ps->m_nMsgHeadLen);
                pc = pMsg->sm_c;
                /*过期事件的处理*/
                if(pc->sc_nOvertime != pMsg->sm_nOvertime)
                {

                    std::list<char *>::iterator itmp = itbegin;
                    itbegin++;
                    ps->m_listSendQueue.erase(itmp);
                    ps->m_pMem->ngx_relase_memery(pMsgBuf);
                    ps->m_nListSendQueue--;
                    continue;
                }
                /*事件驱动发送标记这是没法剔除 因为不知道有没有发送成功呢*/
                if(pc->sc_nSend > 0)
                {
                    itbegin++;
                    continue;
                }
                /*记录下来发送消息队列*/
                pc->sc_pMsgPkg = pMsgBuf;
                pc->sc_pMsgPointer = pMsgBuf + ps->m_nMsgHeadLen;
                pc->sc_nMsgLen = ntohs(pHead->sp_nPkgLen);
                std::list<char *>::iterator tmp = itbegin;
                itbegin++;
                ps->m_listSendQueue.erase(tmp);
                ps->m_nListSendQueue--;
                int nLen = pc->sc_nMsgLen;
                char *p = pc->sc_pMsgPointer;
                ssize_t n = ps->ngx_send(pc,p,nLen);
                if(n > 0)
                {
                    if(n == nLen)
                    {
                        ps->m_pMem->ngx_relase_memery(pMsgBuf);
                        pc->sc_pMsgPkg = nullptr;
                        pc->sc_pMsgPointer = nullptr;
                        pc->sc_nMsgLen = 0;
                        pc->sc_nSend = 0;
                        CNgx_cpp_main::m_ngx_log.ngx_log_stderr(0,"成功的发送了一个包");
                        continue;
                    }
                    else
                    {
                        /*发送了一部分 但是没发送完 可能是发送缓冲区已满 要加入epoll中才行*/
                        pc->sc_pMsgPointer = pc->sc_pMsgPointer + n;
                        pc->sc_nMsgLen = pc->sc_nMsgLen - n;
                        pc->sc_nSend++;
                        if(!ps->ngx_epoll_add_event(pc->sc_nFd,EPOLL_CTL_MOD,EPOLLOUT,0,pc))
                        {
                            CNgx_cpp_main::m_ngx_log.ngx_log_error_core(NGX_LOG_ERR,nErrno,
                            "CNgx_cpp_socket::ngx_server_msg_proc()中ngx_epoll_add_event()错了");
                        }
                        CNgx_cpp_main::m_ngx_log.ngx_log_stderr(0,"发送了但只发送了一部分");
                        continue;
                    }
                }
                else if(n == 0)
                {
                    ps->m_pMem->ngx_relase_memery(pc->sc_pMsgPkg);
                    pc->sc_pMsgPkg = nullptr;
                    pc->sc_pMsgPointer = nullptr;
                    pc->sc_nMsgLen = 0;
                    pc->sc_nSend = 0;
                    continue;
                }
                else if(-1 == n)
                {
                    /*发送缓冲区已经满了【一个字节都没发出去，说明发送 缓冲区当前正好是满的】*/
                    pc->sc_nSend++;
                    if(!ps->ngx_epoll_add_event(pc->sc_nFd,EPOLL_CTL_MOD,EPOLLOUT,0,pc))
                    {
                        CNgx_cpp_main::m_ngx_log.ngx_log_error_core(NGX_LOG_ERR,nErrno,
                            "CNgx_cpp_socket::ngx_server_msg_proc()中ngx_epoll_add_event()错了");
                    }
                    continue;
                }
                else
                {
                    /*能走到这里的，应该就是返回值-2了，一般就认为对端断开了，等待recv()来做断开socket以及回收资源*/
                    ps->m_pMem->ngx_relase_memery(pc->sc_pMsgPkg);
                    pc->sc_pMsgPkg = nullptr;
                    pc->sc_pMsgPointer = nullptr;
                    pc->sc_nMsgLen = 0;
                    pc->sc_nSend = 0;
                    continue;
                }
            }    
            if(nErrno = pthread_mutex_unlock(&ps->m_listSendQueueMtx))
            {
                CNgx_cpp_main::m_ngx_log.ngx_log_error_core(NGX_LOG_ERR,nErrno,
                "CNgx_cpp_socket::ngx_server_msg_proc()中pthread_mutex_unlock()错了");
            }        
        }

            
        if(CNgx_cpp_main::m_ngx_exit)
        {
            break;
        }
    }
    return (void *)0;
}

ssize_t CNgx_cpp_socket::ngx_send(SNgx_connection *c,char * sBuf,size_t nLen)
{
    ssize_t n = -1;
    while(true)
    {
        n = send(c->sc_nFd,sBuf,nLen,0);
        if(n > 0)
        {
            return n;
        }
        if(n == 0)
        {
            return 0;
        }
        if(errno == EAGAIN)
        {
            /*缓冲区满了*/
            return -1;
        }
        if(errno == EINTR)
        {
            CNgx_cpp_main::m_ngx_log.ngx_log_error_core(NGX_LOG_INFO,errno,
            "ngx_send()中send返回EINTR ");
        }
        else
        {
            return -2;
        }
    }
    return 0;
}

ssize_t CNgx_cpp_socket::ngx_recv(SNgx_connection *c,char * sBuf,size_t nLen)
{
    ssize_t n = recv(c->sc_nFd,sBuf,nLen,0);
    if(0 == n)
    {
        /*客户端关闭了连接 释放连接池*/
        ngx_close_connection(c);
        CNgx_cpp_main::m_ngx_log.ngx_log_error_core(NGX_LOG_INFO,errno,
            "ngx_recv()中recv返回 0 客户端正常关闭连接");
        return -1;
    }
    if(n < 0)
    {
        if(errno == EAGAIN || errno == EWOULDBLOCK)
        {
            CNgx_cpp_main::m_ngx_log.ngx_log_error_core(NGX_LOG_ERR,errno,
                "ngx_recv()中ngx_recv()错了，在LT模式下不应该有这个错误");
            return -1;
        }
        if(errno == EINTR)
        {
            /*这个不算错误，当一个信号产生是 捕捉函数返回可能产生这个信号*/
            CNgx_cpp_main::m_ngx_log.ngx_log_error_core(NGX_LOG_ERR,errno,
                "ngx_recv()中ngx_recv()错了，在LT模式下不应该有这个错误");
            return -1;
        }
        if(errno == ECONNRESET)
        {

        }
        else
        {
            CNgx_cpp_main::m_ngx_log.ngx_log_error_core(NGX_LOG_ERR,errno,
                "ngx_recv()中ngx_recv()错了，未知错误打印出来瞧瞧");
        }
        ngx_close_connection(c);
        return -1;
    }
    return n;
}
