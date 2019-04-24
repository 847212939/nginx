#pragma once

#define PKG_MAX_LENTH   30000

#define PKG_HD_INIT     0
#define PKG_HD_RECVING  1
#define PKG_BD_INIT     2
#define PKG_BD_RECVING  3

#define PKG_HD_LENTH    20

#pragma pack (1)

/*包头结构*/
struct SNgx_pkgHead
{
    unsigned short  sp_nPkgLen;
    unsigned short  sp_nMegCode;
    int             sp_ncrc32;
};
#pragma pack ()
