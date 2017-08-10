#ifndef __MTK_EP_QUEUE_H__
#define __MTK_EP_QUEUE_H__

extern uint16_t mtk_get_fifo(MGC_LinuxCd *pThis, uint16_t size);
extern int mtk_put_fifo(MGC_LinuxCd *pThis, uint16_t addr, int size);
extern void mtk_dump_fifo_node(MGC_LinuxCd *pThis);
extern uint8_t mtk_select_endpoint(MGC_LinuxCd *pThis, struct urb *pUrb);

#endif
