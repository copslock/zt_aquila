/*
 *  Copyright (c) 2008 Atheros Communications Inc.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <ieee80211_var.h>
#include <ieee80211_channel.h>
#include <ieee80211_mlme.h>
#include <ieee80211_target.h>
#include <ieee80211_rateset.h>
#include <ieee80211_wds.h>

/*Begin:added by duanmingzhe for 80211 cache*/
extern void ieee80211_vap_cache_attach(struct ieee80211vap *vap); //Added by duanmingzhe for 80211 cache
/*End:added by duanmingzhe for 80211 cache*/

int
ieee80211_vap_setup(struct ieee80211com *ic, struct ieee80211vap *vap,
                    int opmode, int scan_priority_base, int flags,
                    const u_int8_t bssid[IEEE80211_ADDR_LEN])
{
#define IEEE80211_C_OPMODE                                              \
    (IEEE80211_C_IBSS | IEEE80211_C_HOSTAP | IEEE80211_C_AHDEMO |       \
     IEEE80211_C_MONITOR)
 
    int i;

    vap->iv_ic = ic;
    vap->iv_create_flags=flags;
    vap->iv_flags = ic->ic_flags;       /* propagate common flags */
    vap->iv_flags_ext = ic->ic_flags_ext;
    vap->iv_ath_cap = ic->ic_ath_cap;
    /* Default Multicast traffic to lowest rate of 1000 Kbps*/
    vap->iv_mcast_fixedrate = 0;
    vap->iv_caps = ic->ic_caps &~ IEEE80211_C_OPMODE;
    vap->iv_ath_cap &= ~IEEE80211_ATHC_WME;
    vap->iv_node_count = 0;
    vap->iv_ccmpsw_seldec = 1;
    atomic_set(&vap->iv_rx_gate,0);

    for (i = 0; i < IEEE80211_APPIE_MAX_FRAMES; i++) {
        LIST_INIT(&vap->iv_app_ie_list[i]);
    }

    /*
     * By default, supports sending our Country IE and 802.11h
     * informations (but also depends on the lower ic flags).
     */
    ieee80211_vap_country_ie_set(vap);
    /* yanggs modify for disble 11h by default */

	/*zhaoyang1 transplant from 717*/
    /* ieee80211_vap_doth_set(vap);*/
    ieee80211_vap_doth_clear(vap);
    /* yanggs modify end */
	/*zhaoyang1 transplant end*/
	
    switch (opmode) {
    case IEEE80211_M_STA:
        /* turn on sw bmiss timer by default */
        ieee80211_vap_sw_bmiss_set(vap);
        break;
    case IEEE80211_M_IBSS:
        vap->iv_caps |= IEEE80211_C_IBSS;
        vap->iv_ath_cap &= ~IEEE80211_ATHC_XR;
        break;
    case IEEE80211_M_AHDEMO:
        vap->iv_caps |= IEEE80211_C_AHDEMO;
        vap->iv_ath_cap &= ~IEEE80211_ATHC_XR;
        break;
    case IEEE80211_M_HOSTAP:
        vap->iv_caps |= IEEE80211_C_HOSTAP;
        vap->iv_ath_cap &= ~(IEEE80211_ATHC_XR | IEEE80211_ATHC_TURBOP);
        break;
    case IEEE80211_M_MONITOR:
        vap->iv_caps |= IEEE80211_C_MONITOR;
        vap->iv_ath_cap &= ~(IEEE80211_ATHC_XR | IEEE80211_ATHC_TURBOP);
        break;
    case IEEE80211_M_WDS:
        vap->iv_caps |= IEEE80211_C_WDS;
        vap->iv_ath_cap &= ~(IEEE80211_ATHC_XR | IEEE80211_ATHC_TURBOP);
        IEEE80211_VAP_WDS_ENABLE(vap);
        break;
    }
    vap->iv_opmode = opmode;
#ifdef ATH_SUPPORT_TxBF
    vap->iv_txbfmode = 1;
#endif
#if ATH_SUPPORT_WPA_SUPPLICANT_CHECK_TIME
	vap->iv_rejoint_attemp_time = 20;
#endif
    vap->iv_scan_priority_base = scan_priority_base;

    vap->iv_chanchange_count = 0;
    vap->channel_change_done = 0;
    #ifdef ATH_SUPPORT_QUICK_KICKOUT
    vap->iv_sko_th = ATH_TX_MAX_CONSECUTIVE_XRETRIES;
    #endif
    
    spin_lock_init(&vap->iv_lock);

    vap->iv_bsschan = ic->ic_curchan; /* initialize bss chan to cur chan */
    /*
     * Enable various functionality by default if we're capable.
     */
        
    /* NB: bg scanning only makes sense for station mode right now */
    if ((ic->ic_opmode == IEEE80211_M_STA) &&
        (vap->iv_caps & IEEE80211_C_BGSCAN))
        vap->iv_flags |= IEEE80211_F_BGSCAN;

    vap->iv_dtim_period = IEEE80211_DTIM_DEFAULT;
    vap->iv_bmiss_count_for_reset = IEEE80211_DEFAULT_BMISS_COUNT_RESET;
    vap->iv_bmiss_count_max = IEEE80211_DEFAULT_BMISS_COUNT_MAX;

    vap->iv_des_mode = IEEE80211_MODE_AUTO;
    vap->iv_cur_mode = IEEE80211_MODE_AUTO;
    vap->iv_des_modecaps = (1 << IEEE80211_MODE_AUTO);
    vap->iv_des_chan[IEEE80211_MODE_AUTO]           = IEEE80211_CHAN_ANYC;
    vap->iv_des_chan[IEEE80211_MODE_11A]            = ieee80211_find_dot11_channel(ic, 0, IEEE80211_MODE_11A);
    vap->iv_des_chan[IEEE80211_MODE_11B]            = ieee80211_find_dot11_channel(ic, 0, IEEE80211_MODE_11B);
    vap->iv_des_chan[IEEE80211_MODE_11G]            = ieee80211_find_dot11_channel(ic, 0, IEEE80211_MODE_11G);
    vap->iv_des_chan[IEEE80211_MODE_11NA_HT20]      = ieee80211_find_dot11_channel(ic, 0, IEEE80211_MODE_11NA_HT20);
    vap->iv_des_chan[IEEE80211_MODE_11NG_HT20]      = ieee80211_find_dot11_channel(ic, 0, IEEE80211_MODE_11NG_HT20);
    vap->iv_des_chan[IEEE80211_MODE_11NA_HT40PLUS]  = ieee80211_find_dot11_channel(ic, 0, IEEE80211_MODE_11NA_HT40PLUS);
    vap->iv_des_chan[IEEE80211_MODE_11NA_HT40MINUS] = ieee80211_find_dot11_channel(ic, 0, IEEE80211_MODE_11NA_HT40MINUS);
    vap->iv_des_chan[IEEE80211_MODE_11NG_HT40PLUS]  = ieee80211_find_dot11_channel(ic, 0, IEEE80211_MODE_11NG_HT40PLUS);
    vap->iv_des_chan[IEEE80211_MODE_11NG_HT40MINUS] = ieee80211_find_dot11_channel(ic, 0, IEEE80211_MODE_11NG_HT40MINUS);

    /* apply registry settings */
    vap->iv_des_ibss_chan     = ic->ic_reg_parm.defaultIbssChannel;
    vap->iv_rateCtrlEnable    = ic->ic_reg_parm.rateCtrlEnable;

    vap->iv_fixed_rateset     = ic->ic_reg_parm.transmitRateSet;
    vap->iv_fixed_retryset    = ic->ic_reg_parm.transmitRetrySet;
    vap->iv_max_aid = IEEE80211_AID_DEF; /* default max aid(should be a config param ?) */
    vap->iv_keep_alive_timeout  =  IEEE80211_DEFULT_KEEP_ALIVE_TIMEOUT;       /* keep alive time out */
    vap->iv_inact_count =  (vap->iv_keep_alive_timeout + IEEE80211_INACT_WAIT -1)/IEEE80211_INACT_WAIT;

#ifdef ATH_SUPPORT_TxBF
    vap->iv_autocvupdate      = ic->ic_reg_parm.autocvupdate;
    vap->iv_cvupdateper       = ic->ic_reg_parm.cvupdateper;
#endif

    /* set WMM-related parameters */
    vap->iv_wmm_enable        = ic->ic_reg_parm.wmeEnabled;
    if (ieee80211com_has_cap(ic, IEEE80211_C_WME)) {
        if (vap->iv_wmm_enable) {
            ieee80211_vap_wme_set(vap);
        } else  {
            ieee80211_vap_wme_clear(vap);
        }
    } else {
        ieee80211_vap_wme_clear(vap);
    }

    vap->iv_wmm_power_save    = 0;
    vap->iv_smps_rssithresh   = ic->ic_reg_parm.smpsRssiThreshold;
    vap->iv_smps_datathresh   = ic->ic_reg_parm.smpsDataThreshold;

    {
        u_int8_t    uapsd_flag;
        
        uapsd_flag = (ic->ic_reg_parm.uapsd.vo ? WME_CAPINFO_UAPSD_VO : 0) |
                     (ic->ic_reg_parm.uapsd.vi ? WME_CAPINFO_UAPSD_VI : 0) |
                     (ic->ic_reg_parm.uapsd.bk ? WME_CAPINFO_UAPSD_BK : 0) |
                     (ic->ic_reg_parm.uapsd.be ? WME_CAPINFO_UAPSD_BE : 0);

        ieee80211_set_uapsd_flags(vap, uapsd_flag);
    }

    IEEE80211_ADDR_COPY(vap->iv_myaddr, ic->ic_myaddr);
    IEEE80211_ADDR_COPY(vap->iv_my_hwaddr, ic->ic_my_hwaddr);

    for(i=0;i<IEEE80211_MAX_VAP_EVENT_HANDLERS; ++i) {                         
        vap->iv_event_handler[i] = NULL;
    }

    vap->iv_tsf_offset.offset = 0;
    vap->iv_tsf_offset.offset_negative = false;

    /*by defualt wep key cache will not be allocated in first four slots */
    vap->iv_wep_keycache = 0; 
    
    /* attach other modules */
    ieee80211_rateset_vattach(vap);
    ieee80211_proto_vattach(vap);
    ieee80211_node_vattach(vap);   
    ieee80211_crypto_vattach(vap);
    ieee80211_vap_pause_vattach(ic,vap); 
    ieee80211_power_vattach(vap,
                            1,  /* enable full sleep */
                            ic->ic_reg_parm.sleepTimePwrSaveMax,
                            ic->ic_reg_parm.sleepTimePwrSave,
                            ic->ic_reg_parm.sleepTimePerf,
                            ic->ic_reg_parm.inactivityTimePwrSaveMax,
                            ic->ic_reg_parm.inactivityTimePwrSave,
                            ic->ic_reg_parm.inactivityTimePerf,
                            ic->ic_reg_parm.smpsDynamic,
                            ic->ic_reg_parm.psPollEnabled);
    ieee80211_mlme_vattach(vap);
    ieee80211_scan_table_vattach(vap, &(vap->iv_scan_table), ic->ic_osdev);
    ieee80211_aplist_vattach(&(vap->iv_candidate_aplist),
                             vap,
                             ieee80211_vap_get_scan_table(vap),
                             vap->iv_ic->ic_scanner,
                             vap->iv_ic->ic_osdev);
    ieee80211_aplist_config_vattach(&(vap->iv_aplist_config), vap->iv_ic->ic_osdev);
    ieee80211_acl_attach(vap);

    ieee80211_scs_vattach(vap);

    return 1;
#undef  IEEE80211_C_OPMODE
}

/*zhaoyang1 transplant from 717*/
/*Begin:Added by duanmingzhe for traffic limit*/
static void vap_traffic_limit_timer_fn(unsigned long data)
{
	struct ieee80211vap *vap = (struct ieee80211vap *) data;

	u_long current_tick = jiffies;
	
	if (vap->vap_traffic_limit)
	{
		if (vap->vap_traffic_tick == -1)
			vap->vap_traffic_tick = current_tick;
		
		if (time_after(current_tick, (vap->vap_traffic_tick + HZ)))
		{
			vap->vap_traffic_tick += HZ / 5;
			
			if (atomic_read(&(vap->vap_packet_number)) >= vap->vap_packet_max_number / 5)
			{
				atomic_sub((int)(vap->vap_packet_max_number / 5), &(vap->vap_packet_number));
			}
			else
			{
				atomic_set(&(vap->vap_packet_number), 0);
			}

			/*ljy--modified begin to separate traffic limit between rx and tx*/
			#if 0
			if (atomic_read(&(vap->vap_packet_number_send)) >= vap->vap_packet_max_number / 5)
			{
				atomic_sub((int)(vap->vap_packet_max_number / 5), &(vap->vap_packet_number_send));
			}
			#endif
			if (atomic_read(&(vap->vap_packet_number_send)) >= vap->vap_packet_max_number_send / 5)
			{
				atomic_sub((int)(vap->vap_packet_max_number_send / 5), &(vap->vap_packet_number_send));
			}
			/*ljy--modified end*/
			else
			{
				atomic_set(&(vap->vap_packet_number_send), 0);
			}
		}
	}

	mod_timer(&(vap->traffic_limit_timer), current_tick + HZ / 5);
}

static int vap_setup_timer(struct ieee80211vap *vap)
{

	/*init traffic_limit-timer*/
	init_timer(&(vap->traffic_limit_timer));

	vap->traffic_limit_timer.function = vap_traffic_limit_timer_fn;
	vap->traffic_limit_timer.data = (unsigned long) vap;
	vap->traffic_limit_timer.expires = jiffies + HZ / 5;
	/* init the max packet number*/
	vap->vap_packet_max_number = 102400000;
	vap->every_node_packet_max_number = 102400000;

	/*ljy--add begin to separate traffic limit between rx and tx*/
	vap->vap_packet_max_number_send = 102400000;
	vap->every_node_packet_max_number_send = 102400000;
	/*ljy--add end*/

	/*ljy--add begin to adjust policy of traffic limit*/
	vap->vap_traffic_tick = -1;
	atomic_set(&(vap->vap_packet_number), 0);
	atomic_set(&(vap->vap_packet_number_send), 0);
	/*ljy--add end*/
	
	/*init traffic limit flags*/
	vap->vap_traffic_limit = 0;
	vap->every_node_traffic_limit = 0;
	add_timer(&(vap->traffic_limit_timer));
	return 0;

}
/*End:Added by duanmingzhe for traffic limit*/
/*suzhaoyu add for customer online-traffic limit*/
static OS_TIMER_FUNC(ieee80211_online_traffic_check)
{
	struct ieee80211vap *vap;
	OS_GET_TIMER_ARG(vap, struct ieee80211vap *);
	if(vap->lowest_traffic_limit_switch==0)
		return;
	ieee80211_iterate_nt(ieee80211_iterate_nt_fn,vap);
 }
/*suzhaoyu addend*/
/*zhaoyang1 transplant end*/

int
ieee80211_vap_attach(struct ieee80211vap *vap)
{
    int ret;

    IEEE80211_ADD_VAP_TARGET(vap);

    /*
     * XXX: It looks like we always need a bss node around
     * for transmit before association (e.g., probe request
     * in scan operation). When we actually join a BSS, we'll
     * create a new node and free the old one.
     */
    ret = ieee80211_node_latevattach(vap);

    if (ret == 0) {
        IEEE80211_UPDATE_TARGET_IC(vap->iv_bss);
    }

    /*
     * If IQUE is NOT enabled at compiling, ieee80211_me_attach attaches
     * the empty op table to vap->iv_me_ops;
     * If IQUE is enabled, the initialization is done by the following 
     * function, and the op table is correctly attached.
     */
    ieee80211_ique_attach(vap);
    ieee80211_nawds_attach(vap);

    vap->iv_vap_ath_info_handle = ieee80211_vap_ath_info_attach(vap);
	/*zhaoyang1 transplant from 717*/
    /*Begin:Added by duanmingzhe for traffic limit*/
    vap_setup_timer(vap);
    /*End:Added by duanmingzhe for traffic limit*/
    /*Begin:Added by duanmingzhe for auto-transmit*/
    vap->iv_transmit_switch = 0;
    vap->iv_transmit_rate = 300000;
    /*End:Added by duanmingzhe for auto-transmit*/

	/*pengruofeng--add begin to develop the policy of mac binding 2011-5-31*/
	vap->vap_dhcp_enable = 0;
	vap->vap_ip_auto_learning = 0;
	vap->vap_pppoe_enable = 0;
	/*pengruofeng --add end 2011-5-31*/

	/*pengruofeng--add begin for user isolation 2011-5-31*/
	vap->iv_switch = 0;
	/*pengruofeng--add end 2011-5-31*/

	/*pengruofeng add for spoofing attack of data frame 2011-5-31*/
	vap->iv_seq_detect_num = 5;
	/*pengruofeng -- add end 2011-5-31*/
	/*zhaoyang1 transplant end*/

    return ret;
}

void
ieee80211_vap_detach(struct ieee80211vap *vap)
{
    int i;

    ieee80211_vap_ath_info_detach(vap->iv_vap_ath_info_handle);
    ieee80211_node_latevdetach(vap);
    ieee80211_proto_vdetach(vap);
    ieee80211_power_vdetach(vap);
    ieee80211_mlme_vdetach(vap);
    ieee80211_scs_vdetach(vap);

	/*zhaoyang1 transplant from 717*/
    /*Begin:Added by duanmingzhe for traffic limit*/
    del_timer(&(vap->traffic_limit_timer));
    /*End:Added by duanmingzhe for traffic limit*/
	/*zhaoyang1 transplant end*/
	
    /*
     * detach ique features/functions
     */
    if (vap->iv_ique_ops.me_detach) {
        vap->iv_ique_ops.me_detach(vap);
    }
    if (vap->iv_ique_ops.hbr_detach) {
        vap->iv_ique_ops.hbr_detach(vap);
    }
    ieee80211_aplist_vdetach(&(vap->iv_candidate_aplist));
    ieee80211_aplist_config_vdetach(&(vap->iv_aplist_config));
    ieee80211_resmgr_vdetach(vap->iv_ic->ic_resmgr, vap);
    ieee80211_acl_detach(vap);
    ieee80211_scan_table_vdetach(&(vap->iv_scan_table));
    ieee80211_acl_detach(vap);

    for (i = 0; i < IEEE80211_WEP_NKID; i++) {
        ieee80211_crypto_freekey(vap, &vap->iv_nw_keys[i]);
    }

    spin_lock_destroy(&vap->iv_lock);
}

int
ieee80211_vap_register_events(struct ieee80211vap *vap, wlan_event_handler_table *evtab)
{
    vap->iv_evtable = evtab;
    return 0;
}

int
ieee80211_vap_register_mlme_events(struct ieee80211vap *vap, os_handle_t oshandle, wlan_mlme_event_handler_table *evtab)
{
    int i;
    /* unregister if there exists one already */
    ieee80211_vap_unregister_mlme_events(vap,oshandle,evtab);
    IEEE80211_VAP_LOCK(vap);
    for (i=0;i<IEEE80211_MAX_VAP_MLME_EVENT_HANDLERS; ++i) {
        if ( vap->iv_mlme_evtable[i] == NULL) {
            vap->iv_mlme_evtable[i] = evtab;
            vap->iv_mlme_arg[i] = oshandle;
            IEEE80211_VAP_UNLOCK(vap);
            return 0;
        }
    }
    IEEE80211_VAP_UNLOCK(vap);
    IEEE80211_DPRINTF(vap, IEEE80211_MSG_MLME, "%s mlme evtable is full.\n", __func__);
    return ENOMEM;
}

int
ieee80211_vap_unregister_mlme_events(struct ieee80211vap *vap,os_handle_t oshandle, wlan_mlme_event_handler_table *evtab)
{
    int i;
    IEEE80211_VAP_LOCK(vap);
    for (i=0;i<IEEE80211_MAX_VAP_MLME_EVENT_HANDLERS; ++i) {
        if ( vap->iv_mlme_evtable[i] == evtab && vap->iv_misc_arg[i] == oshandle) {
            vap->iv_mlme_evtable[i] = NULL;
            vap->iv_mlme_arg[i] = NULL;
            IEEE80211_VAP_UNLOCK(vap);
            return 0;
        }
    }
    IEEE80211_VAP_UNLOCK(vap);
    IEEE80211_DPRINTF(vap, IEEE80211_MSG_MLME, "%s The handler is not in the evtable.\n", __func__);
    return EEXIST;
}

int
ieee80211_vap_register_misc_events(struct ieee80211vap *vap, os_handle_t oshandle, wlan_misc_event_handler_table *evtab)
{
    int i;
    /* unregister if there exists one already */
    ieee80211_vap_unregister_misc_events(vap,oshandle,evtab);
    IEEE80211_VAP_LOCK(vap);
    for (i=0;i<IEEE80211_MAX_MISC_EVENT_HANDLERS; ++i) {
        if ( vap->iv_misc_evtable[i] == NULL) {
            vap->iv_misc_evtable[i] = evtab;
            vap->iv_misc_arg[i] = oshandle;
            IEEE80211_VAP_UNLOCK(vap);
            return 0;
        }
    }
    IEEE80211_VAP_UNLOCK(vap);
    return ENOMEM;
}

int
ieee80211_vap_unregister_misc_events(struct ieee80211vap *vap,os_handle_t oshandle, wlan_misc_event_handler_table *evtab)
{
    int i;
    IEEE80211_VAP_LOCK(vap);
    for (i=0;i<IEEE80211_MAX_MISC_EVENT_HANDLERS; ++i) {
        if ( vap->iv_misc_evtable[i] == evtab && vap->iv_misc_arg[i] == oshandle) {
            vap->iv_misc_evtable[i] = NULL;
            vap->iv_misc_arg[i] = NULL;
            IEEE80211_VAP_UNLOCK(vap);
            return 0;
        }
    }
    IEEE80211_VAP_UNLOCK(vap);
    return EEXIST;
}

int ieee80211_vap_register_ccx_events(struct ieee80211vap *vap, os_if_t osif, wlan_ccx_handler_table *evtab)
{
    vap->iv_ccx_arg = osif;
    vap->iv_ccx_evtable = evtab;
    return 0;
}

int
ieee80211_vap_match_ssid(struct ieee80211vap *vap, const u_int8_t *ssid, u_int8_t ssidlen)
{
    int i;

    for (i = 0; i < vap->iv_des_nssid; i++) {
        if ((vap->iv_des_ssid[i].len == ssidlen) &&
            (OS_MEMCMP(vap->iv_des_ssid[i].ssid, ssid, ssidlen) == 0)) {
            /* find a matching entry */
            return 1;
        }
    }

    return 0;
}

/*
 * free the vap and deliver event.
 */
void ieee80211_vap_free(struct ieee80211vap *vap)
{
    struct ieee80211com *ic  = vap->iv_ic;
    os_if_t             osif = vap->iv_ifp;

    ic->ic_vap_delete(vap);
    IEEE80211COM_DELIVER_VAP_EVENT(ic, osif, IEEE80211_VAP_DELETED);
}

/**
 * @register a vap event handler.
 * ARGS :
 *  ieee80211_vap_event_handler : vap event handler
 *  arg                         : argument passed back via the evnt handler
 * RETURNS:
 *  on success returns 0.
 *  on failure returns a negative value.
 * allows more than one event handler to be registered.
 */
int ieee80211_vap_unregister_event_handler(ieee80211_vap_t vap,ieee80211_vap_event_handler evhandler, void *arg)
{
    int i;
    IEEE80211_VAP_LOCK(vap);
    for (i=0;i<IEEE80211_MAX_VAP_EVENT_HANDLERS; ++i) {
        if ( vap->iv_event_handler[i] == evhandler &&  vap->iv_event_handler_arg[i] == arg ) {
            vap->iv_event_handler[i] = NULL;
            vap->iv_event_handler_arg[i] = NULL;
            IEEE80211_VAP_UNLOCK(vap);
            return 0;
        }
    }
    IEEE80211_VAP_UNLOCK(vap);
    return EEXIST;

}

/**
 * @unregister a vap event handler.
 * ARGS :
 *  ieee80211_vap_event_handler : vap event handler
 *  arg                         : argument passed back via the evnt handler
 * RETURNS:
 *  on success returns 0.
 *  on failure returns a negative value.
 */
int ieee80211_vap_register_event_handler(ieee80211_vap_t vap,ieee80211_vap_event_handler evhandler, void *arg)
{
    int i;
    /* unregister if there exists one already */
    ieee80211_vap_unregister_event_handler(vap,evhandler,arg);

    IEEE80211_VAP_LOCK(vap);
    for (i=0;i<IEEE80211_MAX_VAP_EVENT_HANDLERS; ++i) {
        if ( vap->iv_event_handler[i] == NULL) {
            vap->iv_event_handler[i] = evhandler;
            vap->iv_event_handler_arg[i] = arg;
            IEEE80211_VAP_UNLOCK(vap);
            return 0;
        }
    }
    IEEE80211_VAP_UNLOCK(vap);
    return ENOMEM;
}

void ieee80211_vap_deliver_event(struct ieee80211vap *vap, ieee80211_vap_event *event)
{
    int i;                                                                 
    void *arg;                                                                                                                          
    ieee80211_vap_event_handler evhandler;
    IEEE80211_VAP_LOCK(vap);
    for(i=0;i<IEEE80211_MAX_VAP_EVENT_HANDLERS; ++i) {                         
        if (vap->iv_event_handler[i]) {                                   
            evhandler =  vap->iv_event_handler[i];                                
            arg = vap->iv_event_handler_arg[i];               
            IEEE80211_VAP_UNLOCK(vap);
            (* evhandler) (vap, event,arg);               
            IEEE80211_VAP_LOCK(vap);
        }                                                                 
    }                                                                     
    IEEE80211_VAP_UNLOCK(vap);
}

/**
 * @ get opmode
 * ARGS:
 *  vap    : handle to vap.
 * RETURNS: returns opmode of the vap.
 */
enum ieee80211_opmode ieee80211_vap_get_opmode(ieee80211_vap_t vap)
{
    return  vap->iv_opmode;
}

const char* ieee80211_opmode2string( enum ieee80211_opmode opmode)
{
    switch ( opmode )
    {
    case IEEE80211_M_STA:
         return "IEEE80211_M_STA";
    case IEEE80211_M_IBSS:    
     return "IEEE80211_M_IBSS";
    case IEEE80211_M_AHDEMO:  
     return "IEEE80211_M_AHDEMO";
    case IEEE80211_M_HOSTAP:
         return "IEEE80211_M_HOSTAP";
    case IEEE80211_M_MONITOR: 
     return "IEEE80211_M_MONITOR";
    case IEEE80211_M_WDS:     
     return "IEEE80211_M_WDS";
    case IEEE80211_M_BTAMP:  
     return "IEEE80211_M_BTAMP";
    case IEEE80211_M_ANY:     
     return "IEEE80211_M_ANY";

    default:
     return "Unknown ieee80211_opmode"; 
    }
};


/*
 * External UMAC Interface
 */
wlan_if_t
wlan_vap_create(wlan_dev_t            devhandle,
                enum ieee80211_opmode opmode,
                int                   scan_priority_base,
                int                   flags,
                u_int8_t              *bssid)
{
#if ATH_SUPPORT_FLOWMAC_MODULE
#define FLOWMAC_FLOWCONTROL_VAP 1
#define FLOWMAC_FLOWCONTROL_ETH 2
#endif
    struct ieee80211com *ic = devhandle;
    struct ieee80211vap *vap;

    IEEE80211_DPRINTF_IC(ic, IEEE80211_VERBOSE_LOUD, IEEE80211_MSG_DEBUG,
                         "%s : enter. devhandle=0x%x, opmode=%s, flags=0x%x\n", 
             __func__, 
             devhandle,
             ieee80211_opmode2string(opmode),
             flags
             );

    vap = ic->ic_vap_create(ic, opmode, scan_priority_base, flags, bssid);
    if (vap == NULL) {
        printk("%s: failed to create a vap object\n", __func__);
        return NULL;
    }

    ieee80211_vap_pause_late_vattach(ic,vap); 
    ieee80211_resmgr_vattach(ic->ic_resmgr, vap);

    ieee80211_vap_deleted_clear(vap); /* clear the deleted */

    /* when all  done,  add vap to queue */
    IEEE80211_COMM_LOCK(ic);
    TAILQ_INSERT_TAIL(&ic->ic_vaps, vap, iv_next);
    IEEE80211_COMM_UNLOCK(ic);

    IEEE80211_DPRINTF_IC(ic, IEEE80211_VERBOSE_LOUD, IEEE80211_MSG_DEBUG,
             "%s : exit. devhandle=0x%x, opmode=%s, flags=0x%x.\n",
             __func__, 
             devhandle,
             ieee80211_opmode2string(opmode),
             flags
             );
	/* TODO FIXME     
	*  To check the ath layer flow control status in softc and the create     
	*  enable or disable the flow control status in vap      
	*/
#if ATH_SUPPORT_FLOWMAC_MODULE    
	vap->iv_flowmac = FLOWMAC_FLOWCONTROL_VAP | FLOWMAC_FLOWCONTROL_ETH;
#endif   
    ieee80211_vap_cache_attach(vap); //added by duanmingzhe for 80211 cache
	/*zhaoyang1 transplant from 717*/
    /*Begin:Added by duanmingzhe for user isolation*/
    vap->iv_switch = 0;
    /*End:Added by duanmingzhe for user isolation*/
    /*Begin:Add by duanmingzhe for develop the policy of mac binding*/
    vap->vap_dhcp_enable = 0;
    vap->vap_ip_auto_learning = 0;
    vap->vap_pppoe_enable = 0;
    /*End:Add by duanmingzhe for develop the policy of mac binding*/
	/*zhaoyang add for noisefloor switch when channel 0*/
	vap->noisefloor_enable = 1;
	/*zhaoyang add end*/
	/*zhaoyang modify for add rd_trap switch*/
#if ATH_SUPPORT_WAPI
	vap->rd_trap = 0; 
#endif
	/*zhaoyang modify end*/
    /*suzhaoyu add for customer online-traffic limit*/
    vap->lowest_traffic_limit_switch = 0; 
    vap->lowest_traffic_limit_threshold = 0;
    vap->lowest_traffic_limit_timelength =5;
    OS_INIT_TIMER(ic->ic_osdev, &(vap->online_traffic_timer), ieee80211_online_traffic_check, (void *) (vap));
    /*suzhaoyu addend*/
	/*zhaoyang1 transplant end*/
    /*<begin : transplant by caizhibang  from apv5*/
    /*yanggs add for thinap wds*/
    vap->vap_wds = 0;
    /*yanggs add end*/
    /*end : transplant by caizhibang from apv5 >*/
    return vap;
}

int
wlan_vap_delete(wlan_if_t vaphandle)
{
    struct ieee80211vap *vap = vaphandle;
    struct ieee80211com *ic = vap->iv_ic;

    IEEE80211_DPRINTF_IC(ic, IEEE80211_VERBOSE_LOUD,IEEE80211_MSG_DEBUG,
                         "%s : enter. vaphandle=0x%x\n", 
             __func__, 
             vaphandle
             );

    ieee80211_vap_pause_vdetach(ic,vap);

    IEEE80211_COMM_LOCK(ic);
#ifdef MAGPIE_HIF_GMAC
    if (ic->ic_chanchange_cnt)
        ic->ic_chanchange_cnt -= ic->ic_chanchange_tbtt;
#endif    
    if (ieee80211_vap_deleted_is_clear(vap)) /* if not deleted then it is on the list */
    {
        TAILQ_REMOVE(&ic->ic_vaps, vap, iv_next);
        if (TAILQ_EMPTY(&ic->ic_vaps))      /* reset to supported mode */
            ic->ic_opmode = IEEE80211_M_STA;
        ieee80211_vap_deleted_set(vap); /* mark it as deleted */
        IEEE80211_COMM_UNLOCK(ic);

        /*
         * In case iv_bss was not stopped or is in scanning.
         * TBD: BSS should have been stopped now. We can save the time for stop bss again.
         */
        wlan_mlme_stop_bss(vap,
                           WLAN_MLME_STOP_BSS_F_SEND_DEAUTH        | 
                           WLAN_MLME_STOP_BSS_F_CLEAR_ASSOC_STATE  |
                           WLAN_MLME_STOP_BSS_F_WAIT_RX_DONE       |
                           WLAN_MLME_STOP_BSS_F_NO_RESET);

		/*zhaoyang1 transplant from 717*/
		/*suzhaoyu add for customer online-traffic limit*/
		OS_FREE_TIMER(&vap->online_traffic_timer);
		/*suzhaoyu addend*/
		/*zhaoyang1 transplant end*/
        ieee80211_sta_leave(vap->iv_bss);
        ieee80211_node_vdetach(vap);
    } else {
        IEEE80211_COMM_UNLOCK(ic);
    }

    IEEE80211_DPRINTF_IC(ic, IEEE80211_VERBOSE_LOUD,IEEE80211_MSG_DEBUG,
                         "%s : exit. vaphandle=0x%x\n", 
             __func__, 
             vaphandle
             );
    return 0;
}

int wlan_vap_allocate_mac_addr(wlan_dev_t devhandle, u_int8_t *bssid)
{
    struct ieee80211com *ic = devhandle;
    return ic->ic_vap_alloc_macaddr(ic,bssid);
}

int wlan_vap_free_mac_addr(wlan_dev_t devhandle, u_int8_t *bssid)
{
    struct ieee80211com *ic = devhandle;
    return ic->ic_vap_free_macaddr(ic,bssid);
}
int
wlan_vap_register_event_handlers(wlan_if_t vaphandle,
                                 wlan_event_handler_table *evtable)
{
    return ieee80211_vap_register_events(vaphandle,evtable);
}

int
wlan_vap_register_mlme_event_handlers(wlan_if_t vaphandle,
                                 os_handle_t oshandle,
                                 wlan_mlme_event_handler_table *evtable)
{
    return ieee80211_vap_register_mlme_events(vaphandle, oshandle, evtable);
}

int
wlan_vap_unregister_mlme_event_handlers(wlan_if_t vaphandle,
                                 os_handle_t oshandle,
                                 wlan_mlme_event_handler_table *evtable)
{
    return ieee80211_vap_unregister_mlme_events(vaphandle, oshandle, evtable);
}

int
wlan_vap_register_misc_event_handlers(wlan_if_t vaphandle,
                                 os_handle_t oshandle,
                                 wlan_misc_event_handler_table *evtable)
{
    return ieee80211_vap_register_misc_events(vaphandle, oshandle, evtable);
}

int
wlan_vap_unregister_misc_event_handlers(wlan_if_t vaphandle,
                                 os_handle_t oshandle,
                                 wlan_misc_event_handler_table *evtable)
{
    return ieee80211_vap_unregister_misc_events(vaphandle, oshandle, evtable);
}

int
wlan_vap_register_ccx_event_handlers(wlan_if_t vaphandle,
                                 os_if_t osif,
                                 wlan_ccx_handler_table *evtable)
{
    return ieee80211_vap_register_ccx_events(vaphandle, osif, evtable);
}

os_if_t 
wlan_vap_get_registered_handle(wlan_if_t vap)
{
    return (os_if_t)vap->iv_ifp;
}

void 
wlan_vap_set_registered_handle(wlan_if_t vap, os_if_t osif)
{
    vap->iv_ifp = osif;
}

wlan_dev_t 
wlan_vap_get_devhandle(wlan_if_t vap)
{
    return (wlan_dev_t)vap->iv_ic;
}

struct wlan_iter_func_arg {
    ieee80211_vap_iter_func iter_func;
    void  *iter_arg;
};

static INLINE void
wlan_iter_func(void *arg, struct ieee80211vap *vap, bool is_last_vap)
{
    struct wlan_iter_func_arg *params =  (struct wlan_iter_func_arg *) arg;  
    if (params->iter_func) {
        params->iter_func(params->iter_arg,vap);
    }
}

u_int32_t wlan_iterate_vap_list(wlan_dev_t ic,ieee80211_vap_iter_func iter_func,void *arg)
{
    u_int32_t num_vaps=0;
    struct wlan_iter_func_arg params;  
    
    params.iter_func = iter_func;
    params.iter_arg = arg;
    ieee80211_iterate_vap_list_internal(ic,wlan_iter_func,(void *) &params,num_vaps);
    return num_vaps;
}

enum ieee80211_opmode
wlan_vap_get_opmode(wlan_if_t vaphandle)
{
    return vaphandle->iv_opmode;
}

u_int8_t *
wlan_vap_get_macaddr(wlan_if_t vaphandle)
{
    return vaphandle->iv_myaddr;
}

u_int8_t *
wlan_vap_get_hw_macaddr(wlan_if_t vaphandle)
{
    return vaphandle->iv_my_hwaddr;
}

ieee80211_aplist_config_t
ieee80211_vap_get_aplist_config(struct ieee80211vap *vap)
{
    return vap->iv_aplist_config;
}

ieee80211_candidate_aplist_t
ieee80211_vap_get_aplist(struct ieee80211vap *vap)
{
    return vap->iv_candidate_aplist;
}

ieee80211_scan_table_t
ieee80211_vap_get_scan_table(struct ieee80211vap *vap)
{
    /*
     * If multiple-VAP scan is enabled, each VAP holds a scan table which is
     * created during VAP initialization.
     * If multiple-VAP scan is disabled, the scan table is kept by the IC and
     * shared by VAPs.
     */
#if ATH_SUPPORT_MULTIPLE_SCANS
    return vap->iv_scan_table;
#else
    return vap->iv_ic->ic_scan_table;
#endif
}

static void
ieee80211_reset_stats(struct ieee80211vap *vap, int reset_hw)
{
    struct ieee80211com *ic = vap->iv_ic;

    OS_MEMZERO(&vap->iv_unicast_stats, sizeof(struct ieee80211_mac_stats));
    OS_MEMZERO(&vap->iv_multicast_stats, sizeof(struct ieee80211_mac_stats));

    if (reset_hw) {
        OS_MEMZERO(&ic->ic_phy_stats[0],
                   sizeof(struct ieee80211_phy_stats) * IEEE80211_MODE_MAX);

        /* clear H/W phy counters */
        ic->ic_clear_phystats(ic);
    }
}

const struct ieee80211_stats *
wlan_gen_stats(wlan_if_t vaphandle)
{
    return &vaphandle->iv_stats;
}

const struct ieee80211_mac_stats *
wlan_mac_stats(wlan_if_t vaphandle, int is_mcast)
{
    if (is_mcast)
        return &vaphandle->iv_multicast_stats;
    else
        return &vaphandle->iv_unicast_stats;
}

const struct ieee80211_phy_stats *
wlan_phy_stats(wlan_dev_t devhandle, enum ieee80211_phymode mode)
{
    struct ieee80211com *ic = devhandle;

    KASSERT(mode != IEEE80211_MODE_AUTO && mode < IEEE80211_MODE_MAX,
            ("Invalid PHY mode\n"));
    
    ic->ic_update_phystats(ic, mode);
    return &ic->ic_phy_stats[mode];
}

systime_t ieee80211_get_last_data_timestamp(wlan_if_t vaphandle)
{
    return vaphandle->iv_lastdata;
}

systime_t ieee80211_get_directed_frame_timestamp(wlan_if_t vaphandle)
{
    /*
     * Now that we have an API it's a good opportunity to synchronize access
     * to this field.
     */
    return vaphandle->iv_last_directed_frame;
}

systime_t ieee80211_get_last_ap_frame_timestamp(wlan_if_t vaphandle)
{
    /*
     * Now that we have an API it's a good opportunity to synchronize access
     * to this field.
     */
    return vaphandle->iv_last_ap_frame;
}

systime_t wlan_get_directed_frame_timestamp(wlan_if_t vaphandle)
{
    return ieee80211_get_directed_frame_timestamp(vaphandle);
}

systime_t wlan_get_last_ap_frame_timestamp(wlan_if_t vaphandle)
{
    return ieee80211_get_last_ap_frame_timestamp(vaphandle);
}

systime_t ieee80211_get_traffic_indication_timestamp(wlan_if_t vaphandle)
{
    return vaphandle->iv_last_traffic_indication;
}

systime_t wlan_get_traffic_indication_timestamp(wlan_if_t vaphandle)
{
    return ieee80211_get_traffic_indication_timestamp(vaphandle);
}

bool ieee80211_is_connected(wlan_if_t vaphandle)
{
    return (ieee80211_vap_ready_is_set(vaphandle));
}

bool wlan_is_connected(wlan_if_t vaphandle)
{
    return (ieee80211_vap_ready_is_set(vaphandle));
}


int wlan_vap_get_bssid(wlan_if_t vaphandle, u_int8_t *bssid)
{
    /* need locking to prevent changing the iv_bss */
    IEEE80211_VAP_LOCK(vaphandle);
    if (vaphandle->iv_bss) {
        IEEE80211_ADDR_COPY(bssid, vaphandle->iv_bss->ni_bssid);
        IEEE80211_VAP_UNLOCK(vaphandle);
        return EOK;
    }
    IEEE80211_VAP_UNLOCK(vaphandle);
    return -EINVAL;
}

int wlan_reset_start(wlan_if_t vaphandle, ieee80211_reset_request *reset_req)
{
    struct ieee80211com *ic = vaphandle->iv_ic;

    /*
     * TBD: Flush data queues only for vaphandle if 
     * IEEE80211_RESET_TYPE_DOT11_INTF is set.
     */
    return ic->ic_reset_start(ic, reset_req->no_flush);
}

static int
ieee80211_vap_reset(struct ieee80211vap *vap, ieee80211_reset_request *reset_req)
{
    struct ieee80211com *ic = vap->iv_ic;
    
    /* Cancel pending MLME requests */
    wlan_mlme_cancel(vap);

    /*
     * Reset node table include all nodes.
     * NB: pairwise keys will be deleted during node cleanup.
     */
    ieee80211_reset_bss(vap);

    /* Reset aplist configuration parameters */
    ieee80211_aplist_config_init(ieee80211_vap_get_aplist_config(vap));

    /* Reset RSN settings */
    ieee80211_rsn_reset(vap);
    
    /* Reset statistics */
    ieee80211_reset_stats(vap, reset_req->reset_hw);
    
    /* Reset some of the misc. vap settings */
    vap->iv_des_modecaps = (1 << IEEE80211_MODE_AUTO);
    vap->iv_des_nssid = 0;
    OS_MEMZERO(&vap->iv_des_ssid[0], (sizeof(ieee80211_ssid) * IEEE80211_SCAN_MAX_SSID));

    /* Because reset_start has graspped a mutex which chan_set
     *will also try to grasp, so don't call ieee80211_set_channel here.
     */
#if !ATH_RESET_SERIAL
    /* Reset some MIB variables if required */
    if (reset_req->set_default_mib) {
        /*
         * NB: Only IEEE80211_RESET_TYPE_DOT11_INTF can reset MIB variables
         */
        KASSERT(reset_req->type == IEEE80211_RESET_TYPE_DOT11_INTF, ("invalid reset request\n"));
        
        if (reset_req->reset_mac) {
            /* reset regdmn module */
            ieee80211_regdmn_reset(ic);
        }

        if (reset_req->reset_phy) {
            /* set the desired PHY mode to 11b */
            vap->iv_des_mode = reset_req->phy_mode;

            /* change to the default PHY mode if required */
            /* set wireless mode */
            ieee80211_setmode(ic, vap->iv_des_mode, vap->iv_opmode);

            /* set default channel */
            ASSERT(vap->iv_des_chan[vap->iv_des_mode] != IEEE80211_CHAN_ANYC);
            ieee80211_set_channel(ic, vap->iv_des_chan[vap->iv_des_mode]);
            vap->iv_bsschan = ic->ic_curchan;
        }
    }
#endif

    return 0;
}

struct ieee80211_vap_iter_reset_arg {
    ieee80211_reset_request *reset_req;
    int  err;
};

static INLINE void ieee80211_vap_iter_reset(void *arg, wlan_if_t vap, bool is_last_vap) 
{
    struct ieee80211_vap_iter_reset_arg *params= (struct ieee80211_vap_iter_reset_arg *) arg;
    /*
     * In case iv_bss was not stopped.
     */
    wlan_mlme_stop_bss(vap,
                       WLAN_MLME_STOP_BSS_F_FORCE_STOP_RESET |
                       WLAN_MLME_STOP_BSS_F_WAIT_RX_DONE);

    params->err = ieee80211_vap_reset(vap, params->reset_req);
}

int
wlan_reset(wlan_if_t vaphandle, ieee80211_reset_request *reset_req)
{
    struct ieee80211vap *vap = vaphandle;
    struct ieee80211com *ic = vap->iv_ic;
    int err = 0;

    /* NB: must set H/W MAC address before chip reset */
    if (reset_req->reset_mac && IEEE80211_ADDR_IS_VALID(reset_req->macaddr) &&
        !IEEE80211_ADDR_EQ(reset_req->macaddr, ic->ic_myaddr)) {

        IEEE80211_ADDR_COPY(ic->ic_myaddr, reset_req->macaddr);
        ic->ic_set_macaddr(ic, reset_req->macaddr);
        IEEE80211_ADDR_COPY(vap->iv_myaddr, ic->ic_myaddr);
        /*
         * TBD: if OS tries to set mac addr when multiple VAP co-exist,
         * we need to notify other VAPs and the corresponding ports
         * so that the port owner can change source address!!
         */
    }

    /* reset UMAC software states */
    if (reset_req->type == IEEE80211_RESET_TYPE_DOT11_INTF) {
        /*
         * In case iv_bss was not stopped.
         */
        wlan_mlme_stop_bss(vap,
                           WLAN_MLME_STOP_BSS_F_FORCE_STOP_RESET |
                           WLAN_MLME_STOP_BSS_F_WAIT_RX_DONE);

        err = ieee80211_vap_reset(vap, reset_req);
    } else if (reset_req->type == IEEE80211_RESET_TYPE_DEVICE) {
        u_int32_t num_vaps;
        struct ieee80211_vap_iter_reset_arg params; 
        params.err=0;
        params.reset_req = reset_req;
        ieee80211_iterate_vap_list_internal(ic,ieee80211_vap_iter_reset,((void *) &params),num_vaps);  
        err = params.err;
    }

    /* TBD: Always reset the hardware? */
    err = ic->ic_reset(ic);
    if (err)
        return err;

    return err;
}

int wlan_reset_end(wlan_if_t vaphandle, ieee80211_reset_request *reset_req)
{
    struct ieee80211com *ic = vaphandle->iv_ic;
    int ret;

    ret = ic->ic_reset_end(ic, reset_req->no_flush);
#if ATH_RESET_SERIAL
    /* Reset some MIB variables if required */
    if (reset_req->set_default_mib) {
        struct ieee80211vap *vap = vaphandle;
        /*
         * NB: Only IEEE80211_RESET_TYPE_DOT11_INTF can reset MIB variables
         */
        KASSERT(reset_req->type == IEEE80211_RESET_TYPE_DOT11_INTF, ("invalid reset request\n"));
        
        if (reset_req->reset_mac) {
            /* reset regdmn module */
            ieee80211_regdmn_reset(ic);
        }

        if (reset_req->reset_phy) {
            /* set the desired PHY mode to 11b */
            vap->iv_des_mode = reset_req->phy_mode;

            /* change to the default PHY mode if required */
            /* set wireless mode */
            ieee80211_setmode(ic, vap->iv_des_mode, vap->iv_opmode);

            /* set default channel */
            ASSERT(vap->iv_des_chan[vap->iv_des_mode] != IEEE80211_CHAN_ANYC);
            ieee80211_set_channel(ic, vap->iv_des_chan[vap->iv_des_mode]);
            vap->iv_bsschan = ic->ic_curchan;
        }
    }
#endif

    return ret;
    
}

int wlan_getrssi(wlan_if_t vaphandle, wlan_rssi_info *rssi_info, u_int32_t flags)
{
    struct ieee80211vap   *vap = vaphandle;
    struct ieee80211_node *bss_node; 
    int err;

    bss_node = ieee80211_ref_bss_node(vap);
    err =  wlan_node_getrssi(bss_node,rssi_info,flags);
    ieee80211_free_node(bss_node);
    return err;
}

int wlan_send_probereq(
    wlan_if_t       vaphandle,
    const u_int8_t  *destination,
    const u_int8_t  *bssid,
    const u_int8_t  *ssid, 
    const u_int32_t ssidlen,
    const void      *optie, 
    const size_t    optielen)
{
    struct ieee80211vap     *vap = vaphandle;
    struct ieee80211_node   *bss_node;
    int                     err;

    bss_node = ieee80211_ref_bss_node(vap);
    err = ieee80211_send_probereq(bss_node,
                                  vaphandle->iv_myaddr,
                                  destination,
                                  bssid,
                                  ssid,
                                  ssidlen,
                                  optie,
                                  optielen);

    ieee80211_free_node(bss_node);

    return err;
}

int wlan_get_txrate_info(wlan_if_t vaphandle, ieee80211_rate_info *rate_info)
{ 
    struct ieee80211vap     *vap = vaphandle;
    struct ieee80211_node   *bss_node ; 
    int err;

    bss_node = ieee80211_ref_bss_node(vap);
    err =  wlan_node_txrate_info(bss_node,rate_info);
    ieee80211_free_node(bss_node);
    return err;
}

int wlan_vap_create_flags(wlan_if_t vaphandle)
{
    return vaphandle->iv_create_flags;
}


ieee80211_resmgr_vap_priv_t 
ieee80211vap_get_resmgr(ieee80211_vap_t vap)
{
    return vap->iv_resmgr_priv;
}

void 
ieee80211vap_set_resmgr(ieee80211_vap_t vap, ieee80211_resmgr_vap_priv_t resmgr_priv)
{
    vap->iv_resmgr_priv = resmgr_priv;
}

/*
 * @Set station tspec.
 * ARGS :
 *  wlan_if_t           : vap handle
 *  u_int_8             : value of tspec state
 * RETURNS:             : void.
 */
void wlan_set_tspecActive(wlan_if_t vaphandle, u_int8_t val)
{
    struct ieee80211com *ic = vaphandle->iv_ic;
    ieee80211_set_tspecActive(ic, val);
}

/*
 * @Indicates whether station tspec is negotiated or not.
 * ARGS :
 *  wlan_if_t           : vap handle
 * RETURNS:             : value of tspec state.
 */
int wlan_is_tspecActive(wlan_if_t vaphandle)
{
    struct ieee80211com *ic = vaphandle->iv_ic;
    return ieee80211_is_tspecActive(ic);
}

u_int32_t wlan_get_tsf32(wlan_if_t vaphandle)
{
    struct ieee80211com *ic = vaphandle->iv_ic;
    return ieee80211_get_tsf32(ic);
}

int wlan_vap_get_tsf_offset(wlan_if_t vaphandle, u_int64_t *tsf_offset)
{
    *tsf_offset = vaphandle->iv_tsf_offset.offset;
    return EOK;
}

