/**
 * @file onebox_ioctl.h
 * @author 
 * @version 1.0
 *
 * @section LICENSE
 *
 * This software embodies materials and concepts that are confidential to Redpine
 * Signals and is made available solely pursuant to the terms of a written license
 * agreement with Redpine Signals
 *
 * @section DESCRIPTION
 *
 * This file contians the ioctl related prototypes and macros
 *  
 */

#ifndef __ONEBOX_AP_IOCTL_H__
#define __ONEBOX_AP_IOCTL_H__

#define ONEBOX_VAP_CREATE        SIOCIWLASTPRIV - 0xf  //FIXME: check for exact value b4 execution
#define ONEBOX_VAP_DELETE        SIOCIWLASTPRIV - 0x10
#define ONEBOX_HOST_IOCTL        SIOCIWLASTPRIV - 0x0B
#define RSI_WATCH_IOCTL          SIOCIWLASTPRIV - 0xd 
#define SET_BEACON_INVL          18
#define SET_BGSCAN_PARAMS        19
#define DO_BGSCAN                20
#define BGSCAN_SSID              21
#define EEPROM_READ_IOCTL        22
#define EEPROM_WRITE_IOCTL       23
#define PS_REQUEST               25
#define UAPSD_REQ                26
#define RESET_ADAPTER            32
#define RESET_PER_Q_STATS        33 
#define AGGR_LIMIT               34
#define RX_FILTER		35
#define MASTER_READ		36
#define MASTER_WRITE		37
#define TEST_MODE		38
#define RF_PWR_MODE		39
#define SET_COUNTRY 40

#define ONEBOX_SET_BB_RF         SIOCIWLASTPRIV - 0x08 
#define ONEBOX_SET_CW_MODE       SIOCIWLASTPRIV - 0x05 

/* To reset the per queue traffic stats */
//FIXME: Free ioctl num 0 , can be used.
//#define xxx      SIOCIWFIRSTPRIV + 0

#define SIOCGAUTOSTATS                (SIOCIWFIRSTPRIV+21)

bool check_valid_bgchannel(uint16 *data_ptr, uint8_t supported_band);
int ieee80211_ioctl_delete_vap(struct ieee80211com *ic, struct ifreq *ifr, struct net_device *mdev);
int ieee80211_ioctl_create_vap(struct ieee80211com *ic, struct ifreq *ifr, struct net_device *mdev);
int onebox_ioctl(struct net_device *dev,struct ifreq *ifr, int cmd);
typedef int (*ioctl_handler_t)(PONEBOX_ADAPTER adapter, struct iwreq *wrq);
#endif
