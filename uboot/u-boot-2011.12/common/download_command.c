#include "download_command.h"
#include "fastboot.h"
#include "sparse_format.h"
#include <common.h>
#include <command.h>
#include <jffs2/jffs2.h>
#include <mmc.h>
#include <bootimg.h>

#ifdef CONFIG_USB_FASTBOOT

extern void *download_base;
extern unsigned download_max;
extern unsigned download_size;
extern unsigned fastboot_state;
extern struct fastboot_var *varlist;

static const unsigned char wipedata_bin[] = {
    0x62, 0x6f, 0x6f, 0x74, 0x2d, 0x72, 0x65, 0x63, 0x6f, 0x76, 0x65, 0x72, 
    0x79, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x72, 0x65, 0x63, 0x6f, 0x76, 0x65, 0x72, 0x79, 
    0x0a, 0x2d, 0x2d, 0x77, 0x69, 0x70, 0x65, 0x5f, 0x64, 0x61, 0x74, 0x61, 
    0x0a
};

static unsigned hex2unsigned(const char *x)
{
    unsigned n = 0;

    while(*x) {
        switch(*x) {
        case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9':
            n = (n << 4) | (*x - '0');
            break;
        case 'a': case 'b': case 'c':
        case 'd': case 'e': case 'f':
            n = (n << 4) | (*x - 'a' + 10);
            break;
        case 'A': case 'B': case 'C':
        case 'D': case 'E': case 'F':
            n = (n << 4) | (*x - 'A' + 10);
            break;
        default:
            return n;
        }
        x++;
    }

    return n;
}

void download_value_init()
{
	download_base = CFG_LOAD_ADDR + 0x100000;
	download_size = 0;
	return;
}

static int download_standard(u32 data_length)
{
	int r;
	printf("USB Transferring... .bufer[x%x], len=%d\n",download_base, data_length);

	r = usb_read(download_base, data_length);

	if ((r < 0) || ((unsigned) r != data_length))
	{
		printf("Read USB error r = %d, data_length = %d.\n", r, data_length);
		fastboot_state = STATE_ERROR;
		return -1;
	}
	download_size = data_length;

	printf("read OK.  data:0x%x\n", *((int*)download_base));

	return 0;
}

void do_resetBootloader(const char *arg,const char *data, unsigned int sz)
{
	printf("rebooting the device\n");
	fastboot_okay("");
 	do_reset(NULL,NULL,NULL,NULL);
}
void do_continue(const char *arg, const char *data, unsigned int sz)
{
	printf("continue the boot\n");
	fastboot_okay("");
	fastboot_state = STATE_ERROR;
}
void do_reboot(const char *arg, const char *data, unsigned int sz)
{
	printf("rebooting the device\n");
	extern int emmc_write_to_partition(unsigned char* buf, char* pname, u64 offset, unsigned int size);
	emmc_write_to_partition((unsigned char*)"00000000", "misc", 512, sizeof("fastboot"));
	fastboot_okay("");
 	do_reset(NULL,NULL,NULL,NULL);
}


int do_download(const char *arg, const char *data,unsigned int sz )
{
	char response[64];
	unsigned len = hex2unsigned(data);
	printf("do_download len = %d, filesize = %d .\n", len, sz);
	
	if(len == 0)
	{
		fastboot_okay("");
		return;
	}
	sprintf(response, "DATA%08x", len);
	if (usb_write(response, strlen(response)) < 0)
	{
	    printf("cmd_download -- usb write fail\n");
		return;
	}

	download_standard(len);
	fastboot_okay("");
	
    return 0;
}


#define CMD_FLASH_TO_EMMC(data,offset,sz)														\
do {																							\
    char *local_args[7];																		\
    char p0[64], p1[32], p2[32], p3[32];														\
	local_args[0] = "mmc";																		\
    local_args[1] = "write.b";																	\
    sprintf(p0, "0x%lx", CONFIG_SYS_MMC_ENV_DEV);												\
    local_args[2] = p0;																			\
    sprintf(p1, "0x%lx", data); 																\
    local_args[3] = p1;																			\
    sprintf(p2, "0x%llx", offset); 																\
    local_args[4] = p2;																			\
    sprintf(p3, "0x%lx", sz);																	\
    local_args[5] = p3;																			\
    if (do_mmcops(NULL, 0, 6, local_args)) {	fastboot_fail("mmc cops fail");	 return -1;	}	\											
} while(0)

int cmd_flash_emmc_sparse_img(char * data, unsigned long long offset,unsigned int sz)
{
	unsigned int chunk;
	unsigned int chunk_data_sz;
	sparse_header_t *sparse_header;
	chunk_header_t *chunk_header;
	unsigned int total_blocks = 0;
    char *local_args[7];
    char p0[64], p1[32], p2[32], p3[32];
	printf( "Enter cmd_flash_sparse_img()\n");


	/* Read and skip over sparse image header */
	sparse_header = (sparse_header_t *) data;
	data += sparse_header->file_hdr_sz;
	if(sparse_header->file_hdr_sz > sizeof(sparse_header_t))
	{
		/* Skip the remaining bytes in a header that is longer than
		* we expected.
		*/
		data += (sparse_header->file_hdr_sz - sizeof(sparse_header_t));
	}

	printf ("=== Sparse Image Header ===\n");
	printf ("magic: 0x%x\n", sparse_header->magic);
	printf ("major_version: 0x%x\n", sparse_header->major_version);
	printf ("minor_version: 0x%x\n", sparse_header->minor_version);
	printf ("file_hdr_sz: %d\n", sparse_header->file_hdr_sz);
	printf ("chunk_hdr_sz: %d\n", sparse_header->chunk_hdr_sz);
	printf ("blk_sz: %d\n", sparse_header->blk_sz);
	printf ("total_blks: %d\n", sparse_header->total_blks);
	printf ("total_chunks: %d\n", sparse_header->total_chunks);

	printf("Writing Flash ... ");
	/* Start processing chunks */
	for (chunk=0; chunk<sparse_header->total_chunks; chunk++)
	{
		/* Read and skip over chunk header */
		chunk_header = (chunk_header_t *) data;
		data += sizeof(chunk_header_t);

		printf ("=== Chunk Header ===\n");
		printf ("chunk_type: 0x%x\n", chunk_header->chunk_type);
		printf ("chunk_data_sz: 0x%x\n", chunk_header->chunk_sz);
		printf ("total_size: 0x%x\n", chunk_header->total_sz);

		if(sparse_header->chunk_hdr_sz > sizeof(chunk_header_t))
		{
			/* Skip the remaining bytes in a header that is longer than
			* we expected.
			*/
			data += (sparse_header->chunk_hdr_sz - sizeof(chunk_header_t));
		}

		chunk_data_sz = sparse_header->blk_sz * chunk_header->chunk_sz;
		switch (chunk_header->chunk_type)
		{
		case CHUNK_TYPE_RAW:
			if(chunk_header->total_sz != (sparse_header->chunk_hdr_sz +
				chunk_data_sz))
			{
				fastboot_fail("Bogus chunk size for chunk type Raw");
				return -1;
			}


			//dprintf(INFO, "[Flash Base Address:0x%llx offset:0x%llx]-[size:%d]-[DRAM Address:0x%x]\n",
			//	ptn , ((uint64_t)total_blocks*sparse_header->blk_sz), chunk_data_sz, data);

			CMD_FLASH_TO_EMMC(data,(offset + ((uint64_t)total_blocks*sparse_header->blk_sz)),chunk_data_sz);

			total_blocks += chunk_header->chunk_sz;
			data += chunk_data_sz;
			break;

		case CHUNK_TYPE_DONT_CARE:
			total_blocks += chunk_header->chunk_sz;
			break;

		case CHUNK_TYPE_CRC:
			if(chunk_header->total_sz != sparse_header->chunk_hdr_sz)
			{
				fastboot_fail("Bogus chunk size for chunk type Dont Care");
				return -1;
			}
			total_blocks += chunk_header->chunk_sz;
			data += chunk_data_sz;
			break;

		default:
			fastboot_fail("Unknown chunk type");
			return -1;
		}
	}

	printf("Wrote %d blocks, expected to write %d blocks\n",total_blocks, sparse_header->total_blks);

	if(total_blocks != sparse_header->total_blks)
	{
		fastboot_fail("sparse image write failure");
		return -1;
	}

	fastboot_okay("Write Flash OK");
	
	return 0;;
}

/* Ported by Zhendong Wang starts for fastboot supporting packed image */

/* Note: This function must accordate with the the u32_to_u8 function in
 * image_pack.c. Otherwise, the parsed u32 is incorrect
 */
void u8_array_to_u32( const u8 * array, u32 * num ) {
    u32 i = 0;
    *num = 0;
    for( i = 0; i < 4; i++ ) {
        *num <<= 8;
        *num += array[i];
    }
} 

int do_update_packed_images(void)
{
    int i;
    u8 *pTmp, *name;
    img_hdr *images_header;
    sparse_header_t *sparse_header;
    u32 size, partition_num, num_images, images_size, header_size;
    u64 partition_size, partition_offset;

    images_header = download_base;
    pTmp = download_base;

    u8_array_to_u32(&images_header->num_images, &num_images);
    u8_array_to_u32(&images_header->images_size, &images_size);
    u8_array_to_u32(&images_header->header_size, &header_size);
    if (num_images < 1) {
	printf("This packed images contain no image.\n");
	fastboot_fail("The packed images contain no image");
	return MULTI_IMAGE_ERROR;
    }

    pTmp += header_size;
    struct {
	u8 name[PARTITION_NAME_SIZE];
	u32 size;
    } *img_info = images_header->img_info;

    for (i = 0; i < num_images; i++) {
	name = img_info[i].name;
	u8_array_to_u32(&img_info[i].size, &size);

	if (find_partition(name, &partition_num,
		    &partition_size, &partition_offset)) {
	    printf("The partition %s is not defined.\n", name);
	    fastboot_fail("The partition is not defined");
	    return MULTI_IMAGE_ERROR;
	}
	if (size > partition_size) {
	    printf("Image size is larger than the partition size.\n");
	    fastboot_fail("Image size is larger than the partition size");
            return MULTI_IMAGE_ERROR;
	}

	sparse_header = pTmp;
	printf("Write partition %s.\n", name);

	if (sparse_header->magic != SPARSE_HEADER_MAGIC) {
	    CMD_FLASH_TO_EMMC(pTmp, partition_offset, size);
	    fastboot_okay("finished");
	} else {
	    if (cmd_flash_emmc_sparse_img(pTmp,
			partition_offset, size)) {
		fastboot_fail("mmc cops fail");
		return  MULTI_IMAGE_ERROR;
	    }
	    fastboot_okay("finished");
	}
	pTmp += size; 
    }

    return MULTI_IMAGE_WRITE_SUCCESS;
}

/* Ported by Zhendong Wang starts for fastboot supporting packed image ends */

int do_update(const char *arg,const char * data,unsigned int sz )
{
    /* part modified by David Li starts for fastboot supporting packed image */
    if( strncmp( ( const char* ) download_base, "BOOTLDR!", 8 ) == 0 ){
        return do_update_packed_images();
    }
    /* part modified by David Li ends */
	sparse_header_t *sparse_header;

    u8 part_num;
    u64 part_size, part_offset;
	sz = download_size;
	printf("do_update filesize = %d .\n", sz);
	
    if (find_partition(data, &part_num, &part_size, &part_offset))
    {
        printf("the %s partition not defined at mtdparts!\n", *arg );
        return -1;
    }


    data = (const char*)download_base;
	sparse_header = (sparse_header_t *) data;

	printf("cmd_flash_emmc, data:0x%x,, n sparse_header->magic = 0x%x\n",*(int*)data, sparse_header->magic );

    if(sz > part_size) 
    {  
		fastboot_fail("file size larger");
        printf("update abort! file size is larger than partition size.\n");
        return -1;            
    }

	if (sparse_header->magic != SPARSE_HEADER_MAGIC)
	{
		 CMD_FLASH_TO_EMMC(data,part_offset,sz);
		 fastboot_okay("Write Flash OK");
	}
	else
	{
		if (cmd_flash_emmc_sparse_img(data, part_offset, sz))
			{
				fastboot_fail("mmc cops fail");
				return -1;
			}
		
		fastboot_okay("");
	}
    return 0;
}


int do_getvar(const char *arg, const char *data,unsigned int sz )
{
	struct fastboot_var *var;
	char response[64];
	printf("do_getvar begin:.\n");
	if(!strcmp(data, "all")){
		for (var = varlist; var; var = var->next){
			sprintf(response, "\t%s: %s", var->name, var->value);
			fastboot_info(response);
		}
    		fastboot_okay("Done!!");
		return;
	}
	for (var = varlist; var; var = var->next) {
		if (!strcmp(var->name, data)) {
                fastboot_okay(var->value);
			return;
		}
	}
	fastboot_okay("");
}

int do_erase_emmc(const char *arg, const char *data,unsigned int sz )
{
	u8 part_num;
    u64 part_size, part_offset;
	sz = sizeof(wipedata_bin)/sizeof(wipedata_bin[0]);
	printf("do_erase_emmc begin.\n");
	
    if (find_partition("misc", &part_num, &part_size, &part_offset))
    {
        printf("the %s partition not defined at mtdparts!\n", *data );
        return -1;
    }

	if (!strcmp("userdata", data)) {
		
		printf("begin erase userdata.\n");
		data = (const char*)wipedata_bin;
		CMD_FLASH_TO_EMMC(data,part_offset,sz);
		printf("end erase userdata.\n");
	}
	else
	{
		
		printf("didnot support this erase command.\n");
	}
	fastboot_okay("");
	
    return 0;
}

#endif

