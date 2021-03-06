#include <linux/if_arp.h>
#include <net/iw_handler.h>

#include <net80211/ieee80211_var.h>
#include "mbuf.h"
#include "common.h"

#define MAC_ADDR(addr) \
  ((unsigned char *)(addr)) [0], \
  ((unsigned char *)(addr)) [1], \
  ((unsigned char *)(addr)) [2], \
  ((unsigned char *)(addr)) [3], \
  ((unsigned char *)(addr)) [4], \
  ((unsigned char *)(addr)) [5]

#define RTM_IEEE80211_ASSOC     100     /* station associate (bss mode) */
#define RTM_IEEE80211_REASSOC   101     /* station re-associate (bss mode) */
#define RTM_IEEE80211_DISASSOC  102     /* station disassociate (bss mode) */
#define RTM_IEEE80211_JOIN      103     /* station join (ap mode) */
#define RTM_IEEE80211_LEAVE     104     /* station leave (ap mode) */
#define RTM_IEEE80211_SCAN      105     /* scan complete, results available */
#define RTM_IEEE80211_REPLAY    106     /* sequence counter replay detected */
#define RTM_IEEE80211_MICHAEL   107     /* Michael MIC failure detected */
#define RTM_IEEE80211_REJOIN    108     /* station re-associate (ap mode) */
#define RTM_IEEE80211_WDS       109     /* WDS discovery (ap mode) */
#define RTM_IEEE80211_CSA       110     /* Channel Switch Announcement event */
#define RTM_IEEE80211_RADAR     111     /* radar event */
#define RTM_IEEE80211_CAC       112     /* Channel Availability Check event */
#define RTM_IEEE80211_DEAUTH    113     /* station deauthenticate */
#define RTM_IEEE80211_AUTH      114     /* station authenticate (ap mode) */
#define RTM_IEEE80211_COUNTRY   115     /* discovered country code (sta mode) */
#define RTM_IEEE80211_RADIO     116     /* RF kill switch state change */


/* These values should be equal with what are being used in suppliant for decoding */
#define DRIVER_EVENT_REMAIN_ON_CHANNEL        0
#define DRIVER_EVENT_CANCEL_REMAIN_ON_CHANNEL 1
#define DRIVER_EVENT_MGMT                     2
#define DRIVER_EVENT_ASSOC                    3

void
ieee80211_vap_destroy(struct ieee80211vap *vap)
{
	struct ieee80211com *ic = vap->iv_ic;

	printk("In %s Line %d delete vap\n", __func__, __LINE__);
	if(vap->iv_opmode == IEEE80211_M_STA) {
	ieee80211_cancel_scan(vap);
	}
	ic->ic_vap_delete(vap, 1);      
	printk("In %s Line %d delete vap\n", __func__, __LINE__);
	return;
}

/*
 * Allocate and setup a management frame of the specified
 * size.  We return the mbuf and a pointer to the start
 * of the contiguous data area that's been reserved based
 * on the packet length.  The data area is forced to 32-bit
 * alignment and the buffer length to a multiple of 4 bytes.
 * This is done mainly so beacon frames (that require this)
 * can use this interface too.
 */

//:w!typedef __kernel_size_t size_t;
struct mbuf *
ieee80211_getmgtframe(uint8_t **frm, int headroom, int pktlen)
{
	netbuf_ctrl_block_m_t *m = NULL;
	const u_int align = sizeof(u_int32_t);
	struct sk_buff *skb;
	u_int len;

	len = roundup(pktlen + headroom, 4);
	skb = dev_alloc_skb(len + align - 1);
	if (skb != NULL)
	{
		u_int off = ((unsigned long) skb->data) % align;
		if (off != 0)
			skb_reserve(skb, align - off);

		SKB_CB(skb)->auth_pkt = 0;
		SKB_CB(skb)->ni = NULL;
		SKB_CB(skb)->flags = 0;
		SKB_CB(skb)->next = NULL;

		/* FIXME: Ravi */
		skb_reserve(skb, headroom);
		*frm = skb_put(skb, pktlen);
		memset(skb->data, 0, skb->len);
		m = onebox_translate_skb_to_netcb_m(skb);
	}
	return m;
}

void
//get_random_bytes(void *p, size_t n)
get_random_bytes(void *p, int  n)
{
	uint8_t *dp = p;

	while (n > 0) {
		uint32_t v = arc4random();
		size_t nb = n > sizeof(uint32_t) ? sizeof(uint32_t) : n;
		bcopy(&v, dp, n > sizeof(uint32_t) ? sizeof(uint32_t) : n);
		dp += sizeof(uint32_t), n -= nb;
	}
}
#if 1
int
ieee80211_node_dectestref(struct ieee80211_node *ni)
{
  /* Return 0 if this is the last reference, else return 0 */
  if (atomic_dec_and_test(&ni->ni_refcnt))
  {
    atomic_inc(&ni->ni_refcnt);		  
    return 1;
  }
  else
  {
    return 0;
  }
}
#endif
/*
 * Helper function for events that pass just a single mac address.
 */
static void
notify_macaddr(struct ifnet *ifp, int op, const uint8_t mac[IEEE80211_ADDR_LEN])
{
        union iwreq_data wrqu;

	memset(&wrqu, 0, sizeof(wrqu));
        wrqu.ap_addr.sa_family = ARPHRD_ETHER;

	IEEE80211_ADDR_COPY(wrqu.ap_addr.sa_data, mac);
        wireless_send_event(ifp, SIOCGIWAP, &wrqu, NULL);
}

void
ieee80211_notify_node_join(struct ieee80211_node *ni, int newassoc)
{
	struct ieee80211vap *vap = ni->ni_vap;
#ifndef ONEBOX_CONFIG_CFG80211
	struct net_device *dev = vap->iv_ifp;
	union iwreq_data wreq;
	char local_buf[MAX_MGMT_PKT_SIZE + 4];		/* XXX */
	uint16_t buf_len = 0;

	memset(local_buf, 0, 6);
#if 0
	if (ni == vap->iv_bss)
		return;
#endif
	memset(&wreq, 0, sizeof(wreq));
	wreq.addr.sa_family = ARPHRD_ETHER;

	*(uint16_t *)local_buf = DRIVER_EVENT_ASSOC;
	buf_len = sizeof(uint16_t);

	*(uint16_t *)&local_buf[buf_len] = newassoc ? 0 : 1; /* Sending wether reassoc or not */
	buf_len += sizeof(uint16_t);

	if (ni == vap->iv_bss) 	{
		netif_carrier_on(vap->iv_ifp);
		IEEE80211_ADDR_COPY(&local_buf[buf_len], ni->ni_bssid);
		buf_len += IEEE80211_ADDR_LEN;
	} else {
		IEEE80211_ADDR_COPY(&local_buf[buf_len], ni->ni_macaddr);
		buf_len += IEEE80211_ADDR_LEN;

		*(uint16_t *)&local_buf[buf_len] = ni->ni_ies.len;
		buf_len += sizeof(uint16_t);

		memcpy(&local_buf[buf_len], ni->ni_ies.data, ni->ni_ies.len);
		buf_len += ni->ni_ies.len;
	}
	wreq.data.length = buf_len;

	wireless_send_event(dev, IWEVCUSTOM, &wreq, local_buf);
#else
//	printk("NL80211 : In %s and %d\n",__func__,__LINE__);
	cfg80211_connect_result_vap(vap, ni->ni_macaddr);
#endif
}

void
ieee80211_notify_node_leave(struct ieee80211_node *ni)
{
	struct ieee80211vap *vap = ni->ni_vap;
#ifndef ONEBOX_CONFIG_CFG80211
        struct net_device *dev = vap->iv_ifp;
        union iwreq_data wreq;
	
	//struct ifnet *ifp;	
	#if 0
        if (ni == vap->iv_bss) {
	        return;
	}// else {
	//	notify_macaddr(ifp, RTM_IEEE80211_LEAVE, ni->ni_macaddr);
	//}
			#endif

		printk("disconnection event to supplicant\n");
        /* fire off wireless event station leaving */
        memset(&wreq, 0, sizeof(wreq));
        IEEE80211_ADDR_COPY(wreq.addr.sa_data, ni->ni_macaddr);
        wreq.addr.sa_family = ARPHRD_ETHER;
        wireless_send_event(dev, IWEVEXPIRED, &wreq, NULL);
#else
		cfg80211_disconnect_result_vap(vap);
#endif

/*	IEEE80211_NOTE(vap, IEEE80211_MSG_NODE, ni, "%snode leave",
	    (ni == vap->iv_bss) ? "bss " : "");

	if (ni == vap->iv_bss) {
		rt_ieee80211msg(ifp, RTM_IEEE80211_DISASSOC, NULL, 0);
		if_link_state_change(ifp, LINK_STATE_DOWN);
	} else {
		// fire off wireless event station leaving 
		notify_macaddr(ifp, RTM_IEEE80211_LEAVE, ni->ni_macaddr);
	}*/
}

void
ieee80211_notify_scan_done(struct ieee80211vap *vap)
{
#ifndef ONEBOX_CONFIG_CFG80211	
	struct net_device *dev = vap->iv_ifp;
	struct ieee80211com *ic = vap->iv_ic;
	union iwreq_data wreq;	
	struct ieee80211_scan_state *ss = ic->ic_scan;
	int result;

	IEEE80211_DPRINTF(vap, IEEE80211_MSG_SCAN, "%s\n", "notify scan done");
	
	wreq.data.length = 0;
	wreq.data.flags =0;
	if(vap->iv_state == IEEE80211_S_RUN)
	{
		//st->st_newscan = 1;
		result = ss->ss_ops->scan_end(ss, vap);
		//sta_update_notseen(st);
	}
	wireless_send_event(dev, SIOCGIWSCAN, &wreq, NULL);
#else
	struct ieee80211req ireq;
	if(vap->scan_request)
	{
		ireq.i_type = IEEE80211_IOC_SCAN_RESULTS;
		ireq.i_len = 24*1024;
		ieee80211_ioctl_get80211(vap, 0, &ireq);
	}
#endif
}
	
void
ieee80211_notify_replay_failure(struct ieee80211vap *vap,
	const struct ieee80211_frame *wh, const struct ieee80211_key *k,
	u_int64_t rsc, int tid)
{
	struct ifnet *ifp = vap->iv_ifp;
        static const char *tag = "MLME-REPLAYFAILURE.indication";
        union iwreq_data wrqu;
        char buf[128];          /* XXX */

        IEEE80211_NOTE_MAC(vap, IEEE80211_MSG_CRYPTO, wh->i_addr2,
                "%s replay detected <keyix %d, rsc %llu >",
                k->wk_cipher->ic_name, k->wk_keyix,
                (unsigned long long)rsc);

        /* TODO: needed parameters: count, keyid, key type, src address, TSC */
        snprintf(buf, sizeof(buf), "%s(keyid=%d %scast addr=" MAC_FMT ")", tag, 
                k->wk_keyix,
                IEEE80211_IS_MULTICAST(wh->i_addr2) ?  "broad" : "uni",
                MAC_ADDR(wh->i_addr2));
        memset(&wrqu, 0, sizeof(wrqu));
        wrqu.data.length = strlen(buf);
        wireless_send_event(ifp, IWEVCUSTOM, &wrqu, buf);
	return;
}

void
ieee80211_notify_michael_failure(struct ieee80211vap *vap,
	const struct ieee80211_frame *wh, u_int keyix)
{
	static const char *tag = "MLME-MICHAELMICFAILURE.indication";
	struct net_device *dev = vap->iv_ifp;
	union iwreq_data wrqu;
	char buf[128];		/* XXX */

	IEEE80211_NOTE_MAC(vap, IEEE80211_MSG_CRYPTO, wh->i_addr2,
		"Michael MIC verification failed <keyix %d>", keyix);
	vap->iv_stats.is_rx_tkipmic++;

	/* TODO: needed parameters: count, keyid, key type, src address, TSC */
	snprintf(buf, sizeof(buf), "%s(keyid=%d %scast addr=" MAC_FMT ")", tag,
		keyix, IEEE80211_IS_MULTICAST(wh->i_addr2) ?  "broad" : "uni",
		MAC_ADDR(wh->i_addr2));
	memset(&wrqu, 0, sizeof(wrqu));
	wrqu.data.length = strlen(buf);
	wireless_send_event(dev, IWEVCUSTOM, &wrqu, buf);
}

void
ieee80211_notify_wds_discover(struct ieee80211_node *ni)
{
	struct ieee80211vap *vap = ni->ni_vap;
	struct ifnet *ifp = vap->iv_ifp;

	notify_macaddr(ifp, RTM_IEEE80211_WDS, ni->ni_macaddr);
}

void
ieee80211_notify_csa(struct ieee80211com *ic,
	const struct ieee80211_channel *c, int mode, int count)
{
	struct ifnet *ifp = ic->ic_ifp;
	struct ieee80211_csa_event iev;

	memset(&iev, 0, sizeof(iev));
	iev.iev_flags = c->ic_flags;
	iev.iev_freq = c->ic_freq;
	iev.iev_ieee = c->ic_ieee;
	iev.iev_mode = mode;
	iev.iev_count = count;
	rt_ieee80211msg(ifp, RTM_IEEE80211_CSA, &iev, sizeof(iev));
}

void
ieee80211_notify_radar(struct ieee80211com *ic,
	const struct ieee80211_channel *c)
{
	struct ifnet *ifp = ic->ic_ifp;
	struct ieee80211_radar_event iev;
	memset(&iev, 0, sizeof(iev));
	iev.iev_flags = c->ic_flags;
	iev.iev_freq = c->ic_freq;
	iev.iev_ieee = c->ic_ieee;
	rt_ieee80211msg(ifp, RTM_IEEE80211_RADAR, &iev, sizeof(iev));
}

void
ieee80211_notify_cac(struct ieee80211com *ic,
	const struct ieee80211_channel *c, enum ieee80211_notify_cac_event type)
{
	struct ifnet *ifp = ic->ic_ifp;
	struct ieee80211_cac_event iev;
	memset(&iev, 0, sizeof(iev));
	iev.iev_flags = c->ic_flags;
	iev.iev_freq = c->ic_freq;
	iev.iev_ieee = c->ic_ieee;
	iev.iev_type = type;
	rt_ieee80211msg(ifp, RTM_IEEE80211_CAC, &iev, sizeof(iev));
}

void
ieee80211_notify_node_deauth(struct ieee80211_node *ni)
{
	struct ieee80211vap *vap = ni->ni_vap;
	struct ifnet *ifp = vap->iv_ifp;

	IEEE80211_NOTE(vap, IEEE80211_MSG_NODE, ni, "%s", "node deauth");

	notify_macaddr(ifp, RTM_IEEE80211_DEAUTH, ni->ni_macaddr);
}

void
ieee80211_notify_node_auth(struct ieee80211_node *ni)
{
	struct ieee80211vap *vap = ni->ni_vap;
	struct ifnet *ifp = vap->iv_ifp;

	IEEE80211_NOTE(vap, IEEE80211_MSG_NODE, ni, "%s", "node auth");

	notify_macaddr(ifp, RTM_IEEE80211_AUTH, ni->ni_macaddr);
}

void
ieee80211_notify_country(struct ieee80211vap *vap,
	const uint8_t bssid[IEEE80211_ADDR_LEN], const uint8_t cc[2])
{
	struct ifnet *ifp = vap->iv_ifp;
	struct ieee80211_country_event iev;
	memset(&iev, 0, sizeof(iev));
	IEEE80211_ADDR_COPY(iev.iev_addr, bssid);
	iev.iev_cc[0] = cc[0];
	iev.iev_cc[1] = cc[1];
	rt_ieee80211msg(ifp, RTM_IEEE80211_COUNTRY, &iev, sizeof(iev));
}

void
ieee80211_notify_radio(struct ieee80211com *ic, int state)
{
	return;
}

#ifdef ENABLE_P2P_SUPPORT
void send_remain_on_channel_event(struct ieee80211vap *vap, uint16_t freq, uint16_t duration)
{
	struct net_device *dev = vap->iv_ifp;
	union iwreq_data wrqu;
	char local_buf[6];		/* XXX */
	uint16_t buf_len;

	memset(local_buf, 0, 6);

	*(uint16_t *)local_buf = DRIVER_EVENT_REMAIN_ON_CHANNEL;
	buf_len = sizeof(uint16_t);

	*(uint16_t *)&local_buf[buf_len] = freq;
	buf_len += sizeof(uint16_t);

	*(uint16_t *)&local_buf[buf_len] = duration;
	buf_len += sizeof(uint16_t);

	memset(&wrqu, 0, sizeof(wrqu));
	wrqu.data.length = buf_len;

	wireless_send_event(dev, IWEVCUSTOM, &wrqu, local_buf);
}

void send_cancel_remain_on_channel_event(struct ieee80211vap *vap, uint16_t freq)
{
	struct net_device *dev = vap->iv_ifp;
	union iwreq_data wrqu;
	char local_buf[4];		/* XXX */
	uint16_t buf_len;

	memset(local_buf, 0, 4);

	*(uint16_t *)local_buf = DRIVER_EVENT_CANCEL_REMAIN_ON_CHANNEL;
	buf_len = sizeof(uint16_t);

	*(uint16_t *)&local_buf[buf_len] = freq;
	buf_len += sizeof(uint16_t);

	memset(&wrqu, 0, sizeof(wrqu));
	wrqu.data.length = buf_len;

	wireless_send_event(dev, IWEVCUSTOM, &wrqu, local_buf);
}

void notify_recv_mgmt_pkt(struct ieee80211vap *vap, uint8_t *buf, uint16_t len, uint16_t recv_freq)
{
	struct net_device *dev = vap->iv_ifp;
	union iwreq_data wrqu;
	char local_buf[MAX_MGMT_PKT_SIZE + 4];		/* XXX */
	uint16_t buf_len;

	memset(local_buf, 0, (MAX_MGMT_PKT_SIZE + 4));

	*(uint16_t *)local_buf = DRIVER_EVENT_MGMT;
	buf_len = sizeof(uint16_t);

	*(uint16_t *)&local_buf[buf_len] = recv_freq;
	buf_len += sizeof(uint16_t);

	memcpy(&local_buf[buf_len], buf, len);
	buf_len += len;

	memset(&wrqu, 0, sizeof(wrqu));
	wrqu.data.length = buf_len;

	wireless_send_event(dev, IWEVCUSTOM, &wrqu, local_buf);
}
#endif

/* static char *version = RELEASE_VERSION; */

MODULE_AUTHOR("Errno Consulting, Sam Leffler");
MODULE_DESCRIPTION("802.11 wireless LAN protocol support");
#ifdef MODULE_VERSION
MODULE_VERSION(RELEASE_VERSION);
#endif
#ifdef MODULE_LICENSE
MODULE_LICENSE("Dual BSD/GPL");
#endif

extern  void ieee80211_auth_setup(void);

static int __init
init_wlan(void)
{
        return 0;
}
module_init(init_wlan);

static void __exit
exit_wlan(void)
{
}
module_exit(exit_wlan);

EXPORT_SYMBOL(ieee80211_notify_michael_failure);
EXPORT_SYMBOL(ieee80211_notify_replay_failure);
EXPORT_SYMBOL(ieee80211_notify_country);
EXPORT_SYMBOL(ieee80211_notify_scan_done);
