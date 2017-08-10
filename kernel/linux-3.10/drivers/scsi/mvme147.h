#ifndef MVME147_H

/* $Id: //DTV/MP_BR/DTV_X_IDTV1501_1538_4_001_46_007/apollo/linux_core/kernel/linux-3.10/drivers/scsi/mvme147.h#1 $
 *
 * Header file for the MVME147 built-in SCSI controller for Linux
 *
 * Written and (C) 1993, Hamish Macdonald, see mvme147.c for more info
 *
 */

#include <linux/types.h>

int mvme147_detect(struct scsi_host_template *);
int mvme147_release(struct Scsi_Host *);

#ifndef CMD_PER_LUN
#define CMD_PER_LUN		2
#endif

#ifndef CAN_QUEUE
#define CAN_QUEUE		16
#endif

#endif /* MVME147_H */
