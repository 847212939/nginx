#pragma once
#include "ngx_cpp_global.h"
#include "ngx_cpp_socket.h"
#include "ngx_cpp_memery.h"
#include "ngx_cpp_crc32.h"

typedef class CNgx_cpp_logic CNgx_cpp_logic;
typedef bool (CNgx_cpp_logic::*pkg_handler)(SNgx_connection *,SNgx_message *,char *,unsigned short);
struct SNgx_pkgHandler
{
    int             sh_nMSgCode;        /*事件类型*/
    pkg_handler     sh_handler;         /*事件回调函数*/
};

class CNgx_cpp_logic : public CNgx_cpp_socket
{
public:
    void ngx_process_data(char *pPkg);
    bool ngx_initializer();
public:
    CNgx_cpp_logic();
    ~CNgx_cpp_logic();
private:
    bool ngx_login(SNgx_connection *c,SNgx_message *pMsg,char *pBuf,unsigned short nLen);
    bool ngx_register(SNgx_connection *c,SNgx_message *pMsg,char *pBuf,unsigned short nLen);
    bool ngx_heartbeat(SNgx_connection *c,SNgx_message *pMsg,char *pBuf,unsigned short nLen);
    
private:
    static pkg_handler  m_handler[];
private:
    std::vector<SNgx_pkgHandler *>      m_vectHandler;
    CNgx_cpp_memery                    *m_pMem;
    CNgx_cpp_crc32                     *m_pCrc32;
    pthread_mutex_t                     m_mutex;
    int                                 m_nHandler;
};