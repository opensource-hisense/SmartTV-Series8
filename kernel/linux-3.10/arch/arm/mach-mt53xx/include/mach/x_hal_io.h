/*
 * Licensed under the GPL
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html#TOC1
 *
 * $Author: dtvbm11 $
 * $Date: 2015/12/22 $
 * $RCSfile:  $
 * $Revision: #1 $
 *
 * hal_io defines
 *
 */

#ifndef __X_HAL_IO_H__
#define __X_HAL_IO_H__

#include <x_typedef.h>
#include <mach/platform.h>
#include <asm/io.h>

// field access macro-----------------------------------------------------------

/* field macros */
#define Fld(wid,shft,ac)    (((UINT32)wid<<16)|(shft<<8)|ac)
#define Fld_wid(fld)    (UINT8)((fld)>>16)
#define Fld_shft(fld)   (UINT8)((fld)>>8)
#define Fld_ac(fld)     (UINT8)(fld)

/* access method*/
#define AC_FULLB0       1
#define AC_FULLB1       2
#define AC_FULLB2       3
#define AC_FULLB3       4
#define AC_FULLW10      5
#define AC_FULLW21      6
#define AC_FULLW32      7
#define AC_FULLDW       8
#define AC_MSKB0        11
#define AC_MSKB1        12
#define AC_MSKB2        13
#define AC_MSKB3        14
#define AC_MSKW10       15
#define AC_MSKW21       16
#define AC_MSKW32       17
#define AC_MSKDW        18

/* --------FLD help macros, mask32 to mask8,mask16,maskalign ----------*/
/* mask32 -> mask8 */
#define MSKB0(msk)  (UINT8)(msk)
#define MSKB1(msk)  (UINT8)((msk)>>8)
#define MSKB2(msk)  (UINT8)((msk)>>16)
#define MSKB3(msk)  (UINT8)((msk)>>24)
/* mask32 -> mask16 */
#define MSKW0(msk)  (UINT16)(msk)
#define MSKW1(msk)  (UINT16)((msk)>>8)
#define MSKW2(msk)  (UINT16)((msk)>>16)
                        /* mask32 -> maskalign */
#define MSKAlignB(msk)  (((msk)&0xff)?(msk):(\
            ((msk)&0xff00)?((msk)>>8):(\
            ((msk)&0xff0000)?((msk)>>16):((msk)>>24)\
        )\
    ))

/* --------FLD help macros, mask32 to mask8,mask16,maskalign ----------*/
#define Fld2Msk32(fld)  /*lint -save -e504 */ (((UINT32)0xffffffff>>(32-Fld_wid(fld)))<<Fld_shft(fld)) /*lint -restore */
#define Fld2MskB0(fld)  MSKB0(Fld2Msk32(fld))
#define Fld2MskB1(fld)  MSKB1(Fld2Msk32(fld))
#define Fld2MskB2(fld)  MSKB2(Fld2Msk32(fld))
#define Fld2MskB3(fld)  MSKB3(Fld2Msk32(fld))
#define Fld2MskBX(fld,byte) ((UINT8)(Fld2Msk32(fld)>>((byte&3)*8)))

#define Fld2MskW0(fld)  MSKW0(Fld2Msk32(fld))
#define Fld2MskW1(fld)  MSKW1(Fld2Msk32(fld))
#define Fld2MskW2(fld)  MSKW2(Fld2Msk32(fld))
#define Fld2MskWX(fld,byte) ((UINT16)(Fld2Msk32(fld)>>((byte&3)*8)))

#define Fld2MskAlignB(fld)  MSKAlignB(Fld2Msk32(fld))
#define FldshftAlign(fld)   ((Fld_shft(fld)<8)?Fld_shft(fld):(\
            (Fld_shft(fld)<16)?(Fld_shft(fld)-8):(\
            (Fld_shft(fld)<24)?(Fld_shft(fld)-16):(Fld_shft(fld)-24)\
        )\
    ))
#define ValAlign2Fld(val,fld)   ((val)<<FldshftAlign(fld))

#define u1IO32Read1B(reg32) (*(volatile UINT8 *)(reg32))
extern UINT16 u2IO32Read2B(UINT32 reg32);
#define u4IO32Read4B(reg32) (*(volatile UINT32 *)(reg32))

#define vIO32Write1B(reg32, val8) vIO32Write1BMsk(reg32,val8,0xff)
#define vIO32Write2B(reg32, val16) vIO32Write2BMsk(reg32,val16,0xffff)
#define vIO32Write4B(reg32,val32) (*(volatile UINT32 *)(reg32)=(val32))

static inline void vIO32Write1BMsk(UINT32 reg32, UINT32 val8, UINT8 msk8)
{
    unsigned long flags;
    UINT32 u4Val, u4Msk;
    UINT8 bByte;

    bByte = reg32&3;
    reg32 &= ~3;
    val8 &= msk8;
    u4Msk = ~(UINT32)(msk8<<((UINT32)bByte<<3));

    local_irq_save(flags);
    u4Val = (*(volatile UINT32 *)(reg32));
    u4Val = ((u4Val & u4Msk) | ((UINT32)val8 << (bByte<<3)));
    (*(volatile UINT32 *)(reg32)=(u4Val));
    local_irq_restore(flags);

}
extern void vIO32Write2BMsk(UINT32 reg32, UINT32 val16, UINT16 msk16);
extern void vIO32Write4BMsk(UINT32 reg32, UINT32 val32, UINT32 msk32);


#define IO32ReadFldAlign(reg32,fld) /*lint -save -e506 -e504 -e514 -e62 -e737 -e572 -e961 -e648 -e701 -e732 -e571 */ \
    (((Fld_ac(fld)>=AC_FULLB0)&&(Fld_ac(fld)<=AC_FULLB3))?u1IO32Read1B((reg32)+(Fld_ac(fld)-AC_FULLB0)):( \
    ((Fld_ac(fld)>=AC_FULLW10)&&(Fld_ac(fld)<=AC_FULLW32))?u2IO32Read2B((reg32)+(Fld_ac(fld)-AC_FULLW10)):( \
    (Fld_ac(fld)==AC_FULLDW)? u4IO32Read4B(reg32):( \
    ((Fld_ac(fld)>=AC_MSKB0)&&(Fld_ac(fld)<=AC_MSKB3))?((u1IO32Read1B((reg32)+(Fld_ac(fld)-AC_MSKB0))&Fld2MskBX(fld,(Fld_ac(fld)-AC_MSKB0)))>>((Fld_shft(fld)-8*(Fld_ac(fld)-AC_MSKB0))&7)):( \
    ((Fld_ac(fld)>=AC_MSKW10)&&(Fld_ac(fld)<=AC_MSKW32))?((u2IO32Read2B((reg32)+(Fld_ac(fld)-AC_MSKW10))&Fld2MskWX(fld,(Fld_ac(fld)-AC_MSKW10)))>>((Fld_shft(fld)-8*(Fld_ac(fld)-AC_MSKW10))&15)):( \
    (Fld_ac(fld)==AC_MSKDW)?((u4IO32Read4B(reg32)&Fld2Msk32(fld))>>Fld_shft(fld)):0 \
  ))))))  /*lint -restore */

#define vIO32WriteFldAlign(reg32,val,fld) /*lint -save -e506 -e504 -e514 -e62 -e737 -e572 -e961 -e648 -e701 -e732 -e571 */ \
    (((Fld_ac(fld)>=AC_FULLB0)&&(Fld_ac(fld)<=AC_FULLB3))?vIO32Write1B((reg32)+(Fld_ac(fld)-AC_FULLB0),(val)),0:( \
    ((Fld_ac(fld)>=AC_FULLW10)&&(Fld_ac(fld)<=AC_FULLW32))?vIO32Write2B((reg32)+(Fld_ac(fld)-AC_FULLW10),(val)),0:( \
    (Fld_ac(fld)==AC_FULLDW)?vIO32Write4B((reg32),(val)),0:( \
    ((Fld_ac(fld)>=AC_MSKB0)&&(Fld_ac(fld)<=AC_MSKB3))?vIO32Write1BMsk((reg32)+(Fld_ac(fld)-AC_MSKB0),ValAlign2Fld((val),fld),Fld2MskBX(fld,(Fld_ac(fld)-AC_MSKB0))),0:( \
    ((Fld_ac(fld)>=AC_MSKW10)&&(Fld_ac(fld)<=AC_MSKW32))?vIO32Write2BMsk((reg32)+(Fld_ac(fld)-AC_MSKW10),ValAlign2Fld((val),fld),Fld2MskWX(fld,(Fld_ac(fld)-AC_MSKW10))),0:( \
    (Fld_ac(fld)==AC_MSKDW)?vIO32Write4BMsk((reg32),((UINT32)(val)<<Fld_shft(fld)),Fld2Msk32(fld)),0:0\
    )))))) /*lint -restore */

extern void __iomem *pBimIoMap;
extern void __iomem *pPdwncIoMap;
extern void __iomem *pCkgenIoMap;
extern void __iomem *pDramIoMap;
extern void __iomem *pMcuIoMap;
#ifdef CONFIG_HAVE_ARM_SCU
extern void __iomem *pScuIoMap;
#endif

#define HAL_READ32(_reg_)           (*((volatile unsigned long*)(_reg_)))
#define HAL_WRITE32(_reg_, val)     (*((volatile unsigned long*)(_reg_))) = val
#define IO_READ32(base, offset)     HAL_READ32((base) + (offset))
#define IO_WRITE32(base, offset, val)    HAL_WRITE32((base) + (offset), val)
#define CKGEN_READ32(offset)        IO_READ32(pCkgenIoMap, (offset))
#define MCU_READ32(off)             IO_READ32(pMcuIoMap, off)
#define MCU_WRITE32(off, val)       IO_WRITE32(pMcuIoMap, (off), (val))

static inline u32 __bim_readl(u32 regaddr32)
{
    return __raw_readl(pBimIoMap + regaddr32);
}

static inline void __bim_writel(u32 regval32, u32 regaddr32)
{
    __raw_writel(regval32, pBimIoMap + regaddr32);
}

static inline u32 __pdwnc_readl(u32 regaddr32)
{
        return __raw_readl(pPdwncIoMap + regaddr32);
}

static inline void __pdwnc_writel(u32 regval32, u32 regaddr32)
{
        __raw_writel(regval32, pPdwncIoMap + regaddr32);
}

#ifdef CONFIG_HAVE_ARM_SCU
static inline void __iomem *scu_base_addr(void)
{
	return (void __iomem *)pScuIoMap;
}
#endif

#endif /* __X_HAL_IO_H__ */
