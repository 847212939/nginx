#pragma once
#include "ngx_cpp_global.h"

class CNgx_cpp_log_message
{
public:
    void ngx_log_init();
    void ngx_log_error_core(int level,  int err, const char *fmt, ...);
    void ngx_log_stderr(int err, const char *fmt, ...);
    u_char * ngx_snprintf(u_char *buf, size_t max, const char *fmt, ...);
public:
    CNgx_cpp_log_message():m_pid(0)
    {
        m_log.fd=-1;
    };
    ~CNgx_cpp_log_message()
    {
        if(m_log.fd)
            close(m_log.fd);
    };
private:
    struct SNgx_log
    {
	    int    log_level;   /*日志级别 或者日志类型，ngx_macro.h里分0-8共9个级别*/
	    int    fd;          /*日志文件描述符*/
    };
    static u_char err_levels[][20];
private:
    static u_char * ngx_log_errno(u_char *buf, u_char *last, int err);
    static u_char * ngx_vslprintf(u_char *buf, u_char *last,const char *fmt,va_list args);
    static u_char * ngx_sprintf_num(u_char *buf, u_char *last, uint64_t ui64, u_char zero, uintptr_t hexadecimal, uintptr_t width);
    static u_char * ngx_slprintf(u_char *buf, u_char *last, const char *fmt, ...); 
private:
    SNgx_log m_log;
    pid_t    m_pid;
};


