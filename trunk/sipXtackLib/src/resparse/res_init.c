/*
* Copyright (c) 1985, 1989, 1993
*    The Regents of the University of California.  All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions
* are met:
* 1. Redistributions of source code must retain the above copyright
*    notice, this list of conditions and the following disclaimer.
* 2. Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in the
*    documentation and/or other materials provided with the distribution.
* 3. All advertising materials mentioning features or use of this software
*    must display the following acknowledgement:
*      This product includes software developed by the University of
*      California, Berkeley and its contributors.
* 4. Neither the name of the University nor the names of its contributors
*    may be used to endorse or promote products derived from this software
*    without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
* ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
* FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
* OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
* LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
* OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
* SUCH DAMAGE.
*/

/*
* Portions Copyright (c) 1993 by Digital Equipment Corporation.
*
* Permission to use, copy, modify, and distribute this software for any
* purpose with or without fee is hereby granted, provided that the above
* copyright notice and this permission notice appear in all copies, and that
* the name of Digital Equipment Corporation not be used in advertising or
* publicity pertaining to distribution of the document or software without
* specific, written prior permission.
*
* THE SOFTWARE IS PROVIDED "AS IS" AND DIGITAL EQUIPMENT CORP. DISCLAIMS ALL
* WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES
* OF MERCHANTABILITY AND FITNESS.   IN NO EVENT SHALL DIGITAL EQUIPMENT
* CORPORATION BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
* DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
* PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS
* ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
* SOFTWARE.
*/

/*
* Portions Copyright (c) 1996 by Internet Software Consortium.
*
* Permission to use, copy, modify, and distribute this software for any
* purpose with or without fee is hereby granted, provided that the above
* copyright notice and this permission notice appear in all copies.
*
* THE SOFTWARE IS PROVIDED "AS IS" AND INTERNET SOFTWARE CONSORTIUM DISCLAIMS
* ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES
* OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL INTERNET SOFTWARE
* CONSORTIUM BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
* DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
* PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS
* ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
* SOFTWARE.
*/

#ifndef __pingtel_on_posix__
#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)res_init.c  8.1 (Berkeley) 6/7/93";
static char orig_rcsid[] = "From: Id: res_init.c,v 8.7 1996/11/18 09:10:04 vixie Exp $";
static char rcsid[] = "";
#endif /* LIBC_SCCS and not lint */

#ifdef WINCE
#   include <types.h>
#else
#   include <sys/types.h>
#endif

#include <time.h>

/* Reordered includes and separated into win/vx --GAT */
#if defined(_WIN32)
#       include <resparse/wnt/sys/param.h>
#       include <winsock2.h>
#       include <resparse/wnt/netinet/in.h>
#       include <resparse/wnt/arpa/inet.h>
#       include <resparse/wnt/arpa/nameser.h>
#       include <resparse/wnt/resolv/resolv.h>
#ifndef WINCE
#       include <process.h>
#endif
#       include "resparse/wnt/inet_aton.h"
#       include "os/wnt/getWindowsDNSServers.h"
//#     include <iphlpapi.h>
#elif defined(_VXWORKS)
#       include <sys/socket.h>
#       include <hostLib.h>
#       include <netinet/in.h>
#       include <arpa/inet.h>
#       include <sys/times.h>
/* Use local lnameser.h for info missing from VxWorks version --GAT */
/* lnameser.h is a subset of resparse/wnt/arpa/nameser.h                */
#       include <resolv/nameser.h>
#       include <resparse/vxw/arpa/lnameser.h>
/* Use local lresolv.h for info missing from VxWorks version --GAT */
/* lresolv.h is a subset of resparse/wnt/resolv/resolv.h               */
#       include <resolv/resolv.h>
#       include <resparse/vxw/resolv/lresolv.h>
#       include <unistd.h>
#       include <taskLib.h> /* Needed for taskIdSelf --GAT */
#       include <inetLib.h> /* Needed for inet_aton --GAT */
#endif
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "resparse/res_config.h"
#include <os/OsDefs.h>

/* defined in OsSocket */
extern unsigned long osSocketGetDefaultBindAddress();

#ifndef _VXWORKS /* [ */
static void res_setoptions __P((char *, char *));

#ifdef RESOLVSORT
static const char sort_mask[] = "/&";
#define ISSORTMASK(ch) (strchr(sort_mask, ch) != NULL)
#if defined(_WIN32)
static u_int32 net_mask __P((struct in_addr));
#elif defined(_VXWORKS)
static uint32_t net_mask __P((struct in_addr));
#endif
#endif
#endif /* _VXWORKS ] */

#if !defined(isascii) /* XXX - could be a function */
# define isascii(c) (!(c & 0200))
#endif

/*
* Resolver state default settings.
*/

struct __res_state _sip_res /* Changed to avoiding clash with previoud defn*/
# if defined(__BIND_RES_TEXT)
   = { RES_TIMEOUT, }      /* Motorola, et al. */
# endif
;

u_int res_random_id()
{
   struct timeval now;

   now.tv_sec = (long)time((time_t*) &now.tv_sec);
   now.tv_usec = 0;
#if defined(_WIN32)
   return (0xffff & (now.tv_sec ^ now.tv_usec ^ _getpid()));
#elif defined(_VXWORKS)
   return (0xffff & (now.tv_sec ^ now.tv_usec ^ taskIdSelf()));
#endif
}

/*
* Added default VxWorks version of res_init to return an error, since
* resolvInit is currently being called by the kernal.  If that call is
* ever removed, must modify the res_init function to work within VxWorks
* to initialize the namesevers, domain, search and other parameters.
* This default routine only supplied to prevent res_init, as currently
* coded, from being used in VxWorks. --GAT
*/
#if defined(_VXWORKS)
int res_init()
{
   printf("Should already be initialized by resolvInit in kernel.\n");
   return (-1);
}
#elif defined(_WIN32)
/*
* Set up default settings.  If the configuration file exist, the values
* there will have precedence.  Otherwise, the server address is set to
* INADDR_ANY and the default domain name comes from the gethostname().
*
* An interrim version of this code (BIND 4.9, pre-4.4BSD) used 127.0.0.1
* rather than INADDR_ANY ("0.0.0.0") as the default name server address
* since it was noted that INADDR_ANY actually meant ``the first interface
* you "ifconfig"'d at boot time'' and if this was a SLIP or PPP interface,
* it had to be "up" in order for you to reach your own name server.  It
* was later decided that since the recommended practice is to always
* install local static routes through 127.0.0.1 for all your network
* interfaces, that we could solve this problem without a code change.
*
* The configuration file should always be used, since it is the only way
* to specify a default domain.  If you are running a server on your local
* machine, you should say "nameserver 0.0.0.0" or "nameserver 127.0.0.1"
* in the configuration file.
*
* Return 0 if completes successfully, -1 on error
*/

int res_init()
{
   char* szBuff = NULL;
   int ret = 0;
   unsigned long addr = osSocketGetDefaultBindAddress();
   struct in_addr naddr;

   naddr.S_un.S_addr = addr;
   szBuff = inet_ntoa(naddr);
   ret = res_init_ip(szBuff);

   return ret;
}

int res_init_ip(const char* localIp)
{
   char DNSServers[6][MAXIPLEN];
   int dnsSvrCnt;
   register FILE *fp;
   register char *cp, **pp;
   register int n;
   char buf[MAXDNAME];
   int nserv = 0;    /* number of nameserver records read from file */
   int haveenv = 0;
   int havesearch = 0;
#ifdef RESOLVSORT
   int nsort = 0;
   char *net;
#endif
#ifndef RFC1535
   int dots;
#endif
   static char szLocalIp[32] = "";
   char* szBuff = NULL;
   unsigned long defaultAddr = osSocketGetDefaultBindAddress();
   struct in_addr naddr;

   if (localIp == NULL || localIp[0] == 0)
   {
      // localIp Not specified - use default
      naddr.S_un.S_addr = defaultAddr;
      szBuff = inet_ntoa(naddr);
      strncpy(szLocalIp, szBuff, 32);
   }
   else
   {
      if (strcmp(szLocalIp, localIp) == 0)
      {
         return 0; // no need to init again if the ip address is the same
         // as the last call to this function.
      }
      strncpy(szLocalIp, localIp, 32);
   }

   /*
   * These three fields used to be statically initialized.  This made
   * it hard to use this code in a shared library.  It is necessary,
   * now that we're doing dynamic initialization here, that we preserve
   * the old semantics: if an application modifies one of these three
   * fields of _res before res_init() is called, res_init() will not
   * alter them.  Of course, if an application is setting them to
   * _zero_ before calling res_init(), hoping to override what used
   * to be the static default, we can't detect it and unexpected results
   * will follow.  Zero for any of these fields would make no sense,
   * so one can safely assume that the applications were already getting
   * unexpected results.
   *
   * _res.options is tricky since some apps were known to diddle the bits
   * before res_init() was first called. We can't replicate that semantic
   * with dynamic initialization (they may have turned bits off that are
   * set in RES_DEFAULT).  Our solution is to declare such applications
   * "broken".  They could fool us by setting RES_INIT but none do (yet).
   */
   if (!_sip_res.retrans)
      _sip_res.retrans = RES_TIMEOUT;
   if (!_sip_res.retry)
      _sip_res.retry = 4;
   if (!(_sip_res.options & RES_INIT))
      _sip_res.options = RES_DEFAULT;

   /*
   * This one used to initialize implicitly to zero, so unless the app
   * has set it to something in particular, we can randomize it now.
   */
   if (!_sip_res.id)
      _sip_res.id = res_random_id();

#ifdef USELOOPBACK
#if defined(_WIN32)  /* added win32 version for loopback --GAT */
   _sip_res.nsaddr.sin_addr.s_addr  = INADDR_LOOPBACK;
#elif defined(_VXWORKS)
   _sip_res.nsaddr.sin_addr = inet_makeaddr(IN_LOOPBACKNET, 1);
#endif
#else
   _sip_res.nsaddr.sin_addr.s_addr = osSocketGetDefaultBindAddress();
#endif
   _sip_res.nsaddr.sin_family = AF_INET;
   _sip_res.nsaddr.sin_port = htons(NAMESERVER_PORT);
   _sip_res.nscount = 1;
   _sip_res.ndots = 1;
   _sip_res.pfcode = 0;

   if (dnsSvrCnt = getWindowsDNSServers(DNSServers, 6, szLocalIp))
   {
      struct in_addr a;
      int nservIndex = 0;
      while((nservIndex < dnsSvrCnt) &&
         (nserv < MAXNS)) // do not insert more than
         // the list will hold
      {
         if (IS_INET_RETURN_OK( inet_aton(DNSServers[nservIndex], &a)) )
         {
            _sip_res.nsaddr_list[nserv].sin_addr = a;
            _sip_res.nsaddr_list[nserv].sin_family = AF_INET;
            _sip_res.nsaddr_list[nserv].sin_port =
               htons(NAMESERVER_PORT);
            nserv++;
         }
         else
         {
            osPrintf("invalid DNS Server %d of %d\n",
               nserv, dnsSvrCnt);
         }
         nservIndex++;
      }
   }
   /* Allow user to override the local domain definition */
#if !defined(WINCE)
   if (/*issetugid() == 0 && */(cp = getenv("LOCALDOMAIN")) != NULL) {
      (void)strncpy(_sip_res.defdname, cp, sizeof(_sip_res.defdname) - 1);
      _sip_res.defdname[sizeof(_sip_res.defdname) - 1] = '\0';
      haveenv++;

      /*
      * Set search list to be blank-separated strings
      * from rest of env value.  Permits users of LOCALDOMAIN
      * to still have a search list, and anyone to set the
      * one that they want to use as an individual (even more
      * important now that the rfc1535 stuff restricts searches)
      */
      cp = _sip_res.defdname;
      pp = _sip_res.dnsrch;
      *pp++ = cp;
      for (n = 0; *cp && pp < _sip_res.dnsrch + MAXDNSRCH; cp++) {
         if (*cp == '\n')        /* silly backwards compat */
            break;
         else if (*cp == ' ' || *cp == '\t') {
            *cp = 0;
            n = 1;
         } else if (n) {
            *pp++ = cp;
            n = 0;
            havesearch = 1;
         }
      }
      /* null terminate last domain if there are excess */
      while (*cp != '\0' && *cp != ' ' && *cp != '\t' && *cp != '\n')
         cp++;
      *cp = '\0';
      *pp++ = 0;
   }
#endif
#define MATCH(line, name) \
   (!strncmp(line, name, sizeof(name) - 1) && \
   (line[sizeof(name) - 1] == ' ' || \
   line[sizeof(name) - 1] == '\t'))

   if ((fp = fopen(_PATH_RESCONF, "r")) != NULL) {
      /* read the config file */
      while (fgets(buf, sizeof(buf), fp) != NULL) {
         /* skip comments */
         if (*buf == ';' || *buf == '#')
            continue;
         /* read default domain name */
         if (MATCH(buf, "domain")) {
            if (haveenv)        /* skip if have from environ */
               continue;
            cp = buf + sizeof("domain") - 1;
            while (*cp == ' ' || *cp == '\t')
               cp++;
            if ((*cp == '\0') || (*cp == '\n'))
               continue;
            strncpy(_sip_res.defdname, cp, sizeof(_sip_res.defdname) - 1);
            _sip_res.defdname[sizeof(_sip_res.defdname) - 1] = '\0';
            if ((cp = strpbrk(_sip_res.defdname, " \t\n")) != NULL)
               *cp = '\0';
            havesearch = 0;
            continue;
         }
         /* set search list */
         if (MATCH(buf, "search")) {
            if (haveenv)        /* skip if have from environ */
               continue;
            cp = buf + sizeof("search") - 1;
            while (*cp == ' ' || *cp == '\t')
               cp++;
            if ((*cp == '\0') || (*cp == '\n'))
               continue;
            strncpy(_sip_res.defdname, cp, sizeof(_sip_res.defdname) - 1);
            _sip_res.defdname[sizeof(_sip_res.defdname) - 1] = '\0';
            if ((cp = strchr(_sip_res.defdname, '\n')) != NULL)
               *cp = '\0';
            /*
            * Set search list to be blank-separated strings
            * on rest of line.
            */
            cp = _sip_res.defdname;
            pp = _sip_res.dnsrch;
            *pp++ = cp;
            for (n = 0; *cp && pp < _sip_res.dnsrch + MAXDNSRCH; cp++) {
               if (*cp == ' ' || *cp == '\t') {
                  *cp = 0;
                  n = 1;
               } else if (n) {
                  *pp++ = cp;
                  n = 0;
               }
            }
            /* null terminate last domain if there are excess */
            while (*cp != '\0' && *cp != ' ' && *cp != '\t')
               cp++;
            *cp = '\0';
            *pp++ = 0;
            havesearch = 1;
            continue;
         }
         /* read nameservers to query */
         if (MATCH(buf, "nameserver") && nserv < MAXNS) {
            struct in_addr a;

            cp = buf + sizeof("nameserver") - 1;
            while (*cp == ' ' || *cp == '\t')
               cp++;
            if ((*cp != '\0') && (*cp != '\n') && IS_INET_RETURN_OK( inet_aton(cp, &a)) ) {
               _sip_res.nsaddr_list[nserv].sin_addr = a;
               _sip_res.nsaddr_list[nserv].sin_family = AF_INET;
               _sip_res.nsaddr_list[nserv].sin_port =
                  htons(NAMESERVER_PORT);
               nserv++;
            }
            continue;
         }
#ifdef RESOLVSORT
         if (MATCH(buf, "sortlist")) {
            struct in_addr a;

            cp = buf + sizeof("sortlist") - 1;
            while (nsort < MAXRESOLVSORT) {
               while (*cp == ' ' || *cp == '\t')
                  cp++;
               if (*cp == '\0' || *cp == '\n' || *cp == ';')
                  break;
               net = cp;
               while (*cp && !ISSORTMASK(*cp) && *cp != ';' &&
                  isascii(*cp) && !isspace(*cp))
                  cp++;
               n = *cp;
               *cp = 0;
               if (IS_INET_RETURN_OK( inet_aton(net, &a)) ){
                  _sip_res.sort_list[nsort].addr = a;
                  if (ISSORTMASK(n)) {
                     *cp++ = n;
                     net = cp;
                     while (*cp && *cp != ';' &&
                        isascii(*cp) && !isspace(*cp))
                        cp++;
                     n = *cp;
                     *cp = 0;
                     if (IS_INET_RETURN_OK( inet_aton(net, &a)) ){
                        _sip_res.sort_list[nsort].mask = a.s_addr;
                     } else {
                        _sip_res.sort_list[nsort].mask = 
                           net_mask(_sip_res.sort_list[nsort].addr);
                     }
                  } else {
                     _sip_res.sort_list[nsort].mask = 
                        net_mask(_sip_res.sort_list[nsort].addr);
                  }
                  nsort++;
               }
               *cp = n;
            }
            continue;
         }
#endif
         if (MATCH(buf, "options")) {
            res_setoptions(buf + sizeof("options") - 1, "conf");
            continue;
         }
      }
#ifdef RESOLVSORT
      _sip_res.nsort = nsort;
#endif
      (void) fclose(fp);
   }

   if (nserv > 1)
      _sip_res.nscount = nserv;

   if (_sip_res.defdname[0] == 0 &&
      gethostname(buf, sizeof(_sip_res.defdname) - 1) == 0 &&
      (cp = strchr(buf, '.')) != NULL)
      strcpy(_sip_res.defdname, cp + 1);

   /* find components of local domain that might be searched */
   if (havesearch == 0) {
      pp = _sip_res.dnsrch;
      *pp++ = _sip_res.defdname;
      *pp = NULL;

#ifndef RFC1535
      dots = 0;
      for (cp = _sip_res.defdname; *cp; cp++)
         dots += (*cp == '.');

      cp = _sip_res.defdname;
      while (pp < _sip_res.dnsrch + MAXDFLSRCH) {
         if (dots < LOCALDOMAINPARTS)
            break;
         cp = strchr(cp, '.') + 1;    /* we know there is one */
         *pp++ = cp;
         dots--;
      }
      *pp = NULL;
#ifdef DEBUG
      if (_sip_res.options & RES_DEBUG) {
         printf(";; res_init()... default dnsrch list:\n");
         for (pp = _sip_res.dnsrch; *pp; pp++)
            printf(";;\t%s\n", *pp);
         printf(";;\t..END..\n");
      }
#endif
#endif /* !RFC1535 */
   }

   /*      if (issetugid())
   _sip_res.options |= RES_NOALIASES;
   else */
#ifndef WINCE
   if ((cp = getenv("RES_OPTIONS")) != NULL)
      res_setoptions(cp, "env");
#endif
   _sip_res.options |= RES_INIT;
   return (0);
}
#endif

#ifndef _VXWORKS /* [ */
static void res_setoptions(char *options, char *source)
{
   char *cp = options;
   int i;

#ifdef DEBUG
   if (_sip_res.options & RES_DEBUG)
      printf(";; res_setoptions(\"%s\", \"%s\")...\n",
      options, source);
#endif
   while (*cp) {
      /* skip leading and inner runs of spaces */
      while (*cp == ' ' || *cp == '\t')
         cp++;
      /* search for and process individual options */
      if (!strncmp(cp, "ndots:", sizeof("ndots:") - 1)) {
         i = atoi(cp + sizeof("ndots:") - 1);
         if (i <= RES_MAXNDOTS)
            _sip_res.ndots = i;
         else
            _sip_res.ndots = RES_MAXNDOTS;
#ifdef DEBUG
         if (_sip_res.options & RES_DEBUG)
            printf(";;\tndots=%d\n", _sip_res.ndots);
#endif
      } else if (!strncmp(cp, "debug", sizeof("debug") - 1)) {
#ifdef DEBUG
         if (!(_sip_res.options & RES_DEBUG)) {
            printf(";; res_setoptions(\"%s\", \"%s\")..\n",
               options, source);
            _sip_res.options |= RES_DEBUG;
         }
         printf(";;\tdebug\n");
#endif
      } else if (!strncmp(cp, "inet6", sizeof("inet6") - 1)) {
         _sip_res.options |= RES_USE_INET6;
      } else if (!strncmp(cp, "no_tld_query", sizeof("no_tld_query") - 1)) {
         _sip_res.options |= RES_NOTLDQUERY;
      } else {
         /* XXX - print a warning here? */
      }
      /* skip to next run of spaces */
      while (*cp && *cp != ' ' && *cp != '\t')
         cp++;
   }
}

#ifdef RESOLVSORT
/* XXX - should really support CIDR which means explicit masks always. */
#if defined(_WIN32)
static u_int32
#elif defined(_VXWORKS)
static uint32_t
#endif
net_mask(in)            /* XXX - should really use system's version of this */
struct in_addr in;
{
#if defined(_WIN32)
   register u_int32 i = ntohl(in.s_addr);
#elif defined(_VXWORKS)
   register uint32_t i = ntohl(in.s_addr);
#endif

   if (IN_CLASSA(i))
      return (htonl(IN_CLASSA_NET));
   else if (IN_CLASSB(i))
      return (htonl(IN_CLASSB_NET));
   return (htonl(IN_CLASSC_NET));
}
#endif
#endif /* _VXWORKS ] */

/*
* Weak aliases for applications that use certain private entry points,
* and fail to include <resolv.h>.
*/
#undef res_init
#endif /* __pingtel_on_posix__ */
