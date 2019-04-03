#include "ngx_cpp_log_message.h"
#include "ngx_cpp_config.h"

using namespace std;

u_char CNgx_cpp_log_message::err_levels[][20]  = 
{
    {"stderr"},    /*0：控制台错误*/
    {"emerg"},     /*1：紧急*/
    {"alert"},     /*2：警戒*/
    {"crit"},      /*3：严重*/
    {"error"},     /*4：错误*/
    {"warn"},      /*5：警告*/
    {"notice"},    /*6：注意*/
    {"info"},      /*7：信息*/
    {"debug"}      /*8：调试*/
};

void CNgx_cpp_log_message::ngx_log_init()
{
    u_char *plogname = NULL;
    size_t nlen=0;
    CNgx_cpp_config *p_config = CNgx_cpp_config::get_m_pconfig();
    plogname = (u_char *)p_config->get_string("Log");
    if(plogname == NULL)
        plogname = (u_char *) NGX_ERROR_LOG_PATH;
    m_log.log_level = p_config->get_def_int("LogLevel",NGX_LOG_NOTICE);
    m_log.fd = open((const char *)plogname,O_WRONLY|O_APPEND|O_CREAT,0644);  
    if (m_log.fd == -1)
    {
        ngx_log_stderr(errno,"[alert] could not open error log file: open() \"%s\" failed", plogname);
        m_log.fd = STDERR_FILENO;
    } 
    return;
}

void CNgx_cpp_log_message::ngx_log_error_core(int level,  int err, const char *fmt, ...)
{
    u_char  *last;
    u_char  errstr[NGX_MAX_ERROR_STR+1]{0};
    last = errstr + NGX_MAX_ERROR_STR;   
    struct timeval   tv;
    struct tm        tm;
    time_t           sec; 
    u_char           *p; 
    va_list          args;
    memset(&tv,0,sizeof(struct timeval));
    memset(&tm,0,sizeof(struct tm));
    gettimeofday(&tv, NULL);     
    sec = tv.tv_sec;
    localtime_r(&sec, &tm);    /*线程安全可重入函数*/
    tm.tm_mon++;
    tm.tm_year += 1900;
    u_char strcurrtime[40]={0};
    ngx_slprintf(strcurrtime,  
                    (u_char *)-1,
                    "%4d/%02d/%02d %02d:%02d:%02d",
                    tm.tm_year, tm.tm_mon,
                    tm.tm_mday, tm.tm_hour,
                    tm.tm_min, tm.tm_sec);
    p = ngx_cpymem(errstr,strcurrtime,strlen((const char *)strcurrtime));
    p = ngx_slprintf(p, last, " [%s] ", err_levels[level]);
    p = ngx_slprintf(p, last, "%P: ",m_pid);
    va_start(args, fmt);
    p = ngx_vslprintf(p, last, fmt, args);
    va_end(args);
    if (err)
        p = ngx_log_errno(p, last, err);
    if (p >= (last - 1))
        p = (last - 1) - 1;
    *p++ = '\n';   
    ssize_t   n;
    while(1) 
    {        
        if (level > m_log.log_level) 
            break;
        n = write(m_log.fd,errstr,p - errstr);  //文件写入成功后，如果中途
        if (n == -1) 
        {
            if(errno == ENOSPC)
            {
                //磁盘没空间了
                //没空间还写个毛线啊
                //先do nothing吧；
            }
            else
            {
                if(m_log.fd != STDERR_FILENO) //当前是定位到文件的，则条件成立
                {
                    n = write(STDERR_FILENO,errstr,p - errstr);
                }
            }
        }
        break;
    } 
    return;
}

void CNgx_cpp_log_message::ngx_log_stderr(int err, const char *fmt, ...)
{
    va_list args;
    u_char  errstr[NGX_MAX_ERROR_STR+1]{0}; 
    u_char  *p,*last;
    last = errstr + NGX_MAX_ERROR_STR;
    p = ngx_cpymem(errstr, "nginx: ", 7);
    va_start(args, fmt);
    p = ngx_vslprintf(p,last,fmt,args);
    va_end(args);
    if (err)
        p = ngx_log_errno(p, last, err);
    if (p >= (last - 1))
        p = (last - 1) - 1;
    *p++ = '\n';
    if(m_log.fd>STDERR_FILENO)
    {
        ngx_log_error_core(NGX_LOG_NOTICE,err,(const char *)errstr);
        return ;
    }
    write(STDERR_FILENO,errstr,p - errstr);
    
}

/*把错误码对应的字符串组合到buf中一起打印到日志文件中去*/
u_char * CNgx_cpp_log_message::ngx_log_errno(u_char *buf, u_char *last, int err)
{
    char * p_error=strerror(err);
    size_t len = strlen(p_error);
    char leftstr[10]{0}; 
    sprintf(leftstr," (%d: ",err);
    size_t leftlen = strlen(leftstr);
    char rightstr[] = ") "; 
    size_t rightlen = strlen(rightstr);
    size_t extralen = leftlen + rightlen; //左右的额外宽度
    if ((buf + len + extralen) < last)
    {
        buf = ngx_cpymem(buf, leftstr, leftlen);
        buf = ngx_cpymem(buf, p_error, len);
        buf = ngx_cpymem(buf, rightstr, rightlen);
    }
    return buf;
}

u_char *CNgx_cpp_log_message::ngx_vslprintf(u_char *buf, u_char *last,const char *fmt,va_list args)
{
    u_char     zero;
    uintptr_t  width,sign,hex,frac_width,scale,n;
    int64_t    i64;
    uint64_t   ui64; 
    u_char     *p;
    double     f;
    uint64_t   frac;
    while (*fmt && buf < last)
    {
        if (*fmt == '%')
        {
            zero  = (u_char) ((*++fmt == '0') ? '0' : ' ');
            width = 0;
            sign  = 1;
            hex   = 0;
            frac_width = 0;
            i64 = 0;
            ui64 = 0;
            while (*fmt >= '0' && *fmt <= '9')   
                width = width * 10 + (*fmt++ - '0');
            while(1)
            {
                switch (*fmt)
                {
                case 'u':
                    sign = 0;
                    fmt++;
                    continue;
                case 'X':
                    hex = 2;
                    sign = 0;
                    fmt++;
                    continue;
                case 'x':
                    hex = 1;
                    sign = 0;
                    fmt++;
                    continue;
                case '.':
                    fmt++;
                    while(*fmt >= '0' && *fmt <= '9')
                        frac_width = frac_width * 10 + (*fmt++ - '0'); 
                    break;
                default:
                    break;                
                }
                break;
            }
            switch (*fmt) 
            {
            case '%':
                *buf++ = '%';
                fmt++;
                continue;

            case 'd':
                if (sign)
                    i64 = (int64_t) va_arg(args, int);
                else
                    ui64 = (uint64_t) va_arg(args, u_int);    
                break;
            case 's':
                p = va_arg(args, u_char *);
                while (*p && buf < last)
                    *buf++ = *p++;
                fmt++;
                continue;
            case 'P':
                i64 = (int64_t) va_arg(args, pid_t);
                sign = 1;
                break;
            case 'f': 
                f = va_arg(args, double);
                if (f < 0)
                {
                    *buf++ = '-';
                    f = -f;
                }
                ui64 = (int64_t)f;
                frac = 0;
                if (frac_width)
                {
                    scale = 1;
                    for (n = frac_width; n; n--) 
                        scale *= 10;
                    frac = (uint64_t) ((f - (double) ui64) * scale + 0.5);
                    if (frac == scale)   
                    {
                        ui64++;
                        frac = 0;
                    }
                } 
                buf = ngx_sprintf_num(buf, last, ui64, zero, 0, width);
                if (frac_width)
                {
                    if (buf < last) 
                        *buf++ = '.';
                    buf = ngx_sprintf_num(buf, last, frac, '0', 0, frac_width);
                }
                fmt++;
                continue;
            default:
                *buf++ = *fmt++;
                continue; 
            }
            if (sign)
            {
                if (i64 < 0)
                {
                    *buf++ = '-';
                    ui64 = (uint64_t) -i64;
                }
                else
                    ui64 = (uint64_t) i64;
            }  
            buf = ngx_sprintf_num(buf, last, ui64, zero, hex, width); 
            fmt++;
        }
        else
            *buf++ = *fmt++;
    }
    return buf;
}

u_char * CNgx_cpp_log_message::ngx_sprintf_num(u_char *buf, u_char *last, uint64_t ui64, u_char zero, 
    uintptr_t hexadecimal, uintptr_t width)
{
    u_char      *p, temp[NGX_INT64_LEN + 1];            
    size_t      len;
    uint32_t    ui32;
    static u_char   hex[] = "0123456789abcdef";
    static u_char   HEX[] = "0123456789ABCDEF";
    p = temp + NGX_INT64_LEN;
    if (hexadecimal == 0)  
    {
        if (ui64 <= (uint64_t) NGX_MAX_UINT32_VALUE)
        {
            ui32 = (uint32_t) ui64;
            do
                *--p = (u_char) (ui32 % 10 + '0');
            while (ui32 /= 10);
        }
        else
        {
            do 
                *--p = (u_char) (ui64 % 10 + '0');
            while (ui64 /= 10);
        }
    }
    else if (hexadecimal == 1)
    {
        do 
            *--p = hex[(uint32_t) (ui64 & 0xf)];    
        while (ui64 >>= 4);
    } 
    else
    { 
        do 
            *--p = HEX[(uint32_t) (ui64 & 0xf)];
        while (ui64 >>= 4);
    }
    len = (temp + NGX_INT64_LEN) - p;
    while (len++ < width && buf < last)
        *buf++ = zero;
    len = (temp + NGX_INT64_LEN) - p;
    if((buf + len) >= last)
        len = last - buf;
    return ngx_cpymem(buf, p, len);
}

u_char *CNgx_cpp_log_message::ngx_slprintf(u_char *buf, u_char *last, const char *fmt, ...) 
{
    va_list   args;
    u_char   *p;
    va_start(args, fmt); //使args指向起始的参数
    p = ngx_vslprintf(buf, last, fmt, args);
    va_end(args);        //释放args   
    return p;
}



