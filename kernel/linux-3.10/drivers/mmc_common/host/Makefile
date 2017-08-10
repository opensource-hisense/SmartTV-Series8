#
# Makefile for MMC/SD host controller drivers
#

# add mmc code path as head file seach path
ccflags-y += -I$(srctree)/drivers/mmc_common/
ifeq ($(CONFIG_MMC_MTK),y)
obj-$(CONFIG_MMC_MTK)    += mtk_msdc.o mtk_msdc_dbg.o
obj-$(CONFIG_MTK_USE_NEW_TUNE) += mtk_msdc_autok.o
obj-$(CONFIG_MTK_USE_LEGACY_TUNE) += mtk_msdc_ett_dat.o
else
obj-$(CONFIG_MTKEMMC_BOOT)    += mtk_msdc.o mtk_msdc_dbg.o
obj-$(CONFIG_KMSG_DUMP_EMMC)  += mtk_msdc_dump.o
obj-$(CONFIG_MTK_USE_NEW_TUNE) += mtk_msdc_autok.o
obj-$(CONFIG_MTK_USE_LEGACY_TUNE) += mtk_msdc_ett_dat.o
obj-$(CONFIG_MTKSDIO_BOOT)    += mtk_msdc.o
obj-$(CONFIG_MTKSDMMC_USE)    += mtk_msdc.o mtk_msdc_dbg.o
obj-$(CONFIG_MTKMSDC_PARTITION) += mtk_msdc_part.o
endif
obj-$(CONFIG_MMC_ARMMMCI)	+= mmci.o
obj-$(CONFIG_MMC_PXA)		+= pxamci.o
obj-$(CONFIG_MMC_IMX)		+= imxmmc.o
obj-$(CONFIG_MMC_MXC)		+= mxcmmc.o
obj-$(CONFIG_MMC_MXS)		+= mxs-mmc.o
obj-$(CONFIG_MMC_SDHCI)		+= sdhci.o
obj-$(CONFIG_MMC_SDHCI_PCI)	+= sdhci-pci.o
obj-$(subst m,y,$(CONFIG_MMC_SDHCI_PCI))	+= sdhci-pci-data.o
obj-$(CONFIG_MMC_SDHCI_PXAV3)	+= sdhci-pxav3.o
obj-$(CONFIG_MMC_SDHCI_PXAV2)	+= sdhci-pxav2.o
obj-$(CONFIG_MMC_SDHCI_S3C)	+= sdhci-s3c.o
obj-$(CONFIG_MMC_SDHCI_SPEAR)	+= sdhci-spear.o
obj-$(CONFIG_MMC_WBSD)		+= wbsd.o
obj-$(CONFIG_MMC_AU1X)		+= au1xmmc.o
obj-$(CONFIG_MMC_OMAP)		+= omap.o
obj-$(CONFIG_MMC_OMAP_HS)	+= omap_hsmmc.o
obj-$(CONFIG_MMC_AT91)		+= at91_mci.o
obj-$(CONFIG_MMC_ATMELMCI)	+= atmel-mci.o
obj-$(CONFIG_MMC_TIFM_SD)	+= tifm_sd.o
obj-$(CONFIG_MMC_MSM)		+= msm_sdcc.o
obj-$(CONFIG_MMC_MVSDIO)	+= mvsdio.o
obj-$(CONFIG_MMC_DAVINCI)       += davinci_mmc.o
obj-$(CONFIG_MMC_SPI)		+= mmc_spi.o
ifeq ($(CONFIG_OF),y)
obj-$(CONFIG_MMC_SPI)		+= of_mmc_spi.o
endif
obj-$(CONFIG_MMC_S3C)   	+= s3cmci.o
obj-$(CONFIG_MMC_SDRICOH_CS)	+= sdricoh_cs.o
obj-$(CONFIG_MMC_TMIO)		+= tmio_mmc.o
obj-$(CONFIG_MMC_TMIO_CORE)	+= tmio_mmc_core.o
tmio_mmc_core-y			:= tmio_mmc_pio.o
tmio_mmc_core-$(subst m,y,$(CONFIG_MMC_SDHI))	+= tmio_mmc_dma.o
obj-$(CONFIG_MMC_SDHI)		+= sh_mobile_sdhi.o
obj-$(CONFIG_MMC_CB710)		+= cb710-mmc.o
obj-$(CONFIG_MMC_VIA_SDMMC)	+= via-sdmmc.o
obj-$(CONFIG_SDH_BFIN)		+= bfin_sdh.o
obj-$(CONFIG_MMC_DW)		+= dw_mmc.o
obj-$(CONFIG_MMC_DW_PLTFM)	+= dw_mmc-pltfm.o
obj-$(CONFIG_MMC_DW_PCI)	+= dw_mmc-pci.o
obj-$(CONFIG_MMC_SH_MMCIF)	+= sh_mmcif.o
obj-$(CONFIG_MMC_JZ4740)	+= jz4740_mmc.o
obj-$(CONFIG_MMC_VUB300)	+= vub300.o
obj-$(CONFIG_MMC_USHC)		+= ushc.o

obj-$(CONFIG_MMC_SDHCI_PLTFM)		+= sdhci-pltfm.o
obj-$(CONFIG_MMC_SDHCI_CNS3XXX)		+= sdhci-cns3xxx.o
obj-$(CONFIG_MMC_SDHCI_ESDHC_IMX)	+= sdhci-esdhc-imx.o
obj-$(CONFIG_MMC_SDHCI_DOVE)		+= sdhci-dove.o
obj-$(CONFIG_MMC_SDHCI_TEGRA)		+= sdhci-tegra.o
obj-$(CONFIG_MMC_SDHCI_OF_ESDHC)	+= sdhci-of-esdhc.o
obj-$(CONFIG_MMC_SDHCI_OF_HLWD)		+= sdhci-of-hlwd.o

ifeq ($(CONFIG_CB710_DEBUG),y)
	CFLAGS-cb710-mmc	+= -DDEBUG
endif
