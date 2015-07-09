
#ifdef ONEBOX_CONFIG_CFG80211
#include<linux/kernel.h>
#include<net80211/ieee80211_var.h>
#include<net80211/ieee80211_ioctl.h>
#include<net80211/ieee80211_crypto.h>
#include<net80211/ieee80211.h>
#include "cfg80211_ioctl.h"



void *temp_vap;

int  onebox_prepare_ioctl_cmd(struct ieee80211vap *vap, uint8_t type, const void *data, int val, int len )
{
	struct ieee80211req *ireq;

	ireq = kmalloc(sizeof(struct ieee80211req), GFP_KERNEL);
	ireq->i_type = type;
	ireq->i_data = (void *)data;
	ireq->i_val = val;
	ireq->i_len = len;
	return ieee80211_ioctl_set80211(vap, 0, ireq);
	
}

uint8_t onebox_add_key(struct net_device *ndev, uint8_t index, 
                       const uint8_t  *mac_addr, struct ieee80211_key_params *params)
{
	struct ieee80211vap *vap = netdev_priv(ndev);
	struct ieee80211req_key wk;

	memset(&wk, 0, sizeof(wk));

//	printk("NL80211 : In %s LINE %d\n",__func__,__LINE__);
	if (params->cipher == WPA_ALG_NONE) 
	{
		if (mac_addr == NULL ||
		    memcmp(mac_addr, "\xff\xff\xff\xff\xff\xff",
			      IEEE80211_ADDR_LEN) == 0)
		{
			return onebox_delete_key(ndev, NULL, index);
		}
		else
		{
			return onebox_delete_key(ndev, mac_addr, index);
		}
	}
//	printk("NL80211 : In %s LINE %d params->cipher %x index %d\n",__func__,__LINE__, params->cipher, index);
	switch (params->cipher)
	{
		case WLAN_CIPHER_SUITE_WEP40:
		case WLAN_CIPHER_SUITE_WEP104:
		{
			if(index > 3)
			{
				break;
			}
			wk.ik_type = IEEE80211_CIPHER_WEP;
			break;
		}
		case WLAN_CIPHER_SUITE_TKIP:
		{
			wk.ik_type = IEEE80211_CIPHER_TKIP;
			break;
		}
		case WLAN_CIPHER_SUITE_CCMP:
		{
			wk.ik_type = IEEE80211_CIPHER_AES_CCM;
			break;
		}
		default:
			printk("%s: unknown alg=%d", __func__, params->cipher);
			return -1;

		break;
	}
	wk.ik_flags = IEEE80211_KEY_RECV ;
	wk.ik_flags |= IEEE80211_KEY_XMIT;

	/**need to set the flag based on transmission**/
	/*need to fix
	if(set_tx)
		wk.ik_flags |= IEEE80211_KEY_XMIT;*/

	if (mac_addr == NULL) 
	{
		memset(wk.ik_macaddr, 0xff, IEEE80211_ADDR_LEN);
		wk.ik_keyix = index;
	} 
	else 
	{
		memcpy(wk.ik_macaddr, mac_addr, IEEE80211_ADDR_LEN);
	}

	if (memcmp(wk.ik_macaddr, "\xff\xff\xff\xff\xff\xff",
		      IEEE80211_ADDR_LEN) == 0) 
	{
		wk.ik_flags |= IEEE80211_KEY_GROUP;
		wk.ik_keyix = index;
	} 
	else 
	{
		wk.ik_keyix = index == 0 ? IEEE80211_KEYIX_NONE : index;
	}

	//if (wk.ik_keyix != IEEE80211_KEYIX_NONE && set_tx)/* need to fix */
	if (wk.ik_keyix != IEEE80211_KEYIX_NONE )/* FIXME */
	{
		wk.ik_flags |= IEEE80211_KEY_DEFAULT;
	}

	wk.ik_keylen = params->key_len;
	memcpy(&wk.ik_keyrsc, params->seq, params->seq_len);
	memcpy(wk.ik_keydata, params->key, params->key_len);
//	printk("NL80211 : In %s LINE %d\n",__func__,__LINE__);

	if(onebox_prepare_ioctl_cmd(vap, IEEE80211_IOC_WPAKEY, (void *)&wk, 0, sizeof(wk)) < 0)
	{
		return ONEBOX_STATUS_FAILURE;
	}
	else
	{
		return ONEBOX_STATUS_SUCCESS;
	}
}

uint8_t
onebox_delete_key(struct net_device *ndev, const uint8_t *mac_addr, uint8_t index)
{
	struct ieee80211vap *vap = netdev_priv(ndev);
	struct ieee80211req_del_key wk;

	if (mac_addr == NULL) 
	{
		//printk("%s: key_idx=%d", __func__, index);
		wk.idk_keyix = index;
	} 
	else 
	{
		//printk("%s: addr=" MACSTR, __func__, MAC2STR(addr));
		memcpy(wk.idk_macaddr, mac_addr, IEEE80211_ADDR_LEN);
		wk.idk_keyix = (u_int8_t) IEEE80211_KEYIX_NONE;	/* XXX */
	}

	if(onebox_prepare_ioctl_cmd(vap, IEEE80211_IOC_DELKEY, (void *)&wk, 0, sizeof(wk)) < 0)
	{
		return ONEBOX_STATUS_FAILURE;
	}
	else
	{
		return ONEBOX_STATUS_SUCCESS;
	}
} 

int cfg80211_disconnect(struct net_device *ndev, int reason_code)
{
	struct ieee80211req_mlme mlme;
	struct ieee80211vap *vap = netdev_priv(ndev);

	memset(&mlme, 0, sizeof(mlme));
	mlme.im_op = IEEE80211_MLME_DISASSOC;
	mlme.im_reason = reason_code;

	if(reason_code == 99)
	{
		printk("In %s and %d \n", __func__, __LINE__);
		return 0;
	}
	printk("In %s Line %d recvd disconnect reason code %d\n", __func__, __LINE__, reason_code);
	if(onebox_prepare_ioctl_cmd(vap, IEEE80211_IOC_MLME, &mlme, 0, sizeof(mlme)) < 0)
	{
		return ONEBOX_STATUS_FAILURE;
	}
	else
	{
		return ONEBOX_STATUS_SUCCESS;
	}
}

uint8_t 
onebox_siwfrag(struct iw_param *frag)
{
	struct ieee80211vap *vap;
	struct ieee80211req ireq;

	vap = temp_vap;
	if (frag->disabled)
	{
		ireq.i_val = 2346;
	}
	else if ((frag->value < 256) || (frag->value > 2346))
	{
		return -EINVAL;
	}
	else
	{
		ireq.i_val = (frag->value & ~0x1);
	}

	ireq.i_type = IEEE80211_IOC_FRAGTHRESHOLD;
//	printk("NL80211 : In %s LINE %d \n",__func__,__LINE__);
	return ieee80211_ioctl_set80211(vap, 0, &ireq);
}

uint8_t 
onebox_siwrts(struct iw_param *rts)
{
	struct ieee80211vap *vap;
	struct ieee80211req ireq;
	
	vap = temp_vap;
//	printk("NL80211 : In %s and %d vap %p\n",__func__,__LINE__, vap);
	if (rts->disabled)
	{
		ireq.i_val = IEEE80211_RTS_MAX;
	}
	else if ((IEEE80211_RTS_MIN <= rts->value) &&
	         (rts->value <= IEEE80211_RTS_MAX))
	{
		ireq.i_val = rts->value;
	}
	else
	{
		return -EINVAL;
	}

//	printk("NL80211 : In %s and %d vap %p val %d\n",__func__,__LINE__, vap, ireq.i_val);
	ireq.i_type = IEEE80211_IOC_RTSTHRESHOLD;
	return ieee80211_ioctl_set80211(vap, 0, &ireq);
}

uint8_t tx_power(int dbm)
{
	struct ieee80211vap *vap;
	struct ieee80211req ireq;
	vap = temp_vap;

	ireq.i_val = dbm;
	ireq.i_type = IEEE80211_IOC_TXPOWER;

//	printk("NL80211 : In %s and %d vap %p dbm %d\n",__func__,__LINE__, vap, dbm);
	return ieee80211_ioctl_set80211(vap, 0, &ireq);
}

int
onebox_set_if_media(struct net_device *dev, int media)
{
	struct ieee80211vap *vap = netdev_priv(dev);
	struct ifreq ifr;

//	printk("STAIOCTL: %s %d \n", __func__, __LINE__);
	memset(&ifr, 0, sizeof(ifr));
	ifr.ifr_ifru.ifru_ivalue = media;
	if(ifmedia_ioctl(vap->iv_ifp, (struct ifreq *)&ifr, &vap->iv_media, SIOCSIFMEDIA) < 0)
	{
		return ONEBOX_STATUS_FAILURE;
	}
	else
	{
		return ONEBOX_STATUS_SUCCESS;
	}
}

int
onebox_get_if_media(struct net_device *dev)
{
	struct ifmediareq ifmr;
	struct ieee80211vap *vap = netdev_priv(dev);

//	printk("STAIOCTL: %s %d \n", __func__, __LINE__);
	memset(&ifmr, 0, sizeof(ifmr));

	if(ifmedia_ioctl(vap->iv_ifp, (struct ifreq *)&ifmr, &vap->iv_media, SIOCGIFMEDIA) < 0)
	{
		return ONEBOX_STATUS_FAILURE;
	}
	return ifmr.ifm_current;
}

int
onebox_set_mediaopt(struct net_device *dev, uint32_t mask, uint32_t mode)
{
	int media = onebox_get_if_media(dev);

	if (media < 0)
	{
		printk("NL80211 : In %s and %d\n",__func__,__LINE__);
		return -1;
	}
	media &= ~mask;
	media |= mode;

	if (onebox_set_if_media(dev, media) < 0)
	{
		return ONEBOX_STATUS_FAILURE;
	}
	else
	{
		return ONEBOX_STATUS_SUCCESS;
	}
}

int
onebox_driver_nl80211_set_wpa(struct ieee80211vap *vap, int enabled)
{
	return onebox_driver_nl80211_set_wpa_internal(vap, enabled ? 3 : 0, enabled);
}

int
onebox_driver_nl80211_set_wpa_internal(struct ieee80211vap *vap, int wpa, int privacy)
{
	if (!wpa && onebox_prepare_ioctl_cmd(vap, IEEE80211_IOC_APPIE, NULL, IEEE80211_APPIE_WPA, 0) < 0)
	{
		return ONEBOX_STATUS_FAILURE;
	}

	if (onebox_prepare_ioctl_cmd(vap, IEEE80211_IOC_PRIVACY, NULL, privacy, 0) < 0)
	{
		return ONEBOX_STATUS_FAILURE;
	}

	if (onebox_prepare_ioctl_cmd(vap, IEEE80211_IOC_WPA, NULL, wpa, 0) < 0)
	{
		return ONEBOX_STATUS_FAILURE;
	}

	return ONEBOX_STATUS_SUCCESS;
}

#if 0
int nl80211_ctrl_iface(struct net_device *ndev, int enable)
{
	struct ifreq *ifr;
	memset(&ifr, 0, sizeof(ifr));

	os_nl_strlcpy(ifr->ifr_name, ndev->name, sizeof(ifr->ifr_name));
	if(enable)
	{
		ifr->ifr_flags |= IFF_UP;	
	}
	else
	{
		ifr->ifr_flags &= ~IFF_UP;	
	}
	
	if(ieee80211_ioctl(ndev, ifr, SIOCSIFFLAGS) < 0)
	{
		printk("Failed to set Ioctl SIOCSIFFLAGS\n");
		return -1;
	}
	return 0;
}
#endif

int cfg80211_scan(void *temp_req, struct net_device *ndev, uint8_t num_ch, uint16_t *chans, 
                     const uint8_t *ie, size_t ie_len, int n_ssids, void *ssid)
{
	struct ieee80211_ssid *ssids = (struct ieee80211_ssid *)ssid;
	struct ieee80211vap *vap = netdev_priv(ndev);
	struct ieee80211_scan_req sr;
	uint8_t i;
	temp_vap = vap;
	
	memset(&sr, 0, sizeof(sr));

	if(vap->iv_state == IEEE80211_S_RUN) //Fix made for bgscan
	{
		printk("NL80211 : In %s and %d \n" , __func__, __LINE__);
		scan_done(scan_request, NULL);
		return 0;
	}
	if(onebox_set_mediaopt(ndev, IFM_OMASK, 0))
	{
		printk("Failed to set operation mode\n");
		return -1;
	}
	
	if(onebox_set_mediaopt(ndev, IFM_MMASK, 0))
	{
		printk("Failed to set modulation mode\n");
		return -1;
	}

	if(onebox_prepare_ioctl_cmd(vap, IEEE80211_IOC_ROAMING, NULL, IEEE80211_ROAMING_AUTO, 0) < 0)
	{
		printk("Roaming Ioctl Not set\n");
		return -1;
	}

	if(onebox_driver_nl80211_set_wpa(vap, 1) < 0)
	{
		printk("Failed to set WPA\n");
		return -1;
	}

#if 0
	if(nl80211_ctrl_iface(ndev, 1) < 0)
	{
		printk("Failed to set interafce \n");
		return -1;
	}
#endif
	if(ie && ie_len)
	{
		if(onebox_prepare_ioctl_cmd(vap, IEEE80211_IOC_APPIE, ie, 
		                   (IEEE80211_FC0_TYPE_MGT | IEEE80211_FC0_SUBTYPE_PROBE_REQ), ie_len) < 0)
		{
			printk("IE Ioctl not set\n");
			return -1;
		}
	}
	sr.sr_flags = IEEE80211_IOC_SCAN_ACTIVE | IEEE80211_IOC_SCAN_ONCE | IEEE80211_IOC_SCAN_NOJOIN;
	sr.sr_duration = IEEE80211_IOC_SCAN_FOREVER;
	
	if(n_ssids > 0)
	{
		sr.sr_nssid = n_ssids;
		sr.sr_flags |= IEEE80211_IOC_SCAN_CHECK;
	}
	for (i = 0; i < sr.sr_nssid; i++) 
	{
		sr.sr_ssid[i].len = ssids->ssid_len;
		memcpy(sr.sr_ssid[i].ssid, ssids->ssid, sr.sr_ssid[i].len);
	}


	if(num_ch)
	{
		sr.num_freqs = num_ch;
		for(i = 0; i < num_ch; i++)
		{
			sr.freqs[i] = chans[i];
			printk("Scanning Selective channels mentioned by user %d\n", sr.freqs[i]);
		}
	}
	else
	{
		sr.num_freqs = 0;
		printk("Scanning All channels\n");
		
	}
	
	vap->scan_request = temp_req;	
	
	if(onebox_prepare_ioctl_cmd(vap, IEEE80211_IOC_SCAN_REQ, (void *)&sr, 0, sizeof(sr)) < 0)
	{
		return ONEBOX_STATUS_FAILURE;
	}
	else
	{
		return ONEBOX_STATUS_SUCCESS;
	}
}

size_t os_nl_strlcpy(char *dest, const char *src, size_t siz)
{
	const char *s = src;
	size_t left = siz;

	if (left) {
		/* Copy string up to the maximum size of the dest buffer */
		while (--left != 0) {
			if ((*dest++ = *s++) == '\0')
				break;
		}
	}

	if (left == 0) {
		/* Not enough room for the string; force NUL-termination */
		if (siz != 0)
			*dest = '\0';
		while (*s++)
			; /* determine total src string length */
	}

	return s - src - 1;
}


int cfg80211_inform_scan_results(void *arg, void *se_nl)
{
	struct ieee80211vap *vap;
	struct ieee80211_scan_entry *nl = (struct ieee80211_scan_entry *)se_nl;
	unsigned char ssid[6];
	unsigned short val, cap_info;
	//int signal;
	int chno;
	int binfo_len;
	struct ieee80211_channel *channel;
	struct wireless_dev *wdev;
	channel = NULL;
	vap = temp_vap;
	wdev = vap->wdev;
	nl80211_memcp(ssid, nl->se_bssid, 6);
	val = nl->se_intval;
	cap_info = nl->se_capinfo;
	channel = nl->se_chan;
//	signal = nl->se_rssi;
	chno = nl->se_chan->ic_ieee;
	binfo_len = nl->se_ies.len;
//	printk("NL80211 : In %s and %d vap %p temp_vap %p\n",__func__,__LINE__,vap, temp_vap);

//	printk("NL80211 : In %s and %d wdev %p ssid %s val %d cap_info %d binfo_len %d\n",__func__,__LINE__,wdev, ssid, val, cap_info, binfo_len);
	if(scan_results_sup(wdev, ssid, val, cap_info, channel, chno, nl->se_ies.data, binfo_len, nl->se_ssid) < 0)
	{
		return ONEBOX_STATUS_FAILURE;
	}
	else
	{
		return ONEBOX_STATUS_SUCCESS;
	}
}

int scan_completed(struct ieee80211vap *vap)
{
	if(vap->scan_request)
	{
		void *temp_scan_req;
		temp_scan_req = vap->scan_request;
		scan_done(temp_scan_req);
		vap->scan_request = NULL;
		return ONEBOX_STATUS_SUCCESS;
	}
	return -1;
}

void* nl80211_memcp(void *to, const void *from, int len)
{
	return memcpy(to, from, len);
}

int cfg80211_connect_result_vap(struct ieee80211vap *vap, uint8_t mac[6])
{
	struct wireless_dev *wdev = vap->wdev;
//	printk("NL80211 : In %s and %d wdev %p\n",__func__,__LINE__, wdev);
	connection_confirm(wdev, mac);
	return ONEBOX_STATUS_SUCCESS;
}

int cfg80211_disconnect_result_vap(struct ieee80211vap *vap)
{
	struct wireless_dev *wdev = vap->wdev;
	disconnection_confirm(wdev);
	return ONEBOX_STATUS_SUCCESS;
}

uint8_t onebox_wep_key(struct net_device *ndev, int index, uint8_t *mac_addr, uint8_t key_len, const uint8_t *key)
{
	struct ieee80211vap *vap = netdev_priv(ndev);
	struct ieee80211req_key wk;

	memset(&wk, 0, sizeof(wk));

//	printk("NL80211 : In %s LINE %d\n",__func__,__LINE__);
	
	wk.ik_type = IEEE80211_CIPHER_WEP;

	wk.ik_flags = IEEE80211_KEY_RECV ;
	wk.ik_flags |= IEEE80211_KEY_XMIT;

	/**need to set the flag based on transmission**/
	/*need to fix
	if(set_tx)
		wk.ik_flags |= IEEE80211_KEY_XMIT;*/

	printk("ASSOC CMD: In %s and %d mac_addr = %02x:%02x:%02x:%02x:%02x:%02x \n", __func__, __LINE__, mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
	mac_addr = NULL;
	if (mac_addr == NULL) 
	{
		memset(wk.ik_macaddr, 0xff, IEEE80211_ADDR_LEN);
		wk.ik_keyix = index;
	} 
	else 
	{
		memcpy(wk.ik_macaddr, mac_addr, IEEE80211_ADDR_LEN);
	}
	/*
	 * Deduce whether group/global or unicast key by checking
	 * the address (yech).  Note also that we can only mark global
	 * keys default; doing this for a unicast key is an error.
	 */
	if (memcmp(wk.ik_macaddr, "\xff\xff\xff\xff\xff\xff",
		      IEEE80211_ADDR_LEN) == 0) {
		wk.ik_flags |= IEEE80211_KEY_GROUP;
		wk.ik_keyix = index;
	} else {
		wk.ik_keyix = index == 0 ? IEEE80211_KEYIX_NONE :
			index;
	}
	//if (wk.ik_keyix != IEEE80211_KEYIX_NONE && set_tx)/* need to fix */
	if (wk.ik_keyix != IEEE80211_KEYIX_NONE )/* need to fix */
		wk.ik_flags |= IEEE80211_KEY_DEFAULT;

	wk.ik_keylen = key_len;
//	memcpy(&wk.ik_keyrsc, params->seq, params->seq_len);
	memcpy(wk.ik_keydata, key, key_len);

//	printk("NL80211 : In %s LINE %d\n",__func__,__LINE__);
	if(onebox_prepare_ioctl_cmd(vap, IEEE80211_IOC_WPAKEY, (void *)&wk, 0, sizeof(wk)) < 0)
	{
		return ONEBOX_STATUS_FAILURE;
	}
	else
	{
		return ONEBOX_STATUS_SUCCESS;
	}
}

int cfg80211_connect_res(struct net_device *ndev, int auth, const uint8_t *ie, size_t ie_len, unsigned char *ssid, 
                         size_t ssid_len, unsigned char *bssid, int len, int privacy)
{
	struct ieee80211vap *vap;
	struct ieee80211req_mlme mlme;
	int authmode;
	vap = netdev_priv(ndev);

	if(onebox_set_mediaopt(ndev, IFM_OMASK, 0) < 0)
	{
		printk("Failed to set Operation mode\n");
		return -1;
	}

	if ((auth & WPA_AUTH_ALG_OPEN) &&
	    (auth & WPA_AUTH_ALG_SHARED))
	{
		authmode = IEEE80211_AUTH_AUTO;
	}
	else if (auth & WPA_AUTH_ALG_SHARED)
	{
		authmode = IEEE80211_AUTH_SHARED;
	}
	else
	{
		authmode = IEEE80211_AUTH_OPEN;
	}

	if(onebox_prepare_ioctl_cmd(vap, IEEE80211_IOC_AUTHMODE, NULL, authmode, 0) < 0)
	{
		printk("Authmode Not set\n");
		return -1;
	}

	if(onebox_prepare_ioctl_cmd(vap, IEEE80211_IOC_APPIE, ie, IEEE80211_APPIE_WPA, ie_len) < 0)
	{
		printk("Appie Ioctl Not set\n");
		return -1;
	}
	
	if(onebox_prepare_ioctl_cmd(vap, IEEE80211_IOC_PRIVACY, NULL, privacy, 0) < 0)
	{
		printk("Privacy Ioctl Not set\n");
		return -1;
	}

	if((ie_len && onebox_prepare_ioctl_cmd(vap, IEEE80211_IOC_WPA, NULL, ie[0] == 48 ? 2 : 1, 0)) < 0)
	{
		printk("WPA Ioctl Not set\n");
		return -1;
	}

	if(((ssid != NULL) && onebox_prepare_ioctl_cmd(vap, IEEE80211_IOC_SSID, ssid, 0, ssid_len)) < 0)
	{
		printk("SSID Ioctl Not set\n");
		return -1;
	}
	
	memset(&mlme, 0, sizeof(mlme));
	mlme.im_op = IEEE80211_MLME_ASSOC;
	if(ssid != NULL)
	{
		memcpy(mlme.im_ssid, ssid, len);
	}
	if(bssid != NULL)
	{
		memcpy(mlme.im_macaddr, bssid, 6);
	}
	mlme.im_ssid_len = len;

	if(onebox_prepare_ioctl_cmd(vap, IEEE80211_IOC_MLME, &mlme, 0, sizeof(mlme)) < 0)
	{
		printk("MLME Ioctl Not set\n");
		return -1;
	}
	return 0;
}
#endif


