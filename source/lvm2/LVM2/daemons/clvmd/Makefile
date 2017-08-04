#
# Copyright (C) 2004 Red Hat, Inc. All rights reserved.
#
# This file is part of LVM2.
#
# This copyrighted material is made available to anyone wishing to use,
# modify, copy, or redistribute it subject to the terms and conditions
# of the GNU General Public License v.2.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software Foundation,
# Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

srcdir = .
top_srcdir = ../..
top_builddir = ../..

CMAN_LIBS = 
CMAN_CFLAGS = 
CONFDB_LIBS = 
CONFDB_CFLAGS = 
CPG_LIBS = 
CPG_CFLAGS = 
DLM_LIBS = 
DLM_CFLAGS = 
QUORUM_LIBS = 
QUORUM_CFLAGS = 
SALCK_LIBS = 
SALCK_CFLAGS = 

SOURCES = \
	clvmd-command.c  \
	clvmd.c          \
	lvm-functions.c  \
	refresh_clvmd.c

ifeq ("no", "yes")
	DEFS += -DDEBUG
endif

ifneq (,$(findstring cman,, "none,"))
	SOURCES += clvmd-cman.c
	LMLIBS += $(CMAN_LIBS) $(CONFDB_LIBS) $(DLM_LIBS)
	CFLAGS += $(CMAN_CFLAGS) $(CONFDB_CFLAGS) $(DLM_CFLAGS)
	DEFS += -DUSE_CMAN
endif

ifneq (,$(findstring openais,, "none,"))
	SOURCES += clvmd-openais.c
	LMLIBS += $(CONFDB_LIBS) $(CPG_LIBS) $(SALCK_LIBS)
	CFLAGS += $(CONFDB_CFLAGS) $(CPG_CFLAGS) $(SALCK_CFLAGS)
	DEFS += -DUSE_OPENAIS
endif

ifneq (,$(findstring corosync,, "none,"))
	SOURCES += clvmd-corosync.c
	LMLIBS += $(CONFDB_LIBS) $(CPG_LIBS) $(DLM_LIBS) $(QUORUM_LIBS)
	CFLAGS += $(CONFDB_CFLAGS) $(CPG_CFLAGS) $(DLM_CFLAGS) $(QUORUM_CFLAGS)
	DEFS += -DUSE_COROSYNC
endif

ifneq (,$(findstring singlenode,, &quot;none,&quot;))
	SOURCES += clvmd-singlenode.c
	DEFS += -DUSE_SINGLENODE
endif

ifeq ($(MAKECMDGOALS),distclean)
	SOURCES += clvmd-cman.c
	SOURCES += clvmd-openais.c
	SOURCES += clvmd-corosync.c
	SOURCES += clvmd-singlenode.c
endif

TARGETS = \
	clvmd

LVMLIBS = $(LVMINTERNAL_LIBS)

ifeq ("", "yes")
	LVMLIBS += -ldevmapper-event
endif
 
include $(top_builddir)/make.tmpl

LVMLIBS += -ldevmapper
LIBS += $(PTHREAD_LIBS)

DEFS += -D_REENTRANT
CFLAGS += -fno-strict-aliasing

INSTALL_TARGETS = \
	install_clvmd

clvmd: $(OBJECTS) $(top_builddir)/lib/liblvm-internal.a
	$(CC) $(CFLAGS) $(LDFLAGS) -o clvmd $(OBJECTS) \
		$(LVMLIBS) $(LMLIBS) $(LIBS)

.PHONY: install_clvmd

install_clvmd: $(TARGETS)
	$(INSTALL_PROGRAM) -D clvmd $(usrsbindir)/clvmd

install: $(INSTALL_TARGETS)

install_cluster: $(INSTALL_TARGETS)
