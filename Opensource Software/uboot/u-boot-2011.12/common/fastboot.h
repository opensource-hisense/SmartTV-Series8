#ifndef __FASTBOOT_H
#define __FASTBOOT_H

#include <../drivers/usb/udc.h>

struct fastboot_cmd {
	struct fastboot_cmd *next;
	const char *prefix;
	unsigned prefix_len;
    unsigned sec_support;
	void (*handle)(const char *arg,const char *data,int sz );
};

struct fastboot_var {
	struct fastboot_var *next;
	char name[64];
    char value[64];
};

void fastboot_okay(const char *info);
void fastboot_fail(const char *reason);
void fastboot_register(const char *prefix, void (*handle)(const char *arg, const char *data,int sz ), unsigned char security_enabled);
void fastboot_info(const char *reason);



#define STATE_OFFLINE	0
#define STATE_COMMAND	1
#define STATE_COMPLETE	2
#define STATE_ERROR	3
#define STATE_ONLINE    4

#define REQ_STATE_DONE     0
#define REQ_STATE_READING  1
#define REQ_STATE_WRITING  2
#define REQ_STATE_FAIL     3

#endif
