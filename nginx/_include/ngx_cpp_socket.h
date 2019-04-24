#pragma once 
#include "ngx_cpp_macro.h"
#include "ngx_cpp_global.h"
#include "ngx_cpp_config.h"
#include "ngx_cpp_comm.h"
#include "ngx_cpp_memery.h"

typedef class   CNgx_cpp_socket   CNgx_cpp_socket;                  /*这种提前声明真的好用*/
typedef struct  SNgx_listening    SNgx_listening;
typedef struct  SNgx_connection   SNgx_connection;
typedef struct  SNgx_message      SNgx_message;
typedef void (CNgx_cpp_socket::*ngx_handler)(SNgx_connection *);    /*成员函数指针回调函数*/

struct SNgx_listening
{
    int                 sl_nFd;             /*监听套接字*/
    int                 sl_nPort;           /*监听端口*/
    SNgx_connection    *sl_sConnection;     /*和SNgx_connection结构互相记录*/
};

struct SNgx_connection
{
    SNgx_connection();
    ~SNgx_connection();
    void sc_init_connection();
    void sc_put_one_connection();
    int                 sc_nFd;             /*套接字加入红黑树中的*/
    uint64_t            sc_nOvertime;       /**/
    struct sockaddr     sc_sMyaddr;         /*用来记录connect进来的IP地址和端口信息*/
    uint8_t             sc_nWrite;          /*些准备就绪标志*/
    unsigned char       sc_nStat;           /*状态机*/
    char                sc_pHeadBuf[PKG_HD_LENTH];/*接收方保存包头内容的*/
    char               *sc_pPointer;        /*申请buf的指针*/
    int                 sc_nBufLen;         /*接受方还剩多少长度*/
    char               *sc_pPkg;            /*指向接收方的申请的内存地址*/
    char               *sc_pMsgPkg;         /*指向要发送的数据的首地址 申请*/
    char               *sc_pMsgPointer;     /*指向要发送的申请的内存地址*/
    int                 sc_nMsgLen;         /*要发送的数据还剩多少的长度*/
    std::atomic<int>    sc_nSend;           /*事件驱动发送数据的标记*/
    pthread_mutex_t     sc_mutex;           /*为每一个连接池中都设置一个互斥的锁*/
                                            /*以免在多线程中多连接池中的连接进行多线程的访问造成不可预知到结果*/
    SNgx_listening     *sc_sListening;      /*和SNgx_listening 结构互相绑定记录*/
    ngx_handler         sc_rdhandler;       /*读事件回调函数*/
    ngx_handler         sc_wthandler;       /*写事件回调函数*/
    uint32_t            sc_nEvents;         /*记录epoll中的结构中对应的事件类型*/
    time_t              sc_nOldTime;        /*延迟回收时间记录*/
};

/*消息头结构*/
struct SNgx_message
{
    SNgx_connection    *sm_c;               /*记录连接池的指针*/
    uint64_t            sm_nOvertime;       /*用于比较时间过期的情况*/
};

class CNgx_cpp_socket
{
public:
    bool    ngx_listening_initialize();
    bool    ngx_initialize();
    bool    ngx_epoll_init();
    int     ngx_epoll_process_events(int timer);
    ssize_t ngx_recv(SNgx_connection *c,char * sBuf,size_t nLen);
    ssize_t ngx_send(SNgx_connection *c,char * sBuf,size_t nLen);
    void    ngx_send_queun_and_post(char *pSendQueue);

public:
    virtual ~CNgx_cpp_socket();
    CNgx_cpp_socket();
private:
    SNgx_connection *ngx_get_connection(int fd);

    static  void *ngx_server_recycle_proc(void *pArg);
    static  void *ngx_server_msg_proc(void *pArg);
    void    ngx_free_connection(SNgx_connection *c);
    void    ngx_close_connection(SNgx_connection *c);
    void    ngx_close_now_connection(SNgx_connection *c);
    bool    ngx_open_listening_sockets();
    bool    ngx_setnonblocking(int sockfd);
    void    ngx_close_listening_sockets();
    void    ngx_event_accept(SNgx_connection * c);
    void    ngx_wait_read_handler(SNgx_connection * c);
    void    ngx_wait_write_handler(SNgx_connection * c);
    void    ngx_wait_request_handler_proc_p1(SNgx_connection * c);
    void    ngx_wait_request_handler_proc_pLast(SNgx_connection * c);
    bool    ngx_epoll_add_event(int fd,uint32_t eventtype,uint32_t flag,int bcaction,SNgx_connection * c);
    size_t  ngx_sock_ntop(struct sockaddr *sa,int port,u_char *text,size_t len);
    void    ngx_init_connection();
    void    ngx_free_new_and_thread();
private:    
    struct SNgx_thread
    {
        pthread_t               st_handle;      /*线程创建的句柄*/  
        CNgx_cpp_socket        *st_this;        /*这个参数用于把类中的this传进来*/ 
        bool                    st_bRuning;
        SNgx_thread(CNgx_cpp_socket * p):st_this(p),st_bRuning(false){}
        ~SNgx_thread(){}
    };
private:
    std::list<SNgx_connection *>    m_listTotal;                        /*连接池中总连接*/
    std::list<SNgx_connection *>    m_listLeisure;                      /*连接池中空闲连接*/
    std::list<SNgx_connection *>    m_listRecycle;                      /*连接池中等待回收连接*/
    std::vector<SNgx_listening *>   m_vect;                             /*SNgx_listening * 指针监听端口和监听套接字记录*/
    std::list<char *>               m_listSendQueue;
    std::atomic<int>                m_nListTotal;
    std::atomic<int>                m_nListLeisure;
    std::atomic<int>                m_nListSendQueue;
    CNgx_cpp_config                *m_pconfig;                          /*配置文件句柄*/
    struct epoll_event              m_events[NGX_MAX_EPOLL_EVENTS];     /*epoll_wait 用来取出有信号的套接字事件*/
    CNgx_cpp_memery                *m_pMem;
    SNgx_thread                    *m_pSthread;
    SNgx_thread                    *m_pMsgthread;
    pthread_mutex_t                 m_listMtx;                          /*回收链表的锁*/
    pthread_mutex_t                 m_listSendQueueMtx; 
    time_t                          m_nOldTime;
    sem_t                           m_sem;                              /*信号量*/
    int                             m_epollfd;                          /*epoll_create的返回句柄*/
    int                             m_ListenCount;                      /*侦听端口的总数 配置文件中读取*/
    int                             m_nConnections;                     /*连接池的数量*/
    int                             m_nPkgHeadLen;                      /*包头的长度*/
    int                             m_nMsgHeadLen;                      /*消息头的长度*/
    int                             m_nRecvQueue;                       /*接受队列长度*/
    
};

    /*考虑到多线程中连接池的回收会有问题*/
    /*主要有两种情况：1.还未建立三次握手的情况 2.建立三次握手的情况*/
    /*****第一种情况：因为没有多线程的引入不会存在recv的情况*/
    /*****第二种情况：在recv函数中 接收到 0 的时候客户端关闭连接，本服务器采用的LT模式这是
    epoll_wait会有通知导致recv接受为零,是客户端关闭了客户端关闭的时候要回收连接池,假如刚要
    回收连接池的时候一个线程取走了这个连接池对里面的成员操作而回收连接池的时候刚好在线程操作
    的时候对里面的指针置空的话会导致程序崩溃，对空指针的操作这样来说对服务器是极度危险的所以
    引入了两个队列来处理连接吃的问题，让回收的连接睡够足够的秒数才能进入空闲的连接池中所以引
    入了两个list把连接池中的连接取走所以采用直接关闭的方法*/