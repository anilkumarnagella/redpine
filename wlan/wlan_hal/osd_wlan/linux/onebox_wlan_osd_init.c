/**
 * @file onebox_osd_main.c
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
 * This file contains all the Linux network device specific code.
 */
#include "onebox_common.h"
#include "onebox_linux.h"
#include "onebox_sdio_intf.h"

static uint32 tcp_csumoffld_enable =0;
static uint8 device_mac_addr[6] = {0x00, 0x23, 0xa7, 0x1b, 0x52, 0x3a};

static PONEBOX_ADAPTER adapter;
/**
 * Allocate & initialize the network device.This function
 * allocates memory for the network device & initializes 
 * it with ethernet generic values.
 *
 * @param  size of the priv area to be allocated.  
 * @return Pointer to the network device structure. 
 */
static struct net_device* onebox_allocdev(int32 sizeof_priv)
{
	struct net_device *dev = NULL;

	dev = alloc_netdev(sizeof_priv, "rpine%d", ether_setup);
	return dev;
}/* End of <onebox_allocdev> */

/**
 * This function register the network device.
 *
 * @param  Pointer to our network device structure.  
 * @return On success 0 is returned else a negative value. 
 */
static int32 onebox_registerdev(struct net_device *dev)
{
	return register_netdev(dev);
}/* End of <onebox_registerdev> */

/**
 * This function unregisters the network device & returns it 
 * back to the kernel.
 *
 * @param  Pointer to our network device structure.  
 * @return VOID. 
 */
VOID unregister_dev(struct net_device *dev)
{
	unregister_netdev(dev);
	free_netdev(dev);
	return;
} /* End of <onebox_unregisterdev> */

/**
 * This function gives the statistical information regarding 
 * the interface.
 *
 * @param  Pointer to our network device strucure.  
 * @return Pointer to the net_device_stats structure . 
 */
static struct net_device_stats* onebox_get_stats(struct net_device *dev)
{
	PONEBOX_ADAPTER Adapter = netdev_priv(dev);
	return &Adapter->stats;
} /* End of <onebox_get_stats> */

/**
 * This function performs all net device related operations
 * like allocating,initializing and registering the netdevice.
 *
 * @param  VOID.  
 * @return On success 0 is returned else a negative value. 
 */
struct net_device* wlan_netdevice_op(VOID)
{    
	struct net_device *dev; 

#if KERNEL_VERSION_BTWN_2_6_(18, 27)
	/*Allocate & initialize the network device structure*/  
	dev = onebox_allocdev(sizeof(ONEBOX_ADAPTER));    
	if (dev == NULL)
	{
		ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,
		             (TEXT("%s:Failure in allocation of net-device\n"),__func__));
		return dev;
	}

	dev->open                 = device_open;
	dev->stop                 = device_close;
	dev->hard_start_xmit      = onebox_xmit;
	dev->get_stats            = onebox_get_stats;   
	dev->do_ioctl             = onebox_ioctl;
	//dev->wireless_handlers    = &onebox_handler_def;  
	dev->hard_header_len      = FRAME_DESC_SZ + RSI_DESCRIPTOR_SZ; /* 64 + 16 */

	if (tcp_csumoffld_enable)
	{
		dev->features |= NETIF_F_IP_CSUM;
	}
	if (onebox_registerdev(dev) != 0)
	{
		ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,
		             (TEXT("%s: Registration of net-device failed\n"), __func__));
		free_netdev(dev);
		return NULL;      
	}
#elif KERNEL_VERSION_GREATER_THAN_2_6_(27)
	static const struct net_device_ops onebox_netdev_ops =
	{
		.ndo_open           = device_open,
		.ndo_stop           = device_close,
		.ndo_start_xmit     = onebox_xmit,
		.ndo_get_stats      = onebox_get_stats,
		.ndo_do_ioctl       = onebox_ioctl,
	};
	
	/*Allocate & initialize the network device structure*/
	dev = onebox_allocdev(sizeof(ONEBOX_ADAPTER));    
	if (dev == NULL) {
		ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,
		             (TEXT("%s: Failure in allocation of net-device\n"),
		              __func__));
		goto noalloc;
	}

	//dev->wireless_handlers    = &onebox_handler_def;
	dev->hard_header_len    = FRAME_DESC_SZ + ONEBOX_DESCRIPTOR_SZ; /* 64 + 16 */
	dev->netdev_ops         = &onebox_netdev_ops;

	if (tcp_csumoffld_enable)
	{
		dev->features          |= NETIF_F_IP_CSUM;
	}

	if (onebox_registerdev(dev) != 0) {
		ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,
		             (TEXT("%s: Registration of net-device failed\n"),
		              __func__));
		goto nodev;
	}
	return dev;
nodev:
	free_netdev(dev);
	dev = NULL;
noalloc:
	return dev;
#endif
}/* End of <wlan_netdevice_op> */

/**
 * This function is called by OS when we UP the
 * interface.
 *
 * @param  Pointer to network device
 * @return Success 
 */
int device_open(struct net_device *dev)
{
	//netif_start_queue(dev);
	dev->flags |=  IFF_RUNNING;
	return 0;
}

/**
 * This function is called by OS when the interface
 * status changed to DOWN.
 *
 * @param  dev  Pointer to network device
 */
int device_close(struct net_device *dev)
{
	if (!netif_queue_stopped(dev))
	{
		netif_stop_queue(dev);
	}
	return 0;
}

static ONEBOX_STATUS wlan_gpl_read_pkt(netbuf_ctrl_block_t *netbuf_cb)
{
	struct driver_assets *d_assets = onebox_get_driver_asset();
	PONEBOX_ADAPTER w_adapter = (PONEBOX_ADAPTER)d_assets->techs[WLAN_ID].priv;

	w_adapter->os_intf_ops->onebox_acquire_sem(&w_adapter->wlan_gpl_lock, 0);
	if (d_assets->techs[WLAN_ID].drv_state == MODULE_ACTIVE) {
		wlan_read_pkt(w_adapter, netbuf_cb);
	} else {
		printk("WLAN is being removed.. Dropping Pkt\n");
		w_adapter->os_intf_ops->onebox_free_pkt(w_adapter, netbuf_cb, 0);
		netbuf_cb = NULL;
	}
	w_adapter->os_intf_ops->onebox_release_sem(&w_adapter->wlan_gpl_lock);
	return ONEBOX_STATUS_SUCCESS;
}

static void dump_wlan_mgmt_pending_pkts(void)
{
	netbuf_ctrl_block_t *netbuf_cb = NULL;
	struct driver_assets *d_assets = onebox_get_driver_asset();
	PONEBOX_ADAPTER adapter = (PONEBOX_ADAPTER)d_assets->techs[WLAN_ID].priv;

	for(;;)
	{	
			ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,
					(TEXT("%s:%d ==> PKT IN MGMT QUEUE count %d  <==\n"), __func__, __LINE__, adapter->os_intf_ops->onebox_netbuf_queue_len(&adapter->host_tx_queue[MGMT_SOFT_Q])));
		if (adapter->os_intf_ops->onebox_netbuf_queue_len(&adapter->host_tx_queue[MGMT_SOFT_Q]))
		{
			//netbuf_cb = core_dequeue_pkt(adapter, MGMT_SOFT_Q);
			netbuf_cb = adapter->os_intf_ops->onebox_dequeue_pkt((void *)&adapter->host_tx_queue[MGMT_SOFT_Q]);
			ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,
					(TEXT("%s:%d ==> PKT IN MGMT QUEUE <==\n"), __func__, __LINE__));
			if(netbuf_cb){
				adapter->core_ops->onebox_dump(ONEBOX_ZONE_ERROR, (uint8 *)netbuf_cb->data, netbuf_cb->len);
				adapter->os_intf_ops->onebox_free_pkt(adapter, netbuf_cb, 0);
			}
		}
		else{
			break;
		}
	}

}

static ONEBOX_STATUS wlan_update_buf_status(uint8 device_buf_status)
{
	struct driver_assets *d_assets = onebox_get_driver_asset();
	PONEBOX_ADAPTER w_adapter = (PONEBOX_ADAPTER)d_assets->techs[WLAN_ID].priv;

	if (d_assets->techs[WLAN_ID].drv_state == MODULE_ACTIVE) {
		
		//printk("_UBS_");
		w_adapter->buf_status_updated = 1;
		if(device_buf_status & (ONEBOX_BIT(SD_PKT_MGMT_BUFF_FULL)))
		{
			w_adapter->mgmt_buffer_full = 1;
		}
		else
		{
			w_adapter->mgmt_buffer_full = 0;
		}
		if(device_buf_status & (ONEBOX_BIT(SD_PKT_BUFF_FULL)))
		{
			w_adapter->buffer_full = 1;
		}
		else
		{
			w_adapter->buffer_full = 0;
		}
		if(device_buf_status & (ONEBOX_BIT(SD_PKT_BUFF_SEMI_FULL)))
		{
			w_adapter->semi_buffer_full = 1;
		}
		else
		{
			w_adapter->semi_buffer_full = 0;
		}
			w_adapter->os_intf_ops->onebox_set_event(&(w_adapter->sdio_scheduler_event));
			/* This Interrupt Is Recvd When The Hardware Buffer Is Empty */
			ONEBOX_DEBUG(ONEBOX_ZONE_INFO, 
				(TEXT("%s: ==> BUFFER_AVILABLE <==\n"), __func__));
			w_adapter->buf_avilable_counter++;
	} else {
		printk("WLAN not active and hence no buffer updates\n");
	}
	return ONEBOX_STATUS_SUCCESS;
}
/**
 * Os sends packet to driver using this function.
 * @param  Pointer to struct sk_buff containing the payload
 *      to be transmitted.
 * @param  Pointer to network device
 */
int onebox_xmit(struct sk_buff *skb, struct net_device *dev)
{
	void * ni;
	uint16 flags;
	PONEBOX_ADAPTER adapter;
	netbuf_ctrl_block_t *netbuf_cb;
	adapter = netdev_priv(dev);

	FUNCTION_ENTRY(ONEBOX_ZONE_DATA_SEND);


	netbuf_cb = (netbuf_ctrl_block_t *)skb->cb;
	/* Store the required skb->cb content temporarily before modifying netbuf_cb
	 * because both of them have the same memory */

	ni =(void *)SKB_CB(skb)->ni;
	flags =(uint16)SKB_CB(skb)->flags;

	/* Now assign the required fields of netbuf_cb for hal operations */
	netbuf_cb->ni = ni;
	netbuf_cb->flags = flags;

	netbuf_cb->skb_priority = skb_get_queue_mapping(skb);

	netbuf_cb->pkt_addr = (void *)skb;
	netbuf_cb->len = skb->len;
	netbuf_cb->data = skb->data;
	netbuf_cb->priority = skb->priority;
	adapter->core_ops->onebox_dump(ONEBOX_ZONE_INFO, netbuf_cb->data, netbuf_cb->len);
	adapter->core_ops->onebox_core_xmit(adapter, netbuf_cb);
	return ONEBOX_STATUS_SUCCESS;
}

/*
 * This function deregisters WLAN firmware
 * @param  Pointer to adapter structure.  
 * @return 0 if success else -1. 
 */

static ONEBOX_STATUS wlan_deregister_fw(PONEBOX_ADAPTER adapter)
{
	onebox_mac_frame_t *mgmt_frame;
	netbuf_ctrl_block_t *netbuf_cb = NULL;
	ONEBOX_STATUS status = ONEBOX_STATUS_SUCCESS;

	ONEBOX_DEBUG(ONEBOX_ZONE_INFO,
			(TEXT("===> Deregister WLAN FW <===\n")));

	netbuf_cb = adapter->os_intf_ops->onebox_alloc_skb(FRAME_DESC_SZ);
	if(netbuf_cb == NULL)
	{	
		ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s: Unable to allocate skb\n"), __func__));
		status = ONEBOX_STATUS_FAILURE;
		return status;

	}
	adapter->os_intf_ops->onebox_add_data_to_skb(netbuf_cb, FRAME_DESC_SZ);
	mgmt_frame = (onebox_mac_frame_t *)netbuf_cb->data;

	adapter->os_intf_ops->onebox_memset(mgmt_frame, 0, FRAME_DESC_SZ);

	/* FrameType*/
	mgmt_frame->desc_word[1] = ONEBOX_CPU_TO_LE16(WLAN_DE_REGISTER);
#define IMMEDIATE_WAKEUP 1
	mgmt_frame->desc_word[0] = ((ONEBOX_WIFI_MGMT_Q << 12)| (IMMEDIATE_WAKEUP << 15));
	netbuf_cb->tx_pkt_type = WLAN_TX_M_Q;

	printk("<==== DEREGISTER FRAME ====>\n");
	adapter->core_ops->onebox_dump(ONEBOX_ZONE_ERROR, (PUCHAR)mgmt_frame, FRAME_DESC_SZ);
	status = adapter->onebox_send_pkt_to_coex(netbuf_cb, WLAN_Q);
	if (status != ONEBOX_STATUS_SUCCESS) 
	{ 
		ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,(TEXT 
			     ("%s: Failed To Write The Packet\n"),__func__));
	}
#undef IMMEDIATE_WAKEUP
	adapter->os_intf_ops->onebox_free_pkt(adapter, netbuf_cb, 0);
  	return status;
}

/**
 * This function is triggered whenever wlan module is 
 * inserted. Links wlan module with hal module 
 * work is done here.
 *
 * @return 
 */
static int32 wlan_insert(void)
{
	struct net_device *dev = NULL;
	uint8 count =0;

	struct onebox_core_operations *core_ops = onebox_get_core_wlan_operations();
	struct onebox_net80211_operations *net80211_ops = onebox_get_net80211_operations();
	struct onebox_devdep_operations *devdep_ops = onebox_get_devdep_wlan_operations();
	struct onebox_osi_host_intf_operations *osi_host_intf_ops = onebox_get_osi_host_intf_operations();
	struct onebox_osd_host_intf_operations *osd_host_intf_ops = onebox_get_osd_host_intf_operations();
	struct onebox_os_intf_operations *os_intf_ops = onebox_get_os_intf_operations();
	struct ieee80211_rate_ops *core_rate_ops = onebox_get_ieee80211_rate_ops();
	struct onebox_wlan_osd_operations *wlan_osd_ops = onebox_get_wlan_osd_operations_from_origin();
	struct driver_assets *d_assets = onebox_get_driver_asset();
	struct wireless_techs *wlan_d;

	ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,(TEXT("In %s Line %d: initializing WLAN Layer\n"),__func__, __LINE__));

	os_intf_ops->onebox_acquire_sem(&d_assets->wlan_init_lock, 0);

	if(d_assets->techs[WLAN_ID].drv_state == MODULE_ACTIVE) {
		
	printk("In %s Line %d WLAN Module is already initialized\n", __func__, __LINE__);
	os_intf_ops->onebox_release_sem(&d_assets->wlan_init_lock);
	return ONEBOX_STATUS_SUCCESS;
	}
	dev = wlan_netdevice_op();
	if (!dev) {
		ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,
		             (TEXT("%s: Failed to initialize a network device\n"), __func__));
		goto nodev;
	}

	ONEBOX_DEBUG(ONEBOX_ZONE_INIT,
	             (TEXT("%s: Net device operations suceeded\n"), __func__));
	adapter = os_intf_ops->onebox_get_priv(dev); /*we can also use dev->priv;*/

	os_intf_ops->onebox_memset(adapter, 0, sizeof(ONEBOX_ADAPTER));

	/* Initialise the Core and device dependent operations */
	adapter->core_ops          = core_ops;
	adapter->net80211_ops      = net80211_ops;
	adapter->devdep_ops        = devdep_ops;
	adapter->osd_host_intf_ops = osd_host_intf_ops;
	adapter->osi_host_intf_ops = osi_host_intf_ops;
	adapter->os_intf_ops       = os_intf_ops;
	adapter->core_rate_ops     = core_rate_ops;
	adapter->wlan_osd_ops      = wlan_osd_ops;

	adapter->dev = dev;

	os_intf_ops->onebox_init_dyn_mutex(&adapter->wlan_gpl_lock);

	ONEBOX_DEBUG(ONEBOX_ZONE_INIT,(TEXT("%s: Mutex init successfull\n"), __func__));

	os_intf_ops->onebox_init_event(&(adapter->sdio_scheduler_event));
	os_intf_ops->onebox_init_event(&(adapter->stats_event));
	os_intf_ops->onebox_init_event(&(adapter->bb_rf_event));

	d_assets->techs[WLAN_ID].priv = (void *)adapter;
#ifdef USE_SDIO_INTF
	adapter->pfunction              = (struct sdio_func *)d_assets->pfunc;
	adapter->buffer_status_register = ONEBOX_DEVICE_BUFFER_STATUS_REGISTER; 
#else
	adapter->pfunction            = (struct usb_interface *)d_assets->pfunc;
	adapter->buffer_status_register = d_assets->techs[WLAN_ID].buffer_status_reg_addr; 
#endif
	adapter->onebox_send_pkt_to_coex = send_pkt_to_coex;

	if (wlan_osd_ops->onebox_init_wlan_thread(&(adapter->sdio_scheduler_thread_handle),
					          "WLAN-Thread",
					          0,
					          devdep_ops->onebox_sdio_scheduler_thread, 
					          adapter) != ONEBOX_STATUS_SUCCESS)
	{
		ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,(TEXT("%s: Unable to initialize thrd\n"), __func__));
		goto fail;
	}
	ONEBOX_DEBUG(ONEBOX_ZONE_INIT,(TEXT("%s: Initialized thread & Event\n"), __func__));

	/* start the transmit thread */
	os_intf_ops->onebox_start_thread( &(adapter->sdio_scheduler_thread_handle));

	os_intf_ops->onebox_strcpy(adapter->name, "onebox-mobile"); 
	os_intf_ops->onebox_start_netq(adapter->dev);
	init_waitqueue_head(&d_assets->techs[WLAN_ID].deregister_event);
	
	/*FIXME: why is read reg parameters not called from here?? */	
	//read_reg_parameters(adapter);
	adapter->recv_channel = 1;
	adapter->RFType = ONEBOX_RF_8111;
	adapter->def_chwidth = BW_20Mhz;
  /*The operating band will be later initialized based on the band from eeprom reads */
	adapter->operating_band = BAND_2_4GHZ;
	adapter->operating_chwidth = BW_20Mhz;
	adapter->calib_mode  = 0;
	adapter->per_lpbk_mode  = 0;
	adapter->aggr_limit.tx_limit = 11;
	adapter->aggr_limit.rx_limit = 64;
	adapter->Driver_Mode = d_assets->asset_role;
	printk("Driver mode in WLAN is %d\n", adapter->Driver_Mode);
	if (adapter->Driver_Mode == RF_EVAL_LPBK_CALIB)
	{
		/*FIXME: Try to optimize these conditions */
		ONEBOX_DEBUG(ONEBOX_ZONE_INIT, (TEXT("onebox_read_reg_param: RF Eval LPBK CALIB mode on\n")));    
		adapter->Driver_Mode = RF_EVAL_MODE_ON; /* RF EVAL mode */
		adapter->calib_mode  = 1;
		adapter->per_lpbk_mode  = 1;
	} 
	else if (adapter->Driver_Mode == RF_EVAL_LPBK)
	{
		ONEBOX_DEBUG(ONEBOX_ZONE_INIT, (TEXT("onebox_read_reg_param: RF Eval LPBK mode on\n")));    
		adapter->Driver_Mode = RF_EVAL_MODE_ON; /* RF EVAL mode */
		adapter->per_lpbk_mode  = 1;
	} 
	adapter->PassiveScanEnable = ONEBOX_FALSE;
	adapter->config_params.BT_coexistence = 0;
	adapter->isMobileDevice = ONEBOX_TRUE;
	adapter->max_stations_supported  = MAX_STATIONS_SUPPORTED;

	adapter->os_intf_ops->onebox_memcpy(adapter->mac_addr, device_mac_addr, ETH_ALEN);
	adapter->os_intf_ops->onebox_memcpy(adapter->dev->dev_addr, adapter->mac_addr, ETH_ALEN);
	
	ONEBOX_DEBUG(ONEBOX_ZONE_FSM, (TEXT("WLAN : Card read indicated\n")));
	/* initializing tx soft queues */
	for (count = 0; count < NUM_SOFT_QUEUES; count++) 
		adapter->os_intf_ops->onebox_netbuf_queue_init(&adapter->host_tx_queue[count]);

	if (onebox_umac_init_done(adapter)!= ONEBOX_STATUS_SUCCESS)
		goto fail;


	d_assets->techs[WLAN_ID].drv_state = MODULE_ACTIVE;
	wlan_d = &d_assets->techs[WLAN_ID];
	wlan_d->tx_intention = 1;
	while(1) {
		msleep(1);
		d_assets->update_tx_status(WLAN_ID);
		if(wlan_d->tx_access)
			break;
		count++;
		if(count > 500)
			break;
		printk("Waiting for TX access\n");
	}

	if (!wlan_d->tx_access) {
		d_assets->techs[WLAN_ID].deregister_flags = 1;
		if (wait_event_timeout((wlan_d->deregister_event), (d_assets->techs[WLAN_ID].deregister_flags == 0), msecs_to_jiffies(6000) )) {
		} else {
			printk("ERR: In %s Line %d Initialization of WLAN Failed as Wlan TX access is not granted from Common Hal \n", __func__, __LINE__);
			goto fail;
		}
	}

	if (onebox_load_bootup_params(adapter) != ONEBOX_STATUS_SUCCESS) {
		ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,(TEXT ("%s: Failed to load bootup parameters\n"), 
			     __func__));
		goto fail;
	}

	ONEBOX_DEBUG(ONEBOX_ZONE_FSM,
		     (TEXT("%s: BOOTUP Parameters loaded successfully\n"),__func__));

	adapter->fsm_state = FSM_LOAD_BOOTUP_PARAMS ;


	os_intf_ops->onebox_init_dyn_mutex(&adapter->ic_lock_vap);
	printk("wlan hal: Init proc entry call\n");
	if (setup_wlan_procfs(adapter))
		ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,(TEXT ("%s: Failed to setup wlan procfs entry\n"),__func__)); 

	os_intf_ops->onebox_release_sem(&d_assets->wlan_init_lock);
	//Initializing Queue Depths
	adapter->host_txq_maxlen[BK_Q_STA] = adapter->host_txq_maxlen[BK_Q_AP] = MAX_BK_QUEUE_LEVEL; 
	adapter->host_txq_maxlen[BE_Q_STA] = adapter->host_txq_maxlen[BE_Q_AP] = MAX_BE_QUEUE_LEVEL; 
	adapter->host_txq_maxlen[VI_Q_STA] = adapter->host_txq_maxlen[VI_Q_AP] = MAX_VI_QUEUE_LEVEL; 
	adapter->host_txq_maxlen[VO_Q_STA] = adapter->host_txq_maxlen[VO_Q_AP] = MAX_VO_QUEUE_LEVEL; 
	return ONEBOX_STATUS_SUCCESS;    

fail:
	if (dev)
		unregister_dev(dev);
nodev:
	os_intf_ops->onebox_release_sem(&d_assets->wlan_init_lock);
	return -ENOMEM;    
}/* End <wlan_insert> */

/**
 * This function removes the wlan module safely..
 *
 * @param  Pointer to sdio_func structure.  
 * @param  Pointer to sdio_device_id structure.  
 * @return VOID. 
 */
static int32 wlan_remove(void)
{
	struct net_device *dev = adapter->dev;
	struct onebox_os_intf_operations *os_intf_ops = onebox_get_os_intf_operations();
	struct onebox_wlan_osd_operations *wlan_osd_ops = onebox_get_wlan_osd_operations_from_origin();
	struct driver_assets *d_assets = onebox_get_driver_asset();
	struct wireless_techs *wlan_d = &d_assets->techs[WLAN_ID];
	
	adapter->os_intf_ops->onebox_acquire_sem(&adapter->wlan_gpl_lock, 0);
#if 0
	if(wlan_d->fw_state == FW_INACTIVE)
	{
		printk("%s Line %d FW is already in INACTIVE state no need to perform any changes\n", __func__, __LINE__);
		adapter->os_intf_ops->onebox_release_sem(&adapter->wlan_gpl_lock);
		return ONEBOX_STATUS_SUCCESS;
	}
#endif

	FUNCTION_ENTRY(ONEBOX_ZONE_INFO);   

#if KERNEL_VERSION_BTWN_2_6_(18, 26)
	wlan_osd_ops->onebox_kill_wlan_thread(&adapter->sdio_scheduler_thread_handle); 
#elif KERNEL_VERSION_GREATER_THAN_2_6_(27)
	wlan_osd_ops->onebox_kill_wlan_thread(adapter); 
#endif

	os_intf_ops->onebox_delete_event(&(adapter->stats_event));

	/* Removing channel_util_timeout timer here */
	if (adapter == NULL) {
		printk("In %s %d NULL ADAPTER\n", __func__, __LINE__);
	}

	printk("In %s Line %d \n", __func__, __LINE__);

	if ((adapter->channel_util_timeout.function != NULL) &&
	    (adapter->os_intf_ops->onebox_sw_timer_pending(&adapter->channel_util_timeout))) {
		printk("In %s Line %d channel utilization timer was pending\n", __func__, __LINE__);
		adapter->os_intf_ops->onebox_remove_timer(&adapter->channel_util_timeout);
		printk("In %s Line %d \n", __func__, __LINE__);
	}
	printk("In %s Line %d \n", __func__, __LINE__);

	adapter->core_ops->onebox_core_deinit(adapter);
	if (d_assets->card_state != GS_CARD_DETACH) {
					wlan_d->tx_intention = 1;
					d_assets->update_tx_status(WLAN_ID);
					if(!wlan_d->tx_access) {
									d_assets->techs[WLAN_ID].deregister_flags = 1;
									if(wait_event_timeout((wlan_d->deregister_event), (d_assets->techs[WLAN_ID].deregister_flags == 0), msecs_to_jiffies(6000))) {
													if(wlan_d->tx_access)
													wlan_deregister_fw(adapter);	
	}
									else
													printk("Failed to get sleep exit\n");
					} else {
									wlan_deregister_fw(adapter);	
					}
					WLAN_TECH.tx_intention = 0;
					WLAN_TECH.tx_access = 0;
					d_assets->update_tx_status(WLAN_ID);
	}

	d_assets->techs[WLAN_ID].fw_state = FW_INACTIVE;

  /*Return the network device to the kernel*/
	if(adapter->dev != NULL) {
					printk("IN %s Line %d Unregistering netdev \n", __func__, __LINE__);
					unregister_dev(dev);
				//adapter->dev = NULL;
				destroy_wlan_procfs();
	}
	adapter->os_intf_ops->onebox_release_sem(&adapter->wlan_gpl_lock);

	FUNCTION_EXIT(ONEBOX_ZONE_INFO);

	return ONEBOX_STATUS_SUCCESS;
}/* End <onebox_disconnect> */

ONEBOX_STATIC int32 onebox_wlangpl_module_init(VOID)
{
	int32 rc = 0;
	struct driver_assets *d_assets =
		onebox_get_driver_asset();
	
	d_assets->techs[WLAN_ID].drv_state = MODULE_INSERTED;
	
	d_assets->techs[WLAN_ID].inaugurate = wlan_insert;
	d_assets->techs[WLAN_ID].disconnect = wlan_remove;
	d_assets->techs[WLAN_ID].onebox_get_pkt_from_coex = wlan_gpl_read_pkt;
	d_assets->techs[WLAN_ID].onebox_get_buf_status = wlan_update_buf_status;
	d_assets->techs[WLAN_ID].wlan_dump_mgmt_pending = dump_wlan_mgmt_pending_pkts;
	
	if (d_assets->card_state == GS_CARD_ABOARD) {
		if((d_assets->techs[WLAN_ID].fw_state == FW_ACTIVE) && (d_assets->techs[WLAN_ID].drv_state != MODULE_ACTIVE)) {
			rc = wlan_insert();
			if (rc) {
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,(TEXT ("%s: failed to insert "
					"wlan error[%d]\n"),__func__, rc)); 
				return 0;
			}
		}
	}	
	
	printk("WLAN : Wlan gpl installed\n ");

	return 0;
}

ONEBOX_STATIC VOID onebox_wlangpl_module_exit(VOID)
{
	struct driver_assets *d_assets =
		onebox_get_driver_asset();
	struct wireless_techs *wlan_d;

	printk("In %s Line %d \n", __func__, __LINE__);

	if (d_assets->techs[WLAN_ID].drv_state == MODULE_ACTIVE) {
		printk("In %s Line %d waiting for even\n", __func__, __LINE__);
		//d_assets->techs[WLAN_ID].deregister_flags = 1;
		wlan_d = &d_assets->techs[WLAN_ID];
		wlan_remove();
	}
	d_assets->techs[WLAN_ID].drv_state = MODULE_REMOVED;
	d_assets->techs[WLAN_ID].inaugurate = NULL;
	d_assets->techs[WLAN_ID].disconnect = NULL;
	d_assets->techs[WLAN_ID].onebox_get_pkt_from_coex = NULL;
	d_assets->techs[WLAN_ID].onebox_get_buf_status = NULL;

	return;
}

module_init(onebox_wlangpl_module_init);
module_exit(onebox_wlangpl_module_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Redpine Signals Inc");
MODULE_DESCRIPTION("Coexistance Solution From Redpine Signals");
MODULE_SUPPORTED_DEVICE("Godavari RS911x WLAN Modules");
MODULE_VERSION("0.1");
