/** 
 * @file star_regUtil.h
 * @brief The header file to define register utility functions including accessing register
 * @author mtk02196
 */
 
#ifndef _STAR_REGUTIL_H_
#define _STAR_REGUTIL_H_

#include <asm/io.h>

/* debug level */
enum
{
    STAR_ERR = 0,
    STAR_WARN,
    STAR_DBG,
    STAR_VERB,
    STAR_DBG_MAX
};

#ifndef STAR_DBG_LVL_DEFAULT
#define STAR_DBG_LVL_DEFAULT    STAR_WARN
#endif
extern int star_dbg_lvl;

#define STAR_MSG(lvl, fmt...)   do {\
                                    if (lvl <= star_dbg_lvl)\
                                        printk("star: " fmt);\
                                } while(0)

/**
 * @brief set 32bit register
 * @param reg register
 * @param value value to be set
 */
static void __inline__ StarSetReg(void __iomem *reg, u32 value)
{
	STAR_MSG(STAR_VERB, "StarSetReg(%p)=%08x\n", reg, value);
	//writel(value, reg);
	iowrite32(value, reg);
}

/**
 * @brief wait hardware ready
 * @param reg register
 * @param checkValue
 */
static void __inline__ StarWaitReady(void __iomem *reg, u32 checkValue, u32 startBit, u32 mask, u32 sleepTime, u32 maxSleeps)
{
	//writel(value, reg);
	STAR_MSG(STAR_VERB, "StarWaitReady(%p)\n", reg);
    while(((ioread32(reg)&(mask<<startBit)) != checkValue)&&(maxSleeps--))
    {
        msleep(sleepTime);
    }
}

/**
 * @brief get 32bit register value
 * @param reg register
 * @return the register's value
 */
static u32 __inline__ StarGetReg(void __iomem *reg)
{
    //u32 data = readl(reg);
    u32 data = ioread32(reg);
    STAR_MSG(STAR_VERB, "StarGetReg(%p)=%08x\n", reg, data);
    return data;
}

/**
 * @brief set bit value to register
 * @param reg register
 * @param bin bit to be set
 */
static void __inline__ StarSetBit(void __iomem *reg, u32 bit)
{
    //u32 data = readl(reg);
    u32 data =ioread32(reg);
    data |= bit;
    STAR_MSG(STAR_VERB, "StarSetBit(%p,bit:%08x)=%08x\n", reg, bit, data);
    //writel(data, reg);
	iowrite32(data, reg);
}

/**
 * @brief clear bit value of register
 * @param reg register
 * @param bin bit to be cleared
 */
static void __inline__ StarClearBit(void __iomem *reg, u32 bit)
{
    //u32 data = readl(reg);
    u32 data =ioread32(reg);
    data &= ~bit;
    STAR_MSG(STAR_VERB, "StarClearBit(%p,bit:%08x)=%08x\n", reg, bit, data);
    //writel(data, reg);
    iowrite32(data, reg);
}

/**
 * @brief set bitMask with value
 * @param reg register
 * @param mask bit mask to be set(overwrite)
 * @param offset offset of bitMask
 * @param value value to be set
 */
static void __inline__ StarSetBitMask(void __iomem *reg, u32 mask, u32 offset, u32 value)
{
    //u32 data = readl(reg);
    u32 data =ioread32(reg);
    data = ((data & ~(mask<<offset)) | ((value<<offset) & (mask<<offset)));
    STAR_MSG(STAR_VERB, "StarSetBitMask(%p,mask:%08x,offset:%08x)=%08x(value)\n", reg, mask, offset, value);
    //writel(data, reg);
    iowrite32(data, reg);
}

/**
 * @brief get value of biMask
 * @param reg register
 * @param mask bit mask to be get
 * @param offset offset of bitMask
 * @return the bitMask value
 */
static u32 __inline__ StarGetBitMask(void __iomem *reg, u32 mask, u32 offset)
{
    //u32 data = readl(reg);
    u32 data =ioread32(reg);
    data = ((data>>offset) & mask);
    STAR_MSG(STAR_VERB, "StarGetBitMask(%p,mask:%08x,offset:%08x)=%08x(data)\n", reg, mask, offset, data);
    return data;
}

/**
 * @brief get 32bit register value
 * @param reg register
 * @param bit bit to be checked if set or not
 * @return 1: set, 0: not set
 */
static u32 __inline__ StarIsSetBit(void __iomem *reg, u32 bit)
{
    //u32 data = readl(reg);
    u32 data =ioread32(reg);
    data &= bit;
    STAR_MSG(STAR_VERB, "StarIsSetBit(%p,bit:%08x)=%08x\n", reg, bit, data);
    return (data?1:0);
}

#endif /* _STAR_REGUTIL_H_ */
