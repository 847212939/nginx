#pragma once 
#include "ngx_cpp_global.h"
#include "ngx_cpp_config.h"

class CNgx_cpp_socket
{
public:
    struct SNgx_listening
    {
        int fd;
        int port;    /*监听端口*/
    };
public:
    bool ngx_open_listening_sockets();
    bool setnonblocking(int sockfd);
    void ngx_close_listening_sockets();
public:
    CNgx_cpp_socket();
    ~CNgx_cpp_socket();
private:
    std::vector<SNgx_listening *> m_vect;
    int m_ListenCount;
};

