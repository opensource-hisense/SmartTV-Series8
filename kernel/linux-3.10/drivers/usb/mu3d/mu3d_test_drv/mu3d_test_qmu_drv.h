#ifndef DDR_QMU_H
#define DDR_QMU_H

#include "mu3d_test_usb_drv.h"
#include "../mu3d_hal/mu3d_hal_osal.h"
#include "../mu3d_hal/mu3d_hal_hw.h"


#undef EXTERN

#ifdef _QMU_DRV_EXT_
#define EXTERN 
#else 
#define EXTERN extern
#endif

EXTERN DEV_INT8 cfg_rx_zlp_en;
EXTERN DEV_INT8 cfg_rx_coz_en;
EXTERN DEV_INT8 g_run_stress;
EXTERN DEV_INT8 g_insert_hwo;
EXTERN DEV_UINT32 rx_done_count;
EXTERN DEV_UINT32 rx_IOC_count;



EXTERN void mu3d_hal_alloc_qmu_mem(void);
EXTERN void mu3d_hal_init_qmu(void);
EXTERN void dev_QMURX(DEV_INT32 ep_rx);
EXTERN DEV_UINT8 dev_ep_reset(void);
EXTERN void dev_set_dma_busrt_limiter(DEV_INT8 busrt,DEV_INT8 limiter);
EXTERN void dev_tx_rx(DEV_INT32 ep_rx,DEV_INT32 ep_tx);
EXTERN void dev_qmu_loopback(DEV_INT32 ep_rx,DEV_INT32 ep_tx);
EXTERN void dev_qmu_loopback_ext(DEV_INT32 ep_rx,DEV_INT32 ep_tx);
EXTERN void dev_notification(DEV_INT8 type,DEV_INT32 valuel,DEV_INT32 valueh);
EXTERN irqreturn_t u3d_vbus_rise_handler(int irq, void *dev_id);
EXTERN irqreturn_t u3d_vbus_fall_handler(int irq, void *dev_id);
EXTERN irqreturn_t u3d_inter_handler(int irq, void *dev_id);
EXTERN void dev_start_stress(USB_DIR dir,DEV_INT32 ep_num);
EXTERN void dev_prepare_gpd(DEV_INT32 num,USB_DIR dir,DEV_INT32 ep_num,DEV_UINT8* buf);
EXTERN void dev_prepare_gpd_short(DEV_INT32 num,USB_DIR dir,DEV_INT32 ep_num,DEV_UINT8* buf);
EXTERN void dev_prepare_stress_gpd(DEV_INT32 num,USB_DIR dir,DEV_INT32 ep_num,DEV_UINT8* buf);
EXTERN void mu3d_hal_restart_qmu_no_flush(DEV_INT32 Q_num, USB_DIR dir, DEV_INT8 method);
EXTERN void dev_qmu_rx(DEV_INT32 ep_rx);
EXTERN void gpd_ptr_align(USB_DIR dir,DEV_UINT32 num,PGPD ptr);
EXTERN void *bd_phys_to_virt(void *paddr,USB_DIR dir,DEV_UINT32 num);


#undef EXTERN


#endif
