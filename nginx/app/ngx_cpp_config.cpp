#include "ngx_cpp_config.h"

using namespace std;
CNgx_cpp_config * CNgx_cpp_config::m_pconfig=nullptr;

CNgx_cpp_config * CNgx_cpp_config::get_m_pconfig()
{
    if(!m_pconfig)
    {
        m_pconfig=new CNgx_cpp_config;
        static CRecycle recycle;
    }
    return m_pconfig;
}

/*加载配置文件信息*/
bool CNgx_cpp_config::config_load()
{
    FILE * fd=fopen("nginx.conf","r");
    if( fd < 0 )
    {
        cout << "CNgx_cpp_config::config_load()中的fopen()出错" << endl;
        return false;
    }
    char ALineBuf[501]{0};
    /*feof(fd)文件是否结束，是返回1，否返回0；*/
    while(!feof(fd))
    {
        if(nullptr==fgets(ALineBuf,sizeof(ALineBuf),fd))
            continue;
        if(0==*ALineBuf)
            continue;
        if( *ALineBuf=='#' || *ALineBuf=='[' || *ALineBuf=='\n' 
                        || *ALineBuf=='\r' || *ALineBuf=='\t' || *ALineBuf==' ')
            continue;
        int n=strlen(ALineBuf);
tryagaing:
        if(n > 0)
        {
            if(ALineBuf[n-1]==10 || ALineBuf[n-1]==13 || ALineBuf[n-1]==32)
            {
                --n;
                goto tryagaing;
            }
            else
            {
                ALineBuf[n]='\0';
            }   
        }
        /*截取配置文件中信息保存*/
        cut_out(ALineBuf);
    }
    fclose(fd);
    return true;
}

/*截取配置文件中的信息放在vector中*/
 void CNgx_cpp_config::cut_out(char *p_buf)
 {
    char * pContent=nullptr;
    char * pName=strchr(p_buf,'=');
    pContent=pName;
    SConfigItem * pSConfigItem = new SConfigItem;
    while(++pContent)
    {
        if(*pContent!=' ' && *pContent!='=')
        {
            strcpy(pSConfigItem->ItemContent,pContent);
            break;
        }
    }
    while(--pName)
    {
        if(*pName!=' ')
        {
            *(++pName)='\0';
            strcpy(pSConfigItem->ItemName,p_buf);
            break;
        }
    }
    m_configitem.push_back(pSConfigItem);
 }

 const char * CNgx_cpp_config::get_string(const char * p_itemname)const
 {
    for(SConfigItem * p : m_configitem)
    {
        if(0 == strcmp(p_itemname,p->ItemName))
            return p->ItemContent;
    }
    return nullptr;

 }
    
 const int CNgx_cpp_config::get_def_int(const char * p_itemname,int def)const
 {
    for(SConfigItem * p : m_configitem)
    {
        if(0 == strcmp(p_itemname,p->ItemName))
            return atoi(p->ItemContent);
    }
    return def;
 }