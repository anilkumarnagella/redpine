/**
 * @file onebox_usb_main_osd.c
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
 * The file contains Generic HAL changes for USB.
 */

#include "onebox_common.h"
#include <linux/usb.h>

typedef struct urb_context_s {
	PONEBOX_ADAPTER adapter;
	netbuf_ctrl_block_t *netbuf_addr;
	uint8 ep_num;
} urb_context_t;

static PONEBOX_ADAPTER adapter;
ONEBOX_EXTERN int onebox_register_os_intf_operations(struct onebox_os_intf_operations *os_intf_ops);
ONEBOX_STATUS onebox_usb_sg_card_write(PONEBOX_ADAPTER adapter, void *buf,uint16 len,uint8 ep_num);
ONEBOX_EXTERN uint8 *ap_mac_addr;
static uint32 fw;
ONEBOX_STATUS onebox_gspi_init(PONEBOX_ADAPTER adapter);

int onebox_probe (struct usb_interface *pfunction,
                  const struct usb_device_id *id
                 );

static uint8 *segment[2];
static uint32 write_fail;
VOID onebox_disconnect (struct usb_interface *pfunction);
uint32 onebox_find_bulkInAndOutEndpoints (struct usb_interface *interface, PONEBOX_ADAPTER rsi_dev);
ONEBOX_STATUS onebox_usb_card_write(PONEBOX_ADAPTER adapter, void *buf, uint16 len, uint8 ep_num);

static const struct usb_device_id onebox_IdTable[] = 
{
	{ USB_DEVICE(0x0303, 0x0100) },
	{ USB_DEVICE(0x041B, 0x0301) },
	{ USB_DEVICE(0x041B, 0x0201) },
	{ USB_DEVICE(0x041B, 0x9113) },
	{ USB_DEVICE(0x1618, 0x9113) },
#ifdef GDVR_DRV
	{ USB_DEVICE(0x041B, 0x9330) },
#endif
	{ /* Blank */},
};

ONEBOX_STATIC struct usb_driver onebox_driver = 
{
	.name       = "Onebox-USB WLAN",
	.probe      = onebox_probe,
	.disconnect     = onebox_disconnect,
	.id_table   = onebox_IdTable,
};


static ONEBOX_STATUS ulp_read_write(PONEBOX_ADAPTER adapter, uint16 addr, uint16 *data, uint16 len_in_bits)
{
	if((master_reg_write(adapter, GSPI_DATA_REG1, ((addr << 6) | (data[1] & 0x3f)), 2) < 0)) {
		goto fail;
	}

	if((master_reg_write(adapter, GSPI_DATA_REG0, *(uint16 *)&data[0], 2)) < 0) {
		goto fail;
	}

	if((onebox_gspi_init(adapter)) < 0) {
		goto fail;
	}

	if((master_reg_write(adapter, GSPI_CTRL_REG1, ((len_in_bits - 1) | GSPI_TRIG), 2)) < 0) {
		goto fail;
	}

 	msleep(10);
	return ONEBOX_STATUS_SUCCESS;
fail:
	return ONEBOX_STATUS_FAILURE;
}

ONEBOX_STATUS onebox_gspi_init(PONEBOX_ADAPTER adapter)
{
	uint32 gspi_ctrl_reg0_val;
	//! RF control reg 
	//! clk_ratio [3:0] 
	/* Programming gspi frequency = soc_frequency / 2 */
	/* TODO Warning : ULP seemed to be not working
	 * well at high frequencies. Modify accordingly */
	gspi_ctrl_reg0_val = 0x4;
	//! csb_setup_time [5:4] 
	gspi_ctrl_reg0_val |= 0x10; 
	//! csb_hold_time [7:6] 
	gspi_ctrl_reg0_val |= 0x40; 
	//! csb_high_time [9:8] 
	gspi_ctrl_reg0_val |= 0x100; 
	//! spi_mode [10] 
	gspi_ctrl_reg0_val |= 0x000; 
	//! clock_phase [11] 
	gspi_ctrl_reg0_val |= 0x000; 
	/* Initializing GSPI for ULP read/writes */
	return master_reg_write(adapter, GSPI_CTRL_REG0, gspi_ctrl_reg0_val, 2);
}

/**
 * This function resets and re-initializes the card.
 *
 * @param  Pointer to usb_func.  
 * @VOID 
 */
static void onebox_reset_card(PONEBOX_ADAPTER adapter)
{
	uint16 temp[4] = {0};

	*(uint32 *)temp = 2;
	printk("%s %d\n",__func__,__LINE__);
	if((ulp_read_write(adapter, WATCH_DOG_TIMER_1, &temp[0], 32)) < 0) {
		goto fail;
	}

	*(uint32 *)temp = 0;
	printk("%s %d\n",__func__,__LINE__);
	if((ulp_read_write(adapter, WATCH_DOG_TIMER_2, temp, 32)) < 0) {
		goto fail;
	}

	*(uint32 *)temp = 50;
	printk("%s %d\n",__func__,__LINE__);
	if((ulp_read_write(adapter, WATCH_DOG_DELAY_TIMER_1, temp, 32)) < 0) {
		goto fail;
	}

	*(uint32 *)temp = 0;
	printk("%s %d\n",__func__,__LINE__);
	if((ulp_read_write(adapter, WATCH_DOG_DELAY_TIMER_2, temp, 32)) < 0) {
		goto fail;
	}

	*(uint32 *)temp = ((0xaa000) | RESTART_WDT | BYPASS_ULP_ON_WDT);
	printk("%s %d\n",__func__,__LINE__);
	if((ulp_read_write(adapter, WATCH_DOG_TIMER_ENABLE, temp, 32)) < 0) {
		goto fail;
	}
	return;
fail:
	printk("Reset card Failed\n");
	return;
}


/**
 * This function is called by kernel when the driver provided 
 * Vendor and device IDs are matched. All the initialization 
 * work is done here.
 *
 * @param  Pointer to sdio_func structure.  
 * @param  Pointer to sdio_device_id structure.  
 * @return SD_SUCCESS in case of successful initialization or 
 *         a negative error code signifying failure. 
 */
int   onebox_probe(struct usb_interface *pfunction, const struct usb_device_id *id)
{
	struct onebox_coex_osi_operations *coex_osi_ops = onebox_get_coex_osi_operations();
	struct onebox_osi_host_intf_operations *osi_host_intf_ops = onebox_get_osi_host_intf_operations();
	struct onebox_osd_host_intf_operations *osd_host_intf_ops = onebox_get_osd_host_intf_operations();
	struct onebox_os_intf_operations *os_intf_ops = onebox_get_os_intf_operations_from_origin();
	struct driver_assets *d_assets = onebox_get_driver_asset();

	/* Register gpl related os interface operations */
	onebox_register_os_intf_operations(os_intf_ops);

	ONEBOX_DEBUG(ONEBOX_ZONE_INIT,(TEXT("%s: Initialization function called\n"), __func__));

	adapter = os_intf_ops->onebox_mem_zalloc(sizeof(ONEBOX_ADAPTER),GFP_KERNEL);
	if(adapter==NULL)
	{
		ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,(TEXT("%s:Memory allocation for adapter failed\n"), __func__));   
		goto fail;
	}
	os_intf_ops->onebox_memset(adapter, 0, sizeof(ONEBOX_ADAPTER));

	/*Claim the usb interface*/ /* Need to check whether it is needed or not */
	//FIXME://onebox_usb_claim_host(pfunction);
	segment[0] = (uint8 *)kmalloc(2048, GFP_ATOMIC);//FIXME:
	segment[1] = (uint8 *)kmalloc(2048, GFP_ATOMIC);//FIXME:

	ONEBOX_DEBUG(ONEBOX_ZONE_INIT,
                          (TEXT("Initialized HAL/CORE/DEV_DEPENDENT Operations\n")));   

	//printk("PROBE: In %s Line %d adapter = %p usbdev = %p\n", __func__, __LINE__, adapter, adapter->usbdev);

	adapter->usbdev = interface_to_usbdev(pfunction);
	//printk("PROBE: In %s Line %d usbdev = %p\n", __func__, __LINE__, adapter->usbdev);
	if(adapter->usbdev == NULL)
	{
		return ONEBOX_TRUE;
	}

	if (onebox_find_bulkInAndOutEndpoints (pfunction, adapter))
	{
		printk ("Error2\n");
		goto fail;
	}
	//printk("PROBE: In %s Line %d adapter = %p usbdev = %p\n", __func__, __LINE__, adapter, adapter->usbdev);

	/* storing our device information in interface device for future refrences */	
	usb_set_intfdata(pfunction, adapter);
	ONEBOX_DEBUG(ONEBOX_ZONE_INIT,(TEXT("%s: Enabled the interface\n"), __func__));
	/* Initialise the Core and device dependent operations */
	adapter->coex_osi_ops = coex_osi_ops;
	adapter->osd_host_intf_ops = osd_host_intf_ops;
	adapter->osi_host_intf_ops = osi_host_intf_ops;
	adapter->os_intf_ops = os_intf_ops;

	adapter->pfunction = pfunction;
	d_assets->global_priv = (void *)adapter;
	d_assets->pfunc       = (void *)pfunction;

	ONEBOX_DEBUG(ONEBOX_ZONE_INIT,(TEXT("%s: Context setting suceeded\n"), __func__));   
	adapter->DataRcvPacket = (uint8*)os_intf_ops->onebox_mem_zalloc((ONEBOX_RCV_BUFFER_LEN * 4),
	                                                                GFP_KERNEL|GFP_DMA);
	if(adapter->DataRcvPacket==NULL)
	{
		ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,(TEXT("%s:Memory allocation for receive buffer failed\n"), __func__));   
		goto fail;
	}

	os_intf_ops->onebox_init_static_mutex(&adapter->sdio_interrupt_lock);
	os_intf_ops->onebox_init_static_mutex(&adapter->usb_dev_sem);
	os_intf_ops->onebox_init_dyn_mutex(&adapter->transmit_lock);
/* coex */
	if(adapter->coex_osi_ops->onebox_common_hal_init(d_assets, adapter)  != ONEBOX_STATUS_SUCCESS) {
		ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,(TEXT("%s Line %d: Failed to initialize common HAL\n"), __func__, __LINE__));
		goto fail;
	}

	os_intf_ops->onebox_init_event(&(adapter->usb_rx_scheduler_event));

	if (adapter->osi_host_intf_ops->onebox_init_host_interface(adapter)!= ONEBOX_STATUS_SUCCESS)
	{
		ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,(TEXT("%s:Failed to init slave regs\n"), __func__));
		os_intf_ops->onebox_mem_free(adapter->DataRcvPacket);
		goto fail;
	}

	/*** Receive thread */
	adapter->rx_usb_urb[0]    =  usb_alloc_urb(0, GFP_KERNEL);
	adapter->rx_usb_urb[1]    =  usb_alloc_urb(0, GFP_KERNEL);
	adapter->tx_usb_urb[0]    =  usb_alloc_urb(0, GFP_KERNEL);
	adapter->tx_usb_urb[1]    =  usb_alloc_urb(0, GFP_KERNEL);
  adapter->os_intf_ops->onebox_init_event(&(adapter->usb_tx_event));

	//adapter->rx_usb_urb[0]->transfer_buffer = adapter->DataRcvPacket;
	adapter->TransmitBlockSize = 252;
	adapter->ReceiveBlockSize  = 252;
	if (os_intf_ops->onebox_init_rx_thread(&(adapter->usb_rx_scheduler_thread_handle),
	                                    "RX-Thread",
	                                    0,
	                                    coex_osi_ops->onebox_usb_rx_scheduler_thread, 
	                                    adapter) != ONEBOX_STATUS_SUCCESS)
	{
		ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,(TEXT("%s: Unable to initialize thrd\n"), __func__));
		os_intf_ops->onebox_mem_free(adapter->DataRcvPacket);
		goto fail;
	}
	printk("Receive thread started successfully\n");
	ONEBOX_DEBUG(ONEBOX_ZONE_INIT,(TEXT("%s: Initialized thread & Event\n"), __func__));

	/* start the receive thread */
	os_intf_ops->onebox_start_thread( &(adapter->usb_rx_scheduler_thread_handle));

	coex_osi_ops->onebox_read_reg_params(adapter);
	/*To verify whether Fw is already downloaded */
	master_reg_read(adapter, MISC_REG, &fw, 2);//read 12 register
	fw &= 1;
	if (!(fw)) 
	{ 
		if(adapter->fw_load_mode == FLASH_RAM_NO_SBL) {
			if((master_reg_write (adapter,MISC_REG,0x0,2)) < 0) {
				printk("Writing into address MISC_REG addr 0x41050012 Failed\n");
				//goto fail;
			}
		}
		if(coex_osi_ops->onebox_device_init(adapter, 1)) /* Load firmware */
		{
			ONEBOX_DEBUG(ONEBOX_ZONE_INIT,(TEXT("%s:Failed in device initialization\n"), __func__));
			//goto fail;
		}
		if(adapter->fw_load_mode == FLASH_RAM_NO_SBL) {
			if((master_reg_write (adapter,ROM_PC_RESET_ADDRESS,0xab,1)) < 0) {
				printk("Resetting ROM PC Failed\n");
				//goto fail;
			}
		}
	}

	adapter->osd_host_intf_ops->onebox_rcv_urb_submit(adapter,
			adapter->rx_usb_urb[0], 1); 

	adapter->osd_host_intf_ops->onebox_rcv_urb_submit(adapter,
			adapter->rx_usb_urb[1], 2); 
	ONEBOX_DEBUG(ONEBOX_ZONE_INIT,(TEXT("%s: master_reg_write done\n"), __func__));

	d_assets->card_state = GS_CARD_ABOARD;

	return ONEBOX_FALSE;    
fail:
#if KERNEL_VERSION_BTWN_2_6_(18, 26)
	adapter->os_intf_ops->onebox_kill_thread(&adapter->sdio_scheduler_thread_handle); 
#elif KERNEL_VERSION_GREATER_THAN_2_6_(27)
	adapter->os_intf_ops->onebox_kill_thread(adapter); 
#endif
	adapter->os_intf_ops->onebox_remove_proc_entry();
	ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,(TEXT("%s: Failed to initialize...Exiting\n"), __func__));  
	return ONEBOX_TRUE;
}/* End <onebox_probe> */

static int disconnect_assets(struct driver_assets *d_assets)
{
	int s = ONEBOX_STATUS_SUCCESS;

	d_assets->card_state = GS_CARD_DETACH;

	if (d_assets->techs[WLAN_ID].drv_state == MODULE_ACTIVE) {
		d_assets->techs[WLAN_ID].drv_state = MODULE_INSERTED;
		if (d_assets->techs[WLAN_ID].disconnect) {
			d_assets->techs[WLAN_ID].disconnect();
		}
	}

	if (d_assets->techs[BT_ID].drv_state == MODULE_ACTIVE) {
		d_assets->techs[BT_ID].drv_state = MODULE_INSERTED;
		if (d_assets->techs[BT_ID].disconnect) {
			d_assets->techs[BT_ID].disconnect();
		}
	}

	if (d_assets->techs[ZB_ID].drv_state == MODULE_ACTIVE) {
		d_assets->techs[ZB_ID].drv_state = MODULE_INSERTED;
		if (d_assets->techs[ZB_ID].disconnect) {
			d_assets->techs[ZB_ID].disconnect();
		}
	}
	return s;
}

/**
 * This function performs the reverse of the probe function..
 *
 * @param  Pointer to sdio_func structure.  
 * @param  Pointer to sdio_device_id structure.  
 * @return VOID. 
 */
VOID onebox_disconnect ( struct usb_interface *pfunction)
{
	struct onebox_os_intf_operations *os_intf_ops;
	struct driver_assets *d_assets =
		onebox_get_driver_asset();
	//PONEBOX_ADAPTER adapter = usb_get_intfdata(pfunction);

	//printk("PROBE: In %s Line %d adapter = %p\n", __func__, __LINE__, adapter);
	if (!adapter) 
	{
		ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,(" Adapter is NULL \n"));
		return;
	}
	onebox_reset_card(adapter);
	//printk("PROBE: In %s Line %d adapter = %p dev = %p\n", __func__, __LINE__, adapter, dev);
	os_intf_ops = onebox_get_os_intf_operations_from_origin(); 


	FUNCTION_ENTRY(ONEBOX_ZONE_INFO);   
  os_intf_ops->onebox_set_event(&adapter->usb_tx_event);
  os_intf_ops->onebox_delete_event(&(adapter->usb_tx_event));

	/* Remove upper layer protocols firstly */
	disconnect_assets(d_assets); 

#ifdef USE_WORKQUEUES	
	flush_workqueue(adapter->int_work_queue);
	destroy_workqueue(adapter->int_work_queue);
	adapter->os_intf_ops->onebox_queue_purge(&adapter->deferred_rx_queue);
#endif

#ifdef USE_TASKLETS
	tasklet_kill(&adapter->int_bh_tasklet);
#endif

	os_intf_ops->onebox_mem_free(adapter->DataRcvPacket);
	os_intf_ops->onebox_kill_thread(adapter); 
	os_intf_ops->onebox_kill_rx_thread(adapter); 

	os_intf_ops->onebox_delete_event(&(adapter->coex_tx_event));
	os_intf_ops->onebox_delete_event(&(adapter->flash_event));

	/* deinit device */
	ONEBOX_DEBUG(ONEBOX_ZONE_INIT,(TEXT("%s: calling device deinitialization\n"), __func__));
	adapter->coex_osi_ops->onebox_device_deinit(adapter);

	/* Remove the proc file system created for the device*/
	adapter->os_intf_ops->onebox_remove_proc_entry();
	ONEBOX_DEBUG(ONEBOX_ZONE_INIT,(TEXT("%s: Uninitialized Procfs\n"), __func__));   

	/*Disable the interface*/
	ONEBOX_DEBUG(ONEBOX_ZONE_INFO,(TEXT("%s: Disabling the interface\n"), __func__));

	/* Release the interrupt handler */
	adapter->stop_card_write = 2; //stopping all writes after deinit
  usb_free_urb(adapter->rx_usb_urb[0]);
  usb_free_urb(adapter->rx_usb_urb[1]);
  usb_free_urb(adapter->tx_usb_urb[0]);
  usb_free_urb(adapter->tx_usb_urb[1]);
	os_intf_ops->onebox_mem_free(adapter);
	if(write_fail) 
	{
    write_fail = 0;
	}
	FUNCTION_EXIT(ONEBOX_ZONE_INFO);

	return;
}/* End <onebox_disconnect> */

/**
 * This function reads one byte of information from a register.
 *
 * @param  Pointer to Driver adapter structure.  
 * @param  Function Number.  
 * @param  Address of the register.  
 * @param  Pointer to the data that stores the data read.  
 * @return On success ONEBOX_STATUS_SUCCESS else ONEBOX_STATUS_FAILURE. 
 */
ONEBOX_STATUS read_register(PONEBOX_ADAPTER ext_adapter, uint32 Addr,
                            uint8 *data)  
{
#if ((defined USE_USB_INTF) && (defined USB_BUFFER_STATUS_INDI))
	struct driver_assets *d_assets = onebox_get_driver_asset();
	uint16 data1 = 0;
	PONEBOX_ADAPTER adapter = (PONEBOX_ADAPTER)d_assets->global_priv;
	master_reg_read(adapter->usbdev, Addr, &data1, 2);//read single byte from register
	*data = *(uint8 *)&data1;
#endif
	return ONEBOX_STATUS_SUCCESS;
}/* End <read_register> */

/**
 * This function writes one byte of information into a register.
 *
 * @param  Pointer to Driver adapter structure.  
 * @param  Function Number.  
 * @param  Address of the register.  
 * @param  Pointer to the data tha has to be written.  
 * @return On success ONEBOX_STATUS_SUCCESS else ONEBOX_STATUS_FAILURE. 
 */
int write_register(PONEBOX_ADAPTER adapter,uint8 reg_dmn,uint32 Addr,uint8 *data)
{
		return ONEBOX_STATUS_SUCCESS;
}/* End <write_register> */

/**
 * This function read multiple bytes of information from the SD card.
 *
 * @param  Pointer to Driver adapter structure.  
 * @param  Function Number.  
 * @param  Address of the register.  
 * @param  Length of the data to be read.  
 * @param  Pointer to the read data.  
 * @return On success ONEBOX_STATUS_SUCCESS else ONEBOX_STATUS_FAILURE. 
 */
ONEBOX_STATUS read_register_multiple(PONEBOX_ADAPTER adapter, 
                                     uint32 Addr,
                                     uint32 Count,
                                     uint8 *data )
{
		return ONEBOX_STATUS_SUCCESS;

}/* End <read_register_multiple> */  

/**
 * This function writes multiple bytes of information to the SD card.
 *
 * @param  Pointer to Driver adapter structure.  
 * @param  Function Number.  
 * @param  Address of the register.  
 * @param  Length of the data.  
 * @param  Pointer to the data that has to be written.  
 * @return On success ONEBOX_STATUS_SUCCESS else ONEBOX_STATUS_FAILURE. 
 */
ONEBOX_STATUS write_register_multiple(PONEBOX_ADAPTER adapter,
                                      uint32 Addr,
                                      uint8 *data,
                                      uint32 Count
                                     )
{
	ONEBOX_STATUS status_l;

	status_l = 0;
	if (write_fail) 
	{
		return status_l;
	}
	if(adapter == NULL || Addr == 0 )
	{
		printk("PROBE: In %s Line %d unable to card write check\n", __func__, __LINE__);
		return ONEBOX_STATUS_FAILURE;
	}
	status_l = onebox_usb_sg_card_write(adapter, data, Count,  Addr);
	return status_l;
} /* End <write_register_multiple> */

ONEBOX_STATUS onebox_usb_sg_card_write(PONEBOX_ADAPTER adapter, void *buf, uint16 len, uint8 ep_num)
{
	ONEBOX_STATUS status_l;
	uint8 *seg;

	status_l = 0;
	if (write_fail)
	{
		printk("%s: Unable to write, device not responding endpoint:%d\n", __func__, ep_num);
		return status_l;
	}

	if ((adapter->coex_mode == WIFI_ZIGBEE) && (ep_num == 2)) {
		seg = segment[0];
		memcpy(seg, buf, len);
	} else {
		seg = segment[0];
		memset(seg, 0, ONEBOX_USB_TX_HEAD_ROOM);
		memcpy(seg + ONEBOX_USB_TX_HEAD_ROOM, buf, len);
		len += ONEBOX_USB_TX_HEAD_ROOM;
	}

	status_l = onebox_usb_card_write(adapter, seg, len, ep_num);
	return status_l;
}

void usb_tx_done_handler(struct urb *urb) 
{

	PONEBOX_ADAPTER adapter = urb->context;

	if (urb->status) 
	{
    ONEBOX_DEBUG (ONEBOX_ZONE_ERROR, 
                 ("USB CARD WRITE FAILED WITH ERROR CODE :%10d  length %d\n", urb->status, urb->actual_length));
    ONEBOX_DEBUG (ONEBOX_ZONE_ERROR, ("PKT Tried to Write is \n"));
    adapter->coex_osi_ops->onebox_dump(ONEBOX_ZONE_ERROR, urb->transfer_buffer, urb->actual_length);
		write_fail = 1;
		return;
	}
	adapter->os_intf_ops->onebox_set_event(&adapter->usb_tx_event);
}

ONEBOX_STATUS onebox_usb_card_write(PONEBOX_ADAPTER adapter, void *buf, uint16 len, uint8 ep_num)
{
  ONEBOX_STATUS status_l;

  adapter->tx_usb_urb[0]->transfer_flags |= URB_ZERO_PACKET;

  usb_fill_bulk_urb (adapter->tx_usb_urb[0],
                     adapter->usbdev,
                     usb_sndbulkpipe (adapter->usbdev,
                     adapter->bulk_out_endpointAddr[ep_num - 1]),
                     (void *)buf, len, usb_tx_done_handler,
                     adapter);

  status_l = usb_submit_urb(adapter->tx_usb_urb[0], GFP_KERNEL); 

  if (status_l < 0) 
  {
    ONEBOX_DEBUG (ONEBOX_ZONE_ERROR, ("Failed To Submit URB Status :%10d \n", status_l));
    write_fail = 1;
    return status_l;
  }

  if(adapter->os_intf_ops->onebox_wait_event(&adapter->usb_tx_event, 60*HZ)/*60 secs*/) {
    adapter->os_intf_ops->onebox_reset_event(&adapter->usb_tx_event);
    return ONEBOX_STATUS_SUCCESS;
  }
  ONEBOX_DEBUG (ONEBOX_ZONE_ERROR, ("USB CARD WRITE FAILED WITH ERROR CODE :%10d \n", status_l));
  adapter->coex_osi_ops->onebox_dump(ONEBOX_ZONE_ERROR, 
                                     adapter->tx_usb_urb[0]->transfer_buffer, 
                                     adapter->tx_usb_urb[0]->actual_length);
  adapter->os_intf_ops->onebox_reset_event(&adapter->usb_tx_event);
  write_fail = 1;
  return ONEBOX_STATUS_FAILURE;
}

/**
 * This function registers the client driver.
 *
 * @param  VOID.  
 * @return 0 if success else a negative number. 
 */
int register_driver(void)
{
	return (usb_register(&onebox_driver));
}

/**
 * This function unregisters the client driver.
 *
 * @param  VOID.  
 * @return VOID. 
 */
void unregister_driver(void)
{
	usb_deregister(&onebox_driver);
}


/*FUNCTION*********************************************************************
Function Name:  onebox_remove
Description:    Dummy for linux sdio
Returned Value: None
Parameters: 
----------------------------+-----+-----+-----+------------------------------
Name                        | I/P | O/P | I/O | Purpose
----------------------------+-----+-----+-----+------------------------------
None
 ******************************************************************************/
ONEBOX_STATUS remove(void)
{
	/**Dummy for Linux*/
	//FIXME: Kill all the VAP'S
	return 0;
}

/**
 * This function initializes the bulk endpoints 
 * to the device.
 *
 * @param
 *      interface       Interface descriptor
 * @param
 *      rsi_dev         driver control block
 * @return         
 *      0 on success and -1 on failure 
 */
uint32 onebox_find_bulkInAndOutEndpoints (struct usb_interface *interface, PONEBOX_ADAPTER rsi_dev)
{
	struct usb_host_interface *iface_desc;
	struct usb_endpoint_descriptor *endpoint;
	int buffer_size;
	int ret_val = -ENOMEM, i, bep_found = 0, binep_found = 0;
	uint8 ep_num = 0;
	uint32 pipe = 0;

	printk ("{%s+}\n", __FUNCTION__);

	/* set up the endpoint information */
	/* check out the endpoints */
	/* use only the first bulk-in and bulk-out endpoints */

	if((interface == NULL) || (rsi_dev == NULL)) 
	{
		printk ("Null pointer in {%s}\n", __func__);
		return ret_val;
	}

	iface_desc = &(interface->altsetting[0]);

	ONEBOX_DEBUG(ONEBOX_ZONE_INFO,
	             ("bNumEndpoints :%08x \n", iface_desc->desc.bNumEndpoints));
	for (i = 0; i < iface_desc->desc.bNumEndpoints; ++i) 
	{
		endpoint = &(iface_desc->endpoint[i].desc);

		ONEBOX_DEBUG (ONEBOX_ZONE_INFO, ("IN LOOP :%08x \n", bep_found));
#if 0		
		if ((!(rsi_dev->bulk_in_endpointAddr)) && 
		      (endpoint->bEndpointAddress & USB_DIR_IN) &&	/* Direction */
		      ((endpoint->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) == USB_ENDPOINT_XFER_BULK)) 
		{
			/* we found a bulk in endpoint */
			ONEBOX_DEBUG (ONEBOX_ZONE_INFO, ("IN EP :%08x \n", i));
			buffer_size = endpoint->wMaxPacketSize;
			//buffer_size = MAX_RX_PKT_SZ;
			rsi_dev->bulk_in_size = buffer_size;
			rsi_dev->bulk_in_endpointAddr = endpoint->bEndpointAddress;
			ep_num = endpoint->bEndpointAddress & USB_ENDPOINT_NUMBER_MASK;
			printk("IN ep num is %d\n",ep_num);
			pipe = usb_rcvbulkpipe(adapter->usbdev, ep_num);
			printk("IN pipe num is %8x\n",pipe);
			pipe = usb_rcvbulkpipe(adapter->usbdev, endpoint->bEndpointAddress);
			printk("IN pipe num 2 is %8x\n",pipe);
		}
#else		
		if ((!(rsi_dev->bulk_in_endpointAddr[binep_found])) && 
		      (endpoint->bEndpointAddress & USB_DIR_IN) &&	/* Direction */
		      ((endpoint->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) == USB_ENDPOINT_XFER_BULK)) 
		{
			/* we found a bulk in endpoint */
			ONEBOX_DEBUG (ONEBOX_ZONE_INFO, ("IN EP :%08x \n", i));
			buffer_size = endpoint->wMaxPacketSize;
			//buffer_size = MAX_RX_PKT_SZ;
			rsi_dev->bulk_in_endpointAddr[binep_found] = endpoint->bEndpointAddress;
			ep_num = endpoint->bEndpointAddress & USB_ENDPOINT_NUMBER_MASK;
			printk("IN ep num is %d\n",ep_num);
			pipe = usb_rcvbulkpipe(adapter->usbdev, ep_num);
			printk("IN pipe num is %8x\n",pipe);
			pipe = usb_rcvbulkpipe(adapter->usbdev, endpoint->bEndpointAddress);
			printk("IN pipe num 2 is %8x\n",pipe);
			/* on some platforms using this kind of buffer alloc
			 * call eliminates a dma "bounce buffer".
			 *
			 * NOTE: you'd normally want i/o buffers that hold
			 * more than one packet, so that i/o delays between
			 * packets don't hurt throughput.
			 */
			buffer_size = endpoint->wMaxPacketSize;
			rsi_dev->bulk_in_size[binep_found] = buffer_size;
			//rsi_dev->tx_urb->transfer_flags = (URB_NO_TRANSFER_DMA_MAP );
			binep_found++;
		}
#endif		
		if (!rsi_dev->bulk_out_endpointAddr[bep_found] &&
		    !(endpoint->bEndpointAddress & USB_DIR_IN) &&
		    ((endpoint->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) == USB_ENDPOINT_XFER_BULK)) 
		{
			ONEBOX_DEBUG (ONEBOX_ZONE_INFO, ("OUT EP :%08x \n", i));
			/* we found a bulk out endpoint */
			rsi_dev->bulk_out_endpointAddr[bep_found] = endpoint->bEndpointAddress;
			ep_num = endpoint->bEndpointAddress & USB_ENDPOINT_NUMBER_MASK;
			printk("OUT ep num is %d\n",ep_num);
			pipe = usb_sndbulkpipe(adapter->usbdev, ep_num);
			printk("OUT pipe num is %8x\n",pipe);
			pipe = usb_sndbulkpipe(adapter->usbdev, endpoint->bEndpointAddress);
			printk("OUT pipe num 2 is %8x\n",pipe);
			/* on some platforms using this kind of buffer alloc
			 * call eliminates a dma "bounce buffer".
			 *
			 * NOTE: you'd normally want i/o buffers that hold
			 * more than one packet, so that i/o delays between
			 * packets don't hurt throughput.
			 */
			buffer_size = endpoint->wMaxPacketSize;
			rsi_dev->bulk_out_size[bep_found] = buffer_size;
			//rsi_dev->tx_urb->transfer_flags = (URB_NO_TRANSFER_DMA_MAP );
			bep_found++;
		}
		if ((bep_found >= MAX_BULK_EP) || (binep_found >= MAX_BULK_EP))
		{
			break;
		}
	}

	if (!(rsi_dev->bulk_in_endpointAddr[0] && rsi_dev->bulk_out_endpointAddr[0])) 
	{
		printk ("Couldn't find both bulk-in and bulk-out endpoints");
		return ret_val;
	} else 
	{
		ONEBOX_DEBUG (ONEBOX_ZONE_INFO, ("EP INIT SUCESS \n"));
	}

	return 0;
}

/* This function reads the data from given register address 
 *
 * @param
 *      Adapter         Pointer to driver's private data area.
 * @param
 *      reg             Address of the register
 * @param
 *      value           Value to write
 * @param
 *      len             Number of bytes to write
 * @return         
 *      none
 */
ONEBOX_STATUS master_reg_read (PONEBOX_ADAPTER adapter, uint32 reg, uint32 * value, uint16 len)
{
	uint8 temp_buf[4];
	ONEBOX_STATUS status_l;
	struct usb_device *usbdev = adapter->usbdev;
	status_l = ONEBOX_STATUS_SUCCESS;
	len = 2;/* FIXME */
	//printk("arguments to ctrl msg usbdev = %p value = %p\n", usbdev, value );
	status_l = usb_control_msg (usbdev,
	                            usb_rcvctrlpipe (usbdev, 0),
	                            USB_VENDOR_REGISTER_READ,
	                            USB_TYPE_VENDOR | USB_DIR_IN | USB_RECIP_DEVICE,
	                            ((reg & 0xffff0000) >> 16), (reg & 0xffff),
	                            (void *) temp_buf, len, HZ * 5);

	*value = (temp_buf[0] | (temp_buf[1] << 8));
	if (status_l < 0)
	{
		ONEBOX_DEBUG (ONEBOX_ZONE_INFO,
		              ("REG READ FAILED WITH ERROR CODE :%010x \n", status_l));
		return status_l;
	}
	ONEBOX_DEBUG (ONEBOX_ZONE_INFO, ("USB REG READ VALUE :%10x \n", *value));
	return status_l;
}

/**
 * This function writes the given data into the given register address 
 *
 * @param
 *      Adapter         Pointer to driver's private data area.
 * @param
 *      reg             Address of the register
 * @param
 *      value           Value to write
 * @param
 *      len             Number of bytes to write
 * @return         
 *      none
 */
ONEBOX_STATUS master_reg_write (PONEBOX_ADAPTER adapter, uint32 reg,
    uint32 value, uint16 len)
{
	uint8 usb_reg_buf[4];
	ONEBOX_STATUS status_l;
	struct usb_device *usbdev = adapter->usbdev;

	printk("IN %s LINE %d\n",__FUNCTION__,__LINE__);
	status_l = ONEBOX_STATUS_SUCCESS;
	usb_reg_buf[0] = (value & 0x000000ff);
	usb_reg_buf[1] = (value & 0x0000ff00) >> 8;
	usb_reg_buf[2] = 0x0;
	usb_reg_buf[3] = 0x0;
	ONEBOX_DEBUG (ONEBOX_ZONE_INFO, ("USB REG WRITE VALUE :%10x \n", value));
	status_l = usb_control_msg (usbdev,
	                            usb_sndctrlpipe (usbdev, 0),
	                            USB_VENDOR_REGISTER_WRITE, USB_TYPE_VENDOR,
	                            ((reg & 0xffff0000) >> 16), (reg & 0xffff),
	                            (void *) usb_reg_buf, len, HZ * 5);
	if (status_l < 0)
	{
		ONEBOX_DEBUG (ONEBOX_ZONE_INFO,
		              ("REG WRITE FAILED WITH ERROR CODE :%10d \n", status_l));
	}
	return status_l;
}

/**
 * This function submits the given URB to the USB stack 
 *
 * @param
 *      Adapter         Pointer to driver's private data area.
 * @param
 *      URB             URB to submit
 * @return         
 *      none
 */
void onebox_rx_urb_submit (PONEBOX_ADAPTER adapter, struct urb *urb, uint8 ep_num)
{
	void (*done_handler) (struct urb *);
	urb_context_t *context_ptr = adapter->os_intf_ops->onebox_mem_zalloc(sizeof(urb_context_t), GFP_ATOMIC);	
	context_ptr->adapter = adapter;
	context_ptr->netbuf_addr = adapter->os_intf_ops->onebox_alloc_skb(ONEBOX_RCV_BUFFER_LEN);
	adapter->os_intf_ops->onebox_add_data_to_skb(context_ptr->netbuf_addr, ONEBOX_RCV_BUFFER_LEN);
	context_ptr->ep_num = ep_num;
	urb->transfer_buffer = context_ptr->netbuf_addr->data;
	adapter->total_usb_rx_urbs_submitted++;

	if (ep_num == 1) {
		done_handler = onebox_rx_done_handler;
	} else {
		done_handler = onebox_rx_done_handler;
	}

	usb_fill_bulk_urb (urb,
	                   adapter->usbdev,
	                   usb_rcvbulkpipe (adapter->usbdev,
					           adapter->bulk_in_endpointAddr[ep_num - 1]),
	                   urb->transfer_buffer, 2100, done_handler,
	                   context_ptr);

	if (usb_submit_urb(urb, GFP_ATOMIC)) 
	{
		printk ("submit(bulk rx_urb)");
	}
}

/**
 * This function is called when a packet is received from 
 * the stack. This is rx done callback.
 *
 * @param
 *      URB             Received URB
 * @return         
 *      none
 */
void onebox_rx_done_handler (struct urb *urb)
{
	uint8 pkt_type = 0;
	urb_context_t *context_ptr = urb->context;
	PONEBOX_ADAPTER adapter = context_ptr->adapter;
	netbuf_ctrl_block_t *netbuf_recv_pkt = context_ptr->netbuf_addr;

	//printk("rx_done, status %d, length %d", urb->status, urb->actual_length);
	//printk("PROBE: In %s Line %d\n", __func__, __LINE__);
	if (urb->status) 
	{
		printk("rx_done, status %d, length %d \n", urb->status, urb->actual_length);
		adapter->coex_osi_ops->onebox_dump(ONEBOX_ZONE_ERROR, urb->transfer_buffer, urb->actual_length);
		//goto resubmit;
		return;
	}
	adapter->total_usb_rx_urbs_done++;
	if (urb->actual_length == 0)
	{
		printk ("==> ERROR: ZERO LENGTH <==\n");
		goto resubmit;
	}
	//netbuf_recv_pkt->len = urb->actual_length;

	adapter->os_intf_ops->onebox_netbuf_trim(netbuf_recv_pkt, urb->actual_length);

	if (adapter->Driver_Mode == QSPI_FLASHING ||
	    adapter->flashing_mode_on) {
			pkt_type = COEX_PKT;
	} else {
		if (context_ptr->ep_num == 1) {
			pkt_type = WLAN_PKT;	
		} else if (context_ptr->ep_num == 2) {
			if (adapter->coex_mode == WIFI_BT_LE || adapter->coex_mode == WIFI_BT_CLASSIC)
				pkt_type = BT_PKT;
			else if (adapter->coex_mode == WIFI_ZIGBEE)
				pkt_type = ZIGB_PKT;
			else
				pkt_type = WLAN_PKT;
		}
	}
	netbuf_recv_pkt->rx_pkt_type = pkt_type;
	//printk("%s: Packet type Received = %d\n", __func__, pkt_type);

	process_usb_rcv_pkt(adapter, netbuf_recv_pkt, pkt_type);

	adapter->os_intf_ops->onebox_mem_free(context_ptr);
	onebox_rx_urb_submit(adapter, urb, context_ptr->ep_num);
	return;

resubmit:
	adapter->os_intf_ops->onebox_mem_free(context_ptr);
	adapter->os_intf_ops->onebox_free_pkt(adapter, netbuf_recv_pkt, 0);
	onebox_rx_urb_submit(adapter, urb, context_ptr->ep_num);
	return;
}

/**
 * This function writes multiple bytes of information to the SD card.
 *
 * @param  Pointer to Driver adapter structure.  
 * @param  Function Number.  
 * @param  Address of the register.  
 * @param  Length of the data.  
 * @param  Pointer to the data that has to be written.  
 * @return On success ONEBOX_STATUS_SUCCESS else ONEBOX_STATUS_FAILURE. 
 */
ONEBOX_STATUS write_ta_register_multiple(PONEBOX_ADAPTER adapter,
                                      uint32 Addr,
                                      uint8 *data,
                                      uint32 Count
                                     )
{
	ONEBOX_STATUS status_l;
	uint8 *buf;
	uint32 transfer;
	
	//printk("PROBE: In %s Line %d\n", __func__, __LINE__);
	buf = kzalloc(4096, GFP_KERNEL);
	if (!buf)
	{
		printk("%s: Failed to allocate memory\n",__func__);
		return -ENOMEM;
	}
	
	status_l = ONEBOX_STATUS_SUCCESS;
	
	while (Count) 
	{
		transfer = min_t(int, Count, 4096);

		printk("In %s Line %d transfer %d\n ",__func__,__LINE__,transfer);
		memcpy(buf, data, transfer);
		printk("ctrl pipe number is %8x\n",usb_sndctrlpipe(adapter->usbdev, 0));
	
		status_l = usb_control_msg (adapter->usbdev,
		                            usb_sndctrlpipe (adapter->usbdev, 0),
		                            USB_VENDOR_REGISTER_WRITE, USB_TYPE_VENDOR,
		                            ((Addr & 0xffff0000) >> 16), (Addr & 0xffff),
		                            (void *) buf, transfer, HZ * 5);
		if (status_l < 0) 
		{
			ONEBOX_DEBUG (ONEBOX_ZONE_INFO,
			              ("REG WRITE FAILED WITH ERROR CODE :%10d \n", status_l));
			kfree(buf);
			return status_l;
		}
		else 
		{
			Count -= transfer;
			data += transfer;
			Addr += transfer;
		}
	}
	ONEBOX_DEBUG (ONEBOX_ZONE_INFO,
	              ("REG WRITE WAS SUCCESSFUL :%10d \n", status_l));
	kfree(buf);
	return 0;
} /* End <write_register_multiple> */

/**
 * This function reads multiple bytes of information to the SD card.
 *
 * @param  Pointer to Driver adapter structure.  
 * @param  Function Number.  
 * @param  Address of the register.  
 * @param  Length of the data.  
 * @param  Pointer to the data that has to be written.  
 * @return On success ONEBOX_STATUS_SUCCESS else ONEBOX_STATUS_FAILURE. 
 */
ONEBOX_STATUS read_ta_register_multiple(PONEBOX_ADAPTER adapter,
                                      uint32 Addr,
                                      uint8 *data,
                                      uint32 Count
                                     )
{
	ONEBOX_STATUS status_l;
	uint8 *buf;
	uint32 transfer;
	
	//printk("PROBE: In %s Line %d\n", __func__, __LINE__);
	buf = kzalloc(4096, GFP_KERNEL);
	if (!buf)
	{
		//printk("PROBE: Failed to allocate memory\n");
		return -ENOMEM;
	}
	
	status_l = ONEBOX_STATUS_SUCCESS;
	while (Count) 
	{
		transfer = min_t(int, Count, 4096);

		printk("In %s Line %d transfer %d count %d \n ",__func__,__LINE__,transfer,Count);
		status_l = usb_control_msg (adapter->usbdev,
		                            usb_rcvctrlpipe (adapter->usbdev, 0),
		                            USB_VENDOR_REGISTER_READ,
	                            	    USB_TYPE_VENDOR | USB_DIR_IN | USB_RECIP_DEVICE,
		                            ((Addr & 0xffff0000) >> 16), (Addr & 0xffff),
		                            (void *) buf, transfer, HZ * 5);
		memcpy(data, buf, transfer);
		adapter->coex_osi_ops->onebox_dump(ONEBOX_ZONE_ERROR, buf, transfer);
		if (status_l < 0) 
		{
			ONEBOX_DEBUG (ONEBOX_ZONE_INFO,
			              ("REG WRITE FAILED WITH ERROR CODE :%10d \n", status_l));
			kfree(buf);
			return status_l;
		}
		else 
		{
			Count -= transfer;
			data += transfer;
			Addr += transfer;
		}
	}
	ONEBOX_DEBUG (ONEBOX_ZONE_INFO,
	              ("MASTER READ IS SUCCESSFUL :%10d \n", status_l));
	kfree(buf);
	return 0;
} /* End <write_register_multiple> */
EXPORT_SYMBOL(onebox_rx_urb_submit);
