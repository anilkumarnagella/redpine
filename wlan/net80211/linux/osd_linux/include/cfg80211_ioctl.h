#ifndef __CFG80211_IOCTL_H
#define __CFG80211_IOCTL_H

#define ONEBOX_STATUS_FAILURE -1
#define ONEBOX_STATUS_SUCCESS 0

#define	IFM_OMASK	0x0000ff00	/* Type specific options */
#define	IFM_MMASK	0x00070000	/* Mode */

struct ieee80211vap;
size_t os_nl_strlcpy(char *dest, const char *src, size_t siz);
void nl80211_mem_alloc(void **ptr, unsigned short len, unsigned short flags);
void* nl80211_memcp(void *to, const void *from, int len);
void* nl80211_memcpy(void *to, const void *from, int len);
int scan_results_sup(struct wireless_dev *wdev, unsigned char ssid[6], int val,
		         int cap_info, struct ieee80211_channel *channel, 
	                int signal, uint8_t *binfo, int binfo_len, uint8_t *network_ssid);
int scan_done(void *temp_scan_req);

struct ieee80211_key_params
{
	uint8_t *key;
	uint8_t *seq;
	int key_len;
	int seq_len;
	uint32_t cipher;
};

#define IEEE80211_MAX_SSID_LEN 32
struct ieee80211_ssid
{
	uint8_t ssid[IEEE80211_MAX_SSID_LEN];
	uint8_t ssid_len;
};
/* cipher suite selectors */
#define WLAN_CIPHER_SUITE_USE_GROUP	0x000FAC00
#define WLAN_CIPHER_SUITE_WEP40		0x000FAC01
#define WLAN_CIPHER_SUITE_TKIP		0x000FAC02
/* reserved: 				0x000FAC03 */
#define WLAN_CIPHER_SUITE_CCMP		0x000FAC04
#define WLAN_CIPHER_SUITE_WEP104	0x000FAC05
#define WLAN_CIPHER_SUITE_AES_CMAC	0x000FAC06

#define WPA_PROTO_WPA BIT(0)
#define WPA_PROTO_RSN BIT(1)

#define WPA_AUTH_ALG_OPEN BIT(0)
#define WPA_AUTH_ALG_SHARED BIT(1)
#define WPA_AUTH_ALG_LEAP BIT(2)
#define WPA_AUTH_ALG_FT BIT(3)

enum wpa_alg {
	WPA_ALG_NONE,
	WPA_ALG_WEP,
	WPA_ALG_TKIP,
	WPA_ALG_CCMP,
	WPA_ALG_IGTK,
	WPA_ALG_PMK
};
typedef struct
{
	unsigned short timestamp[4];
	unsigned short beacon_intvl;
	unsigned short capability;
	unsigned char variable[0];
}__attribute__ ((packed)) BEACON_PROBE_FORMAT;



#if 0
struct ieee80211_ies1 {
	/* the following are either NULL or point within data */
	uint8_t	*wpa_ie;	/* captured WPA ie */
	uint8_t	*rsn_ie;	/* captured RSN ie */
	uint8_t	*wme_ie;	/* captured WME ie */
	uint8_t	*ath_ie;	/* captured Atheros ie */
	uint8_t	*htcap_ie;	/* captured HTCAP ie */
	uint8_t	*htinfo_ie;	/* captured HTINFO ie */
	uint8_t	*tdma_ie;	/* captured TDMA ie */
	uint8_t *meshid_ie;	/* captured MESH ID ie */
	uint8_t	*spare[4];
	/* NB: these must be the last members of this structure */
	uint8_t	*data;		/* frame data > 802.11 header */
	int	len;		/* data size in bytes */
};

struct nl80211_ieee80211_scan_entry {
	uint8_t		se_macaddr[6];
	uint8_t		se_bssid[6];
	/* XXX can point inside se_ies */
	uint8_t		se_ssid[2+32];
	uint8_t		se_rates[2+15];
	uint8_t		se_xrates[2+15];
	union {
		uint8_t		data[8];
		u_int64_t	tsf;
	} se_tstamp;			/* from last rcv'd beacon */
	uint16_t	se_intval;	/* beacon interval (host byte order) */
	uint16_t	se_capinfo;	/* capabilities (host byte order) */
	struct ieee80211_channel *se_chan;/* channel where sta found */
	uint16_t	se_timoff;	/* byte offset to TIM ie */
	uint16_t	se_fhdwell;	/* FH only (host byte order) */
	uint8_t		se_fhindex;	/* FH only */
	uint8_t		se_dtimperiod;	/* DTIM period */
	uint16_t	se_erp;		/* ERP from beacon/probe resp */
	int8_t		se_rssi;	/* avg'd recv ssi */
	int8_t		se_noise;	/* noise floor */
	uint8_t		se_cc[2];	/* captured country code */
	uint8_t		se_meshid[2+32];
	struct ieee80211_ies1 se_ies;	/* captured ie's */
	u_int		se_age;		/* age of entry (0 on create) */
};
#endif

uint8_t cfg80211_wrapper_attach(struct net_device *dev,void *ptr, int size);
void cfg80211_wrapper_free_wdev(struct wireless_dev *wdev);

int onebox_prepare_ioctl_cmd(struct ieee80211vap *vap, uint8_t type, const void *data, int val, int len );

uint8_t onebox_delete_key(struct net_device *ndev, const uint8_t *mac_addr, uint8_t index);

uint8_t  onebox_add_key(struct net_device *ndev, uint8_t index, 
                             const uint8_t *mac_addr,struct ieee80211_key_params *params );

uint8_t onebox_wep_key(struct net_device *ndev, int index, uint8_t *mac_addr, uint8_t key_len, const uint8_t *key);
uint8_t onebox_siwrts(struct iw_param *rts);
uint8_t onebox_siwfrag(struct iw_param *frag);
uint8_t tx_power(int dbm);
int onebox_set_if_media(struct net_device *dev, int media);
int onebox_get_if_media(struct net_device *dev);
int onebox_set_mediaopt(struct net_device *dev, uint32_t mask, uint32_t mode);
int onebox_driver_nl80211_set_wpa(struct ieee80211vap *vap, int enabled);
int onebox_driver_nl80211_set_wpa_internal(struct ieee80211vap *vap, int wpa, int privacy);
int nl80211_ctrl_iface(struct net_device *ndev, int enable);
int cfg80211_connect_res(struct net_device *ndev, int auth, const uint8_t *ie, size_t ie_len, unsigned char *ssid, 
						 size_t ssid_len, unsigned char *bssid, int len, int privacy);
int cfg80211_inform_scan_results(void *arg, void *se_nl);
int cfg80211_disconnect(struct net_device *ndev, int reason_code);
int cfg80211_connect_result_vap(struct ieee80211vap *vap, uint8_t mac[6]);
int connection_confirm(struct wireless_dev *wdev, uint8_t mac[6]);
int cfg80211_disconnect_result_vap(struct ieee80211vap *vap);
int disconnection_confirm(struct wireless_dev *wdev);
#endif

