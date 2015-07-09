#include <linux/module.h>
#include <linux/kernel.h>
#include "onebox_datatypes.h"
#include "onebox_common.h"
#include "onebox_pktpro.h"
#include "onebox_sdio_intf.h"


uint32 onebox_zone_enabled = ONEBOX_ZONE_INFO |
                             ONEBOX_ZONE_INIT |
                             ONEBOX_ZONE_OID |
                             ONEBOX_ZONE_MGMT_SEND |
                             ONEBOX_ZONE_MGMT_RCV |
                             ONEBOX_ZONE_DATA_SEND |
                             ONEBOX_ZONE_DATA_RCV |
                             ONEBOX_ZONE_FSM | 
                             ONEBOX_ZONE_ISR |
                             ONEBOX_ZONE_MGMT_DUMP |
                             ONEBOX_ZONE_DATA_DUMP |
                	     			 ONEBOX_ZONE_DEBUG |
                	     			 ONEBOX_ZONE_AUTORATE |
                             ONEBOX_ZONE_PWR_SAVE |
                             ONEBOX_ZONE_ERROR |
                             0;

struct driver_assets d_assets = {0};

static uint sdio_clock     = 0;
static bool enable_high_speed     = 0;

//uint8 firmware_path[256]  = "/system/lib/wifi/firmware_wifi/";
uint8 firmware_path[256]  = "/home/rsi/release/firmware/";
static uint16 driver_mode = 1;
static uint16 coex_mode = 1; /*Default coex mode is WIFI alone */
static uint16 ta_aggr = 0;
static uint16 skip_fw_load = 0; /* Default disable skipping fw loading */
static uint16 fw_load_mode = 1; /* Default fw loading mode is full flash with Secondary Boot loader*/
static uint16 lp_ps_handshake_mode = 0; /* Default No HandShake mode*/
static uint16 ulp_ps_handshake_mode = 2; /* Default PKT HandShake mode*/
static uint16 rf_power_val = 0; /* Default 3.3V */ 
static uint16 device_gpio_type = TA_GPIO; /* Default TA GPIO */ 

module_param(lp_ps_handshake_mode,ushort,0);
MODULE_PARM_DESC(lp_ps_handshake_mode, "Enable (0) No HandShake Mode     \
					Enable (1) GPIO HandShake Mode");
module_param(ulp_ps_handshake_mode,ushort,0);
MODULE_PARM_DESC(ulp_ps_handshake_mode, "Enable (0) No HandShake Mode     \
					 Enable (1) GPIO HandShake Mode \
					 Enable (2) Packet HandShake Mode");
module_param(rf_power_val,ushort,0);
MODULE_PARM_DESC(rf_power_val, "Enable (0) 3.3 Volts power for RF \
				Enable (1) 1.9 Volts power for RF");
module_param(device_gpio_type,ushort,0);
MODULE_PARM_DESC(device_gpio_type, "Enable (TA_GPIO) selects TA GPIO \
				Enable (ULP_GPIO) selects ULP GPIO");

module_param(driver_mode,ushort,0);
MODULE_PARM_DESC(driver_mode, "Enable (1) WiFi mode or Enable (2) RF Eval mode \
                               Enable (3) RF_EVAL_LPBK_CALIB mode (4) RF_EVAL_LPBK mode \
                               Enable (5) QSPI BURNING mode");
module_param(coex_mode,ushort,0);
MODULE_PARM_DESC(coex_mode, "(1) WiFi ALONE (2) WIFI AND BT \
                               (3) WIFI AND ZIGBEE");
module_param(ta_aggr, ushort, 0);
MODULE_PARM_DESC(ta_aggr, "No of pkts to aggregate from TA to host");

module_param(fw_load_mode, ushort, 0);
MODULE_PARM_DESC(fw_load_mode, " FW Download Mode	\
				1 - Full Flash mode with Secondary Boot Loader\
				2 - Full RAM mode with Secondary Boot Loader \
				3 - Flash + RAM mode with Secondary Boot Loader\
				4 - Flash + RAM mode without Secondary Boot Loader");

module_param(skip_fw_load, ushort, 0);
MODULE_PARM_DESC(skip_fw_load, " 1 to Skip fw loading else 2");

module_param(onebox_zone_enabled,uint,0);
module_param(sdio_clock, uint, 0);
module_param(enable_high_speed, bool, 0);
MODULE_PARM_DESC(enable_high_speed, "Enable (1) or Disable (0) High speed mode");
MODULE_PARM_DESC(sdio_clock, "SDIO Clock frequency in MHz");
module_param_string(firmware_path, firmware_path, sizeof(firmware_path), 0);
MODULE_PARM_DESC(firmware_path, "Location of firmware files");

ONEBOX_STATUS read_reg_parameters (PONEBOX_ADAPTER adapter)
{
	struct driver_assets *d_assets = onebox_get_driver_asset();

	if(lp_ps_handshake_mode == 0)
	{
		d_assets->lp_ps_handshake_mode	= NO_HAND_SHAKE;
	}
	else if(lp_ps_handshake_mode == 1)
	{
		d_assets->lp_ps_handshake_mode	= GPIO_HAND_SHAKE;
	}

	if(ulp_ps_handshake_mode == 0)
	{
		d_assets->ulp_ps_handshake_mode	= NO_HAND_SHAKE;
	}
	else if(ulp_ps_handshake_mode == 2)
	{
		d_assets->ulp_ps_handshake_mode	= PACKET_HAND_SHAKE;
	}
	else if(ulp_ps_handshake_mode == 1)
	{
		d_assets->ulp_ps_handshake_mode	= GPIO_HAND_SHAKE;
	}


	if (rf_power_val == 0)
		d_assets->rf_power_val = RF_POWER_3_3;
	else
		d_assets->rf_power_val = RF_POWER_1_9;

	if (device_gpio_type == TA_GPIO)
		d_assets->device_gpio_type = TA_GPIO;
	else
		d_assets->device_gpio_type = ULP_GPIO;

	if (driver_mode == WIFI_MODE_ON)
	{
		ONEBOX_DEBUG(ONEBOX_ZONE_INIT, (TEXT("%s: WiFi mode on\n"), __func__));    
		adapter->Driver_Mode = WIFI_MODE_ON;
	}
	else if (driver_mode == RF_EVAL_MODE_ON)
	{
		ONEBOX_DEBUG(ONEBOX_ZONE_INIT, (TEXT("%s: RF Evaluation mode on\n"), __func__));    
		adapter->Driver_Mode = RF_EVAL_MODE_ON;
	}
	else if (driver_mode == RF_EVAL_LPBK_CALIB)
	{
    /*FIXME: Try to optimize these conditions */
		ONEBOX_DEBUG(ONEBOX_ZONE_INIT, (TEXT("%s: RF Eval LPBK CALIB mode on\n"), __func__));    
		adapter->Driver_Mode = RF_EVAL_LPBK_CALIB; /* RF EVAL mode */
	} 
	else if (driver_mode == RF_EVAL_LPBK)
	{
		ONEBOX_DEBUG(ONEBOX_ZONE_INIT, (TEXT("%s: RF Eval LPBK mode on\n"), __func__));    
		adapter->Driver_Mode = RF_EVAL_LPBK; /* RF EVAL mode */
	} 
	else if (driver_mode == QSPI_FLASHING)
	{
		ONEBOX_DEBUG(ONEBOX_ZONE_INIT, (TEXT("%s: QSPI_FLASHING mode on\n"), __func__));    
		adapter->Driver_Mode = QSPI_FLASHING;
		adapter->flashing_mode = QSPI_FLASHING;
	} 
	else if (driver_mode == QSPI_UPDATE)
	{
		ONEBOX_DEBUG(ONEBOX_ZONE_INIT, (TEXT("%s: QSPI_UPDATE mode on\n"), __func__));    
		adapter->Driver_Mode = QSPI_FLASHING;
		adapter->flashing_mode = QSPI_UPDATE;
	}
	else if (driver_mode == SNIFFER_MODE)
	{
		ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s: Sniffer mode on\n"), __func__));
		adapter->Driver_Mode = SNIFFER_MODE;
	}
	else if (driver_mode == SWBL_FLASHING_NOSBL)
	{
		ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s: Sw Bootloader Flashing mode on\n"), __func__));
		adapter->Driver_Mode = QSPI_FLASHING;
		adapter->flashing_mode = SWBL_FLASHING_NOSBL;
	}
	else if (driver_mode == SWBL_FLASHING_NOSBL_FILE)
	{
		ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s: Sw Bootloader Flashing mode on with Calib from file\n"), __func__));
		adapter->Driver_Mode = QSPI_FLASHING;
		adapter->flashing_mode = SWBL_FLASHING_NOSBL_FILE;
	}
	else if (driver_mode == SWBL_FLASHING_SBL)
	{
		ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s: Sw Bootloader Flashing mode on\n"), __func__));
		adapter->Driver_Mode = QSPI_FLASHING;
		adapter->flashing_mode = SWBL_FLASHING_SBL;
	}
	else
	{
		ONEBOX_DEBUG(ONEBOX_ZONE_INIT, (TEXT("%s: WiFi mode on\n"), __func__));    
		adapter->Driver_Mode = WIFI_MODE_ON;
	} /* End if <condition> */

	if(coex_mode == WIFI_ALONE) {
		ONEBOX_DEBUG(ONEBOX_ZONE_INIT, (TEXT("%s: WiFi ALONE\n"), __func__));    
		adapter->coex_mode = WIFI_ALONE;
	}
	else if(coex_mode == WIFI_BT_CLASSIC) {
		ONEBOX_DEBUG(ONEBOX_ZONE_INIT, (TEXT("%s: WiFi + BT Classic Mode\n"), __func__));    
		adapter->coex_mode = WIFI_BT_CLASSIC;
	}
	else if(coex_mode == WIFI_ZIGBEE) {
		ONEBOX_DEBUG(ONEBOX_ZONE_INIT, (TEXT("%s: WiFi + Zigbee Mode\n"), __func__));    
		adapter->coex_mode = WIFI_ZIGBEE;
	}
	else if(coex_mode == WIFI_BT_LE) {
		ONEBOX_DEBUG(ONEBOX_ZONE_INIT, (TEXT("%s: WiFi + BT Low Energy Mode\n"), __func__));    
		adapter->coex_mode = WIFI_BT_LE;
	}
	else {
		ONEBOX_DEBUG(ONEBOX_ZONE_INIT, (TEXT("%s: DEFAULT WiFi ALONE selelcted as no priority is Mentioned\n"), __func__));    
		adapter->coex_mode = WIFI_ALONE;
	}

	if (fw_load_mode == FULL_FLASH_SBL)
	{
		ONEBOX_DEBUG(ONEBOX_ZONE_INIT, (TEXT("%s: Full flash mode with Secondary Bootloader\n"), __func__));    
		adapter->fw_load_mode = FULL_FLASH_SBL;
	}
	else if (fw_load_mode == FULL_RAM_SBL)
	{
		ONEBOX_DEBUG(ONEBOX_ZONE_INIT, (TEXT("%s: Full RAM mode with Secondary Bootloader\n"), __func__));    
		adapter->fw_load_mode = FULL_RAM_SBL;
	}
	else if (fw_load_mode == FLASH_RAM_SBL)
	{
		ONEBOX_DEBUG(ONEBOX_ZONE_INIT, (TEXT("%s: Flash + RAM mode with Secondary Bootloader\n"), __func__));    
		adapter->fw_load_mode = FLASH_RAM_SBL;
	}
	else if (fw_load_mode == FLASH_RAM_NO_SBL)
	{
		ONEBOX_DEBUG(ONEBOX_ZONE_INIT, (TEXT("%s: Flash + RAM mode without/No Secondary Bootloader\n"), __func__));    
		adapter->fw_load_mode = FLASH_RAM_NO_SBL;
	}
	else
	{
		ONEBOX_DEBUG(ONEBOX_ZONE_INIT, (TEXT("%s: Default Full flash mode with Secondary Bootloader\n"), __func__));
		adapter->fw_load_mode = FULL_FLASH_SBL;
	}

	adapter->skip_fw_load = skip_fw_load;

#if LINUX_VERSION_CODE == KERNEL_VERSION(2, 6, 18)
	if (enable_high_speed)
	{
		ONEBOX_DEBUG(ONEBOX_ZONE_INIT, (TEXT("%s: High speed mode on\n"), __func__));
		adapter->sdio_high_speed_enable = 1;
	}
	else
	{
		adapter->sdio_high_speed_enable = 0;
	} /* End if <condition> */
#endif

	adapter->sdio_clock_speed = sdio_clock;
	d_assets->ta_aggr    = ta_aggr;	
	d_assets->asset_role = adapter->Driver_Mode;

	return ONEBOX_STATUS_SUCCESS;
}
/* This function initializes the common hal */
ONEBOX_STATUS common_hal_init(struct driver_assets *d_assets, PONEBOX_ADAPTER adapter)
{
	//struct onebox_os_intf_operations *os_intf_ops = onebox_get_os_intf_operations_from_origin();
	struct onebox_coex_osi_operations *coex_osi_ops = onebox_get_coex_osi_operations();
	int count;
	
	printk("In %s Line %d initializing common Hal init \n", __func__, __LINE__);
	for (count = 0; count < COEX_SOFT_QUEUES; count++) 
		adapter->os_intf_ops->onebox_netbuf_queue_init(&adapter->coex_queues[count]);

	adapter->os_intf_ops->onebox_netbuf_queue_init(&adapter->deferred_rx_queue);

	ONEBOX_DEBUG(ONEBOX_ZONE_INIT,(TEXT("%s: Mutex init successfull\n"), __func__));

	adapter->os_intf_ops->onebox_init_event(&(adapter->coex_tx_event));
	adapter->os_intf_ops->onebox_init_event(&(adapter->flash_event));
	adapter->os_intf_ops->onebox_init_dyn_mutex(&d_assets->tx_access_lock);
	adapter->os_intf_ops->onebox_init_dyn_mutex(&d_assets->wlan_init_lock);
	adapter->os_intf_ops->onebox_init_dyn_mutex(&d_assets->bt_init_lock);
	adapter->os_intf_ops->onebox_init_dyn_mutex(&d_assets->zigbee_init_lock);
	d_assets->update_tx_status = &update_tx_status;

	if (adapter->os_intf_ops->onebox_init_thread(&(adapter->sdio_scheduler_thread_handle),
	                                    "COEX-TX-Thread",
	                                    0,
	                                    coex_osi_ops->onebox_coex_transmit_thread, 
	                                    adapter) != ONEBOX_STATUS_SUCCESS)
	{
		ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,(TEXT("%s: Unable to initialize thrd\n"), __func__));
		adapter->os_intf_ops->onebox_mem_free(adapter->DataRcvPacket);
		printk("In %s Line %d initializing common Hal init \n", __func__, __LINE__);
		return ONEBOX_STATUS_FAILURE;
	}

	ONEBOX_DEBUG(ONEBOX_ZONE_INIT,(TEXT("%s: Initialized thread & Event\n"), __func__));

	/* start the transmit thread */
	adapter->os_intf_ops->onebox_start_thread( &(adapter->sdio_scheduler_thread_handle));

	printk("Common hal: Init proc entry call\n");
	/* Create proc filesystem */
	if (adapter->os_intf_ops->onebox_init_proc(adapter) != 0)
	{
		ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,(TEXT("%s: Failed to initialize procfs\n"), __func__));
	printk("In %s Line %d initializing common Hal init \n", __func__, __LINE__);
		return ONEBOX_STATUS_FAILURE;
	}
	d_assets->common_hal_fsm = COMMAN_HAL_WAIT_FOR_CARD_READY;
	printk("In %s Line %d initializing common Hal init \n", __func__, __LINE__);

#ifdef USE_WORKQUEUES
/* coex workqeue */
	printk("HAL : Using WORKQUEUES as the interrupt bottom halfs\n");
	adapter->int_work_queue = adapter->os_intf_ops->onebox_create_work_queue("onebox_workerQ");
	if (!adapter->int_work_queue) {
		printk("HAL : Unable to create work queue\n");
		return ONEBOX_STATUS_FAILURE;
	}

	INIT_WORK((struct work_struct *)&adapter->defer_work, &deferred_rx_packet_parser);
#endif

#ifdef USE_TASKLETS
	printk("HAL : Using TASKLETS as the interrupt bottom halfs\n");
	tasklet_init(&adapter->int_bh_tasklet,
		     &deferred_rx_tasklet,
		     adapter);
#endif
#ifdef GPIO_HANDSHAKE
	adapter->os_intf_ops->onebox_gpio_init();
#endif
	return ONEBOX_STATUS_SUCCESS;
}

/**
 * This Function Initializes The HAL
 *
 * @param pointer to HAL control block
 * @return
 *  ONEBOX_STATUS_SUCCESS on success, ONEBOX_STATUS_FAILURE on failure
 */
ONEBOX_STATUS device_init(PONEBOX_ADAPTER adapter, uint8 fw_load)
{
	ONEBOX_STATUS status = ONEBOX_STATUS_SUCCESS;
	uint32 regout_val = 0;
	uint8 bl_req = 0;

	FUNCTION_ENTRY(ONEBOX_ZONE_INIT);
	if(adapter->Driver_Mode == QSPI_FLASHING)
		adapter->fsm_state = FSM_CARD_NOT_READY;
	else
		adapter->fsm_state = FSM_DEVICE_READY;

	if(adapter->fw_load_mode == FULL_FLASH_SBL ||
			adapter->fw_load_mode == FULL_RAM_SBL ||
			adapter->fw_load_mode == FLASH_RAM_SBL) {
		bl_req = 1;
	} else if(adapter->fw_load_mode == FLASH_RAM_NO_SBL) {
		bl_req = 0;
	} else {
		printk("Unexpected fw load mode %d.. Returning failure\n",adapter->fw_load_mode);
		goto fail;
	}

	if(fw_load && !adapter->skip_fw_load) {
		bl_cmd_start_timer(adapter, BL_CMD_TIMEOUT);
		while(!adapter->bl_timer_expired) {
			if(adapter->osd_host_intf_ops->onebox_master_reg_read(adapter, 
						SWBL_REGOUT, &regout_val, 2) < 0) {
				printk("%s:%d REGOUT reading failed..\n",__func__,
						__LINE__);
				goto fail;
			}
			if((regout_val >> 8) == REGOUT_VALID) {
				break;
			}
		}
		if(adapter->bl_timer_expired) {
			printk("%s:%d REGOUT reading timed out..\n",__func__,
					__LINE__);
			printk("Software Boot loader Not Present\n");
			if(bl_req) {
				printk("Expecting Software boot loader tobe present."
						"Enable/flash Software boot loader\n"); 
				goto fail;
			} else {
				printk("Software boot loader is disabled as expected\n");
			}
		} else {
			bl_cmd_stop_timer(adapter);
			printk("Software Boot loader Present\n");
			if(bl_req) {
				printk("Software boot loader is enabled as expected\n");
			} else {
				printk("Expecting Software boot loader tobe disabled\n");
				goto fail;
			}
		}

		printk("Received Board Version Number is %x\n",(regout_val & 0xff));

		if((adapter->osd_host_intf_ops->onebox_master_reg_write(adapter, 
						SWBL_REGOUT, (REGOUT_INVALID | REGOUT_INVALID << 8), 2)) < 0) {
			printk("%s:%d REGOUT writing failed..\n",__func__,
					__LINE__);
			goto fail;
		}

		if(adapter->fw_load_mode == FLASH_RAM_NO_SBL)
			status = load_ta_instructions(adapter);
		else
			status = load_fw_thru_sbl(adapter);
	}
	FUNCTION_EXIT(ONEBOX_ZONE_INIT);
	return status;
fail:
	return ONEBOX_STATUS_FAILURE;
} /* onebox_device_init */

/**
 * This Function Free The Memory Allocated By HAL Module
 *
 * @param pointer to HAL control block
 *
 * @return
 * ONEBOX_STATUS_SUCCESS on success, ONEBOX_STATUS_FAILURE on failure
 */
ONEBOX_STATUS device_deinit(PONEBOX_ADAPTER adapter)
{

	FUNCTION_ENTRY(ONEBOX_ZONE_INFO);

	//adapter->coex_osi_ops->onebox_core_deinit(adapter);

#ifdef GPIO_HANDSHAKE
	adapter->os_intf_ops->onebox_gpio_deinit();
#endif
	FUNCTION_EXIT(ONEBOX_ZONE_INFO);
	return ONEBOX_STATUS_SUCCESS;
}

static struct onebox_os_intf_operations    *os_intf_ops;

int onebox_register_os_intf_operations (struct onebox_os_intf_operations *os_intf_opeartions)
{
	os_intf_ops = os_intf_opeartions;
	return 0;
}
struct onebox_os_intf_operations *onebox_get_os_intf_operations(void)
{
	return os_intf_ops;
}

ONEBOX_STATIC int32 onebox_nongpl_module_init(VOID)
{
	ONEBOX_DEBUG(ONEBOX_ZONE_INFO,(TEXT("onebox_nongpl_module_init called and registering the nongpl driver\n")));
	return 0;
}

ONEBOX_STATIC VOID onebox_nongpl_module_exit(VOID)
{
	ONEBOX_DEBUG(ONEBOX_ZONE_INFO,(TEXT("onebox_nongpl_module_exit called and unregistering the nongpl driver\n")));
	return;
}

module_init(onebox_nongpl_module_init);
module_exit(onebox_nongpl_module_exit);
EXPORT_SYMBOL(onebox_zone_enabled);
EXPORT_SYMBOL(onebox_register_os_intf_operations);
EXPORT_SYMBOL(onebox_get_os_intf_operations);
//FIXME This should be passed through adapter, should not be exported
EXPORT_SYMBOL(firmware_path);
