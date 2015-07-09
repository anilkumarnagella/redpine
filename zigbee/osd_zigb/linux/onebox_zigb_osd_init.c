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
#include "onebox_zigb_ioctl.h"

static PONEBOX_ADAPTER adapter;
static uint8 device_mac_addr[6] = {0x00, 0x23, 0xa7, 0x27, 0x03, 0x91};

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

	dev = alloc_netdev(sizeof_priv, "zigb%d", ether_setup);

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
struct net_device* zigb_netdevice_op(VOID)
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
	dev->hard_start_xmit      = zigb_xmit;
	dev->get_stats            = onebox_get_stats;   
	dev->do_ioctl             = zigb_ioctl;
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
	static const struct net_device_ops zigb_netdev_ops =
	{
		.ndo_open           = device_open,
		.ndo_stop           = device_close,
		.ndo_start_xmit     = zigb_xmit,
		.ndo_get_stats      = onebox_get_stats,
		.ndo_do_ioctl       = zigb_ioctl,
	};
	
	/*Allocate & initialize the network device structure*/
	dev = onebox_allocdev(sizeof(ONEBOX_ADAPTER));    
	if (dev == NULL)
	{
		ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,
		             (TEXT("%s: Failure in allocation of net-device\n"),
		              __func__));
		return dev;
	}

	//dev->wireless_handlers    = &onebox_handler_def;
	dev->hard_header_len    = FRAME_DESC_SZ + ONEBOX_DESCRIPTOR_SZ; /* 64 + 16 */
	dev->netdev_ops         = &zigb_netdev_ops;

	if (onebox_registerdev(dev) != 0)
	{
		ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,
		             (TEXT("%s: Registration of net-device failed\n"),
		              __func__));
		free_netdev(dev);
		return NULL;
	}
#endif
	return dev;
}/* End of <zigb_netdevice_op> */

static ONEBOX_STATUS zigb_gpl_read_pkt(netbuf_ctrl_block_t *netbuf_cb)
{
	struct driver_assets *d_assets = onebox_get_driver_asset();
	PONEBOX_ADAPTER z_adapter = (PONEBOX_ADAPTER)d_assets->techs[ZB_ID].priv;

	z_adapter->os_intf_ops->onebox_acquire_sem(&z_adapter->zigb_gpl_lock, 0);
	if (d_assets->techs[ZB_ID].drv_state == MODULE_ACTIVE) {
		zigb_read_pkt(z_adapter, netbuf_cb);
	} else {
		ONEBOX_DEBUG(ONEBOX_ZONE_INFO, (TEXT("ZIGB is being removed.. Dropping Pkt\n")));
		z_adapter->os_intf_ops->onebox_free_pkt(z_adapter, netbuf_cb, 0);
		netbuf_cb = NULL;
	}

	z_adapter->os_intf_ops->onebox_release_sem(&z_adapter->zigb_gpl_lock);
	return ONEBOX_STATUS_SUCCESS;
}

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

/**
 * Os sends packet to driver using this function.
 * @param  Pointer to struct sk_buff containing the payload
 *      to be transmitted.
 * @param  Pointer to network device
 */
int zigb_xmit(struct sk_buff *skb, struct net_device *dev)
{
	/* Dummy for Zigbee currently */
	return ONEBOX_STATUS_SUCCESS;
}

/*
 * This function deregisters ZIGB firmware
 * @param  Pointer to adapter structure.  
 * @return 0 if success else -1. 
 */

static ONEBOX_STATUS zigb_deregister_fw(PONEBOX_ADAPTER adapter)
{
	uint8 *frame_desc;
	netbuf_ctrl_block_t *netbuf_cb = NULL;
	ONEBOX_STATUS status = ONEBOX_STATUS_SUCCESS;

	ONEBOX_DEBUG(ONEBOX_ZONE_INFO,
			(TEXT("===> Deregister ZIGB FW <===\n")));

	netbuf_cb = adapter->os_intf_ops->onebox_alloc_skb(FRAME_DESC_SZ);
	if(netbuf_cb == NULL)
	{	
		ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s: Unable to allocate skb\n"), __func__));
		status = ONEBOX_STATUS_FAILURE;
		return status;

	}
	adapter->os_intf_ops->onebox_add_data_to_skb(netbuf_cb, FRAME_DESC_SZ);

	adapter->os_intf_ops->onebox_memset(netbuf_cb->data, 0, FRAME_DESC_SZ);
	
	frame_desc = (uint8 *)netbuf_cb->data;

	frame_desc[12] = 0x0; /* Sequence Number */
	frame_desc[13] = 0x1; /* Direction */
	frame_desc[14] = 0x7; /* Interface */
	frame_desc[15] = ZIGB_DEREGISTER; /* Packet Type */
	netbuf_cb->tx_pkt_type = ZIGB_TX_Q;
	status = adapter->onebox_send_pkt_to_coex(netbuf_cb, VIP_Q);
	if (status != ONEBOX_STATUS_SUCCESS) 
	{ 
		ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,(TEXT 
			     ("%s: Failed To Write The Packet\n"),__func__));
	}
	adapter->os_intf_ops->onebox_free_pkt(adapter, netbuf_cb, 0);
  	return status;
}

/**
 * This function is triggered whenever zigb module is 
 * inserted. Links zigb module with hal module 
 * work is done here.
 *
 * @return 
 */
static int32 zigb_insert(void)
{
	int32 rc = 0;
	struct net_device *dev = NULL;

	struct onebox_osi_host_intf_operations *osi_host_intf_ops = onebox_get_osi_host_intf_operations();
	struct onebox_osd_host_intf_operations *osd_host_intf_ops = onebox_get_osd_host_intf_operations();
	struct onebox_os_intf_operations *os_intf_ops = onebox_get_os_intf_operations();
	struct onebox_zigb_osd_operations *osd_zigb_ops = onebox_get_zigb_osd_ops();
	struct onebox_osi_zigb_ops *osi_zigb_ops = onebox_get_osi_zigb_ops();
	struct driver_assets *d_assets = onebox_get_driver_asset();

	ONEBOX_DEBUG(ONEBOX_ZONE_INIT,(TEXT("%s: Initialization function called\n"), __func__));
	os_intf_ops->onebox_acquire_sem(&d_assets->zigbee_init_lock, 0);
	if(d_assets->techs[ZB_ID].drv_state == MODULE_ACTIVE) {
		
	printk("In %s Line %d Zigbee Module is already initialized\n", __func__, __LINE__);
	os_intf_ops->onebox_release_sem(&d_assets->zigbee_init_lock);
	return ONEBOX_STATUS_SUCCESS;
	}

	dev = zigb_netdevice_op();
	if (!dev) {
		ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,
		             (TEXT("%s: Failed to perform netdevice operations\n"), __func__));
		goto nodev;
	}

	ONEBOX_DEBUG(ONEBOX_ZONE_INIT,
	            (TEXT("%s: Net device operations suceeded\n"), __func__));
	adapter = os_intf_ops->onebox_get_priv(dev); /*we can also use dev->priv;*/

	os_intf_ops->onebox_memset(adapter, 0, sizeof(ONEBOX_ADAPTER));

	/* Initialise the Core and device dependent operations */
	adapter->osi_zigb_ops      = osi_zigb_ops;
	adapter->zigb_osd_ops      = osd_zigb_ops;
	adapter->osd_host_intf_ops = osd_host_intf_ops;
	adapter->osi_host_intf_ops = osi_host_intf_ops;
	adapter->os_intf_ops       = os_intf_ops;

	os_intf_ops->onebox_init_dyn_mutex(&adapter->zigb_gpl_lock);

	adapter->dev = dev;

	adapter->os_intf_ops->onebox_memcpy(adapter->mac_addr, device_mac_addr, ETH_ALEN);
	adapter->os_intf_ops->onebox_memcpy(adapter->dev->dev_addr, adapter->mac_addr, ETH_ALEN);

	ONEBOX_DEBUG(ONEBOX_ZONE_INIT,(TEXT("%s: Mutex init successfull\n"), __func__));

	d_assets->techs[ZB_ID].priv = (void *)adapter;
	adapter->onebox_send_pkt_to_coex = send_pkt_to_coex;

	os_intf_ops->onebox_strcpy(adapter->name, "onebox_zigb"); 

	adapter->fsm_state = FSM_DEVICE_READY;

	init_waitqueue_head(&d_assets->techs[ZB_ID].deregister_event);
	rc = adapter->osi_zigb_ops->onebox_core_init(adapter);
	if (rc) {
		ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,(TEXT("%s: failed to init ZIGBEE, error[%d]\n"), __func__, rc));
		goto nocoreinit;
	}

	d_assets->techs[ZB_ID].drv_state = MODULE_ACTIVE;
	d_assets->techs[ZB_ID].default_ps_en = 0;

	rc = setup_zigb_procfs(adapter);
	if (rc) 
		ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,(TEXT("%s: failed to init zigbee procfs entry, error[%d]\n"), __func__, rc));

	os_intf_ops->onebox_release_sem(&d_assets->zigbee_init_lock);
	return ONEBOX_STATUS_SUCCESS;    

nocoreinit:
	if (dev)
		unregister_dev(dev);
nodev:
	os_intf_ops->onebox_release_sem(&d_assets->zigbee_init_lock);
	return -ENOMEM;    
} /* End <zigb_insert> */

/**
 * This function removes the zigb module safely..
 *
 * @param  Pointer to sdio_func structure.  
 * @param  Pointer to sdio_device_id structure.  
 * @return VOID. 
 */
static int32 zigb_remove(void)
{
	struct net_device *dev = adapter->dev;
	struct driver_assets *d_assets = onebox_get_driver_asset();


	adapter->os_intf_ops->onebox_acquire_sem(&adapter->zigb_gpl_lock, 0);
	FUNCTION_ENTRY(ONEBOX_ZONE_INFO);   

	zigb_deregister_fw(adapter);	

	d_assets->techs[ZB_ID].fw_state = FW_INACTIVE;

	destroy_zigb_procfs();	

	adapter->osi_zigb_ops->onebox_core_deinit(adapter);
	adapter->os_intf_ops->onebox_release_sem(&adapter->zigb_gpl_lock);
        /*Return the network device to the kernel*/
	unregister_dev(dev);

	FUNCTION_EXIT(ONEBOX_ZONE_INFO);
	return ONEBOX_STATUS_SUCCESS;
}/* End <zigb_remove> */

ONEBOX_STATIC int32 onebox_zigbgpl_module_init(VOID)
{
	int32 rc = 0;
	struct driver_assets *d_assets =
		onebox_get_driver_asset();
	
	d_assets->techs[ZB_ID].drv_state = MODULE_INSERTED;
	
	d_assets->techs[ZB_ID].inaugurate = zigb_insert;
	d_assets->techs[ZB_ID].disconnect = zigb_remove;
	d_assets->techs[ZB_ID].onebox_get_pkt_from_coex = zigb_gpl_read_pkt;
	
	if (d_assets->card_state == GS_CARD_ABOARD) {
		if(d_assets->techs[ZB_ID].fw_state == FW_ACTIVE) {
			rc = zigb_insert();
			if (rc) {
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,(TEXT ("%s: failed to insert "
					"zigb error[%d]\n"),__func__, rc)); 
				return 0;
			}
		}
	}	
	
	ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("ZIGB : Zigb gpl installed\n ")));

	return 0;
}

ONEBOX_STATIC VOID onebox_zigbgpl_module_exit(VOID)
{
	struct driver_assets *d_assets =
		onebox_get_driver_asset();
	
	if (d_assets->techs[ZB_ID].drv_state == MODULE_ACTIVE) {
		d_assets->techs[ZB_ID].drv_state = MODULE_REMOVED;
		zigb_remove();
	} else {
		d_assets->techs[ZB_ID].drv_state = MODULE_REMOVED;
	}
	
	
	d_assets->techs[ZB_ID].inaugurate = NULL;
	d_assets->techs[ZB_ID].disconnect = NULL;
	d_assets->techs[ZB_ID].onebox_get_pkt_from_coex = NULL;

	return;
}

module_init(onebox_zigbgpl_module_init);
module_exit(onebox_zigbgpl_module_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Redpine Signals Inc");
MODULE_DESCRIPTION("Coexistance Solution From Redpine Signals");
MODULE_SUPPORTED_DEVICE("Godavari RS911x WLAN Modules");
MODULE_VERSION("0.1");
