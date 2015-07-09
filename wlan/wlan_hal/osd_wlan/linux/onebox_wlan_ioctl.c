/**
 * @file onebox_hal_ioctl.c
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
 * This file contians the code for handling ioctls.
 */

#include "onebox_common.h"

/**
 *  This function will return index of a given ioctl command.
 *  And the output parameter private indicates whether the given
 *  ioctl is a standard ioctl command or a private ioctl.
 *   0: Standard
 *   1: Private ioctl
 *  -1: Illiegal ioctl
 *
 * @param  value of the ioctl command, input to this function
 * @param  Indicates whether the ioctl is private or standart, output pointer
 * @return returns index of the ioctl
 */

static uint32 last_total_beacon_count;
static int get_ioctl_index(int cmd, int *private)
{
	int index = 0;
	*private = 0;

	if ( (cmd >= SIOCIWFIRSTPRIV) && (cmd <= SIOCIWLASTPRIV)) 
	{
		/* Private IOCTL */
		index = cmd - SIOCIWFIRSTPRIV;
		*private = 1;
	} 
	else if ((cmd >= 0x8B00) && (cmd <= 0x8B2D)) 
	{
		/* Standard IOCTL */
		index = cmd - 0x8B00;
		*private = 0;
	} 
	else 
	{
		*private = -1;
	}
	return index;
}

/**
 * This function handles the ioctl for deleting a VAP.
 * @param  Pointer to the ieee80211com structure
 * @param  Pointer to the ifreq structure
 * @param  Pointer to the netdevice structure
 * @return Success or failure  
 */
int
ieee80211_ioctl_delete_vap(struct ieee80211com *ic, struct ifreq *ifr, struct net_device *mdev)
{
	struct ieee80211vap *vap = NULL;
	struct ieee80211_clone_params cp;
	char name[IFNAMSIZ];
	uint8_t wait_for_lock = 0;
	uint8_t vap_del_flag = 0;
	//PONEBOX_ADAPTER adapter = (PONEBOX_ADAPTER)netdev_priv(mdev);

	if (!capable(CAP_NET_ADMIN))
	{
		return -EPERM;
	}

	if (copy_from_user(&cp, ifr->ifr_data, sizeof(cp)))
	{
		ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("Copy from user failed\n")));
		return -EFAULT;
	}

	strncpy(name, cp.icp_parent, sizeof(name));

	TAILQ_FOREACH(vap, &ic->ic_vaps, iv_next) 
	{
		printk("User given vap name =%s and list of vap names=%s\n", name, vap->iv_ifp->name);
		if (!strcmp(vap->iv_ifp->name, name)) 
		{
			//adapter->net80211_ops->onebox_ifdetach(ic);
			printk("Deleting vap\n");
			ic->ic_vap_delete(vap, wait_for_lock);
			vap_del_flag =1;
			break;
		}
	}
	if(vap_del_flag)
	{
			return 0;
	}
	else
	{
		printk("Invalid VAP name given for deletion\n");
		return -1;
	}
}
/* timeout handler for channel utilization*/
void ch_util_timeout_handler(PONEBOX_ADAPTER adapter)
{
	struct ieee80211com *ic = &adapter->vap_com;

	/* stop time // calculate total time taken // start time again*/
	ic->ch_util->stop_time = jiffies; 
	adapter->ch_util_tot_time = ic->ch_util->tot_time = (ic->ch_util->stop_time - ic->ch_util->start_time);  
	ic->ch_util->start_time = jiffies;
	/*modify timer */
	adapter->os_intf_ops->onebox_mod_timer(&adapter->channel_util_timeout, msecs_to_jiffies(1000));
}

/**
 * This function creates a virtual ap.This is public as it must be
 * implemented outside our control (e.g. in the driver).
 * @param  Pointer to the ieee80211com structure
 * @param  Pointer to the ifreq structure
 * @param  Pointer to the netdevice structure
 * @return Success or failure  
 */
int ieee80211_ioctl_create_vap(struct ieee80211com *ic, struct ifreq *ifr,
                                struct net_device *mdev)
{
	struct ieee80211_clone_params cp;
	struct ieee80211vap *vap;
	char name[IFNAMSIZ];
	PONEBOX_ADAPTER adapter = (PONEBOX_ADAPTER)netdev_priv(mdev);
	//uint8 vap_id;

	if (!capable(CAP_NET_ADMIN))
	{
		return -EPERM;
	}
	if (copy_from_user(&cp, ifr->ifr_data, sizeof(cp)))
	{
		ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("Copy from user failed\n")));
		return -EFAULT;
	}

	strncpy(name, cp.icp_parent, sizeof(name));
#if 0
	vap_id = adapter->os_intf_ops->onebox_extract_vap_id(name);
	TAILQ_FOREACH(vap, &ic->ic_vaps, iv_next)
	{
		if(vap && (vap->hal_priv_vap->vap_id == vap_id)) {
			ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("Virtual Interface with similar name is already created\n")));
			return -EFAULT;
		}
	}
#endif
	printk("Name for vap creation is: %s rtnl %d\n", name, rtnl_is_locked());
	/* FIXME */ //Check 3,6,7 param whether it should be 0 or not
	vap = adapter->core_ops->onebox_create_vap(ic, name, 0, cp.icp_opmode, cp.icp_flags, NULL, adapter->mac_addr);
	if (vap == NULL)
	{
		printk("VAP = NULL\n");
		return -EIO;
	}


	/* return final device name */
	strncpy(ifr->ifr_name, vap->iv_ifp->name, IFNAMSIZ);
	return 0;
}

bool check_valid_bgchannel(uint16 *data_ptr, uint8_t supported_band)
{
	uint8_t ii, jj;
	uint8_t num_chan = *((uint8 *)(data_ptr) + 6) ;
	uint16_t chan_5g[] = {36, 40, 44, 48, 149, 153, 157, 161, 165};
	uint16_t chan_check[num_chan];

	memcpy(chan_check, (uint16 *)(data_ptr + 6), 2*num_chan);

	if (!supported_band) {
		for (ii = 0; ii < num_chan; ii++) {
			for (jj = 0; jj < num_chan; jj++) {
				if (chan_check[ii] == chan_5g[jj]) {
					printk("ERROR: Trying to program 5GHz channel on a card supporting only 2.4GHz\n");
					return false;
				}
			}
		}
	}

	return true;
}

static void send_sleep_req_in_per_mode(PONEBOX_ADAPTER adapter, uint8 *data)
{
		onebox_mac_frame_t *mgmt_frame;
		ONEBOX_STATUS status = 0;
		uint8  pkt_buffer[MAX_MGMT_PKT_SIZE];
		struct pwr_save_params ps_params_ioctl;//Parameters to store IOCTL parameters from USER
#ifndef USE_USB_INTF
		uint8 request =1;
#endif

		mgmt_frame = (onebox_mac_frame_t *)pkt_buffer;
		memcpy(&ps_params_ioctl, data, sizeof(struct pwr_save_params));

		adapter->os_intf_ops->onebox_memset(mgmt_frame, 0, MAX_MGMT_PKT_SIZE);
		mgmt_frame->desc_word[0] = ONEBOX_CPU_TO_LE16(sizeof(mgmt_frame->u.ps_req_params) | (ONEBOX_WIFI_MGMT_Q << 12));
		mgmt_frame->desc_word[1] = ONEBOX_CPU_TO_LE16(WAKEUP_SLEEP_REQUEST);

		mgmt_frame->u.ps_req_params.ps_req.sleep_type = ps_params_ioctl.sleep_type; //LP OR ULP
		mgmt_frame->u.ps_req_params.listen_interval = ps_params_ioctl.listen_interval;
		mgmt_frame->u.ps_req_params.ps_req.sleep_duration = ps_params_ioctl.deep_sleep_wakeup_period;
		mgmt_frame->u.ps_req_params.ps_req.ps_en = ps_params_ioctl.ps_en;
		mgmt_frame->u.ps_req_params.ps_req.connected_sleep = DEEP_SLEEP;
						
		if(!ps_params_ioctl.ps_en) {
			mgmt_frame->desc_word[0] |= 1 << 15; //IMMEDIATE WAKE UP
		}

		ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (" <==== Sending Power save request =====> In %s Line %d  \n", __func__, __LINE__));
		status = adapter->devdep_ops->onebox_send_internal_mgmt_frame(adapter,
						(uint16 *)mgmt_frame,
						FRAME_DESC_SZ + sizeof(mgmt_frame->u.ps_req_params));
#ifndef USE_USB_INTF
			msleep(2);
			printk("Writing disable to wakeup register\n");
			status = adapter->osd_host_intf_ops->onebox_write_register(adapter,
											0,
											SDIO_WAKEUP_REG,
											&request);	 
#endif

		return ;
}

/**
 *  Calls the corresponding (Private) IOCTL functions
 *
 * @param  pointer to the net_device
 * @param  pointer to the ifreq
 * @param  value of the ioctl command, input to this function
 * @return returns 0 on success otherwise returns the corresponding 
 * error code for failure
 */
int onebox_ioctl(struct net_device *dev,struct ifreq *ifr, int cmd)
{
	PONEBOX_ADAPTER adapter = (PONEBOX_ADAPTER)netdev_priv(dev);
	struct ieee80211com *ic = &adapter->vap_com;
	struct iwreq *wrq = (struct iwreq *)ifr;
	int index, priv, ret_val=0;
	struct ieee80211_node *ni = NULL;
	struct ieee80211vap *vap = NULL;
	uint8_t macaddr[IEEE80211_ADDR_LEN];
	int error;
	struct minstrel_node *ar_stats = NULL;
	unsigned int value = 0;
	unsigned int channel = 1;
	unsigned int set_band = BAND_2_4GHZ;
	unsigned int status;
	onebox_mac_frame_t *mgmt_frame;
	struct test_mode test;
  	ONEBOX_DEBUG(ONEBOX_ZONE_OID, ("In onebox_ioctl function\n"));
	/* Check device is present or not */
	if (!netif_device_present(dev))
	{
		printk("Device not present\n");
		return -ENODEV;
	}

	/* Get the IOCTL index */
	index = get_ioctl_index(cmd, &priv);

	/*vap creation command*/
	switch(cmd)
	{
		case RSI_WATCH_IOCTL:
		{
			wrq->u.data.length = 4;
			if (adapter->buffer_full)
			{
				adapter->watch_bufferfull_count++;

				if (adapter->watch_bufferfull_count > 10) /* FIXME : not 10, should dependent on time */
				{
					/* Incase of continous buffer full, give the last beacon counter */
					ret_val = copy_to_user(wrq->u.data.pointer, &last_total_beacon_count, 4);
					return ret_val;
				}
			}

			last_total_beacon_count = adapter->total_beacon_count;
			ret_val = copy_to_user(wrq->u.data.pointer, &last_total_beacon_count, 4);
			return ret_val;
		} 
		break;
		case ONEBOX_VAP_CREATE:
		{
			if ((adapter->Driver_Mode != WIFI_MODE_ON) && (adapter->Driver_Mode != SNIFFER_MODE))
			{
				printk("Driver Mode is not in WIFI_MODE vap creation is not Allowed\n");	
				return ONEBOX_STATUS_FAILURE;
			}
			ONEBOX_DEBUG(ONEBOX_ZONE_INFO, (" VAP Creation \n"));
			ret_val = ieee80211_ioctl_create_vap(ic, ifr, dev);

			if(ret_val == 0)
			{
				ONEBOX_DEBUG(ONEBOX_ZONE_INFO, (TEXT("Created VAP with dev name:%s\n"),ifr->ifr_name));
			}
			return ret_val;
		}
		break;
		case ONEBOX_VAP_DELETE:
		{
			ONEBOX_DEBUG(ONEBOX_ZONE_INFO, (" VAP delete \n"));
			ret_val = ieee80211_ioctl_delete_vap(ic, ifr, dev);
			if(ret_val == 0)
			{
				ONEBOX_DEBUG(ONEBOX_ZONE_INFO, (TEXT("Deleted VAP with dev name:%s\n"),ifr->ifr_name));
			}
			return ret_val;
		}
#define IS_RUNNING(ifp) \
((ifp->if_flags & IFF_UP) && (ifp->if_drv_flags & IFF_DRV_RUNNING))
		case SIOCSIFFLAGS:
		{
			printk("In SIOCSIFFLAGS case dev->flags =%x\n", dev->if_flags);
			/* Not doing anything here */
			if (IS_RUNNING(dev)) 
			{
				/* Nothing to be done here */
			} 
			else if (dev->if_flags & IFF_UP) 
			{
				dev->if_drv_flags |= IFF_DRV_RUNNING;
				ieee80211_start_all(ic);
			} 
			else 
			{
				dev->if_drv_flags &= ~IFF_DRV_RUNNING;
			} 
			return ret_val;
		}
		break;
		case SIOCGAUTOSTATS:
		{
			error = copy_from_user(macaddr, wrq->u.data.pointer, IEEE80211_ADDR_LEN);
			if (error != 0)
			{
				return error;
			}
			TAILQ_FOREACH(vap, &ic->ic_vaps, iv_next) 
			{ 
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT(" mac addr=%02x:%02x:%02x:%02x:%02x:%02x\n"), macaddr[0], 
				macaddr[1], macaddr[2], macaddr[3], macaddr[4], macaddr[5]));
				ni = adapter->net80211_ops->onebox_find_node(&vap->iv_ic->ic_sta, macaddr);
				if (ni == NULL)
				{    
					ONEBOX_DEBUG(ONEBOX_ZONE_OID, (TEXT("Ni is null vap node not found\n")));
					return ENOENT;
				}
			}
			ONEBOX_DEBUG(ONEBOX_ZONE_OID, (TEXT(" Found VAP node\n")));
			if((&ni->hal_priv_node) && ni->hal_priv_node.ni_mn)
			{
				ONEBOX_DEBUG(ONEBOX_ZONE_OID, (TEXT("Autorate Minstrel node pointer = %p\n"), ni->hal_priv_node.ni_mn));
				error = copy_to_user( wrq->u.data.pointer, ni->hal_priv_node.ni_mn, sizeof(struct minstrel_node));
				ar_stats = ni->hal_priv_node.ni_mn;
				error = copy_to_user( wrq->u.data.pointer + sizeof(struct minstrel_node), 
				                      ni->hal_priv_node.ni_mn->r, 
				                      sizeof(struct minstrel_rate) * (ar_stats->n_rates));
			}
		}
		break;
		case ONEBOX_HOST_IOCTL:
		{
			if(adapter->Driver_Mode == WIFI_MODE_ON)
			{
				value = wrq->u.data.length;
				switch((unsigned char)wrq->u.data.flags)
				{
					case PER_RECEIVE_STOP:
						adapter->recv_stop = 1;
						adapter->rx_running = 0;
						ONEBOX_DEBUG(ONEBOX_ZONE_INFO,("PER_RECEIVE_STOP\n"));
					case PER_RECEIVE:
						adapter->os_intf_ops->onebox_reset_event(&(adapter->stats_event));
						if (!adapter->rx_running)
						{
							if(!(adapter->core_ops->onebox_stats_frame(adapter)))
							{
								adapter->rx_running = 1;
								if (adapter->recv_stop)
								{
									adapter->recv_stop = 0;
									adapter->rx_running = 0;
									return ONEBOX_STATUS_SUCCESS;
								}  
								adapter->os_intf_ops->onebox_wait_event(&(adapter->stats_event), EVENT_WAIT_FOREVER);
								ret_val = copy_to_user(wrq->u.data.pointer, &adapter->sta_info, sizeof(per_stats));
								return ret_val;
							}
						}
						else
						{
							adapter->os_intf_ops->onebox_wait_event(&(adapter->stats_event), EVENT_WAIT_FOREVER);
							ret_val = copy_to_user(wrq->u.data.pointer, &adapter->sta_info, sizeof(per_stats));
							return ret_val;
						}
						break;
					case SET_BEACON_INVL:
						if (IEEE80211_BINTVAL_MIN_AP <= value &&
								value <= IEEE80211_BINTVAL_MAX) 
						{
							ic->ic_bintval = ((value + 3) & ~(0x3));
						} 
						else
						{
							ret_val = EINVAL;
						}
						adapter->beacon_interval = ONEBOX_CPU_TO_LE16(((value + 3) & ~(0x3)));
						break;
					case SET_ENDPOINT:
						value = ((unsigned short)wrq->u.data.flags >> 8); //endpoint value 
						printk("ENDPOINT type is : %d \n",value);
						if (!adapter->band_supported) {
							if (value == 2 || value == 3) {
								printk("ERROR: 5GHz endpoint not supported\n");
								return -EINVAL;
							}
						}
						adapter->endpoint = value;
						adapter->devdep_ops->onebox_program_bb_rf(adapter);
						break;
					case ANT_SEL:
						value = ((unsigned short)wrq->u.data.flags >> 8); //endpoint value 
						printk("ANT_SEL value is : %d \n",value);
						adapter->devdep_ops->onebox_program_ant_sel(adapter, value);
						break;
					case SET_BGSCAN_PARAMS:
						ONEBOX_DEBUG(ONEBOX_ZONE_INFO, (TEXT("<<< BGSCAN >>>\n")));
//						onebox_send_bgscan_params(adapter, wrq->u.data.pointer , 0);
						TAILQ_FOREACH(vap, &ic->ic_vaps, iv_next) 
						{ 
							ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT(" mac addr=%02x:%02x:%02x:%02x:%02x:%02x\n"), macaddr[0], 
										macaddr[1], macaddr[2], macaddr[3], macaddr[4], macaddr[5]));
							if(vap->iv_opmode == IEEE80211_M_STA)
							{
								if (!check_valid_bgchannel(wrq->u.data.pointer, adapter->band_supported)) {
									printk("Invalid channel in bg param set; 5GHz not supported by card\n");
									return -EINVAL;
								}

								memset(&vap->hal_priv_vap->bgscan_params_ioctl, 0, sizeof(mgmt_frame->u.bgscan_params));
								memcpy(&vap->hal_priv_vap->bgscan_params_ioctl, wrq->u.data.pointer, sizeof(mgmt_frame->u.bgscan_params));
								vap->hal_priv_vap->bgscan_params_ioctl.bg_ioctl = 1;
								if(vap->iv_state == IEEE80211_S_RUN && (!adapter->sta_mode.delay_sta_support_decision_flag))
								{
									printk("Bgscan: In %s and %d \n", __func__, __LINE__);
									if(onebox_send_bgscan_params(vap, wrq->u.data.pointer , 0))
                  {
                    return ONEBOX_STATUS_FAILURE;
                  }
								}
								else
								{
									printk("Bgscan: In %s and %d \n", __func__, __LINE__);
									return 0;
								}
								ni = adapter->net80211_ops->onebox_find_node(&vap->iv_ic->ic_sta, vap->iv_myaddr);
								if (ni == NULL)
								{    
									ONEBOX_DEBUG(ONEBOX_ZONE_OID, (TEXT("Ni is null vap node not found\n")));
									return ENOENT;
								}

								send_bgscan_probe_req(adapter, ni, 0);
								return 0;
							}
						}
						printk("Issue IOCTL after vap creation in %s Line %d\n", __func__, __LINE__);
						return -EINVAL;
						break;
					case DO_BGSCAN:
							ONEBOX_DEBUG(ONEBOX_ZONE_INFO, (TEXT("<<< DO BGSCAN IOCTL Called >>>\n")));
							TAILQ_FOREACH(vap, &ic->ic_vaps, iv_next) 
							{ 
								if(vap && (vap->iv_opmode == IEEE80211_M_STA))
								{
									ni = adapter->net80211_ops->onebox_find_node(&vap->iv_ic->ic_sta, vap->iv_myaddr);
									if (ni == NULL)
									{    
										ONEBOX_DEBUG(ONEBOX_ZONE_OID, (TEXT("Ni is null vap node not found\n")));
										return ENOENT;
									}
									if(vap && (vap->hal_priv_vap->bgscan_params_ioctl.bg_ioctl))
									{
										memcpy(&vap->hal_priv_vap->bgscan_params_ioctl.bg_cmd_flags, wrq->u.data.pointer, sizeof(uint16_t));
										if((vap->iv_state == IEEE80211_S_RUN))
										{
											send_bgscan_probe_req(adapter, ni, vap->hal_priv_vap->bgscan_params_ioctl.bg_cmd_flags);
										}
										return 0;
									}
									else
									{
										printk("Issue this IOCTL only after issuing bgscan_params ioctl\n");
										return -EINVAL;
									}
								}
							}
							printk("Issue IOCTL after vap creation in %s Line %d\n", __func__, __LINE__);
							return -EINVAL;

						break;
					case BGSCAN_SSID:
						ONEBOX_DEBUG(ONEBOX_ZONE_INFO, (TEXT("<<< BGSCAN SSID >>>\n")));
						TAILQ_FOREACH(vap, &ic->ic_vaps, iv_next) 
						{ 
							ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT(" mac addr=%02x:%02x:%02x:%02x:%02x:%02x\n"), macaddr[0], 
							macaddr[1], macaddr[2], macaddr[3], macaddr[4], macaddr[5]));
							if(vap->iv_opmode == IEEE80211_M_STA)
							{
								memcpy(&vap->hal_priv_vap->bg_ssid,  wrq->u.data.pointer, sizeof(vap->hal_priv_vap->bg_ssid));
								printk("SSID len is %d ssid is %s\n", vap->hal_priv_vap->bg_ssid.ssid_len, vap->hal_priv_vap->bg_ssid.ssid);
							}
						}

						break;
#ifdef PWR_SAVE_SUPPORT
						case PS_REQUEST:
						{
							printk("Name for vap creation is rtnl %d\n",  rtnl_is_locked());
							printf("In %s Line %d issued PS_REQ ioctl\n", __func__, __LINE__);
							TAILQ_FOREACH(vap, &ic->ic_vaps, iv_next) 
							{ 
								if(vap->iv_opmode == IEEE80211_M_STA){
									break;
								}
							}
							if(vap)
							{
								memcpy(&vap->hal_priv_vap->ps_params_ioctl, wrq->u.data.pointer, sizeof(vap->hal_priv_vap->ps_params));
								printk("monitor interval %d\n", vap->hal_priv_vap->ps_params_ioctl.monitor_interval);
								driver_ps.update_ta = 1;
								
								
								update_pwr_save_status(vap, PS_ENABLE, IOCTL_PATH);


#if 0
								if(ps_params_def.ps_en)
								{
									driver_ps.en = 1;
								}
								else
								{
									driver_ps.en = 0;
								}
#endif

#if 0
#ifndef USE_USB_INTF
								if((vap->iv_state == IEEE80211_S_RUN) || (vap->iv_state == IEEE80211_S_INIT))
#else
								if((vap->iv_state == IEEE80211_S_RUN))
#endif
								{
								//	adapter->devdep_ops->onebox_send_ps_params(adapter, &vap->hal_priv_vap->ps_params);
									//pwr_sve_event_handler(vap);
								}
#endif
							}
							else
							{
													ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("ERROR: Give IOCTL after station vap Creation\n")));
													return -EINVAL;
							}
							break;
						}
						case UAPSD_REQ:
						{
							TAILQ_FOREACH(vap, &ic->ic_vaps, iv_next) 
							{ 
								if(vap->iv_opmode == IEEE80211_M_STA){
									break;
								}
							}
							if(vap && (vap->iv_state == IEEE80211_S_RUN))
							{
								memcpy(&vap->hal_priv_vap->uapsd_params_ioctl, wrq->u.data.pointer, sizeof(vap->hal_priv_vap->uapsd_params_ioctl));
							}
							else if(vap)
							{
								memcpy(&vap->hal_priv_vap->uapsd_params_ioctl, wrq->u.data.pointer, sizeof(vap->hal_priv_vap->uapsd_params_ioctl));
								printk("acs %02x wakeup %02x \n", vap->hal_priv_vap->uapsd_params_ioctl.uapsd_acs, vap->hal_priv_vap->uapsd_params_ioctl.uapsd_wakeup_period);
								memcpy(&vap->hal_priv_vap->uapsd_params_updated, &vap->hal_priv_vap->uapsd_params_ioctl, sizeof(vap->hal_priv_vap->uapsd_params_ioctl));
							}
							else
							{
								printk("Give IOCTL after vap Creation\n");
								return -EINVAL;
							}
							break;
						}
#endif
						case RESET_ADAPTER:
						{
							TAILQ_FOREACH(vap, &ic->ic_vaps, iv_next) 
							{ 
								if(vap->iv_opmode == IEEE80211_M_STA){
									break;
								}
							}
							if(vap)
							{
#if 0
								printk("Copying ioctl values to updated \n");
								if((&vap->hal_priv_vap->uapsd_params_ioctl) && (&vap->hal_priv_vap->uapsd_params_updated))
								{
									memcpy(&vap->hal_priv_vap->uapsd_params_updated, &vap->hal_priv_vap->uapsd_params_ioctl, sizeof(vap->hal_priv_vap->uapsd_params_ioctl));
								}
#endif
								printk("Resetting the Adapter settings \n");
								ni = vap->iv_bss; 
								if(ni && (vap->iv_state == IEEE80211_S_RUN))
								{
									printk("Issuing sta leave cmd\n");
									adapter->net80211_ops->onebox_ieee80211_sta_leave(ni);
								}
							}
							break;
						}
						case RX_FILTER:
						{
							memcpy(&adapter->rx_filter_word,wrq->u.data.pointer,sizeof(adapter->rx_filter_word));
							printk("Setting RX_FILTER %04x\n", adapter->rx_filter_word);
   							status = onebox_send_rx_filter_frame(adapter, adapter->rx_filter_word);
							if(status < 0)
							{
								printk("Sending of RX filter frame failed\n");
							}
							break;
						}
						case RF_PWR_MODE:
						{
							memcpy(&adapter->rf_pwr_mode, wrq->u.data.pointer, sizeof(adapter->rf_pwr_mode));	
							printk("Setting RF PWR MODE %04x\n", adapter->rf_pwr_mode);
							printk("Setting RF PWR MODE %d\n", adapter->rf_pwr_mode);
							break;
						}
						case RESET_PER_Q_STATS:
						{
							int q_num;
							ONEBOX_DEBUG(ONEBOX_ZONE_INFO,("Resetting WMM stats\n"));
							if (copy_from_user(&q_num, ifr->ifr_data, sizeof(q_num)))
							{
								ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("Copy from user failed\n")));
								return -EFAULT;
							}
							if(q_num < MAX_HW_QUEUES) {
								adapter->total_tx_data_dropped[q_num] = 0;
								adapter->total_tx_data_sent[q_num] = 0;
							}else if(q_num == 15) {
								memset(adapter->total_tx_data_dropped, 0, sizeof(adapter->total_tx_data_dropped));
								memset(adapter->total_tx_data_sent, 0, sizeof(adapter->total_tx_data_sent));
							
							} else {
								ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("INVALID Q_NUM\n")));
								return -EFAULT;
					
							}
							/* FIXME: Reset all the queue stats for now. Individual queue stats for AP/STA needs to be
							 * modified.
							 */
							/* Reset Station queue stats */
#if 0
							adapter->total_sta_data_vo_pkt_send = 0;
							adapter->total_sta_vo_pkt_freed = 0;
							adapter->total_sta_data_vi_pkt_send = 0;
							adapter->total_sta_vi_pkt_freed = 0;
							adapter->total_sta_data_be_pkt_send = 0;
							adapter->total_sta_be_pkt_freed = 0;
							adapter->total_sta_data_bk_pkt_send = 0;
							adapter->total_sta_bk_pkt_freed = 0;

							/* Reset AP queue stats */
							adapter->total_ap_data_vo_pkt_send = 0;
							adapter->total_ap_vo_pkt_freed = 0;
							adapter->total_ap_data_vi_pkt_send = 0;
							adapter->total_ap_vi_pkt_freed = 0;
							adapter->total_ap_data_be_pkt_send = 0;
							adapter->total_ap_be_pkt_freed = 0;
							adapter->total_ap_data_bk_pkt_send = 0;
							adapter->total_ap_bk_pkt_freed = 0;

							adapter->tx_vo_dropped = 0;
							adapter->tx_vi_dropped = 0;
							adapter->tx_be_dropped = 0;
							adapter->tx_bk_dropped = 0;
#endif
						

			 		adapter->buf_semi_full_counter = 0;
			 		adapter->buf_full_counter = 0;
					adapter->no_buffer_fulls = 0;

#if 0
							switch(qnum)
							{
								case VO_Q:
									adapter->total_data_vo_pkt_send = 0;
									adapter->total_vo_pkt_freed = 0;
									break;
								case VI_Q:
									adapter->total_data_vi_pkt_send = 0;
									adapter->total_vi_pkt_freed = 0;
									break;
								case BE_Q:
									adapter->total_data_be_pkt_send = 0;
									adapter->total_be_pkt_freed = 0;
									break;
								case BK_Q:
									adapter->total_data_bk_pkt_send = 0;
									adapter->total_bk_pkt_freed = 0;
									break;
								default:
									adapter->total_data_vo_pkt_send = 0;
									adapter->total_vo_pkt_freed = 0;
									adapter->total_data_vi_pkt_send = 0;
									adapter->total_vi_pkt_freed = 0;
									adapter->total_data_be_pkt_send = 0;
									adapter->total_be_pkt_freed = 0;
									adapter->total_data_bk_pkt_send = 0;
									adapter->total_bk_pkt_freed = 0;
									break;
							}
#endif
						}
					break;
					case AGGR_LIMIT:
						if(copy_from_user(&adapter->aggr_limit, wrq->u.data.pointer, wrq->u.data.length))
						{
							ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT(" Aggr Params Copying Failed\n")));
							return -EINVAL;
						}
						printk("%s: Aggr params set are tx=%d rx=%d\n", __func__, adapter->aggr_limit.tx_limit, adapter->aggr_limit.rx_limit);
						TAILQ_FOREACH(vap, &ic->ic_vaps, iv_next) 
						{ 
							if(vap->iv_opmode == IEEE80211_M_STA){
								break;
							}
						}
						if(vap)
						{
							vap->hal_priv_vap->aggr_rx_limit = adapter->aggr_limit.rx_limit;
						}
	 				break;
					case MASTER_READ:
					{
						printk("performing master read\n");
						if(adapter->devdep_ops->onebox_do_master_ops(adapter, wrq->u.data.pointer, ONEBOX_MASTER_READ) != ONEBOX_STATUS_SUCCESS)
						{
							ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT(" Data Read Failed\n")));
						}
					}
	 				break;
					case MASTER_WRITE:
					{
						printk("performing master write \n");
						if(adapter->devdep_ops->onebox_do_master_ops(adapter, wrq->u.data.pointer, ONEBOX_MASTER_WRITE) != ONEBOX_STATUS_SUCCESS)
						{
							ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT(" Data Write Failed\n")));
						}
					}
	 				break;
					case TEST_MODE:
					{
						printk("starting in test mode");
						memcpy(&test, wrq->u.data.pointer, sizeof(test));
						if(adapter->devdep_ops->onebox_send_debug_frame(adapter, &test) != ONEBOX_STATUS_SUCCESS)
						{
							ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT(" Sending Debug frame Failed\n")));
						}
					}
	 				break;
          case SET_COUNTRY:
          switch(value)
          {
            /* Countries in US Region */
            case CTRY_BELGIUM:
              ic->ic_regdomain.location = ' ';
              ic->ic_regdomain.isocc[0] = 'B';		
              ic->ic_regdomain.isocc[1] = 'G';		
              ic->ic_regdomain.pad[0] = 0;			//Just an indication of the country, to refer index in regdm_table[]
              break;
            case CTRY_CANADA:
              ic->ic_regdomain.location = ' ';		
              ic->ic_regdomain.isocc[0] = 'C';		
              ic->ic_regdomain.isocc[1] = 'A';		
              ic->ic_regdomain.pad[0] = 0;			
              break;
            case CTRY_MEXICO:				
              ic->ic_regdomain.location = ' ';
              ic->ic_regdomain.isocc[0] = 'M';	
              ic->ic_regdomain.isocc[1] = 'X';	
              ic->ic_regdomain.pad[0] = 0;			
              break;
            case CTRY_UNITED_STATES:  
              ic->ic_regdomain.location = ' ';
              ic->ic_regdomain.isocc[0] = 'U';
              ic->ic_regdomain.isocc[1] = 'S';
              ic->ic_regdomain.pad[0] = 0;			
              break;

              /* Countries in EU Region */
            case CTRY_FRANCE:
              ic->ic_regdomain.location = ' ';	
              ic->ic_regdomain.isocc[0] = 'F';	
              ic->ic_regdomain.isocc[1] = 'R';	
              ic->ic_regdomain.pad[0] = 1;			
              break;
            case CTRY_GERMANY:
              ic->ic_regdomain.location = ' ';	
              ic->ic_regdomain.isocc[0] = 'G';	
              ic->ic_regdomain.isocc[1] = 'R';	
              ic->ic_regdomain.pad[0] = 1;			
              break;
            case CTRY_ITALY:
              ic->ic_regdomain.location = ' ';
              ic->ic_regdomain.isocc[0] = 'I';	
              ic->ic_regdomain.isocc[1] = 'T';	
              ic->ic_regdomain.pad[0] = 1;			
              break;

              /* Countries in Japan Region */
            case CTRY_JAPAN:
              ic->ic_regdomain.location = ' ';	
              ic->ic_regdomain.isocc[0] = 'J';	
              ic->ic_regdomain.isocc[1] = 'P';
              ic->ic_regdomain.pad[0] = 2;			
              break;

              /* Countries in Rest of the World Region */

            case CTRY_AUSTRALIA:
              ic->ic_regdomain.location = ' ';
              ic->ic_regdomain.isocc[0] = 'A';	
              ic->ic_regdomain.isocc[1] = 'U';	
              ic->ic_regdomain.pad[0] = 3;			
              break;
            case CTRY_INDIA:
              ic->ic_regdomain.location = ' ';
              ic->ic_regdomain.location = ' ';	
              ic->ic_regdomain.isocc[0] = 'I';	
              ic->ic_regdomain.isocc[1] = 'N';	
              ic->ic_regdomain.pad[0] = 3;			
              break;
            case CTRY_IRAN:
              ic->ic_regdomain.location = ' ';
              ic->ic_regdomain.isocc[0] = 'I';	
              ic->ic_regdomain.isocc[1] = 'R';	
              ic->ic_regdomain.pad[0] = 3;			
              break;
            case CTRY_MALAYSIA:
              ic->ic_regdomain.location = ' ';
              ic->ic_regdomain.isocc[0] = 'M';	
              ic->ic_regdomain.isocc[1] = 'L';	
              ic->ic_regdomain.pad[0] = 3;			
              break;
            case CTRY_NEW_ZEALAND:			
              ic->ic_regdomain.location = ' ';
              ic->ic_regdomain.isocc[0] = 'N';	
              ic->ic_regdomain.isocc[1] = 'Z';	
              ic->ic_regdomain.pad[0] = 3;			
              break;
            case CTRY_RUSSIA:
              ic->ic_regdomain.location = ' ';
              ic->ic_regdomain.isocc[0] = 'R';	
              ic->ic_regdomain.isocc[1] = 'S';	
              ic->ic_regdomain.pad[0] = 3;			
              break;
            case CTRY_SINGAPORE:
              ic->ic_regdomain.location = ' ';
              ic->ic_regdomain.isocc[0] = 'S';	
              ic->ic_regdomain.isocc[1] = 'G';	
              ic->ic_regdomain.pad[0] = 3;			
              break;
            case CTRY_SOUTH_AFRICA:
              ic->ic_regdomain.location = ' ';
              ic->ic_regdomain.isocc[0] = 'S';	
              ic->ic_regdomain.isocc[1] = 'A';	
              ic->ic_regdomain.pad[0] = 3;			
              break;
            default:  
              ic->ic_regdomain.location = ' ';
              ic->ic_regdomain.isocc[0] = '\0';
              ic->ic_regdomain.isocc[1] = '\0';
              ic->ic_regdomain.pad[0] = 3;			
          }
          printk("In %s Line %d *********Recvd IOCTL code %d \n", __func__, __LINE__, value);
          adapter->net80211_ops->onebox_media_init(ic);
          break;

					default:
						ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,("No match yet\n"));
						return -EINVAL;
						break;
				}
			}
			else if(adapter->Driver_Mode == SNIFFER_MODE)
			{
					switch((unsigned char)wrq->u.data.flags)
					{
						case PER_RECEIVE:
							adapter->endpoint_params.per_ch_bw = *(uint8 *)wrq->u.data.pointer;
							adapter->recv_channel = (uint8)(wrq->u.data.flags >> 8);
							adapter->endpoint_params.channel = adapter->recv_channel;
							printk("In %s and %d ch_width %d recv_channel %d \n", __func__, __LINE__, adapter->endpoint_params.per_ch_bw, adapter->recv_channel);
							adapter->devdep_ops->onebox_band_check(adapter);
							adapter->devdep_ops->onebox_set_channel(adapter,adapter->recv_channel);
							return ret_val;
							break;
#ifdef TRILITHIC_RELEASE
						case CH_UTIL_START:
							if(adapter->ch_util_start_flag == 0){
								if(copy_from_user(ic->ch_util, wrq->u.data.pointer, wrq->u.data.length))
								{
									ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("Copying Failed\n")));
									return -EINVAL;
								}
								++adapter->ch_util_start_flag;
								ONEBOX_DEBUG(ONEBOX_ZONE_INFO,("**** Channel Utilization test Started ****\n"));
							}
							else
							{
								if(adapter->ch_util_start_flag == 1){
									ic->ch_util->start_time = jiffies;
									ic->ch_util->crc_pkt = 0;
									adapter->os_intf_ops->onebox_init_sw_timer(&adapter->channel_util_timeout, (uint32)adapter,(void *)&ch_util_timeout_handler, msecs_to_jiffies(1000));
									++adapter->ch_util_start_flag;
								}
								//msleep(1000);
								ic->ch_util->tot_time = adapter->ch_util_tot_time;
								//printk("%s: %d %lu\n",__func__,__LINE__,ic->ch_util->tot_time);
								if(copy_to_user(wrq->u.data.pointer, ic->ch_util, wrq->u.data.length))
								{
									ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("Copying Failed with error \n")));
									return -EINVAL;
								}
								ic->ch_util->tot_on_air_occupancy = 0;
								ic->ch_util->on_air_occupancy = 0;
								ic->ch_util->tot_time = 0;
								ic->ch_util->tot_len_of_pkt = 0;
								ic->ch_util->tot_length_of_pkt = 0;
								adapter->ch_util_tot_time = 0;
							}
							break;
						case CH_UTIL_STOP:
							adapter->ch_util_start_flag = 0;
							adapter->os_intf_ops->onebox_memset(ic->ch_util, 0, sizeof(struct ch_utils));
							adapter->os_intf_ops->onebox_remove_timer(&adapter->channel_util_timeout);
							ONEBOX_DEBUG(ONEBOX_ZONE_INFO,("**** Channel Utilization test Stopped ****\n"));
							break;
#endif
						default:
							printk("Failed: in %d case\n",(unsigned char)wrq->u.data.flags);
							ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,("No match yet\n"));
							return -EINVAL;
							break;
					}
			}
			else 
			{
				switch((unsigned char)wrq->u.data.flags)
				{
					case PER_TRANSMIT:
						if(copy_from_user(&adapter->endpoint_params, wrq->u.data.pointer, wrq->u.data.length))
						{
							ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("Copying Failed\n")));
							return -EINVAL;
						}
						if(adapter->core_ops->onebox_start_per_tx(adapter))
						{
							ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("Invalid arguments issued by user\n")));
							return -EINVAL;
						}
						return 0;
						break;
					case PER_RECEIVE_STOP:
						adapter->recv_stop = 1;
						adapter->rx_running = 0;
						ONEBOX_DEBUG(ONEBOX_ZONE_INFO,("PER_RECEIVE_STOP\n"));
					case PER_RECEIVE:
						ONEBOX_DEBUG(ONEBOX_ZONE_INFO,("per_ch_bw :%d \n",adapter->endpoint_params.per_ch_bw));
						adapter->endpoint_params.per_ch_bw = *(uint8 *)wrq->u.data.pointer;
						adapter->recv_channel = (uint8)(wrq->u.data.flags >> 8);
						adapter->endpoint_params.channel = adapter->recv_channel;
						if (adapter->endpoint_params.channel == 0xFF)
						{
							if(adapter->devdep_ops->onebox_mgmt_send_bb_reset_req(adapter) != ONEBOX_STATUS_SUCCESS) {
								return ONEBOX_STATUS_FAILURE;
							}
						}
						else if (adapter->endpoint_params.channel)
						{
							adapter->os_intf_ops->onebox_reset_event(&(adapter->bb_rf_event));
							adapter->devdep_ops->onebox_band_check(adapter);
							adapter->devdep_ops->onebox_set_channel(adapter,adapter->recv_channel);
							adapter->fsm_state = FSM_SCAN_CFM;		
							adapter->os_intf_ops->onebox_wait_event(&(adapter->bb_rf_event), EVENT_WAIT_FOREVER);
							adapter->fsm_state = FSM_MAC_INIT_DONE;
						}
						adapter->os_intf_ops->onebox_reset_event(&(adapter->stats_event));
						if (!adapter->rx_running)
						{
							if(!(adapter->core_ops->onebox_stats_frame(adapter)))
							{
								adapter->rx_running = 1;
								if (adapter->recv_stop)
								{
									adapter->recv_stop = 0;
									adapter->rx_running = 0;
									return ONEBOX_STATUS_SUCCESS;
								}  
								adapter->os_intf_ops->onebox_wait_event(&(adapter->stats_event), EVENT_WAIT_FOREVER);
								ret_val = copy_to_user(wrq->u.data.pointer, &adapter->sta_info, sizeof(per_stats));
								return ret_val;
							}
						}
						else
						{
							adapter->os_intf_ops->onebox_wait_event(&(adapter->stats_event), EVENT_WAIT_FOREVER);
							ret_val = copy_to_user(wrq->u.data.pointer, &adapter->sta_info, sizeof(per_stats));
							return ret_val;
						}
						break;
					case PER_PACKET:
						if(copy_from_user(&adapter->per_packet, wrq->u.data.pointer, wrq->u.data.length))
						{
							ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("Copying PER Packet Failed in %s\n"), __func__));
							return -EINVAL;
						}
						ONEBOX_DEBUG(ONEBOX_ZONE_DEBUG, (TEXT("Copying PER Packet in %s\n"), __func__));
						adapter->core_ops->onebox_dump(ONEBOX_ZONE_DEBUG, (PUCHAR)&adapter->per_packet.packet, adapter->per_packet.length);
						return 0;
						break;
					case ANT_SEL:
						value = ((unsigned short)wrq->u.data.flags >> 8); //endpoint value 
						printk("ANT_SEL value is : %d \n",value);
						adapter->devdep_ops->onebox_program_ant_sel(adapter, value);
						break;
					case SET_ENDPOINT:
						value = ((unsigned short)wrq->u.data.flags >> 8); //endpoint value 
						printk("ENDPOINT type is : %d \n",value);
						adapter->endpoint = value;
#ifdef PROGRAMMING_BBP_TA	
						adapter->devdep_ops->onebox_program_bb_rf(adapter);
#else
            			if (value == 0)
						{
							adapter->endpoint_params.per_ch_bw = 0;
							adapter->endpoint_params.channel = 1;
						}
            			else if (value == 1)
						{
							adapter->endpoint_params.per_ch_bw = 6;
							adapter->endpoint_params.channel = 1;
						}
						else if (value == 2)
						{
							adapter->endpoint_params.per_ch_bw = 0;
							adapter->endpoint_params.channel = 36;
						}
						else if (value == 3)
						{
							adapter->endpoint_params.per_ch_bw = 6;
							adapter->endpoint_params.channel = 36;
						}
						adapter->devdep_ops->onebox_band_check(adapter);
#endif
						break;
          case EEPROM_READ_IOCTL:
          {
            ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,("ONEBOX_IOCTL: EEPROM READ  \n"));
	          adapter->os_intf_ops->onebox_memset(&adapter->eeprom, 0, sizeof(EEPROMRW));
            if(copy_from_user(&adapter->eeprom, wrq->u.data.pointer, 
                              (sizeof(EEPROMRW) - sizeof(adapter->eeprom.data))))
            {
              ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("Copying Failed\n")));
              return -EINVAL;
            }

            status = adapter->devdep_ops->onebox_eeprom_rd(adapter);
            if (status == ONEBOX_STATUS_SUCCESS)
            {
              adapter->os_intf_ops->onebox_reset_event(&(adapter->bb_rf_event));
              adapter->os_intf_ops->onebox_wait_event(&(adapter->bb_rf_event), 10000); 
						  ONEBOX_DEBUG(ONEBOX_ZONE_DEBUG, (TEXT(" eeprom length: %d, \n"), adapter->eeprom.length));
						  ONEBOX_DEBUG(ONEBOX_ZONE_DEBUG, (TEXT(" eeprom offset: %d, \n"), adapter->eeprom.offset));
              ret_val = copy_to_user(wrq->u.data.pointer, &adapter->eeprom, sizeof(EEPROMRW));
              return ret_val;
            }
            else
            {	
              return ONEBOX_STATUS_FAILURE;
            }
          }
          break;
#if 0
        case EEPROM_WRITE_IOCTL:
          {
            ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,("ONEBOX_IOCTL: EEPROM WRITE  \n"));
#if 1
            if(copy_from_user(&adapter->eeprom, wrq->u.data.pointer, wrq->u.data.length))
            {
              ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("Copying Failed\n")));
              return -EINVAL;
            }
#endif
            adapter->eeprom.length = (wrq->u.data.length - 10);
            ONEBOX_DEBUG(ONEBOX_ZONE_INFO,
                (TEXT("===> Frame to WRITE IN TO EEPROM <===\n")));

            mgmt_frame = (onebox_mac_frame_t *)pkt_buffer;

            adapter->os_intf_ops->onebox_memset(mgmt_frame, 0, FRAME_DESC_SZ);

            /* FrameType*/
            mgmt_frame->desc_word[1] = ONEBOX_CPU_TO_LE16(EEPROM_WRITE);
            mgmt_frame->desc_word[0] = (ONEBOX_WIFI_MGMT_Q << 12 | adapter->eeprom.length);
            if (!adapter->eeprom_erase)
            {
              mgmt_frame->desc_word[2] = ONEBOX_CPU_TO_LE16(BIT(10));
              adapter->eeprom_erase = 1;
            }  
            /* Number of bytes to read*/
            mgmt_frame->desc_word[3] = ONEBOX_CPU_TO_LE16(adapter->eeprom.length);
            /* Address to read*/
            mgmt_frame->desc_word[4] = ONEBOX_CPU_TO_LE16(adapter->eeprom.offset);
            mgmt_frame->desc_word[5] = ONEBOX_CPU_TO_LE16(adapter->eeprom.offset >> 16);
          	adapter->os_intf_ops->onebox_memcpy(mgmt_frame->u.byte.buf, adapter->eeprom.data, adapter->eeprom.length);
  
            adapter->core_ops->onebox_dump(ONEBOX_ZONE_ERROR, (PUCHAR)mgmt_frame, FRAME_DESC_SZ + adapter->eeprom.length);
            status = adapter->osi_host_intf_ops->onebox_host_intf_write_pkt(adapter,
                (uint8 *)mgmt_frame,
                FRAME_DESC_SZ + adapter->eeprom.length);
            if (status == ONEBOX_STATUS_SUCCESS)
            {
              adapter->os_intf_ops->onebox_wait_event(&(adapter->bb_rf_event), 10000); 
              adapter->os_intf_ops->onebox_memset(adapter->eeprom.data, 0, 480);
              return ONEBOX_STATUS_SUCCESS;
            }
            else
            {	
              return ONEBOX_STATUS_FAILURE;
            }
          }
          break;
#endif
						case PS_REQUEST:
						{
									if((adapter->Driver_Mode == RF_EVAL_MODE_ON)) {
										
										send_sleep_req_in_per_mode(adapter, wrq->u.data.pointer);

									}
							break;
						}
						case RF_PWR_MODE:
						{
							memcpy(&adapter->rf_pwr_mode, wrq->u.data.pointer, sizeof(adapter->rf_pwr_mode));	
							adapter->rf_power_mode_change = 1;
							printk("Setting RF PWR MODE %04x\n", adapter->rf_pwr_mode);
							break;
						}

					default:
						ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,("No match yet\n"));
						return -EINVAL;
						break;
				}
			}
			return ret_val;
		}
		break;
		case ONEBOX_SET_BB_RF:
		{
//			if(adapter->Driver_Mode == RF_EVAL_MODE_ON)
			{
				if(copy_from_user(&adapter->bb_rf_params.Data[0], wrq->u.data.pointer, (sizeof(adapter->bb_rf_params))))
				{
					ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("Copying Failed\n")));
					return -EINVAL;
				}
				if(adapter->devdep_ops->onebox_set_bb_rf_values(adapter, wrq))
				{
					ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("Invalid arguments issued by user\n")));
					return -EINVAL;
				}
				return ONEBOX_STATUS_SUCCESS;
			}	
		}
		break;
		case ONEBOX_SET_CW_MODE:
		{
			if(adapter->Driver_Mode == RF_EVAL_MODE_ON)
      		{
        		channel = (uint8 )wrq->u.data.flags;
				if (!channel)
					channel = 1;
        		if(channel <= 14)
        		{
          			set_band = BAND_2_4GHZ;
					adapter->endpoint = 0;
       			}
        		else if((channel >= 36) && (channel <= 165))
        		{
          			set_band = BAND_5GHZ;
					adapter->endpoint = 2;
        		}
        		if (adapter->operating_band != set_band)
        		{
          			adapter->operating_band = set_band;
          			adapter->rf_reset = 1;
					adapter->devdep_ops->onebox_program_bb_rf(adapter);
        		}
        		adapter->devdep_ops->onebox_set_channel(adapter,channel);
				adapter->fsm_state = FSM_SCAN_CFM;		
				adapter->os_intf_ops->onebox_reset_event(&(adapter->bb_rf_event));
				adapter->os_intf_ops->onebox_wait_event(&(adapter->bb_rf_event), EVENT_WAIT_FOREVER);
				adapter->fsm_state = FSM_MAC_INIT_DONE;

				channel = (unsigned int)wrq->u.data.flags; //cw_type & subtype info
				adapter->cw_type	=	(channel & 0x0f00) >> 8 ;
				adapter->cw_sub_type	=	(channel & 0xf000) >> 12 ;
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("ONEBOX_IOCTL:  SET_CW_MODE , cw_type:%d cw_mode:%d channel : %d\n"),adapter->cw_sub_type,adapter->cw_type, channel));
				if(adapter->devdep_ops->onebox_cw_mode(adapter, adapter->cw_sub_type))
				{
				  ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("Invalid arguments issued by user\n")));
				  return -EINVAL;
				}
				break;
				return ONEBOX_STATUS_SUCCESS;
		      }	
		}
		break;
		default:
		{
			if (priv == 0) /* req is a standard ioctl */ 
			{
				ONEBOX_DEBUG(ONEBOX_ZONE_INFO, ("Standard ioctl: %d\n", index));
			} 
			else /* Ignore it, Bad ioctl */ 
			{
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, ("Unrecognised ioctl: %d\n", cmd));
			}
			return -EFAULT;
		}
	}
	return ret_val;
}


