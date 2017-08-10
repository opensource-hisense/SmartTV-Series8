/*
 * Driver for NAND support, Rick Bronson
 * borrowed heavily from:
 * (c) 1999 Machine Vision Holdings, Inc.
 * (c) 1999, 2000 David Woodhouse <dwmw2@infradead.org>
 *
 * Ported 'dynenv' to 'nand env.oob' command
 * (C) 2010 Nanometrics, Inc.
 * 'dynenv' -- Dynamic environment offset in NAND OOB
 * (C) Copyright 2006-2007 OpenMoko, Inc.
 * Added 16-bit nand support
 * (C) 2004 Texas Instruments
 *
 * Copyright 2010 Freescale Semiconductor
 * The portions of this file whose copyright is held by Freescale and which
 * are not considered a derived work of GPL v2-only code may be distributed
 * and/or modified under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 */

#include <common.h>
#include <linux/mtd/mtd.h>
#include <command.h>
#include <watchdog.h>
#include <malloc.h>
#include <asm/byteorder.h>
#include <jffs2/jffs2.h>
#include <nand.h>
#include <bootimg.h>

#if defined(CONFIG_CMD_MTDPARTS)

/* partition handling routines */
int mtdparts_init(void);
int id_parse(const char *id, const char **ret_id, u8 *dev_type, u8 *dev_num);
int find_dev_and_part(const char *id, struct mtd_device **dev,
		      u8 *part_num, struct part_info **part);
int find_partition(const char *part_name, u8 *part_num, u32 *part_size, u32 *part_offset);
#endif

#if 1
extern int do_bootm(cmd_tbl_t *, int, int, char * const argv[]);
extern int get_boot_bank_index(void);
extern int do_usb (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[]);
extern int do_fat_fsload (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[]);
extern int do_load (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[]);
extern int IsRunOnUsb(char* uenv, int uenv_size);
extern void sig_authetication(unsigned long u4StartAddr, unsigned long u4Size,
    unsigned long *pu4Signature, unsigned long *pu4EncryptedKey);
#endif

static int nand_dump(nand_info_t *nand, ulong off, int only_oob, int repeat)
{
	int i;
	u_char *datbuf, *oobbuf, *p;
	static loff_t last;

	if (repeat)
		off = last + nand->writesize;

	last = off;

	datbuf = malloc(nand->writesize + nand->oobsize);
	oobbuf = malloc(nand->oobsize);
	if (!datbuf || !oobbuf) {
		puts("No memory for page buffer\n");
		return 1;
	}
	off &= ~(nand->writesize - 1);
	loff_t addr = (loff_t) off;
	struct mtd_oob_ops ops;
	memset(&ops, 0, sizeof(ops));
	ops.datbuf = datbuf;
	ops.oobbuf = oobbuf; /* must exist, but oob data will be appended to ops.datbuf */
	ops.len = nand->writesize;
	ops.ooblen = nand->oobsize;
	ops.mode = MTD_OOB_RAW;
	i = nand->read_oob(nand, addr, &ops);
	if (i < 0) {
		printf("Error (%d) reading page %08lx\n", i, off);
		free(datbuf);
		free(oobbuf);
		return 1;
	}
	printf("Page %08lx dump:\n", off);
	i = nand->writesize >> 4;
	p = datbuf;

	while (i--) {
		if (!only_oob)
			printf("\t%02x %02x %02x %02x %02x %02x %02x %02x"
			       "  %02x %02x %02x %02x %02x %02x %02x %02x\n",
			       p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7],
			       p[8], p[9], p[10], p[11], p[12], p[13], p[14],
			       p[15]);
		p += 16;
	}
	puts("OOB:\n");
	i = nand->oobsize >> 3;
	while (i--) {
		printf("\t%02x %02x %02x %02x %02x %02x %02x %02x\n",
		       p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7]);
		p += 8;
	}
	free(datbuf);
	free(oobbuf);

	return 0;
}

/* ------------------------------------------------------------------------- */

static int set_dev(int dev)
{
	if (dev < 0 || dev >= CONFIG_SYS_MAX_NAND_DEVICE ||
	    !nand_info[dev].name) {
		puts("No such device\n");
		return -1;
	}

	if (nand_curr_device == dev)
		return 0;

	printf("Device %d: %s", dev, nand_info[dev].name);
	puts("... is now current device\n");
	nand_curr_device = dev;

#ifdef CONFIG_SYS_NAND_SELECT_DEVICE
	board_nand_select_device(nand_info[dev].priv, dev);
#endif

	return 0;
}

static inline int str2off(const char *p, loff_t *num)
{
	char *endptr;

	*num = simple_strtoull(p, &endptr, 16);
	return *p != '\0' && *endptr == '\0';
}

static inline int str2long(const char *p, ulong *num)
{
	char *endptr;

	*num = simple_strtoul(p, &endptr, 16);
	return *p != '\0' && *endptr == '\0';
}

static int get_part(const char *partname, int *idx, loff_t *off, loff_t *size)
{
#ifdef CONFIG_CMD_MTDPARTS
	struct mtd_device *dev;
	struct part_info *part;
	u8 pnum;
	int ret;

	ret = mtdparts_init();
	if (ret)
		return ret;

	ret = find_dev_and_part(partname, &dev, &pnum, &part);
	if (ret)
		return ret;

	if (dev->id->type != MTD_DEV_TYPE_NAND) {
		puts("not a NAND device\n");
		return -1;
	}

	*off = part->offset;
	*size = part->size;
	*idx = dev->id->num;

	ret = set_dev(*idx);
	if (ret)
		return ret;

	return 0;
#else
	puts("offset is not a number\n");
	return -1;
#endif
}

static int arg_off(const char *arg, int *idx, loff_t *off, loff_t *maxsize)
{
	if (!str2off(arg, off))
		return get_part(arg, idx, off, maxsize);

	if (*off >= nand_info[*idx].size) {
		puts("Offset exceeds device limit\n");
		return -1;
	}

	*maxsize = nand_info[*idx].size - *off;
	return 0;
}

static int arg_off_size(int argc, char *const argv[], int *idx,
			loff_t *off, loff_t *size)
{
	int ret;
	loff_t maxsize;

	if (argc == 0) {
		*off = 0;
		*size = nand_info[*idx].size;
		goto print;
	}

	ret = arg_off(argv[0], idx, off, &maxsize);
	if (ret)
		return ret;

	if (argc == 1) {
		*size = maxsize;
		goto print;
	}

	if (!str2off(argv[1], size)) {
		printf("'%s' is not a number\n", argv[1]);
		return -1;
	}

	if (*size > maxsize) {
		puts("Size exceeds partition or device limit\n");
		return -1;
	}

print:
	printf("device %d ", *idx);
	if (*size == nand_info[*idx].size)
		puts("whole chip\n");
	else
		printf("offset 0x%llx, size 0x%llx\n",
		       (unsigned long long)*off, (unsigned long long)*size);
	return 0;
}

#ifdef CONFIG_CMD_NAND_LOCK_UNLOCK
static void print_status(ulong start, ulong end, ulong erasesize, int status)
{
	printf("%08lx - %08lx: %08lx blocks %s%s%s\n",
		start,
		end - 1,
		(end - start) / erasesize,
		((status & NAND_LOCK_STATUS_TIGHT) ?  "TIGHT " : ""),
		((status & NAND_LOCK_STATUS_LOCK) ?  "LOCK " : ""),
		((status & NAND_LOCK_STATUS_UNLOCK) ?  "UNLOCK " : ""));
}

static void do_nand_status(nand_info_t *nand)
{
	ulong block_start = 0;
	ulong off;
	int last_status = -1;

	struct nand_chip *nand_chip = nand->priv;
	/* check the WP bit */
	nand_chip->cmdfunc(nand, NAND_CMD_STATUS, -1, -1);
	printf("device is %swrite protected\n",
		(nand_chip->read_byte(nand) & 0x80 ?
		"NOT " : ""));

	for (off = 0; off < nand->size; off += nand->erasesize) {
		int s = nand_get_lock_status(nand, off);

		/* print message only if status has changed */
		if (s != last_status && off != 0) {
			print_status(block_start, off, nand->erasesize,
					last_status);
			block_start = off;
		}
		last_status = s;
	}
	/* Print the last block info */
	print_status(block_start, off, nand->erasesize, last_status);
}
#endif

#ifdef CONFIG_ENV_OFFSET_OOB
unsigned long nand_env_oob_offset;

int do_nand_env_oob(cmd_tbl_t *cmdtp, int argc, char *const argv[])
{
	int ret;
	uint32_t oob_buf[ENV_OFFSET_SIZE/sizeof(uint32_t)];
	nand_info_t *nand = &nand_info[0];
	char *cmd = argv[1];

	if (CONFIG_SYS_MAX_NAND_DEVICE == 0 || !nand->name) {
		puts("no devices available\n");
		return 1;
	}

	set_dev(0);

	if (!strcmp(cmd, "get")) {
		ret = get_nand_env_oob(nand, &nand_env_oob_offset);
		if (ret)
			return 1;

		printf("0x%08lx\n", nand_env_oob_offset);
	} else if (!strcmp(cmd, "set")) {
		loff_t addr;
		loff_t maxsize;
		struct mtd_oob_ops ops;
		int idx = 0;

		if (argc < 3)
			goto usage;

		if (arg_off(argv[2], &idx, &addr, &maxsize)) {
			puts("Offset or partition name expected\n");
			return 1;
		}

		if (idx != 0) {
			puts("Partition not on first NAND device\n");
			return 1;
		}

		if (nand->oobavail < ENV_OFFSET_SIZE) {
			printf("Insufficient available OOB bytes:\n"
			       "%d OOB bytes available but %d required for "
			       "env.oob support\n",
			       nand->oobavail, ENV_OFFSET_SIZE);
			return 1;
		}

		if ((addr & (nand->erasesize - 1)) != 0) {
			printf("Environment offset must be block-aligned\n");
			return 1;
		}

		ops.datbuf = NULL;
		ops.mode = MTD_OOB_AUTO;
		ops.ooboffs = 0;
		ops.ooblen = ENV_OFFSET_SIZE;
		ops.oobbuf = (void *) oob_buf;

		oob_buf[0] = ENV_OOB_MARKER;
		oob_buf[1] = addr / nand->erasesize;

		ret = nand->write_oob(nand, ENV_OFFSET_SIZE, &ops);
		if (ret) {
			printf("Error writing OOB block 0\n");
			return ret;
		}

		ret = get_nand_env_oob(nand, &nand_env_oob_offset);
		if (ret) {
			printf("Error reading env offset in OOB\n");
			return ret;
		}

		if (addr != nand_env_oob_offset) {
			printf("Verification of env offset in OOB failed: "
			       "0x%08llx expected but got 0x%08lx\n",
			       (unsigned long long)addr, nand_env_oob_offset);
			return 1;
		}
	} else {
		goto usage;
	}

	return ret;

usage:
	return cmd_usage(cmdtp);
}

#endif

static void nand_print_and_set_info(int idx)
{
	nand_info_t *nand = &nand_info[idx];
	struct nand_chip *chip = nand->priv;
	const int bufsz = 32;
	char buf[bufsz];

	printf("Device %d: ", idx);
	if (chip->numchips > 1)
		printf("%dx ", chip->numchips);
	printf("%s, sector size %u KiB\n",
	       nand->name, nand->erasesize >> 10);
	printf("  Page size  %8d b\n", nand->writesize);
	printf("  OOB size   %8d b\n", nand->oobsize);
	printf("  Erase size %8d b\n", nand->erasesize);

	/* Set geometry info */
	sprintf(buf, "%x", nand->writesize);
	setenv("nand_writesize", buf);

	sprintf(buf, "%x", nand->oobsize);
	setenv("nand_oobsize", buf);

	sprintf(buf, "%x", nand->erasesize);
	setenv("nand_erasesize", buf);
}

int do_nand(cmd_tbl_t * cmdtp, int flag, int argc, char * const argv[])
{
	int i, ret = 0;
	ulong addr;
	loff_t off, size;
	char *cmd, *s;
	nand_info_t *nand;
#ifdef CONFIG_SYS_NAND_QUIET
	int quiet = CONFIG_SYS_NAND_QUIET;
#else
	int quiet = 0;
#endif
	const char *quiet_str = getenv("quiet");
	int dev = nand_curr_device;
	int repeat = flag & CMD_FLAG_REPEAT;

	/* at least two arguments please */
	if (argc < 2)
		goto usage;

	if (quiet_str)
		quiet = simple_strtoul(quiet_str, NULL, 0) != 0;

	cmd = argv[1];

	/* Only "dump" is repeatable. */
	if (repeat && strcmp(cmd, "dump"))
		return 0;

	if (strcmp(cmd, "info") == 0) {

		putc('\n');
		for (i = 0; i < CONFIG_SYS_MAX_NAND_DEVICE; i++) {
			if (nand_info[i].name)
				nand_print_and_set_info(i);
		}
		return 0;
	}

	if (strcmp(cmd, "device") == 0) {
		if (argc < 3) {
			putc('\n');
			if (dev < 0 || dev >= CONFIG_SYS_MAX_NAND_DEVICE)
				puts("no devices available\n");
			else
				nand_print_and_set_info(dev);
			return 0;
		}

		dev = (int)simple_strtoul(argv[2], NULL, 10);
		set_dev(dev);

		return 0;
	}

#ifdef CONFIG_ENV_OFFSET_OOB
	/* this command operates only on the first nand device */
	if (strcmp(cmd, "env.oob") == 0)
		return do_nand_env_oob(cmdtp, argc - 1, argv + 1);
#endif

	/* The following commands operate on the current device, unless
	 * overridden by a partition specifier.  Note that if somehow the
	 * current device is invalid, it will have to be changed to a valid
	 * one before these commands can run, even if a partition specifier
	 * for another device is to be used.
	 */
	if (dev < 0 || dev >= CONFIG_SYS_MAX_NAND_DEVICE ||
	    !nand_info[dev].name) {
		puts("\nno devices available\n");
		return 1;
	}
	nand = &nand_info[dev];

	if (strcmp(cmd, "bad") == 0) {
		printf("\nDevice %d bad blocks:\n", dev);
		for (off = 0; off < nand->size; off += nand->erasesize)
			if (nand_block_isbad(nand, off))
				printf("  %08llx\n", (unsigned long long)off);
		return 0;
	}

	/*
	 * Syntax is:
	 *   0    1     2       3    4
	 *   nand erase [clean] [off size]
	 */
	if (strncmp(cmd, "erase", 5) == 0 || strncmp(cmd, "scrub", 5) == 0) {
		nand_erase_options_t opts;
		/* "clean" at index 2 means request to write cleanmarker */
		int clean = argc > 2 && !strcmp("clean", argv[2]);
		int scrub_yes = argc > 2 && !strcmp("-y", argv[2]);
		int o = (clean || scrub_yes) ? 3 : 2;
		int scrub = !strncmp(cmd, "scrub", 5);
		int spread = 0;
		int args = 2;
		const char *scrub_warn =
			"Warning: "
			"scrub option will erase all factory set bad blocks!\n"
			"         "
			"There is no reliable way to recover them.\n"
			"         "
			"Use this command only for testing purposes if you\n"
			"         "
			"are sure of what you are doing!\n"
			"\nReally scrub this NAND flash? <y/N>\n";

		if (cmd[5] != 0) {
			if (!strcmp(&cmd[5], ".spread")) {
				spread = 1;
			} else if (!strcmp(&cmd[5], ".part")) {
				args = 1;
			} else if (!strcmp(&cmd[5], ".chip")) {
				args = 0;
			} else {
				goto usage;
			}
		}

		/*
		 * Don't allow missing arguments to cause full chip/partition
		 * erases -- easy to do accidentally, e.g. with a misspelled
		 * variable name.
		 */
		if (argc != o + args)
			goto usage;

		printf("\nNAND %s: ", cmd);
		/* skip first two or three arguments, look for offset and size */
		if (arg_off_size(argc - o, argv + o, &dev, &off, &size) != 0)
			return 1;

		nand = &nand_info[dev];

		memset(&opts, 0, sizeof(opts));
		opts.offset = off;
		opts.length = size;
		opts.jffs2  = clean;
		opts.quiet  = quiet;
		opts.spread = spread;

		if (scrub) {
			if (!scrub_yes)
				puts(scrub_warn);

			if (scrub_yes)
				opts.scrub = 1;
			else if (getc() == 'y') {
				puts("y");
				if (getc() == '\r')
					opts.scrub = 1;
				else {
					puts("scrub aborted\n");
					return -1;
				}
			} else {
				puts("scrub aborted\n");
				return -1;
			}
		}
		ret = nand_erase_opts(nand, &opts);
		printf("%s\n", ret ? "ERROR" : "OK");

		return ret == 0 ? 0 : 1;
	}

	if (strncmp(cmd, "dump", 4) == 0) {
		if (argc < 3)
			goto usage;

		off = (int)simple_strtoul(argv[2], NULL, 16);
		ret = nand_dump(nand, off, !strcmp(&cmd[4], ".oob"), repeat);

		return ret == 0 ? 1 : 0;
	}

	if (strncmp(cmd, "read", 4) == 0 || strncmp(cmd, "write", 5) == 0) {
		size_t rwsize;
		int read;

		if (argc < 4)
			goto usage;

		addr = (ulong)simple_strtoul(argv[2], NULL, 16);

		read = strncmp(cmd, "read", 4) == 0; /* 1 = read, 0 = write */
		printf("\nNAND %s: ", read ? "read" : "write");
		if (arg_off_size(argc - 3, argv + 3, &dev, &off, &size) != 0)
			return 1;

		nand = &nand_info[dev];
		rwsize = size;

		s = strchr(cmd, '.');
		if (!s || !strcmp(s, ".jffs2") || !strcmp(s, ".ubifs")||
		    !strcmp(s, ".e") || !strcmp(s, ".i")) {
			if (read)
				ret = nand_read_skip_bad(nand, off, &rwsize,
							 (u_char *)addr);
			else
				ret = nand_write_skip_bad(nand, off, &rwsize,
							  (u_char *)addr, 0);
#ifdef CONFIG_CMD_NAND_TRIMFFS
		} else if (!strcmp(s, ".trimffs")) {
			if (read) {
				printf("Unknown nand command suffix '%s'\n", s);
				return 1;
			}
			ret = nand_write_skip_bad(nand, off, &rwsize,
						(u_char *)addr,
						WITH_DROP_FFS);
#endif
#ifdef CONFIG_CMD_NAND_YAFFS
		} else if (!strcmp(s, ".yaffs")) {
			if (read) {
				printf("Unknown nand command suffix '%s'.\n", s);
				return 1;
			}
			ret = nand_write_skip_bad(nand, off, &rwsize,
						(u_char *)addr, WITH_YAFFS_OOB);
#endif
		} else if (!strcmp(s, ".oob")) {
			/* out-of-band data */
			mtd_oob_ops_t ops = {
				.oobbuf = (u8 *)addr,
				.ooblen = rwsize,
				.mode = MTD_OOB_RAW
			};

			if (read)
				ret = nand->read_oob(nand, off, &ops);
			else
				ret = nand->write_oob(nand, off, &ops);
		} else if (!strcmp(s, ".raw")) {
			/* Raw access */
			mtd_oob_ops_t ops = {
				.datbuf = (u8 *)addr,
				.oobbuf = ((u8 *)addr) + nand->writesize,
				.len = nand->writesize,
				.ooblen = nand->oobsize,
				.mode = MTD_OOB_RAW
			};

			rwsize = nand->writesize + nand->oobsize;

			if (read)
				ret = nand->read_oob(nand, off, &ops);
			else
				ret = nand->write_oob(nand, off, &ops);
		} else {
			printf("Unknown nand command suffix '%s'.\n", s);
			return 1;
		}

		printf(" %zu bytes %s: %s\n", rwsize,
		       read ? "read" : "written", ret ? "ERROR" : "OK");

		return ret == 0 ? 0 : 1;
	}

	if (strcmp(cmd, "markbad") == 0) {
		argc -= 2;
		argv += 2;

		if (argc <= 0)
			goto usage;

		while (argc > 0) {
			addr = simple_strtoul(*argv, NULL, 16);

			if (nand->block_markbad(nand, addr)) {
				printf("block 0x%08lx NOT marked "
					"as bad! ERROR %d\n",
					addr, ret);
				ret = 1;
			} else {
				printf("block 0x%08lx successfully "
					"marked as bad\n",
					addr);
			}
			--argc;
			++argv;
		}
		return ret;
	}

	if (strcmp(cmd, "biterr") == 0) {
		/* todo */
		return 1;
	}

#ifdef CONFIG_CMD_NAND_LOCK_UNLOCK
	if (strcmp(cmd, "lock") == 0) {
		int tight = 0;
		int status = 0;
		if (argc == 3) {
			if (!strcmp("tight", argv[2]))
				tight = 1;
			if (!strcmp("status", argv[2]))
				status = 1;
		}
		if (status) {
			do_nand_status(nand);
		} else {
			if (!nand_lock(nand, tight)) {
				puts("NAND flash successfully locked\n");
			} else {
				puts("Error locking NAND flash\n");
				return 1;
			}
		}
		return 0;
	}

	if (strcmp(cmd, "unlock") == 0) {
		if (arg_off_size(argc - 2, argv + 2, &dev, &off, &size) < 0)
			return 1;

		if (!nand_unlock(&nand_info[dev], off, size)) {
			puts("NAND flash successfully unlocked\n");
		} else {
			puts("Error unlocking NAND flash, "
			     "write and erase will probably fail\n");
			return 1;
		}
		return 0;
	}
#endif

usage:
	return cmd_usage(cmdtp);
}

U_BOOT_CMD(
	nand, CONFIG_SYS_MAXARGS, 1, do_nand,
	"NAND sub-system",
	"info - show available NAND devices\n"
	"nand device [dev] - show or set current device\n"
	"nand read - addr off|partition size\n"
	"nand write - addr off|partition size\n"
	"    read/write 'size' bytes starting at offset 'off'\n"
	"    to/from memory address 'addr', skipping bad blocks.\n"
	"nand read.raw - addr off|partition\n"
	"nand write.raw - addr off|partition\n"
	"    Use read.raw/write.raw to avoid ECC and access the page as-is.\n"
#ifdef CONFIG_CMD_NAND_TRIMFFS
	"nand write.trimffs - addr off|partition size\n"
	"    write 'size' bytes starting at offset 'off' from memory address\n"
	"    'addr', skipping bad blocks and dropping any pages at the end\n"
	"    of eraseblocks that contain only 0xFF\n"
#endif
#ifdef CONFIG_CMD_NAND_YAFFS
	"nand write.yaffs - addr off|partition size\n"
	"    write 'size' bytes starting at offset 'off' with yaffs format\n"
	"    from memory address 'addr', skipping bad blocks.\n"
#endif
	"nand erase[.spread] [clean] off size - erase 'size' bytes "
	"from offset 'off'\n"
	"    With '.spread', erase enough for given file size, otherwise,\n"
	"    'size' includes skipped bad blocks.\n"
	"nand erase.part [clean] partition - erase entire mtd partition'\n"
	"nand erase.chip [clean] - erase entire chip'\n"
	"nand bad - show bad blocks\n"
	"nand dump[.oob] off - dump page\n"
	"nand scrub [-y] off size | scrub.part partition | scrub.chip\n"
	"    really clean NAND erasing bad blocks (UNSAFE)\n"
	"nand markbad off [...] - mark bad block(s) at offset (UNSAFE)\n"
	"nand biterr off - make a bit error at offset (UNSAFE)"
#ifdef CONFIG_CMD_NAND_LOCK_UNLOCK
	"\n"
	"nand lock [tight] [status]\n"
	"    bring nand to lock state or display locked pages\n"
	"nand unlock [offset] [size] - unlock section"
#endif
#ifdef CONFIG_ENV_OFFSET_OOB
	"\n"
	"nand env.oob - environment offset in OOB of block 0 of"
	"    first device.\n"
	"nand env.oob set off|partition - set enviromnent offset\n"
	"nand env.oob get - get environment offset"
#endif
);

#ifdef CC_SECURE_ROM_BOOT
// Auth kernel image at addr
static void nand_auth_image(nand_info_t *nand, struct part_info *part, ulong addr, ulong skip)
{
	image_header_t *hdr = (image_header_t *)addr;
    u32 sig_offset = image_get_image_size(hdr);
    #ifdef CC_SECURE_BOOT_V2
    // fix signature offset with pack size - fill 0xff by 16 bytes alignment by sign tool
    sig_offset = ((sig_offset) + 0x10 ) & (~0xf);
    #endif
 	printf("kernel: ");
    nand_part_read(nand, part, sig_offset+skip, 0x200, (u_char *)CFG_LOAD_ADDR);
    sig_authetication(addr, sig_offset, (unsigned long *)CFG_LOAD_ADDR, (unsigned long *)(CFG_LOAD_ADDR+0x100));
}

// Auth ramdisk image at addr
static void nand_auth_image_ex(struct part_info *part, ulong addr, ulong skip, ulong size_to_check)
{
	#ifndef CC_SECURE_BOOT_WO_RAMDISK
 	printf("ramdisk: ");
    #ifdef CC_SECURE_BOOT_V2
    // this only check size_to_check
    //sig_authetication(addr, size_to_check, (unsigned long *)(addr + skip - 0x100), (unsigned long *)(addr + skip));
    // this check full ramdisk size
    sig_authetication(addr, skip - 0x100, (unsigned long *)(addr + skip - 0x100), (unsigned long *)(addr + skip));
    #else // CC_SECURE_BOOT_V2
    sig_authetication(addr, size_to_check, (unsigned long *)(addr + skip - 0x200), (unsigned long *)(addr + skip - 0x100));
    #endif // CC_SECURE_BOOT_V2
    #endif // CC_SECURE_BOOT_WO_RAMDISK
}
#else
#define nand_auth_image(nand, part, addr, skip)
#define nand_auth_image_ex(part, addr, skip, size_to_check)
#endif

static int nand_load_image(cmd_tbl_t *cmdtp, nand_info_t *nand,
			   ulong offset, ulong addr, char *cmd)
{
	int r;
	char *ep, *s;
	size_t cnt;
	image_header_t *hdr;
    char *local_args[3];
    char local_args_temp[3][0x30];
#if defined(CONFIG_FIT)
	const void *fit_hdr = NULL;
#endif

	s = strchr(cmd, '.');
	if (s != NULL &&
	    (strcmp(s, ".jffs2") && strcmp(s, ".e") && strcmp(s, ".i") && strcmp(s, ".lzo"))) {
		printf("Unknown nand load suffix '%s'\n", s);
		show_boot_progress(-53);
		return 1;
	}

	printf("\nLoading from %s, offset 0x%lx\n", nand->name, offset);

	cnt = nand->writesize;
	r = nand_read_skip_bad(nand, offset, &cnt, (u_char *) addr);
	if (r) {
		puts("** Read error\n");
		show_boot_progress (-56);
		return 1;
	}
	show_boot_progress (56);

	switch (genimg_get_format ((void *)addr)) {
	case IMAGE_FORMAT_LEGACY:
		hdr = (image_header_t *)addr;

		show_boot_progress (57);
		image_print_contents (hdr);

		cnt = image_get_image_size (hdr);
		break;
#if defined(CONFIG_FIT)
	case IMAGE_FORMAT_FIT:
		fit_hdr = (const void *)addr;
		puts ("Fit image detected...\n");

		cnt = fit_get_size (fit_hdr);
		break;
#endif
	default:
		show_boot_progress (-57);
		puts ("** Unknown image type\n");
		return 1;
	}
	show_boot_progress (57);

	r = nand_read_skip_bad(nand, offset, &cnt, (u_char *) addr);
	if (r) {
		puts("** Read error\n");
		show_boot_progress (-58);
		return 1;
	}
	show_boot_progress (58);

#if !(CONFIG_FAST_BOOT) //skip verify for fast boot
    if (((ep = getenv("verify")) != NULL) && (strcmp(ep, "y") == 0))
    {
        puts ("   Verifying Checksum ... ");
        if (!image_check_dcrc (hdr))
        {
            printf ("Bad Data CRC\n");
            show_boot_progress (-58);
            return NULL;
        }
        puts ("OK\n");
    }
#endif

#if defined(CONFIG_FIT)
	/* This cannot be done earlier, we need complete FIT image in RAM first */
	if (genimg_get_format ((void *)addr) == IMAGE_FORMAT_FIT) {
		if (!fit_check_format (fit_hdr)) {
			show_boot_progress (-150);
			puts ("** Bad FIT image format\n");
			return 1;
		}
		show_boot_progress (151);
		fit_print_contents (fit_hdr);
	}
#endif

    if (cmd == NULL)
    {
        return 0;
    }

	/* Loading ok, update default load address */

	load_addr = addr;

    /* Check if we should attempt an auto-start */
    if (((ep = getenv("autostart")) != NULL) &&
        (strcmp(ep, "yes") == 0) &&
        (hdr->ih_type == IH_TYPE_KERNEL))
    {
        struct part_info part;

        local_args[0] = cmd;

        printf("Automatic boot of image at addr 0x%08lx ...\n", addr);

        part.offset = offset;
        nand_auth_image(nand, &part, addr, 0);

        if ((ep = getenv("initrd")) != NULL)
        {
            local_args[1] = local_args_temp[1];
            local_args[2] = local_args_temp[2];
            addr = CFG_RD_ADDR;
            sprintf(local_args[1], "0x%x", (unsigned int)load_addr);
            sprintf(local_args[2], "0x%x", (unsigned int)addr);
            do_bootm(cmdtp, 0, 3, local_args);
        }
        else
        {
            local_args[1] = NULL;
            local_args[2] = NULL;
            do_bootm(cmdtp, 0, 1, local_args);
        }

        return cnt;
    }

    return 0;
}


// This function is for loading image without header, i.e. squashfs
//
static int nand_load_image_wt_header(cmd_tbl_t *cmdtp, nand_info_t *nand,
               ulong offset, ulong addr, char *cmd)
{
    int r;
    char *ep, *s;
    size_t cnt;

    if(cmd != NULL)
    {
    	s = strchr(cmd, '.');
    	if (s != NULL &&
    	    (strcmp(s, ".jffs2") && strcmp(s, ".e") && strcmp(s, ".i"))) {
    		printf("Unknown nand load suffix '%s'\n", s);
    		return 1;
    	}
    }

    printf("\nLoading from %s, offset 0x%lx\n", nand->name, offset);

    if ((ep = getenv("squashfs_size")) != NULL)
    {
        cnt = simple_strtoul (ep, NULL, 16);
    }
    else
    {
        cnt = 0x200000;
    }

    r = nand_read_skip_bad(nand, offset, &cnt, (u_char *)addr);
    if (r)
    {
        puts("** Read error\n");
        return 1;
    }

    return cnt;
}

int nand_read_from_partition(unsigned char* buf, char* pname, unsigned int offset, unsigned int size)
{
    struct mtd_device *dev;
    struct part_info *part;
    u8 pnum;
    char env[32];

    if ((mtdparts_init() == 0) && (find_dev_and_part(pname, &dev, &pnum, &part) == 0))
    {
        sprintf(env, "0x%x", size);
        setenv("squashfs_size", env);
        nand_load_image_wt_header(NULL, &nand_info[dev->id->num], part->offset+offset, (ulong)buf, NULL);
        return 0;
    }

    return 1;
}


struct nand_fill_data
{
        nand_info_t *nand;
        int data_left;
        int data_offset;
        u_char *data_addr;
};

static int nand_fill(void *data)
{
	int recv_size, ret;
	struct nand_fill_data *ef = (struct nand_fill_data*)data;

	if (!ef->data_left)
		return -1;

	// Wait for previous block complete.
	recv_size = ef->nand->writesize;

	nand_async_read_wait_finish(ef->nand, ef->data_offset, recv_size, ef->data_addr);
	ef->data_left -= recv_size;
	ef->data_offset += recv_size;
	ef->data_addr += recv_size;

	// Now send command for next.
	if (ef->data_left)
	{
		ret = nand_async_read_start(ef->nand, ef->data_offset, recv_size, ef->data_addr);
		if (ret)
			printf("emmc_async_DMA_start %d ret %d\n", recv_size, ret);
	}

	return recv_size;
}
extern int unlzo_read(u8 *input, int in_len, int (*fill) (void *fill_data), void *fill_data, u8 *output, int *posp);

// Load & decompress LZO image at the same time.
//
// @param cnt      Size of compressed image on NAND.
// @param src_buf  Output buffer of source compressed image.
// @param out_buf  Output buffer of decompressed data
// @param advance  Number of bytes reads from NAND, include skipped length
// @param decomp_size  Decompressed data size.
// @param skip     Number of bytes to skip at start.
static int nand_read_unlzo(nand_info_t *nand, ulong offset, size_t *cnt, u_char *src_buf,
                           u_char *out_buf, size_t *advance, int *decomp_size, int skip)
{
    int nand_page_size;
    int ret, len, dma_size, extra, rval=0;
    struct nand_fill_data fdata;

    *advance = nand_async_read_init(nand, offset, *cnt);

    // align to page size, also check for non-aligned offset.
    nand_page_size = nand->writesize;
    extra = offset & (nand_page_size-1);
    len = (*cnt + extra + nand_page_size-1) & (~(nand_page_size-1));

    dma_size = nand_page_size;
    fdata.nand = nand;
    fdata.data_left = len - dma_size;
    fdata.data_offset = offset - extra + dma_size;
    fdata.data_addr = src_buf - extra + dma_size;

    // Read first block, must wait for first block
    ret = nand_async_read_start(nand, offset - extra, dma_size, src_buf);
    if (ret)
    {
        printf("nand_async_read_start done %d\n", ret);
    }

    nand_async_read_wait_finish(nand, offset - extra, dma_size, src_buf);

    // Check for LZO header, return fail if not found.
    if (*(int*)(src_buf+extra+skip) != 0x4f5a4c89)
    {
        rval = 1;
        goto out;
    }

    // Handle unaligned offset, use memcpy to remove the extra read data.
    if (extra)
    {
            memcpy(src_buf, src_buf + extra, dma_size - extra);
    }

    // Now, read next, don't wait.
    if (fdata.data_left)
    {
        nand_async_read_start(nand, fdata.data_offset, dma_size, fdata.data_addr);
    }

    ret = unlzo_read(src_buf+skip, dma_size-extra-skip, nand_fill, &fdata, out_buf, decomp_size);
    if (ret)
    {
        printf("block unlzo failed.. %d\n", ret);
        rval = 1;
        goto out;
    }

out:
    // Send finish command.
    nand_async_read_cleanup();
    return rval;
}

static int nand_load_image_lzo(cmd_tbl_t *cmdtp, nand_info_t *nand, struct part_info *part, char *cmd)
{
    int r;
    ulong addr, offset;
    size_t advance, cnt;
    int decomp_size;
    char *local_args[3];

    // Booting UIMAGE + LZO kernel.
    // !!!!!FIXME!!!!! We don't know the size of image, try part size.
    addr = 0x7fc0;
    cnt = part->size;
    offset = part->offset;
    r = nand_read_unlzo(nand, offset, &cnt,
                        (void*)CFG_RD_SRC_ADDR, (void*)addr, &advance, &decomp_size, 0x40);

	if(decomp_size + addr >= 0x1000000)
		{
			printf("\n The decomp kernel size is too large and will overlap with uboot !\n");
			while(1);
		}

    if (!r)
    {
        // Verify the image.
        nand_auth_image(nand, part, CFG_RD_SRC_ADDR, 0);

        // Good. Decompress work, boot it
        load_addr = addr;
        local_args[0] = cmd;
        local_args[1] = NULL;
        local_args[2] = NULL;
        do_bootm(cmdtp, 0, 1, local_args);
    }

    // read fail, go back and try normal flow
    return 0;
}


// load uImage and ramdisk from usb and setup usb boot env
int do_usbfileboot(cmd_tbl_t * cmdtp, int flag, int argc, char *argv[])
{
    char env[64];
    ulong addr;
    char *ep;
    size_t cnt;
    image_header_t *hdr;

    ulong dramaddr;
    char *local_args[7];
    char p0[512], p1[32], p2[32], p3[32];

    printf("USB File boot...\n");

    // usb start
    printf("### usb start\n");
    sprintf(p0, "usb");
    sprintf(p1, "start");
    local_args[0] = p0;
    local_args[1] = p1;
    if (do_usb(cmdtp, 0, 2, local_args) != 0)
    {
        return -1;
    }

    // load ramdisk
    dramaddr = CFG_RD_ADDR; //ramdisk addr

    printf("### fatload usb 0 0x%x uRamdisk\n", (unsigned int)dramaddr);
    sprintf(p0, "fatload");
    sprintf(p1, "usb");
    sprintf(p2, "0");
    sprintf(p3, "0x%x", (unsigned int)dramaddr);
    local_args[0] = p0;
    local_args[1] = p1;
    local_args[2] = p2;
    local_args[3] = p3;
    local_args[4] = "uRamdisk";

    if (do_fat_fsload(cmdtp, 0, 5, local_args) != 0)
    {
        return -1;
    }

    sprintf(env, "0x%x", CFG_RD_ADDR);
    setenv("initrd", env);

    // load uImage
    dramaddr = 0x8000; //uImage addr

    printf("### fatload usb 0 0x%x uImage\n", (unsigned int)dramaddr);
    sprintf(p0, "fatload");
    sprintf(p1, "usb");
    sprintf(p2, "0");
    sprintf(p3, "0x%x", (unsigned int)dramaddr);
    local_args[0] = p0;
    local_args[1] = p1;
    local_args[2] = p2;
    local_args[3] = p3;
    local_args[4] = "uImage";

    if (do_fat_fsload(cmdtp, 0, 5, local_args) != 0)
    {
        return -1;
    }

    // setup the bootargs enviroment
    sprintf(p0, "load");
    sprintf(p1, "usbfile_env");
    local_args[0] = p0;
    local_args[1] = p1;
    do_load(cmdtp, 0, 2, local_args);

    addr = dramaddr;

    switch (genimg_get_format ((void *)addr))
    {
    case IMAGE_FORMAT_LEGACY:
        hdr = (image_header_t *)addr;

        show_boot_progress (57);
        image_print_contents (hdr);

        cnt = image_get_image_size (hdr);
        break;

#if defined(CONFIG_FIT)
    case IMAGE_FORMAT_FIT:
        fit_hdr = (const void *)addr;
        puts ("Fit image detected...\n");

        cnt = fit_get_size (fit_hdr);
        break;
#endif

    default:
        show_boot_progress (-57);
        puts ("** Unknown image type\n");
        return 1;
    }

#if defined(CONFIG_FIT)
    /* This cannot be done earlier, we need complete FIT image in RAM first */
    if (genimg_get_format ((void *)addr) == IMAGE_FORMAT_FIT)
    {
        if (!fit_check_format (fit_hdr))
        {
            show_boot_progress (-150);
            puts ("** Bad FIT image format\n");
            return 1;
        }
        show_boot_progress (151);
        fit_print_contents (fit_hdr);
    }
#endif

    /* Loading ok, update default load address */

    load_addr = dramaddr;

    /* Check if we should attempt an auto-start */
    if (((ep = getenv("autostart")) != NULL) &&
        (strcmp(ep, "yes") == 0) &&
        (hdr->ih_type == IH_TYPE_KERNEL))
    {
        local_args[0] = argv[0];

        printf("Automatic boot of image at addr 0x%08lx ...\n", addr);

        #ifdef CC_SECURE_ROM_BOOT
        {
            printf("### fatload usb 0 0x%x sig\n", CFG_LOAD_ADDR);
            sprintf(p0, "fatload");
            sprintf(p1, "usb");
            sprintf(p2, "0");
            sprintf(p3, "0x%x", CFG_LOAD_ADDR);
            local_args[0] = p0;
            local_args[1] = p1;
            local_args[2] = p2;
            local_args[3] = p3;
            local_args[4] = "sig";

            if(do_fat_fsload(cmdtp, 0, 5, local_args) != 0)
            {
                return -1;
            }

            sig_authetication(addr, 0x40, (unsigned long *)CFG_LOAD_ADDR, (unsigned long *)(CFG_LOAD_ADDR+0x100));
        }
        #endif

        if ((ep = getenv("initrd")) != NULL)
        {
            addr = CFG_RD_ADDR;
            sprintf(local_args[1], "0x%x", (unsigned int)load_addr);
            sprintf(local_args[2], "0x%x", (unsigned int)addr);
            do_bootm(cmdtp, 0, 3, local_args);
        }
        else
        {
            local_args[1] = NULL;
            local_args[2] = NULL;
            do_bootm(cmdtp, 0, 1, local_args);
        }
        return cnt;
    }
    return 0;
}

#define WITH_PADDING_SIZE(size, page)    (((size)+(page)-1) & (~((page)-1)))

int do_nandboot(cmd_tbl_t * cmdtp, int flag, int argc, char * const argv[])
{
	char *boot_device = NULL;
    char *tmp, env[64];
	int idx;
	ulong addr, offset = 0;
    ulong rootfsoffset = 0;
#if defined(CONFIG_CMD_JFFS2) && defined(CONFIG_JFFS2_CMDLINE)
	struct mtd_device *dev;
	struct part_info *part;
	u8 pnum;

    #ifdef CC_SECURE_ROM_BOOT
    if(IsRunOnUsb(NULL, 0))
    {
        do_usbfileboot(cmdtp, flag, argc, argv);
    }
    #endif

	if (argc >= 2)
    {
        char *p = argv[1];
		if (!(str2long(p, &addr)) && (mtdparts_init() == 0) &&
		    (find_dev_and_part(p, &dev, &pnum, &part) == 0))
		{
            // Fix kernel address at 0x8000 to avoid from reallocation time
            addr = 0x8000;

			if (dev->id->type != MTD_DEV_TYPE_NAND)
            {
				puts("Not a NAND device\n");
				return 1;
			}

            if (argc > 3)
            {
                goto usage;
            }

			if (argc == 3)
            {
                if ((strcmp (argv[2], "rootfs") == 0) ||
                    (strcmp (argv[2], "rootfsA") == 0) ||
                    (strcmp (argv[2], "rootfsB") == 0))
                {
                    u8 part_num;
                    u64 part_size, part_offset;
                    if (find_partition(argv[2], &part_num, &part_size, &part_offset) == 0)
                    {
                        rootfsoffset = part_offset;
                    }
                    else
                    {
                        printf("\n** Ramdisk partition %s not found!\n", argv[2]);
                        show_boot_progress(-55);
                        return 1;
                    }
                }
                else
                {
                    goto usage;
                }
            }
            else if (argc == 2)
            {
                // android boot
                char *local_args[3];
                char local_args_temp[3][16];
                char env[32];
                struct boot_img_hdr hdr;
                int offset, magic_found, ret;
                size_t advance, cnt, sig_skip;
                int decomp_size;

                // 1. load header
                sprintf(env, "0x%x", sizeof(boot_img_hdr));
                setenv("squashfs_size", env);
                offset = part->offset;
                cnt = 2048;
                nand_read_skip_bad_ex(&nand_info[dev->id->num], offset, &cnt, (void*)CFG_RD_ADDR, &advance);
                memcpy((void*)&hdr, (void*)CFG_RD_ADDR, sizeof(boot_img_hdr));
                magic_found = !strncmp((char*)hdr.magic, BOOT_MAGIC, BOOT_MAGIC_SIZE);

                if (magic_found && strstr(argv[0], ".lzo"))
                {
                    // 2. load kernel, Parallel load & decompress LZO.
                    // kernel, must be uImage + LZO format.
                    offset += advance;
                    sig_skip = advance;
                    addr = 0x7fc0;
                    cnt = WITH_PADDING_SIZE(hdr.kernel_size, hdr.page_size);
                    ret = nand_read_unlzo(&nand_info[dev->id->num], offset, &cnt,
                                          (void*)CFG_RD_SRC_ADDR, (void*)addr, &advance, &decomp_size, 0x40);

					if(decomp_size + addr >= 0x1000000)
						{
							printf("\n The decomp kernel size is too large and will overlap with uboot !\n");
							while(1);
						}
                    if (ret)
                    {
                        // LZO read fail, read as normal uImage.
                        addr = 0x8000;
                        nand_read_skip_bad_ex(&nand_info[dev->id->num], offset, &cnt, (void*)addr, &advance);
                    }

                    // 3. verify the kernel.
                    nand_auth_image(&nand_info[dev->id->num], part, CFG_RD_SRC_ADDR, sig_skip);

                    // 4. ramdisk
                    offset += advance;
                    cnt = WITH_PADDING_SIZE(hdr.ramdisk_size, hdr.page_size);

                    nand_read_unlzo(&nand_info[dev->id->num], offset, &cnt,
                                    (void*)CFG_RD_SRC_ADDR, (void*)CFG_RD_ADDR, &advance, &decomp_size, 0);

					// 5. verify the ramdisk
					nand_auth_image_ex(part, CFG_RD_SRC_ADDR,  hdr.ramdisk_size, 256);

                    sprintf(env, "0x%x", CFG_RD_ADDR);
                    setenv("initrd", env);
                    sprintf(env, "0x%x", CFG_RD_ADDR+decomp_size);
                    setenv("initrd_end", env);

                    // 6. launch kernel
                    local_args[1] = local_args_temp[1];
                    local_args[2] = local_args_temp[2];
                    sprintf(local_args[1], "0x%lx", addr);
                    sprintf(local_args[2], "0x%x", CFG_RD_ADDR);
                    return do_bootm(cmdtp, 0, 3, local_args);
                }
                else if (magic_found)
                {
                    // 2. load kernel
                    offset += advance;
                    cnt = WITH_PADDING_SIZE(hdr.kernel_size, hdr.page_size);
                    nand_read_skip_bad_ex(&nand_info[dev->id->num], offset, &cnt, (void*)addr, &advance);

                    // 3. load ramdisk
                    offset += advance;
                    cnt = WITH_PADDING_SIZE(hdr.ramdisk_size, hdr.page_size);
                    nand_read_skip_bad_ex(&nand_info[dev->id->num], offset, &cnt, (void*)CFG_RD_ADDR, &advance);

                    sprintf(env, "0x%x", CFG_RD_ADDR);
                    setenv("initrd", env);
                    sprintf(env, "0x%x", CFG_RD_ADDR+hdr.ramdisk_size);
                    setenv("initrd_end", env);

                    // 4. launch kernel
                    local_args[1] = local_args_temp[1];
                    local_args[2] = local_args_temp[2];
                    sprintf(local_args[1], "0x%lx", addr);
                    sprintf(local_args[2], "0x%x", CFG_RD_ADDR);
                    return do_bootm(cmdtp, 0, 3, local_args);
                }
                else
                {
                    printf("\n no valid boot image!\n");
                    show_boot_progress(-55);
                    return 1;
                }
            }

            // Load rootfs
            if (((tmp = getenv("ramdisk")) != NULL) && (strcmp(tmp, "yes") == 0))
            {
                nand_load_image(cmdtp, &nand_info[dev->id->num],
                        rootfsoffset, CFG_RD_ADDR, argv[0]);

                sprintf(env, "0x%x", CFG_RD_ADDR);
                setenv("initrd", env);
            }
            else
            {
                setenv("initrd", NULL);
            }

            // Loading .lzo kernel if specified.
            tmp = strchr(argv[0], '.');
            if (tmp && !strcmp(tmp, ".lzo"))
                nand_load_image_lzo(cmdtp, &nand_info[dev->id->num], part, argv[0]);

            // Load kernel
            return nand_load_image(cmdtp, &nand_info[dev->id->num], part->offset, addr, argv[0]);
        }
    }
#endif

	show_boot_progress(52);
	switch (argc) {
	case 1:
		addr = CONFIG_SYS_LOAD_ADDR;
		boot_device = getenv("bootdevice");
		break;
	case 2:
		addr = simple_strtoul(argv[1], NULL, 16);
		boot_device = getenv("bootdevice");
		break;
	case 3:
		addr = simple_strtoul(argv[1], NULL, 16);
		boot_device = argv[2];
		break;
	case 4:
		addr = simple_strtoul(argv[1], NULL, 16);
		boot_device = argv[2];
		offset = simple_strtoul(argv[3], NULL, 16);
		break;

    case 5:
        addr = simple_strtoul(argv[1], NULL, 16);
        boot_device = argv[2];
        offset = simple_strtoul(argv[3], NULL, 16);
        rootfsoffset = simple_strtoul(argv[4], NULL, 16);
        break;

	default:
#if defined(CONFIG_CMD_JFFS2) && defined(CONFIG_JFFS2_CMDLINE)
usage:
#endif
		show_boot_progress(-53);
		return cmd_usage(cmdtp);
	}

	show_boot_progress(53);
	if (!boot_device) {
		puts("\n** No boot device **\n");
		show_boot_progress(-54);
		return 1;
	}
	show_boot_progress(54);

	idx = simple_strtoul(boot_device, NULL, 16);

	if (idx < 0 || idx >= CONFIG_SYS_MAX_NAND_DEVICE || !nand_info[idx].name) {
		printf("\n** Device %d not available\n", idx);
		show_boot_progress(-55);
		return 1;
	}
	show_boot_progress(55);
    if (rootfsoffset != 0)
    {
        nand_load_image(cmdtp, &nand_info[idx], rootfsoffset, addr + rootfsoffset, argv[0]);
        sprintf(env, "0x%x", (unsigned int)rootfsoffset);
        setenv("initrd", env);
    }
    else
    {
        setenv("initrd", NULL);
    }
	return nand_load_image(cmdtp, &nand_info[idx], offset, addr, argv[0]);
}

U_BOOT_CMD(nboot, 4, 1, do_nandboot,
	"boot from NAND device",
	"[.lzo] [partition] | [[[loadAddr] dev] offset]");

//-----------------------------------------------------------------------------
int nand_upgrade(u_long srcaddr, u_long noffset, u_long size, u_long *retnand)
{
    u_long count;
    u_char *pbuf;
    nand_erase_options_t opts;
    nand_info_t *mtd = &nand_info[nand_curr_device];
    pbuf = (u_char *)srcaddr;

    if (size % mtd->erasesize)
    {
        printf("nand size must be block alignment!\n");
        return -1;
    }

    *retnand = 0;
    count = mtd->erasesize;
    while ((int)size > 0)
    {
        *retnand += count;

        if (nand_block_isbad(mtd, noffset) != 0)
        {
            printf("bad bloack at 0x%x, skip\n", (unsigned int)noffset);
            noffset += count;
            continue;
        }

        memset(&opts, 0, sizeof(opts));
        opts.offset = noffset;
        opts.length = mtd->erasesize;
        opts.jffs2  = 0;
        opts.quiet  = 0;

        if (nand_erase_opts(mtd, &opts) != 0)
        {
            printf("erase 0x%x fail, skip\n", (unsigned int)noffset);
            noffset += count;
            continue;
        }

        if (nand_write(mtd, noffset, (size_t *)&count, (u_char *)pbuf) != 0)
        {
            printf("write 0x%x fail, skip\n", (unsigned int)noffset);

            // Mark bad
            nand_block_markbad(mtd, noffset);
            noffset += count;
            continue;
        }

        printf("write 0x%x ok\n", (unsigned int)noffset);
        size -= count;
        noffset += count;
        pbuf += count;
    }

    return 0;
}


