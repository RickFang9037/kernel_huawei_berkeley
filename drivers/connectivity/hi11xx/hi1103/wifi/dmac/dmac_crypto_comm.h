

#ifndef __DMAC_CRYPTO_COMM_H__
#define __DMAC_CRYPTO_COMM_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#ifdef _PRE_WLAN_FEATURE_SOFT_CRYPTO

/*****************************************************************************
  1 其他头文件包含
*****************************************************************************/
#include "oam_ext_if.h"

#include "hal_commom_ops.h"

#include "mac_frame.h"

#include "wlan_types.h"


#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_DMAC_CRYPTO_COMM_H
/*****************************************************************************
  2 宏定义
*****************************************************************************/

/*****************************************************************************
  3 枚举定义
*****************************************************************************/

/*****************************************************************************
  4 全局变量声明
*****************************************************************************/
struct arc4_ctx
{
    oal_uint8 S[256];
    oal_uint8 x, y;
};

/*****************************************************************************
  5 消息头定义
*****************************************************************************/


/*****************************************************************************
  6 消息定义
*****************************************************************************/


/*****************************************************************************
  7 STRUCT定义
*****************************************************************************/


/*****************************************************************************
  8 UNION定义
*****************************************************************************/


/*****************************************************************************
  9 OTHERS定义
*****************************************************************************/


/*****************************************************************************
  10 函数声明
*****************************************************************************/
extern oal_void arc4_set_key(OAL_CONST oal_uint8 *puc_key, oal_uint32 ul_key_len);
extern oal_void arc4_crypt(oal_uint8 *out, OAL_CONST oal_uint8 *in);
extern oal_uint32 do_crc32(oal_uint8 *puc_data, oal_uint32 ul_data_len, oal_uint8 uc_comp);


#endif /* _PRE_WLAN_FEATURE_SOFT_CRYPTO */

#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

#endif /* #ifndef __DMAC_CRYPTO_COMM_H__ */
