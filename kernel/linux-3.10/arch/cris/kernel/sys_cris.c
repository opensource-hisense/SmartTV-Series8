/* $Id: //DTV/MP_BR/DTV_X_IDTV1501_1538_4_001_46_007/apollo/linux_core/kernel/linux-3.10/arch/cris/kernel/sys_cris.c#1 $
 *
 * linux/arch/cris/kernel/sys_cris.c
 *
 * This file contains various random system calls that
 * have a non-standard calling sequence on some platforms.
 * Since we don't have to do any backwards compatibility, our
 * versions are done in the most "normal" way possible.
 *
 */

#include <linux/errno.h>
#include <linux/sched.h>
#include <linux/syscalls.h>
#include <linux/mm.h>
#include <linux/fs.h>
#include <linux/smp.h>
#include <linux/sem.h>
#include <linux/msg.h>
#include <linux/shm.h>
#include <linux/stat.h>
#include <linux/mman.h>
#include <linux/file.h>
#include <linux/ipc.h>

#include <asm/uaccess.h>
#include <asm/segment.h>

asmlinkage long
sys_mmap2(unsigned long addr, unsigned long len, unsigned long prot,
          unsigned long flags, unsigned long fd, unsigned long pgoff)
{
	/* bug(?): 8Kb pages here */
        return sys_mmap_pgoff(addr, len, prot, flags, fd, pgoff);
}
