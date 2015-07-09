/**
 * @file      	onebox_zigb_nl.c
 * @version	1.0
 * @date        2014-Nov-20
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
 * This file contains the following functions.
 *      zigb_genlrecv
 *      zigb_register_genl
 *      zigb_unregister_genl
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

int32 zigb_genlrecv(struct sk_buff *skb, struct genl_info *info);

#define ONEBOX_ZIGB_GENL_FAMILY "Obx-ZIGBgenl"

#define GET_ADAPTER_FROM_GENLCB(gcb) \
		(PONEBOX_ADAPTER)(gcb->gc_drvpriv)

/* 
 * attribute policy: defines which attribute has 
 * which type (e.g int, char * etc)
 * possible values defined in net/netlink.h
 */
static struct nla_policy zigb_genl_policy[RSI_USER_A_MAX + 1] = {
	[RSI_USER_A_MSG] = { .type = NLA_NUL_STRING },
};

/* family definition */
static struct genl_family zigb_genl_family = {
	.id      = 0,
	.hdrsize = 0,
	.name    = ONEBOX_ZIGB_GENL_FAMILY,
	.version = RSI_VERSION_NR,
	.maxattr = RSI_USER_A_MAX,
};

static struct genl_ops zigb_genl_ops = {
	.cmd    = RSI_USER_C_CMD,
	.flags  = 0,
	.policy = zigb_genl_policy,
	.doit   = zigb_genlrecv,
	.dumpit = NULL,
};

/*==========================================================================/
 * @fn          zigb_genlrecv(struct sk_buff *rskb, struct genl_info *info)
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
 *===========================================================================*/
int32 zigb_genlrecv(struct sk_buff *skb, struct genl_info *info)
{
	uint8 *data;
	uint16 desc;
	int32  rc = -1, len, pkttype;
	struct genl_cb *gcb;
	netbuf_ctrl_block_t *netbuf_cb;
	PONEBOX_ADAPTER adapter = NULL;
	
	if(!(gcb = global_gcb))
		return -1;
	
	adapter = GET_ADAPTER_FROM_GENLCB(global_gcb);
	if (!adapter)
		return -1;

	gcb->gc_info = info;
	gcb->gc_skb  = skb;

	data = adapter->os_intf_ops->onebox_genl_recv_handle(gcb);
	if (!data) {
		ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,
		    (TEXT("%s: genlrecv handler fail on family `%s'\n"),
		    __func__, gcb->gc_name)); 
		goto err;
	}

	gcb->gc_info = NULL;
	gcb->gc_skb  = NULL;

	desc = *(uint16 *)&data[0];
	len = desc & 0x0FFF;
	pkttype = ((desc & 0xF000) >> 12);

	ONEBOX_DEBUG(ONEBOX_ZONE_INFO,
	    (TEXT("%s: rx data, desc %x len %x pkttype %x\n"),
	    __func__, desc, len, pkttype));

	adapter->osi_zigb_ops->onebox_dump(ONEBOX_ZONE_INFO, data, len + 16);
	
	netbuf_cb = adapter->os_intf_ops->onebox_alloc_skb(len + 16);
	if (!netbuf_cb) {
		rc = -ENOMEM;
		goto err;
	}

	adapter->os_intf_ops->onebox_add_data_to_skb(netbuf_cb, len + 16);

	adapter->os_intf_ops->onebox_memcpy(netbuf_cb->data, data, (len + 16));

	rc = adapter->osi_zigb_ops->onebox_send_pkt(adapter, netbuf_cb);
	if (rc) 
		goto err;

	return 0;

err:
	ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,
	    (TEXT("%s: error(%d) occured \n"), __func__, rc)); 
	return rc;
}

/*================================================================/
 * @fn          int32 zigb_register_genl(PONEBOX_ADAPTER adapter)
 * @brief       Registers genl family and operations
 * @param[in]   none
 * @param[out]  none
 * @return      errCode
 *              0  = SUCCESS
 *              else FAIL
 * @section description
 * This API is used to register genl ops and family.
 *==================================================================*/
int32 zigb_register_genl(PONEBOX_ADAPTER adapter)
{
	int32 rc = -1;
	struct genl_cb *gcb;

	gcb = adapter->os_intf_ops->onebox_mem_zalloc(sizeof(*gcb), 
			GFP_KERNEL);
	if (!gcb) {
		rc = -ENOMEM;
		return rc;
	}

	gcb->gc_drvpriv = adapter;
	global_gcb = adapter->genl_cb = gcb;
	
	gcb->gc_family = &zigb_genl_family;
	gcb->gc_policy = &zigb_genl_policy[0];
	gcb->gc_ops    = &zigb_genl_ops;	
	gcb->gc_name   = ONEBOX_ZIGB_GENL_FAMILY;
	gcb->gc_n_ops  = 1;
	gcb->gc_pid    = gcb->gc_done = 0;
	   
	rc = adapter->os_intf_ops->onebox_genl_init(gcb);
	if (rc != 0) 
		goto err;

	return rc;
	
err:
	ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,
		(TEXT("%s: error(%d) occured\n"), __func__, rc));
	adapter->os_intf_ops->onebox_mem_free(gcb);
	adapter->genl_cb = global_gcb = NULL;

	return rc;
}

/*================================================================*/
/**
 * @fn          int32 zigb_unregister_genl(PONEBOX_ADAPTER adapter)
 * @brief       Unregisters genl family and operations
 * @param[in]   none
 * @param[out]  none
 * @return      errCode
 *              0  = SUCCESS
 *              else FAIL
 * @section description
 * This API is used to unregister genl related ops.
 *================================================================*/
int32 zigb_unregister_genl(PONEBOX_ADAPTER adapter)
{
	int rc;
	struct genl_cb *gcb;

	if (!(gcb = adapter->genl_cb))
		return -ENODEV;
     
	rc = adapter->os_intf_ops->onebox_genl_deinit(gcb);
	if (rc != 0) 
		goto err;

	return rc;

err:
	ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,
		(TEXT("%s: error(%d) occured\n"), __func__, rc));
	return rc;

}
