#pragma once

#pragma pack (1)

struct SNgx_login
{
    char    si_sName[32];
    char    si_sPass[32];
};

struct SNgx_register
{
    int     sr_nType;
    char    sr_sName[56];
    char    sr_sPass[40];
};





#pragma pack ()