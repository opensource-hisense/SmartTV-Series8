/* Copyright (C) 1998, 2001 Free Software Foundation, Inc.

   This file is part of GNU Inetutils.

   GNU Inetutils is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   GNU Inetutils is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GNU Inetutils; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA. */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <signal.h>

#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <netinet/ip.h>
/*#include <netinet/ip_icmp.h> -- deliberately not including this */
#include <arpa/inet.h>
#include <icmp.h>

int
icmp_timestamp_encode (u_char *buffer, size_t bufsize, int ident, int seqno)
{
  icmphdr_t *icmp;
  struct timeval tv;
  unsigned long v;

  if (bufsize < 20)
    return -1;

  gettimeofday (&tv, NULL);
  v = htonl ((tv.tv_sec % 86400) * 1000 + tv.tv_usec / 1000);

  icmp = (icmphdr_t *)buffer;
  icmp->icmp_otime = v;
  icmp->icmp_rtime = v;
  icmp->icmp_ttime = v;
  icmp_generic_encode (buffer, bufsize, ICMP_TIMESTAMP, ident, seqno);
  return 0;
}

