
/**
 * @file onebox_os_intf_ops.h
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
 * This file contians the function prototypes of the callbacks used across
 * differnet layers in the driver
 * 
 */
#include "onebox_common.h"
#include "onebox_thread.h"
#include "onebox_linux.h"

#ifdef USE_USB_INTF
#include <linux/usb.h>
#endif

/* OS_INTERFACE OPERATIONS */
struct onebox_os_intf_operations 
{
	uint8* (*onebox_strcpy)(char *s1, const char *s2);
	void (*onebox_mem_alloc)(void **ptr,uint16 len, gfp_t flags);
	void* (*onebox_mem_zalloc)(int size, gfp_t flags);
	void (*onebox_mem_free)(VOID *ptr);
	void  (*onebox_vmem_free)(VOID *ptr); 
	void* (*onebox_memcpy)(void *to, const void *from, int len);
	void* (*onebox_memset)(void *src, int val, int len);
	int   (*onebox_memcmp)(const void *s1,const void *s2,int len);
	void* (*onebox_memmove)(void *s1, const void *s2, int n);
	//uint8 (*onebox_extract_vap_id)(const char *str);
	int (*onebox_wait_event)(ONEBOX_EVENT *event,uint32 timeOut);
	void (*onebox_set_event)(ONEBOX_EVENT *event);
	void (*onebox_reset_event)(ONEBOX_EVENT *event);
	int  (*onebox_mod_timer)( void* timer,
	                          unsigned long expires);
	void  (*onebox_do_gettimeofday)(VOID *tv_now);
	void* (*onebox_vmalloc)(unsigned long size);
	//FIXME: Fill sys_time_struct sowjanya
	// void  (*onebox_fill_sys_time_struct)(SYS_TIME_STRUCT *ptr);
	void  (*onebox_usec_delay)(unsigned int usecs);
	void  (*onebox_msec_delay)(unsigned int msecs);  
	void   (*onebox_acquire_sem_interruptible)(PVOID sem,
	                                           int  delay_msec);
	ONEBOX_STATUS (*onebox_acquire_sem)(PVOID sem, int  delay_msec);
	BOOLEAN (*onebox_release_sem)(PVOID sem);
	int32 (*onebox_init_event)(ONEBOX_EVENT *pEvent);
	int32 (*onebox_delete_event)(ONEBOX_EVENT *pEvent);
	void (*onebox_queue_head_init)(struct sk_buff_head *list);
	void (*onebox_queue_purge)(struct sk_buff_head *list);
	void (*onebox_init_dyn_mutex)(struct semaphore *sem_name);
	void (*onebox_init_static_mutex)(void *mutex);
	int  (*onebox_acquire_mutex)(void *  sem);
	void (*onebox_release_mutex)(PVOID mutex);
	void (*onebox_init_spinlock)(void *lock);
	void (*onebox_acquire_spinlock)(void* lock,unsigned long flags);
	void (*onebox_release_spinlock)(void* lock,unsigned long flags);
	void (*onebox_spin_lock_irqsave)(void *lock , int flags);
	void (*onebox_spin_lock_irqrestore)(void *lock , int flags);
	unsigned long (*onebox_get_jiffies) (void);
	void (*onebox_get_random_bytes)(void *buf, int nbytes);
	void (*onebox_free_pkt)(PONEBOX_ADAPTER oneboxClient,
	                        netbuf_ctrl_block_t* pkt,
	                        ONEBOX_STATUS Status);
	int  (*onebox_init_proc)(PONEBOX_ADAPTER Adapter);
	void (*onebox_remove_proc_entry)(void);
	void (*onebox_init_timer)(PONEBOX_ADAPTER oneboxClient);  
	int (*onebox_start_thread)(onebox_thread_handle_t *handle);
	int (*onebox_init_thread)(onebox_thread_handle_t *handle, uint8 *name,
	                          uint32 priority, thread_function function_ptr,
	                          void *context);
#ifdef USE_USB_INTF
	int (*onebox_init_rx_thread)(onebox_thread_handle_t *handle, uint8 *name,
	                          uint32 priority, thread_function function_ptr,
	                          void *context);
#endif
	#if KERNEL_VERSION_BTWN_2_6_(18, 26)
	int (*onebox_kill_thread)(onebox_thread_handle_t *handle);
	#elif KERNEL_VERSION_GREATER_THAN_2_6_(27)
	int (*onebox_kill_thread)(PONEBOX_ADAPTER adapter);
#ifdef USE_USB_INTF
	int (*onebox_kill_rx_thread)(PONEBOX_ADAPTER adapter);
#endif
	#endif
	void (*onebox_init_workq)(struct work_struct *work, void *function);
	int32 (*onebox_request_irq)(unsigned int irq, int handler,
	                            unsigned long  irqflags,
	                            const char *  devname,
	                            void *  dev_id);
	int  (*onebox_set_irq_type)(unsigned int irq,unsigned int type);
	struct workqueue_struct * (*onebox_create_singlethread_workqueue)(const char *name);
	PUCHAR (*onebox_get_firmware)(const int8 *fn,uint32 *read_length,
	                               uint32 type,uint8 *firmware_path);
	int(*onebox_write_to_file)(const int8 *fn, uint16 *dp, uint32 write_len, uint32 type,
				 uint8 *file_path);
	PUCHAR (*onebox_add_data_to_skb)(netbuf_ctrl_block_t *netbuf_cb, int len);
	netbuf_ctrl_block_t* (*onebox_alloc_skb)(int len);
	int (*onebox_is_ifp_txq_stopped)(void *dev);
	int (*onebox_is_sub_txq_stopped)(void *dev, int ac);
	void (*onebox_start_netq)(void *dev);
	void (*onebox_start_sub_txq)(void *dev, int ac);
	void (*onebox_start_ifp_txq)(void *dev);
	void (*onebox_stop_ifp_txq)(void *dev);
	void (*onebox_stop_sub_txq)(void *dev, int ac);
	void (*onebox_create_kthread)(struct work_struct *work);
	netbuf_ctrl_block_t* (*onebox_allocate_skb)(int len);
	PUCHAR (*onebox_push_data)(void *addr,int len);
  	void (*onebox_reserve_data)(netbuf_ctrl_block_t *netbuf_cb, int reserve_len);
	netbuf_ctrl_block_t* (*onebox_dequeue_pkt)(void * addr);
	PUCHAR (*onebox_change_hdr_size)(netbuf_ctrl_block_t *netbuf_cb, uint16 len);
	void (*onebox_netbuf_queue_init)(void *addr);
	void (*onebox_netbuf_queue_tail)(void *addr,void *buffer);
	int (*onebox_netbuf_queue_len)(void *addr);
	void* (*onebox_get_priv)(void *addr);
	void (*onebox_unregisterdev)(struct net_device *dev);
	int (*onebox_dev_open)(PONEBOX_ADAPTER adapter);
	int (*onebox_dev_close)(PONEBOX_ADAPTER adapter);
	struct net_device * (*onebox_netdevice_op)(void);
	void (*onebox_schedule)(void);
	
	int (*onebox_atomic_read)(void *addr);
	void (*onebox_completion_event)(void *addr,int val );
	
	void (*onebox_netbuf_adj)(netbuf_ctrl_block_t *netbuf_cb_t, int len);
	void (*onebox_netbuf_trim)(netbuf_ctrl_block_t *netbuf_cb_t, int len);
#ifdef BYPASS_RX_DATA_PATH
	void (*onebox_indicate_pkt_to_os)(void *dev, netbuf_ctrl_block_t *netbuf_cb_t);

	void (*onebox_append_netbuf_cb)(netbuf_ctrl_block_t *dest, netbuf_ctrl_block_t *src );
	netbuf_ctrl_block_t * (*onebox_get_last_netbuf_cb)(netbuf_ctrl_block_t *netbuf_cb);
#endif
	void (*onebox_init_sw_timer)(struct timer_list *timer, uint32 data, void *function, unsigned long timeout);
	void (*onebox_add_sw_timer)(void *timer);
	void (*onebox_remove_timer)(void *timer);
	int  (*onebox_sw_timer_pending)(void *timer);
#if KERNEL_VERSION_LESS_THAN_3_6(0)
	int (*onebox_queue_work)(struct workqueue_struct *wq, struct work_struct *work);
#else
	bool (*onebox_queue_work)(struct workqueue_struct *, struct work_struct *);
#endif
	void (*onebox_tasklet_sched)(struct tasklet_struct *);
	void (*onebox_netbuf_queue_head)(void *addr,void *buffer);
	struct workqueue_struct * (*onebox_create_work_queue)(uint8 *work_queue_name);
	int32 (*onebox_genl_init)(struct genl_cb * gcb);
	int32 (*onebox_genl_deinit)(struct genl_cb * gcb);
	int32 (*onebox_genl_app_send)(struct genl_cb *gcb,
				      netbuf_ctrl_block_t *netbuf_cb);
	uint8 *(*onebox_genl_recv_handle)(struct genl_cb *gcb);
#ifdef GPIO_HANDSHAKE
	void (*onebox_set_host_status)(int value);
	void (*onebox_gpio_init)(void);
	void (*onebox_gpio_deinit)(void);
	int32 (*onebox_get_device_status)(void );
#endif

};

/* HOST_INTERFACE OPERATIONS */
struct onebox_osd_host_intf_operations
{
	ONEBOX_STATUS (*onebox_read_register)(PONEBOX_ADAPTER adapter, uint32 Addr, uint8 *data);
	int (*onebox_write_register)(PONEBOX_ADAPTER adapter,uint8 reg_dmn,uint32 Addr,uint8 *data);
	ONEBOX_STATUS (*onebox_read_multiple)(PONEBOX_ADAPTER adapter, uint32 Addr, uint32 Count, uint8 *data );
	ONEBOX_STATUS (*onebox_write_multiple)(PONEBOX_ADAPTER adapter, uint32 Addr, uint8 *data, uint32 Count);
	int (*onebox_register_drv)(void);
	void (*onebox_unregister_drv)(void);
	ONEBOX_STATUS (*onebox_remove)(void);
#ifdef USE_USB_INTF
	void (*onebox_rcv_urb_submit) (PONEBOX_ADAPTER adapter, struct urb *urb, uint8 ep_num);
	ONEBOX_STATUS (*onebox_ta_write_multiple)(PONEBOX_ADAPTER adapter, uint32 Addr, uint8 *data, uint32 Count);
	ONEBOX_STATUS (*onebox_ta_read_multiple)(PONEBOX_ADAPTER adapter, uint32 Addr, uint8 *data, uint32 Count);
#endif
	int32 (*onebox_deregister_irq)(PONEBOX_ADAPTER adapter);
	ONEBOX_STATUS (*onebox_master_reg_read)(PONEBOX_ADAPTER adapter,uint32 addr,uint32 *data, uint16 size);
	ONEBOX_STATUS (*onebox_master_reg_write)(PONEBOX_ADAPTER adapter,uint32 addr,uint32 data,uint16 size);
};

/* HOST_INTERFACE OPERATIONS */
struct onebox_osi_host_intf_operations
{
	ONEBOX_STATUS (*onebox_host_intf_read_pkt)(PONEBOX_ADAPTER adapter,uint8 *pkt,uint32 Len);
	ONEBOX_STATUS (*onebox_host_intf_write_pkt)(PONEBOX_ADAPTER adapter,uint8 *pkt,uint32 Len, uint8 queueno);
	ONEBOX_STATUS (*onebox_ack_interrupt)(PONEBOX_ADAPTER adapter,uint8 int_BIT);
	ONEBOX_STATUS (*onebox_disable_sdio_interrupt)(PONEBOX_ADAPTER adapter);
	ONEBOX_STATUS (*onebox_init_host_interface)(PONEBOX_ADAPTER adapter);
	ONEBOX_STATUS (*onebox_master_access_msword)(PONEBOX_ADAPTER adapter, uint16 ms_word);
};
/* EOF */
