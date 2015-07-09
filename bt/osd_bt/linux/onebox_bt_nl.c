/**
 * @file      		onebox_bt_nl.c
 * @version             1.1
 * @date                2014-Nov-18
 *
 * Copyright(C) Redpine Signals 2013
 * All rights reserved by Redpine Signals.
 *
 * @section License
 * This program should be used on your own responsibility.
 * Redpine Signals assumes no responsibility for any losses
 * incurred by customers or third parties arising from the use of this file.
 *
 * @brief This contains all the functions with netlink socket
 * usage
 *
 * @section Description
 * This file contains Bluetooth netlink infrastructre
 *
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/skbuff.h>
#include <linux/errno.h>

#include <net/genetlink.h>
#include <net/sock.h>

#include "onebox_common.h"
#include "onebox_linux.h"
#include "onebox_coex.h"

static struct genl_cb *global_gcb;
int32 onebox_genlrecv(struct sk_buff *rskb, struct genl_info *info);

#define ONEBOX_BT_GENL_FAMILY "Obx-BTgenl"
			     
#define GET_ADAPTER_FROM_GENLCB(gcb) \
		(PONEBOX_ADAPTER)((gcb)->gc_drvpriv)

/* 
 * attribute policy: defines which attribute has 
 * which type (e.g int, char * etc)
 * possible values defined in net/netlink.h
 */
static struct nla_policy bt_genl_policy[RSI_USER_A_MAX + 1] = {
	[RSI_USER_A_MSG] = { .type = NLA_NUL_STRING },
};

/* family definition */
static struct genl_family bt_genl_family = {
	.id      = 0,
	.hdrsize = 0,
	.name    = ONEBOX_BT_GENL_FAMILY,
	.version = RSI_VERSION_NR,
	.maxattr = RSI_USER_A_MAX,
};

static struct genl_ops bt_genl_ops = {
	.cmd    = RSI_USER_C_CMD,
	.flags  = 0,
	.policy = bt_genl_policy,
	.doit   = onebox_genlrecv,
	.dumpit = NULL,
};

/*==============================================*/
/**
 * @fn          onebox_genlrecv
 *              
 * @brief       Gets the command request from user space
 *              over netlink socket
 * @param[in]   struct sk_buff *skb_2, pointer to sk_buff structure
 * @param[in]   struct genl_info *info, read command info pointer
 * @param[out]  none
 * @return      errCode
 *              0  = SUCCESS
 *              else FAIL
 * @section description
 * This API is used to read the command from user over netlink
 * socket.
 */
int32 onebox_genlrecv(struct sk_buff *skb, struct genl_info *info)
{
	uint8 *data;
	int32 rc = -1, len, pkttype;
	struct genl_cb *gcb;
	netbuf_ctrl_block_t *netbuf_cb;
	PONEBOX_ADAPTER adapter = NULL;

	if (!(gcb = global_gcb))
		return -1;

	if (!(adapter = GET_ADAPTER_FROM_GENLCB(global_gcb)))
		return -1;

	gcb->gc_info = info;
	gcb->gc_skb = skb;

	data = adapter->os_intf_ops->onebox_genl_recv_handle(gcb);
	if (!data) {
		ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,
		   (TEXT("%s: genlrecv handler fail on family `%s'\n"),
		   __func__, gcb->gc_name)); 
		goto err;
	}

	gcb->gc_info = NULL;
	gcb->gc_skb  = NULL;

	pkttype = *(uint16 *)&data[0];
	len     = *(uint16 *)&data[2];

	data += 16;

	ONEBOX_DEBUG(ONEBOX_ZONE_INFO,
	   (TEXT("%s: len %x pkttype %x\n"), __func__, len, pkttype));

	adapter->osi_bt_ops->onebox_dump(ONEBOX_ZONE_INFO, data, len);
	
	netbuf_cb = adapter->os_intf_ops->onebox_alloc_skb(len + 
			REQUIRED_HEADROOM_FOR_BT_HAL);
	if (!netbuf_cb) {
		rc = -ENOMEM;
		goto err;
	}

	adapter->os_intf_ops->onebox_reserve_data(netbuf_cb, 
			REQUIRED_HEADROOM_FOR_BT_HAL);

	adapter->os_intf_ops->onebox_add_data_to_skb(netbuf_cb, len);

	adapter->os_intf_ops->onebox_memcpy(netbuf_cb->data, data, len);

	netbuf_cb->bt_pkt_type = pkttype; 

	rc = adapter->osi_bt_ops->onebox_bt_xmit(adapter, netbuf_cb);
	if (rc) 
		goto err;

	return 0;

err:
	ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,
		(TEXT("%s: error(%d) occured \n"), __func__, rc)); 
	return rc;
}

/**
 * @fn          send_pkt_to_btgenl
 *              
 * @brief       Sends a message to the userspace
 *              over netlink socket
 * @param[in]   PONEBOX_ADAPTER adapter pointer to sk_buff adapter
 * @param[in]   netbuf_ctrl_block_t *netbuf_cb pointer to the message 
 * 		which has to be sent to the application.
 * @param[out]  none
 * @return      errCode
 *              0  = SUCCESS
 *              else FAIL
 * @section description
 * This API is used to send a command to the user over netlink
 * socket.
 */
int32 send_pkt_to_btgenl(PONEBOX_ADAPTER adapter, 
			 netbuf_ctrl_block_t *netbuf_cb)
{
	int8 rc = -1;
	uint8 *data;
	uint32 len;
	struct genl_cb *gcb;

	if (!(gcb = adapter->genl_cb))
		return -EFAULT;

	data = netbuf_cb->data;
	len  = netbuf_cb->len;

	if (!data || !len) 
		return -ENODATA;

	rc = adapter->os_intf_ops->onebox_genl_app_send(gcb, netbuf_cb);
	if (rc) {
		ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,
		   (TEXT("%s: netbuf_cb %p fail(%d) to send\n"),
		   __func__, netbuf_cb, rc));
		return rc;
	}

	return rc;
}

/*==============================================*/
/**
 * @fn          btgenl_init
 * @brief       Registers genl family and operations
 * @param[in]   none
 * @param[out]  none
 * @return      errCode
 *              0  = SUCCESS
 *              else FAIL
 * @section description
 * This API is used to register genl ops and family.
 */
int32 btgenl_init(PONEBOX_ADAPTER adapter)
{
	int32 rc = -1;
	struct genl_cb *gcb;

	gcb = adapter->os_intf_ops->onebox_mem_zalloc(sizeof(*gcb),
			GFP_KERNEL);
	if (!gcb) 
		return -ENOMEM;

	gcb->gc_drvpriv = adapter;
	global_gcb = adapter->genl_cb = gcb;
	
	gcb->gc_family  = &bt_genl_family;
	gcb->gc_policy  = &bt_genl_policy[0];
	gcb->gc_ops     = &bt_genl_ops;	
	gcb->gc_n_ops   = 1;
	gcb->gc_name    = ONEBOX_BT_GENL_FAMILY;
	gcb->gc_pid     = gcb->gc_done = 0;
	gcb->gc_assetid = BT_ID;

	rc = adapter->os_intf_ops->onebox_genl_init(gcb);
	if (rc) {
		ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,
		   (TEXT("%s: family %s genl "
		         "registration fail(%d)\n"),
		   __func__, gcb->gc_name, rc)); 
		goto out_genl_init;
	}

	return 0;

out_genl_init:
	ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,
	   (TEXT("%s: genl_init failed  error(%d) occured\n"), __func__, rc));

	adapter->os_intf_ops->onebox_mem_free(gcb);
	adapter->genl_cb = global_gcb = NULL;

	return rc;
}

/*==============================================*/
/**
 * @fn          btgenl_deinit
 * @brief       Unregisters genl family and operations
 * @param[in]   none
 * @param[out]  none
 * @return      errCode
 *              0  = SUCCESS
 *              else FAIL
 * @section description
 * This API is used to unregister genl related ops.
 */
int32 btgenl_deinit(PONEBOX_ADAPTER adapter)
{
	int32 rc = -1;
	struct genl_cb *gcb;

	gcb = adapter->genl_cb;

	rc = adapter->os_intf_ops->onebox_genl_deinit(gcb);
	if (rc) {
		ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,
		   (TEXT("%s: family %s genl deinit fail(%d)"),
		   __func__, gcb->gc_name, rc)); 
		return rc;
	}

	if (gcb) {
		adapter->os_intf_ops->onebox_mem_free(gcb);
		global_gcb = adapter->genl_cb = NULL;
	}

	return rc;
}
