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

#include <osdep.h>

#include <ieee80211_var.h>
#include <ieee80211_channel.h>
#include <ieee80211_rateset.h>
/*zhaoyang1 transplant from 717*/
/*pengruofeng add for wids alarm 2011-5-30*/
#include "osif_private.h"
/*pengruofeng add end 2011-5-30*/
#ifdef PC018
#include <ar9350.h>/*wangyu add 2011-05-23*/
#if 1
#define BG_LED1 15
#define BG_LED2 19
#define A_LED 21
#else
#define BG_LED1 17
#define BG_LED2 19
#define A_LED 14

#endif
//wangyu add for gpio_led
extern int (*turn_up_2G_led)(void);
extern int (*turn_down_2G_led)(void);
extern int (*turn_up_5G_led)(void);
extern int (*turn_down_5G_led)(void);
//end:wangyu add for gpio_led
#endif
/*zhaoyang1 transplant end*/

/* XXX tunables */
#define AGGRESSIVE_MODE_SWITCH_HYSTERESIS   3   /* pkts / 100ms */
#define HIGH_PRI_SWITCH_THRESH              10  /* pkts / 100ms */

#define IEEE80211_VAP_STATE_LOCK_INIT(_vap)       spin_lock_init(&(_vap)->iv_state_info.iv_state_lock)
#define IEEE80211_VAP_STATE_LOCK(_vap)            spin_lock(&(_vap)->iv_state_info.iv_state_lock)
#define IEEE80211_VAP_STATE_UNLOCK(_vap)          spin_unlock(&(_vap)->iv_state_info.iv_state_lock)
#define IEEE80211_VAP_STATE_LOCK_DESTROY(_vap)         spin_lock_destroy(&(_vap)->iv_state_info.iv_state_lock)

const char *ieee80211_state_name[IEEE80211_S_MAX] = {
    "INIT",     /* IEEE80211_S_INIT */
    "SCAN",
    "JOIN",     /* IEEE80211_S_JOIN */
    "AUTH",
    "ASSOC",
    "RUN",       /* IEEE80211_S_RUN */
    "DFS_WAIT",
    "WAIT_TXDONE", /* IEEE80211_S_WAIT_TXDONE */
    "STOPPING",
    "STANDBY"
};

const char *ieee80211_mgt_subtype_name[] = {
    "assoc_req",    "assoc_resp",   "reassoc_req",  "reassoc_resp",
    "probe_req",    "probe_resp",   "reserved#6",   "reserved#7",
    "beacon",       "atim",         "disassoc",     "auth",
    "deauth",       "action",       "reserved#14",  "reserved#15"
};


const char *ieee80211_wme_acnames[] = {
    "WME_AC_BE",
    "WME_AC_BK",
    "WME_AC_VI",
    "WME_AC_VO",
    "WME_UPSD"
};

int ieee80211_state_event(struct ieee80211vap *vap, enum ieee80211_state_event event);

static void ieee80211_vap_scan_event_handler(struct ieee80211vap *originator, ieee80211_scan_event *event, void *arg)
{
    struct ieee80211vap *vap = (struct ieee80211vap *) arg;

    /*
     * Ignore notifications received due to scans requested by other modules.
     */
    if (vap != originator) {
        return;
    }

    switch(event->type) {
     case IEEE80211_SCAN_STARTED:
         IEEE80211_DPRINTF(vap, IEEE80211_MSG_STATE,
                           "%s: orig=%08p vap=%08p scan_id %d event %d reason %d\n", 
                           __func__, 
                           originator, vap,
                           event->scan_id, event->type, event->reason);

         ieee80211_state_event(vap, IEEE80211_STATE_EVENT_SCAN_START);
         break;
     case IEEE80211_SCAN_COMPLETED:
     case IEEE80211_SCAN_DEQUEUED:
         IEEE80211_DPRINTF(vap, IEEE80211_MSG_STATE,
                           "%s: orig=%08p vap=%08p scan_id %d event %d reason %d\n", 
                           __func__, 
                           originator, vap,
                           event->scan_id, event->type, event->reason);

         ieee80211_state_event(vap, IEEE80211_STATE_EVENT_SCAN_END);
         break;
    default:
         break;
    }
}

/*zhaoyang1 transplant from 717*/
/*pengruofeng add for wids alarm 2011-5-30*/
static void
ieee80211_tx_wids_alarm(unsigned long arg)
{
	struct ieee80211vap *vap = (struct ieee80211vap *) arg;
	struct net_device *dev = ((osif_dev *)vap->iv_ifp)->netdev;
	
	if(vap->iv_opmode == IEEE80211_M_HOSTAP){
		union iwreq_data wrq;
		struct wids_detect_attacker_entry *next = NULL;
		struct sta_interfer_detect_entry  *sta_next = NULL;
		char buf[100],buf_sta[80];
		
		int sta_count=0;
		memset(buf, 0, 100);
		memset(buf_sta, 0, 80);
		memset(&wrq, 0, sizeof(wrq));
		
		
		WIDS_LOCK_IRQ(&(vap->iv_sta));
		if(vap->flood_detect == 1){
			next = NULL;
			LIST_FOREACH(next, &vap->iv_sta.attacker, attacker) {
				if (next->assoc_req_cnt > vap->attack_max_cnt) {
					printk("--------------%s--------assoc:%d , max:%d\n",__func__,next->assoc_req_cnt,vap->attack_max_cnt);//pengruofeng_debug
					buf[0]=IEEE80211_WIDS_FLOOD_DETECT;
					memcpy(buf+1,next->mac,IEEE80211_ADDR_LEN);
					memcpy(buf+IEEE80211_ADDR_LEN+1,vap->iv_myaddr,IEEE80211_ADDR_LEN);
					buf[13]=IEEE80211_WIDS_FASSOC_REQUEST;
					buf[14]=vap->iv_ic->ic_curchan->ic_ieee;
					buf[15]=vap->iv_bss->ni_rssi;
					wrq.data.length = 16;
					wireless_send_event(dev, IWEVWIDSALERM, &wrq, buf);
					if((vap->iv_sta).attack_assoc_req_cnt < 1000)
						(vap->iv_sta).attack_assoc_req_cnt++;
					if(next->attack_cnt < 1000)
						next->attack_cnt++;
				}

				if (next->auth_req_cnt > vap->attack_max_cnt) {
					buf[0]=IEEE80211_WIDS_FLOOD_DETECT;
					memcpy(buf+1,next->mac,IEEE80211_ADDR_LEN);
					memcpy(buf+IEEE80211_ADDR_LEN+1,vap->iv_myaddr,IEEE80211_ADDR_LEN);
					buf[13]=IEEE80211_WIDS_FAUTH_REQUEST;
					buf[14]=vap->iv_ic->ic_curchan->ic_ieee;
					buf[15]=next->rssi;
					wrq.data.length = 16;
					wireless_send_event(dev, IWEVWIDSALERM, &wrq, buf);
					if((vap->iv_sta).attack_auth_req_cnt < 1000 )
						(vap->iv_sta).attack_auth_req_cnt++;
		
					if(next->attack_cnt < 1000)
						next->attack_cnt++;
				}

				if (next->reassoc_req_cnt > vap->attack_max_cnt) {
					buf[0]=IEEE80211_WIDS_FLOOD_DETECT;
					memcpy(buf+1,next->mac,IEEE80211_ADDR_LEN);
					memcpy(buf+IEEE80211_ADDR_LEN+1,vap->iv_myaddr,IEEE80211_ADDR_LEN);
					buf[13]=IEEE80211_WIDS_FREASSOC_REQUEST;
					buf[14]=vap->iv_ic->ic_curchan->ic_ieee;
					buf[15]=next->rssi;
					wrq.data.length = 16;
					wireless_send_event(dev, IWEVWIDSALERM, &wrq, buf);
					if((vap->iv_sta).attack_reassoc_req_cnt < 1000 )
						(vap->iv_sta).attack_reassoc_req_cnt++;
					if(next->attack_cnt < 1000)
						next->attack_cnt++;
				}

				if (next->deauth_cnt > vap->attack_max_cnt) {
					buf[0]=IEEE80211_WIDS_FLOOD_DETECT;
					memcpy(buf+1,next->mac,IEEE80211_ADDR_LEN);
					memcpy(buf+IEEE80211_ADDR_LEN+1,vap->iv_myaddr,IEEE80211_ADDR_LEN);
					buf[13]=IEEE80211_WIDS_FDEAUTH_REQUEST;
					buf[14]=vap->iv_ic->ic_curchan->ic_ieee;
					buf[15]=next->rssi;
					wrq.data.length = 16;
					wireless_send_event(dev, IWEVWIDSALERM, &wrq, buf);
					if((vap->iv_sta).attack_deauth_cnt < 1000 )
						(vap->iv_sta).attack_deauth_cnt++;
					if(next->attack_cnt < 1000)
						next->attack_cnt++;
				}

				if (next->disassoc_req_cnt > vap->attack_max_cnt) {
					buf[0]=IEEE80211_WIDS_FLOOD_DETECT;
					memcpy(buf+1,next->mac,IEEE80211_ADDR_LEN);
					memcpy(buf+IEEE80211_ADDR_LEN+1,vap->iv_myaddr,IEEE80211_ADDR_LEN);
					buf[13]=IEEE80211_WIDS_FDISASSO_REQUEST;
					buf[14]=vap->iv_ic->ic_curchan->ic_ieee;
					buf[15]=next->rssi;	
					wrq.data.length = 16;
					wireless_send_event(dev, IWEVWIDSALERM, &wrq, buf);
					if((vap->iv_sta).attack_disassoc_req_cnt < 1000 )
						(vap->iv_sta).attack_disassoc_req_cnt++;
					if(next->attack_cnt < 1000)
						next->attack_cnt++;
				}

				if (next->null_data_cnt > vap->attack_max_cnt) {
					buf[0]=IEEE80211_WIDS_FLOOD_DETECT;
					memcpy(buf+1,next->mac,IEEE80211_ADDR_LEN);
					memcpy(buf+IEEE80211_ADDR_LEN+1,vap->iv_myaddr,IEEE80211_ADDR_LEN);
					buf[13]=IEEE80211_WIDS_FNULL_DATA;
					buf[14]=vap->iv_ic->ic_curchan->ic_ieee;
					buf[15]=next->rssi;
					wrq.data.length = 16;
					wireless_send_event(dev, IWEVWIDSALERM, &wrq, buf);
					if((vap->iv_sta).attack_null_data_cnt < 1000 )
						(vap->iv_sta).attack_null_data_cnt++;
					if(next->attack_cnt < 1000)
						next->attack_cnt++;
				}

				if (next->probe_req_cnt > vap->attack_max_cnt && next->probe_req_cnt > vap->probe_attack_max_cnt) {
					buf[0]=IEEE80211_WIDS_FLOOD_DETECT;
					memcpy(buf+1,next->mac,IEEE80211_ADDR_LEN);
					memcpy(buf+IEEE80211_ADDR_LEN+1,vap->iv_myaddr,IEEE80211_ADDR_LEN);
					buf[13]=IEEE80211_WIDS_FPROBE_REQUEST;
					buf[14]=vap->iv_ic->ic_curchan->ic_ieee;
					buf[15]=next->rssi;
					wrq.data.length = 16;
					wireless_send_event(dev, IWEVWIDSALERM, &wrq, buf);
					if((vap->iv_sta).attack_probe_req_cnt < 1000 )
						(vap->iv_sta).attack_probe_req_cnt++;
					if(next->attack_cnt < 1000)
						next->attack_cnt++;
				}

				if (next->action_cnt > vap->attack_max_cnt ) {
					buf[0]=IEEE80211_WIDS_FLOOD_DETECT;
					memcpy(buf+1,next->mac,IEEE80211_ADDR_LEN);
					memcpy(buf+IEEE80211_ADDR_LEN+1,vap->iv_myaddr,IEEE80211_ADDR_LEN);
					buf[13]=IEEE80211_WIDS_FACTION;
					buf[14]=vap->iv_ic->ic_curchan->ic_ieee;
					buf[15]=next->rssi;
					wrq.data.length = 16;
					wireless_send_event(dev, IWEVWIDSALERM, &wrq, buf);
					if((vap->iv_sta).attack_action_cnt < 1000 )
						(vap->iv_sta).attack_action_cnt++;
					if(next->attack_cnt < 1000)
						next->attack_cnt++;
				}
				next->auth_req_cnt=0;
				next->reassoc_req_cnt=0;
				next->deauth_cnt=0;
				next->disassoc_req_cnt=0;
				next->null_data_cnt=0;
				next->probe_req_cnt=0;
				next->assoc_req_cnt=0;
				next->action_cnt=0;
				
				if((vap->iv_sta).attacker_list_node_cnt >= 50 && !next->attack_cnt){
					LIST_REMOVE(next, attacker);
					kfree(next);
					(vap->iv_sta).attacker_list_node_cnt--;
				}
			}
			
			if((vap->iv_sta).attacker_list_node_cnt >= 50)
			{
				for (next = LIST_FIRST(&vap->iv_sta.attacker); next;
				     next = LIST_FIRST(&vap->iv_sta.attacker)) {
					LIST_REMOVE(next, attacker);
					kfree(next);
				}
				(vap->iv_sta).attacker_list_node_cnt=0;
			}
			
		}

		if(vap->spoof_detect == 1){
			next = NULL;
			LIST_FOREACH(next, &vap->iv_sta.attacker, attacker) {
				if (next->spoof_deauth_cnt) {
					printk("--------------%s--------spoof_deauth_cnt:%d\n",__func__,next->spoof_deauth_cnt);//pengruofeng_debug
					buf[0]=IEEE80211_WIDS_SPOOF_DETECT;
					memcpy(buf+1,next->mac,IEEE80211_ADDR_LEN);
					memcpy(buf+IEEE80211_ADDR_LEN+1,vap->iv_myaddr,IEEE80211_ADDR_LEN);
					buf[13]=IEEE80211_WIDS_SPOOF_DEAUTH;
					buf[14]=vap->iv_ic->ic_curchan->ic_ieee;
					buf[15]=next->rssi;
					wrq.data.length = 16;
					wireless_send_event(dev, IWEVWIDSALERM, &wrq, buf);
					if((vap->iv_sta).attack_spoof_deauth_cnt < 1000)
						(vap->iv_sta).attack_spoof_deauth_cnt++;
					if(next->attack_cnt < 1000)
						next->attack_cnt++;
				}
				
				if (next->spoof_disassoc_cnt) {
					printk("--------------%s--------spoof_disassoc_cnt:%d\n",__func__,next->spoof_disassoc_cnt);//pengruofeng_debug
					buf[0]=IEEE80211_WIDS_SPOOF_DETECT;
					memcpy(buf+1,next->mac,IEEE80211_ADDR_LEN);
					memcpy(buf+IEEE80211_ADDR_LEN+1,vap->iv_myaddr,IEEE80211_ADDR_LEN);
					buf[13]=IEEE80211_WIDS_SPOOF_DISASSOC;
					buf[14]=vap->iv_ic->ic_curchan->ic_ieee;
					buf[15]=next->rssi;
					wrq.data.length = 16;
					wireless_send_event(dev, IWEVWIDSALERM, &wrq, buf);
					if((vap->iv_sta).attack_spoof_disassoc_cnt < 1000)
						(vap->iv_sta).attack_spoof_disassoc_cnt++;
					if(next->attack_cnt < 1000)
						next->attack_cnt++;
				}
				next->spoof_deauth_cnt=0;
				next->spoof_disassoc_cnt=0;
				
				if((vap->iv_sta).attacker_list_node_cnt >= 50 && !next->attack_cnt){
					LIST_REMOVE(next, attacker);
					kfree(next);
					(vap->iv_sta).attacker_list_node_cnt--;
				}
				
			}
			
			if((vap->iv_sta).attacker_list_node_cnt >= 50)
			{
				for (next = LIST_FIRST(&vap->iv_sta.attacker); next;
				     next = LIST_FIRST(&vap->iv_sta.attacker)) {
					LIST_REMOVE(next, attacker);
					kfree(next);
				}
				(vap->iv_sta).attacker_list_node_cnt=0;
			}
			
		}

		if(vap->weakiv_detect == 1){
			next = NULL;
			LIST_FOREACH(next, &vap->iv_sta.attacker, attacker) {
				if (next->weakiv_cnt) {
					buf[0]=IEEE80211_WIDS_WEAKIV_DETECT;
					memcpy(buf+1,next->mac,IEEE80211_ADDR_LEN);
					memcpy(buf+IEEE80211_ADDR_LEN+1,vap->iv_myaddr,IEEE80211_ADDR_LEN);
					buf[13]=IEEE80211_WIDS_WEAKIV_DETECT;
					buf[14]=vap->iv_ic->ic_curchan->ic_ieee;
					buf[15]=next->rssi;
					wrq.data.length = 16;
					wireless_send_event(dev, IWEVWIDSALERM, &wrq, buf);
					if((vap->iv_sta).attack_weakiv_cnt < 1000)
						(vap->iv_sta).attack_weakiv_cnt++;
					if(next->attack_cnt < 1000)
						next->attack_cnt++;
				}
				next->weakiv_cnt=0;
				
				if((vap->iv_sta).attacker_list_node_cnt >= 50 && !next->attack_cnt){
					LIST_REMOVE(next, attacker);
					kfree(next);
					(vap->iv_sta).attacker_list_node_cnt--;
				}
				
			}
			
			if((vap->iv_sta).attacker_list_node_cnt >= 50)
			{
				for (next = LIST_FIRST(&vap->iv_sta.attacker); next;
				     next = LIST_FIRST(&vap->iv_sta.attacker)) {
					LIST_REMOVE(next, attacker);
					kfree(next);
				}
				(vap->iv_sta).attacker_list_node_cnt=0;
			}
			
		}
		WIDS_UNLOCK_IRQ(&(vap->iv_sta));
		
		WIDS_LOCK_IRQ(&(vap->iv_sta_interferece));
		if(vap->sta_interferece_detect){
			sta_next = NULL;
			LIST_FOREACH(sta_next, &vap->iv_sta_interferece.sta_interfer, sta_interfer) {
				if (sta_next->count > vap->sta_interferece_detect) {
					memcpy(&buf_sta[6*sta_count],sta_next->mac,6);
					sta_count++;
					sta_next->count=0;
				}
				else{
					sta_next->count=0;
					LIST_REMOVE(sta_next, sta_interfer);
					kfree(sta_next);
				}
				
				
			}
			
			if(sta_count >= vap->sta_interferece_count )
			{
				if(!(vap->iv_sta_interferece).trap_flag)
				{
					(vap->iv_sta_interferece).clear_flag = 0;
					(vap->iv_sta_interferece).trap_flag = 1;
					buf[0]=IEEE80211_STA_INTERFER_DETECT;
					buf[1]=vap->iv_ic->ic_curchan->ic_ieee;
					buf[2]=sta_count;
					memcpy(buf+3,buf_sta,sta_count*6);
					memcpy(buf+sta_count*6+3,vap->iv_myaddr,IEEE80211_ADDR_LEN);
					wrq.data.length = 3 + (sta_count + 1)*6;
					wireless_send_event(dev, IWEVWIDSALERM, &wrq, buf);
				}
			}
			else
			{
				if(!(vap->iv_sta_interferece).clear_flag)
				{
					(vap->iv_sta_interferece).trap_flag = 0;
					(vap->iv_sta_interferece).clear_flag = 1;
					buf[0]=IEEE80211_STA_INTERFER_CLEAR;
					buf[1]=vap->iv_ic->ic_curchan->ic_ieee;
					memcpy(buf+2,vap->iv_myaddr,IEEE80211_ADDR_LEN);
					wrq.data.length = 2 + 6;
					wireless_send_event(dev, IWEVWIDSALERM, &wrq, buf);

				}
			}
				
		}
		WIDS_UNLOCK_IRQ(&(vap->iv_sta_interferece));
		
		/*ljy--modified begin for the interval of wids detect*/
		//mod_timer(&(vap->iv_widsalarm), jiffies + msecs_to_jiffies(1000));
		mod_timer(&(vap->iv_widsalarm), jiffies + msecs_to_jiffies(vap->wids_interval));
		/*ljy--modified end*/
		
		
	}
}
/*pengruofeng add end 2011-5-30*/
/*zhaoyang1 transplant end*/


void
ieee80211_proto_attach(struct ieee80211com *ic)
{
    ic->ic_protmode = IEEE80211_PROT_CTSONLY;

    ic->ic_wme.wme_hipri_switch_hysteresis =
        AGGRESSIVE_MODE_SWITCH_HYSTERESIS;

#if 0  /* XXX TODO */
    ieee80211_auth_setup();
#endif
}

void
ieee80211_proto_detach(struct ieee80211com *ic)
{
}

void ieee80211_proto_vattach(struct ieee80211vap *vap)
{
    int    rc;
    
#ifdef notdef
    vap->iv_rtsthreshold = IEEE80211_RTS_DEFAULT;
#else
    vap->iv_rtsthreshold = IEEE80211_RTS_MAX;
#endif
    vap->iv_fragthreshold = IEEE80211_FRAGMT_THRESHOLD_MAX;
    vap->iv_fixed_rate.mode = IEEE80211_FIXED_RATE_NONE;

	 /*zhaoyang1 transplant from 717*/
     /* pengruofeng add for wids 2011-5-30*/
		
    if(vap->iv_opmode == IEEE80211_M_HOSTAP){
	init_timer(&(vap->iv_widsalarm));
	vap->iv_widsalarm.function = ieee80211_tx_wids_alarm;
	vap->iv_widsalarm.data = (unsigned long) vap;
	vap->wids_interval = 1000;
	mod_timer(&(vap->iv_widsalarm), jiffies + msecs_to_jiffies(vap->wids_interval));
			
	WIDS_LOCK_INIT(&(vap->iv_sta), "wids");
			
	LIST_INIT(&vap->iv_sta.attacker);
	(vap->iv_sta).attacker_entry = NULL;
	(vap->iv_sta).attack_assoc_req_cnt=0;
	(vap->iv_sta).attack_auth_req_cnt=0;
	(vap->iv_sta).attack_deauth_cnt=0;
	(vap->iv_sta).attack_disassoc_req_cnt=0;
	(vap->iv_sta).attack_null_data_cnt=0;
	(vap->iv_sta).attack_probe_req_cnt=0;
	(vap->iv_sta).attack_reassoc_req_cnt=0;
	(vap->iv_sta).attack_action_cnt=0;
	(vap->iv_sta).attack_spoof_deauth_cnt=0;
	(vap->iv_sta).attack_spoof_disassoc_cnt=0;
	(vap->iv_sta).attack_weakiv_cnt=0;
	(vap->iv_sta).attacker_list_node_cnt=0;
	vap->attack_max_cnt = 30;
	vap->probe_attack_max_cnt = 0;
			
		WIDS_LOCK_INIT(&(vap->iv_sta_interferece), "sta interferece");
		LIST_INIT(&vap->iv_sta_interferece.sta_interfer);
		(vap->iv_sta_interferece).sta_interfer_entry = NULL;
		(vap->iv_sta_interferece).trap_flag=0;
		(vap->iv_sta_interferece).clear_flag=1;
		vap->sta_interferece_count=2;
		vap->sta_interferece_detect = 0;
	vap->iv_weakiv_threshold = 50;
			
    }
    /* pengruofeng add end 2011-5-30*/
	/*zhaoyang1 transplant end*/
    IEEE80211_VAP_STATE_LOCK_INIT(vap);

    vap->iv_state_info.iv_state = IEEE80211_S_INIT;
    rc = wlan_scan_register_event_handler(vap, ieee80211_vap_scan_event_handler,(void *) vap);
    if (rc != EOK) {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_ANY,
                          "%s: wlan_scan_register_event_handler() failed handler=%08p,%08p rc=%08X\n",
                          __func__, ieee80211_vap_scan_event_handler, vap, rc);
    }
}

void
ieee80211_proto_vdetach(struct ieee80211vap *vap)
{
    int    rc;
    
    rc = wlan_scan_unregister_event_handler(vap, ieee80211_vap_scan_event_handler,(void *) vap);
    if (rc != EOK) {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_ANY,
                          "%s: wlan_scan_unregister_event_handler() failed handler=%08p,%08p rc=%08X\n",
                          __func__, ieee80211_vap_scan_event_handler, vap, rc);
    }
    IEEE80211_VAP_STATE_LOCK_DESTROY(vap);

	/*zhaoyang1 transplant from 717*/
	/*pengruofeng--add for wids 2011-5-30*/
    if (vap->iv_opmode == IEEE80211_M_HOSTAP)
    {
        	del_timer(&(vap->iv_widsalarm));
	WIDS_LOCK_DESTROY(&(vap->iv_sta));
    }
    /*pengruofeng--add end 2011-5-30*/
	/*zhaoyang1 transplant end*/
}

/*
 * event handler for the vap state machine.
 */
int
ieee80211_state_event(struct ieee80211vap *vap, enum ieee80211_state_event event)
{
    enum ieee80211_state cur_state;
    enum ieee80211_state nstate;
    struct ieee80211com *ic = vap->iv_ic;
    ieee80211_vap_event evt;
    u_int32_t           vap_node_count = 0;
    bool                deleteVap = FALSE;

    IEEE80211_DPRINTF(vap, IEEE80211_MSG_STATE,
                      "%s: VAP state event %d \n", __func__,event);
    /*
     * Provide synchronization on vap state change.
     * Cannot guarantee the final vap state to be requested state.
     * Caller is responsible for state change serialization.
     */

    /*
     * Temp workaround before resource manager control ic state change.
     * For now, ic/sc state is also being changed in this thread context.
     * We need to synchronize ic/sc state change across vaps.
     * TBD: resource manager should keep its own vap states and flags
     * (active, ready ...etc) and change ic/sc state accordingly.
     * Restore IEEE80211_VAP_STATE_LOCK after it is done.
     */
    //IEEE80211_VAP_STATE_LOCK(vap);
    IEEE80211_STATE_LOCK(ic);
    /*
     * reentrancy check to catch any cases of reentrancy.
    */
    if(vap->iv_state_info.iv_sm_running) {
      ieee80211com_note(ic,"ASSERT: Can not reenter VAP State Machine \n"); 
    }
    vap->iv_state_info.iv_sm_running = true;

    cur_state =(enum ieee80211_state)vap->iv_state_info.iv_state;
    nstate = cur_state;

    /*
     * Preprocess of the event.
     */
    switch(event) {
        case IEEE80211_STATE_EVENT_RESUME:
            if (cur_state != IEEE80211_S_STANDBY) {
                evt.type = IEEE80211_VAP_RESUMED;
                ieee80211_vap_deliver_event(vap, &evt);
            }
            break;

        case IEEE80211_STATE_EVENT_FORCE_STOP:
            /*
             * Caller is responsible for cleaning up node and data queues. 
             */
            nstate = IEEE80211_S_INIT;
            break;

        case IEEE80211_STATE_EVENT_BSS_NODE_FREED:
            /* Multiple threads can try to remove the node count. */
            IEEE80211_VAP_LOCK(vap);
            vap_node_count = (--vap->iv_node_count);
            IEEE80211_VAP_UNLOCK(vap);
            IEEE80211_DPRINTF(vap, IEEE80211_MSG_STATE,
           "%s: VAP EVENT_NODE_FREED node count %d \n", __func__,vap_node_count);
            break;

        default:
            break;
    }


    /*
     * main event handler switch statement.
     */

    switch(cur_state) {

    case IEEE80211_S_INIT:
        switch(event) {
           case IEEE80211_STATE_EVENT_UP:
               nstate = IEEE80211_S_RUN;
               break;
           case IEEE80211_STATE_EVENT_JOIN:
               nstate = IEEE80211_S_JOIN;
               break;
           case IEEE80211_STATE_EVENT_SCAN_START:
               nstate = IEEE80211_S_SCAN;
               break;
           case IEEE80211_STATE_EVENT_BSS_NODE_FREED:
                if (vap_node_count == 1 &&
                    (!ieee80211_vap_deleted_is_set(vap))) {
                    ASSERT(!ieee80211_vap_standby_is_set(vap));
                    IEEE80211COM_DELIVER_VAP_EVENT(vap->iv_ic, vap->iv_ifp, IEEE80211_VAP_STOPPED);
                } else if (vap_node_count == 0) {
                    ASSERT(ieee80211_vap_deleted_is_set(vap));
                    /* Last node, delete vap. */
                    deleteVap = TRUE;
                } 
                
    /*Begin :deleted by zhanghu for the system handle up when the ath.0-1 deleted AZT-43 2012-11-12*/
    /****************************
    
        //<Begin :caizhibang modify for ZT tunnel 2012-2-24				
		   //added by chenming for CHINAMOBILE-353
                else if(ieee80211_vap_deleted_is_set(vap))
                {
                    deleteVap = TRUE;
                    vap->iv_node_count = 0;
                } 
                //ended by chenming
        //End :caizhibang modify for ZT tunnel 2012-2-24>
        
        ******************************/
    /*End:deleted by zhanghu for the system handle up when the ath.0-1 deleted AZT-43 2012-11-12*/

                break;
           case IEEE80211_STATE_EVENT_DFS_WAIT:
             nstate = IEEE80211_S_DFS_WAIT;
             break;
           case IEEE80211_STATE_EVENT_DFS_CLEAR:
               nstate = IEEE80211_S_RUN;
             break;
	   case IEEE80211_STATE_EVENT_CHAN_SET:
              vap->iv_state_info.iv_sm_running = false;
              IEEE80211_STATE_UNLOCK(ic);
               return 0;
               break;
           default:
               break; 
        }
        break;
    case IEEE80211_S_SCAN:
        switch(event) {
           case IEEE80211_STATE_EVENT_UP:
               nstate = IEEE80211_S_RUN;
               break;
           case IEEE80211_STATE_EVENT_JOIN:
               nstate = IEEE80211_S_JOIN;
               break;
           case IEEE80211_STATE_EVENT_SCAN_END:
               nstate = IEEE80211_S_INIT;
               break;
           case IEEE80211_STATE_EVENT_DFS_CLEAR:
               nstate = IEEE80211_S_RUN;
             break;
           case IEEE80211_STATE_EVENT_DFS_WAIT:
             nstate = IEEE80211_S_DFS_WAIT;
             break;
	   case IEEE80211_STATE_EVENT_CHAN_SET:
               vap->iv_state_info.iv_sm_running = false;
               IEEE80211_STATE_UNLOCK(ic);
               return 0;
               break;
           default:
               break;
        }
        break;
    /*
     * If the caller is not blocked and waiting for bss to be stopped,
     * this state can be viewed as INIT state by the caller.
     * We should allow caller to put VAP in any state.
     */ 
    case IEEE80211_S_STOPPING:
        switch(event) {
           case IEEE80211_STATE_EVENT_UP:
               if (!ieee80211_vap_standby_is_set(vap))
                   nstate = IEEE80211_S_RUN;
               break;
           case IEEE80211_STATE_EVENT_JOIN:
               if (!ieee80211_vap_standby_is_set(vap))
                   nstate = IEEE80211_S_JOIN;
               break;
           case IEEE80211_STATE_EVENT_SCAN_START:
               if (!ieee80211_vap_standby_is_set(vap))
                   nstate = IEEE80211_S_SCAN;
               break;
           case IEEE80211_STATE_EVENT_DOWN:
               ieee80211_vap_standby_clear(vap);
               break;
           case IEEE80211_STATE_EVENT_BSS_NODE_FREED:
                if (vap_node_count == 1 &&
                    (!ieee80211_vap_deleted_is_set(vap))) {
                    if (ieee80211_vap_standby_is_set(vap)) {
                        nstate = IEEE80211_S_STANDBY;
                    } else {
                        nstate = IEEE80211_S_INIT;
                    }
                } else if (vap_node_count == 0) {
                    ASSERT(ieee80211_vap_deleted_is_set(vap));
                    /* Last node, delete vap. */
                    deleteVap = TRUE;
                    nstate = IEEE80211_S_INIT;
                } 
                break;
           case IEEE80211_STATE_EVENT_DFS_CLEAR:
               nstate = IEEE80211_S_RUN;
             break;
           case IEEE80211_STATE_EVENT_DFS_WAIT:
             nstate = IEEE80211_S_DFS_WAIT;
             break;
           case IEEE80211_STATE_EVENT_CHAN_SET:
               vap->iv_state_info.iv_sm_running = false;
               IEEE80211_STATE_UNLOCK(ic);
               return 0;
               break;
           default:
               break; 
        }
        break;
    case IEEE80211_S_JOIN:
        switch(event) {
           case IEEE80211_STATE_EVENT_UP:
               nstate = IEEE80211_S_RUN;
               break;
           case IEEE80211_STATE_EVENT_DOWN:
               nstate = IEEE80211_S_STOPPING;
               break;
           case IEEE80211_STATE_EVENT_DFS_CLEAR:
               nstate = IEEE80211_S_RUN;
             break;
           case IEEE80211_STATE_EVENT_DFS_WAIT:
             nstate = IEEE80211_S_DFS_WAIT;
             break;
	   case IEEE80211_STATE_EVENT_CHAN_SET:
               vap->iv_state_info.iv_sm_running = false;
               IEEE80211_STATE_UNLOCK(ic);
               return 0;
               break;
           default:
               break; 
        }
        break;
    case IEEE80211_S_RUN:
        switch(event) {
           case IEEE80211_STATE_EVENT_JOIN:
               nstate = IEEE80211_S_JOIN;
               break;
           case IEEE80211_STATE_EVENT_STANDBY:
               ieee80211_vap_standby_set(vap);
               nstate = IEEE80211_S_STOPPING;
               break;
           case IEEE80211_STATE_EVENT_DOWN:
               nstate = IEEE80211_S_STOPPING;
               break;
           case IEEE80211_STATE_EVENT_DFS_WAIT:
             nstate = IEEE80211_S_DFS_WAIT;
             break;
           case IEEE80211_STATE_EVENT_DFS_CLEAR:
               nstate = IEEE80211_S_RUN;
	       break;
	   case IEEE80211_STATE_EVENT_CHAN_SET:
               vap->iv_state_info.iv_sm_running = false;
               IEEE80211_STATE_UNLOCK(ic);
               return 0;
               break;
	case IEEE80211_STATE_EVENT_SCAN_END:
           case IEEE80211_STATE_EVENT_SCAN_START:
               vap->iv_state_info.iv_sm_running = false;
               IEEE80211_STATE_UNLOCK(ic);
               return 0;
               break;
           default:
               break; 
        }
	break;
    case IEEE80211_S_DFS_WAIT:
	switch(event) {
	case IEEE80211_STATE_EVENT_JOIN:
	    nstate = cur_state;
	    break;
	case IEEE80211_STATE_EVENT_UP:
	    nstate = cur_state;
	    break;
	case IEEE80211_STATE_EVENT_DFS_CLEAR:
	    nstate = IEEE80211_S_RUN;
	    break;
	case IEEE80211_STATE_EVENT_DOWN:
	    nstate = IEEE80211_S_STOPPING;
	    break;
	case IEEE80211_STATE_EVENT_CHAN_SET:
	    nstate = IEEE80211_S_INIT;
	    break;
	default:
	    break;
        }
	break;
    case IEEE80211_S_AUTH:
    case IEEE80211_S_ASSOC:
        break;
    case IEEE80211_S_STANDBY:
        switch(event) {
           case IEEE80211_STATE_EVENT_UP:
               nstate = IEEE80211_S_RUN;
               break;
           case IEEE80211_STATE_EVENT_DOWN:
               nstate = IEEE80211_S_INIT;
               break;
	   case IEEE80211_STATE_EVENT_CHAN_SET:
               vap->iv_state_info.iv_sm_running = false;
               IEEE80211_STATE_UNLOCK(ic);
               return 0;
               break;
           default:
               break; 
        }
        break;
    default:
        break;
    }

    /*
     * allow RUN to RUN state transition for IBSS mode .
     */
    if ( nstate == cur_state && !(ic->ic_opmode == IEEE80211_M_IBSS && nstate == IEEE80211_S_RUN)) {
        vap->iv_state_info.iv_sm_running = false;
        IEEE80211_STATE_UNLOCK(ic);
        if (deleteVap) {
            IEEE80211_DPRINTF(vap, IEEE80211_MSG_STATE, "%s: VAP state transition %s -> %s vap_free \n", __func__,
                      ieee80211_state_name[cur_state],
                      ieee80211_state_name[nstate]);
            ieee80211_vap_free(vap);
        }
        return 0;
    }

    IEEE80211_DPRINTF(vap, IEEE80211_MSG_STATE,
                      "%s: VAP state transition %s -> %s \n", __func__,
                      ieee80211_state_name[cur_state],
                      ieee80211_state_name[nstate]);

    /*
     * exit action for the current state.
     */
    switch(cur_state) {
        /*
         * send vap power  state info to the resource  manager and other registred handlers.
         * if resource manager is not compiled in, then directly change the chip power state.
         * from INIT --> * we need to generrate events before we handle the action for each state.
         *  so that resource manager can enable th HW before vap executes.
         */
    case IEEE80211_S_INIT:
        evt.type = IEEE80211_VAP_ACTIVE;
        ieee80211_vap_deliver_event(vap, &evt);
        break;
    case IEEE80211_S_RUN:
        if (nstate != IEEE80211_S_RUN) {
            /* from RUN -> others , back to active*/
            ieee80211_vap_ready_clear(vap);
            evt.type = IEEE80211_VAP_DOWN;
            ieee80211_vap_deliver_event(vap, &evt);
#if UMAC_SUPPORT_VAP_PAUSE
            ieee80211_vap_pause_reset(vap);    /* Reset VAP pause counters/state */
#endif
        } 
        break;
    default:
        break;

    }

    /*
     * entry action for each state .
     */
    switch(nstate) {
    case IEEE80211_S_STANDBY:
        ieee80211_vap_standby_clear(vap);
        ieee80211_vap_dfswait_clear(vap);
        /*
         * fall through.
         * ready, active, scanning...etc flags need to be cleared
         * iv_down needs to be called.
         */
    case IEEE80211_S_INIT:
        if (event == IEEE80211_STATE_EVENT_FORCE_STOP &&
            cur_state != IEEE80211_S_STOPPING) {
            /* 
             * This is a forced stop. We need to call stopping first
             * to release beacon resource.
             */
            vap->iv_stopping(vap);
        }

        if (cur_state != IEEE80211_S_STANDBY) {
            ASSERT(!ieee80211_vap_standby_is_set(vap));
            ieee80211_vap_ready_clear(vap);
            ieee80211_vap_active_clear(vap);
            ieee80211_vap_scanning_clear(vap);
            vap->iv_down(vap);
        }

        /* 
         * from *(Active) --> INIT we need to generrate events after we handle the action for INIT state.
         * so that resource monager will disable HW as the  last operation.
         */
        if (nstate == IEEE80211_S_STANDBY) {
            evt.type = IEEE80211_VAP_STANDBY;
            ieee80211_vap_deliver_event(vap, &evt);
        } else {
            evt.type = IEEE80211_VAP_FULL_SLEEP;
            ieee80211_vap_deliver_event(vap, &evt);
            IEEE80211COM_DELIVER_VAP_EVENT(vap->iv_ic, vap->iv_ifp, IEEE80211_VAP_STOPPED);
        }
        ieee80211_vap_dfswait_clear(vap);

        IEEE80211_VAP_LOCK(vap);
        if (vap->iv_node_count == 0) {
            // IEEE80211_STATE_EVENT_BSS_NODE_FREED event has been received.
            ASSERT(ieee80211_vap_deleted_is_set(vap));
            deleteVap = TRUE;
        }
        IEEE80211_VAP_UNLOCK(vap);
        break;
     case IEEE80211_S_STOPPING:
         vap->iv_stopping(vap);
         ieee80211_vap_ready_clear(vap);
         ieee80211_vap_active_clear(vap);
         ieee80211_vap_scanning_clear(vap);
         ieee80211_vap_dfswait_clear(vap);
         if (!ieee80211_vap_standby_is_set(vap)) {
             /* 
              * Note: if standby flag is set, then this is not really
              * "Stopping" but "waiting to go to Standby". Therefore,
              * only send VAP_STOPPING if standby flag unset.
              */
             evt.type = IEEE80211_VAP_STOPPING;
             ieee80211_vap_deliver_event(vap, &evt);
         }
         break;
     case IEEE80211_S_SCAN:
         ieee80211_vap_active_set(vap);
         ieee80211_vap_ready_clear(vap);
         ieee80211_vap_scanning_set(vap);
         vap->iv_listen(vap);
	 ieee80211_vap_dfswait_clear(vap);
         break;
     case IEEE80211_S_JOIN:
         ieee80211_vap_scanning_clear(vap);
         ieee80211_vap_ready_clear(vap);
         vap->iv_join(vap);
         ieee80211_vap_active_set(vap);
	 ieee80211_vap_dfswait_clear(vap);
         break;
     case IEEE80211_S_RUN:
         /*
          * to avoid the race, set the ready  flag after bringing the vap
          * up.
          */
         ieee80211_vap_scanning_clear(vap);
         ieee80211_vap_active_set(vap);
         vap->iv_up(vap);
         ieee80211_vap_ready_set(vap);

         /* Set default mcast rate */
         ieee80211_set_mcast_rate(vap);

         ieee80211_rptplacement_set_param(vap, IEEE80211_RPT_DEVUP, 1);

         if (cur_state != IEEE80211_S_RUN) {

             if (cur_state == IEEE80211_S_STANDBY) {
                evt.type = IEEE80211_VAP_RESUMED;
                ieee80211_vap_deliver_event(vap, &evt);
             } else {
                /* from other -> RUN */
                evt.type = IEEE80211_VAP_UP;
                ieee80211_vap_deliver_event(vap, &evt);
             }
         }
	 ieee80211_vap_dfswait_clear(vap);
         break;
    case IEEE80211_S_DFS_WAIT:
	ieee80211_vap_dfswait_set(vap);
	ieee80211_vap_active_clear(vap);
	ieee80211_vap_ready_clear(vap);
	ieee80211_vap_scanning_clear(vap);
	break;
    default:
        break;
    }

    vap->iv_state_info.iv_state = (u_int32_t)nstate;

    vap->iv_state_info.iv_sm_running = false;
    IEEE80211_STATE_UNLOCK(ic);

    if (deleteVap) {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_STATE, "%s: vap_free \n", __func__);
        ieee80211_vap_free(vap);
    }

    return 0;
}

int
ieee80211_vap_start(struct ieee80211vap *vap)
{
	/*zhaoyang1 transplant from 717*/
	//wangyu add 2011-05-23
	#ifdef PC018
	if((vap->iv_cur_mode == IEEE80211_MODE_11A) \
		||(vap->iv_cur_mode == IEEE80211_MODE_TURBO_A ) \
		||(vap->iv_cur_mode == IEEE80211_MODE_11NA_HT20 ) \
		||(vap->iv_cur_mode == IEEE80211_MODE_11NA_HT40PLUS ) \
		||(vap->iv_cur_mode == IEEE80211_MODE_11NA_HT40MINUS)){
		if(turn_up_5G_led){
		turn_up_5G_led();
			}
		}
	if((vap->iv_cur_mode == IEEE80211_MODE_11B ) \
		||(vap->iv_cur_mode == IEEE80211_MODE_11G) \
		||(vap->iv_cur_mode == IEEE80211_MODE_FH ) \
		||(vap->iv_cur_mode == IEEE80211_MODE_TURBO_G ) \
		||(vap->iv_cur_mode == IEEE80211_MODE_11NG_HT20) \
		||(vap->iv_cur_mode == IEEE80211_MODE_11NG_HT40PLUS) \
		||(vap->iv_cur_mode == IEEE80211_MODE_11NG_HT40MINUS) \
		||(vap->iv_cur_mode == IEEE80211_MODE_11NG_HT40) \
		||(vap->iv_cur_mode == IEEE80211_MODE_11NA_HT40 )){
		if(turn_up_2G_led){
		turn_up_2G_led();
			}
		}
#endif
	//wangyu add end
	//zhengkun add 2011-11-11
	
	#ifdef APM82181
	if((vap->iv_cur_mode == IEEE80211_MODE_11A) \
		||(vap->iv_cur_mode == IEEE80211_MODE_TURBO_A ) \
		||(vap->iv_cur_mode == IEEE80211_MODE_11NA_HT20 ) \
		||(vap->iv_cur_mode == IEEE80211_MODE_11NA_HT40PLUS ) \
		||(vap->iv_cur_mode == IEEE80211_MODE_11NA_HT40MINUS))
	{
       extern  void open_5G_led();
	   open_5G_led();
	}
	if((vap->iv_cur_mode == IEEE80211_MODE_11B ) \
		||(vap->iv_cur_mode == IEEE80211_MODE_11G) \
		||(vap->iv_cur_mode == IEEE80211_MODE_FH ) \
		||(vap->iv_cur_mode == IEEE80211_MODE_TURBO_G ) \
		||(vap->iv_cur_mode == IEEE80211_MODE_11NG_HT20) \
		||(vap->iv_cur_mode == IEEE80211_MODE_11NG_HT40PLUS) \
		||(vap->iv_cur_mode == IEEE80211_MODE_11NG_HT40MINUS) \
		||(vap->iv_cur_mode == IEEE80211_MODE_11NG_HT40) \
		||(vap->iv_cur_mode == IEEE80211_MODE_11NA_HT40 ))
	{
     extern void open_2G_led();
	 open_2G_led();
	}	
#endif
	//zhengkun add end 
	/*zhaoyang1 transplant end*/
  return  ieee80211_state_event(vap,IEEE80211_STATE_EVENT_UP);
}

void
ieee80211_vap_stop(struct ieee80211vap *vap, bool force)
{
	/*zhaoyang1 transplant from 717*/
	//wangyu add 2011-05-23
	#ifdef PC018
	if((vap->iv_cur_mode == IEEE80211_MODE_11A) \
		||(vap->iv_cur_mode == IEEE80211_MODE_TURBO_A ) \
		||(vap->iv_cur_mode == IEEE80211_MODE_11NA_HT20 ) \
		||(vap->iv_cur_mode == IEEE80211_MODE_11NA_HT40PLUS ) \
		||(vap->iv_cur_mode == IEEE80211_MODE_11NA_HT40MINUS)){
	if(turn_down_5G_led){
		turn_down_5G_led();
		}
		}
	if((vap->iv_cur_mode == IEEE80211_MODE_11B ) \
		||(vap->iv_cur_mode == IEEE80211_MODE_11G) \
		||(vap->iv_cur_mode == IEEE80211_MODE_FH ) \
		||(vap->iv_cur_mode == IEEE80211_MODE_TURBO_G ) \
		||(vap->iv_cur_mode == IEEE80211_MODE_11NG_HT20) \
		||(vap->iv_cur_mode == IEEE80211_MODE_11NG_HT40PLUS) \
		||(vap->iv_cur_mode == IEEE80211_MODE_11NG_HT40MINUS) \
		||(vap->iv_cur_mode == IEEE80211_MODE_11NG_HT40) \
		||(vap->iv_cur_mode == IEEE80211_MODE_11NA_HT40 )){
		if(turn_down_2G_led){
		turn_down_2G_led();
			}
		}
	#endif
	//wangyu add end
   //zhengkun add 2011-11-11
    #ifdef APM82181
	if((vap->iv_cur_mode == IEEE80211_MODE_11A) \
		||(vap->iv_cur_mode == IEEE80211_MODE_TURBO_A ) \
		||(vap->iv_cur_mode == IEEE80211_MODE_11NA_HT20 ) \
		||(vap->iv_cur_mode == IEEE80211_MODE_11NA_HT40PLUS ) \
		||(vap->iv_cur_mode == IEEE80211_MODE_11NA_HT40MINUS))
	{
       extern  void close_5G_led();
	   close_5G_led();
	}
	if((vap->iv_cur_mode == IEEE80211_MODE_11B ) \
		||(vap->iv_cur_mode == IEEE80211_MODE_11G) \
		||(vap->iv_cur_mode == IEEE80211_MODE_FH ) \
		||(vap->iv_cur_mode == IEEE80211_MODE_TURBO_G ) \
		||(vap->iv_cur_mode == IEEE80211_MODE_11NG_HT20) \
		||(vap->iv_cur_mode == IEEE80211_MODE_11NG_HT40PLUS) \
		||(vap->iv_cur_mode == IEEE80211_MODE_11NG_HT40MINUS) \
		||(vap->iv_cur_mode == IEEE80211_MODE_11NG_HT40) \
		||(vap->iv_cur_mode == IEEE80211_MODE_11NA_HT40 ))
	{
       extern  void close_2G_led();
	   close_2G_led();
	
	}
	#endif
   //zhengkun add end 
   /*zhaoyang1 transplant end*/
    if (force) {
        ieee80211_state_event(vap, IEEE80211_STATE_EVENT_FORCE_STOP);
    } else {
        ieee80211_state_event(vap, IEEE80211_STATE_EVENT_DOWN);
    }    
}

void
ieee80211_vap_standby(struct ieee80211vap *vap)
{
    ieee80211_state_event(vap, IEEE80211_STATE_EVENT_STANDBY);
}

/*
 * temp WAR for windows hang (dead lock). It can be removed when VAP SM is re-written (bug 65137).
 */
static void
ieee80211_vap_bss_node_freed_handle(struct ieee80211vap *vap, void* workItemHandle)
{
    ieee80211_state_event(vap, IEEE80211_STATE_EVENT_BSS_NODE_FREED);
    OS_FREE_ROUTING(workItemHandle);
}

void
ieee80211_vap_bss_node_freed(struct ieee80211vap *vap)
{
    /*
     * ieee80211_vap_bss_node_freed_handle takes a "struct ieee80211vap *"
     * pointer as its initial arg, rather than the "void *" pointer
     * specified by os_tasklet_routine_t, so a typecast is needed.
     */
    OS_SCHEDULE_ROUTING(
        vap->iv_ic->ic_osdev,
        (os_tasklet_routine_t) ieee80211_vap_bss_node_freed_handle,
        vap);
}

int
ieee80211_vap_join(struct ieee80211vap *vap)
{
  return  ieee80211_state_event(vap,IEEE80211_STATE_EVENT_JOIN);
}

/*
 * Reset 11g-related state.
 */
void
ieee80211_reset_erp(struct ieee80211com *ic,
                    enum ieee80211_phymode mode,
                    enum ieee80211_opmode opmode)
{
#define IS_11G(m) \
    ((m) == IEEE80211_MODE_11G || (m) == IEEE80211_MODE_TURBO_G)

    struct ieee80211_channel *chan = ieee80211_get_current_channel(ic);

    IEEE80211_DISABLE_PROTECTION(ic);

    /*
     * Preserve the long slot and nonerp station count if
     * switching between 11g and turboG. Otherwise, inactivity
     * will cause the turbo station to disassociate and possibly
     * try to leave the network.
     * XXX not right if really trying to reset state
     */
    if (IS_11G(mode) ^ IS_11G(ic->ic_curmode)) {
        ic->ic_nonerpsta = 0;
        ic->ic_longslotsta = 0;
    }

    /*
     * Short slot time is enabled only when operating in 11g
     * and not in an IBSS.  We must also honor whether or not
     * the driver is capable of doing it.
     */
    ieee80211_set_shortslottime(
        ic,
        IEEE80211_IS_CHAN_A(chan) ||
        IEEE80211_IS_CHAN_11NA(chan) ||
        ((IEEE80211_IS_CHAN_ANYG(chan) || IEEE80211_IS_CHAN_11NG(chan)) &&
         (opmode == IEEE80211_M_HOSTAP || opmode == IEEE80211_M_BTAMP) &&
         (ic->ic_caps & IEEE80211_C_SHSLOT)));

    /*
     * Set short preamble and ERP barker-preamble flags.
     */
    if (IEEE80211_IS_CHAN_A(chan) ||
        IEEE80211_IS_CHAN_11NA(chan) ||
        (ic->ic_caps & IEEE80211_C_SHPREAMBLE)) {
        IEEE80211_ENABLE_SHPREAMBLE(ic);
        IEEE80211_DISABLE_BARKER(ic);
    } else {
        IEEE80211_DISABLE_SHPREAMBLE(ic);
        IEEE80211_ENABLE_BARKER(ic);
    }
#undef IS_11G
}

/*
 * Set the short slot time state and notify the driver.
 */
void
ieee80211_set_shortslottime(struct ieee80211com *ic, int onoff)
{
    if (onoff)
        IEEE80211_ENABLE_SHSLOT(ic);
    else
        IEEE80211_DISABLE_SHSLOT(ic);

    if (ic->ic_updateslot != NULL)
        ic->ic_updateslot(ic);
}

void
ieee80211_wme_initparams(struct ieee80211vap *vap)
{
    if (vap->iv_opmode == IEEE80211_M_HOSTAP) {
        /*
         * For AP, we need to synchronize with SWBA interrupt.
         * So we need to lock out interrupt.
         */
    //    OS_EXEC_INTSAFE(vap->iv_ic->ic_osdev, ieee80211_wme_initparams_locked, vap);
        ieee80211_wme_initparams_locked(vap);
    } else {
        ieee80211_wme_initparams_locked(vap);
    }
}

void
ieee80211_wme_initparams_locked(struct ieee80211vap *vap)
{
    struct ieee80211com *ic = vap->iv_ic;
    struct ieee80211_wme_state *wme = &ic->ic_wme;
    typedef struct phyParamType { 
        u_int8_t aifsn; 
        u_int8_t logcwmin;
        u_int8_t logcwmax; 
        u_int16_t txopLimit;
        u_int8_t acm;
    } paramType;
    enum ieee80211_phymode mode;
    const paramType *pPhyParam, *pBssPhyParam;

    static const struct phyParamType phyParamForAC_BE[IEEE80211_MODE_MAX] = {
        /* IEEE80211_MODE_AUTO           */ {          3,                4,                6,                  0,              0 },    
        /* IEEE80211_MODE_11A            */ {          3,                4,                6,                  0,              0 },
        /* IEEE80211_MODE_11B            */ {          3,                5,                7,                  0,              0 },
        /* IEEE80211_MODE_11G            */ {          3,                4,                6,                  0,              0 },
        /* IEEE80211_MODE_FH             */ {          3,                5,                7,                  0,              0 },
        /* IEEE80211_MODE_TURBO          */ {          2,                3,                5,                  0,              0 },
        /* IEEE80211_MODE_TURBO          */ {          2,                3,                5,                  0,              0 },
        /* IEEE80211_MODE_11NA_HT20      */ {          3,                4,                6,                  0,              0 }, 
        /* IEEE80211_MODE_11NG_HT20      */ {          3,                4,                6,                  0,              0 },
        /* IEEE80211_MODE_11NA_HT40PLUS  */ {          3,                4,                6,                  0,              0 }, 
        /* IEEE80211_MODE_11NA_HT40MINUS */ {          3,                4,                6,                  0,              0 },
        /* IEEE80211_MODE_11NG_HT40PLUS  */ {          3,                4,                6,                  0,              0 }, 
        /* IEEE80211_MODE_11NG_HT40MINUS */ {          3,                4,                6,                  0,              0 }};
    static const struct phyParamType phyParamForAC_BK[IEEE80211_MODE_MAX] = {
        /* IEEE80211_MODE_AUTO           */ {          7,                4,               10,                  0,              0 },
        /* IEEE80211_MODE_11A            */ {          7,                4,               10,                  0,              0 },
        /* IEEE80211_MODE_11B            */ {          7,                5,               10,                  0,              0 },
        /* IEEE80211_MODE_11G            */ {          7,                4,               10,                  0,              0 },
        /* IEEE80211_MODE_FH             */ {          7,                5,               10,                  0,              0 },
        /* IEEE80211_MODE_TURBO          */ {          7,                3,               10,                  0,              0 },
        /* IEEE80211_MODE_TURBO          */ {          7,                3,               10,                  0,              0 },
        /* IEEE80211_MODE_11NA_HT20      */ {          7,                4,               10,                  0,              0 }, 
        /* IEEE80211_MODE_11NG_HT20      */ {          7,                4,               10,                  0,              0 },
        /* IEEE80211_MODE_11NA_HT40PLUS  */ {          7,                4,               10,                  0,              0 }, 
        /* IEEE80211_MODE_11NA_HT40MINUS */ {          7,                4,               10,                  0,              0 },
        /* IEEE80211_MODE_11NG_HT40PLUS  */ {          7,                4,               10,                  0,              0 }, 
        /* IEEE80211_MODE_11NG_HT40MINUS */ {          7,                4,               10,                  0,              0 }};
    static const struct phyParamType phyParamForAC_VI[IEEE80211_MODE_MAX] = {
        /* IEEE80211_MODE_AUTO           */ {          1,                3,               4,                  94,              0 },
        /* IEEE80211_MODE_11A            */ {          1,                3,               4,                  94,              0 },
        /* IEEE80211_MODE_11B            */ {          1,                4,               5,                 188,              0 },
        /* IEEE80211_MODE_11G            */ {          1,                3,               4,                  94,              0 },
        /* IEEE80211_MODE_FH             */ {          1,                4,               5,                 188,              0 },
        /* IEEE80211_MODE_TURBO          */ {          1,                2,               3,                  94,              0 },
        /* IEEE80211_MODE_TURBO          */ {          1,                2,               3,                  94,              0 },
        /* IEEE80211_MODE_11NA_HT20      */ {          1,                3,               4,                  94,              0 }, 
        /* IEEE80211_MODE_11NG_HT20      */ {          1,                3,               4,                  94,              0 },
        /* IEEE80211_MODE_11NA_HT40PLUS  */ {          1,                3,               4,                  94,              0 }, 
        /* IEEE80211_MODE_11NA_HT40MINUS */ {          1,                3,               4,                  94,              0 },
        /* IEEE80211_MODE_11NG_HT40PLUS  */ {          1,                3,               4,                  94,              0 }, 
        /* IEEE80211_MODE_11NG_HT40MINUS */ {          1,                3,               4,                  94,              0 }};
    static const struct phyParamType phyParamForAC_VO[IEEE80211_MODE_MAX] = {
        /* IEEE80211_MODE_AUTO           */ {          1,                2,               3,                  47,              0 },
        /* IEEE80211_MODE_11A            */ {          1,                2,               3,                  47,              0 },
        /* IEEE80211_MODE_11B            */ {          1,                3,               4,                 102,              0 },
        /* IEEE80211_MODE_11G            */ {          1,                2,               3,                  47,              0 },
        /* IEEE80211_MODE_FH             */ {          1,                3,               4,                 102,              0 },
        /* IEEE80211_MODE_TURBO          */ {          1,                2,               2,                  47,              0 },
        /* IEEE80211_MODE_TURBO          */ {          1,                2,               2,                  47,              0 },
        /* IEEE80211_MODE_11NA_HT20      */ {          1,                2,               3,                  47,              0 }, 
        /* IEEE80211_MODE_11NG_HT20      */ {          1,                2,               3,                  47,              0 },
        /* IEEE80211_MODE_11NA_HT40PLUS  */ {          1,                2,               3,                  47,              0 }, 
        /* IEEE80211_MODE_11NA_HT40MINUS */ {          1,                2,               3,                  47,              0 },
        /* IEEE80211_MODE_11NG_HT40PLUS  */ {          1,                2,               3,                  47,              0 }, 
        /* IEEE80211_MODE_11NG_HT40MINUS */ {          1,                2,               3,                  47,              0 }};
    static const struct phyParamType bssPhyParamForAC_BE[IEEE80211_MODE_MAX] = {
        /* IEEE80211_MODE_AUTO           */ {          3,                4,              10,                   0,              0 },
        /* IEEE80211_MODE_11A            */ {          3,                4,              10,                   0,              0 },
        /* IEEE80211_MODE_11B            */ {          3,                5,              10,                   0,              0 },
        /* IEEE80211_MODE_11G            */ {          3,                4,              10,                   0,              0 },
        /* IEEE80211_MODE_FH             */ {          3,                5,              10,                   0,              0 },
        /* IEEE80211_MODE_TURBO          */ {          2,                3,              10,                   0,              0 },
        /* IEEE80211_MODE_TURBO          */ {          2,                3,              10,                   0,              0 },
        /* IEEE80211_MODE_11NA_HT20      */ {          3,                4,              10,                   0,              0 }, 
        /* IEEE80211_MODE_11NG_HT20      */ {          3,                4,              10,                   0,              0 },
        /* IEEE80211_MODE_11NA_HT40PLUS  */ {          3,                4,              10,                   0,              0 }, 
        /* IEEE80211_MODE_11NA_HT40MINUS */ {          3,                4,              10,                   0,              0 },
        /* IEEE80211_MODE_11NG_HT40PLUS  */ {          3,                4,              10,                   0,              0 }, 
        /* IEEE80211_MODE_11NG_HT40MINUS */ {          3,                4,              10,                   0,              0 }};
    static const struct phyParamType bssPhyParamForAC_VI[IEEE80211_MODE_MAX] = {
        /* IEEE80211_MODE_AUTO           */ {          2,                3,               4,                  94,              0 },
        /* IEEE80211_MODE_11A            */ {          2,                3,               4,                  94,              0 },
        /* IEEE80211_MODE_11B            */ {          2,                4,               5,                 188,              0 },
        /* IEEE80211_MODE_11G            */ {          2,                3,               4,                  94,              0 },
        /* IEEE80211_MODE_FH             */ {          2,                4,               5,                 188,              0 },
        /* IEEE80211_MODE_TURBO          */ {          2,                2,               3,                  94,              0 },
        /* IEEE80211_MODE_TURBO          */ {          2,                2,               3,                  94,              0 },
        /* IEEE80211_MODE_11NA_HT20      */ {          2,                3,               4,                  94,              0 }, 
        /* IEEE80211_MODE_11NG_HT20      */ {          2,                3,               4,                  94,              0 },
        /* IEEE80211_MODE_11NA_HT40PLUS  */ {          2,                3,               4,                  94,              0 }, 
        /* IEEE80211_MODE_11NA_HT40MINUS */ {          2,                3,               4,                  94,              0 },
        /* IEEE80211_MODE_11NG_HT40PLUS  */ {          2,                3,               4,                  94,              0 }, 
        /* IEEE80211_MODE_11NG_HT40MINUS */ {          2,                3,               4,                  94,              0 }};
    static const struct phyParamType bssPhyParamForAC_VO[IEEE80211_MODE_MAX] = {
        /* IEEE80211_MODE_AUTO           */ {          2,                2,               3,                  47,              0 },    
        /* IEEE80211_MODE_11A            */ {          2,                2,               3,                  47,              0 },
        /* IEEE80211_MODE_11B            */ {          2,                3,               4,                 102,              0 },
        /* IEEE80211_MODE_11G            */ {          2,                2,               3,                  47,              0 },
        /* IEEE80211_MODE_FH             */ {          2,                3,               4,                 102,              0 },
        /* IEEE80211_MODE_TURBO          */ {          1,                2,               2,                  47,              0 },
        /* IEEE80211_MODE_TURBO          */ {          1,                2,               2,                  47,              0 },
        /* IEEE80211_MODE_11NA_HT20      */ {          2,                2,               3,                  47,              0 }, 
        /* IEEE80211_MODE_11NG_HT20      */ {          2,                2,               3,                  47,              0 },
        /* IEEE80211_MODE_11NA_HT40PLUS  */ {          2,                2,               3,                  47,              0 }, 
        /* IEEE80211_MODE_11NA_HT40MINUS */ {          2,                2,               3,                  47,              0 },
        /* IEEE80211_MODE_11NG_HT40PLUS  */ {          2,                2,               3,                  47,              0 }, 
        /* IEEE80211_MODE_11NG_HT40MINUS */ {          2,                2,               3,                  47,              0 }};

    int i;

    //IEEE80211_BEACON_LOCK_ASSERT(ic);

    if ((ic->ic_caps & IEEE80211_C_WME) == 0)
        return;

    /*
     * Select mode; we can be called early in which case we
     * always use auto mode.  We know we'll be called when
     * entering the RUN state with bsschan setup properly
     * so state will eventually get set correctly
     */
    if (vap->iv_bsschan != IEEE80211_CHAN_ANYC)
        mode = ieee80211_chan2mode(vap->iv_bsschan);
    else
        mode = IEEE80211_MODE_AUTO;
    for (i = 0; i < WME_NUM_AC; i++) {
        switch (i) {
        case WME_AC_BK:
            pPhyParam = &phyParamForAC_BK[mode];
            pBssPhyParam = &phyParamForAC_BK[mode];
            break;
        case WME_AC_VI:
            pPhyParam = &phyParamForAC_VI[mode];
            pBssPhyParam = &bssPhyParamForAC_VI[mode];
            break;
        case WME_AC_VO:
            pPhyParam = &phyParamForAC_VO[mode];
            pBssPhyParam = &bssPhyParamForAC_VO[mode];
            break;
        case WME_AC_BE:
        default:
            pPhyParam = &phyParamForAC_BE[mode];        
            pBssPhyParam = &bssPhyParamForAC_BE[mode];
            break;
        }

        if (vap->iv_opmode == IEEE80211_M_HOSTAP) {
            wme->wme_wmeChanParams.cap_wmeParams[i].wmep_acm = pPhyParam->acm;
            wme->wme_wmeChanParams.cap_wmeParams[i].wmep_aifsn = pPhyParam->aifsn;  
            wme->wme_wmeChanParams.cap_wmeParams[i].wmep_logcwmin = pPhyParam->logcwmin;    
            wme->wme_wmeChanParams.cap_wmeParams[i].wmep_logcwmax = pPhyParam->logcwmax;        
            wme->wme_wmeChanParams.cap_wmeParams[i].wmep_txopLimit = pPhyParam->txopLimit;
        } else {
            wme->wme_wmeChanParams.cap_wmeParams[i].wmep_acm = pBssPhyParam->acm;
            wme->wme_wmeChanParams.cap_wmeParams[i].wmep_aifsn = pBssPhyParam->aifsn;   
            wme->wme_wmeChanParams.cap_wmeParams[i].wmep_logcwmin = pBssPhyParam->logcwmin; 
            wme->wme_wmeChanParams.cap_wmeParams[i].wmep_logcwmax = pBssPhyParam->logcwmax;     
            wme->wme_wmeChanParams.cap_wmeParams[i].wmep_txopLimit = pBssPhyParam->txopLimit;
        }
        wme->wme_wmeBssChanParams.cap_wmeParams[i].wmep_acm = pBssPhyParam->acm;
        wme->wme_wmeBssChanParams.cap_wmeParams[i].wmep_aifsn = pBssPhyParam->aifsn;    
        wme->wme_wmeBssChanParams.cap_wmeParams[i].wmep_logcwmin = pBssPhyParam->logcwmin;  
        wme->wme_wmeBssChanParams.cap_wmeParams[i].wmep_logcwmax = pBssPhyParam->logcwmax;      
        wme->wme_wmeBssChanParams.cap_wmeParams[i].wmep_txopLimit = pBssPhyParam->txopLimit;
    }
    /* NB: check ic_bss to avoid NULL deref on initial attach */
    if (vap->iv_bss != NULL) {
        /*
         * Calculate agressive mode switching threshold based
         * on beacon interval.
         */
        wme->wme_hipri_switch_thresh =
            (HIGH_PRI_SWITCH_THRESH * ieee80211_node_get_beacon_interval(vap->iv_bss)) / 100;
        ieee80211_wme_updateparams_locked(vap);
    }
}

static void
ieee80211_vap_iter_check_burst(void *arg, wlan_if_t vap) 
{
    u_int8_t *pburstEnabled= (u_int8_t *) arg;
    if (vap->iv_ath_cap & IEEE80211_ATHC_BURST) {
        *pburstEnabled=1;
    }

}

void
ieee80211_wme_amp_overloadparams_locked(struct ieee80211com *ic)
{
    struct ieee80211_wme_state *wme = &ic->ic_wme;

    wme->wme_chanParams.cap_wmeParams[WME_AC_BE].wmep_txopLimit 
        = ic->ic_reg_parm.ampTxopLimit;
    wme->wme_bssChanParams.cap_wmeParams[WME_AC_BE].wmep_txopLimit 
        = ic->ic_reg_parm.ampTxopLimit;

    wme->wme_update(ic);
}

/*
 * Update WME parameters for ourself and the BSS.
 */
void
ieee80211_wme_updateparams_locked(struct ieee80211vap *vap)
{
    static const struct { u_int8_t aifsn; u_int8_t logcwmin; u_int8_t logcwmax; u_int16_t txopLimit;}
    phyParam[IEEE80211_MODE_MAX] = {
        /* IEEE80211_MODE_AUTO           */ {          2,                4,               10,           64 },   
        /* IEEE80211_MODE_11A            */ {          2,                4,               10,           64 },
        /* IEEE80211_MODE_11B            */ {          2,                5,               10,           64 },
        /* IEEE80211_MODE_11G            */ {          2,                4,               10,           64 },
        /* IEEE80211_MODE_FH             */ {          2,                5,               10,           64 },
        /* IEEE80211_MODE_TURBO          */ {          1,                3,               10,           64 },
        /* IEEE80211_MODE_11NA_HT20      */ {          2,                4,               10,           64 }, 
        /* IEEE80211_MODE_11NG_HT20      */ {          2,                4,               10,           64 },
        /* IEEE80211_MODE_11NA_HT40PLUS  */ {          2,                4,               10,           64 }, 
        /* IEEE80211_MODE_11NA_HT40MINUS */ {          2,                4,               10,           64 },
        /* IEEE80211_MODE_11NG_HT40PLUS  */ {          2,                4,               10,           64 }, 
        /* IEEE80211_MODE_11NG_HT40MINUS */ {          2,                4,               10,           64 }};
    struct ieee80211com *ic = vap->iv_ic;
    struct ieee80211_wme_state *wme = &ic->ic_wme;
    enum ieee80211_phymode mode;
    int i;

    /* set up the channel access parameters for the physical device */
    for (i = 0; i < WME_NUM_AC; i++) {
        wme->wme_chanParams.cap_wmeParams[i].wmep_acm 
            = wme->wme_wmeChanParams.cap_wmeParams[i].wmep_acm;        
        wme->wme_chanParams.cap_wmeParams[i].wmep_aifsn 
            = wme->wme_wmeChanParams.cap_wmeParams[i].wmep_aifsn;
        wme->wme_chanParams.cap_wmeParams[i].wmep_logcwmin 
            = wme->wme_wmeChanParams.cap_wmeParams[i].wmep_logcwmin;
        wme->wme_chanParams.cap_wmeParams[i].wmep_logcwmax 
            = wme->wme_wmeChanParams.cap_wmeParams[i].wmep_logcwmax;
        wme->wme_chanParams.cap_wmeParams[i].wmep_txopLimit 
            = wme->wme_wmeChanParams.cap_wmeParams[i].wmep_txopLimit;
        wme->wme_bssChanParams.cap_wmeParams[i].wmep_acm 
            = wme->wme_wmeBssChanParams.cap_wmeParams[i].wmep_acm;            
        wme->wme_bssChanParams.cap_wmeParams[i].wmep_aifsn 
            = wme->wme_wmeBssChanParams.cap_wmeParams[i].wmep_aifsn;
        wme->wme_bssChanParams.cap_wmeParams[i].wmep_logcwmin 
            = wme->wme_wmeBssChanParams.cap_wmeParams[i].wmep_logcwmin;
        wme->wme_bssChanParams.cap_wmeParams[i].wmep_logcwmax 
            = wme->wme_wmeBssChanParams.cap_wmeParams[i].wmep_logcwmax;
        wme->wme_bssChanParams.cap_wmeParams[i].wmep_txopLimit 
            = wme->wme_wmeBssChanParams.cap_wmeParams[i].wmep_txopLimit;
    }

    /*
     * Select mode; we can be called early in which case we
     * always use auto mode.  We know we'll be called when
     * entering the RUN state with bsschan setup properly
     * so state will eventually get set correctly
     */
    if (vap->iv_bsschan != IEEE80211_CHAN_ANYC)
        mode = ieee80211_chan2mode(vap->iv_bsschan);
    else
        mode = IEEE80211_MODE_AUTO;

    if ((vap->iv_opmode == IEEE80211_M_HOSTAP &&
        ((wme->wme_flags & WME_F_AGGRMODE) != 0)) ||
        ((vap->iv_opmode == IEEE80211_M_STA || vap->iv_opmode == IEEE80211_M_IBSS) &&
         (vap->iv_bss->ni_flags & IEEE80211_NODE_QOS) == 0) ||
         ieee80211_vap_wme_is_clear(vap)) {
        u_int8_t burstEnabled=0;
        /* check if bursting  enabled on at least one vap */
        wlan_iterate_vap_list(ic,ieee80211_vap_iter_check_burst,(void *) &burstEnabled);
        wme->wme_chanParams.cap_wmeParams[WME_AC_BE].wmep_aifsn = phyParam[mode].aifsn;
        wme->wme_chanParams.cap_wmeParams[WME_AC_BE].wmep_logcwmin = phyParam[mode].logcwmin;
        wme->wme_chanParams.cap_wmeParams[WME_AC_BE].wmep_logcwmax = phyParam[mode].logcwmax;       
        wme->wme_chanParams.cap_wmeParams[WME_AC_BE].wmep_txopLimit 
            = burstEnabled ? phyParam[mode].txopLimit : 0;
        wme->wme_bssChanParams.cap_wmeParams[WME_AC_BE].wmep_aifsn = phyParam[mode].aifsn;
        wme->wme_bssChanParams.cap_wmeParams[WME_AC_BE].wmep_logcwmin = phyParam[mode].logcwmin;
        wme->wme_bssChanParams.cap_wmeParams[WME_AC_BE].wmep_logcwmax = phyParam[mode].logcwmax;
        wme->wme_bssChanParams.cap_wmeParams[WME_AC_BE].wmep_txopLimit 
            = burstEnabled ? phyParam[mode].txopLimit : 0;      
    }
    
    if (ic->ic_opmode == IEEE80211_M_HOSTAP &&
        ic->ic_sta_assoc < 2 &&
        (wme->wme_flags & WME_F_AGGRMODE) != 0) {
        static const u_int8_t logCwMin[IEEE80211_MODE_MAX] = {
            /* IEEE80211_MODE_AUTO           */   3,     
            /* IEEE80211_MODE_11A            */   3,
            /* IEEE80211_MODE_11B            */   4,
            /* IEEE80211_MODE_11G            */   3,
            /* IEEE80211_MODE_FH             */   4, 
            /* IEEE80211_MODE_TURBO_A        */   3,
            /* IEEE80211_MODE_TURBO_G        */   3,
            /* IEEE80211_MODE_11NA_HT20      */   3, 
            /* IEEE80211_MODE_11NG_HT20      */   3,
            /* IEEE80211_MODE_11NA_HT40PLUS  */   3, 
            /* IEEE80211_MODE_11NA_HT40MINUS */   3,
            /* IEEE80211_MODE_11NG_HT40PLUS  */   3, 
            /* IEEE80211_MODE_11NG_HT40MINUS */   3
        };

        wme->wme_chanParams.cap_wmeParams[WME_AC_BE].wmep_logcwmin 
            = wme->wme_bssChanParams.cap_wmeParams[WME_AC_BE].wmep_logcwmin 
            = logCwMin[mode];
    }
    if (vap->iv_opmode == IEEE80211_M_HOSTAP) {   /* XXX ibss? */
        u_int8_t    temp_cap_info   = wme->wme_bssChanParams.cap_info;
        u_int8_t    temp_info_count = temp_cap_info & WME_QOSINFO_COUNT;
        /*
         * Arrange for a beacon update and bump the parameter
         * set number so associated stations load the new values.
         */
        wme->wme_bssChanParams.cap_info = 
            (temp_cap_info & WME_QOSINFO_UAPSD) |
            ((temp_info_count + 1) & WME_QOSINFO_COUNT);
        vap->iv_flags |= IEEE80211_F_WMEUPDATE;
    }

    /* Override txOpLimit for video stream for now.
     * else WMM failures are observed in 11N testbed
     * Refer to bug #22594 and 31082
     */
    if (vap->iv_opmode == IEEE80211_M_STA) {
        if (VAP_NEED_CWMIN_WORKAROUND(vap)) {
             /* For UB9x, override CWmin for video stream for now
             * else WMM failures are observed in 11N testbed
             * Refer to bug #70038 ,#70039 and #70041
             */
            if (wme->wme_chanParams.cap_wmeParams[WME_AC_VI].wmep_logcwmin == 3) {
                wme->wme_chanParams.cap_wmeParams[WME_AC_VI].wmep_logcwmin = 4;
            }
        }
        wme->wme_chanParams.cap_wmeParams[WME_AC_VI].wmep_txopLimit = 0;
    }
    
#if ATH_SUPPORT_IBSS_WMM  
    if (vap->iv_opmode == IEEE80211_M_IBSS) {
        wme->wme_chanParams.cap_wmeParams[WME_AC_BE].wmep_txopLimit = 0;
        wme->wme_chanParams.cap_wmeParams[WME_AC_VI].wmep_txopLimit = 0;
    }    
#endif    
                                                       
    wme->wme_update(ic);

    IEEE80211_DPRINTF(vap, IEEE80211_MSG_WME,
                      "%s: WME params updated, cap_info 0x%x\n", __func__,
                      vap->iv_opmode == IEEE80211_M_STA ?
                      wme->wme_wmeChanParams.cap_info :
                      wme->wme_bssChanParams.cap_info);
}

/*
 * Update WME parameters for ourself and the BSS.
 */
void
ieee80211_wme_updateinfo_locked(struct ieee80211vap *vap)
{
    struct ieee80211com *ic = vap->iv_ic;
    struct ieee80211_wme_state *wme = &ic->ic_wme;

    if (vap->iv_opmode == IEEE80211_M_HOSTAP) {   /* XXX ibss? */
        u_int8_t    temp_cap_info   = wme->wme_bssChanParams.cap_info;
        u_int8_t    temp_info_count = temp_cap_info & WME_QOSINFO_COUNT;
        /*
         * Arrange for a beacon update and bump the parameter
         * set number so associated stations load the new values.
         */
        wme->wme_bssChanParams.cap_info = 
            (temp_cap_info & WME_QOSINFO_UAPSD) |
            ((temp_info_count + 1) & WME_QOSINFO_COUNT);
        vap->iv_flags |= IEEE80211_F_WMEUPDATE;
    }

    wme->wme_update(ic);

    IEEE80211_DPRINTF(vap, IEEE80211_MSG_WME,
                      "%s: WME info updated, cap_info 0x%x\n", __func__,
                      vap->iv_opmode == IEEE80211_M_STA ?
                      wme->wme_wmeChanParams.cap_info :
                      wme->wme_bssChanParams.cap_info);
}

void
ieee80211_wme_updateparams(struct ieee80211vap *vap)
{
    struct ieee80211com *ic = vap->iv_ic;
    
    if (ic->ic_caps & IEEE80211_C_WME) {
        if (vap->iv_opmode == IEEE80211_M_HOSTAP) {
            /*
             * For AP, we need to synchronize with SWBA interrupt.
             * So we need to lock out interrupt.
             */
            OS_EXEC_INTSAFE(ic->ic_osdev, ieee80211_wme_updateparams_locked, vap);
        } else {
            ieee80211_wme_updateparams_locked(vap);
        }
    }
}

void
ieee80211_wme_updateinfo(struct ieee80211vap *vap)
{
    struct ieee80211com *ic = vap->iv_ic;
    
    if (ic->ic_caps & IEEE80211_C_WME) {
        if (vap->iv_opmode == IEEE80211_M_HOSTAP) {
            /*
             * For AP, we need to synchronize with SWBA interrupt.
             * So we need to lock out interrupt.
             */
            OS_EXEC_INTSAFE(ic->ic_osdev, ieee80211_wme_updateinfo_locked, vap);
        } else {
            ieee80211_wme_updateinfo_locked(vap);
        }
    }
}

void
ieee80211_dump_pkt(struct ieee80211com *ic,
                   const u_int8_t *buf, int len, int rate, int rssi)
{
    const struct ieee80211_frame *wh;
    int type, subtype;
    int i;

    printk("%8p (%lu): \n", buf, (unsigned long) OS_GET_TIMESTAMP());
    
    wh = (const struct ieee80211_frame *)buf;
    type = wh->i_fc[0] & IEEE80211_FC0_TYPE_MASK;
    subtype = (wh->i_fc[0] & IEEE80211_FC0_SUBTYPE_MASK) >> IEEE80211_FC0_SUBTYPE_SHIFT;
    
    switch (wh->i_fc[1] & IEEE80211_FC1_DIR_MASK) {
    case IEEE80211_FC1_DIR_NODS:
        printk("NODS %s", ether_sprintf(wh->i_addr2));
        printk("->%s", ether_sprintf(wh->i_addr1));
        printk("(%s)", ether_sprintf(wh->i_addr3));
        break;
    case IEEE80211_FC1_DIR_TODS:
        printk("TODS %s", ether_sprintf(wh->i_addr2));
        printk("->%s", ether_sprintf(wh->i_addr3));
        printk("(%s)", ether_sprintf(wh->i_addr1));
        break;
    case IEEE80211_FC1_DIR_FROMDS:
        printk("FRDS %s", ether_sprintf(wh->i_addr3));
        printk("->%s", ether_sprintf(wh->i_addr1));
        printk("(%s)", ether_sprintf(wh->i_addr2));
        break;
    case IEEE80211_FC1_DIR_DSTODS:
        printk("DSDS %s", ether_sprintf((u_int8_t *)&wh[1]));
        printk("->%s", ether_sprintf(wh->i_addr3));
        printk("(%s", ether_sprintf(wh->i_addr2));
        printk("->%s)", ether_sprintf(wh->i_addr1));
        break;
    }
    switch (type) {
    case IEEE80211_FC0_TYPE_DATA:
        printk(" data");
        break;
    case IEEE80211_FC0_TYPE_MGT:
        printk(" %s", ieee80211_mgt_subtype_name[subtype]);
        break;
    default:
        printk(" type#%d", type);
        break;
    }
    if (IEEE80211_QOS_HAS_SEQ(wh)) {
        const struct ieee80211_qosframe *qwh = 
            (const struct ieee80211_qosframe *)buf;
        printk(" QoS [TID %u%s]", qwh->i_qos[0] & IEEE80211_QOS_TID,
               qwh->i_qos[0] & IEEE80211_QOS_ACKPOLICY ? " ACM" : "");
    }

    if (wh->i_fc[1] & IEEE80211_FC1_WEP) {
        int off;

        off = ieee80211_anyhdrspace(ic, wh);
        printk(" WEP [IV %.02x %.02x %.02x",
               buf[off+0], buf[off+1], buf[off+2]);
        if (buf[off+IEEE80211_WEP_IVLEN] & IEEE80211_WEP_EXTIV)
            printk(" %.02x %.02x %.02x",
                   buf[off+4], buf[off+5], buf[off+6]);
        printk(" KID %u]", buf[off+IEEE80211_WEP_IVLEN] >> 6);
    }
    if (rate >= 0)
        printk(" %dM", rate / 1000);
    if (rssi >= 0)
        printk(" +%d", rssi);
    printk("\n");
    if (len > 0) {
        for (i = 0; i < len; i++) {
            if ((i & 1) == 0)
                printk(" ");
            printk("%02x", buf[i]);
        }
        printk("\n");
    }
}
/*zhaoyang modify for add CL101673.fix.patch for dusi*/
void
ieee80211_dump_pkt_debug(const u_int8_t *buf)
{
	const struct ieee80211_frame *wh;
	int type, subtype;

	printk("%8p (%lu): \n", buf, (unsigned long) OS_GET_TIMESTAMP());

	wh = (const struct ieee80211_frame *)buf;
	type = wh->i_fc[0] & IEEE80211_FC0_TYPE_MASK;
	subtype = (wh->i_fc[0] & IEEE80211_FC0_SUBTYPE_MASK) >> IEEE80211_FC0_SUBTYPE_SHIFT;

	switch (wh->i_fc[1] & IEEE80211_FC1_DIR_MASK) {
		case IEEE80211_FC1_DIR_NODS:
			 printk("NODS %s", ether_sprintf(wh->i_addr2));
			 printk("->%s", ether_sprintf(wh->i_addr1));
			 printk("(%s)", ether_sprintf(wh->i_addr3));
			 break;
		case IEEE80211_FC1_DIR_TODS:
			printk("TODS %s", ether_sprintf(wh->i_addr2));
			printk("->%s", ether_sprintf(wh->i_addr3));
			printk("(%s)", ether_sprintf(wh->i_addr1));
			break;
		case IEEE80211_FC1_DIR_FROMDS:
			printk("FRDS %s", ether_sprintf(wh->i_addr3));
			printk("->%s", ether_sprintf(wh->i_addr1));
			printk("(%s)", ether_sprintf(wh->i_addr2));
			break;
		case IEEE80211_FC1_DIR_DSTODS:
			printk("DSDS %s", ether_sprintf((u_int8_t *)&wh[1]));
			printk("->%s", ether_sprintf(wh->i_addr3));
			printk("(%s", ether_sprintf(wh->i_addr2));
			printk("->%s)", ether_sprintf(wh->i_addr1));
			break;
	}
	switch (type) {
		case IEEE80211_FC0_TYPE_DATA:
			 printk(" data");
			 break;
		case IEEE80211_FC0_TYPE_MGT:
			 printk(" %s", ieee80211_mgt_subtype_name[subtype]);
			 break;
	     default:
		 	printk(" type#%d", type);
			break;
	}
	if (IEEE80211_QOS_HAS_SEQ(wh)) {
		const struct ieee80211_qosframe *qwh = 
			(const struct ieee80211_qosframe *)buf;
		printk(" QoS [TID %u%s]", qwh->i_qos[0] & IEEE80211_QOS_TID,
			qwh->i_qos[0] & IEEE80211_QOS_ACKPOLICY ? " ACM" : "");
		}
	}
/*zhaoyang modify end*/
static void
get_ht20_only(void *arg, struct ieee80211_node *ni)
{
    u_int32_t *num_sta = arg;

    if (!ni->ni_associd)
        return;

    if ((ni->ni_flags & IEEE80211_NODE_HT) && 
        ((ni->ni_flags & IEEE80211_NODE_40_INTOLERANT) ||
         (ni->ni_flags & IEEE80211_NODE_REQ_HT20))) {
        (*num_sta)++;
    }
}

void
ieee80211_change_cw(struct ieee80211com *ic)
{
    enum ieee80211_cwm_width cw_width = ic->ic_cwm_get_width(ic);
    u_int32_t num_ht20_only = 0;

    if (ic->ic_flags & IEEE80211_F_COEXT_DISABLE)
        return;

    ieee80211_iterate_node(ic, get_ht20_only, &num_ht20_only);

    if (cw_width == IEEE80211_CWM_WIDTH40) {
        if (num_ht20_only) {
            ic->ic_bss_to20(ic);
        }
    } else if (cw_width == IEEE80211_CWM_WIDTH20) {
        if (num_ht20_only == 0) {
            ic->ic_bss_to40(ic);
        }
    }
}
