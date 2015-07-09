/**
 * @file onebox_usb_main_osi.c
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
#include "onebox_sdio_intf.h"

/**
 * This function writes the packet to the device.
 * @param  Pointer to the driver's private data structure
 * @param  Pointer to the data to be written on to the device
 * @param  Length of the data to be written on to the device.  
 * @return 0 if success else a negative number. 
 */
ONEBOX_STATUS host_intf_write_pkt(PONEBOX_ADAPTER adapter, uint8 *pkt, uint32 Len, uint8 q_no)
{
	uint32 block_size    = adapter->TransmitBlockSize;
	uint32 num_blocks,Address,Length;
	uint32 queueno = (q_no & 0x7);
#ifndef RSI_SDIO_MULTI_BLOCK_SUPPORT
	uint32 ii;
#endif  
	ONEBOX_STATUS status = ONEBOX_STATUS_SUCCESS;
	if( !Len  && (queueno == ONEBOX_WIFI_DATA_Q ))
	{
		ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,(TEXT(" Wrong length \n")));
		return ONEBOX_STATUS_FAILURE;
	} /* End if <condition> */  
	num_blocks = Len/block_size;
	if (Len % block_size)
	{
		num_blocks++;
	}
#ifdef ENABLE_SDIO_CHANGE
	if (num_blocks < 2)
	{
		num_blocks = 2;
	}
#endif
	Address = num_blocks * block_size | (queueno << 12);
	Length  = num_blocks * block_size;

#if 0		
	Address = (queueno == ONEBOX_WIFI_MGMT_Q) ? 1 : 2;  //FIXME: This is endpoint num which should be calculated from pkt[8] & 0xff epnum = 1(Int mgmt) 2 (Data)
#else
	/* Fill endpoint numbers based on queueno */
	if ((queueno == COEX_TX_Q) || (queueno == WLAN_TX_M_Q) || (queueno == WLAN_TX_D_Q)) {
		Address = 1;
	} else if ((queueno == BT_TX_Q) || (queueno == ZIGB_TX_Q)) {
		Address = 2;	
	}
#endif
#ifdef RSI_SDIO_MULTI_BLOCK_SUPPORT
	status = adapter->osd_host_intf_ops->onebox_write_multiple(adapter,
	                                                           Address, 
	                                                           (uint8 *)pkt,
	                                                           Len);
	if (status != ONEBOX_STATUS_SUCCESS)
	{
		ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,
		             (TEXT("%s: Unable to write onto the card: %d\n"),__func__,
		             status));
		printk(" <======= Desc details written previously ========== >\n");
		adapter->coex_osi_ops->onebox_dump(ONEBOX_ZONE_ERROR, (PUCHAR)adapter->prev_desc, FRAME_DESC_SZ);
		printk(" Current Pkt: Card write PKT details\n");
		adapter->coex_osi_ops->onebox_dump(ONEBOX_ZONE_ERROR, (PUCHAR)pkt, Length);
		onebox_abort_handler(adapter);
	} /* End if <condition> */
#else
	/* Non multi block read */
	for(ii = 0; ii < num_blocks; ii++)
	{
		if(ii==0)
		{
			status = adapter->osd_host_intf_ops->onebox_write_multiple(adapter,
			                                                           (num_blocks*block_size |
			                                                           (queueno<<12)),
			                                                           pkt + (ii*block_size),
			                                                           block_size);
		}
		else
		{
			status = adapter->osd_host_intf_ops->onebox_write_multiple(adapter,
			                                                           (num_blocks*block_size),
			                                                           pkt + (ii*block_size),
			                                                           block_size);
		} /* End if <condition> */
		if(status != ONEBOX_STATUS_SUCCESS) 
		{
			ONEBOX_DEBUG(ONEBOX_ZONE_DATA_SEND,
			             (TEXT("Multi Block Support: Writing to CARD Failed\n")));
			onebox_abort_handler(adapter);
			status = ONEBOX_STATUS_FAILURE;
		} /* End if <condition> */
	} /* End for loop */
#endif
	ONEBOX_DEBUG(ONEBOX_ZONE_DATA_SEND,
	             (TEXT("%s:Successfully written onto card\n"),__func__));
	return status;
}/*End <host_intf_write_pkt>*/

/**
 * This function reads the packet from the SD card.
 * @param  Pointer to the driver's private data structure
 * @param  Pointer to the packet data  read from the the device
 * @param  Length of the data to be read from the device.  
 * @return 0 if success else a negative number. 
 */
ONEBOX_STATUS host_intf_read_pkt(PONEBOX_ADAPTER adapter,uint8 *pkt,uint32 length)
{
	//uint32 Blocksize = adapter->ReceiveBlockSize;
	ONEBOX_STATUS Status  = ONEBOX_STATUS_SUCCESS;
	//uint32 num_blocks;

	ONEBOX_DEBUG(ONEBOX_ZONE_DATA_RCV,(TEXT( "%s: Reading %d bytes from the card\n"),__func__, length));
	if (!length)
	{
		//ONEBOX_DEBUG(ONEBOX_ZONE_DEBUG,
		 //            (TEXT( "%s: Pkt size is zero\n"),__func__));
		return Status;
	}

	//num_blocks = (length / Blocksize);
	
	/*Reading the actual data*/  
	Status = adapter->osd_host_intf_ops->onebox_read_multiple(adapter,
	                                                          length,
	                                                          length, /*num of bytes*/
	                                                          (uint8 *)pkt); 

	if (Status != ONEBOX_STATUS_SUCCESS)
	{
		ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,(TEXT(
		             "%s: Failed to read frame from the card: %d\n"),__func__,
		             Status));
		printk("Card Read PKT details len =%d :\n", length);
		adapter->coex_osi_ops->onebox_dump(ONEBOX_ZONE_ERROR, (PUCHAR)pkt, length);

		return Status;
	}
	return Status;
}

/**
 * This function intern calls the SDIO slave registers initialization function 
 * where SDIO slave registers are being initialized.
 *
 * @param  Pointer to our adapter structure.  
 * @return 0 on success and -1 on failure. 
 */
ONEBOX_STATUS init_host_interface(PONEBOX_ADAPTER adapter)
{
	ONEBOX_DEBUG(ONEBOX_ZONE_INIT,(TEXT("%s: Initializing interface\n"),__func__)); 
	ONEBOX_DEBUG(ONEBOX_ZONE_INFO,(TEXT("%s: Sending init cmd\n"),__func__)); 

	/* Initialise the SDIO slave registers */
	return onebox_init_sdio_slave_regs(adapter);
}/* End <onebox_init_interface> */


/**
 * This function does the actual initialization of SDBUS slave registers. 
 *
 * @param  Pointer to the driver adapter structure. 
 * @return ONEBOX_STATUS_SUCCESS on success and ONEBOX_STATUS_FAILURE on failure. 
 */

ONEBOX_STATUS onebox_init_sdio_slave_regs(PONEBOX_ADAPTER adapter)
{
	uint8 byte;
	ONEBOX_STATUS status = ONEBOX_STATUS_SUCCESS;
	uint8 reg_dmn;
	FUNCTION_ENTRY(ONEBOX_ZONE_INIT);

	reg_dmn = 0; //TA domain
	/* initialize Next read delay */
	if(adapter->next_read_delay)
	{
		ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,
		             (TEXT("%s: Initialzing SDIO_NXT_RD_DELAY2\n"), __func__));
		byte = adapter->next_read_delay;

		status = adapter->osd_host_intf_ops->onebox_write_register(adapter,
		                                                           reg_dmn,
		                                                           SDIO_NXT_RD_DELAY2,&byte);
		if(status)
		{
			ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,
			             (TEXT("%s: fail to write SDIO_NXT_RD_DELAY2\n"), __func__));
			return ONEBOX_STATUS_FAILURE;
		}
	}

	if(adapter->sdio_high_speed_enable)
	{
#define SDIO_REG_HIGH_SPEED      0x13
		ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,
		             (TEXT("%s: Enabling SDIO High speed\n"), __func__));
		byte = 0x3;

		status = adapter->osd_host_intf_ops->onebox_write_register(adapter,reg_dmn,SDIO_REG_HIGH_SPEED,&byte);              
		if(status)
		{
			ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,
			             (TEXT("%s: fail to enable SDIO high speed\n"), __func__));
			return ONEBOX_STATUS_FAILURE;
		}
	}

	/* This tells SDIO FIFO when to start read to host */
	ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,
	             (TEXT("%s: Initialzing SDIO read start level\n"), __func__));
	byte = 0x24;

	status = adapter->osd_host_intf_ops->onebox_write_register(adapter,reg_dmn,SDIO_READ_START_LVL,&byte);              
	if(status)
	{
		ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,
		             (TEXT("%s: fail to write SDIO_READ_START_LVL\n"), __func__));
		return ONEBOX_STATUS_FAILURE;
	} 

	/* Change these parameters to load firmware */
	/* This tells SDIO FIFO when to start read to host */
	ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,
	             (TEXT("%s: Initialzing FIFO ctrl registers\n"), __func__));
	byte = (128-32);

	status = adapter->osd_host_intf_ops->onebox_write_register(adapter,reg_dmn,SDIO_READ_FIFO_CTL,&byte);              
	if(status)
	{
		ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,
		             (TEXT("%s: fail to write SDIO_READ_FIFO_CTL\n"), __func__));
		return ONEBOX_STATUS_FAILURE;
	}

	/* This tells SDIO FIFO when to start read to host */
	byte = 32;
	status = adapter->osd_host_intf_ops->onebox_write_register(adapter,reg_dmn,SDIO_WRITE_FIFO_CTL,&byte);              
	if(status)
	{
		ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,
		             (TEXT("%s: fail to write SDIO_WRITE_FIFO_CTL\n"), __func__));
		return ONEBOX_STATUS_FAILURE;
	}

	FUNCTION_EXIT(ONEBOX_ZONE_INIT);
	return ONEBOX_STATUS_SUCCESS;
}/* End <onebox_init_sdio_slave_regs> */

/**
 * This function reads the abort register until it is cleared.
 *
 * This function is invoked when ever CMD53 read or write gets
 * failed. 
 *
 * @param  Pointer to the driver adapter structure. 
 * @return 0 on success and -1 on failure. 
 */

ONEBOX_STATUS onebox_abort_handler(PONEBOX_ADAPTER adapter )
{
	return ONEBOX_STATUS_SUCCESS;
}
