#pragma once
#include "ngx_cpp_global.h"
#include <vector>

/*设计模式之单例类*/
class CNgx_cpp_config
{
public:
    static CNgx_cpp_config * get_m_pconfig();
    bool config_load();
    void cut_out(char * p_buf);
    const char * get_string(const char * p_itemname)const;
    const int get_def_int(const char * p_itemname,int def)const;
public:
    /*日志文件信息结构*/
    struct SConfigItem
    {
        char ItemName[50];
        char ItemContent[500];
    };

public:
    class CRecycle
    {
    public:
        ~CRecycle()
        {
            if(m_pconfig)
                delete m_pconfig;
        }
    };

private:
    static CNgx_cpp_config *     m_pconfig;
    std::vector<SConfigItem *>   m_configitem;
private:
    CNgx_cpp_config(){}
    ~CNgx_cpp_config()
    {
        for(SConfigItem * p : m_configitem)
            delete p;
        m_configitem.clear();
    }
};
