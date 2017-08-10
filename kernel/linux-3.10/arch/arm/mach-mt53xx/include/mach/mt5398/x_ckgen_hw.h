/*
 * Licensed under the GPL
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html#TOC1
 *
 * $Author: dtvbm11 $
 * $Date: 2015/12/22 $
 * $RCSfile:  $
 * $Revision: #1 $
 *
 * 5398 CKGEN defines
 *
 */

#ifndef __X_5398_CKGEN_HW_H__
#define __X_5398_CKGEN_HW_H__

//=====================================================================
// Type definitions

typedef enum
{
    CAL_SRC_SAWLESSPLL = 0,             // 0
    CAL_SRC_CPUPLL_D2,                  // 1
    CAL_SRC_SYSPLL_D2,                  // 2
    CAL_SRC_APLL1,                      // 3
    CAL_SRC_APLL_D3,                    // 4
    CAL_SRC_APLL2,                      // 5
    CAL_SRC_APLL2_D3,                   // 6
    CAL_SRC_ADCPLL,                     // 7
    CAL_SRC_M_CK,                       // 8
    CAL_SRC_G3DPLL,                     // 9        VDECPLL
    CAL_SRC_FBOUT,                      // 0x0A
    CAL_SRC_USBPLL,                     // 0x0B
    CAL_SRC_CPUPLL_MONCLK,              // 0x0C
    CAL_SRC_ETHPLL_CLK675M,             // 0x0D
    CAL_SRC_XTAL,                       // 0x0E
    CAL_SRC_ADC_CKTOUT,                 // 0x0F
    CAL_SRC_CVBSADC_CKIND,              // 0x10
    CAL_SRC_HDMI_PLL340M,               // 0x11
    CAL_SRC_HDMI_PIX340M,               // 0x12
    CAL_SRC_HDMI_DEEP170M,              // 0x13
    CAL_SRC_DDDS1_VSP,                  // 0x14
    CAL_SRC_DDDS2_VSP,                  // 0x15
    CAL_SRC_VPLL_TCON_CK,               // 0x16
    CAL_SRC_LVDS_DPIX_CK,               // 0x17
    CAL_SRC_PIXPLL,                     // 0x18

    CAL_SRC_CLKDIG_CTS_D11,             // 0x19
    CAL_SRC_TCLK,                       // 0x1A
    CAL_SRC_OCLK,                       // 0x1B
    CAL_SRC_LVDS_CLK_CTS,               // 0x1C
    CAL_SRC_CVBSADC_CKOUT_2,            // 0x1D

    CAL_SRC_CVBSADC_CKOUT = 0x1F,       // 0x1F
    CAL_SRC_DA_SRV_SPL,                 // 0x20
    CAL_SRC_CKOUT_DEMOD,                // 0x21
    CAL_SRC_HDMI_0_DEEP340M_CK,         // 0x22
    CAL_SRC_HDMI_0_PIX340M_CK,          // 0x23
    CAL_SRC_HDMI_0_PLL340M_CK,          // 0x24
    CAL_SRC_HDMI_1_DEEP340M_CK,         // 0x25
    CAL_SRC_HDMI_1_PIX340M_CK,          // 0x26
    CAL_SRC_HDMI_1_PLL340M_CK,          // 0x27
    CAL_SRC_HDMI_2_DEEP340M_CK,         // 0x28
    CAL_SRC_HDMI_2_PIX340M_CK,          // 0x29
    CAL_SRC_HDMI_2_PLL340M_CK,          // 0x2A
    CAL_SRC_HDMI_3_DEEP340M_CK,         // 0x2B
    CAL_SRC_HDMI_3_PIX340M_CK,          // 0x2C
    CAL_SRC_HDMI_3_PLL340M_CK,          // 0x2D
    CAL_SRC_LVDSB_EVEN_CLK_DIV_125M_D11,// 0x2E
    CAL_SRC_LVDSB_ODD_CLK_DIV_125M_D11, // 0x2F
    CAL_SRC_LVDSA_EVEN_CLK_DIV_125M_D11,// 0x30
    CAL_SRC_LVDSA_ODD_CLK_DIV_125M_D11, // 0x31
    CAL_SRC_CLKDIG_D11,                 // 0x32
    CAL_SRC_VOPLL_TEST_CK,              // 0x33
    CAL_SRC_SYNC_R_CK,                  // 0x34
    CAL_SRC_SYNC_L_CK,                  // 0x35
    CAL_SRC_CVBSADC_CKIND_2,            // 0x36

    CAL_SRC_HADDS3_CK_98M = 58,         // 0x3A
    CAL_SRC_HADDS3_FBCLK_BUF,           // 0x3B
    CAL_SRC_PL_RCLK250,                 // 0x3C
    CAL_SRC_HADDS2_FBCLK_BUF,           // 0x3D
    CAL_SRC_PLLGP_TSTCK,                // 0x3E
    CAL_SRC_CPUPLL_FBCLK_BUF,           // 0x3F
    CAL_SRC_CPUPLL_REFCLK_BUF,          // 0x40
    CAL_SRC_CKT_B,                      // 0x41
    CAL_SRC_CKT_A,                      // 0x42
    CAL_SRC_USB20_MONCLK_3P,            // 0x43
    CAL_SRC_USB20_MONCLK_2P,            // 0x44
    CAL_SRC_USB20_MONCLK_1P,            // 0x45
    CAL_SRC_USB20_MONCLK,               // 0x46
    CAL_SRC_MEMPLL_MONCLK_PRE,          // 0x47
    CAL_SRC_MEMFB_D16_CK,               // 0x48
    CAL_SRC_MEMREF_D16_CK,              // 0x49
    CAL_SRC_GRA3D_ROSC_0,               // 0x4A
    CAL_SRC_HADDS3_CK_294M,             // 0x4B
    CAL_SRC_CA9_ROSC_CK_0 = 0x50,       // 0x50
    CAL_SRC_CA9_ROSC_CK_1,              // 0x51
    CAL_SRC_CA9_ROSC_CK_2,              // 0x52
    CAL_SRC_CA9_ROSC_CK_3,              // 0x53
    CAL_SRC_CA9_ROSC_CK_4,              // 0x54
    CAL_SRC_CA9_ROSC_CK_5,              // 0x55
    CAL_SRC_CA9_ROSC_CK_6,              // 0x56
    CAL_SRC_CA9_ROSC_CK_7,              // 0x57
    CAL_SRC_CA9_ROSC_CK_8,              // 0x58
    CAL_SRC_CA9_ROSC_CK_9,              // 0x59
    CAL_SRC_CA9_ROSC_CK_10,             // 0x5A
    CAL_SRC_CA9_ROSC_CK_11,             // 0x5B
    CAL_SRC_MON_TCLK_DIV,               // 0x5C
    CAL_SRC_MON_OCLK_DIV2,              // 0x5D
    CAL_SRC_MON_OCLK_DIV,               // 0x5E
    CAL_SRC_MON_CLK75M_CK,              // 0x5F
    CAL_SRC_DDDS2_CK,                   // 0x60
    CAL_SRC_CA9_CPM_OUT,                // 0x61
    CAL_SRC_MAIN2_H,                    // 0x62
    CAL_SRC_BLK2RS_HS,                  // 0x63
    CAL_SRC_MIB_OUT_M_HS,               // 0x64
    CAL_SRC_MIO_CK_P0_DIV4,             // 0x65
    CAL_SRC_MIO_CK_P0_DIV8,             // 0x66
    CAL_SRC_MEMPLL_D8 = 102,            // 0x66

    CAL_SRC_PCIE_REF_CK,                // 0x67
    CAL_SRC_DMSS_VSP,                   // 0x68
    CAL_SRC_HADDS3_REFCLK_216M,         // 0x69
    CAL_SRC_HADDS2_REFCLK_216M,         // 0x6A
    CAL_SRC_TAPLL_CK,                   // 0x6B
    CAL_SRC_TAPLL_FBCLK_BUF,            // 0x6C
    CAL_SRC_TAPLL_REFCLK_BUF,           // 0x6D
    CAL_SRC_G3DPLL_FBCLK_BUF,           // 0x6E
    CAL_SRC_G3DPLL_REFCLK_BUF,          // 0x6F

    CAL_SRC_ADC_CKOUT = 112,            // 0x70
    CAL_SRC_AD_SOGY_OUT_MON,            // 0x71
    CAL_SRC_AD_SOGY_OUT,                // 0x72
    CAL_SRC_AD_VSYNC_OUT,               // 0x73
    CAL_SRC_AD_HSYNC_OUT,               // 0x74
    CAL_SRC_AD_FB_OUT,                  // 0x75
    CAL_SRC_AD_SOGY_ADC_CKOUT,          // 0x76

    CAL_SRC_AD_GPIO_PWM_0 = 122,        // 0x7A
    CAL_SRC_AD_GPIO_PWM_1,              // 0x7B
    CAL_SRC_AD_GPIO_PWM_2,              // 0x7C
    CAL_SRC_AD_GPIO_PWM_3,              // 0x7D
    CAL_SRC_AD_GPIO_PWM_4,              // 0x7E
    CAL_SRC_AD_GPIO_PWM_5,              // 0x7F

    SRC_CPU_CLK = 128,                  // 0x80
    SRC_MEM_CLK,                        // 0x81
    SRC_BUS_CLK,                        // 0x82
    SRC_TIMER_CLK,                      // 0x83
    SRC_SYSPLL_CLK,                     // 0x84
    SRC_APLL1_CLK,                      // 0x85
    SRC_APLL2_CLK,                      // 0x86
    SRC_SAWLESSPLL_CLK,                 // 0x87
    SRC_ADCPLL_CLK,                     // 0x88
    SRC_ETHPLL_CLK,                     // 0x89
    SRC_ETHNETPLL_CLK,                  // 0x8A
    SRC_VDECPLL_CLK,                    // 0x8B
    SRC_XPLL_CLK,                       // 0x8C
    SRC_VOPLL_CLK,                      // 0x8D

    SRC_POCLK = 144,                    // 0x90
    SRC_VDOIN_MPCLK,                    // 0x91
    SRC_MIB_OCLK,                       // 0x92
    SRC_POCLK_DPLL,                     // 0x93

    SRC_MJC_CLK = 160,                  // 0xA0
    SRC_UNKNOWN
} CAL_SRC_T;



#define IO_CKGEN_BASE (IO_VIRT + 0xD000)

#define CKGEN_PLLCALIB (IO_CKGEN_BASE + 0x0C0)
    #define FLD_CALI_FAIL Fld(1,31,AC_MSKB3)//[31:31]
    #define FLD_SOFT_RST_CAL Fld(1,30,AC_MSKB3)//[30:30]
    #define FLD_CAL_SEL Fld(3,16,AC_MSKB2)//[18:16]
    #define FLD_CAL_TRI Fld(1,15,AC_MSKB1)//[15:15]
    #define FLD_DBGCKSEL Fld(7,8,AC_MSKB1)//[14:8]
    #define FLD_CAL_MODE Fld(2,0,AC_MSKB0)//[1:0]
#define CKGEN_PLLCALIBCNT (IO_CKGEN_BASE + 0x0CC)
    #define FLD_CALI_CNT Fld(32,0,AC_FULLDW)//[31:0]
#define CKGEN_CPU_CKCFG (IO_CKGEN_BASE + 0x208)
    #define FLD_BUS_CK_TST Fld(3,11,AC_MSKB1)//[13:11]
    #define FLD_BUS_CK_SEL Fld(3,8,AC_MSKB1)//[10:8]
    #define FLD_CPU_CK_TST Fld(3,4,AC_MSKB0)//[6:4]
    #define FLD_CPU_CK_SEL Fld(4,0,AC_MSKB0)//[3:0]

#endif /* __X_5398_CKGEN_HW_H__ */
