
/*
 * @file
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
 * This file contians the code specific to Litefi-AP architecture.
 */

#ifdef ONEBOX_CONFIG_CFG80211

#include <net/cfg80211.h>
#include <linux/rtnetlink.h>
#include <linux/version.h>
#include <linux/hardirq.h>
#include "linux/wireless.h"
#include "cfg80211_wrapper.h"
#include "cfg80211_ioctl.h"


static struct cfg80211_ops onebox_cfg80211_ops = {
	.set_wiphy_params = onebox_cfg80211_set_wiphy_params,
#if(LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 38))
	.set_tx_power = onebox_cfg80211_set_txpower,
#endif
	.scan = onebox_cfg80211_scan,
	.connect = onebox_cfg80211_connect,
	.disconnect = onebox_cfg80211_disconnect,
	.add_key = onebox_cfg80211_add_key,
	.del_key = onebox_cfg80211_del_key,
	.set_default_key = onebox_cfg80211_set_default_key,
	.get_station = onebox_cfg80211_get_station,
};


/**
 * This function sets the wiphy parameters if changed. 
 *
 * @param  Pointer to wiphy structure.  
 * @param  Value that indicates which wiphy parameter changed.  
 * @return GANGES_STATUS_SUCCESS on success else negative number on failure. 
 */
ONEBOX_STATUS 
onebox_cfg80211_set_wiphy_params(struct wiphy *wiphy, uint32_t changed)
{
	struct iw_param rts;
	struct iw_param frag;

	if (changed & WIPHY_PARAM_RTS_THRESHOLD)
	{
		rts.value = wiphy->rts_threshold;
		rts.disabled = 0;
		if(onebox_siwrts(&rts) < 0)
		{
			return ONEBOX_STATUS_FAILURE;
		}
		else
		{
			return ONEBOX_STATUS_SUCCESS;
		}
	}

	if (changed & WIPHY_PARAM_FRAG_THRESHOLD)
	{
		frag.value = wiphy->frag_threshold;
		frag.disabled = 0;
		if(onebox_siwfrag(&frag) < 0)
		{
			return ONEBOX_STATUS_FAILURE;
		}
		else
		{
			return ONEBOX_STATUS_SUCCESS;
		}
	}
	return ONEBOX_STATUS_SUCCESS;
}

#if(LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 38))
ONEBOX_STATUS onebox_cfg80211_add_key(struct wiphy *wiphy,struct net_device *ndev,
                                      uint8_t index, const uint8_t  *mac_addr, struct key_params *params)
#else
ONEBOX_STATUS onebox_cfg80211_add_key(struct wiphy *wiphy,struct net_device *ndev,
                                      uint8_t index, bool pairwise, const uint8_t  *mac_addr, struct key_params *params)
#endif
{
	if(onebox_add_key(ndev, index, mac_addr, (struct ieee80211_key_params *)params) < 0)
	{
		return ONEBOX_STATUS_FAILURE;
	}
	else
	{
		return ONEBOX_STATUS_SUCCESS;
	}
}

/**
 * This function deletes the key of particular key_index. 
 *
 * @param  Pointer to wiphy structure.  
 * @param  Pointer to network device structure.  
 * @param  Key Index to be deleted.  
 * @param  Value to identify pairwise or group key.  
 * @param  Pointer to the MAC address of the station to delete keys.  
 * @return GANGES_STATUS_SUCCESS on success else negative number on failure. 
 */

#if(LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 38))
ONEBOX_STATUS 
onebox_cfg80211_del_key(struct wiphy *wiphy, struct net_device *ndev, uint8_t index,
                        const uint8_t *mac_addr )
#else
ONEBOX_STATUS 
onebox_cfg80211_del_key(struct wiphy *wiphy, struct net_device *ndev, uint8_t index,
                        bool pairwise, const uint8_t *mac_addr )
#endif
{
	if(onebox_delete_key(ndev, mac_addr, index) < 0)
	{
		return ONEBOX_STATUS_FAILURE;
	}
	else
	{
		return ONEBOX_STATUS_SUCCESS;	
	}
}

/**
 * This function sets the various connection params for infra mode.
 *
 * @param  Pointer to wiphy structure.  
 * @param  Pointer to network device structure.  
 * @param  Mode to set Infrastructure/IBSS.  
 * @param  Pointer to flags.  
 * @param  Pointer to interface parameters.  
 * @return GANGES_STATUS_SUCCESS on success else negative number on failure. 
 */
ONEBOX_STATUS 
onebox_cfg80211_connect(struct wiphy *wiphy, struct net_device *ndev, struct cfg80211_connect_params *params)
{
	int privacy = params->privacy;
	int auth = params->auth_type;
//	printk("ASSOC CMD: In %s and %d mac_addr = %02x:%02x:%02x:%02x:%02x:%02x ssid = %s \n", __func__, __LINE__, params->bssid[0], params->bssid[1], params->bssid[2], params->bssid[3], params->bssid[4], params->bssid[5], params->ssid);

	if(params->key)
	{
		if(onebox_wep_key(ndev, params->key_idx, params->bssid, params->key_len, params->key) < 0)
		{
			return ONEBOX_STATUS_FAILURE;
		}
	}

	if(cfg80211_connect_res(ndev, auth, params->ie, params->ie_len, params->ssid, 
                            params->ssid_len, params->bssid, params->ssid_len, privacy) < 0)
	{
		return ONEBOX_STATUS_FAILURE;
	}
	else
	{
		return ONEBOX_STATUS_SUCCESS;
	}
}

uint8_t cfg80211_wrapper_attach(struct net_device *vap_dev, void *dev_ptr, int size)
{
	struct device *device;
	struct wireless_dev *wdev;
	device = (struct device *)dev_ptr;
	wdev = onebox_register_wiphy_dev(device, size);
	if (!wdev)
	{
		printk("Failed to create wireless device\n");
		return ONEBOX_STATUS_FAILURE;
	}
	vap_dev->ieee80211_ptr = wdev;
	SET_NETDEV_DEV(vap_dev, wiphy_dev(wdev->wiphy));
	wdev->netdev = vap_dev;
	//FIXME: Assuming default mode of operation is Station
	wdev->iftype = NL80211_IFTYPE_STATION;

	return ONEBOX_STATUS_SUCCESS;
}

#if(LINUX_VERSION_CODE < KERNEL_VERSION(3, 3, 3))
EXPORT_SYMBOL(cfg80211_wrapper_attach);
#endif

/**
 * This function registers our device as a wiphy device and registers the 
 * initial wiphy parameters. We allocate space for wiphy parameters. 
 *
 * @param  Pointer to our device(pfunc).  
 * @return Pointer to the wireless dev structure obtained after registration. 
 */
struct wireless_dev* onebox_register_wiphy_dev(struct device *dev, int size)
{
	struct wireless_dev *wdev = NULL;
#if(LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 38))
	int rtnl_locked = rtnl_is_locked();
#endif
	/* XXX: Mac address read from eeprom */
	uint8_t addr[ETH_ADDR_LEN] = {0x00, 0x23, 0xa7,0x0f, 0x03, 0x06}; 
	uint8_t ii = 0, ret = 0;
	uint16_t freq_list[] = { 5180, 5200, 5220, 5240, 5260, 5280,
	                         5300, 5320, 5500, 5520, 5540, 5560,
	                         5580, 5600, 5620, 5640, 5660, 5680, 
	                         5700, 5745, 5765, 5785, 5805, 5825};
	
	wdev = kzalloc(sizeof(struct wireless_dev), GFP_KERNEL);
	if (!wdev)
	{
		printk("mem allocation failed");
		return NULL;
	}
	/*FIXME: second arg is the private data area to allocate
 	 * Make sure the size of priv data is provided correctly
 	 */
//	printk("NL80211 : In %s LINE %d size %d\n",__func__,__LINE__, size);
	wdev->wiphy = wiphy_new(&onebox_cfg80211_ops, size);
//	printk("NL80211 : In %s LINE %d wdev->wiphy %p\n",__func__,__LINE__,wdev->wiphy);
	if (!wdev->wiphy)
	{
		printk("Couldn't allocate wiphy device\n");
		kfree(wdev);
		return NULL;
	}  
	set_wiphy_dev(wdev->wiphy, dev);
	band_24ghz.bitrates   = bg_rsi_rates;
	band_24ghz.n_bitrates = ARRAY_SIZE(bg_rsi_rates);  
	band_24ghz.n_channels = (MAX_CHANNELS_24GHZ - 1);
	band_5ghz.bitrates   = (bg_rsi_rates + 4);
	band_5ghz.n_bitrates = (ARRAY_SIZE(bg_rsi_rates) - 4);
	band_5ghz.n_channels = (MAX_CHANNELS_5GHZ - 1);
	for (ii = 0; ii < MAX_CHANNELS_24GHZ - 1; ii++)
	{
		channels_24ghz[ii].center_freq = (2412 + ii * 5);
	} /* End for loop */
	channels_24ghz[ii].center_freq = 2484;
	for (ii = 0; ii < MAX_CHANNELS_5GHZ - 1; ii++)
	{
		channels_5ghz[ii].center_freq = freq_list[ii];
	}
	band_24ghz.band = IEEE80211_BAND_2GHZ;
	band_24ghz.channels = &channels_24ghz[0];
	wdev->wiphy->bands[IEEE80211_BAND_2GHZ] = &band_24ghz;
	band_5ghz.band = IEEE80211_BAND_5GHZ;
	band_5ghz.channels = &channels_5ghz[0];
	wdev->wiphy->bands[IEEE80211_BAND_5GHZ] = &band_5ghz;
	wdev->wiphy->n_addresses = 1;
	wdev->wiphy->addresses = (struct mac_address *)&addr[0];
	wdev->wiphy->max_scan_ssids = 1;
	wdev->wiphy->interface_modes = (BIT(NL80211_IFTYPE_STATION));
	wdev->wiphy->signal_type = CFG80211_SIGNAL_TYPE_UNSPEC;
	wdev->wiphy->frag_threshold  = DFL_FRAG_THRSH - 28;
	wdev->wiphy->rts_threshold   = DFL_RTS_THRSH - 28;
	wdev->wiphy->cipher_suites   = cipher_suites;
	wdev->wiphy->n_cipher_suites = ARRAY_SIZE(cipher_suites);

#if 0
	wdev->wiphy->wowlan.flags = WIPHY_WOWLAN_MAGIC_PKT |
			      WIPHY_WOWLAN_DISCONNECT |
			      WIPHY_WOWLAN_GTK_REKEY_FAILURE  |
			      WIPHY_WOWLAN_SUPPORTS_GTK_REKEY |
			      WIPHY_WOWLAN_EAP_IDENTITY_REQ   |
			      WIPHY_WOWLAN_4WAY_HANDSHAKE;
	wdev->wiphy->wowlan.n_patterns = 4;
	wdev->wiphy->wowlan.pattern_min_len = 1;
	wdev->wiphy->wowlan.pattern_max_len = 64;

	wdev->wiphy->max_sched_scan_ssids = 10;
#endif
#if(LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 38))
	if(rtnl_locked)
	{
		rtnl_unlock();
	}
	ret = wiphy_register(wdev->wiphy);
	rtnl_lock();
#else
	ret = wiphy_register(wdev->wiphy);
#endif
	if (ret < 0)
	{
		//ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,(TEXT("Couldn't register wiphy device\n")));
		printk("Couldn't register wiphy device\n");
		wiphy_free(wdev->wiphy);
		return NULL;
	}
	return wdev;
}

/**
 * This function triggers the scan. 
 *
 * @param  Pointer to wiphy structure.  
 * @param  Pointer to network device structure.  
 * @param  Pointer to the cfg80211_scan_request structure
 *         that has the scan parameters.  
 * @return GANGES_STATUS_SUCCESS on success else negative number on failure. 
 */
ONEBOX_STATUS 
onebox_cfg80211_scan(struct wiphy *wiphy, struct net_device *ndev,struct cfg80211_scan_request *request)
{
	uint16_t *channels= NULL;
	uint8_t n_channels = 0;
	ONEBOX_STATUS status = ONEBOX_STATUS_SUCCESS;	

	if(request->n_channels > 0  && (request->n_channels <= IEEE80211_MAX_FREQS_ALLOWED ))
	{

		uint8_t i;
		n_channels = request->n_channels;
        	channels = kzalloc(n_channels * sizeof(uint16_t), GFP_KERNEL);
		if (channels == NULL) 
		{
			printk("failed to set scan channels, scan all channels");
            		n_channels = 0;
        	}
        	for (i = 0; i < n_channels; i++)
		{
			channels[i] =  request->channels[i]->center_freq;
			printk("%s %d:selected chans  are %d\n", __func__, __LINE__,  request->channels[i]->center_freq);
		}
	}
	else
	{
		request->n_channels = 0;

	}

	if(cfg80211_scan(request, ndev, n_channels, channels,  request->ie, request->ie_len, request->n_ssids, request->ssids))
	{
		status = ONEBOX_STATUS_FAILURE;
	}
	else
	{
		/* No code here */
	}
	if(channels)
	{
		kfree(channels);
	}
	return status;
}	

#if(LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 38))
ONEBOX_STATUS 
onebox_cfg80211_set_txpower(struct wiphy *wiphy, enum nl80211_tx_power_setting type, int dbm)
{
	dbm = (dbm / 100); //convert mbm to dbm
	if (type == NL80211_TX_POWER_FIXED)
	{
		if (dbm < 0)
		{
			dbm = 0;
		}
		else if (dbm > 20) /* FIXME - Check the max */
		{
			dbm = 20;
		}
		else
		{
      		/* No code here */
		} /* End if <condition> */
	}
	else 
	{
    	/* Automatic adjustment */
    	dbm = 0xff;
  	} /* End if <condition> */
	return tx_power(dbm);
}
#endif

ONEBOX_STATUS 
onebox_cfg80211_set_default_key(struct wiphy *wiphy, struct net_device *ndev, uint8_t index, bool unicast, bool multicast)
{
	return ONEBOX_STATUS_SUCCESS;
}

ONEBOX_STATUS 
onebox_cfg80211_disconnect(struct wiphy *wiphy, struct net_device *ndev, uint16_t reason_code)
{
	printk("<<< Recvd Disassoc In %s Line %d reason code %d >>>\n", __func__, __LINE__, reason_code);
	if(cfg80211_disconnect(ndev, reason_code) < 0)
	{
		return ONEBOX_STATUS_FAILURE;
	}
	else
	{
		return ONEBOX_STATUS_SUCCESS;
	}
}

ONEBOX_STATUS
onebox_cfg80211_get_station(struct wiphy *wiphy, struct net_device *ndev, uint8_t *mac, struct station_info *info)
{
	return ONEBOX_STATUS_SUCCESS;
}

void cfg80211_wrapper_free_wdev(struct wireless_dev *wdev)
{
	if(!wdev)
	{
		return;
	}
	wiphy_unregister(wdev->wiphy);
	wiphy_free(wdev->wiphy);
	kfree(wdev);
}

#if(LINUX_VERSION_CODE < KERNEL_VERSION(3, 3, 3))
EXPORT_SYMBOL(cfg80211_wrapper_free_wdev);
#endif


int scan_results_sup(struct wireless_dev *wdev, unsigned char ssid[6], int val, int cap_info, 
                     struct ieee80211_channel *channel1, int chan, uint8_t *binfo, int binfo_len, uint8_t *network_ssid)
{
	struct wiphy *wiphy = wdev->wiphy;

	struct ieee80211_mgmt *mgmt = NULL;
	struct cfg80211_bss *bss = NULL;
	char *temp;
	BEACON_PROBE_FORMAT *frame;
	int frame_len = 0;
	uint8_t band;
	int freq;
	struct ieee80211_channel *channel = NULL;

	nl80211_mem_alloc((void **)&mgmt, sizeof(struct ieee80211_mgmt) + 512, GFP_ATOMIC);
	nl80211_memcpy(mgmt->bssid, ssid, 6);
	mgmt->frame_control = cpu_to_le16(IEEE80211_FTYPE_MGMT); 
	frame = (BEACON_PROBE_FORMAT *)(&mgmt->u.beacon); 
	temp = (unsigned char *)&frame->timestamp[0];

	memset(temp, 0, 8);
	frame->beacon_intvl = cpu_to_le16(val);
	frame->capability   = cpu_to_le16(cap_info);
	temp = frame->variable;
	/** Added the below 3 lines code to give ssid field in the data ie **/
	/** Made this change for Hidden mode AP's **/
	if((binfo[0] == WLAN_EID_SSID && binfo[1] == 0))
	{
		*temp++ = WLAN_EID_SSID;
		*temp++ = network_ssid[1];
		memcpy(temp, &network_ssid[2], network_ssid[1]);
		temp = temp + network_ssid[1];
		frame_len = network_ssid[1];
		nl80211_memcpy(temp, binfo, binfo_len );
		binfo_len -= 2;
		binfo += 2 ;
	}
	nl80211_memcpy(temp, binfo, binfo_len );
  	frame_len += offsetof(struct ieee80211_mgmt, u.beacon.variable);
  	frame_len += binfo_len;
	if (chan <= 14)
		band = IEEE80211_BAND_2GHZ;
	else
		band = IEEE80211_BAND_5GHZ;

#if(LINUX_VERSION_CODE <= KERNEL_VERSION(2, 6, 38))
	freq = ieee80211_channel_to_frequency(chan);
#else
	freq = ieee80211_channel_to_frequency(chan, band);
#endif

	channel = ieee80211_get_channel(wiphy, freq);



  	if (!wiphy || !channel || !mgmt || 
    	 frame_len < offsetof(struct ieee80211_mgmt, u.beacon.variable))
  	{
		kfree(mgmt);  
		printk("Error Informing Frame %d \n", __LINE__);  
	    return ONEBOX_STATUS_FAILURE;
  	}
	bss = cfg80211_inform_bss_frame(wiphy, channel, mgmt,
	                                cpu_to_le16(frame_len),
	                                0, GFP_ATOMIC);
	if (!bss)
	{
		kfree(mgmt);
		printk("Error Informing Frame \n");  
		return ONEBOX_STATUS_FAILURE;
	}
	cfg80211_put_bss(bss);/*this is needed to update bss list */
	kfree(mgmt);
	return ONEBOX_STATUS_SUCCESS;
}

int scan_done(void *temp_scan_req)
{
	struct cfg80211_scan_request *req;
	req = scan_req;

	if(req == NULL)
	{
		cfg80211_cqm_rssi_notify(wdev->netdev, 3, GFP_ATOMIC);
		return 0;
	}
	cfg80211_scan_done(req, 0);
	req = NULL;
	return ONEBOX_STATUS_SUCCESS;
}

void nl80211_mem_alloc(void **ptr, unsigned short len, unsigned short flags)
{
	*ptr = kmalloc(len, flags);
}

void* nl80211_memcpy(void *to, const void *from, int len)
{
	return memcpy(to,from,len);
}

int connection_confirm(struct wireless_dev *wdev, uint8_t mac[6])
{
	printk("ASSOC CMD: In %s and %d mac_addr = %02x:%02x:%02x:%02x:%02x:%02x\n", __func__, __LINE__, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	cfg80211_connect_result(wdev->netdev, mac, NULL, 0, NULL, 0, 0, GFP_ATOMIC);
	return ONEBOX_STATUS_SUCCESS;
}

int disconnection_confirm(struct wireless_dev *wdev)
{
	wdev = wdev->netdev->ieee80211_ptr;
	wdev->iftype = NL80211_IFTYPE_STATION;
	cfg80211_disconnected(wdev->netdev, 0, NULL, 0, GFP_ATOMIC);
	return ONEBOX_STATUS_SUCCESS;
}
#endif


