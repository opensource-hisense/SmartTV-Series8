/*
 * WPA Supplicant / main() function for UNIX like OSes and MinGW
 * Copyright (c) 2003-2013, Jouni Malinen <j@w1.fi>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

#include "includes.h"
#ifdef __linux__
#include <fcntl.h>
#endif /* __linux__ */

#include "common.h"
#include "wpa_supplicant_i.h"
#include "driver_i.h"
#include "p2p_supplicant.h"
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <time.h>

//#define CHECK_WLNA0_UP

#ifdef CHECK_WLNA0_UP

#define MAX_TRY_TIMES	15
#define WAIT_INTERVAL	1			/* unit: us, equal to 100ms*/
//#define DRIVER_OK "/proc/probe_complete"

static int wifi_driver_check()
{
	char *arg[4];
	pid_t pid;
	int status;
	int ret = -1;
		
	arg[0] = "ifconfig";
	arg[1] = "wlan0";
	arg[2] = "up";
	arg[3] = NULL;

	pid = fork();
	if (0 == pid)
	{
		/*
		close(STDOUT_FILENO);
		close(STDERR_FILENO);
		*/
		if (0 != execv("/bin/ifconfig", arg))
		{
			return -1;
		}
	}
	else if (pid > 0)
	{
		do {
			if (-1 == waitpid(pid, &status, 0)) 
			{
				printf("waitpid failed\n");
				ret = -1;	
				break;
			}

			ret = WEXITSTATUS(status);
			if (0 == ret)
			{
				break;
			}
		} while (!WIFEXITED(status) && !WIFSIGNALED(status));
	}

	return ret;
}

static int wait4driver_ok()
{
	int cnt;
	int ret = -1;

	for (cnt = 0; cnt < MAX_TRY_TIMES; cnt++)
	{
		ret = wifi_driver_check();
		if (0 == ret)
		{
			break;
		}
		sleep(WAIT_INTERVAL);
	}

	if (0 == ret)
	{
		printf("driver ok: wait %ds\n", cnt);
	}
	else
	{
		printf("driver failed: wait time out %dms\n", cnt * WAIT_INTERVAL);
	}

	return ret;
}
#endif

static void usage(void)
{
	int i;
	printf("%s\n\n%s\n"
	       "usage:\n"
	       "  wpa_supplicant [-BddhKLqq"
#ifdef CONFIG_DEBUG_SYSLOG
	       "s"
#endif /* CONFIG_DEBUG_SYSLOG */
	       "t"
#ifdef CONFIG_DBUS
	       "u"
#endif /* CONFIG_DBUS */
	       "vW] [-P<pid file>] "
	       "[-g<global ctrl>] \\\n"
	       "        [-G<group>] \\\n"
	       "        -i<ifname> -c<config file> [-C<ctrl>] [-D<driver>] "
	       "[-p<driver_param>] \\\n"
	       "        [-b<br_ifname>] [-e<entropy file>]"
#ifdef CONFIG_DEBUG_FILE
	       " [-f<debug file>]"
#endif /* CONFIG_DEBUG_FILE */
	       " \\\n"
	       "        [-o<override driver>] [-O<override ctrl>] \\\n"
	       "        [-N -i<ifname> -c<conf> [-C<ctrl>] "
	       "[-D<driver>] \\\n"
#ifdef CONFIG_P2P
	       "        [-m<P2P Device config file>] \\\n"
#endif /* CONFIG_P2P */
	       "        [-p<driver_param>] [-b<br_ifname>] [-I<config file>] "
	       "...]\n"
	       "\n"
	       "drivers:\n",
	       wpa_supplicant_version, wpa_supplicant_license);

	for (i = 0; wpa_drivers[i]; i++) {
		printf("  %s = %s\n",
		       wpa_drivers[i]->name,
		       wpa_drivers[i]->desc);
	}

#ifndef CONFIG_NO_STDOUT_DEBUG
	printf("options:\n"
	       "  -b = optional bridge interface name\n"
	       "  -B = run daemon in the background\n"
	       "  -c = Configuration file\n"
	       "  -C = ctrl_interface parameter (only used if -c is not)\n"
	       "  -i = interface name\n"
	       "  -I = additional configuration file\n"
	       "  -d = increase debugging verbosity (-dd even more)\n"
	       "  -D = driver name (can be multiple drivers: nl80211,wext)\n"
	       "  -e = entropy file\n");
#ifdef CONFIG_DEBUG_FILE
	printf("  -f = log output to debug file instead of stdout\n");
#endif /* CONFIG_DEBUG_FILE */
	printf("  -g = global ctrl_interface\n"
	       "  -G = global ctrl_interface group\n"
	       "  -K = include keys (passwords, etc.) in debug output\n");
#ifdef CONFIG_DEBUG_SYSLOG
	printf("  -s = log output to syslog instead of stdout\n");
#endif /* CONFIG_DEBUG_SYSLOG */
#ifdef CONFIG_DEBUG_LINUX_TRACING
	printf("  -T = record to Linux tracing in addition to logging\n");
	printf("       (records all messages regardless of debug verbosity)\n");
#endif /* CONFIG_DEBUG_LINUX_TRACING */
	printf("  -t = include timestamp in debug messages\n"
	       "  -h = show this help text\n"
	       "  -L = show license (BSD)\n"
	       "  -o = override driver parameter for new interfaces\n"
	       "  -O = override ctrl_interface parameter for new interfaces\n"
	       "  -p = driver parameters\n"
	       "  -P = PID file\n"
	       "  -q = decrease debugging verbosity (-qq even less)\n");
#ifdef CONFIG_DBUS
	printf("  -u = enable DBus control interface\n");
#endif /* CONFIG_DBUS */
	printf("  -v = show version\n"
	       "  -W = wait for a control interface monitor before starting\n"
#ifdef CONFIG_P2P
	       "  -m = Configuration file for the P2P Device interface\n"
#endif /* CONFIG_P2P */
	       "  -N = start describing new interface\n");

	printf("example:\n"
	       "  wpa_supplicant -D%s -iwlan0 -c/etc/wpa_supplicant.conf\n",
	       wpa_drivers[0] ? wpa_drivers[0]->name : "nl80211");
#endif /* CONFIG_NO_STDOUT_DEBUG */
}

#define MAX_DEBUG_FILE_LEN	128
char g_debug_file_name[MAX_DEBUG_FILE_LEN];

#if 0
static void license(void)
{
#ifndef CONFIG_NO_STDOUT_DEBUG
	printf("%s\n\n%s%s%s%s%s\n",
	       wpa_supplicant_version,
	       wpa_supplicant_full_license1,
	       wpa_supplicant_full_license2,
	       wpa_supplicant_full_license3,
	       wpa_supplicant_full_license4,
	       wpa_supplicant_full_license5);
#endif /* CONFIG_NO_STDOUT_DEBUG */
}
#endif


static void wpa_supplicant_fd_workaround(int start)
{
#ifdef __linux__
	static int fd[3] = { -1, -1, -1 };
	int i;
	/* When started from pcmcia-cs scripts, wpa_supplicant might start with
	 * fd 0, 1, and 2 closed. This will cause some issues because many
	 * places in wpa_supplicant are still printing out to stdout. As a
	 * workaround, make sure that fd's 0, 1, and 2 are not used for other
	 * sockets. */
	if (start) {
		for (i = 0; i < 3; i++) {
			fd[i] = open("/dev/null", O_RDWR);
			if (fd[i] > 2) {
				close(fd[i]);
				fd[i] = -1;
				break;
			}
		}
	} else {
		for (i = 0; i < 3; i++) {
			if (fd[i] >= 0) {
				close(fd[i]);
				fd[i] = -1;
			}
		}
	}
#endif /* __linux__ */
}

#define MAX_LEN			128
int create_p2p0_link()
{
	char *link_name = "/var/run/wpa_supplicant/p2p0";
	char *src_name = "/var/run/wpa_supplicant/p2p-dev-wlan0";

	if (0 ==access(src_name, F_OK))
	{
		if (0 == access(link_name, F_OK))
		{
			remove(link_name);	
		}
			
		symlink(src_name, link_name);
	    wpa_printf(MSG_DEBUG, "********create the link successfully ***************\n");
	}

	return 0;
}

#define DEBUG_FILE  "/3rd_rw/brcm/supplicant-debug-log.txt" 
#define DEBUG_LEVEL "/data/log_level"
int main(int argc, char *argv[])
{
	//int c, i;
	int i;
	struct wpa_interface *ifaces, *iface;
	int iface_count, exitcode = -1;
	struct wpa_params params;
	struct wpa_global *global;
	int count;
        int fd, n;
        char buf[128];
	if (os_program_init())
		return -1;

	os_memset(&params, 0, sizeof(params));

	os_memset(buf, 0, sizeof(buf));
	fd = open(DEBUG_LEVEL, O_RDONLY);
	if (fd < 0) {
		printf("fail to open /data/debug_level file, fd = %d\n", fd);
		params.wpa_debug_level = MSG_ERROR;
	} else {
		n = read(fd, buf, sizeof(buf));
		if (n < 0) {
			printf("read failed,n = %d\n", n);
		} else {
			printf("buf = %s\n", buf);
			if(!os_strcmp(buf, "debug\n")) {
				printf("debug\n");
				params.wpa_debug_level = MSG_DEBUG;
			}else if(!os_strcmp(buf, "info\n")) {
				printf("info\n");
				params.wpa_debug_level = MSG_INFO;
			} else if(!os_strcmp(buf, "dump\n")) {
				printf("dump\n");
				params.wpa_debug_level = MSG_MSGDUMP;
			} else {
				printf("no match, set MSG_ERROR\n");
				params.wpa_debug_level = MSG_ERROR;
			}
		}
		close(fd);
	}

	iface = ifaces = os_zalloc(sizeof(struct wpa_interface));
	if (ifaces == NULL)
		return -1;
	iface_count = 1;

	wpa_supplicant_fd_workaround(1);

#if 0
	for (;;) {
		c = getopt(argc, argv,
			   "b:Bc:C:D:de:f:g:G:hi:I:KLm:No:O:p:P:qsTtuvW");
		if (c < 0)
			break;
		switch (c) {
		case 'b':
			iface->bridge_ifname = optarg;
			break;
		case 'B':
			params.daemonize++;
			break;
		case 'c':
			iface->confname = optarg;
			break;
		case 'C':
			iface->ctrl_interface = optarg;
			break;
		case 'D':
			iface->driver = optarg;
			break;
		case 'd':
#ifdef CONFIG_NO_STDOUT_DEBUG
			printf("Debugging disabled with "
			       "CONFIG_NO_STDOUT_DEBUG=y build time "
			       "option.\n");
			goto out;
#else /* CONFIG_NO_STDOUT_DEBUG */
			params.wpa_debug_level--;
			break;
#endif /* CONFIG_NO_STDOUT_DEBUG */
		case 'e':
			params.entropy_file = optarg;
			break;
#ifdef CONFIG_DEBUG_FILE
		case 'f':
			params.wpa_debug_file_path = optarg;
			break;
#endif /* CONFIG_DEBUG_FILE */
		case 'g':
			params.ctrl_interface = optarg;
			break;
		case 'G':
			params.ctrl_interface_group = optarg;
			break;
		case 'h':
			usage();
			exitcode = 0;
			goto out;
		case 'i':
			iface->ifname = optarg;
			break;
		case 'I':
			iface->confanother = optarg;
			break;
		case 'K':
			params.wpa_debug_show_keys++;
			break;
		case 'L':
			license();
			exitcode = 0;
			goto out;
#ifdef CONFIG_P2P
		case 'm':
			iface->conf_p2p_dev = optarg;
			break;
#endif /* CONFIG_P2P */
		case 'o':
			params.override_driver = optarg;
			break;
		case 'O':
			params.override_ctrl_interface = optarg;
			break;
		case 'p':
			iface->driver_param = optarg;
			break;
		case 'P':
			os_free(params.pid_file);
			params.pid_file = os_rel2abs_path(optarg);
			break;
		case 'q':
			params.wpa_debug_level++;
			break;
#ifdef CONFIG_DEBUG_SYSLOG
		case 's':
			params.wpa_debug_syslog++;
			break;
#endif /* CONFIG_DEBUG_SYSLOG */
#ifdef CONFIG_DEBUG_LINUX_TRACING
		case 'T':
			params.wpa_debug_tracing++;
			break;
#endif /* CONFIG_DEBUG_LINUX_TRACING */
		case 't':
			params.wpa_debug_timestamp++;
			break;
#ifdef CONFIG_DBUS
		case 'u':
			params.dbus_ctrl_interface = 1;
			break;
#endif /* CONFIG_DBUS */
		case 'v':
			printf("%s\n", wpa_supplicant_version);
			exitcode = 0;
			goto out;
		case 'W':
			params.wait_for_monitor++;
			break;
		case 'N':
			iface_count++;
			iface = os_realloc_array(ifaces, iface_count,
						 sizeof(struct wpa_interface));
			if (iface == NULL)
				goto out;
			ifaces = iface;
			iface = &ifaces[iface_count - 1]; 
			os_memset(iface, 0, sizeof(*iface));
			break;
		default:
			usage();
			exitcode = 0;
			goto out;
		}
	}
#endif

	//2 case 'c':
	//iface->confname = "/etc/wifi/wpa_supplicant.conf";
	iface->confname = "/3rd/bin/wpa_supplicant/wpa.conf";

	//3 case 'D':
	iface->driver = "nl80211";

	//4 case 'e':
	params.entropy_file = "/3rd_rw/brcm/entropy.bin";

	//5 case 'g':
	//params.ctrl_interface = "/3rd_rw/wpa_supplicant/wlan0";
	//6 case 'i':
	iface->ifname = "wlan0";
	//7 case m:
	//iface->conf_p2p_dev = "/etc/wifi/p2p_supplicant.conf";
	iface->conf_p2p_dev = "/3rd/bin/wpa_supplicant/p2p.conf";
	//8 case 'O':
	params.override_ctrl_interface = "/var/run/wpa_supplicant";
	//9 case 'p':
	iface->driver_param = "use_p2p_group_interface=1p2p_device=1use_ multi_chan_concurrent=1";

#ifdef CONFIG_DEBUG_FILE
	//case 'd':
	params.wpa_debug_level -= 2;
	//case 'f':
	remove(DEBUG_FILE);
	params.wpa_debug_file_path = DEBUG_FILE;

	//10 case 't':
	params.wpa_debug_timestamp++;
#else
	//case 'q':
	//params.wpa_debug_level += 2;
#endif /* CONFIG_DEBUG_FILE */

	exitcode = 0;
	//Hisense: juweiming add, check whether driver is OK
	for(count = 0; count < 20; count++) {
		if(access("/proc/probe_complete", F_OK) == 0) {
			break;
		} else {
			os_sleep(0, 500000);
		}
	}
	global = wpa_supplicant_init(&params);
	if (global == NULL) {
		wpa_printf(MSG_ERROR, "Failed to initialize wpa_supplicant");
		exitcode = -1;
		goto out;
	} else {
		wpa_printf(MSG_INFO, "Successfully initialized "
			   "wpa_supplicant");
	}

	for (i = 0; exitcode == 0 && i < iface_count; i++) {
		struct wpa_supplicant *wpa_s;

		if ((ifaces[i].confname == NULL &&
		     ifaces[i].ctrl_interface == NULL) ||
		    ifaces[i].ifname == NULL) {
			if (iface_count == 1 && (params.ctrl_interface ||
						 params.dbus_ctrl_interface))
				break;
			usage();
			exitcode = -1;
			break;
		}
		wpa_s = wpa_supplicant_add_iface(global, &ifaces[i]);
		if (wpa_s == NULL) {
			exitcode = -1;
			break;
		}
#ifdef CONFIG_P2P
		if (wpa_s->global->p2p == NULL &&
		    (wpa_s->drv_flags &
		     WPA_DRIVER_FLAGS_DEDICATED_P2P_DEVICE) &&
		    wpas_p2p_add_p2pdev_interface(wpa_s) < 0)
			exitcode = -1;
#endif /* CONFIG_P2P */
	}

	if (0 != create_p2p0_link())
	{
		wpa_printf(MSG_ERROR, "Failed to create p2p0 link");
		/* Not return, because p2p0 only has effect on p2p, sta will be ok*/
	}
	
	if (exitcode == 0)
		exitcode = wpa_supplicant_run(global);

	wpa_supplicant_deinit(global);

out:
	wpa_supplicant_fd_workaround(0);
	os_free(ifaces);
	os_free(params.pid_file);

	os_program_deinit();

	return exitcode;
}
