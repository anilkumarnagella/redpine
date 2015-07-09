#include <net/cfg80211.h>
#include "cfg80211_ioctl.h"

#define ONEBOX_STATUS int
#define ONEBOX_STATUS_FAILURE -1
#define ONEBOX_STATUS_SUCCESS 0


#define DFL_FRAG_THRSH 2346
#define DFL_RTS_THRSH 2346

#define ETH_ADDR_LEN  6

#define MAX_CHANNELS_24GHZ 14
#define MAX_CHANNELS_5GHZ  25

#define IEEE80211_IOC_APPIE  95/* application IE's */
#define	IEEE80211_FC0_TYPE_MGT			0x00
#define	IEEE80211_FC0_SUBTYPE_PROBE_REQ		0x40

#define STD_RATE_MCS7 0x82
#define STD_RATE_MCS6 0x75
#define STD_RATE_MCS5 0x68
#define STD_RATE_MCS4 0x4E
#define STD_RATE_MCS3 0x34            
#define STD_RATE_MCS2 0x27
#define STD_RATE_MCS1 0x1A
#define STD_RATE_MCS0 0x0D 
#define STD_RATE_54   0x6c
#define STD_RATE_48   0x60
#define STD_RATE_36   0x48
#define STD_RATE_24   0x30
#define STD_RATE_18   0x24
#define STD_RATE_12   0x18
#define STD_RATE_11   0x16
#define STD_RATE_09   0x12
#define STD_RATE_06   0x0C
#define STD_RATE_5_5  0x0B
#define STD_RATE_02   0x04
#define STD_RATE_01   0x02

/* cipher suite selectors */
#define WLAN_CIPHER_SUITE_USE_GROUP	0x000FAC00
#define WLAN_CIPHER_SUITE_WEP40		0x000FAC01
#define WLAN_CIPHER_SUITE_TKIP		0x000FAC02
/* reserved: 				0x000FAC03 */
#define WLAN_CIPHER_SUITE_CCMP		0x000FAC04
#define WLAN_CIPHER_SUITE_WEP104	0x000FAC05
#define WLAN_CIPHER_SUITE_AES_CMAC	0x000FAC06


static const u32 cipher_suites[] = {
	WLAN_CIPHER_SUITE_WEP40,
	WLAN_CIPHER_SUITE_WEP104,
	WLAN_CIPHER_SUITE_TKIP,
	WLAN_CIPHER_SUITE_CCMP,
};

enum ieee80211_roamingmode {
	IEEE80211_ROAMING_DEVICE= 0,	/* driver/hardware control */
	IEEE80211_ROAMING_AUTO	= 1,	/* 802.11 layer control */
	IEEE80211_ROAMING_MANUAL= 2,	/* application control */
};
struct ieee80211_rate ;

static struct ieee80211_rate bg_rsi_rates[] = {
	{ .bitrate = STD_RATE_01 * 5},
	{ .bitrate = STD_RATE_02 * 5},
	{ .bitrate = STD_RATE_5_5 * 5},
	{ .bitrate = STD_RATE_11 * 5},
	{ .bitrate = STD_RATE_06 * 5},
	{ .bitrate = STD_RATE_09 * 5},
	{ .bitrate = STD_RATE_12 * 5},
	{ .bitrate = STD_RATE_18 * 5},
	{ .bitrate = STD_RATE_24 * 5},
	{ .bitrate = STD_RATE_36 * 5},
	{ .bitrate = STD_RATE_48 * 5},
	{ .bitrate = STD_RATE_54 * 5},
	{ .bitrate = STD_RATE_MCS0 * 5},
	{ .bitrate = STD_RATE_MCS1 * 5},
	{ .bitrate = STD_RATE_MCS2 * 5},
	{ .bitrate = STD_RATE_MCS3 * 5},
	{ .bitrate = STD_RATE_MCS4 * 5},
	{ .bitrate = STD_RATE_MCS5 * 5},
	{ .bitrate = STD_RATE_MCS6 * 5},
	{ .bitrate = STD_RATE_MCS7 * 5}
};

struct ieee80211_supported_band  band_24ghz, band_5ghz;
struct ieee80211_channel channels_24ghz[MAX_CHANNELS_24GHZ], channels_5ghz[MAX_CHANNELS_5GHZ];

struct ieee80211_scan_req {
	int      sr_flags;
	u_int    sr_duration;   /* duration (ms) */
	u_int    sr_mindwell;   /* min channel dwelltime (ms) */
	u_int    sr_maxdwell;   /* max channel dwelltime (ms) */
	int sr_nssid;
#define IEEE80211_NWID_LEN 32
#define IEEE80211_IOC_SCAN_MAX_SSID 3
	struct 
	{
		uint8_t len;             /* length in bytes */
		uint8_t ssid[IEEE80211_NWID_LEN]; /* ssid contents */
	} sr_ssid[IEEE80211_IOC_SCAN_MAX_SSID];

#define IEEE80211_MAX_FREQS_ALLOWED 25
//#define IEEE80211_MAX_FREQS_ALLOWED 32

//#ifdef ENABLE_P2P_SUPPORT
	uint16_t num_freqs;
	uint16_t freqs[IEEE80211_MAX_FREQS_ALLOWED];
//#endif
};

/* Function prototypes */
uint8_t cfg80211_wrapper_attach(struct net_device *dev, void *dev_ptr, int size);
void cfg80211_wrapper_free_wdev(struct wireless_dev *wdev);

#if(LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 38))
ONEBOX_STATUS 
onebox_cfg80211_set_txpower(struct wiphy *wiphy, enum nl80211_tx_power_setting   type, int dbm);
#endif

#if(LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 38))
ONEBOX_STATUS onebox_cfg80211_del_key(struct wiphy *wiphy, struct net_device *ndev, uint8_t index,
                                      const uint8_t *mac_addr );

ONEBOX_STATUS onebox_cfg80211_add_key(struct wiphy *wiphy,struct net_device *ndev,
                                      uint8_t index, const uint8_t  *mac_addr, struct key_params *params);
#else
ONEBOX_STATUS onebox_cfg80211_del_key(struct wiphy *wiphy, struct net_device *ndev, uint8_t index,
                                      bool pairwise, const uint8_t *mac_addr );

ONEBOX_STATUS onebox_cfg80211_add_key(struct wiphy *wiphy,struct net_device *ndev,
                                      uint8_t index, bool pairwise, const uint8_t  *mac_addr, struct key_params *params);
#endif

ONEBOX_STATUS 
onebox_cfg80211_set_default_key(struct wiphy *wiphy, struct net_device *ndev, uint8_t index, bool unicast, bool multicast);

ONEBOX_STATUS
onebox_cfg80211_get_station(struct wiphy *wiphy, struct net_device *ndev, uint8_t *mac, struct station_info *info);

ONEBOX_STATUS 
onebox_cfg80211_connect(struct wiphy *wiphy, struct net_device *ndev, struct cfg80211_connect_params *params);

ONEBOX_STATUS 
onebox_cfg80211_disconnect(struct wiphy *wiphy, struct net_device *ndev, uint16_t reason_code);

ONEBOX_STATUS onebox_cfg80211_set_wiphy_params(struct wiphy *wiphy, uint32_t changed);

ONEBOX_STATUS 
onebox_cfg80211_scan(struct wiphy *wiphy,struct net_device *ndev,
                     struct cfg80211_scan_request *request);

struct wireless_dev* onebox_register_wiphy_dev(struct device *dev, int size);

int cfg80211_scan(void *temp_req, struct net_device *ndev, uint8_t num_chn,  uint16_t *chan,
                     const uint8_t *ie, size_t ie_len, int n_ssids, void *ssids);
