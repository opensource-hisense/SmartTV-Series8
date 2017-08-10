/* $Id: //DTV/MP_BR/DTV_X_IDTV1501_1538_4_001_46_007/apollo/linux_core/kernel/linux-3.10/drivers/isdn/hardware/eicon/adapter.h#1 $ */

#ifndef __DIVA_USER_MODE_IDI_ADAPTER_H__
#define __DIVA_USER_MODE_IDI_ADAPTER_H__

#define DIVA_UM_IDI_ADAPTER_REMOVED 0x00000001

typedef struct _diva_um_idi_adapter {
	struct list_head link;
	DESCRIPTOR d;
	int adapter_nr;
	struct list_head entity_q;	/* entities linked to this adapter */
	dword status;
} diva_um_idi_adapter_t;


#endif
