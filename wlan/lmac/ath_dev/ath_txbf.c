/*
 * Copyright (c) 2009, Atheros Communications Inc.
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
 *
 *  Implementation of receive path in atheros OS-independent layer.
 */

/*****************************************************************************/
/*! \file ath_txbf.c
**  \brief ATH Transmit Beamforming
**
**  This file contains the functionality for transmit beamforming
**  in the ATH object.
**
**  Copyright (c) 2005 Atheros Communications Inc.  All rights reserved.
**
*/

#include "ath_internal.h"
#include "if_athrate.h"
#include "ratectrl.h"

#ifdef ATH_SUPPORT_TxBF
#ifdef TXBF_TODO
void
ath_rx_get_pos2_data(ath_dev_t dev, u_int8_t **p_data, u_int16_t* p_len, void **rx_status)
{
	struct ath_softc *sc = ATH_DEV_TO_SC(dev);

	*p_data	= sc->pos2_data;
	*p_len	= sc->pos2_data_len;
	*rx_status = &(sc->pos2_rx_status);
	sc->pos2_data = NULL;	
}

bool 
ath_rx_txbfrcupdate(ath_dev_t dev, void *rx_status, u_int8_t *local_h, u_int8_t *CSIFrame, 
    u_int8_t NESSA, u_int8_t NESSB, int BW) 
{
	struct ath_softc *sc = ATH_DEV_TO_SC(dev);
	
	return ath_hal_TxBFRCUpdate(sc->sc_ah, (struct ath_rx_status *)rx_status, 
        local_h, CSIFrame, 2, 2, BW);  // should check parameters
}

void
ath_ap_save_join_mac(ath_dev_t dev, u_int8_t *join_macaddr)
{
	struct ath_softc *sc = ATH_DEV_TO_SC(dev);
	DPRINTF(sc, ATH_DEBUG_ANY,"Joined STA 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x",
        join_macaddr[0], join_macaddr[1], join_macaddr[2], join_macaddr[3], 
        join_macaddr[4], join_macaddr[5]);
	OS_MEMCPY(sc->join_macaddr, join_macaddr,6);
}

void
ath_start_imbf_cal(ath_dev_t dev)
{
	struct ath_softc *sc = ATH_DEV_TO_SC(dev);
	
	DPRINTF(sc, ATH_DEBUG_ANY,"==>%s", __func__);
	sc->radio_cal_process = 0;
	ath_set_timer_period(&sc->sc_imbf_cal_short, 10);
	ath_start_timer(&sc->sc_imbf_cal_short);

}
#endif
/*
 * Get TxBF capabilities from the HAL
 */
int
ath_get_txbfcaps(ath_dev_t dev, ieee80211_txbf_caps_t **txbf_caps)
{
    struct ath_softc *sc = (struct ath_softc *)dev;

    if (sc->sc_txbfsupport) {
        *txbf_caps = (ieee80211_txbf_caps_t *)(ath_hal_gettxbfcapability(sc->sc_ah));
        return AH_TRUE;
    } else {
        return AH_FALSE;
    }
}

HAL_BOOL
ath_txbf_alloc_key(ath_dev_t dev, u_int8_t *mac, u_int16_t *keyix)
{
    struct ath_softc *sc = ATH_DEV_TO_SC(dev);
    struct ath_hal *ah = sc->sc_ah;
    u_int i, j;
    u_int8_t keymac[6], status = AH_TRUE;

    /* search exist key first, if mac address match alloc this one */
    for (i = 0; i < ATH_KEYMAX; i++) {

        if (isset(sc->sc_keymap,i)){
            status=ath_hal_keyreadmac(ah, (u_int16_t)i, keymac);
            if (status == AH_FALSE){
                return status;
            }
            /*DPRINTF(sc, ATH_DEBUG_ANY,"==>%s:match mac address:\n",__func__);
            for (j = 0; j < 6; j++){
                DPRINTF(sc, ATH_DEBUG_ANY,"mac %x keymac %x\n",mac[j],keymac[j]);
            }*/
            for (j = 0; j < 6; j++){
                if (mac[j]!= keymac[j]){
                    break;
                }
            }
            if (j == 6){       //mac address matched
                *keyix = i;
                goto success;
            }   
        }
    }
    

#define    N(a)    (sizeof(a)/sizeof(a[0]))

  /* XXX try i,i+32,i+64,i+32+64 to minimize key pair conflicts */
    for (i = 0; i < N(sc->sc_keymap); i++) {
       u_int8_t b = sc->sc_keymap[i];
        if (b != 0xff) {
            /*
             * One or more slots are free.
             */
            *keyix = i*NBBY;
            while (b & 1) {
                (*keyix)++, b >>= 1;
            }
            setbit(sc->sc_keymap, *keyix);
            goto success;
        }
    }
    return AH_FALSE;
success:
    ath_hal_keysetmac(ah, *keyix, mac);  // set mac address into keycache.
    return AH_TRUE;
#undef N
}

void ath_txbf_set_key (ath_dev_t dev,u_int16_t keyidx,u_int8_t rx_staggered_sounding,u_int8_t channel_estimation_cap
                ,u_int8_t MMSS)
{
    struct ath_softc *sc = ATH_DEV_TO_SC(dev);
    struct ath_hal *ah = sc->sc_ah;

    ath_hal_TxBFSetKey(ah,keyidx,rx_staggered_sounding,channel_estimation_cap,MMSS);
    //DPRINTF(sc, ATH_DEBUG_ANY,"==>%s:keyidx %d, staggered sounding %d,CEC %d,MMSS %d\n",__func__,keyidx,rx_staggered_sounding
    //        ,channel_estimation_cap,MMSS);
}

void 
ath_txbf_set_hw_cvtimeout(ath_dev_t dev, HAL_BOOL opt)
{
    struct ath_softc *sc = ATH_DEV_TO_SC(dev);
    struct ath_hal *ah = sc->sc_ah;
    
    /* when true, it use H/W timer directly 
     * when false, set H/W timer to 1 ms to trigger S/W timer*/
	if (opt == AH_FALSE){
	    /* set hw timer to 1 ms*/
        ah->ah_txbf_hw_cvtimeout = ONE_MS;          
        ath_hal_setHwCvTimeout(ah, AH_FALSE);
    } else {
        /* use H/W timer, don't change timer value here*/
        ath_hal_setHwCvTimeout(ah, AH_TRUE);
	}
}

#ifdef DBG_TXBF
/* this fuction is used a pre-defined method to read value in CV cache. */ 
static u_int32_t
ath_txbf_read_cvcache(struct ath_hal *ah, u_int32_t addr)
{
    u_int32_t tmp,value;
    
#define AR_TXBF_DBG 0x10000
#define AR_TXBF_SW 0x1000c
#define AR_CVCACHE_0 0x12400
    
    tmp=ath_hal_readRegister(ah, AR_TXBF_SW);
    ath_hal_writeRegister(ah, AR_TXBF_SW, tmp);

    ath_hal_writeRegister(ah, addr, 0x40000000);
    do {
        tmp=ath_hal_readRegister(ah, AR_TXBF_SW);
    }   while ((tmp & 0x100000)==0);
    
    value=ath_hal_readRegister(ah, addr);
    tmp &= ~(0x100000);
    ath_hal_writeRegister(ah, AR_TXBF_SW, tmp);
    return (value & 0x3fffffff);
}
#endif

/* This function is used to print out CV cache to check value in CV cache */
void
ath_txbf_print_cv_cache(ath_dev_t dev)
{
#ifdef DBG_TXBF
    struct ath_softc *sc = ATH_DEV_TO_SC(dev);
    struct ath_hal *ah = sc->sc_ah;
    u_int32_t i, addr, tmp;


    tmp=ath_hal_readRegister(ah, AR_TXBF_DBG);
    DbgPrint("==>%s: address 0x10000 %x:\n", __func__, tmp);
    DbgPrint("==>%s:from AR_CVCACHE_0\n", __func__);
    addr=AR_CVCACHE_0;
    for (i=0;i<4;i++){
        DbgPrint("%x ,%x, %x, %x, %x, %x, %x, %x,\n", ath_txbf_read_cvcache(ah, addr+8*i*4),
        ath_txbf_read_cvcache(ah, addr+8*i*4+1*4), ath_txbf_read_cvcache(ah, addr+8*i*4+2*4),
        ath_txbf_read_cvcache(ah, addr+8*i*4+3*4), ath_txbf_read_cvcache(ah, addr+8*i*4+4*4),
        ath_txbf_read_cvcache(ah, addr+8*i*4+5*4), ath_txbf_read_cvcache(ah, addr+8*i*4+6*4),
        ath_txbf_read_cvcache(ah, addr+8*i*4+7*4));
    }
#endif
}


void
ath_txbf_set_rpt_received(ath_node_t node)
{
    struct ath_node *an = ATH_NODE(node);
    struct atheros_node *oan = an->an_rc_node ;
    
    oan->txbf_rpt_received = 1;
}

/* input: frame header pointer 
 * return true if current frame is CV or V report frame
 */  
bool
ath_txbf_chk_rpt_frm(struct ieee80211_frame *wh)
{
    int     subtype = wh->i_fc[0] & IEEE80211_FC0_SUBTYPE_MASK;
    bool    status = false;
    
    /* check action frame type */
    if ((subtype == IEEE80211_FC0_SUBTYPE_ACTION) ||
        (subtype == IEEE80211_FCO_SUBTYPE_ACTION_NO_ACK))
    {
        struct ieee80211_action *ia;
        u_int8_t *frm;

        frm = (u_int8_t *)&wh[1];
        ia = (struct ieee80211_action *) frm;
        
        if ((ia->ia_action == IEEE80211_ACTION_HT_NONCOMP_BF)|| 
            (ia->ia_action == IEEE80211_ACTION_HT_COMP_BF))
        {
            status = true;
        }
    }
    return status;
}
#endif /* ATH_SUPPORT_TxBF */
