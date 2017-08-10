#ifndef __DOWNLOAD_COMMANDS_H
#define __DOWNLOAD_COMMANDS_H

#include <common.h>
#include <watchdog.h>
#include <command.h>
#include <image.h>
#include <malloc.h>
#include <u-boot/zlib.h>
#include <bzlib.h>
#include <environment.h>
#include <lmb.h>
#include <linux/ctype.h>
#include <asm/byteorder.h>
#include <linux/compiler.h>

/* Copy from image_pack.h in tools/fastboot_pack
 * Some const symbols defined here should move to header file after modification 
 * accepted. If you need to modify the const symbols here, please modify them in 
 * pack/unpack tool header, too.
 */
#define BOOTLDR_MAGIC "BOOTLDR!"
#define BOOTLDR_MAGIC_SIZE 8
#define PARTITION_NAME_SIZE 64
#define IMAGE_LISTS_SIZE 64
#define OUTPUT_BUFFER_SIZE ( 1024 * 2 )
#define BIG_BUFFER_SIZE ( 1024 * 1024 * 1024 )

#define MULTI_IMAGE_WRITE_SUCCESS 0
#define MULTI_IMAGE_ERROR -1
  

typedef struct images_header {
  u8 magic[BOOTLDR_MAGIC_SIZE];
  u32 num_images;
  u32 images_size;
  u32 header_size;
  struct {
    u8 name[PARTITION_NAME_SIZE];
    u32 size;
 } img_info[];
} img_hdr ;
/* Copy from image_pack.h in tools/fastboot_pack ends */

void download_value_init(void);

void do_resetBootloader(const char *arg,const char *data, unsigned int sz);
void do_continue(const char *arg, const char *data, unsigned int sz);
void do_reboot(const char *arg, const char *data, unsigned int sz);
int do_download(const char *arg, const char *data,unsigned int sz );
int do_update(const char *arg, const char *data,unsigned int sz );
int do_getvar(const char *arg, const char *data, unsigned int sz );
int do_erase_emmc(const char *arg, const char *data, unsigned int sz);

#endif


