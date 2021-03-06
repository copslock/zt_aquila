/*
 * Copyright (c) 2010, Atheros Communications Inc.
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

#include "ieee80211_node_priv.h"

#if UMAC_SUPPORT_STA

/* should this be configurable ?*/
#define IEE80211_STA_MAX_NODE_SAVEQ_LEN 300
/*
 * Join an infrastructure network
 */
int
ieee80211_sta_join(struct ieee80211vap *vap, ieee80211_scan_entry_t scan_entry)
{
    struct ieee80211com *ic = vap->iv_ic;
    struct ieee80211_node_table *nt = &ic->ic_sta;
    struct ieee80211_node *ni = NULL;
    const u_int8_t *macaddr = ieee80211_scan_entry_macaddr(scan_entry);
    int error = 0;

    ASSERT(vap->iv_opmode == IEEE80211_M_STA);
    
    ni = ieee80211_find_node(nt, macaddr);
    if (ni) {
        /* 
         * reusing old node has a potential for several bugs . The old node may have some state info from previous association.
         * get rid of the old bss node and create a new bss node.
         */
        ieee80211_sta_leave(ni); 
        ieee80211_free_node(ni); 
    }
    /*
     * Create a BSS node.
     */
    ni = ieee80211_alloc_node(nt, vap, macaddr);
    if (ni == NULL)
        return -ENOMEM;
    /* set the maximum number frmaes to be queued when the vap is in fake sleep */        
    ieee80211_node_saveq_set_param(ni,IEEE80211_NODE_SAVEQ_DATA_Q_LEN,IEE80211_STA_MAX_NODE_SAVEQ_LEN);
    /* To become a bss node, a node need an extra reference count, which alloc node already gives */
#ifdef IEEE80211_DEBUG_REFCNT
    ieee80211_note(ni->ni_vap,"%s ,line %u: increase node %p <%s> refcnt to %d\n",
                   __func__, __LINE__, ni, ether_sprintf(ni->ni_macaddr),
                   ieee80211_node_refcnt(ni));
#endif

    /* setup the bss node for association */
    error = ieee80211_setup_node(ni, scan_entry);
    if (error != 0) {
        ieee80211_free_node(ni);
        return error;
    }

    /* copy the beacon timestamp */
    OS_MEMCPY(ni->ni_tstamp.data,
              ieee80211_scan_entry_tsf(scan_entry),
              sizeof(ni->ni_tstamp));

    /*
     * Join the BSS represented by this new node.
     * This function will free up the old BSS node
     * and use this one as the new BSS node.
     */
    ieee80211_sta_join_bss(ni);

    IEEE80211_ADD_NODE_TARGET(ni, ni->ni_vap, 0);

    /* Save our home channel */
    vap->iv_bsschan = ni->ni_chan;
    vap->iv_cur_mode = ieee80211_chan2mode(ni->ni_chan);

    /* Update the DotH falg */
    ieee80211_update_spectrumrequirement(vap);

    /*
     *  The OS will control our security keys.  
     *  If clear, keys will be cleared.
     *  If static WEP, keys will be plumbed before JoinInfra.
     *  If WPA/WPA2, ciphers will be setup, but no keys will be plumbed until 
     *    after they are negotiated.
     *  XXX We should ASSERT that all of the foregoing is true.
     */
    return 0;
}

#endif
