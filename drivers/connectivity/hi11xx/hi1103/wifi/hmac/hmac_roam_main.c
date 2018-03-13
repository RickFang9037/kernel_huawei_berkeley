


#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#ifdef _PRE_WLAN_FEATURE_ROAM

/*****************************************************************************
  1 头文件包含
*****************************************************************************/
#include "oam_ext_if.h"
#include "mac_ie.h"
#include "mac_device.h"
#include "mac_resource.h"
#include "dmac_ext_if.h"
#include "hmac_fsm.h"
#include "hmac_sme_sta.h"
#include "hmac_mgmt_sta.h"
#include "hmac_resource.h"
#include "hmac_device.h"
#include "hmac_11i.h"
#include "hmac_roam_main.h"
#include "hmac_roam_connect.h"
#include "hmac_roam_alg.h"
#ifdef _PRE_PLAT_FEATURE_CUSTOMIZE
#include "hisi_customize_wifi.h"
#endif /* #ifdef _PRE_PLAT_FEATURE_CUSTOMIZE */

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_ROAM_MAIN_C

/*****************************************************************************
  2 全局变量定义
*****************************************************************************/
OAL_STATIC hmac_roam_fsm_func g_hmac_roam_main_fsm_func[ROAM_MAIN_STATE_BUTT][ROAM_MAIN_FSM_EVENT_TYPE_BUTT];
OAL_STATIC oal_uint32  hmac_roam_main_null_fn(hmac_roam_info_stru *pst_roam_info, oal_void *p_param);
OAL_STATIC oal_uint32  hmac_roam_scan_init(hmac_roam_info_stru *pst_roam_info, oal_void *p_param);
OAL_STATIC oal_uint32  hmac_roam_scan_channel(hmac_roam_info_stru *pst_roam_info, oal_void *p_param);
OAL_STATIC oal_uint32  hmac_roam_check_scan_result(hmac_roam_info_stru *pst_roam_info, oal_void *p_param, mac_bss_dscr_stru **ppst_bss_dscr_out);
OAL_STATIC oal_uint32  hmac_roam_connect_to_bss(hmac_roam_info_stru *pst_roam_info, oal_void *p_param);
OAL_STATIC oal_uint32  hmac_roam_to_old_bss(hmac_roam_info_stru *pst_roam_info, oal_void *p_param);
OAL_STATIC oal_uint32  hmac_roam_to_new_bss(hmac_roam_info_stru *pst_roam_info, oal_void *p_param);
OAL_STATIC oal_uint32  hmac_roam_main_del_timer(hmac_roam_info_stru *pst_roam_info);
OAL_STATIC oal_uint32  hmac_roam_main_check_state(hmac_roam_info_stru *pst_roam_info, mac_vap_state_enum_uint8 en_vap_state, roam_main_state_enum_uint8 en_main_state);
OAL_STATIC oal_void    hmac_roam_main_clear(hmac_roam_info_stru *pst_roam_info);
OAL_STATIC oal_uint32  hmac_roam_scan_timeout(hmac_roam_info_stru *pst_roam_info, oal_void *p_param);
OAL_STATIC oal_uint32  hmac_roam_connecting_timeout(hmac_roam_info_stru *pst_roam_info, oal_void *p_param);
OAL_STATIC oal_uint32  hmac_roam_connecting_fail(hmac_roam_info_stru *pst_roam_info, oal_void *p_param);
OAL_STATIC oal_uint32  hmac_roam_handle_fail_handshake_phase(hmac_roam_info_stru *pst_roam_info, oal_void *p_param);
OAL_STATIC oal_uint32  hmac_roam_handle_scan_result(hmac_roam_info_stru *pst_roam_info, oal_void *p_param);

/*****************************************************************************
  3 函数实现
*****************************************************************************/

OAL_STATIC oal_void hmac_roam_fsm_init(oal_void)
{
    oal_uint32  ul_state;
    oal_uint32  ul_event;

    for (ul_state = 0; ul_state < ROAM_MAIN_STATE_BUTT; ul_state++)
    {
        for (ul_event = 0; ul_event < ROAM_MAIN_FSM_EVENT_TYPE_BUTT; ul_event++)
        {
            g_hmac_roam_main_fsm_func[ul_state][ul_event] = hmac_roam_main_null_fn;
        }
    }
    g_hmac_roam_main_fsm_func[ROAM_MAIN_STATE_INIT][ROAM_MAIN_FSM_EVENT_START]                = hmac_roam_scan_init;
    g_hmac_roam_main_fsm_func[ROAM_MAIN_STATE_SCANING][ROAM_MAIN_FSM_EVENT_START]             = hmac_roam_scan_channel;
    g_hmac_roam_main_fsm_func[ROAM_MAIN_STATE_SCANING][ROAM_MAIN_FSM_EVENT_SCAN_RESULT]       = hmac_roam_handle_scan_result;
    g_hmac_roam_main_fsm_func[ROAM_MAIN_STATE_SCANING][ROAM_MAIN_FSM_EVENT_TIMEOUT]           = hmac_roam_scan_timeout;
    g_hmac_roam_main_fsm_func[ROAM_MAIN_STATE_SCANING][ROAM_MAIN_FSM_EVENT_START_CONNECT]     = hmac_roam_connect_to_bss;
    g_hmac_roam_main_fsm_func[ROAM_MAIN_STATE_CONNECTING][ROAM_MAIN_FSM_EVENT_TIMEOUT]        = hmac_roam_connecting_timeout;
    g_hmac_roam_main_fsm_func[ROAM_MAIN_STATE_CONNECTING][ROAM_MAIN_FSM_EVENT_CONNECT_FAIL]   = hmac_roam_connecting_fail;
    g_hmac_roam_main_fsm_func[ROAM_MAIN_STATE_CONNECTING][ROAM_MAIN_FSM_EVENT_HANDSHAKE_FAIL] = hmac_roam_handle_fail_handshake_phase;
    g_hmac_roam_main_fsm_func[ROAM_MAIN_STATE_CONNECTING][ROAM_MAIN_FSM_EVENT_CONNECT_SUCC]   = hmac_roam_to_new_bss;
}


oal_uint32  hmac_roam_main_fsm_action(hmac_roam_info_stru *pst_roam_info, roam_main_fsm_event_type_enum en_event, oal_void *p_param)
{
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_roam_info))
    {
        return OAL_ERR_CODE_PTR_NULL;
    }

    if (pst_roam_info->en_main_state >= ROAM_MAIN_STATE_BUTT)
    {
        return OAL_ERR_CODE_ROAM_STATE_UNEXPECT;
    }

    if (en_event >= ROAM_MAIN_FSM_EVENT_TYPE_BUTT)
    {
        return OAL_ERR_CODE_ROAM_EVENT_UXEXPECT;
    }

    return g_hmac_roam_main_fsm_func[pst_roam_info->en_main_state][en_event](pst_roam_info, p_param);
}


OAL_STATIC oal_void  hmac_roam_main_change_state(hmac_roam_info_stru *pst_roam_info, roam_main_state_enum_uint8 en_state)
{
    if (pst_roam_info)
    {
        OAM_WARNING_LOG2(0, OAM_SF_ROAM,
                      "{hmac_roam_main_change_state::[%d]->[%d]}", pst_roam_info->en_main_state, en_state);
        pst_roam_info->en_main_state = en_state;
    }
}


OAL_STATIC oal_uint32 hmac_roam_main_timeout(oal_void *p_arg)
{
    hmac_roam_info_stru *pst_roam_info;

    if (OAL_UNLIKELY(OAL_PTR_NULL == p_arg))
    {
        return OAL_ERR_CODE_PTR_NULL;
    }
    pst_roam_info = (hmac_roam_info_stru *)p_arg;

    OAM_WARNING_LOG2(0, OAM_SF_ROAM, "{hmac_roam_main_timeout::MAIN_STATE[%d] CONNECT_STATE[%d].}",
                     pst_roam_info->en_main_state, pst_roam_info->st_connect.en_state);

    return hmac_roam_main_fsm_action(pst_roam_info, ROAM_MAIN_FSM_EVENT_TIMEOUT, OAL_PTR_NULL);
}


OAL_STATIC oal_uint32 hmac_roam_renew_privacy(hmac_vap_stru *pst_hmac_vap, mac_bss_dscr_stru *pst_bss_dscr)
{
    oal_uint32                          ul_ret;
    oal_uint32                          ul_match_suite;
    mac_conn_security_stru              st_conn_sec;
    mac_cap_info_stru                  *pst_cap_info;

    pst_cap_info = (mac_cap_info_stru *)&pst_bss_dscr->us_cap_info;

    if (0 == pst_cap_info->bit_privacy)
    {
        return OAL_SUCC;
    }

    if (NL80211_AUTHTYPE_OPEN_SYSTEM != mac_mib_get_AuthenticationMode(&pst_hmac_vap->st_vap_base_info))
    {
        return OAL_SUCC;
    }

    OAL_MEMZERO(&st_conn_sec, OAL_SIZEOF(mac_conn_security_stru));

    if (OAL_PTR_NULL != pst_bss_dscr->puc_wpa_ie)
    {
        st_conn_sec.en_privacy                    = OAL_TRUE;
        mac_ie_get_wpa_cipher(pst_bss_dscr->puc_wpa_ie, &(st_conn_sec.st_crypto));

        ul_match_suite = mac_mib_wpa_pair_match_suites(&pst_hmac_vap->st_vap_base_info, st_conn_sec.st_crypto.aul_pair_suite);
        if (0 != ul_match_suite)
        {
            st_conn_sec.st_crypto.aul_pair_suite[0] = ul_match_suite;
        }
        st_conn_sec.st_crypto.aul_pair_suite[1] = 0;

        ul_match_suite = mac_mib_wpa_akm_match_suites(&pst_hmac_vap->st_vap_base_info, st_conn_sec.st_crypto.aul_pair_suite);
        if (0 != ul_match_suite)
        {
            st_conn_sec.st_crypto.aul_akm_suite[0] = ul_match_suite;
        }
        st_conn_sec.st_crypto.aul_akm_suite[1] = 0;
    }

    if (OAL_PTR_NULL != pst_bss_dscr->puc_rsn_ie)
    {
        st_conn_sec.en_privacy                    = OAL_TRUE;
        mac_ie_get_rsn_cipher(pst_bss_dscr->puc_rsn_ie, &(st_conn_sec.st_crypto));

        ul_match_suite = mac_mib_rsn_pair_match_suites(&pst_hmac_vap->st_vap_base_info, st_conn_sec.st_crypto.aul_pair_suite);
        if (0 != ul_match_suite)
        {
            st_conn_sec.st_crypto.aul_pair_suite[0] = ul_match_suite;
        }
        st_conn_sec.st_crypto.aul_pair_suite[1] = 0;

        ul_match_suite = mac_mib_rsn_akm_match_suites(&pst_hmac_vap->st_vap_base_info, st_conn_sec.st_crypto.aul_pair_suite);
        if (0 != ul_match_suite)
        {
            st_conn_sec.st_crypto.aul_akm_suite[0] = ul_match_suite;
        }
        st_conn_sec.st_crypto.aul_akm_suite[1] = 0;
    }

    ul_ret = mac_vap_init_privacy(&pst_hmac_vap->st_vap_base_info, &st_conn_sec);
    if (OAL_SUCC != ul_ret)
    {
        OAM_ERROR_LOG1(0, OAM_SF_CFG, "{hmac_roam_renew_privacy:: mac_11i_init_privacy failed[%d]!}\r\n", ul_ret);
        return ul_ret;
    }

    return OAL_SUCC;
}


oal_uint32 hmac_roam_init(hmac_vap_stru *pst_hmac_vap)
{
    hmac_roam_info_stru    *pst_roam_info;
    oal_uint32              i = 0;

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_hmac_vap))
    {
        return OAL_ERR_CODE_PTR_NULL;
    }

    if (!IS_LEGACY_VAP(&(pst_hmac_vap->st_vap_base_info)))
    {
        return OAL_SUCC;
    }

    if (OAL_PTR_NULL == pst_hmac_vap->pul_roam_info)
    {
        /* 漫游主结构体内存申请 */
        pst_hmac_vap->pul_roam_info = (oal_uint32 *)OAL_MEM_ALLOC(OAL_MEM_POOL_ID_LOCAL, OAL_SIZEOF(hmac_roam_info_stru), OAL_TRUE);
        if (OAL_PTR_NULL == pst_hmac_vap->pul_roam_info)
        {
            OAM_ERROR_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ROAM, "{hmac_roam_init::OAL_MEM_ALLOC fail.}");
            return OAL_ERR_CODE_ALLOC_MEM_FAIL;
        }
    }
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    else
    {
        hmac_roam_main_del_timer((hmac_roam_info_stru *)pst_hmac_vap->pul_roam_info);
    }
#endif

    pst_roam_info = (hmac_roam_info_stru *)pst_hmac_vap->pul_roam_info;


    /* TBD 参数初始化 */
    oal_memset(pst_hmac_vap->pul_roam_info, 0, OAL_SIZEOF(hmac_roam_info_stru));
#ifdef _PRE_PLAT_FEATURE_CUSTOMIZE
    pst_roam_info->uc_enable             = g_st_wlan_customize.uc_roam_switch;
#else
    pst_roam_info->uc_enable             = 1;
#endif
    pst_roam_info->en_main_state         = ROAM_MAIN_STATE_INIT;
    pst_roam_info->en_current_bss_ignore     = OAL_FALSE;
    pst_roam_info->pst_hmac_vap          = pst_hmac_vap;
    pst_roam_info->pst_hmac_user         = OAL_PTR_NULL;
    pst_roam_info->ul_connected_state    = WPAS_CONNECT_STATE_INIT;
#ifdef _PRE_PLAT_FEATURE_CUSTOMIZE
    pst_roam_info->st_config.uc_scan_band         = (mac_device_check_5g_enable(pst_hmac_vap->st_vap_base_info.uc_device_id)) ? (BIT0|BIT1) : BIT0;
    pst_roam_info->st_config.uc_scan_orthogonal   = g_st_wlan_customize.uc_roam_scan_orthogonal;
    pst_roam_info->st_config.c_trigger_rssi_2G    = g_st_wlan_customize.c_roam_trigger_b;
    pst_roam_info->st_config.c_trigger_rssi_5G    = g_st_wlan_customize.c_roam_trigger_a;
    pst_roam_info->st_config.uc_delta_rssi_2G     = g_st_wlan_customize.c_roam_delta_b;
    pst_roam_info->st_config.uc_delta_rssi_5G     = g_st_wlan_customize.c_roam_delta_a;
#else
    pst_roam_info->st_config.uc_scan_band         = ROAM_BAND_2G_BIT|ROAM_BAND_5G_BIT;
    pst_roam_info->st_config.uc_scan_orthogonal   = ROAM_SCAN_CHANNEL_ORG_BUTT;
    pst_roam_info->st_config.c_trigger_rssi_2G    = ROAM_RSSI_NE70_DB;
    pst_roam_info->st_config.c_trigger_rssi_5G    = ROAM_RSSI_NE70_DB;
    pst_roam_info->st_config.uc_delta_rssi_2G     = ROAM_RSSI_DIFF_10_DB;
    pst_roam_info->st_config.uc_delta_rssi_5G     = ROAM_RSSI_DIFF_10_DB;
#endif

    for(i = 0; i<ROAM_LIST_MAX; i++)
    {
        pst_roam_info->st_alg.st_history.ast_bss[i].us_count_limit      = ROAM_HISTORY_COUNT_LIMIT;
        pst_roam_info->st_alg.st_history.ast_bss[i].ul_timeout          = ROAM_HISTORY_BSS_TIME_OUT;
    }

    for(i = 0; i<ROAM_LIST_MAX; i++)
    {
        pst_roam_info->st_alg.st_blacklist.ast_bss[i].us_count_limit    = ROAM_BLACKLIST_COUNT_LIMIT;
        pst_roam_info->st_alg.st_blacklist.ast_bss[i].ul_timeout        = ROAM_BLACKLIST_NORMAL_AP_TIME_OUT;
    }

    hmac_roam_fsm_init();
    hmac_roam_connect_fsm_init();

    OAM_WARNING_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ROAM, "{hmac_roam_init::SUCC.}");
    return OAL_SUCC;
}


oal_uint32 hmac_roam_info_init(hmac_vap_stru *pst_hmac_vap)
{
    hmac_roam_info_stru    *pst_roam_info;
    oal_uint32              ul_ret;
    mac_roam_trigger_stru   st_roam_trigger;

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_hmac_vap))
    {
        return OAL_ERR_CODE_PTR_NULL;
    }

    if (OAL_PTR_NULL == pst_hmac_vap->pul_roam_info)
    {
        return OAL_ERR_CODE_ROAM_INVALID_VAP;
    }

    pst_roam_info = (hmac_roam_info_stru *)pst_hmac_vap->pul_roam_info;

    pst_roam_info->en_main_state  = ROAM_MAIN_STATE_INIT;
    pst_roam_info->uc_rssi_ignore = 0;
    pst_roam_info->uc_invalid_scan_cnt = 0;
    pst_roam_info->st_connect.en_state = ROAM_CONNECT_STATE_INIT;
    pst_roam_info->st_alg.ul_max_capacity = 0;
    pst_roam_info->st_alg.pst_max_capacity_bss = OAL_PTR_NULL;
    pst_roam_info->st_alg.c_current_rssi = 0;
    pst_roam_info->st_alg.c_max_rssi = 0;
    pst_roam_info->st_alg.uc_another_bss_scaned = 0;
    pst_roam_info->st_alg.uc_invalid_scan_cnt = 0;
    pst_roam_info->st_alg.pst_max_rssi_bss = OAL_PTR_NULL;
    pst_roam_info->pst_hmac_user = mac_res_get_hmac_user(pst_hmac_vap->st_vap_base_info.us_assoc_vap_id);
    if (OAL_PTR_NULL == pst_roam_info->pst_hmac_user)
    {
        OAM_ERROR_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ROAM,
                       "{hmac_roam_info_init::assoc_vap_id[%d] can't found.}", pst_hmac_vap->st_vap_base_info.us_assoc_vap_id);
        return OAL_ERR_CODE_ROAM_INVALID_VAP;
    }

    st_roam_trigger.c_trigger_2G = pst_roam_info->st_config.c_trigger_rssi_2G;
    st_roam_trigger.c_trigger_5G = pst_roam_info->st_config.c_trigger_rssi_5G;

    ul_ret = hmac_config_send_event(&pst_hmac_vap->st_vap_base_info,
                                    WLAN_CFGID_SET_ROAM_TRIGGER,
                                    OAL_SIZEOF(mac_roam_trigger_stru),
                                    (oal_uint8 *)&st_roam_trigger);
    if (OAL_SUCC != ul_ret)
    {
        OAM_ERROR_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ROAM,
                       "{hmac_roam_info_init::send event[INIT_ROAM_TRIGGER] failed[%d].}", ul_ret);
        return ul_ret;
    }

    OAM_INFO_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ROAM, "{hmac_roam_info_init::SUCC.}");
    return OAL_SUCC;
}


oal_uint32 hmac_roam_exit(hmac_vap_stru *pst_hmac_vap)
{
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_hmac_vap))
    {
        return OAL_ERR_CODE_PTR_NULL;
    }

    if (!IS_LEGACY_VAP(&(pst_hmac_vap->st_vap_base_info)))
    {
        return OAL_SUCC;
    }

    if (OAL_PTR_NULL == pst_hmac_vap->pul_roam_info)
    {
        OAM_WARNING_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ROAM, "{hmac_roam_exit::pul_roam_info is NULL.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    FRW_TIMER_IMMEDIATE_DESTROY_TIMER(&(((hmac_roam_info_stru *)pst_hmac_vap->pul_roam_info)->st_connect.st_timer));
    hmac_roam_main_del_timer((hmac_roam_info_stru *)pst_hmac_vap->pul_roam_info);
    if (OAL_PTR_NULL != pst_hmac_vap->pst_net_device)
    {
        oal_net_tx_wake_all_queues(pst_hmac_vap->pst_net_device);
    }
    OAL_MEM_FREE(pst_hmac_vap->pul_roam_info, OAL_TRUE);
    pst_hmac_vap->pul_roam_info = OAL_PTR_NULL;

    OAM_WARNING_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ROAM, "{hmac_roam_exit::SUCC.}");

    return OAL_SUCC;
}


oal_uint32 hmac_roam_show_info(hmac_vap_stru *pst_hmac_vap)
{
    oal_int8               *pc_print_buff;
    hmac_roam_info_stru    *pst_roam_info;

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_hmac_vap))
    {
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_roam_info = (hmac_roam_info_stru *)pst_hmac_vap->pul_roam_info;
    if (OAL_PTR_NULL == pst_hmac_vap->pul_roam_info)
    {
        return OAL_SUCC;
    }

    pc_print_buff = (oal_int8 *)OAL_MEM_ALLOC(OAL_MEM_POOL_ID_LOCAL, OAM_REPORT_MAX_STRING_LEN, OAL_TRUE);
    if (OAL_PTR_NULL == pc_print_buff)
    {
        OAM_ERROR_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_CFG, "{hmac_config_vap_info::pc_print_buff null.}");
        return OAL_ERR_CODE_ALLOC_MEM_FAIL;
    }

    OAL_MEMZERO(pc_print_buff, OAM_REPORT_MAX_STRING_LEN);
    OAL_SPRINTF(pc_print_buff, OAM_REPORT_MAX_STRING_LEN,
                    "ROAM_EN[%d] MAIN_STATE[%d]\n"
                    "ROAM_SCAN_BAND[%d] ROAM_SCAN_ORTH[%d]\n",
                    pst_roam_info->uc_enable, pst_roam_info->en_main_state,
                    pst_roam_info->st_config.uc_scan_band, pst_roam_info->st_config.uc_scan_orthogonal);

    oam_print(pc_print_buff);

    OAL_MEM_FREE(pc_print_buff, OAL_TRUE);

    return OAL_SUCC;
}


OAL_STATIC oal_void hmac_roam_main_start_timer(hmac_roam_info_stru *pst_roam_info, oal_uint32 ul_timeout)
{
    frw_timeout_stru            *pst_timer = &(pst_roam_info->st_timer);

    OAM_INFO_LOG1(0, OAM_SF_ROAM, "{hmac_roam_main_start_timer [%d].}", ul_timeout);

    /* 启动认证超时定时器 */
    FRW_TIMER_CREATE_TIMER(pst_timer,
                           hmac_roam_main_timeout,
                           ul_timeout,
                           pst_roam_info,
                           OAL_FALSE,
                           OAM_MODULE_ID_HMAC,
                           pst_roam_info->pst_hmac_vap->st_vap_base_info.ul_core_id);
}


OAL_STATIC oal_uint32  hmac_roam_main_del_timer(hmac_roam_info_stru *pst_roam_info)
{
    OAM_INFO_LOG0(0, OAM_SF_ROAM, "{hmac_roam_main_del_timer.}");
    FRW_TIMER_IMMEDIATE_DESTROY_TIMER(&(pst_roam_info->st_timer));
    return OAL_SUCC;
}


oal_uint32 hmac_roam_enable(hmac_vap_stru *pst_hmac_vap, oal_uint8 uc_enable)
{
    hmac_roam_info_stru   *pst_roam_info;

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_hmac_vap))
    {
        OAM_ERROR_LOG0(0, OAM_SF_ROAM, "{hmac_roam_enable::pst_hmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_roam_info = (hmac_roam_info_stru *)pst_hmac_vap->pul_roam_info;
    if (OAL_PTR_NULL == pst_roam_info)
    {
        OAM_ERROR_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ROAM, "{hmac_roam_enable::pst_roam_info null .}");
        return OAL_ERR_CODE_ROAM_DISABLED;
    }

    if (uc_enable == pst_roam_info->uc_enable)
    {
        OAM_ERROR_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ROAM, "{hmac_roam_enable::SET[%d] fail .}", uc_enable);
        return OAL_FAIL;
    }

    /* 设置漫游开关 */
    pst_roam_info->uc_enable             = uc_enable;
    pst_roam_info->en_main_state         = ROAM_MAIN_STATE_INIT;
    OAM_INFO_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ROAM, "{hmac_roam_enable::SET[%d] OK!}", uc_enable);

    return OAL_SUCC;
}


oal_uint32 hmac_roam_org(hmac_vap_stru *pst_hmac_vap, oal_uint8 uc_scan_orthogonal)
{
    hmac_roam_info_stru   *pst_roam_info;

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_hmac_vap))
    {
        OAM_ERROR_LOG0(0, OAM_SF_ROAM, "{hmac_roam_org::pst_hmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_roam_info = (hmac_roam_info_stru *)pst_hmac_vap->pul_roam_info;
    if (OAL_PTR_NULL == pst_roam_info)
    {
        OAM_ERROR_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ROAM, "{hmac_roam_org::pst_roam_info null .}");
        return OAL_ERR_CODE_ROAM_DISABLED;
    }

    /* 设置漫游正交 */
    pst_roam_info->st_config.uc_scan_orthogonal = uc_scan_orthogonal;
    OAM_INFO_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ROAM, "{hmac_roam_org::SET[%d] OK!}", uc_scan_orthogonal);

    return OAL_SUCC;
}


oal_uint32 hmac_roam_band(hmac_vap_stru *pst_hmac_vap, oal_uint8 uc_scan_band)
{
    hmac_roam_info_stru   *pst_roam_info;

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_hmac_vap))
    {
        OAM_ERROR_LOG0(0, OAM_SF_ROAM, "{hmac_roam_band::pst_hmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_roam_info = (hmac_roam_info_stru *)pst_hmac_vap->pul_roam_info;
    if (OAL_PTR_NULL == pst_roam_info)
    {
        OAM_ERROR_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ROAM, "{hmac_roam_band::pst_roam_info null .}");
        return OAL_ERR_CODE_ROAM_DISABLED;
    }

    /* 设置漫游频段 */
    pst_roam_info->st_config.uc_scan_band = uc_scan_band;
    OAM_INFO_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ROAM, "{hmac_roam_band::SET[%d] OK!}", uc_scan_band);

    return OAL_SUCC;
}


oal_uint32 hmac_roam_start(hmac_vap_stru *pst_hmac_vap, roam_channel_org_enum uc_scan_type, oal_bool_enum_uint8 en_current_bss_ignore)
{
    oal_uint32             ul_ret;
    hmac_roam_info_stru   *pst_roam_info;

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_hmac_vap))
    {
        OAM_ERROR_LOG0(0, OAM_SF_ROAM, "{hmac_roam_start::pst_hmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_roam_info = (hmac_roam_info_stru *)pst_hmac_vap->pul_roam_info;
    if (OAL_PTR_NULL == pst_roam_info)
    {
        return OAL_ERR_CODE_ROAM_INVALID_VAP;
    }

    ul_ret = hmac_roam_main_check_state(pst_roam_info, MAC_VAP_STATE_UP, ROAM_MAIN_STATE_INIT);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ROAM, "{hmac_roam_start::check_state fail[%d]!}", ul_ret);
        return ul_ret;
    }

    /* 每次漫游前，刷新是否支持漫游到自己的参数 */
    pst_roam_info->st_config.uc_scan_orthogonal = uc_scan_type;
    pst_roam_info->en_current_bss_ignore = en_current_bss_ignore; /* true表示漫游到自己 */

    OAM_WARNING_LOG2(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ROAM,
        "{hmac_roam_start::START succ, uc_scan_type=%d, en_current_bss_ignore=%d.}", uc_scan_type, en_current_bss_ignore);

    hmac_roam_alg_init(pst_roam_info, ROAM_RSSI_CMD_TYPE);

    /* 触发漫游是否搭配扫描0表示不扫描 */
    if (ROAM_SCAN_CHANNEL_ORG_0 == uc_scan_type)
    {
        hmac_roam_main_change_state(pst_roam_info, ROAM_MAIN_STATE_SCANING);

        ul_ret = hmac_roam_main_fsm_action(pst_roam_info, ROAM_MAIN_FSM_EVENT_TIMEOUT, OAL_PTR_NULL);
        if (OAL_SUCC != ul_ret)
        {
            OAM_ERROR_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ROAM, "{hmac_roam_start::START fail[%d].}", ul_ret);
            return ul_ret;
        }
    }
    else
    {
        ul_ret = hmac_roam_main_fsm_action(pst_roam_info, ROAM_MAIN_FSM_EVENT_START, OAL_PTR_NULL);
        if (OAL_SUCC != ul_ret)
        {
            OAM_ERROR_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ROAM, "{hmac_roam_start::START fail[%d].}", ul_ret);
            return ul_ret;
        }
    }

    return OAL_SUCC;
}


oal_uint32 hmac_roam_show(hmac_vap_stru *pst_hmac_vap)
{
    hmac_roam_info_stru             *pst_roam_info;
    hmac_roam_static_stru           *pst_static;
    hmac_roam_config_stru           *pst_info;
    oal_uint8                        uc_vap_id;

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_hmac_vap))
    {
        OAM_ERROR_LOG0(0, OAM_SF_ROAM, "{hmac_roam_show::pst_hmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_roam_info = (hmac_roam_info_stru *)pst_hmac_vap->pul_roam_info;
    if (OAL_PTR_NULL == pst_roam_info)
    {
        OAM_ERROR_LOG0(0, OAM_SF_ROAM, "{hmac_roam_show::pst_roam_info null.}");
        return OAL_ERR_CODE_ROAM_INVALID_VAP;
    }

    pst_static = &pst_roam_info->st_static;
    pst_info   = &(pst_roam_info->st_config);

    uc_vap_id = pst_hmac_vap->st_vap_base_info.uc_vap_id;
    OAM_WARNING_LOG4(uc_vap_id, OAM_SF_ROAM, "{hmac_roam_show::band[%d] uc_scan_orthogonal[%d], c_trigger_rssi_2G=%d c_trigger_rssi_5G=%d}",
                   pst_info->uc_scan_band, pst_info->uc_scan_orthogonal, pst_info->c_trigger_rssi_2G, pst_info->c_trigger_rssi_5G);
    OAM_WARNING_LOG2(uc_vap_id, OAM_SF_ROAM, "{hmac_roam_show::trigger_rssi_cnt[0x%x] trigger_linkloss_cnt[0x%x] .}",
                   pst_static->ul_trigger_rssi_cnt, pst_static->ul_trigger_linkloss_cnt);
    OAM_WARNING_LOG2(uc_vap_id, OAM_SF_ROAM, "{hmac_roam_show::scan_cnt[0x%x] scan_result_cnt[0x%x].}",
                   pst_static->ul_scan_cnt, pst_static->ul_scan_result_cnt);
    OAM_WARNING_LOG3(uc_vap_id, OAM_SF_ROAM, "{hmac_roam_show::connect_cnt[0x%x] roam_new_cnt[0x%x] roam_old_cnt[0x%x].}",
                   pst_static->ul_connect_cnt, pst_static->ul_roam_new_cnt, pst_static->ul_roam_old_cnt);

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  hmac_roam_main_null_fn(hmac_roam_info_stru *pst_roam_info, oal_void *p_param)
{
    OAM_ERROR_LOG0(0, OAM_SF_ROAM, "{hmac_roam_main_null_fn .}");

    return OAL_SUCC;
}


OAL_STATIC oal_void  hmac_roam_scan_comp_cb(void  *p_scan_record)
{
    hmac_scan_record_stru           *pst_scan_record = (hmac_scan_record_stru *)p_scan_record;
    hmac_vap_stru                   *pst_hmac_vap = OAL_PTR_NULL;
    hmac_roam_info_stru             *pst_roam_info;
    hmac_device_stru                *pst_hmac_device;
    hmac_bss_mgmt_stru              *pst_scan_bss_mgmt;

    /* 获取hmac vap */
    pst_hmac_vap = mac_res_get_hmac_vap(pst_scan_record->uc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_ERROR_LOG0(0, OAM_SF_SCAN, "{hmac_roam_scan_comp_cb::pst_hmac_vap is null.");
        return;
    }

    /* 获取hmac device */
    pst_hmac_device = hmac_res_get_mac_dev(pst_hmac_vap->st_vap_base_info.uc_device_id);
    if (OAL_PTR_NULL == pst_hmac_device)
    {
        OAM_ERROR_LOG0(0, OAM_SF_ROAM, "{hmac_roam_scan_complete::device null!}");
        return;
    }

    pst_scan_bss_mgmt = &(pst_hmac_device->st_scan_mgmt.st_scan_record_mgmt.st_bss_mgmt);

    pst_roam_info = (hmac_roam_info_stru *)pst_hmac_vap->pul_roam_info;
    if (pst_roam_info == OAL_PTR_NULL)
    {
        return;
    }

    /* 漫游开关没有开时，不处理扫描结果 */
    if (pst_roam_info->uc_enable == 0)
    {
        return;
    }

    OAM_INFO_LOG0(pst_scan_record->uc_vap_id, OAM_SF_ROAM, "{hmac_roam_scan_complete::handling scan result!}");

    hmac_roam_main_fsm_action(pst_roam_info, ROAM_MAIN_FSM_EVENT_SCAN_RESULT, (void *)pst_scan_bss_mgmt);

    return;
}


OAL_STATIC oal_uint32  hmac_roam_scan_init(hmac_roam_info_stru *pst_roam_info, oal_void *p_param)
{
    oal_uint32              ul_ret;
    mac_scan_req_stru      *pst_scan_params;
    oal_uint8              *puc_cur_ssid;

    ul_ret = hmac_roam_main_check_state(pst_roam_info, MAC_VAP_STATE_UP, ROAM_MAIN_STATE_INIT);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ROAM, "{hmac_roam_scan_init::check_state fail[%d]!}", ul_ret);
        return ul_ret;
    }

    if (mac_is_wep_enabled(&(pst_roam_info->pst_hmac_vap->st_vap_base_info)))
    {
        hmac_roam_ignore_rssi_trigger((pst_roam_info->pst_hmac_vap), OAL_TRUE);
        return OAL_SUCC;
    }

    pst_scan_params = &pst_roam_info->st_scan_params;
    puc_cur_ssid = mac_mib_get_DesiredSSID(&(pst_roam_info->pst_hmac_vap->st_vap_base_info));

    /* 扫描参数初始化 */
    pst_scan_params->en_bss_type         = WLAN_MIB_DESIRED_BSSTYPE_INFRA;
    pst_scan_params->en_scan_type        = WLAN_SCAN_TYPE_ACTIVE;
    pst_scan_params->us_scan_time        = WLAN_DEFAULT_ACTIVE_SCAN_TIME;
    pst_scan_params->uc_probe_delay      = 0;
    pst_scan_params->uc_scan_func        = MAC_SCAN_FUNC_BSS;               /* 默认扫描bss */
    pst_scan_params->p_fn_cb             = hmac_roam_scan_comp_cb;
    pst_scan_params->uc_max_send_probe_req_count_per_channel = 2;
    pst_scan_params->uc_max_scan_count_per_channel           = 2;

    oal_memcopy(pst_scan_params->ast_mac_ssid_set[0].auc_ssid, puc_cur_ssid, WLAN_SSID_MAX_LEN);
    pst_scan_params->uc_ssid_num                             = 1;

    /* 初始扫描请求只指定1个bssid，为广播地址 */
    oal_memset(pst_scan_params->auc_bssid, 0xff, WLAN_MAC_ADDR_LEN);
    pst_scan_params->uc_bssid_num                            = 1;

    ul_ret = hmac_roam_alg_scan_channel_init(pst_roam_info, pst_scan_params);
    if (OAL_SUCC != ul_ret)
    {
        OAM_ERROR_LOG1(0, OAM_SF_ROAM,
                       "{hmac_roam_scan_init::hmac_roam_alg_scan_channel_init fail[%d]}", ul_ret);
        return ul_ret;
    }
    hmac_roam_main_change_state(pst_roam_info, ROAM_MAIN_STATE_SCANING);

    ul_ret = hmac_roam_main_fsm_action(pst_roam_info, ROAM_MAIN_FSM_EVENT_START, (oal_void *)pst_roam_info);
    if (OAL_SUCC != ul_ret)
    {
        hmac_roam_main_change_state(pst_roam_info, ROAM_MAIN_STATE_FAIL);
        return ul_ret;
    }

    return OAL_SUCC;
}

OAL_STATIC oal_uint32  hmac_roam_scan_channel(hmac_roam_info_stru *pst_roam_info, oal_void *p_param)
{
    oal_uint32              ul_ret;

    ul_ret = hmac_roam_main_check_state(pst_roam_info, MAC_VAP_STATE_UP, ROAM_MAIN_STATE_SCANING);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ROAM, "{hmac_roam_scan_channel::check_state fail[%d]!}", ul_ret);
        return ul_ret;
    }

    pst_roam_info->st_static.ul_scan_cnt++;

    /* 发起背景扫描 */
    ul_ret = hmac_fsm_call_func_sta(pst_roam_info->pst_hmac_vap, HMAC_FSM_INPUT_SCAN_REQ, (oal_void *)(&pst_roam_info->st_scan_params));
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG0(0, OAM_SF_ROAM, "{hmac_roam_scan_channel::start scan failed!}");
    }

    /* 启动扫描超时定时器 */
    hmac_roam_main_start_timer(pst_roam_info, ROAM_SCAN_TIME_MAX);

    return OAL_SUCC;
}


oal_uint32  hmac_roam_check_bkscan_result(hmac_vap_stru *pst_hmac_vap, void *p_scan_record)
{
    hmac_roam_info_stru     *pst_roam_info;
    hmac_scan_record_stru   *pst_scan_record = (hmac_scan_record_stru *)p_scan_record;
    hmac_device_stru        *pst_hmac_device;
    hmac_bss_mgmt_stru      *pst_scan_bss_mgmt;
    oal_dlist_head_stru     *pst_entry;
    hmac_scanned_bss_info   *pst_scanned_bss;
    mac_bss_dscr_stru       *pst_bss_dscr = OAL_PTR_NULL;

    if ((OAL_PTR_NULL == pst_hmac_vap) || (OAL_PTR_NULL == pst_scan_record))
    {
        OAM_ERROR_LOG0(0, OAM_SF_ROAM, "{hmac_roam_check_bkscan_result::param null}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_hmac_device = hmac_res_get_mac_dev(pst_hmac_vap->st_vap_base_info.uc_device_id);
    if (OAL_PTR_NULL == pst_hmac_device)
    {
        OAM_ERROR_LOG0(0, OAM_SF_ROAM, "{hmac_roam_check_bkscan_result::device is null}");
        return OAL_ERR_CODE_ROAM_INVALID_VAP;
    }

    if (mac_is_wep_enabled(&pst_hmac_vap->st_vap_base_info))
    {
        return OAL_SUCC;
    }

    pst_roam_info = (hmac_roam_info_stru *)pst_hmac_vap->pul_roam_info;
    if (OAL_PTR_NULL == pst_roam_info || 0 == pst_roam_info->uc_enable)
    {
        return OAL_SUCC;
    }

    pst_scan_bss_mgmt = &(pst_hmac_device->st_scan_mgmt.st_scan_record_mgmt.st_bss_mgmt);

    oal_spin_lock(&(pst_scan_bss_mgmt->st_lock));

    /* 遍历扫描到的bss信息，查找可以漫游的bss */
    OAL_DLIST_SEARCH_FOR_EACH(pst_entry, &(pst_scan_bss_mgmt->st_bss_list_head))
    {
        pst_scanned_bss = OAL_DLIST_GET_ENTRY(pst_entry, hmac_scanned_bss_info, st_dlist_head);
        pst_bss_dscr    = &(pst_scanned_bss->st_bss_dscr_info);

        hmac_roam_alg_bss_in_ess(pst_roam_info, pst_bss_dscr);

        pst_bss_dscr = OAL_PTR_NULL;
    }

    oal_spin_unlock(&(pst_scan_bss_mgmt->st_lock));

    if (OAL_TRUE == hmac_roam_alg_need_to_stop_roam_trigger(pst_roam_info))
    {
        return hmac_roam_ignore_rssi_trigger(pst_hmac_vap, OAL_TRUE);
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  hmac_roam_check_scan_result(hmac_roam_info_stru *pst_roam_info, oal_void *p_param, mac_bss_dscr_stru **ppst_bss_dscr_out)
{
    oal_uint32               ul_ret;
    hmac_bss_mgmt_stru      *pst_bss_mgmt;
    hmac_vap_stru           *pst_hmac_vap;
    oal_dlist_head_stru     *pst_entry;
    hmac_scanned_bss_info   *pst_scanned_bss;
    mac_bss_dscr_stru       *pst_bss_dscr = OAL_PTR_NULL;

    pst_hmac_vap    = pst_roam_info->pst_hmac_vap;
    pst_bss_mgmt    = (hmac_bss_mgmt_stru *)p_param;
    if ((OAL_PTR_NULL == pst_hmac_vap) || (OAL_PTR_NULL == pst_bss_mgmt))
    {
        OAM_ERROR_LOG0(0, OAM_SF_ROAM, "{hmac_roam_check_scan_result::vap invalid!}");
        return OAL_ERR_CODE_ROAM_INVALID_VAP;
    }

    pst_roam_info->st_static.ul_scan_result_cnt++;

    /* 如果扫描到的bss个数为0，退出 */
    if (0 == pst_bss_mgmt->ul_bss_num)
    {
        OAM_WARNING_LOG0(0, OAM_SF_ROAM, "{hmac_roam_check_scan_result::no bss scanned}");
        return OAL_ERR_CODE_ROAM_NO_VALID_BSS;
    }

    /* 获取锁 */
    oal_spin_lock(&(pst_bss_mgmt->st_lock));

    /* 遍历扫描到的bss信息，查找可以漫游的bss */
    OAL_DLIST_SEARCH_FOR_EACH(pst_entry, &(pst_bss_mgmt->st_bss_list_head))
    {
        pst_scanned_bss = OAL_DLIST_GET_ENTRY(pst_entry, hmac_scanned_bss_info, st_dlist_head);
        pst_bss_dscr    = &(pst_scanned_bss->st_bss_dscr_info);

        ul_ret = hmac_roam_alg_bss_check(pst_roam_info, pst_bss_dscr);
        if (OAL_SUCC != ul_ret)
        {
            OAM_ERROR_LOG1(0, OAM_SF_ROAM, "{hmac_roam_check_scan_result::hmac_roam_alg_bss_check fail[%d]}", ul_ret);
        }

        pst_bss_dscr = OAL_PTR_NULL;
    }

    /* 解除锁 */
    oal_spin_unlock(&(pst_bss_mgmt->st_lock));

    pst_bss_dscr = hmac_roam_alg_select_bss(pst_roam_info);
    if (OAL_PTR_NULL == pst_bss_dscr)
    {
        /* 没有扫描到可用的bss，等待定时器超时即可 */
        OAM_WARNING_LOG0(0, OAM_SF_ROAM, "{hmac_roam_check_scan_result::no bss valid}");
        return OAL_ERR_CODE_ROAM_NO_VALID_BSS;
    }

    hmac_roam_renew_privacy(pst_hmac_vap, pst_bss_dscr);

    ul_ret = hmac_check_capability_mac_phy_supplicant(&(pst_hmac_vap->st_vap_base_info), pst_bss_dscr);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ROAM, "{hmac_roam_check_scan_result::check mac and phy capability fail[%d]!}\r\n", ul_ret);
    }

    *ppst_bss_dscr_out = pst_bss_dscr;

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  hmac_roam_handle_scan_result(hmac_roam_info_stru *pst_roam_info, oal_void *p_param)
{
    mac_bss_dscr_stru       *pst_bss_dscr   = OAL_PTR_NULL;
    hmac_scan_rsp_stru       st_scan_rsp;
    mac_device_stru         *pst_mac_device;
    oal_uint32               ul_ret;

    ul_ret = hmac_roam_main_check_state(pst_roam_info, MAC_VAP_STATE_UP, ROAM_MAIN_STATE_SCANING);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ROAM, "{hmac_roam_handle_scan_result::check_state fail[%d]!}", ul_ret);
        return ul_ret;
    }

    pst_mac_device = mac_res_get_dev(pst_roam_info->pst_hmac_vap->st_vap_base_info.uc_device_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_device))
    {
        OAM_ERROR_LOG0(0, OAM_SF_ROAM, "{hmac_roam_handle_scan_result::pst_mac_device null.}");
        return OAL_ERR_CODE_ROAM_INVALID_VAP;
    }

    ul_ret = hmac_roam_check_scan_result(pst_roam_info, p_param, &pst_bss_dscr);

    if(OAL_SUCC == ul_ret)
    {
        pst_roam_info->uc_invalid_scan_cnt = 0;
        /* 扫描结果发给sme */
        OAL_MEMZERO(&st_scan_rsp, OAL_SIZEOF(st_scan_rsp));

        st_scan_rsp.uc_result_code = MAC_SCAN_SUCCESS;

        hmac_send_rsp_to_sme_sta(pst_roam_info->pst_hmac_vap, HMAC_SME_SCAN_RSP, (oal_uint8 *)&st_scan_rsp);

        /* 扫描到可用的bss，开始connect */
        return hmac_roam_main_fsm_action(pst_roam_info, ROAM_MAIN_FSM_EVENT_START_CONNECT, (oal_void *)pst_bss_dscr);
    }

    /* 如果是亮屏的，不暂停漫游 */
    if (OAL_FALSE == pst_mac_device->uc_in_suspend)
    {
        pst_roam_info->uc_invalid_scan_cnt = 0;
    }
    else
    {
        pst_roam_info->uc_invalid_scan_cnt++;
    }

    /* 多次无效扫描暂停漫游，防止在某些场景下一直唤醒HOST */
    if (pst_roam_info->uc_invalid_scan_cnt >= ROAM_INVALID_SCAN_MAX)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ROAM, "{hmac_roam_handle_scan_result::ignore_rssi_trigger after %d invalid_scan.}", pst_roam_info->uc_invalid_scan_cnt);
        pst_roam_info->uc_invalid_scan_cnt = 0;
        hmac_roam_ignore_rssi_trigger(pst_roam_info->pst_hmac_vap, OAL_TRUE);
    }
    /* 删除定时器 */
    hmac_roam_main_del_timer(pst_roam_info);
    hmac_roam_main_clear(pst_roam_info);

    return OAL_SUCC;

}


OAL_STATIC oal_uint32  hmac_roam_main_check_state(hmac_roam_info_stru *pst_roam_info,
                                                              mac_vap_state_enum_uint8 en_vap_state,
                                                              roam_main_state_enum_uint8 en_main_state)
{
    if (pst_roam_info == OAL_PTR_NULL)
    {
        return OAL_ERR_CODE_PTR_NULL;
    }

    if (pst_roam_info->pst_hmac_vap == OAL_PTR_NULL)
    {
        return OAL_ERR_CODE_ROAM_INVALID_VAP;
    }

    if (pst_roam_info->pst_hmac_user == OAL_PTR_NULL)
    {
        return OAL_ERR_CODE_ROAM_INVALID_USER;
    }

    if (0 == pst_roam_info->uc_enable)
    {
        return OAL_ERR_CODE_ROAM_DISABLED;
    }

    if ((pst_roam_info->pst_hmac_vap->st_vap_base_info.en_vap_state != en_vap_state) ||
        (pst_roam_info->en_main_state != en_main_state))
    {
        OAM_WARNING_LOG2(0, OAM_SF_ROAM, "{hmac_roam_main_check_state::unexpect vap State[%d] main_state[%d]!}",
                       pst_roam_info->pst_hmac_vap->st_vap_base_info.en_vap_state, pst_roam_info->en_main_state);
        return OAL_ERR_CODE_ROAM_INVALID_VAP_STATUS;
    }

    return OAL_SUCC;
}



OAL_STATIC oal_void  hmac_roam_main_clear(hmac_roam_info_stru *pst_roam_info)
{
    /* 清理状态 */
    hmac_roam_main_change_state(pst_roam_info, ROAM_MAIN_STATE_INIT);

    hmac_roam_connect_stop(pst_roam_info->pst_hmac_vap);
}



OAL_STATIC oal_uint32  hmac_roam_resume_pm(hmac_roam_info_stru *pst_roam_info, oal_void *p_param)
{
    oal_uint32  ul_ret = OAL_SUCC;

#ifdef _PRE_WLAN_FEATURE_STA_PM
    /* 开低功耗前关闭低功耗超时定时器 */
    if (OAL_TRUE == pst_roam_info->pst_hmac_vap->st_ps_sw_timer.en_is_registerd)
    {
        FRW_TIMER_IMMEDIATE_DESTROY_TIMER(&pst_roam_info->pst_hmac_vap->st_ps_sw_timer);
    }

    ul_ret = hmac_config_set_pm_by_module(&pst_roam_info->pst_hmac_vap->st_vap_base_info, MAC_STA_PM_CTRL_TYPE_ROAM, MAC_STA_PM_SWITCH_ON);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(pst_roam_info->pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_CFG, "{hmac_roam_resume_pm::hmac_config_set_pm_by_module failed[%d].}", ul_ret);
    }
#endif

    return ul_ret;
}


OAL_STATIC oal_uint32  hmac_roam_resume_security_port(hmac_roam_info_stru *pst_roam_info, oal_void *p_param)
{
    oal_uint32              ul_ret      = OAL_SUCC;
    mac_h2d_roam_sync_stru  st_h2d_sync = {0};

    if (pst_roam_info == OAL_PTR_NULL)
    {
        OAM_ERROR_LOG0(0, OAM_SF_ROAM, "{hmac_roam_resume_security_port::pst_roam_info null!}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    if (pst_roam_info->pst_hmac_vap == OAL_PTR_NULL)
    {
        OAM_ERROR_LOG0(0, OAM_SF_ROAM, "{hmac_roam_resume_security_port::vap null!}");
        return OAL_ERR_CODE_ROAM_INVALID_VAP;
    }

    if (pst_roam_info->pst_hmac_user == OAL_PTR_NULL)
    {
        OAM_ERROR_LOG0(0, OAM_SF_ROAM, "{hmac_roam_resume_security_port::user null!}");
        return OAL_ERR_CODE_ROAM_INVALID_USER;
    }

    /* 设置用户8021x端口合法性的状态为合法 */
    mac_user_set_port(&pst_roam_info->pst_hmac_user->st_user_base_info, OAL_TRUE);

    //填充同步信息
    st_h2d_sync.ul_back_to_old = OAL_FALSE;

    //发送同步信息
    ul_ret = hmac_config_send_event(&pst_roam_info->pst_hmac_vap->st_vap_base_info, WLAN_CFGID_ROAM_HMAC_SYNC_DMAC, OAL_SIZEOF(mac_h2d_roam_sync_stru), (oal_uint8 *)&st_h2d_sync);
    if (OAL_SUCC != ul_ret)
    {
        OAM_ERROR_LOG1(pst_roam_info->pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ROAM, "{hmac_roam_resume_security_port::send event[WLAN_CFGID_ROAM_HMAC_SYNC_DMAC] failed[%d].}", ul_ret);
    }

    return ul_ret;
}


OAL_STATIC oal_uint32  hmac_roam_scan_timeout(hmac_roam_info_stru *pst_roam_info, oal_void *p_param)
{
    oal_uint32               ul_ret;

    ul_ret = hmac_roam_main_check_state(pst_roam_info, MAC_VAP_STATE_UP, ROAM_MAIN_STATE_SCANING);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ROAM, "{hmac_roam_scan_timeout::check_state fail[%d]!}", ul_ret);
        return ul_ret;
    }

    hmac_roam_scan_complete(pst_roam_info->pst_hmac_vap);
    return OAL_SUCC;
}


OAL_STATIC oal_uint32  hmac_roam_connecting_timeout(hmac_roam_info_stru *pst_roam_info, oal_void *p_param)
{
    oal_uint32       ul_ret         = OAL_SUCC;

    ul_ret = hmac_roam_main_check_state(pst_roam_info, MAC_VAP_STATE_ROAMING, ROAM_MAIN_STATE_CONNECTING);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ROAM, "{hmac_roam_connecting_timeout::check_state fail[%d]!}", ul_ret);
        return ul_ret;
    }

    ul_ret = hmac_roam_to_old_bss(pst_roam_info, p_param);

    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ROAM,
                       "{hmac_roam_handle_fail_connect_phase:: hmac_roam_to_old_bss fail[%d]!}", ul_ret);
    }

    /* 切换vap的状态为UP，恢复用户节能，恢复发送 */
    ul_ret = hmac_fsm_call_func_sta(pst_roam_info->pst_hmac_vap, HMAC_FSM_INPUT_ROAMING_STOP, OAL_PTR_NULL);

    if (OAL_SUCC != ul_ret)
    {
        OAM_ERROR_LOG1(0, OAM_SF_ROAM,
                       "{hmac_roam_handle_fail_connect_phase:: hmac_fsm_call_func_sta fail[%d]!}", ul_ret);
    }

    hmac_roam_main_clear(pst_roam_info);

    return ul_ret;
}


OAL_STATIC oal_uint32  hmac_roam_connecting_fail(hmac_roam_info_stru *pst_roam_info, oal_void *p_param)
{
    oal_uint32               ul_ret;

    ul_ret = hmac_roam_main_check_state(pst_roam_info, MAC_VAP_STATE_ROAMING, ROAM_MAIN_STATE_CONNECTING);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ROAM, "{hmac_roam_connecting_fail::check_state fail[%d]!}", ul_ret);
        return ul_ret;
    }

    hmac_roam_main_del_timer(pst_roam_info);
    return hmac_roam_connecting_timeout(pst_roam_info, p_param);
}


OAL_STATIC oal_uint32  hmac_roam_handle_fail_handshake_phase(hmac_roam_info_stru *pst_roam_info, oal_void *p_param)
{
    oal_uint32       ul_ret;

    ul_ret = hmac_roam_main_check_state(pst_roam_info, MAC_VAP_STATE_ROAMING, ROAM_MAIN_STATE_CONNECTING);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ROAM, "{hmac_roam_handle_fail_handshake_phase::check_state fail[%d]!}", ul_ret);
        return ul_ret;
    }

    ul_ret = hmac_roam_to_old_bss(pst_roam_info, p_param);

    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(pst_roam_info->pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ROAM,
                       "{hmac_roam_handle_fail_handshake_phase:: hmac_roam_to_old_bss fail[%d]!}", ul_ret);
    }

    /* 切换vap的状态为UP，恢复用户节能，恢复发送 */
    ul_ret = hmac_fsm_call_func_sta(pst_roam_info->pst_hmac_vap, HMAC_FSM_INPUT_ROAMING_STOP, OAL_PTR_NULL);

    if (OAL_SUCC != ul_ret)
    {
        OAM_ERROR_LOG1(pst_roam_info->pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ROAM,
                       "{hmac_roam_handle_fail_handshake_phase:: hmac_fsm_call_func_sta fail[%d]!}", ul_ret);
    }

    hmac_roam_main_clear(pst_roam_info);
    hmac_roam_main_del_timer(pst_roam_info);

    /* 为提高漫游成功的概率，在握手失败时触发立即重新漫游 */
    if (pst_roam_info->st_alg.c_current_rssi > ROAM_RSSI_MAX_TYPE)
    {
        /* 如果是弱信号触发的漫游，先把rssi修改成ROAM_RSSI_LINKLOSS_TYPE来将弱信号跟LINKLOSS触发的重漫游归一 */
        if (pst_roam_info->st_alg.c_current_rssi > ROAM_RSSI_LINKLOSS_TYPE)
        {
            pst_roam_info->st_alg.c_current_rssi = ROAM_RSSI_LINKLOSS_TYPE;
        }
        /* 漫游握手失败时，rssi 逐次减1dBm，一直到到ROAM_RSSI_MAX_TYPE。这样可以最多触发5次重漫游 */
        return hmac_roam_trigger_handle(pst_roam_info->pst_hmac_vap, pst_roam_info->st_alg.c_current_rssi - 1, OAL_FALSE);
    }

    OAM_WARNING_LOG1(0, OAM_SF_ROAM, "{hmac_roam_handle_fail_handshake_phase:: report deauth to wpas! c_current_rssi=%d}", pst_roam_info->st_alg.c_current_rssi);

    /* 删除对应用户 */
    hmac_user_del(&(pst_roam_info->pst_hmac_vap->st_vap_base_info), pst_roam_info->pst_hmac_user);

    hmac_sta_handle_disassoc_rsp(pst_roam_info->pst_hmac_vap, MAC_4WAY_HANDSHAKE_TIMEOUT);

    return ul_ret;
}


OAL_STATIC oal_uint32  hmac_roam_connect_to_bss(hmac_roam_info_stru *pst_roam_info, oal_void *p_param)
{
    hmac_vap_stru           *pst_hmac_vap  = pst_roam_info->pst_hmac_vap;
    hmac_user_stru          *pst_hmac_user = pst_roam_info->pst_hmac_user;
    mac_bss_dscr_stru       *pst_bss_dscr  = (mac_bss_dscr_stru *)p_param;
    hmac_roam_old_bss_stru  *pst_old_bss;
    oal_uint32               ul_ret;
    oal_uint32               ul_need_to_stop_user = 1;
    mac_vap_stru            *pst_mac_vap;
    mac_device_stru         *pst_mac_device;
    mac_vap_stru            *pst_other_vap;
    oal_uint8                uc_vap_idx;

    ul_ret = hmac_roam_main_check_state(pst_roam_info, MAC_VAP_STATE_UP, ROAM_MAIN_STATE_SCANING);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ROAM, "{hmac_roam_connect_to_bss::check_state fail[%d]!}", ul_ret);
        return ul_ret;
    }

    pst_mac_vap = &pst_hmac_vap->st_vap_base_info;
    pst_mac_device = (mac_device_stru *)mac_res_get_dev(pst_mac_vap->uc_device_id);
    if (OAL_PTR_NULL == pst_mac_device)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ROAM, "{hmac_roam_connect_to_bss::pst_mac_device ptr is null!}\r\n");
        return -OAL_EINVAL;
    }

    /* check all vap state in case other vap is connecting now */
    for (uc_vap_idx = 0; uc_vap_idx < pst_mac_device->uc_vap_num; uc_vap_idx++)
    {
        pst_other_vap = mac_res_get_mac_vap(pst_mac_device->auc_vap_id[uc_vap_idx]);
        if (OAL_PTR_NULL == pst_other_vap)
        {
            OAM_WARNING_LOG0(pst_mac_device->auc_vap_id[uc_vap_idx], OAM_SF_ROAM, "{hmac_roam_connect_to_bss::vap is null!}");
            continue;
        }

        if (( pst_other_vap->en_vap_state >= MAC_VAP_STATE_STA_JOIN_COMP) && (pst_other_vap->en_vap_state <= MAC_VAP_STATE_STA_WAIT_ASOC))
        {
            OAM_WARNING_LOG1(pst_other_vap->uc_vap_id, OAM_SF_ROAM, "{hmac_roam_connect_to_bss::vap is connecting, state=[%d]!}",
                pst_other_vap->en_vap_state);
            hmac_roam_main_change_state(pst_roam_info, ROAM_MAIN_STATE_INIT);
            return -OAL_EFAIL;
        }
    }

#ifdef _PRE_WLAN_FEATURE_11R
    if (OAL_TRUE == (mac_mib_get_ft_trainsistion(&pst_hmac_vap->st_vap_base_info) && mac_mib_get_ft_over_ds(&pst_hmac_vap->st_vap_base_info)))
    {
        ul_need_to_stop_user = 0;
    }
#endif //_PRE_WLAN_FEATURE_11R

    if (0 != ul_need_to_stop_user)
    {
        /* 切换vap的状态为ROAMING，将用户节能，暂停发送 */
        ul_ret = hmac_fsm_call_func_sta(pst_hmac_vap, HMAC_FSM_INPUT_ROAMING_START, (oal_void *)pst_bss_dscr);
        if (OAL_SUCC != ul_ret)
        {
            OAM_WARNING_LOG1(0, OAM_SF_ROAM, "{hmac_roam_connect_to_bss::hmac_fsm_call_func_sta fail[%ld]!}", ul_ret);
            return ul_ret;
        }
    }

    /* 原bss信息保存，以便回退 */
    pst_old_bss = &pst_roam_info->st_old_bss;
    pst_old_bss->us_sta_aid = pst_hmac_vap->st_vap_base_info.us_sta_aid;
    pst_old_bss->en_protocol_mode = pst_hmac_vap->st_vap_base_info.en_protocol;
    oal_memcopy(&pst_old_bss->st_cap_info, &(pst_hmac_user->st_user_base_info.st_cap_info), OAL_SIZEOF(mac_user_cap_info_stru));
    oal_memcopy(&pst_old_bss->st_key_info, &(pst_hmac_user->st_user_base_info.st_key_info), OAL_SIZEOF(mac_key_mgmt_stru));
    oal_memcopy(&pst_old_bss->st_user_tx_info, &(pst_hmac_user->st_user_base_info.st_user_tx_info), OAL_SIZEOF(mac_user_tx_param_stru));
    oal_memcopy(&pst_old_bss->st_mib_info, pst_hmac_vap->st_vap_base_info.pst_mib_info, OAL_SIZEOF(wlan_mib_ieee802dot11_stru));
    oal_memcopy(&pst_old_bss->st_op_rates, &(pst_hmac_user->st_op_rates), OAL_SIZEOF(mac_rate_stru));
    oal_memcopy(&pst_old_bss->st_ht_hdl, &(pst_hmac_user->st_user_base_info.st_ht_hdl), OAL_SIZEOF(mac_user_ht_hdl_stru));
    oal_memcopy(&pst_old_bss->st_vht_hdl, &(pst_hmac_user->st_user_base_info.st_vht_hdl), OAL_SIZEOF(mac_vht_hdl_stru));
    pst_old_bss->en_avail_bandwidth = pst_hmac_user->st_user_base_info.en_avail_bandwidth;
    pst_old_bss->en_cur_bandwidth = pst_hmac_user->st_user_base_info.en_cur_bandwidth;
    oal_memcopy(&pst_old_bss->st_channel, &(pst_hmac_vap->st_vap_base_info.st_channel), OAL_SIZEOF(mac_channel_stru));
    oal_memcopy(&pst_old_bss->auc_bssid, &(pst_hmac_vap->st_vap_base_info.auc_bssid), WLAN_MAC_ADDR_LEN);
    pst_old_bss->us_cap_info = pst_hmac_vap->st_vap_base_info.us_assoc_user_cap_info;

    /* 切换状态至connecting */
    hmac_roam_main_change_state(pst_roam_info, ROAM_MAIN_STATE_CONNECTING);

    pst_roam_info->st_static.ul_connect_cnt++;

    /* 设置漫游到的bss能力位，重关联请求使用 */
    pst_hmac_vap->st_vap_base_info.us_assoc_user_cap_info = pst_bss_dscr->us_cap_info;

    hmac_config_set_mgmt_log(&pst_hmac_vap->st_vap_base_info, &pst_hmac_user->st_user_base_info, OAL_TRUE);

    /* 启动connect状态机 */
    ul_ret = hmac_roam_connect_start(pst_hmac_vap, pst_bss_dscr);
    if (OAL_SUCC != ul_ret)
    {
        OAM_ERROR_LOG1(0, OAM_SF_ROAM, "{hmac_roam_connect_to_bss::hmac_roam_connect_start fail[%ld]!}", ul_ret);
        hmac_roam_main_change_state(pst_roam_info, ROAM_MAIN_STATE_FAIL);
        return ul_ret;
    }

    /* 启动connect超时定时器 */
    hmac_roam_main_start_timer(pst_roam_info, ROAM_CONNECT_TIME_MAX);

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  hmac_roam_to_old_bss(hmac_roam_info_stru *pst_roam_info, oal_void *p_param)
{
    oal_uint32                  ul_ret          = OAL_SUCC;
    hmac_vap_stru               *pst_hmac_vap   = pst_roam_info->pst_hmac_vap;
    hmac_user_stru              *pst_hmac_user  = pst_roam_info->pst_hmac_user;
    hmac_roam_old_bss_stru      *pst_old_bss    = &pst_roam_info->st_old_bss;
    mac_h2d_roam_sync_stru      *pst_h2d_sync   = OAL_PTR_NULL;

    if (OAL_PTR_NULL == pst_old_bss)
    {
        OAM_ERROR_LOG0(0, OAM_SF_ROAM, "{hmac_roam_to_old_bss::pst_old_bss null}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_roam_info->st_static.ul_roam_old_cnt++;

    /* 恢复原来bss相关信息 */
    pst_hmac_vap->st_vap_base_info.us_sta_aid  = pst_old_bss->us_sta_aid;
    pst_hmac_vap->st_vap_base_info.en_protocol = pst_old_bss->en_protocol_mode;
    oal_memcopy(&(pst_hmac_user->st_user_base_info.st_cap_info), &pst_old_bss->st_cap_info, OAL_SIZEOF(mac_user_cap_info_stru));
    oal_memcopy(&(pst_hmac_user->st_user_base_info.st_key_info), &pst_old_bss->st_key_info, OAL_SIZEOF(mac_key_mgmt_stru));
    oal_memcopy(&(pst_hmac_user->st_user_base_info.st_user_tx_info),&pst_old_bss->st_user_tx_info, OAL_SIZEOF(mac_user_tx_param_stru));
    oal_memcopy(pst_hmac_vap->st_vap_base_info.pst_mib_info, &pst_old_bss->st_mib_info, OAL_SIZEOF(wlan_mib_ieee802dot11_stru));
    oal_memcopy(&(pst_hmac_user->st_op_rates), &pst_old_bss->st_op_rates, OAL_SIZEOF(mac_rate_stru));
    oal_memcopy(&(pst_hmac_user->st_user_base_info.st_ht_hdl), &pst_old_bss->st_ht_hdl, OAL_SIZEOF(mac_user_ht_hdl_stru));
    oal_memcopy(&(pst_hmac_user->st_user_base_info.st_vht_hdl), &pst_old_bss->st_vht_hdl, OAL_SIZEOF(mac_vht_hdl_stru));
    pst_hmac_user->st_user_base_info.en_avail_bandwidth = pst_old_bss->en_avail_bandwidth;
    pst_hmac_user->st_user_base_info.en_cur_bandwidth   = pst_old_bss->en_cur_bandwidth;
    oal_memcopy(&(pst_hmac_vap->st_vap_base_info.st_channel), &pst_old_bss->st_channel, OAL_SIZEOF(mac_channel_stru));
    oal_memcopy(pst_hmac_vap->st_vap_base_info.auc_bssid, pst_old_bss->auc_bssid, WLAN_MAC_ADDR_LEN);
    pst_hmac_vap->st_vap_base_info.us_assoc_user_cap_info = pst_old_bss->us_cap_info;

    /* 设置用户8021x端口合法性的状态为合法 */
    mac_user_set_port(&pst_hmac_user->st_user_base_info, OAL_TRUE);

    ul_ret = hmac_config_start_vap_event(&pst_hmac_vap->st_vap_base_info, OAL_FALSE);
    if (OAL_SUCC != ul_ret)
    {
        OAM_ERROR_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ROAM,
                       "{hmac_roam_to_old_bss::hmac_config_start_vap_event failed[%d].}", ul_ret);
        return ul_ret;
    }

    /* 相关参数需要配置到dmac */
    hmac_roam_connect_set_join_reg(&pst_hmac_vap->st_vap_base_info);

    /* 更新用户的mac地址，漫游时mac会更新 */
    oal_set_mac_addr(pst_hmac_user->st_user_base_info.auc_user_mac_addr, pst_hmac_vap->st_vap_base_info.auc_bssid);

    ul_ret = hmac_config_user_info_syn(&(pst_hmac_vap->st_vap_base_info), &(pst_hmac_user->st_user_base_info));
    if (OAL_SUCC != ul_ret)
    {
        OAM_ERROR_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ROAM,
                       "{hmac_roam_to_old_bss::hmac_syn_vap_state failed[%d].}", ul_ret);
    }

    /* 回退 bss 时，hmac 2 dmac 同步的相关信息，以便失败的时候回退 */
    pst_h2d_sync = OAL_MEM_ALLOC(OAL_MEM_POOL_ID_LOCAL, OAL_SIZEOF(mac_h2d_roam_sync_stru), OAL_TRUE);
    if (OAL_PTR_NULL == pst_h2d_sync)
    {
        OAM_ERROR_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ROAM, "{hmac_roam_to_old_bss::no buff to alloc sync info!}");
        return OAL_ERR_CODE_ALLOC_MEM_FAIL;
    }

    //填充同步信息
    pst_h2d_sync->ul_back_to_old = OAL_TRUE;
    pst_h2d_sync->us_sta_aid = pst_old_bss->us_sta_aid;
    oal_memcopy(&(pst_h2d_sync->st_channel), &pst_old_bss->st_channel, OAL_SIZEOF(mac_channel_stru));
    oal_memcopy(&(pst_h2d_sync->st_cap_info), &pst_old_bss->st_cap_info, OAL_SIZEOF(mac_user_cap_info_stru));
    oal_memcopy(&(pst_h2d_sync->st_key_info), &pst_old_bss->st_key_info, OAL_SIZEOF(mac_key_mgmt_stru));
    oal_memcopy(&(pst_h2d_sync->st_user_tx_info),&pst_old_bss->st_user_tx_info, OAL_SIZEOF(mac_user_tx_param_stru));

    /* 在漫游过程中可能又建立了聚合，因此回退时需要删除掉 */
    hmac_tid_clear(&pst_hmac_vap->st_vap_base_info, pst_hmac_user);

    //发送同步信息
    ul_ret = hmac_config_send_event(&pst_hmac_vap->st_vap_base_info, WLAN_CFGID_ROAM_HMAC_SYNC_DMAC, OAL_SIZEOF(mac_h2d_roam_sync_stru), (oal_uint8 *)pst_h2d_sync);
    if (OAL_SUCC != ul_ret)
    {
        OAM_ERROR_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ROAM, "{hmac_roam_to_old_bss::send event[WLAN_CFGID_ROAM_HMAC_SYNC_DMAC] failed[%d].}", ul_ret);
    }

    /* 释放同步数据 */
    if(OAL_PTR_NULL != pst_h2d_sync)
    {
        OAL_MEM_FREE(pst_h2d_sync, OAL_TRUE);
    }

    /* user已经关联上，抛事件给DMAC，在DMAC层挂用户算法钩子 */
    hmac_user_add_notify_alg(&pst_hmac_vap->st_vap_base_info, pst_hmac_user->st_user_base_info.us_assoc_id);
    hmac_config_set_mgmt_log(&pst_hmac_vap->st_vap_base_info, &pst_hmac_user->st_user_base_info, OAL_FALSE);

    OAM_WARNING_LOG4(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ROAM,
                  "{hmac_roam_to_old_bss::now resuming to [%02X:XX:XX:%02X:%02X:%02X]}",
                  pst_old_bss->auc_bssid[0],pst_old_bss->auc_bssid[3],pst_old_bss->auc_bssid[4],pst_old_bss->auc_bssid[5]);

    return ul_ret;
}


OAL_STATIC oal_uint32  hmac_roam_to_new_bss(hmac_roam_info_stru *pst_roam_info, oal_void *p_param)
{
    hmac_vap_stru           *pst_hmac_vap  = pst_roam_info->pst_hmac_vap;
    hmac_user_stru          *pst_hmac_user = pst_roam_info->pst_hmac_user;
    oal_uint32               ul_ret;
    wlan_roam_main_band_state_enum_uint8  uc_roam_band_state;

    ul_ret = hmac_roam_main_check_state(pst_roam_info, MAC_VAP_STATE_ROAMING, ROAM_MAIN_STATE_CONNECTING);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ROAM, "{hmac_roam_to_new_bss::check_state fail[%d]!}", ul_ret);
        return ul_ret;
    }

    pst_roam_info->st_static.ul_roam_new_cnt++;
    hmac_roam_alg_add_history(pst_roam_info, pst_hmac_vap->st_vap_base_info.auc_bssid);

    hmac_roam_main_change_state(pst_roam_info, ROAM_MAIN_STATE_UP);

    hmac_roam_main_del_timer(pst_roam_info);

    /* 切换vap的状态为UP，恢复用户节能，恢复发送 */
    ul_ret = hmac_fsm_call_func_sta(pst_hmac_vap, HMAC_FSM_INPUT_ROAMING_STOP, OAL_PTR_NULL);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ROAM, "{hmac_roam_to_new_bss::hmac_fsm_call_func_sta fail! erro code is %u}", ul_ret);
    }
    hmac_config_set_mgmt_log(&pst_hmac_vap->st_vap_base_info, &pst_hmac_user->st_user_base_info, OAL_FALSE);

    OAM_WARNING_LOG4(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ROAM,
                  "{hmac_roam_to_new_bss::roam to [%02X:XX:XX:%02X:%02X:%02X] succ}",
                  pst_hmac_vap->st_vap_base_info.auc_bssid[0],pst_hmac_vap->st_vap_base_info.auc_bssid[3],
                  pst_hmac_vap->st_vap_base_info.auc_bssid[4],pst_hmac_vap->st_vap_base_info.auc_bssid[5]);
    hmac_roam_main_clear(pst_roam_info);

    /* 标识漫游类型，传到dmac做对应业务处理 */
    if(WLAN_BAND_2G == pst_roam_info->st_old_bss.st_channel.en_band)
    {
        if(WLAN_BAND_2G == pst_hmac_vap->st_vap_base_info.st_channel.en_band)
        {
            uc_roam_band_state = WLAN_ROAM_MAIN_BAND_STATE_2TO2;
        }
        else
        {
            uc_roam_band_state = WLAN_ROAM_MAIN_BAND_STATE_2TO5;
        }
    }
    else
    {
        if(WLAN_BAND_2G == pst_hmac_vap->st_vap_base_info.st_channel.en_band)
        {
            uc_roam_band_state = WLAN_ROAM_MAIN_BAND_STATE_5TO2;
        }
        else
        {
            uc_roam_band_state = WLAN_ROAM_MAIN_BAND_STATE_5TO5;
        }
    }

    /* 漫游成功之后，同步到dmac给相关业务做处理 */
    ul_ret = hmac_config_send_event(&pst_hmac_vap->st_vap_base_info, WLAN_CFGID_ROAM_SUCC_H2D_SYNC, OAL_SIZEOF(oal_uint8), (oal_uint8 *)&uc_roam_band_state);
    if (OAL_SUCC != ul_ret)
    {
        OAM_ERROR_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ROAM, "{hmac_roam_to_new_bss::send event[WLAN_CFGID_ROAM_SUCC_H2D_SYNC] failed[%d].}", ul_ret);
    }

    return ul_ret;

}

#if 0

OAL_STATIC oal_uint32  hmac_roam_to_new_bss_fail(hmac_roam_info_stru *pst_roam_info, oal_void *p_param)
{
    /* 漫游失败先关闭漫游主定时器 */
    hmac_roam_main_del_timer(pst_roam_info);

    return hmac_roam_to_old_bss(pst_roam_info, p_param);
}


OAL_STATIC oal_uint32  hmac_roam_stop(hmac_roam_info_stru *pst_roam_info, oal_void *p_param)
{
    hmac_vap_stru           *pst_hmac_vap   = pst_roam_info->pst_hmac_vap;
    oal_uint32               ul_reason_code = 0;
    oal_uint32               ul_ret;

    if (pst_hmac_vap == OAL_PTR_NULL)
    {
        OAM_ERROR_LOG0(0, OAM_SF_ROAM, "{hmac_roam_stop::vap null!}");
        return OAL_ERR_CODE_ROAM_INVALID_VAP;
    }
    if (OAL_PTR_NULL != p_param)
    {
        ul_reason_code = *(oal_uint32 *)p_param;
    }

    OAM_WARNING_LOG2(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ROAM,
                  "{hmac_roam_stop::current state[%d] reacon code[%d]}", pst_roam_info->en_main_state, ul_reason_code);

    hmac_roam_main_change_state(pst_roam_info, ROAM_MAIN_STATE_FAIL);

    /* 切换vap的状态为UP，恢复用户节能，恢复发送 */
    ul_ret = hmac_fsm_call_func_sta(pst_hmac_vap, HMAC_FSM_INPUT_ROAMING_STOP, OAL_PTR_NULL);
    if (OAL_SUCC != ul_ret)
    {
        OAM_ERROR_LOG2(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ROAM,
                       "{hmac_roam_stop::hmac_fsm_call_func_sta fail[%d]!} reason code[%d]", ul_ret, ul_reason_code);
        return ul_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  hmac_roam_flush_buf(struct work_struct *pst_work)
{
    hmac_roam_buf_stru                *pst_roam_buf;
    hmac_roam_info_stru               *pst_roam_info;
    oal_netbuf_stru                   *pst_skb;
    oal_uint32                         ul_ret;
    hmac_vap_stru                     *pst_hmac_vap;

    pst_roam_buf  = container_of(pst_work, struct hmac_roam_buf_stru, st_work);
    pst_roam_info = container_of(pst_roam_buf, struct hmac_roam_info_stru, st_buf);

    if (OAL_PTR_NULL == pst_roam_info)
    {
        OAM_ERROR_LOG0(0, OAM_SF_ROAM, "{hmac_roam_flush_buf::roam_info null!}");
        return OAL_ERR_CODE_ROAM_INVALID_VAP;
    }

    if (OAL_PTR_NULL == pst_roam_buf->pst_wq)
    {
        OAM_ERROR_LOG0(0, OAM_SF_ROAM, "{hmac_roam_flush_buf::pst_wq null!}");
        return OAL_ERR_CODE_ROAM_INVALID_VAP;
    }

    pst_hmac_vap = pst_roam_info->pst_hmac_vap;

    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_ERROR_LOG0(0, OAM_SF_ROAM, "{hmac_roam_flush_buf::hmac_vap null!}");
        return OAL_ERR_CODE_ROAM_INVALID_VAP;
    }

    while(!skb_queue_empty(&pst_roam_buf->st_data_queue))
    {
        pst_skb = skb_dequeue(&pst_roam_buf->st_data_queue);

        ul_ret  = hmac_tx_lan_to_wlan(&(pst_hmac_vap->st_vap_base_info), pst_skb);
        if (SUCC != ul_ret)
        {
            OAM_ERROR_LOG1(0, OAM_SF_ROAM, "{hmac_roam_flush_buf::hmac_tx_lan_to_wlan fail[%d]!}", ul_ret);
            oal_netbuf_free(pst_skb);
        }
    }
}


oal_uint32  hmac_roam_pause_user(hmac_vap_stru *pst_hmac_vap, oal_void *p_param)
{
    hmac_roam_info_stru     *pst_roam_info;
    oal_uint8                uc_vap_id;

    if (pst_hmac_vap == OAL_PTR_NULL)
    {
        OAM_ERROR_LOG0(0, OAM_SF_ROAM, "{hmac_roam_pause_user::vap null!}");
        return OAL_ERR_CODE_ROAM_INVALID_VAP;
    }

    uc_vap_id = pst_hmac_vap->st_vap_base_info.uc_vap_id;

    pst_roam_info = (hmac_roam_info_stru *)pst_hmac_vap->pul_roam_info;
    if (pst_roam_info == OAL_PTR_NULL)
    {
        OAM_ERROR_LOG0(uc_vap_id, OAM_SF_ROAM, "{hmac_roam_pause_user::roam info null!}");
        return OAL_ERR_CODE_ROAM_INVALID_VAP;
    }

    /* 漫游开关没有开时，不暂停用户 */
    if (0 == pst_roam_info->uc_enable)
    {
        OAM_ERROR_LOG0(uc_vap_id, OAM_SF_ROAM, "{hmac_roam_pause_user::roam disabled!}");
        return OAL_ERR_CODE_ROAM_DISABLED;
    }

    if (OAL_PTR_NULL != pst_roam_info->st_buf.pst_wq)
    {
        OAM_ERROR_LOG0(uc_vap_id, OAM_SF_ROAM, "{hmac_roam_pause_user::buf exist!}");
        return OAL_ERR_CODE_ROAM_DISABLED;
    }

    /* 初始化缓存队列 */
    skb_queue_head_init(&(pst_roam_info->st_buf.st_data_queue));
    INIT_WORK(&(pst_roam_info->st_buf.st_work), hmac_roam_flush_buf);
    pst_roam_info->st_buf.pst_wq = create_singlethread_workqueue("hi110x_roam");
    return OAL_SUCC;
}


oal_uint32  hmac_roam_resume_user(hmac_vap_stru *pst_hmac_vap, oal_void *p_param)
{
    hmac_roam_info_stru     *pst_roam_info;
    oal_uint8                uc_vap_id;

    if (pst_hmac_vap == OAL_PTR_NULL)
    {
        OAM_ERROR_LOG0(0, OAM_SF_ROAM, "{hmac_roam_pause_user::vap null!}");
        return OAL_ERR_CODE_ROAM_INVALID_VAP;
    }

    uc_vap_id = pst_hmac_vap->st_vap_base_info.uc_vap_id;

    pst_roam_info = (hmac_roam_info_stru *)pst_hmac_vap->pul_roam_info;
    if (pst_roam_info == OAL_PTR_NULL)
    {
        OAM_ERROR_LOG0(uc_vap_id, OAM_SF_ROAM, "{hmac_roam_pause_user::roam info null!}");
        return OAL_ERR_CODE_ROAM_INVALID_VAP;
    }

    if (OAL_PTR_NULL == pst_roam_info->st_buf.pst_wq)
    {
        OAM_ERROR_LOG0(uc_vap_id, OAM_SF_ROAM, "{hmac_roam_pause_user::buf exist!}");
        return OAL_ERR_CODE_ROAM_DISABLED;
    }

    /* 清空缓存队列 */
    skb_queue_purge(&(pst_roam_info->st_buf.st_data_queue));
    destroy_workqueue(pst_roam_info->st_buf.pst_wq);
    pst_roam_info->st_buf.pst_wq = OAL_PTR_NULL;
    return OAL_SUCC;
}
#endif


oal_uint32  hmac_roam_ignore_rssi_trigger(hmac_vap_stru *pst_hmac_vap, oal_bool_enum_uint8 en_val)
{
    hmac_roam_info_stru              *pst_roam_info;
    oal_uint32                        ul_ret;
    oal_uint8                         uc_vap_id;
    mac_roam_trigger_stru             st_roam_trigger;

    if (pst_hmac_vap == OAL_PTR_NULL)
    {
        OAM_ERROR_LOG0(0, OAM_SF_ROAM, "{hmac_roam_stop_roam_trigger::vap null!}");
        return OAL_ERR_CODE_ROAM_INVALID_VAP;
    }

    uc_vap_id = pst_hmac_vap->st_vap_base_info.uc_vap_id;

    pst_roam_info = (hmac_roam_info_stru *)pst_hmac_vap->pul_roam_info;
    if (OAL_PTR_NULL == pst_roam_info)
    {
        OAM_ERROR_LOG0(uc_vap_id, OAM_SF_ROAM, "{hmac_roam_stop_roam_trigger::roam info null!}");
        return OAL_ERR_CODE_ROAM_INVALID_VAP;
    }

    if (pst_roam_info->uc_rssi_ignore == en_val)
    {
        return OAL_SUCC;
    }

    if (OAL_TRUE == en_val)
    {
        st_roam_trigger.c_trigger_2G = ROAM_RSSI_LINKLOSS_TYPE;
        st_roam_trigger.c_trigger_5G = ROAM_RSSI_LINKLOSS_TYPE;
    }
    else
    {
        st_roam_trigger.c_trigger_2G = pst_roam_info->st_config.c_trigger_rssi_2G;
        st_roam_trigger.c_trigger_5G = pst_roam_info->st_config.c_trigger_rssi_5G;
    }
    ul_ret = hmac_config_send_event(&pst_hmac_vap->st_vap_base_info,
                                    WLAN_CFGID_SET_ROAM_TRIGGER,
                                    OAL_SIZEOF(mac_roam_trigger_stru),
                                    (oal_uint8 *)&st_roam_trigger);
    if (OAL_SUCC != ul_ret)
    {
        OAM_ERROR_LOG2(uc_vap_id, OAM_SF_CFG, "{hmac_roam_stop_roam_trigger::send event[%d] failed[%d].}", en_val, ul_ret);
        return ul_ret;
    }
    pst_roam_info->uc_rssi_ignore = en_val;

    OAM_WARNING_LOG1(uc_vap_id, OAM_SF_ROAM, "{hmac_roam_stop_roam_trigger::[%d] OK!}", en_val);
    return OAL_SUCC;
}


oal_uint32  hmac_roam_pause_user(hmac_vap_stru *pst_hmac_vap, oal_void *p_param)
{
    hmac_roam_info_stru              *pst_roam_info;
    oal_net_device_stru              *pst_net_device;
    oal_uint32                        ul_ret;
    oal_uint8                         uc_vap_id;
    oal_uint8                         uc_roaming_mode;

    if (pst_hmac_vap == OAL_PTR_NULL)
    {
        OAM_ERROR_LOG0(0, OAM_SF_ROAM, "{hmac_roam_pause_user::vap null!}");
        return OAL_ERR_CODE_ROAM_INVALID_VAP;
    }

    uc_vap_id = pst_hmac_vap->st_vap_base_info.uc_vap_id;

    pst_roam_info = (hmac_roam_info_stru *)pst_hmac_vap->pul_roam_info;
    if (OAL_PTR_NULL == pst_roam_info)
    {
        OAM_ERROR_LOG0(uc_vap_id, OAM_SF_ROAM, "{hmac_roam_pause_user::roam info null!}");
        return OAL_ERR_CODE_ROAM_INVALID_VAP;
    }

    /* 漫游开关没有开时，不暂停用户 */
    if (0 == pst_roam_info->uc_enable)
    {
        OAM_ERROR_LOG0(uc_vap_id, OAM_SF_ROAM, "{hmac_roam_pause_user::roam disabled!}");
        return OAL_ERR_CODE_ROAM_DISABLED;
    }

    pst_net_device = pst_hmac_vap->pst_net_device;
    if (OAL_PTR_NULL == pst_net_device)
    {
        OAM_ERROR_LOG0(uc_vap_id, OAM_SF_ROAM, "{hmac_roam_pause_user::net_device null!}");
        return OAL_ERR_CODE_ROAM_INVALID_VAP;
    }

    /* 必须保证vap的状态是UP */
    if (MAC_VAP_STATE_UP != pst_hmac_vap->st_vap_base_info.en_vap_state)
    {
        OAM_WARNING_LOG1(uc_vap_id, OAM_SF_ROAM,
                       "{hmac_roam_pause_user::vap state = [%d] NOT FOR ROAMING!}",
                       pst_hmac_vap->st_vap_base_info.en_vap_state);
        return OAL_ERR_CODE_ROAM_STATE_UNEXPECT;
    }

    /* 暂停所有协议层数据，这样就不需要再hmac搞一个缓存队列了 */
    //oal_net_stop_subqueue(pst_net_device, 0);
    oal_net_tx_stop_all_queues(pst_net_device);
    oal_net_wake_subqueue(pst_net_device, WLAN_HI_QUEUE);

    /* 清空 HMAC层TID信息 */
    hmac_tid_clear(&pst_hmac_vap->st_vap_base_info, pst_roam_info->pst_hmac_user);

#ifdef _PRE_WLAN_FEATURE_STA_PM
    ul_ret = hmac_config_set_pm_by_module(&pst_hmac_vap->st_vap_base_info, MAC_STA_PM_CTRL_TYPE_ROAM, MAC_STA_PM_SWITCH_OFF);
    if (OAL_SUCC != ul_ret)
    {
        OAM_ERROR_LOG1(uc_vap_id, OAM_SF_CFG, "{hmac_roam_pause_user::hmac_config_set_pm_by_module failed[%d].}", ul_ret);
        oal_net_tx_wake_all_queues(pst_net_device);
        return ul_ret;
    }
#endif

    uc_roaming_mode = 1;
    ul_ret = hmac_config_send_event(&pst_hmac_vap->st_vap_base_info, WLAN_CFGID_SET_ROAMING_MODE, OAL_SIZEOF(oal_uint8), (oal_uint8 *)&uc_roaming_mode);
    if (OAL_SUCC != ul_ret)
    {
        OAM_ERROR_LOG1(uc_vap_id, OAM_SF_CFG, "{hmac_roam_pause_user::send event[WLAN_CFGID_SET_ROAMING_MODE] failed[%d].}", ul_ret);
        oal_net_tx_wake_all_queues(pst_net_device);
        return ul_ret;
    }

#ifdef  _PRE_WLAN_FEATURE_VOWIFI
    if (OAL_PTR_NULL != pst_hmac_vap->st_vap_base_info.pst_vowifi_cfg_param)
    {
        if (VOWIFI_LOW_THRES_REPORT == pst_hmac_vap->st_vap_base_info.pst_vowifi_cfg_param->en_vowifi_mode)
        {
            /* 针对漫游和去关联场景,切换vowifi语音状态 */
            hmac_config_vowifi_report((&pst_hmac_vap->st_vap_base_info), 0, OAL_PTR_NULL);
        }
    }
#endif /* _PRE_WLAN_FEATURE_VOWIFI */
    hmac_fsm_change_state(pst_hmac_vap, MAC_VAP_STATE_ROAMING);

    OAM_WARNING_LOG0(uc_vap_id, OAM_SF_ROAM, "{hmac_roam_pause_user::queues stopped!}");
    return OAL_SUCC;
}


oal_uint32  hmac_roam_resume_user(hmac_vap_stru *pst_hmac_vap, oal_void *p_param)
{
    hmac_roam_info_stru              *pst_roam_info;
    oal_net_device_stru              *pst_net_device;
    oal_uint32                        ul_ret;
    oal_uint8                         uc_vap_id;
    oal_uint8                         uc_roaming_mode;

    if (pst_hmac_vap == OAL_PTR_NULL)
    {
        OAM_ERROR_LOG0(0, OAM_SF_ROAM, "{hmac_roam_resume_user::vap null!}");
        return OAL_ERR_CODE_ROAM_INVALID_VAP;
    }

    uc_vap_id = pst_hmac_vap->st_vap_base_info.uc_vap_id;

    pst_roam_info = (hmac_roam_info_stru *)pst_hmac_vap->pul_roam_info;
    if (pst_roam_info == OAL_PTR_NULL)
    {
        OAM_ERROR_LOG0(uc_vap_id, OAM_SF_ROAM, "{hmac_roam_resume_user::roam info null!}");
        return OAL_ERR_CODE_ROAM_INVALID_VAP;
    }

    pst_net_device = pst_hmac_vap->pst_net_device;
    if (OAL_PTR_NULL == pst_net_device)
    {
        OAM_ERROR_LOG0(uc_vap_id, OAM_SF_ROAM, "{hmac_roam_resume_user::net_device null!}");
        return OAL_ERR_CODE_ROAM_INVALID_VAP;
    }

    /* 必须保证vap的状态是roaming */
    if (MAC_VAP_STATE_ROAMING != pst_hmac_vap->st_vap_base_info.en_vap_state)
    {
        hmac_roam_resume_pm(pst_roam_info, OAL_PTR_NULL);
        hmac_roam_resume_security_port(pst_roam_info, OAL_PTR_NULL);
        oal_net_tx_wake_all_queues(pst_net_device);
        OAM_WARNING_LOG1(uc_vap_id, OAM_SF_ROAM,
                       "{hmac_roam_resume_user::vap state[%d] NOT ROAMING!}", pst_hmac_vap->st_vap_base_info.en_vap_state);
        return OAL_SUCC;
    }

    uc_roaming_mode = 0;
    ul_ret = hmac_config_send_event(&pst_hmac_vap->st_vap_base_info, WLAN_CFGID_SET_ROAMING_MODE, OAL_SIZEOF(oal_uint8), (oal_uint8 *)&uc_roaming_mode);
    if (OAL_SUCC != ul_ret)
    {
        OAM_ERROR_LOG1(uc_vap_id, OAM_SF_CFG, "{hmac_roam_resume_user::send event[WLAN_CFGID_SET_ROAMING_MODE] failed[%d].}", ul_ret);
    }

    hmac_fsm_change_state(pst_hmac_vap, MAC_VAP_STATE_UP);

    hmac_roam_resume_pm(pst_roam_info, OAL_PTR_NULL);

    hmac_roam_resume_security_port(pst_roam_info, OAL_PTR_NULL);

    oal_net_tx_wake_all_queues(pst_net_device);

    OAM_WARNING_LOG0(uc_vap_id, OAM_SF_ROAM, "{hmac_roam_resume_user::all_queues awake!}");

    return OAL_SUCC;
}


oal_uint32 hmac_roam_scan_complete(hmac_vap_stru *pst_hmac_vap)
{
    hmac_roam_info_stru              *pst_roam_info;
    hmac_device_stru                 *pst_hmac_device;
    oal_uint8                         uc_vap_id;
    hmac_bss_mgmt_stru               *pst_scan_bss_mgmt;

    if (pst_hmac_vap == OAL_PTR_NULL)
    {
        OAM_ERROR_LOG0(0, OAM_SF_ROAM, "{hmac_roam_scan_complete::vap null!}");
        return OAL_ERR_CODE_ROAM_INVALID_VAP;
    }

    uc_vap_id = pst_hmac_vap->st_vap_base_info.uc_vap_id;

    pst_roam_info = (hmac_roam_info_stru *)pst_hmac_vap->pul_roam_info;
    if (pst_roam_info == OAL_PTR_NULL)
    {
        return OAL_ERR_CODE_ROAM_INVALID_VAP;
    }

    /* 漫游开关没有开时，不处理扫描结果 */
    if (0 == pst_roam_info->uc_enable)
    {
        return OAL_ERR_CODE_ROAM_DISABLED;
    }

    /* 获取hmac device */
    pst_hmac_device = hmac_res_get_mac_dev(pst_hmac_vap->st_vap_base_info.uc_device_id);
    if (OAL_PTR_NULL == pst_hmac_device)
    {
        OAM_ERROR_LOG0(0, OAM_SF_ROAM, "{hmac_roam_scan_complete::device null!}");
        return OAL_ERR_CODE_MAC_DEVICE_NULL;
    }

    pst_scan_bss_mgmt = &(pst_hmac_device->st_scan_mgmt.st_scan_record_mgmt.st_bss_mgmt);

    OAM_INFO_LOG0(uc_vap_id, OAM_SF_ROAM, "{hmac_roam_scan_complete::handling scan result!}");
    return hmac_roam_main_fsm_action(pst_roam_info, ROAM_MAIN_FSM_EVENT_SCAN_RESULT, (void *)pst_scan_bss_mgmt);
}


oal_void  hmac_roam_tbtt_handle(hmac_vap_stru *pst_hmac_vap)
{
    //hmac_roam_connect_rx_tbtt(pst_hmac_vap);
}


oal_uint32 hmac_roam_trigger_handle(hmac_vap_stru *pst_hmac_vap, oal_int8 c_rssi, oal_bool_enum_uint8 en_current_bss_ignore)
{
    hmac_roam_info_stru     *pst_roam_info;
    oal_uint32               ul_ret;

    if (pst_hmac_vap == OAL_PTR_NULL)
    {
        OAM_ERROR_LOG0(0, OAM_SF_ROAM, "{hmac_roam_trigger_handle::pst_hmac_vap null!}");
        return OAL_ERR_CODE_ROAM_INVALID_VAP;
    }

    pst_roam_info = (hmac_roam_info_stru *)pst_hmac_vap->pul_roam_info;
    if(OAL_PTR_NULL == pst_roam_info)
    {
        OAM_WARNING_LOG0(0, OAM_SF_ROAM, "{hmac_roam_trigger_handle::pst_hmac_vap->pul_roam_info is NULL,Return!}");
        return OAL_ERR_CODE_ROAM_EVENT_UXEXPECT;
    }

    /* 每次漫游前，刷新是否支持漫游到自己的参数 */
    pst_roam_info->en_current_bss_ignore = en_current_bss_ignore;

    ul_ret = hmac_roam_main_check_state(pst_roam_info, MAC_VAP_STATE_UP, ROAM_MAIN_STATE_INIT);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ROAM, "{hmac_roam_trigger_handle::check_state fail[%d]!}", ul_ret);
        return ul_ret;
    }

    hmac_roam_alg_init(pst_roam_info, c_rssi);

    ul_ret = hmac_roam_main_fsm_action(pst_roam_info, ROAM_MAIN_FSM_EVENT_START, OAL_PTR_NULL);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ROAM, "{hmac_roam_trigger_handle::START fail[%d].}", ul_ret);
        return ul_ret;
    }
    /*lint -e571*/
    OAM_WARNING_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ROAM, "{hmac_roam_trigger_handle::RSSI[%d].}", c_rssi);
    /*lint +e571*/
    return OAL_SUCC;
}


oal_uint32  hmac_sta_roam_rx_mgmt(hmac_vap_stru *pst_hmac_vap, oal_void *p_param)
{
    dmac_wlan_crx_event_stru *pst_crx_event;

    pst_crx_event = (dmac_wlan_crx_event_stru *)p_param;

    hmac_roam_connect_rx_mgmt(pst_hmac_vap, pst_crx_event);
    return OAL_SUCC;
}

oal_void  hmac_roam_add_key_done(hmac_vap_stru *pst_hmac_vap)
{
    hmac_roam_connect_key_done(pst_hmac_vap);
}


oal_void  hmac_roam_wpas_connect_state_notify(hmac_vap_stru *pst_hmac_vap, wpas_connect_state_enum_uint32 conn_state)
{
    hmac_roam_info_stru              *pst_roam_info;
    oal_uint32                        ul_ret;

    if (pst_hmac_vap == OAL_PTR_NULL)
    {
        OAM_ERROR_LOG0(0, OAM_SF_ROAM, "{hmac_roam_wpas_connect_state_notify::vap null!}");
        return;
    }

    if (!IS_LEGACY_VAP(&pst_hmac_vap->st_vap_base_info))
    {
        return;
    }

    if(WLAN_VAP_MODE_BSS_STA != pst_hmac_vap->st_vap_base_info.en_vap_mode)
    {
        return;
    }

    pst_roam_info = (hmac_roam_info_stru *)pst_hmac_vap->pul_roam_info;
    if (pst_roam_info == OAL_PTR_NULL)
    {
        return;
    }

    if (pst_roam_info->ul_connected_state == conn_state)
    {
        return;
    }

    OAM_WARNING_LOG2(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ROAM,
                    "{hmac_roam_wpas_connect_state_notify:: state changed: [%d]-> [%d]}",
                    pst_roam_info->ul_connected_state, conn_state);

    pst_roam_info->ul_connected_state = conn_state;
    if (WPAS_CONNECT_STATE_IPADDR_OBTAINED == conn_state)
    {
        pst_roam_info->ul_ip_addr_obtained = OAL_TRUE;
    }
    if (WPAS_CONNECT_STATE_IPADDR_REMOVED == conn_state)
    {
        pst_roam_info->ul_ip_addr_obtained = OAL_FALSE;
    }

    ul_ret = hmac_config_send_event(&pst_hmac_vap->st_vap_base_info,
                                    WLAN_CFGID_ROAM_NOTIFY_STATE,
                                    OAL_SIZEOF(oal_uint32),
                                    (oal_uint8 *)&pst_roam_info->ul_ip_addr_obtained);
    if (OAL_SUCC != ul_ret)
    {
        OAM_ERROR_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_CFG,
                       "{hmac_roam_wpas_connect_state_notify::send event failed[%d].}", ul_ret);
        return;
    }

}

#ifdef _PRE_WLAN_FEATURE_11R

oal_uint32 hmac_roam_reassoc(hmac_vap_stru *pst_hmac_vap)
{
    hmac_roam_info_stru              *pst_roam_info;
    oal_uint32                        ul_ret;
    oal_uint8                         uc_vap_id;
    mac_bss_dscr_stru                *pst_bss_dscr;

    if (pst_hmac_vap == OAL_PTR_NULL)
    {
        OAM_ERROR_LOG0(0, OAM_SF_ROAM, "{hmac_roam_reassoc::vap null!}");
        return OAL_ERR_CODE_ROAM_INVALID_VAP;
    }

    uc_vap_id = pst_hmac_vap->st_vap_base_info.uc_vap_id;

    pst_roam_info = (hmac_roam_info_stru *)pst_hmac_vap->pul_roam_info;
    if (pst_roam_info == OAL_PTR_NULL)
    {
        OAM_ERROR_LOG0(uc_vap_id, OAM_SF_ROAM, "{hmac_roam_reassoc::roam info null!}");
        return OAL_ERR_CODE_ROAM_INVALID_VAP;
    }

    /* 漫游开关没有开时，不处理 */
    if (0 == pst_roam_info->uc_enable)
    {
        OAM_ERROR_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ROAM, "{hmac_roam_reassoc::roam disabled.}");
        return OAL_ERR_CODE_ROAM_DISABLED;
    }

    /* 主状态机为非CONNECTING状态，失败 */
    if (pst_roam_info->en_main_state != ROAM_MAIN_STATE_CONNECTING)
    {
        OAM_WARNING_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ROAM,
                       "{hmac_roam_reassoc::roam_en_main_state=[%d] not ROAM_MAIN_STATE_CONNECTING, return.}",
                       pst_roam_info->en_main_state);
        return OAL_ERR_CODE_ROAM_STATE_UNEXPECT;
    }

    /* CONNECT状态机为非WAIT_JOIN状态，失败 */
    if (pst_roam_info->st_connect.en_state != ROAM_CONNECT_STATE_WAIT_ASSOC_COMP)
    {
        OAM_ERROR_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ROAM,
                       "{hmac_roam_reassoc::connect state[%d] error.}", pst_roam_info->st_connect.en_state);
        return OAL_ERR_CODE_ROAM_STATE_UNEXPECT;
    }

    if (OAL_TRUE == (mac_mib_get_ft_trainsistion(&pst_hmac_vap->st_vap_base_info) && mac_mib_get_ft_over_ds(&pst_hmac_vap->st_vap_base_info)))
    {
        pst_bss_dscr = pst_roam_info->st_connect.pst_bss_dscr;
        if (OAL_PTR_NULL == pst_bss_dscr)
        {
            OAM_ERROR_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ROAM,
                       "{hmac_roam_reassoc::pst_bss_dscr is null.}");
            return OAL_ERR_CODE_ROAM_NO_VALID_BSS;
        }

        ul_ret = hmac_fsm_call_func_sta(pst_hmac_vap, HMAC_FSM_INPUT_ROAMING_START, (oal_void *)pst_bss_dscr);
        if (OAL_SUCC != ul_ret)
        {
            OAM_WARNING_LOG1(0, OAM_SF_ROAM, "{hmac_roam_reassoc::hmac_fsm_call_func_sta fail[%ld]!}", ul_ret);
            return ul_ret;
        }

        ul_ret = hmac_roam_connect_ft_reassoc(pst_hmac_vap);
        if (OAL_SUCC != ul_ret)
        {
            OAM_WARNING_LOG1(uc_vap_id, OAM_SF_ROAM,
                           "{hmac_roam_reassoc::hmac_roam_connect_process_ft FAIL[%d]!}", ul_ret);
            return ul_ret;
        }
    }

    return OAL_SUCC;
}

oal_uint32 hmac_roam_rx_ft_action(hmac_vap_stru *pst_hmac_vap, oal_netbuf_stru *pst_netbuf)
{
    dmac_wlan_crx_event_stru st_crx_event;

    OAL_MEMZERO(&st_crx_event, OAL_SIZEOF(dmac_wlan_crx_event_stru));
    st_crx_event.pst_netbuf = pst_netbuf;

    hmac_roam_connect_rx_mgmt(pst_hmac_vap, &st_crx_event);
    return OAL_SUCC;
}

#endif //_PRE_WLAN_FEATURE_11R

oal_void hmac_roam_connect_complete(hmac_vap_stru *pst_hmac_vap, oal_uint32 ul_result)
{
    hmac_roam_info_stru     *pst_roam_info;

    if (pst_hmac_vap == OAL_PTR_NULL)
    {
        OAM_ERROR_LOG0(0, OAM_SF_ROAM, "{hmac_roam_connect_complete::vap null!}");
        return;
    }

    pst_roam_info = (hmac_roam_info_stru *)pst_hmac_vap->pul_roam_info;
    if (pst_roam_info == OAL_PTR_NULL)
    {
        OAM_ERROR_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ROAM, "{hmac_roam_connect_complete::pst_roam_info null!}");
        return;
    }

    /* 漫游开关没有开时，不处理扫描结果 */
    if (0 == pst_roam_info->uc_enable)
    {
        OAM_ERROR_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ROAM, "{hmac_roam_connect_complete::roam disabled!}");
        return;
    }

    if (ul_result == OAL_SUCC)
    {
        hmac_roam_main_fsm_action(pst_roam_info, ROAM_MAIN_FSM_EVENT_CONNECT_SUCC, OAL_PTR_NULL);
    }
    else if(ul_result == OAL_ERR_CODE_ROAM_HANDSHAKE_FAIL)
    {
        hmac_roam_main_fsm_action(pst_roam_info, ROAM_MAIN_FSM_EVENT_HANDSHAKE_FAIL, OAL_PTR_NULL);
    }
    else
    {
        hmac_roam_main_fsm_action(pst_roam_info, ROAM_MAIN_FSM_EVENT_CONNECT_FAIL, OAL_PTR_NULL);
    }
}

#if 0

oal_void hmac_roam_rx_complete(hmac_vap_stru *pst_hmac_vap, oal_uint32 ul_reslut)
{
    hmac_roam_info_stru     *pst_roam_info;

    if (pst_hmac_vap == OAL_PTR_NULL)
    {
        OAM_ERROR_LOG0(0, OAM_SF_ROAM, "{hmac_roam_connect_complete::vap null!}");
        return;
    }

    pst_roam_info = (hmac_roam_info_stru *)pst_hmac_vap->pul_roam_info;
    if (pst_roam_info == OAL_PTR_NULL)
    {
        OAM_ERROR_LOG0(0, OAM_SF_ROAM, "{hmac_roam_connect_complete::pst_roam_info null!}");
        return;
    }

    /* 漫游开关没有开时，不处理扫描结果 */
    if (0 == pst_roam_info->uc_enable)
    {
        OAM_ERROR_LOG0(0, OAM_SF_ROAM, "{hmac_roam_connect_complete::roam disabled!}");
        return;
    }

    if (ul_reslut == OAL_SUCC)
    {
        hmac_roam_main_fsm_action(pst_roam_info, ROAM_MAIN_FSM_EVENT_CONNECT_SUCC, OAL_PTR_NULL);
    }
    else
    {
        hmac_roam_main_fsm_action(pst_roam_info, ROAM_MAIN_FSM_EVENT_CONNECT_FAIL, OAL_PTR_NULL);
    }
}

#endif

oal_void hmac_roam_timeout_test(hmac_vap_stru *pst_hmac_vap)
{
    hmac_roam_connect_timeout(pst_hmac_vap->pul_roam_info);
}

#endif //_PRE_WLAN_FEATURE_ROAM

#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

