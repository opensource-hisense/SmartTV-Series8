
#include "mu3d_hal_osal.h"
#define _MTK_QMU_DRV_EXT_
#include "mu3d_hal_qmu_drv.h"
#undef _MTK_QMU_DRV_EXT_
#include "mu3d_hal_usb_drv.h"
#include "mu3d_hal_hw.h"
//#include "../../board/mt5890/drv_lib/inc/x_hal_arm.h"



/**
 * get_bd - get a null bd
 * @args - arg1: dir, arg2: ep number
 */
PBD get_bd(USB_DIR dir,DEV_UINT32 num){

	PBD ptr;

	if(dir==USB_RX){
		ptr = (PBD)Rx_bd_List[num].pNext;
		os_printk(K_DEBUG,"(Rx_bd_List[%d].pNext : 0x%08X\n", num,(DEV_UINT32)(Rx_bd_List[num].pNext));		

		if((Rx_bd_List[num].pNext+1)<Rx_bd_List[num].pEnd)			
			Rx_bd_List[num].pNext++;		
		else			
			Rx_bd_List[num].pNext=Rx_bd_List[num].pStart;

	}
	else{		
	
		ptr = (PBD)Tx_bd_List[num].pNext;
		os_printk(K_DEBUG,"(Tx_gpd_List[%d].pNext : 0x%08X\n", num,(DEV_UINT32)(Tx_bd_List[num].pNext));
		Tx_bd_List[num].pNext++;
		Tx_bd_List[num].pNext=Tx_bd_List[num].pNext+AT_BD_EXT_LEN;

		if(Tx_bd_List[num].pNext>=Tx_bd_List[num].pEnd){
			
			Tx_bd_List[num].pNext=Tx_bd_List[num].pStart;
		}
	}
	return ptr;
	
}

/**
 * get_bd - get a null gpd
 * @args - arg1: dir, arg2: ep number
 */
PGPD get_gpd(USB_DIR dir,DEV_UINT32 num){

	PGPD ptr;
	
	if(dir==USB_RX){
		ptr = Rx_gpd_List[num].pNext;		
		os_printk(K_DEBUG,"(Rx_gpd_List[%d].pNext : 0x%08X\n", num,(DEV_UINT32)(Rx_gpd_List[num].pNext));		
		if((Rx_gpd_List[num].pNext+1)<Rx_gpd_List[num].pEnd){			
			Rx_gpd_List[num].pNext++;
		}	
		else{			
			Rx_gpd_List[num].pNext=Rx_gpd_List[num].pStart;
		}	
	}
	else{		
	
		ptr = Tx_gpd_List[num].pNext;
		os_printk(K_DEBUG,"(Tx_gpd_List[%d].pNext : 0x%08X\n", num,(DEV_UINT32)(Tx_gpd_List[num].pNext));
		Tx_gpd_List[num].pNext++;
		Tx_gpd_List[num].pNext=Tx_gpd_List[num].pNext+AT_GPD_EXT_LEN;;

		if(Tx_gpd_List[num].pNext>=Tx_gpd_List[num].pEnd){
			
			Tx_gpd_List[num].pNext=Tx_gpd_List[num].pStart;
		}
	}
	return ptr;
}

/**
 * get_bd - align gpd ptr to target ptr
 * @args - arg1: dir, arg2: ep number, arg3: target ptr
 */
void gpd_ptr_align(USB_DIR dir,DEV_UINT32 num,PGPD ptr){
 	DEV_UINT32 run_next;
	run_next =1;//true;
	while(run_next)
	{
	 	if(ptr==get_gpd(dir,num)){
			run_next=0;//false;
	 	}
	}

}

/**
 * bd_virt_to_phys - map bd virtual address to physical address
 * @args - arg1: virtual address, arg2: dir, arg3: ep number
 * @return - physical address
 */
void *bd_virt_to_phys(void *vaddr,USB_DIR dir,DEV_UINT32 num){
	
	DEV_UINT32 *ptr;

	if(dir==USB_RX){
		ptr=(DEV_UINT32*)((DEV_UINT32)vaddr-Rx_bd_Offset[num]);
	}
	else{		
		ptr=(DEV_UINT32*)((DEV_UINT32)vaddr-Tx_bd_Offset[num]);
	}

	return ptr;
}

/**
 * bd_phys_to_virt - map bd physical address to virtual address
 * @args - arg1: physical address, arg2: dir, arg3: ep number
 * @return - virtual address
 */
void *bd_phys_to_virt(void *paddr,USB_DIR dir,DEV_UINT32 num){
	
	DEV_UINT32 * ptr;

	os_printk(K_DEBUG,"paddr : 0x%08X\n", (unsigned int)paddr);
	os_printk(K_DEBUG,"num : %d\n", num);

	if(dir==USB_RX){
		
		os_printk(K_DEBUG,"Rx_bd_Offset[%d] : 0x%08X\n",num ,Rx_bd_Offset[num]);
		ptr=(DEV_UINT32*)((DEV_UINT32)paddr+Rx_bd_Offset[num]);
	}
	else{		
		os_printk(K_DEBUG,"Tx_bd_Offset[%d] : 0x%08X\n",num ,Tx_bd_Offset[num]);
		ptr=(DEV_UINT32*)((DEV_UINT32)paddr+Tx_bd_Offset[num]);
	}
	os_printk(K_DEBUG,"ptr : 0x%08X\n", (unsigned int)ptr);

	return ptr;
}

/**
 * mu3d_hal_gpd_virt_to_phys - map gpd virtual address to physical address
 * @args - arg1: virtual address, arg2: dir, arg3: ep number
 * @return - physical address
 */
void *mu3d_hal_gpd_virt_to_phys(void *vaddr,USB_DIR dir,DEV_UINT32 num){
	
	DEV_UINT32 *ptr;

	if(dir==USB_RX){
		ptr=(DEV_UINT32*)((DEV_UINT32)vaddr-Rx_gpd_Offset[num]);
	}
	else{		
		ptr=(DEV_UINT32*)((DEV_UINT32)vaddr-Tx_gpd_Offset[num]);
	}
	
	return ptr;
}

/**
 * gpd_phys_to_virt - map gpd physical address to virtual address
 * @args - arg1: physical address, arg2: dir, arg3: ep number
 * @return - virtual address
 */
void *gpd_phys_to_virt(void *paddr,USB_DIR dir,DEV_UINT32 num){
	
	DEV_UINT32 * ptr;

	os_printk(K_DEBUG,"paddr : 0x%08X\n", (unsigned int)paddr);
	os_printk(K_DEBUG,"num : %d\n", num);
	
	if(dir==USB_RX){
		os_printk(K_DEBUG,"Rx_gpd_Offset[%d] : 0x%08X\n",num ,Rx_gpd_Offset[num]);
		ptr=(DEV_UINT32*)((DEV_UINT32)paddr+Rx_gpd_Offset[num]);
	}
	else{			
		os_printk(K_DEBUG,"Tx_gpd_Offset[%d] : 0x%08X\n",num ,Tx_gpd_Offset[num]);
		ptr=(DEV_UINT32*)((DEV_UINT32)paddr+Tx_gpd_Offset[num]);
	}
	os_printk(K_DEBUG,"ptr : 0x%08X\n", (unsigned int)ptr);
	return ptr;
}

/**
 * init_bd_list - initialize bd management list
 * @args - arg1: dir, arg2: ep number, arg3: bd virtual addr, arg4: bd ioremap addr, arg5: bd number
 */
void init_bd_list(USB_DIR dir,int num,PBD ptr,PBD io_ptr,DEV_UINT32 size){


	if(dir==USB_RX){
	 	
		Rx_bd_List[num].pStart=(PGPD)ptr;		
		Rx_bd_List[num].pEnd=(PGPD)(ptr+size);		
		Rx_bd_Offset[num]=(DEV_UINT32)ptr-(DEV_UINT32)io_ptr;		
		ptr++;		
		Rx_bd_List[num].pNext=(PGPD)ptr;

		os_printk(K_DEBUG,"Rx_bd_List[%d].pStart : 0x%08X\n", num,(unsigned int)Rx_bd_List[num].pStart);
		os_printk(K_DEBUG,"Rx_bd_List[%d].pNext  : 0x%08X\n", num,(unsigned int)Rx_bd_List[num].pNext);
		os_printk(K_DEBUG,"Rx_bd_List[%d].pEnd   : 0x%08X\n", num,(unsigned int)Rx_bd_List[num].pEnd);
		os_printk(K_DEBUG,"Rx_bd_Offset[%d]   : 0x%08X\n", num,Rx_bd_Offset[num]);
		os_printk(K_DEBUG,"phy : 0x%08X\n",(unsigned int)io_ptr);
		os_printk(K_ERR,"io_ptr : 0x%08X\n",(unsigned int)io_ptr);
		os_printk(K_ERR,"io_ptr end : 0x%08X\n",(unsigned int)(io_ptr+size));
	
	}
	else{		
		Tx_bd_List[num].pStart=(PGPD)ptr;
	 	Tx_bd_List[num].pEnd=(PGPD)((DEV_UINT8*)(ptr+size)+AT_BD_EXT_LEN*size);
		Tx_bd_Offset[num]=(DEV_UINT32)ptr-(DEV_UINT32)io_ptr;	
		ptr++;
	 	Tx_bd_List[num].pNext=(PGPD)((DEV_UINT8*)ptr+AT_BD_EXT_LEN);
		os_printk(K_DEBUG,"Tx_bd_List[%d].pStart : 0x%08X\n", num,(unsigned int)Tx_bd_List[num].pStart);
		os_printk(K_DEBUG,"Tx_bd_List[%d].pNext  : 0x%08X\n", num,(unsigned int)Tx_bd_List[num].pNext);
		os_printk(K_DEBUG,"Tx_bd_List[%d].pEnd   : 0x%08X\n", num,(unsigned int)Tx_bd_List[num].pEnd);
		os_printk(K_DEBUG,"Tx_bd_Offset[%d]   : 0x%08X\n", num,Tx_bd_Offset[num]);
		os_printk(K_DEBUG,"phy : 0x%08X\n",(unsigned int)io_ptr);
		os_printk(K_ERR,"io_ptr : 0x%08X\n",(unsigned int)io_ptr);
		os_printk(K_ERR,"io_ptr end : 0x%08X\n",(unsigned int)(io_ptr+size));
	}

}


/**
 * init_gpd_list - initialize gpd management list
 * @args - arg1: dir, arg2: ep number, arg3: gpd virtual addr, arg4: gpd ioremap addr, arg5: gpd number
 */
void init_gpd_list(USB_DIR dir,int num,PGPD ptr,PGPD io_ptr,DEV_UINT32 size){


	if(dir==USB_RX){
	 	
		Rx_gpd_List[num].pStart=ptr; 	
		Rx_gpd_List[num].pEnd=(PGPD)(ptr+size);		
		Rx_gpd_Offset[num]=(DEV_UINT32)ptr-(DEV_UINT32)(io_ptr); 	
		ptr++;		
		Rx_gpd_List[num].pNext=ptr;
		os_printk(K_DEBUG,"Rx_gpd_List[%d].pStart : 0x%08X\n", num,(unsigned int)Rx_gpd_List[num].pStart);
		os_printk(K_DEBUG,"Rx_gpd_List[%d].pNext  : 0x%08X\n", num,(unsigned int)Rx_gpd_List[num].pNext);
		os_printk(K_DEBUG,"Rx_gpd_List[%d].pEnd   : 0x%08X\n", num,(unsigned int)Rx_gpd_List[num].pEnd);
		os_printk(K_DEBUG,"Rx_gpd_Offset[%d]   : 0x%08X\n", num,Rx_gpd_Offset[num]);
		os_printk(K_DEBUG,"phy : 0x%08X\n",(unsigned int)io_ptr);
		os_printk(K_DEBUG,"phy end : 0x%08X\n",(unsigned int)io_ptr+size);
		os_printk(K_ERR,"io_ptr : 0x%08X\n",(unsigned int)io_ptr);
		os_printk(K_ERR,"io_ptr end : 0x%08X\n",(unsigned int)io_ptr+size);

	}
	else{		
		
		Tx_gpd_List[num].pStart=ptr;
	 	Tx_gpd_List[num].pEnd=(PGPD)((DEV_UINT8*)(ptr+size)+AT_GPD_EXT_LEN*size);
		Tx_gpd_Offset[num]=(DEV_UINT32)ptr-(DEV_UINT32)(io_ptr);	
		ptr++;
	 	Tx_gpd_List[num].pNext=(PGPD)((DEV_UINT8*)ptr+AT_GPD_EXT_LEN);
		os_printk(K_DEBUG,"Tx_gpd_List[%d].pStart : 0x%08X\n", num,(unsigned int)Tx_gpd_List[num].pStart);
		os_printk(K_DEBUG,"Tx_gpd_List[%d].pNext  : 0x%08X\n", num,(unsigned int)Tx_gpd_List[num].pNext);
		os_printk(K_DEBUG,"Tx_gpd_List[%d].pEnd   : 0x%08X\n", num,(unsigned int)Tx_gpd_List[num].pEnd);
		os_printk(K_DEBUG,"Tx_gpd_Offset[%d]   : 0x%08X\n", num,Tx_gpd_Offset[num]);
		os_printk(K_DEBUG,"phy : 0x%08X\n",(unsigned int)io_ptr);
		os_printk(K_DEBUG,"phy end : 0x%08X\n",(unsigned int)io_ptr+size);
		os_printk(K_ERR,"io_ptr : 0x%08X\n",(unsigned int)io_ptr);
		os_printk(K_ERR,"io_ptr end : 0x%08X\n",(unsigned int)io_ptr+size);
	}
}

/**
 * free_gpd - free gpd management list
 * @args - arg1: dir, arg2: ep number
 */
void free_gpd(USB_DIR dir,int num){

	if(dir==USB_RX){
		os_memset(Rx_gpd_List[num].pStart, 0 , MAX_GPD_NUM*sizeof(TGPD));
	}
	else{		
		os_memset(Tx_gpd_List[num].pStart, 0 , MAX_GPD_NUM*sizeof(TGPD));
	}
}

/**
 * mu3d_hal_alloc_qmu_mem - allocate gpd and bd memory for all ep
 * 
 */
void mu3d_hal_alloc_qmu_mem(void){
	DEV_UINT32 i,size;
//	DEV_UINT32 QCR = 0;
	TGPD *ptr,*io_ptr;
	TBD *bptr,*io_bptr;
//	dma_addr_t	dma;

	for(i=1; i<=MAX_QMU_EP; i++){

		size = sizeof(TGPD);
		size *=MAX_GPD_NUM;
//        ptr = dma_alloc_coherent(NULL, size, &dma,
//                GFP_KERNEL);
		ptr = (TGPD*)os_mem_alloc(size);
		os_memset(ptr, 0 , size);
		io_ptr = ptr;//(TGPD*)dma;//(TGPD*)os_ioremap(os_virt_to_phys(ptr),size);
		init_gpd_list(USB_RX,i,ptr,io_ptr,MAX_GPD_NUM);
		Rx_gpd_end[i]=ptr;
		os_printk(K_ERR,"ALLOC RX GPD End [%d] Virtual Mem@0x%08X\n", i, (DEV_UINT32)Rx_gpd_end[i]);
		os_memset(Rx_gpd_end[i], 0 , sizeof(TGPD));
		TGPD_CLR_FLAGS_HWO(Rx_gpd_end[i]);    
	    Rx_gpd_head[i]=Rx_gpd_last[i]=Rx_gpd_end[i];
		os_printk(K_ERR,"RQSAR[%d] : 0x%08X\n", i, (unsigned int)mu3d_hal_gpd_virt_to_phys(Rx_gpd_end[i],USB_RX,i));
		size = sizeof(TGPD);
		size += AT_GPD_EXT_LEN;
		size *=MAX_GPD_NUM;
//        ptr = dma_alloc_coherent(NULL, size, &dma,
//                GFP_KERNEL);
		ptr = (TGPD*)os_mem_alloc(size);
		os_memset(ptr, 0 , size);
		io_ptr = ptr;//(TGPD*)dma;//(TGPD*)os_ioremap(os_virt_to_phys(ptr),size);
		init_gpd_list(USB_TX,i,ptr,io_ptr,MAX_GPD_NUM);
	    Tx_gpd_end[i]= ptr;
	    os_printk(K_ERR,"ALLOC TX GPD End [%d] Virtual Mem@0x%08X\n", i, (DEV_UINT32)Tx_gpd_end[i]);
		os_memset(Tx_gpd_end[i], 0 , sizeof(TGPD)+AT_GPD_EXT_LEN);
	    TGPD_CLR_FLAGS_HWO(Tx_gpd_end[i]);  
	    Tx_gpd_head[i]=Tx_gpd_last[i]=Tx_gpd_end[i];
		os_printk(K_ERR,"TQSAR[%d] : 0x%08X\n", i, (unsigned int)mu3d_hal_gpd_virt_to_phys(Tx_gpd_end[i],USB_TX,i));
		size = (sizeof(TBD));
		size *= MAX_BD_NUM;
//        bptr = dma_alloc_coherent(NULL, size, &dma,
//                GFP_KERNEL);
		bptr = (TBD*)os_mem_alloc(size);
		os_memset(bptr, 0 , size);
		io_bptr = bptr;//(TBD*)dma;//(TBD*)os_ioremap(os_virt_to_phys(bptr),size);
		init_bd_list(USB_RX,i,bptr,io_bptr,MAX_BD_NUM);
		size = (sizeof(TBD));
		size += AT_BD_EXT_LEN;
		size *=MAX_BD_NUM;
//        bptr = dma_alloc_coherent(NULL, size, &dma,
//                GFP_KERNEL);
		bptr = (TBD*)os_mem_alloc(size);
		os_memset(bptr, 0 , size);
		io_bptr = bptr;//(TBD*)dma;//(TBD*)os_ioremap(os_virt_to_phys(bptr),size);
		init_bd_list(USB_TX,i,bptr,io_bptr,MAX_BD_NUM);
    }  
}

/**
 * mu3d_hal_init_qmu - initialize qmu
 * 
 */
void mu3d_hal_init_qmu(void){
	
	DEV_UINT32 i;
    DEV_UINT32 QCR = 0;
	/* Initialize QMU Tx/Rx start address. */
	for(i=1; i<=MAX_QMU_EP; i++){
		QCR|=QMU_RX_EN(i);
		QCR|=QMU_TX_EN(i);
		os_writel(USB_QMU_RQSAR(i), mu3d_hal_gpd_virt_to_phys(Rx_gpd_head[i],USB_RX,i));
		os_writel(USB_QMU_TQSAR(i), mu3d_hal_gpd_virt_to_phys(Tx_gpd_head[i],USB_TX,i));
		Tx_gpd_end[i] = Tx_gpd_last[i] = Tx_gpd_head[i];
		Rx_gpd_end[i] = Rx_gpd_last[i] = Rx_gpd_head[i];
		gpd_ptr_align(USB_TX,i,Tx_gpd_end[i]);
		gpd_ptr_align(USB_RX,i,Rx_gpd_end[i]);
	}  
	/* Enable QMU Tx/Rx. */
	os_writel(U3D_QGCSR, QCR);
	os_writel(U3D_QIESR0, QCR);
	/* Enable QMU interrupt. */
	os_writel(U3D_QIESR1, TXQ_EMPTY_IESR|TXQ_CSERR_IESR|TXQ_LENERR_IESR|RXQ_EMPTY_IESR|RXQ_CSERR_IESR|RXQ_LENERR_IESR|RXQ_ZLPERR_IESR);
	os_writel(U3D_EPIESR, EP0ISR);
}

/**
 * mu3d_hal_cal_checksum - calculate check sum
 * @args - arg1: data buffer, arg2: data length
 */
DEV_UINT8 mu3d_hal_cal_checksum(DEV_UINT8 *data, DEV_INT32 len){
	
 	DEV_UINT8 *uDataPtr, ckSum;
    DEV_INT32 i;
	
 	*(data + 1) = 0x0;
  	uDataPtr = data;
   	ckSum = 0;
   	for (i = 0; i < len; i++){
  		ckSum += *(uDataPtr + i);
   	}
  	return 0xFF - ckSum;
}

/**
 * mu3d_hal_resume_qmu - resume qmu function
 * @args - arg1: ep number, arg2: dir
 */
void mu3d_hal_resume_qmu(DEV_INT32 Q_num,  USB_DIR dir){

    #if 0//defined(USB_RISC_CACHE_ENABLED)  
    //os_flushinvalidateDcache(); 
	if(dir == USB_TX)
        dma_map_single(NULL, &(Tx_gpd_List[Q_num].pStart), sizeof(TGPD)*MAX_GPD_NUM, DMA_TO_DEVICE);
    else
        dma_map_single(NULL, &(Rx_gpd_List[Q_num].pStart), sizeof(TGPD)*MAX_GPD_NUM, DMA_TO_DEVICE);

    #endif

    if(dir == USB_TX){      
		
 		os_writel(USB_QMU_TQCSR(Q_num), QMU_Q_RESUME);	  
        if(!os_readl( USB_QMU_TQCSR(Q_num))){  //judge if Queue is still inactive
            os_writel( USB_QMU_TQCSR(Q_num), QMU_Q_RESUME);
        }
    }
    else if(dir == USB_RX){
			
       	os_writel(USB_QMU_RQCSR(Q_num), QMU_Q_RESUME);  
		if(!os_readl( USB_QMU_RQCSR(Q_num))){
            os_writel( USB_QMU_RQCSR(Q_num), QMU_Q_RESUME);
        }
    }
}

/**
 * mu3d_hal_prepare_tx_gpd - prepare tx gpd/bd 
 * @args - arg1: gpd address, arg2: data buffer address, arg3: data length, arg4: ep number, arg5: with bd or not, arg6: write hwo bit or not,  arg7: write ioc bit or not
 */
TGPD* mu3d_hal_prepare_tx_gpd(TGPD* gpd, DEV_UINT8*pBuf, DEV_UINT32 data_length, DEV_UINT8 ep_num, DEV_UINT8 _is_bdp, DEV_UINT8 isHWO,DEV_UINT8 ioc, DEV_UINT8 bps,DEV_UINT8 zlp){
	DEV_UINT32  offset;
	DEV_INT32 i,bd_num; 

    TBD * bd_next; 
	TBD * bd_head, *bd;  
	DEV_UINT32 length;
	DEV_UINT8 *pBuffer, *vBuffer;
    DEV_UINT8 *_tmp;//,*_tmp1;
   
  	os_printk(K_INFO,"mu3d_hal_prepare_tx_gpd\r\n");

	if(data_length<= bGPD_Extension){
		_is_bdp=0;
	}

    if(!_is_bdp){
		
        TGPD_SET_DATA(gpd, pBuf+bGPD_Extension);
        TGPD_CLR_FORMAT_BDP(gpd);
    }
    else{

		bd_head=(TBD*)get_bd(USB_TX,ep_num);
		os_printk(K_INFO,"Malloc Tx 01 (BD) : 0x%x\r\n", (DEV_UINT32)bd_head);
		bd=bd_head;
		os_memset(bd, 0, sizeof(TBD)+bBD_Extension);		
        length=data_length-bGPD_Extension;
        pBuffer= (DEV_UINT8*)(pBuf+bGPD_Extension);
		offset=bd_buf_size+bBD_Extension;
		bd_num = (!(length%offset)) ? (length/offset) : ((length/offset)+1);

		os_printk(K_INFO,"bd_num : 0x%x\r\n", (DEV_UINT32)bd_num);

		if(offset>length){
			offset=length;
		}

		for(i=0; i<bd_num; i++){
			
			os_printk(K_INFO,"bd[%d] :%x\r\n",i,(unsigned int)bd);
			if(i==(bd_num-1)){
				if(length<bBD_Extension){
					TBD_SET_EXT_LEN(bd, length);
					TBD_SET_BUF_LEN(bd, 0);
					TBD_SET_DATA(bd, pBuffer+bBD_Extension);
				}
				else{
					TBD_SET_EXT_LEN(bd, bBD_Extension);
					TBD_SET_BUF_LEN(bd, length-bBD_Extension);
					TBD_SET_DATA(bd, pBuffer+bBD_Extension);
				}

				TBD_SET_FLAGS_EOL(bd);
				TBD_SET_NEXT(bd, 0);
				TBD_SET_CHKSUM(bd, CHECKSUM_LENGTH);	
				if(bBD_Extension){
					
					vBuffer = os_phys_to_virt(pBuffer);
//					dma_sync_single_for_cpu(NULL,(dma_addr_t)pBuf,g_dma_buffer_size,0);//DMA_BIDIRECTIONAL);
					_tmp=TBD_GET_EXT(bd); 
					os_memcpy(_tmp, vBuffer, bBD_Extension);
//					dma_sync_single_for_device(NULL, (dma_addr_t)pBuf, g_dma_buffer_size,0);// DMA_BIDIRECTIONAL);
				}
				os_printk(K_INFO,"BD number %d\r\n", i+1);
				data_length=length+bGPD_Extension;
				length = 0;
				
				break;
			}else{		
			
				TBD_SET_EXT_LEN(bd, bBD_Extension);
				TBD_SET_BUF_LEN(bd, offset-bBD_Extension);
				TBD_SET_DATA(bd, pBuffer+bBD_Extension);
				TBD_CLR_FLAGS_EOL(bd);
                bd_next = (TBD*)get_bd(USB_TX,ep_num);
                os_memset(bd_next, 0, sizeof(TBD)+bBD_Extension);
				TBD_SET_NEXT(bd, bd_virt_to_phys(bd_next,USB_TX,ep_num));
				TBD_SET_CHKSUM(bd, CHECKSUM_LENGTH);	
				
				if(bBD_Extension){
					
					vBuffer = os_phys_to_virt(pBuffer);
//					dma_sync_single_for_cpu(NULL,(dma_addr_t)pBuf,g_dma_buffer_size,0);//DMA_BIDIRECTIONAL);
					_tmp=TBD_GET_EXT(bd); 
					os_memcpy(_tmp, vBuffer, bBD_Extension);
//					dma_sync_single_for_device(NULL, (dma_addr_t)pBuf, g_dma_buffer_size,0);// DMA_BIDIRECTIONAL);
				}
				length -= offset;
				pBuffer += offset;
				bd=bd_next;
			}
		}			  
		
		TGPD_SET_DATA(gpd, bd_virt_to_phys(bd_head,USB_TX,ep_num));
		TGPD_SET_FORMAT_BDP(gpd);
	}

	os_printk(K_INFO,"GPD data_length %d \r\n", (data_length-bGPD_Extension));

	if(data_length<bGPD_Extension){
		TGPD_SET_BUF_LEN(gpd, 0);	
		TGPD_SET_EXT_LEN(gpd, data_length);
	}
	else{
		TGPD_SET_BUF_LEN(gpd, data_length-bGPD_Extension);
		TGPD_SET_EXT_LEN(gpd, bGPD_Extension);
	}
		
	if(bGPD_Extension){
		vBuffer = os_phys_to_virt(pBuf);
//		dma_sync_single_for_cpu(NULL,(dma_addr_t)pBuf,g_dma_buffer_size,0);//DMA_BIDIRECTIONAL);
		_tmp=TGPD_GET_EXT(gpd);
		os_memcpy(_tmp, vBuffer, bGPD_Extension);
//		dma_sync_single_for_device(NULL, (dma_addr_t)pBuf, g_dma_buffer_size, 0);//DMA_BIDIRECTIONAL);
	}
	if(zlp){
		TGPD_SET_FORMAT_ZLP(gpd);
	}else{
	  	TGPD_CLR_FORMAT_ZLP(gpd);	 
	}
	if(bps){
		TGPD_SET_FORMAT_BPS(gpd); 
	}
	else{
	  	TGPD_CLR_FORMAT_BPS(gpd);	 
	}
	if(ioc){
		TGPD_SET_FORMAT_IOC(gpd);
    }
	else{
	  	TGPD_CLR_FORMAT_IOC(gpd);	 
	}

    //Create next GPD
    Tx_gpd_end[ep_num]=get_gpd(USB_TX ,ep_num);
	
    os_memset(Tx_gpd_end[ep_num], 0 , sizeof(TGPD)+bGPD_Extension);
	TGPD_CLR_FLAGS_HWO(Tx_gpd_end[ep_num]);    
	TGPD_SET_NEXT(gpd, mu3d_hal_gpd_virt_to_phys(Tx_gpd_end[ep_num],USB_TX,ep_num)); 

	if(isHWO){
		TGPD_SET_CHKSUM(gpd, CHECKSUM_LENGTH);
	
		TGPD_SET_FLAGS_HWO(gpd);	
			
	}else{
		TGPD_CLR_FLAGS_HWO(gpd);
        TGPD_SET_CHKSUM_HWO(gpd, CHECKSUM_LENGTH);
	}

	os_printk(K_INFO,"Malloc Tx 01 (GPD+EXT) (Tx_gpd_end) : 0x%x\r\n", (DEV_UINT32)Tx_gpd_end[ep_num]);
    #if defined(USB_RISC_CACHE_ENABLED)  
    os_flushinvalidateDcache();
    #endif

	return gpd;	
}

/**
 * mu3d_hal_prepare_rx_gpd - prepare rx gpd/bd 
 * @args - arg1: gpd address, arg2: data buffer address, arg3: data length, arg4: ep number, arg5: with bd or not, arg6: write hwo bit or not,  arg7: write ioc bit or not
 */
TGPD* mu3d_hal_prepare_rx_gpd(TGPD*gpd, DEV_UINT8*pBuf, DEV_UINT32 data_len, DEV_UINT8 ep_num, DEV_UINT8 _is_bdp, DEV_UINT8 isHWO, DEV_UINT8 ioc, DEV_UINT8 bps, DEV_UINT32 cMaxPacketSize){
	DEV_UINT32	offset;
	DEV_INT32 i,bd_num, length; 
		
	TBD * bd_next; 
	TBD * bd_head, *bd;  
	DEV_UINT8* pBuffer;

	os_printk(K_DEBUG,"GPD 0x%x\r\n", (unsigned int)gpd);
	length = data_len;
	
	if(!_is_bdp){

		TGPD_SET_DATA(gpd, pBuf);
		TGPD_CLR_FORMAT_BDP(gpd);
	}
	else{
			
		bd_head = (TBD*)get_bd(USB_RX,ep_num);
		bd = bd_head;
		os_memset(bd, 0, sizeof(TBD));
		offset=bd_buf_size;
		pBuffer= (DEV_UINT8*)(pBuf);
		bd_num = (!(length%offset)) ? (length/offset) : ((length/offset)+1);
	
		for(i=0; i<bd_num; i++){
				
			os_printk(K_INFO,"bd[%d] :%x\r\n",i,(unsigned int)bd);
			TBD_SET_BUF_LEN(bd, 0);
			TBD_SET_DATA(bd, pBuffer);
			if(i==(bd_num-1)){
				length = (!(length%cMaxPacketSize)) ? (length) : ((length/cMaxPacketSize)+1)*cMaxPacketSize;				
				TBD_SET_DataBUF_LEN(bd, length); //The last one's data buffer lengnth must be precise, or the GPD will never done unless ZLP or short packet.
				TBD_SET_FLAGS_EOL(bd);
				TBD_SET_NEXT(bd, 0);
				TBD_SET_CHKSUM(bd, CHECKSUM_LENGTH); 
				os_printk(K_DEBUG,"BD number %d\r\n", i+1);
				break;
			}else{	
			
				TBD_SET_DataBUF_LEN(bd, offset);
				TBD_CLR_FLAGS_EOL(bd);
				bd_next = (TBD*)get_bd(USB_RX,ep_num);
				os_memset(bd_next, 0, sizeof(TBD));
				TBD_SET_NEXT(bd, bd_virt_to_phys(bd_next,USB_RX,ep_num));
				TBD_SET_CHKSUM(bd, CHECKSUM_LENGTH); 
				pBuffer += offset;
				length -= offset;
				bd=bd_next;
			}
		}			  
		
		TGPD_SET_DATA(gpd, bd_virt_to_phys(bd_head,USB_RX,ep_num));
		TGPD_SET_FORMAT_BDP(gpd);
	}
	
	TGPD_SET_DataBUF_LEN(gpd, gpd_buf_size);
	TGPD_SET_BUF_LEN(gpd, 0);	
	
	if(bps){
		TGPD_SET_FORMAT_BPS(gpd); 
	}
	else{
		TGPD_CLR_FORMAT_BPS(gpd);	 
	}
	if(ioc){
		TGPD_SET_FORMAT_IOC(gpd);
	}
	else{
	  	TGPD_CLR_FORMAT_IOC(gpd);
	}

	Rx_gpd_end[ep_num]=get_gpd(USB_RX ,ep_num);
	os_memset(Rx_gpd_end[ep_num], 0 , sizeof(TGPD));
	TGPD_CLR_FLAGS_HWO(Rx_gpd_end[ep_num]);    
	TGPD_SET_NEXT(gpd, mu3d_hal_gpd_virt_to_phys(Rx_gpd_end[ep_num],USB_RX,ep_num)); 

	if(isHWO){
		TGPD_SET_CHKSUM(gpd, CHECKSUM_LENGTH);
		TGPD_SET_FLAGS_HWO(gpd);
		
	}else{
		TGPD_CLR_FLAGS_HWO(gpd);
		TGPD_SET_CHKSUM_HWO(gpd, CHECKSUM_LENGTH);
	}

	os_printk(K_INFO,"Rx Next GPD 0x%x, current BUF_LEN 0x%x\n",(DEV_UINT32)Rx_gpd_end[ep_num],TBD_GET_DataBUF_LEN(bd));
	os_printk(K_DEBUG,"Rx gpd info { HWO %d, Next_GPD %x ,DataBufferLength %d, DataBuffer %x, Recived Len %d, Endpoint %d, TGL %d, ZLP %d}\r\n", 
					(DEV_UINT32)TGPD_GET_FLAG(gpd), (DEV_UINT32)TGPD_GET_NEXT(gpd), 
					(DEV_UINT32)TGPD_GET_DataBUF_LEN(gpd), (DEV_UINT32)TGPD_GET_DATA(gpd), 
					(DEV_UINT32)TGPD_GET_BUF_LEN(gpd), (DEV_UINT32)TGPD_GET_EPaddr(gpd), 
					(DEV_UINT32)TGPD_GET_TGL(gpd), (DEV_UINT32)TGPD_GET_ZLP(gpd));
	
	return gpd;	
}

/**
 * mu3d_hal_insert_transfer_gpd - insert new gpd/bd 
 * @args - arg1: ep number, arg2: dir, arg3: data buffer, arg4: data length,  arg5: write hwo bit or not,  arg6: write ioc bit or not
 */

void mu3d_hal_insert_transfer_gpd(DEV_INT32 ep_num,USB_DIR dir, DEV_UINT8* buf, DEV_UINT32 count, DEV_UINT8 isHWO, DEV_UINT8 ioc, DEV_UINT8 bps,DEV_UINT8 zlp, DEV_UINT32 cMaxPacketSize ){

 	TGPD* gpd; 
	os_printk(K_INFO,"usb_insert_transfer_qmu\r\n");
	os_printk(K_INFO,"ep_num :%d\r\n", (int)ep_num);
	os_printk(K_INFO,"dir :%d\r\n",dir);
	os_printk(K_INFO,"buf :%x\r\n",(DEV_UINT32)buf);
	os_printk(K_INFO,"count :%x\r\n",count);

 	if(dir == USB_TX){  
		TGPD* gpd = Tx_gpd_end[ep_num];
		os_printk(K_INFO,"TX gpd :%x\r\n", (unsigned int)gpd);
		mu3d_hal_prepare_tx_gpd(gpd, buf, count, ep_num,is_bdp, isHWO,ioc, bps, zlp);
	}
 	else if(dir == USB_RX){		
		gpd = Rx_gpd_end[ep_num];
		os_printk(K_INFO,"RX gpd :%x\r\n",(unsigned int)gpd);
	 	mu3d_hal_prepare_rx_gpd(gpd, buf, count, ep_num,is_bdp, isHWO,ioc, bps,cMaxPacketSize);
	}
	
//    HalFlushDCacheMultipleLine(gpd,sizeof(TGPD)*30);
//    HalFlushDCache();
}

/**
 * mu3d_hal_start_qmu - start qmu function (QMU flow : mu3d_hal_init_qmu ->mu3d_hal_start_qmu -> mu3d_hal_insert_transfer_gpd -> mu3d_hal_resume_qmu)
 * @args - arg1: ep number, arg2: dir
 */
void mu3d_hal_start_qmu(DEV_INT32 Q_num,  USB_DIR dir){ 

    DEV_UINT32 QCR; 
    DEV_UINT32 Temp; 
    DEV_UINT32 txcsr; 

    if(dir == USB_TX){    

		txcsr = USB_ReadCsr32(U3D_TX1CSR0, Q_num) & 0xFFFEFFFF;
		USB_WriteCsr32(U3D_TX1CSR0, Q_num, txcsr | TX_DMAREQEN);	
		QCR = os_readl(U3D_QCR0); 
		os_writel(U3D_QCR0, QCR|QMU_TX_CS_EN(Q_num));
#if (TXZLP==HW_MODE)
		QCR = os_readl(U3D_QCR1);
	 	os_writel(U3D_QCR1, QCR &~ QMU_TX_ZLP(Q_num));
		QCR = os_readl(U3D_QCR2);
		os_writel(U3D_QCR2, QCR|QMU_TX_ZLP(Q_num));
#elif (TXZLP==GPD_MODE)
		QCR = os_readl(U3D_QCR1);
		os_writel(U3D_QCR1, QCR|QMU_TX_ZLP(Q_num));
#endif
		os_writel(U3D_QEMIESR, os_readl(U3D_QEMIESR) | QMU_TX_EMPTY(Q_num));		 
		os_writel(U3D_TQERRIESR0, QMU_TX_LEN_ERR(Q_num)|QMU_TX_CS_ERR(Q_num));
      	Temp = os_readl(USB_QMU_TQCSR(Q_num));
       	os_printk(K_DEBUG,"USB_QMU_TQCSR:0x%08X\n", Temp);  
		
 		if(os_readl(USB_QMU_TQCSR(Q_num))&QMU_Q_ACTIVE){
		  	os_printk(K_DEBUG,"Tx %d Active Now!\r\n", Q_num);
		  	return;
		}
       	#if defined(USB_RISC_CACHE_ENABLED)  
       	os_flushinvalidateDcache();
       	#endif
		os_writel(USB_QMU_TQCSR(Q_num), QMU_Q_START);	         
    }
    else if(dir == USB_RX){     
	
		USB_WriteCsr32(U3D_RX1CSR0, Q_num, USB_ReadCsr32(U3D_RX1CSR0, Q_num) |(RX_DMAREQEN));  
      	QCR = os_readl(U3D_QCR0);
		os_writel(U3D_QCR0, QCR|QMU_RX_CS_EN(Q_num));
		
		#ifdef CFG_RX_ZLP_EN
			QCR = os_readl(U3D_QCR3);
			os_writel(U3D_QCR3, QCR|QMU_RX_ZLP(Q_num));
		#else
			QCR = os_readl(U3D_QCR3);
			os_writel(U3D_QCR3, QCR&~(QMU_RX_ZLP(Q_num)));
		#endif
		
		#ifdef CFG_RX_COZ_EN		
			QCR = os_readl(U3D_QCR3);
			os_writel(U3D_QCR3, QCR|QMU_RX_COZ(Q_num));
		#else
			QCR = os_readl(U3D_QCR3);
			os_writel(U3D_QCR3, QCR&~(QMU_RX_COZ(Q_num)));
		#endif
	
		os_writel(U3D_QEMIESR, os_readl(U3D_QEMIESR) | QMU_RX_EMPTY(Q_num));		
		os_writel(U3D_RQERRIESR0, QMU_RX_LEN_ERR(Q_num)|QMU_RX_CS_ERR(Q_num));
		os_writel(U3D_RQERRIESR1, QMU_RX_EP_ERR(Q_num)|QMU_RX_ZLP_ERR(Q_num));

		if(os_readl(USB_QMU_RQCSR(Q_num))&QMU_Q_ACTIVE){
		  	os_printk(K_DEBUG,"Rx %d Active Now!\r\n", Q_num);
		  	return;
		}
		
     	#if defined(USB_RISC_CACHE_ENABLED)  
      	os_flushinvalidateDcache();
       	#endif
		os_writel(USB_QMU_RQCSR(Q_num), QMU_Q_START);		
    }
			
#if (CHECKSUM_TYPE==CS_16B)
	os_writel(U3D_QCR0, os_readl(U3D_QCR0)|CS16B_EN);
#else
	os_writel(U3D_QCR0, os_readl(U3D_QCR0)&~CS16B_EN);
#endif
				

}

/**
 * mu3d_hal_stop_qmu - stop qmu function (after qmu stop, fifo should be flushed)
 * @args - arg1: ep number, arg2: dir
 */
void mu3d_hal_stop_qmu(DEV_INT32 Q_num,  USB_DIR dir){
  
    if(dir == USB_TX){           	  
  		if(!(os_readl(USB_QMU_TQCSR(Q_num)) & (QMU_Q_ACTIVE))){  
 			os_printk(K_DEBUG,"Tx %d inActive Now!\r\n", Q_num);
		  	return;
		}
		os_writel(USB_QMU_TQCSR(Q_num), QMU_Q_STOP);
		while((os_readl(USB_QMU_TQCSR(Q_num)) & (QMU_Q_ACTIVE)));
		os_printk(K_CRIT,"Tx %d stop Now!\r\n", Q_num);
    }
    else if(dir == USB_RX){	  
		if(!(os_readl(USB_QMU_RQCSR(Q_num))&QMU_Q_ACTIVE)){
			os_printk(K_DEBUG,"Rx %d inActive Now!\r\n", Q_num);
		  	return;
		}
		os_writel(USB_QMU_RQCSR(Q_num), QMU_Q_STOP);
		while((os_readl(USB_QMU_RQCSR(Q_num)) & (QMU_Q_ACTIVE)));
		os_printk(K_DEBUG,"Rx %d stop Now!\r\n", Q_num);
    }
}

/**
 * mu3d_hal_send_stall - send stall
 * @args - arg1: ep number, arg2: dir
 */
void mu3d_hal_send_stall(DEV_INT32 Q_num,  USB_DIR dir){
	
	if(dir == USB_TX){		
		USB_WriteCsr32(U3D_TX1CSR0, Q_num, USB_ReadCsr32(U3D_TX1CSR0, Q_num) | TX_SENDSTALL);
		while(!(USB_ReadCsr32(U3D_TX1CSR0, Q_num) & TX_SENTSTALL));
        USB_WriteCsr32(U3D_TX1CSR0, Q_num, USB_ReadCsr32(U3D_TX1CSR0, Q_num) | TX_SENTSTALL);
		USB_WriteCsr32(U3D_TX1CSR0, Q_num, USB_ReadCsr32(U3D_TX1CSR0, Q_num) &~ TX_SENDSTALL);
		os_printk(K_CRIT,"EP[%d] SENTSTALL!\r\n",Q_num);
	}
    else if(dir == USB_RX){
		USB_WriteCsr32(U3D_RX1CSR0, Q_num, USB_ReadCsr32(U3D_RX1CSR0, Q_num) | RX_SENDSTALL);
		while(!(USB_ReadCsr32(U3D_RX1CSR0, Q_num) & RX_SENTSTALL));
        USB_WriteCsr32(U3D_RX1CSR0, Q_num, USB_ReadCsr32(U3D_RX1CSR0, Q_num) | RX_SENTSTALL);
		USB_WriteCsr32(U3D_RX1CSR0, Q_num, USB_ReadCsr32(U3D_RX1CSR0, Q_num) &~ RX_SENDSTALL);
		os_printk(K_CRIT,"EP[%d] SENTSTALL!\r\n",Q_num);
	}
}


/**
 * mu3d_hal_restart_qmu - clear toggle(or sequence) number and start qmu
 * @args - arg1: ep number, arg2: dir
 */
void mu3d_hal_restart_qmu(DEV_INT32 Q_num,  USB_DIR dir){
    DEV_UINT32 ep_rst; 

	if(dir == USB_TX){    
		ep_rst = BIT16<<Q_num;
		os_printk(K_CRIT,"ep_rst : [%x] \r\n",ep_rst);
		os_writel(U3D_EP_RST, ep_rst);
		os_printk(K_CRIT,"ep_rst \r\n");
		os_ms_delay(1);
		os_writel(U3D_EP_RST, 0);
		os_printk(K_CRIT,"clear ep_rst \r\n");

	}
	else{
		ep_rst = 1<<Q_num;
		os_writel(U3D_EP_RST, ep_rst);
		os_ms_delay(1);
		os_writel(U3D_EP_RST, 0);
	}
	mu3d_hal_start_qmu(Q_num, dir); 
}

/**
 * flush_qmu - stop qmu and align qmu start ptr t0 current ptr
 * @args - arg1: ep number, arg2: dir
 */
void mu3d_hal_flush_qmu(DEV_INT32 Q_num,  USB_DIR dir){

//    DEV_UINT16 wVal,i; 
    TGPD* gpd_current;

    struct USB_REQ *req = mu3d_hal_get_req(Q_num, dir); 
    
    if(dir == USB_TX){     
		
		mu3d_hal_stop_qmu(Q_num, USB_TX);
		os_printk(K_CRIT,"flush_qmu USB_TX\r\n");
		gpd_current = (TGPD*)(os_readl(USB_QMU_TQCPR(Q_num)));
		if(!gpd_current){
			gpd_current = (TGPD*)(os_readl(USB_QMU_TQSAR(Q_num)));
		}
		os_printk(K_CRIT,"gpd_current(P) %x\r\n", (unsigned int)gpd_current);
		gpd_current = gpd_phys_to_virt(gpd_current,USB_TX,Q_num);
		os_printk(K_CRIT,"gpd_current(V) %x\r\n", (unsigned int)gpd_current);
		Tx_gpd_end[Q_num] = Tx_gpd_last[Q_num] = gpd_current;
		gpd_ptr_align(dir,Q_num,Tx_gpd_end[Q_num]);
		free_gpd(dir,Q_num);
		os_writel(USB_QMU_TQSAR(Q_num), mu3d_hal_gpd_virt_to_phys(Tx_gpd_last[Q_num],USB_TX,Q_num));
		os_printk(K_ERR,"USB_QMU_TQSAR %x\r\n", os_readl(USB_QMU_TQSAR(Q_num)));
		req->complete=1;//true;
		os_printk(K_ERR,"TxQ %d Flush Now!\r\n", Q_num);

	}
    else if(dir == USB_RX){	  
		
		os_printk(K_CRIT,"flush_qmu USB_RX\r\n");
		mu3d_hal_stop_qmu(Q_num, USB_RX);
		gpd_current = (TGPD*)(os_readl(USB_QMU_RQCPR(Q_num)));
		if(!gpd_current){
			gpd_current = (TGPD*)(os_readl(USB_QMU_RQSAR(Q_num)));
		}
		os_printk(K_CRIT,"gpd_current(P) %x\r\n", (unsigned int)gpd_current);
		gpd_current = gpd_phys_to_virt(gpd_current,USB_RX,Q_num);
		os_printk(K_CRIT,"gpd_current(V) %x\r\n", (unsigned int)gpd_current);
		Rx_gpd_end[Q_num] /*= Rx_gpd_end[Q_num]*/ = gpd_current;
		gpd_ptr_align(dir,Q_num,Rx_gpd_end[Q_num]);
		free_gpd(dir,Q_num);
		os_writel(USB_QMU_RQSAR(Q_num), mu3d_hal_gpd_virt_to_phys(Rx_gpd_end[Q_num],USB_RX,Q_num));
		os_printk(K_ERR,"USB_QMU_RQSAR %x\r\n", os_readl(USB_QMU_RQSAR(Q_num)));
		req->complete=1;//true;
		os_printk(K_ERR,"RxQ %d Flush Now!\r\n", Q_num);
    }

}


