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

static PONEBOX_ADAPTER adapter;

static ONEBOX_STATUS bt_gpl_read_pkt(netbuf_ctrl_block_t *netbuf_cb)
{
	struct driver_assets *d_assets = onebox_get_driver_asset();
	PONEBOX_ADAPTER b_adapter = (PONEBOX_ADAPTER)d_assets->techs[BT_ID].priv;

	b_adapter->os_intf_ops->onebox_acquire_sem(&b_adapter->bt_gpl_lock, 0);

	if (d_assets->techs[BT_ID].drv_state == MODULE_ACTIVE) {
		bt_read_pkt(b_adapter, netbuf_cb);
	} else {
		printk("WLAN is being removed.. Dropping Pkt\n");
		b_adapter->os_intf_ops->onebox_free_pkt(b_adapter, netbuf_cb, 0);
		netbuf_cb = NULL;
	}

	b_adapter->os_intf_ops->onebox_release_sem(&b_adapter->bt_gpl_lock);

	return ONEBOX_STATUS_SUCCESS;
}

/*
 * This function deregisters BT firmware
 * @param  Pointer to adapter structure.  
 * @return 0 if success else -1. 
 */

static ONEBOX_STATUS bt_deregister_fw(PONEBOX_ADAPTER adapter)
{
	uint16 *frame_desc;
	uint16 pkt_len;
	netbuf_ctrl_block_t *netbuf_cb = NULL;
	ONEBOX_STATUS status = ONEBOX_STATUS_SUCCESS;

	ONEBOX_DEBUG(ONEBOX_ZONE_INFO,
			(TEXT("===> Deregister BT FW <===\n")));

	netbuf_cb = adapter->os_intf_ops->onebox_alloc_skb(FRAME_DESC_SZ);
	if(netbuf_cb == NULL)
	{	
		ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s: Unable to allocate skb\n"), __func__));
		status = ONEBOX_STATUS_FAILURE;
		return status;

	}
	adapter->os_intf_ops->onebox_add_data_to_skb(netbuf_cb, FRAME_DESC_SZ);

	adapter->os_intf_ops->onebox_memset(netbuf_cb->data, 0, FRAME_DESC_SZ);
	
	frame_desc = (uint16 *)netbuf_cb->data;

	/* packet length without descriptor */
	pkt_len = netbuf_cb->len - FRAME_DESC_SZ;

	/* Assigning packet length */
	frame_desc[0] = pkt_len & 0xFFF;

	/* Assigning queue number */
	frame_desc[0] |= (ONEBOX_CPU_TO_LE16(BT_INT_MGMT_Q) & 0x7) << 12;

	/* Assigning packet type in Last word */
	frame_desc[7] = ONEBOX_CPU_TO_LE16(BT_DEREGISTER);

	netbuf_cb->tx_pkt_type = BT_TX_Q;
	
	adapter->osi_bt_ops->onebox_dump(ONEBOX_ZONE_ERROR, netbuf_cb->data, FRAME_DESC_SZ);
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
 * This function is triggered whenever BT module is 
 * inserted. Links BT module with common hal module 
 * @params void
 * @return ONEBOX_STATUS_SUCCESS on success else ONEBOX_STATUS_FAILURE
 */
static int32 bt_insert(void)
{
	int32 rc = 0;
	struct onebox_osi_host_intf_operations *osi_host_intf_ops = 
			onebox_get_osi_host_intf_operations();
	struct onebox_osd_host_intf_operations *osd_host_intf_ops = 
			onebox_get_osd_host_intf_operations();
	struct onebox_os_intf_operations *os_intf_ops = 
			onebox_get_os_intf_operations();
	struct onebox_bt_osd_operations *osd_bt_ops = 
			onebox_get_bt_osd_operations_from_origin();
	struct onebox_osi_bt_ops *osi_bt_ops = 
			onebox_get_osi_bt_ops();
	struct driver_assets *d_assets = 
			onebox_get_driver_asset();

	ONEBOX_DEBUG(ONEBOX_ZONE_INIT,(TEXT("%s: Initialization function called\n"), __func__));

	os_intf_ops->onebox_acquire_sem(&d_assets->bt_init_lock, 0);

	if(d_assets->techs[BT_ID].drv_state == MODULE_ACTIVE) {
		printk("In %s Line %d BT Module is already initialized\n", __func__, __LINE__);
		os_intf_ops->onebox_release_sem(&d_assets->bt_init_lock);
		return ONEBOX_STATUS_SUCCESS;
	}

	if ((sizeof(ONEBOX_ADAPTER) % 32))
		printk("size of onebox adapter is not 32 byte aligned\n");
		
	adapter = os_intf_ops->onebox_mem_zalloc(sizeof(ONEBOX_ADAPTER), GFP_KERNEL);
	if (!adapter) {
		ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,
		    (TEXT("%s:Memory allocation for adapter failed\n"), __func__));   
		goto nomem;
	}

	os_intf_ops->onebox_memset(adapter, 0, sizeof(ONEBOX_ADAPTER));

	/* Initialise the Core and device dependent operations */
	adapter->osd_host_intf_ops = osd_host_intf_ops;
	adapter->osi_host_intf_ops = osi_host_intf_ops;

	adapter->os_intf_ops = os_intf_ops;
	adapter->osd_bt_ops  = osd_bt_ops;
	adapter->osi_bt_ops  = osi_bt_ops;

	ONEBOX_DEBUG(ONEBOX_ZONE_INIT,(TEXT("%s: Mutex init successfull\n"), __func__));

	os_intf_ops->onebox_init_dyn_mutex(&adapter->bt_gpl_lock);
	
	d_assets->techs[BT_ID].priv = (void *)adapter;
	adapter->onebox_send_pkt_to_coex = send_pkt_to_coex;


	os_intf_ops->onebox_strcpy(adapter->name, "onebox_bt"); 

	adapter->fsm_state = FSM_DEVICE_READY;

	rc = adapter->osi_bt_ops->onebox_core_init(adapter);
	if (rc) {
		ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,(TEXT("%s: BT core init failed\n"), __func__));
		goto nocoreinit;	
	}

	init_waitqueue_head(&d_assets->techs[BT_ID].deregister_event);
	d_assets->techs[BT_ID].default_ps_en = 1;
	d_assets->techs[BT_ID].drv_state = MODULE_ACTIVE;

	if (setup_bt_procfs(adapter))
		ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,(TEXT("%s: Failed to setup BT procfs entry\n"), __func__));
	
	os_intf_ops->onebox_release_sem(&d_assets->bt_init_lock);
	return ONEBOX_STATUS_SUCCESS;

nocoreinit:
	ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,(TEXT("%s: Failed to initialize BT error[%d]\n"), __func__, rc));  
	os_intf_ops->onebox_mem_free(adapter);
nomem:
	os_intf_ops->onebox_release_sem(&d_assets->bt_init_lock);
	return ONEBOX_STATUS_FAILURE;
}/* End <bt_insert> */

/**
 * This function removes the wlan module safely..
 *
 * @param  Pointer to sdio_func structure.  
 * @param  Pointer to sdio_device_id structure.  
 * @return VOID. 
 */
static int32 bt_remove(void)
{
	int32 rc = 0;
	struct onebox_os_intf_operations *os_intf_ops = onebox_get_os_intf_operations();
	struct driver_assets *d_assets = onebox_get_driver_asset();
	struct wireless_techs *bt_d;

	adapter->os_intf_ops->onebox_acquire_sem(&adapter->bt_gpl_lock, 0);

	FUNCTION_ENTRY(ONEBOX_ZONE_INFO);   

	if (d_assets->card_state != GS_CARD_DETACH) {
		bt_d = &d_assets->techs[BT_ID];
		bt_d->tx_intention = 1;
		d_assets->update_tx_status(BT_ID);
		if(!bt_d->tx_access) {
			d_assets->techs[BT_ID].deregister_flags = 1;
			if(wait_event_timeout((bt_d->deregister_event), 
						(d_assets->techs[BT_ID].deregister_flags == 0),
						msecs_to_jiffies(6000))) {
				bt_deregister_fw(adapter);	
			}	else {
				printk("Failed to get sleep exit\n");
			}
		} else
			bt_deregister_fw(adapter);	
		BT_TECH.tx_intention = 0;
		BT_TECH.tx_access = 0;
		d_assets->update_tx_status(BT_ID);
	}

	d_assets->techs[BT_ID].fw_state = FW_INACTIVE;

	destroy_bt_procfs();

	ONEBOX_DEBUG(ONEBOX_ZONE_INIT,(TEXT("%s: calling core deinitialization\n"), __func__));   
	rc = adapter->osi_bt_ops->onebox_core_deinit(adapter);
	if (rc)
		ONEBOX_DEBUG(ONEBOX_ZONE_INIT,(TEXT("%s: failed to deinit BT, error[%d]\n"), __func__, rc));   

	adapter->os_intf_ops->onebox_release_sem(&adapter->bt_gpl_lock);

	os_intf_ops->onebox_mem_free(adapter);

	FUNCTION_EXIT(ONEBOX_ZONE_INFO);
	return ONEBOX_STATUS_SUCCESS;
}/* End <bt_remove> */

ONEBOX_STATIC int32 onebox_bt_gpl_module_init(VOID)
{
	int32 rc = 0;
	struct driver_assets *d_assets =
		onebox_get_driver_asset();

	d_assets->techs[BT_ID].drv_state = MODULE_INSERTED;

	d_assets->techs[BT_ID].inaugurate = bt_insert;
	d_assets->techs[BT_ID].disconnect = bt_remove;
	d_assets->techs[BT_ID].onebox_get_pkt_from_coex = bt_gpl_read_pkt;

	if (d_assets->card_state == GS_CARD_ABOARD) {
		if(d_assets->techs[BT_ID].fw_state == FW_ACTIVE) {
			rc = bt_insert();
			if (rc) {
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,(TEXT ("%s: failed to insert "
					"bt error[%d]\n"),__func__, rc)); 
				return 0;
			}
		}
	}	

	ONEBOX_DEBUG(ONEBOX_ZONE_INFO,(TEXT("onebox_btgpl_module_init called and registering the gpl driver\n")));
	return 0;
}

ONEBOX_STATIC VOID onebox_bt_gpl_module_exit(VOID)
{
	struct driver_assets *d_assets =
		onebox_get_driver_asset();

	printk("BT : GPL module exit\n");
	
	if (d_assets->techs[BT_ID].drv_state == MODULE_ACTIVE) {
		bt_remove();
	} 

	d_assets->techs[BT_ID].drv_state = MODULE_REMOVED;
	
	d_assets->techs[BT_ID].inaugurate = NULL;
	d_assets->techs[BT_ID].disconnect = NULL;
	d_assets->techs[BT_ID].onebox_get_pkt_from_coex = NULL;

	ONEBOX_DEBUG(ONEBOX_ZONE_INFO,(TEXT("onebox_btgpl_module_exit called and unregistering the gpl driver\n")));
	return;
}

module_init(onebox_bt_gpl_module_init);
module_exit(onebox_bt_gpl_module_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Redpine Signals Inc");
MODULE_DESCRIPTION("Co-Existance solution from REDPINE SIGNALS");
MODULE_SUPPORTED_DEVICE("Godavari RS911x WLAN Modules");
MODULE_VERSION("0.1");
