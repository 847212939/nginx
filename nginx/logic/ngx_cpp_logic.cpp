#include "ngx_cpp_logic.h"
#include "ngx_cpp_comm.h"
#include "ngx_cpp_socket.h"
#include "ngx_cpp_lock.h"
#include "ngx_cpp_crc32.h"
#include "ngx_cpp_main.h"
#include "ngx_cpp_logicomm.h"

/*添加回调函数的*/
pkg_handler CNgx_cpp_logic::m_handler[]=
{
    &CNgx_cpp_logic::ngx_login,
    &CNgx_cpp_logic::ngx_register,
    &CNgx_cpp_logic::ngx_heartbeat,
};

/*包的处理函数加上调用回调函数的作用，参数是整个包的指针 消息头 + 包头 + 包体*/
void CNgx_cpp_logic::ngx_process_data(char *pPkg)
{
    int nMsg = sizeof(SNgx_message);
    int nHead = sizeof(SNgx_pkgHead);
    SNgx_message *pMsg = (SNgx_message *)pPkg;
    SNgx_pkgHead *pHead = (SNgx_pkgHead *)(pPkg + nMsg);
    char *pBuf = nullptr;
    unsigned short nCode = ntohs(pHead->sp_nMegCode);
    unsigned short nPkgLen = ntohs(pHead->sp_nPkgLen);
    if(nPkgLen == nHead)
    {
        if(pHead->sp_ncrc32 != 0)
        {
            CNgx_cpp_main::m_ngx_log.ngx_log_error_core(NGX_LOG_INFO,0,
            "CNgx_cpp_logic::ngx_process_data()中pHead->sp_ncrc32不为0");
            return ;
        }    
    }
    else
    { 
        pHead->sp_ncrc32 = ntohl(pHead->sp_ncrc32);
        pBuf = pPkg + (nMsg + nHead);
        CNgx_cpp_crc32 *pCrc = CNgx_cpp_crc32::GetInstance();
        if(pHead->sp_ncrc32 != pCrc->Get_CRC((unsigned char *)pBuf,nPkgLen-nHead))
        {
            CNgx_cpp_main::m_ngx_log.ngx_log_error_core(NGX_LOG_INFO,0,
            "CNgx_cpp_logic::ngx_process_data()中Crc32验证码错了");
            return ;
        }
    }
    if(pMsg->sm_nOvertime != pMsg->sm_c->sc_nOvertime)
    {
        /*过期包不作处理直接返回*/
        CNgx_cpp_main::m_ngx_log.ngx_log_error_core(NGX_LOG_INFO,0,
            "CNgx_cpp_logic::ngx_process_data()中过期包");
        return ;
    }
    int nErrno = -1;
    if(nErrno = pthread_mutex_lock(&m_mutex))
    {
        CNgx_cpp_main::m_ngx_log.ngx_log_error_core(NGX_LOG_ERR,nErrno,
            "CNgx_cpp_logic::ngx_process_data()中的pthread_mutex_lock()错了");
    }
    /*循环寻找对应的命令 直接跳出*/
    std::vector<SNgx_pkgHandler *>::iterator itbegin = m_vectHandler.begin();
    std::vector<SNgx_pkgHandler *>::iterator itend = m_vectHandler.end();
    while(itbegin != itend)
    {
        if(nCode == (*itbegin)->sh_nMSgCode)
            break;
        itbegin++;
    }
    if(nErrno = pthread_mutex_unlock(&m_mutex))
    {
       CNgx_cpp_main::m_ngx_log.ngx_log_error_core(NGX_LOG_ERR,nErrno,
            "CNgx_cpp_logic::ngx_process_data()中的pthread_mutex_unlock()错了");
    } 
    if((*itbegin))
    {
        (this->*((*itbegin)->sh_handler))(pMsg->sm_c,pMsg,pBuf,nPkgLen-nHead);
    }
    
}

bool CNgx_cpp_logic::ngx_initializer()
{
    m_nHandler = sizeof(m_handler)/sizeof(m_handler[0]);
    int i = -1;
    while(++i < m_nHandler)
    {
        SNgx_pkgHandler * ph = new SNgx_pkgHandler;
        ph->sh_nMSgCode = NGX_LOGIN + i;
        ph->sh_handler = m_handler[i];
        m_vectHandler.push_back(ph);
    }
    if(!CNgx_cpp_socket::ngx_listening_initialize())
        return false;
    return true;
}

CNgx_cpp_logic::CNgx_cpp_logic()
{
    pthread_mutex_init(&m_mutex,nullptr);
    m_pCrc32 = CNgx_cpp_crc32::GetInstance();
    m_pMem = CNgx_cpp_memery::ngx_get_memery();
}

CNgx_cpp_logic::~CNgx_cpp_logic()
{
    for(SNgx_pkgHandler *pHandler : m_vectHandler)
    {
        if(pHandler)
            delete pHandler;
    }
    pthread_mutex_destroy(&m_mutex);
}

/*pBuf是包体的指针，nLen是包体的长度用于和结构比较 如果是错误的就返回*/
/*登录消息回调函数*/
bool CNgx_cpp_logic::ngx_login(SNgx_connection *c,SNgx_message *pMsg,
char *pBuf,unsigned short nLen)
{
    
    return true;
}

/*注册消息回调函数*/
bool CNgx_cpp_logic::ngx_register(SNgx_connection *c,SNgx_message *pMsg,
char *pBuf,unsigned short nLen)
{
    int nRegister = sizeof(SNgx_register);
    int nMsg = sizeof(SNgx_message);
    int nHead = sizeof(SNgx_pkgHead);
    if(!pBuf)
    {
        CNgx_cpp_main::m_ngx_log.ngx_log_error_core(NGX_LOG_ERR,0,
            "CNgx_cpp_logic::ngx_register()pBuf = nullptr");
        return false;    
    }
    if(nRegister != nLen)
    {
        CNgx_cpp_main::m_ngx_log.ngx_log_error_core(NGX_LOG_ERR,0,
            "CNgx_cpp_logic::ngx_register()中的有恶意用户了");
        return false;
    }
    /*凡是本用户自己的访问都给互斥上去 防止数据混乱*/

    CNgx_cpp_lock CLock(&c->sc_mutex);
    /*包体中的数据找到对应的回调函数就相当找到了结构体的类型然后进行强制类型转换就行了*/
    SNgx_register *pRegister = (SNgx_register *)pBuf;
    /*************************************************************************/
    /***********************进行一些类的数据处理的逻辑***************************/
    /**************************************************************************/
    /**************************************************************************/
    /*申请的内存是消息头+包头+包体的结构*/
    nRegister = 65000;
    int nSize = nMsg + nHead + nRegister;
    char *pMsgPkg  = (char *)m_pMem->ngx_alloc_memery(nSize);

    /*填充消息头*/
    memcpy(pMsgPkg,pMsg,nMsg);
    /*填充包头*/
    SNgx_pkgHead *pSendMsgHead = (SNgx_pkgHead *)(pMsgPkg + nMsg);
    pSendMsgHead->sp_nMegCode = htons(NGX_REGISTER);
    pSendMsgHead->sp_nPkgLen = htons(nSize - nMsg);
    /*填写包体的内容*/
    SNgx_register *pSendMsgBody = (SNgx_register *)(pMsgPkg +(nMsg+nHead));
    /**************************************************************************/
    /***********************随意发挥一下就可以啦********************************/
    /**************************************************************************/
    /**************************************************************************/
    /*包体定义好了在加上加密算法 计算包体的出crc32的值*/
    pSendMsgHead->sp_ncrc32 = htonl(m_pCrc32->Get_CRC((unsigned char *)pBuf,nRegister));

    /*这时候开始发送时数据了*/
    CNgx_cpp_socket::ngx_send_queun_and_post(pMsgPkg);
    /*利用连接池中构造函数的时候初始化的锁给这个连接池里面的数据给锁着防止在别的线程中有人使用*/
    /*如果是多个进程的话 我不好说 因为今天测试了一下多个进程没法使用互斥锁的，所以别的方法还有待考虑*/
    
    return true;
}

/*心跳包消息回调函数*/
bool CNgx_cpp_logic::ngx_heartbeat(SNgx_connection *c,SNgx_message *pMsg,
char *pBuf,unsigned short nLen)
{
    
    return true;
}
