#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <stdarg.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#ifdef TIME_WITH_SYS_TIME
#include <time.h>
#endif
#else
#include <time.h>
#endif
#include <sys/wait.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#include <netdb.h>
#include <pwd.h>

#ifdef HAVE_SSL
#include <openssl/crypto.h>
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#endif

#include "config.h"
#include "sbuf.h"
#include "struct.h"
#include "send.h"
#include "ctcp.h"


extern int h_errno;
extern char logbuf[];
extern int bnclog (confetti * jr, char *logbuff);
extern void add_access (confetti *, accesslist *);
extern int handlepclient (struct cliententry *cptr, int fromwho, int pargc, char **pargv, char *prefix);
extern void bnckill (int reason);
extern int wipechans(struct cliententry *cptr);
extern int remnl (char *buf, int size);
extern void *pmalloc(size_t size);
extern char *helplist[];
extern char *helplista[]; 

extern unsigned char motdb[];

int highfd;
int a_sock = 0;
int s_sock = 0;
struct sockaddr_in muhsin;
struct sockaddr wipi;
int sinlen;

unsigned char allbuf[PACKETBUFF+1];
unsigned char buffer[PACKETBUFF+1];

int setnonblock(int fd)
{
	int flags;
#ifdef O_NONBLOCK
	flags = fcntl(fd, F_GETFL, 0);
	if(flags == -1)
		flags = 0;
	return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
#else
	flags = 1;
	return ioctl(fd, FIONBIO, &flags);
#endif
}


struct cliententry *headclient;
confetti *jack;




struct pdcc *headpdcc;
struct ldcc *headldcc;



int logprint(confetti *jr, const char *format, ...)
{
	time_t clk;
	struct tm *tp;
	const char dayweek[7][4] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
	const char month[12][4] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
	va_list ap;
	va_start(ap, format);

	if(jr->logfile == NULL)
		return 0;

	time(&clk);
	tp = localtime(&clk);
	if(tp != NULL)
		fprintf(jr->logfile, "%.3s %.3s%3d %02d:%02d:%02d %d ", dayweek[tp->tm_wday], month[tp->tm_mon], tp->tm_mday, tp->tm_hour, tp->tm_min, tp->tm_sec, tp->tm_year + 1900);
	vfprintf(jr->logfile, format, ap);
	va_end(ap);
	return 0;
}


int thestat(char *buf,int len, struct cliententry *cptr)
{
	int p,d;
	char *st[2] = 
	{
		"spunbackd",
		"SPUNBACKD",
	};
	d=cptr->flags;
	for(p=0;st[0][p];p++)
	{
		if(p+1 >= len)
		{
			break;
		}
		else
		{
			buf[p]=st[ d&1 ][p];
			
		}
		d=d>>1;
	}
	buf[p]='\0';
	return p;
}

int chanlist(char *buf,int len, struct cliententry *client)
{
	int p,c,f;
	char *s;
	struct chanentry *cptr;
	p=0;
	cptr=client->headchan;
	c=0;
	memset(buf,0,len);
	
	while(cptr)
	{
		c++;
		if(p>0)
		{
			buf[p++]=' ';
		}
		for(s=cptr->chan;*s;s++)
		{
			buf[p++]=*s;
			if(p>=len)
			{
				break;
			}
		}
		if(p<len)
		{
			buf[p]='\0';
			cptr=cptr->next;
		}
		else
		{
			c=-1;
			p--;
			buf[p--]='\0';
			for(f=3;f>0;f--)
			{
				buf[p]='.';
			}
			cptr=NULL;
		}
	}
	return c;
}

/*
 * experimental matching code (WD) I hope this runs faster, who knows...
 */

int check_match (char *mask)
{
  if (!index (mask, '*') && !index (mask, '?'))
    return 0;
  else if (!strcmp (mask, "*"))
    return 1;
  else
    return 2;
}

/*
 * Note, thanx to RogerY and Redtrio for this wonderful code.  Please
 * keep this notice intact.  -- TazQ 
 */

/*
 * My wildcard matching functions. I think it works well :) Seems to be 
 */
/*
 * working stablely, but not too sure.
 */
int
  match (char *str, char *wld)
{
  int i = check_match (wld);

  switch (i)
  {
    case 0:
      return strcasecmp (str, wld);
      break;
    case 1:
      return 0;
      break;
  }

  if (!(*wld) || !(*str))
    return 1;
  while (*wld || *str)
  {
    if (tolower (*wld) == tolower (*str))
    {
      wld++;
      str++;
    }
    else if (tolower (*wld) != tolower (*str) && *wld == '*')
    {
      int i = 0;
      char *st;

      st = (char *) pmalloc( sizeof(char) * strlen(wld) + 1);

      while (*wld == '*' && *wld)
      {
	wld++;
	if (!*wld)
	  return 0;
      }
      while (*wld && *wld != '*' && *wld != '?')
      {
	st[i] = *wld;
	i++;
	wld++;
      }
      st[i] = '\0';
      while (*str && strncasecmp (st, str, strlen (st)))
	str++;
      if (!strncasecmp (st, str, strlen (st)))
	str += strlen (st);
      else
	return 1;
    }
    else if (tolower (*wld) != tolower (*str) && *wld == '?' && *wld && *str)
    {
      wld++;
      str++;
    }
    else if (tolower (*wld) != tolower (*str))
      return 1;
  }
  return 0;
}




int passwordokay (char *s, char *pass)
{
	char *encr;
	char *crypt ();

	if(*pass == '+')
	{
		pass++;
		encr = crypt(s, pass);
	}
	else
		encr = s;

	if(!strcmp(encr, pass) == 0)
		return 1;

	return 0;
}


int connokay (struct sockaddr_in *sa, confetti * jr)
{
	struct hostent *hp;
	accesslist *na;

	hp = gethostbyaddr ((char *) &sa->sin_addr, sizeof (struct in_addr), AF_INET);
	logprint(jack, "Connection from %s\n", inet_ntoa (sa->sin_addr));

	if (!jr->has_alist)
	{
		return 1;
	}
	for (na = jr->alist; na; na = na->next)
	{
		if (na->type == 1)
		{
			if (!match (inet_ntoa (sa->sin_addr), na->addr))
			return 1;
		}
		else if (na->type == 2 && hp)
		{
			if (!match (hp->h_name, na->addr))
			return 1;
		}
	}
	return 0;
}


int send_queued(struct lsock *cptr)
{
	int res;
	int length;
	char *msg;

	
	while(sbuf_getlength(&cptr->sendq) > 0)
	{
		msg = sbuf_pagemap(&cptr->sendq, &length);
		if(msg == NULL)
			break; /*XXX*/
		if(length <= 0)
			break; /*XXX*/
		res = send(cptr->fd, msg, length, 0);
		if(res == -1)
		{
			if(errno == EINTR)
				continue;
			if(errno == EAGAIN)
				break;
			return -1;
		}
		sbuf_delete(&cptr->sendq, res);
	}
	return 0;
}



int wipeclient(struct cliententry *cptr)
{
	wipechans(cptr);
	if(cptr->loc.fd > -1)
	{
		send_queued(&cptr->loc);
		close(cptr->loc.fd);
		cptr->loc.fd=-1;
	}
	if(cptr->srv.fd > -1)
	{
		send_queued(&cptr->srv);
		close(cptr->srv.fd);
		cptr->srv.fd=-1;
	}

	sbuf_clear(&cptr->loc.sendq);
	sbuf_clear(&cptr->loc.recvq);
	sbuf_clear(&cptr->srv.sendq);
	sbuf_clear(&cptr->srv.recvq);
	return 0;
}

int freeclientlist (struct cliententry *cptr)
{
  struct cliententry *hptr;

  while (cptr != NULL)
  {
    hptr = cptr->next;
    wipeclient(cptr);
    cptr = hptr;
  }
  return 1;
}

int bewmstick (void)
{
  freeclientlist(headclient);
  headclient = NULL;
  bnckill(FATALITY);
  return 0;
}

struct cliententry *getclient (struct cliententry *cptr, int nfd)
{
	while (cptr != NULL)
	{
		if (cptr->loc.fd == nfd)
		{
			return cptr;
		}
		if (cptr->srv.fd == nfd)
		{
			return cptr;
		}
		cptr = cptr->next;
	}
	return NULL;
}

int countfds (struct cliententry *cptr)
{
	int p;

	p = 0;
	for(p=0;cptr;cptr=cptr->next)
	{
		if(cptr->loc.fd == -1)
		{
			continue;
		}
		p++;
	}
	return p;
}

char *gethost (struct in_addr *addr)
{
	struct hostent *hp;

	hp = gethostbyaddr ((char *) addr, sizeof (struct in_addr), AF_INET);
	if(hp)
	{
		return hp->h_name;
	}
	return inet_ntoa (*addr);
}

int identwd_lock(struct sockaddr_in *sin, int port)
{
	FILE *fp = NULL;
	struct passwd *pw;
	char filename[1024];
	
	pw = getpwuid (getuid ());
	if(pw == NULL)
	{
		return -1;
	}
	sprintf (filename, "%s/.identwd/%s.%i.LOCK",
	pw->pw_dir, inet_ntoa (sin->sin_addr), port);
	fp = fopen (filename, "w");
	if (fp)
	{
		fprintf (fp, "WAIT\n");
		fclose(fp);
		return 0;
	}
	return -1;
}

int identwd_unlock(int s, struct sockaddr_in *sin, int port, char *uname)
{
	FILE *fp = NULL;
	struct passwd *pw;
	char filename[1024];
	
	pw = getpwuid(getuid());
	if(pw == NULL)
	{
		return -1;
	}
	sprintf (filename, "%s/.identwd/%s.%i.LOCK",
	pw->pw_dir, inet_ntoa (sin->sin_addr), port);
	fp = fopen (filename, "w");
	if (fp)
	{
		int len = sizeof (struct sockaddr_in);
		struct sockaddr_in mysa;

		getsockname (s, (struct sockaddr *) &mysa, &len);
		fprintf (fp, (char *) "%s %i %s\n",
		(char *) inet_ntoa (mysa.sin_addr),
		(u_short) ntohs (mysa.sin_port), uname);
		fclose(fp);
		return 0;
	}
	return -1;
}

int irc_connect(struct cliententry *cptr, char *server, u_short port, char *pass, int ctype)
{
	int fd;
	int res;
	struct hostent *he;
	int rfamily;
	int lfamily;
	int dolocal;
	struct sockaddr_in lsin4;
	struct sockaddr_in rsin4;
	struct sockaddr_in6 lsin6;
	struct sockaddr_in6 rsin6;

	
	cptr->flags &= ~FLAGAUTOCONN;
	cptr->susepass=0;
	
	if(cptr->flags & FLAGCONNECTED)
	{
		tprintf(&cptr->loc, "NOTICE AUTH :Disconnecting old\n", server, port);		
		if(cptr->srv.fd > -1)
		{
			close(cptr->srv.fd);
			cptr->srv.fd=-1;
		}

		cptr->flags &= ~FLAGCONNECTED;
	}
	sbuf_clear(&cptr->srv.sendq);
	sbuf_clear(&cptr->srv.recvq);

	
	tprintf(&cptr->loc, "NOTICE AUTH :Making reality through %s port %i\n", server, port);

	memset(&lsin4, 0, sizeof(lsin4));
	memset(&rsin4, 0, sizeof(rsin4));
	memset(&lsin6, 0, sizeof(lsin6));
	memset(&rsin6, 0, sizeof(rsin6));	
	
	lfamily = rfamily = AF_INET;
	
	do
	{
		res = inet_pton(AF_INET, server, &rsin4.sin_addr);
		if(res)
		{
			rfamily = AF_INET;
			break;
		}
		
		res = inet_pton(AF_INET6, server, &rsin6.sin6_addr);
		if(res)
		{
			rfamily = AF_INET6;
			break;
		}

#ifdef HAVE_GETHOSTBYNAME2		
		if(ctype == 1)
		{
			he = gethostbyname2(server, AF_INET6);
			if(he)
			{
				rfamily = AF_INET6;
				memcpy(&rsin6.sin6_addr, he->h_addr, he->h_length);
				break;
			}
		}
#endif
		he = gethostbyname(server);
		if(he)
		{
			rfamily = AF_INET;
			memcpy(&rsin4.sin_addr, he->h_addr, he->h_length);
			break;
		}

#ifdef HAVE_GETHOSTBYNAME2
		if(ctype != 1)
		{
			he = gethostbyname2(server, AF_INET6);
			if(he)
			{
				rfamily = AF_INET6;
				memcpy(&rsin6.sin6_addr, he->h_addr, he->h_length);
				break;
			}
		}
#endif
		return -1;
	} while(0);

	
	do
	{
		char *server;
		server = cptr->vhost;
		if(server == NULL)
		{
			dolocal = 0;
			break;
		}
		
		dolocal = 1;
		
		res = inet_pton(AF_INET, server, &lsin4.sin_addr);
		if(res)
		{
			lfamily = AF_INET;
			break;
		}
		
		res = inet_pton(AF_INET6, server, &lsin6.sin6_addr);
		if(res)
		{
			lfamily = AF_INET6;
			break;
		}

#ifdef HAVE_GETHOSTBYNAME2
		if(ctype == 1)
		{
			he = gethostbyname2(server, AF_INET6);
			if(he)
			{
				lfamily = AF_INET6;
				memcpy(&lsin6.sin6_addr, he->h_addr, he->h_length);
				break;
			}
		}
#endif
		he = gethostbyname(server);
		if(he)
		{
			lfamily = AF_INET;
			memcpy(&lsin4.sin_addr, he->h_addr, he->h_length);
			break;
		}

#ifdef HAVE_GETHOSTBYNAME2
		if(ctype != 1)
		{
			he = gethostbyname2(server, AF_INET6);
			if(he)
			{
				lfamily = AF_INET6;
				memcpy(&lsin6.sin6_addr, he->h_addr, he->h_length);
				break;
			}
		}
#endif
		lfamily = AF_INET;
		dolocal = 0;
	} while(0);

	if(dolocal && lfamily != rfamily)
		dolocal = 0;


	if(rfamily == AF_INET)
	{
		rsin4.sin_family = AF_INET;
		rsin4.sin_port = htons (port);	
	}
	else /* if(rfamily == AF_INET6) */
	{
		rsin6.sin6_family = AF_INET6;
		rsin6.sin6_port = htons(port);
	}

	lsin4.sin_family = AF_INET;


	fd = socket (rfamily, SOCK_STREAM, 0);
	if(fd == -1)
	{
		tprintf(&cptr->loc, "NOTICE AUTH :Failed Connection\n");
		return -1;
	}

	res = setnonblock(fd);
	if(res == -1)
	{
		tprintf(&cptr->loc, "NOTICE AUTH :Failed Connection\n");
		close(fd);
		return -1;
	}
	
	if(dolocal)
	{
		if(lfamily == AF_INET)
			bind (fd, (struct sockaddr *) &lsin4, sizeof(lsin4));
		else /* if(lfamily == AF_INET6) */
			bind(fd, (struct sockaddr *) &lsin6, sizeof(lsin6));
	}


	if(rfamily == AF_INET && jack->identwd)
		identwd_lock(&rsin4, port);

	if(rfamily == AF_INET)
		res = connect(fd, (struct sockaddr *)&rsin4, sizeof(rsin4));
	else /* if(rfamily == AF_INET6) */
		res = connect(fd, (struct sockaddr *)&rsin6, sizeof(rsin6));
		
	if( res == -1)
	{
		switch(errno)
		{
			case EINPROGRESS:
				cptr->flags |= FLAGCONNECTING;
				break;
			default:
				tprintf(&cptr->loc, "NOTICE AUTH :Failed Connection\n");
				close(fd);
				return -1;
		}
	}
	else
	{
		tprintf(&cptr->loc, "NOTICE AUTH :Suceeded connection\n");
		logprint(jack, "(%i) %s!%s@%s connected to %s\n", cptr->loc.fd, cptr->nick, cptr->uname, cptr->fromip, server);	
	}

	if(rfamily == AF_INET && jack->identwd)
		identwd_unlock(fd, &rsin4, port, cptr->uname);

	strncpy (cptr->onserver, server, HOSTLEN);
	cptr->onserver[HOSTLEN]='\0';
	strncpy (cptr->sid, server, HOSTLEN);
	cptr->sid[HOSTLEN]='\0';

	cptr->srv.fd = fd;
	cptr->flags |= FLAGCONNECTED;

	if(pass)
		tprintf(&cptr->srv, "PASS :%s\n", pass);
	tprintf(&cptr->srv, "NICK %s\n", cptr->nick);	
	tprintf(&cptr->srv, "USER %s \"%s\" \"%s\" :%s\n", cptr->uname, cptr->fromip, cptr->onserver, cptr->realname);
	return 0;
}



/* trust the caller of handleclient to filter \0 \n and \r
 * expects buflen be the len of the string, not counting the \0
 * also expects a \0 to be appended.
 */ 

int handleclient (struct cliententry *cptr, int fromwho, int buflen, char *buf)
{
	int f;
	char *prefix;
	char *pargv[9+1];
	static char ibuf[512+1];
	char *src;
	char *eos;
	int clen;
	
	int pargc;
	int iswhite;
	f=0;


	if(buflen <= 0)
		return 0;

	clen = buflen;
	if(clen > 512)
		clen= 512;

	memcpy(ibuf, buf, clen);
	src = ibuf;
	eos = src + clen;
	*eos = '\0';

	for(;*src == ' '; src++); /* skip leading whitespace */

	prefix=NULL;
	pargc=0;
	iswhite=0; 

	if(*src == ':')
	{
		prefix=++src;
		pargc=0;
	}
	else
		pargv[pargc++] = src;
	
	for(;src < eos; src++)
	{
		if(iswhite) /* inside the whitespace */
		{
			if( *src == ':' )
			{
				pargv[pargc]=src + 1;
				pargc++;
				break; /* hit a string, meaning done */
			}
			if(*src != ' ')
			{
				iswhite=0; /* no longer white */
				pargv[pargc++]=src;
				if(pargc > 9)
					break;
			}
		}
		else
		{
			if(*src == ' ')
			{
				*src='\0';
				iswhite=1;
			}
		}
	}

	f = handlepclient(cptr, fromwho, pargc, pargv, prefix);
	if((cptr->flags & FLAGCONNECTED) && (f == FORWARDCMD))
	{
		f = 0;
		if(fromwho == CLIENT)
		{
			tprintf(&cptr->srv, "%s\n", buf);
		}
		else
		{
			/* don't forward anything if its docked */
			if( !(cptr->flags & FLAGDOCKED))
			{
				tprintf(&cptr->loc, "%s\n", buf);
			}
		}
	}
	return f;
}

void initldcc(fd_set *rfds)
{
	struct ldcc *dccptr;
	for(dccptr = headldcc; dccptr; dccptr=dccptr->next)
	{
		FD_SET(dccptr->fd, rfds);	
		if(dccptr->fd > highfd)
			highfd = dccptr->fd;
	}
}

void chkldcc(fd_set *rfds)
{
	int res;
	struct sockaddr_in sin;
	socklen_t sinlen;
	struct ldcc *dccptr;
	struct ldcc **parent;
	struct pdcc *mptr;
	
	parent = &headldcc;
	dccptr = headldcc;
	while(dccptr)
	{
		if(FD_ISSET(dccptr->fd, rfds))
		{
			sinlen = sizeof(sin);
			res = accept(dccptr->fd, (struct sockaddr *)&sin, &sinlen);
			if(res == -1)
			{
				if(errno == EWOULDBLOCK)
					goto donext;
				if(errno == EINTR)
					continue;
				close(dccptr->fd);
				
				*parent = dccptr->next;
				free(dccptr);
				dccptr=*parent;
				continue;
			}
			
			mptr = malloc(sizeof(struct pdcc));
			if(mptr == NULL)
			{
				close(res);
				goto terml;
			}
			
			memset(mptr, 0 , sizeof(*mptr));
			
			mptr->lfd = res;
			setnonblock(mptr->lfd);
			mptr->rfd = socket (AF_INET, SOCK_STREAM, 0);
			if(mptr->rfd == -1)
			{
				close(mptr->lfd);
				free(mptr);
				goto terml;			
			}
			setnonblock(mptr->rfd);
rr_conn:
			res = connect(mptr->rfd, &dccptr->sin, dccptr->sinlen);
			if(res == -1)
			{
				switch(errno)
				{
					case EINTR:
						goto rr_conn;
					case EINPROGRESS:
						mptr->flags |= 1;
						break;
					default:
						perror("connect");
						close(mptr->lfd);
						close(mptr->rfd);
						goto terml;
				}
			}
			
			mptr->next = headpdcc;
			headpdcc = mptr;
terml:
			close(dccptr->fd);
			*parent = dccptr->next;
			free(dccptr);
			dccptr=*parent;
			continue;		
		}
donext:
		parent = &dccptr->next;
		dccptr = dccptr->next;
	}	
}

void initpdcc(fd_set *rfds, fd_set *wfds)
{
	struct pdcc *dccptr;
	for(dccptr = headpdcc; dccptr; dccptr=dccptr->next)
	{
		if(sbuf_getlength(&dccptr->rsendq) > 0)
		{
			FD_SET(dccptr->rfd, wfds);
			if(dccptr->rfd > highfd)
				highfd = dccptr->rfd;
		}
		else
		{
			FD_SET(dccptr->lfd, rfds);
			if(dccptr->lfd > highfd)
				highfd = dccptr->lfd;
		}

		if(sbuf_getlength(&dccptr->lsendq) > 0)
		{
			FD_SET(dccptr->lfd, wfds);
			if(dccptr->lfd > highfd)
				highfd = dccptr->lfd;
		}
		else
		{
			FD_SET(dccptr->rfd, rfds);
			if(dccptr->rfd > highfd)
				highfd = dccptr->rfd;
		}
	}
}


int dccsend(int fd, struct sbuf *sendq)
{
	int res;
	int length;
	char *msg;

	
	while(sbuf_getlength(sendq) > 0)
	{
		msg = sbuf_pagemap(sendq, &length);
		if(msg == NULL)
			break; /*XXX*/
		if(length <= 0)
			break; /*XXX*/
		res = send(fd, msg, length, 0);
		if(res == -1)
		{
			if(errno == EINTR)
				continue;
			if(errno == EAGAIN)
				break;
			return 1;
		}
		sbuf_delete(sendq, res);
	}
	return 0;

}

int dccrecv(int fd, struct sbuf *recvq)
{
	int res;
	for(;;)
	{
		res = recv(fd, allbuf, PACKETBUFF, 0);
		if(res == -1)
		{
			if(errno == EINTR)
				continue;
			if(errno == EAGAIN)
				break;
			return 1;
		}
		if(res == 0)
			return 2;

		sbuf_put(recvq, allbuf, res);
	}
	return 0;
}


void chkpdcc(fd_set *rfds, fd_set *wfds)
{
	int res;
	struct pdcc *dccptr;
	struct pdcc **parent;


	parent = &headpdcc;
	dccptr = headpdcc;
	while(dccptr)
	{
		if(dccptr->lfd >= 0 && FD_ISSET(dccptr->lfd, wfds))
		{
			res = dccsend(dccptr->lfd, &dccptr->lsendq);
			if(res)
				goto done_err;

		}

		if(dccptr->rfd >= 0 && FD_ISSET(dccptr->rfd, wfds))
		{
			res = dccsend(dccptr->rfd, &dccptr->rsendq);
			if(res)
				goto done_err;
		}


		if(dccptr->lfd >= 0 && FD_ISSET(dccptr->lfd, rfds))
		{
			res = dccrecv(dccptr->lfd, &dccptr->rsendq);
			if(res == 1)
				goto done_err;
			if(res == 2)
				goto done_leof;
		}
	
		if(dccptr->rfd >= 0 && FD_ISSET(dccptr->rfd, rfds))
		{
			res = dccrecv(dccptr->rfd, &dccptr->lsendq);
			if(res == 1)
				goto done_err;
			if(res == 2)
				goto done_reof;
		}
		
next_dcc:		
		parent = &dccptr->next;
		dccptr = dccptr->next;
		continue;
done_err:
		if(dccptr->lfd >= 0)
			close(dccptr->lfd);
		if(dccptr->rfd >= 0)
			close(dccptr->rfd);
		sbuf_clear(&dccptr->lsendq);
		sbuf_clear(&dccptr->rsendq);

		*parent = dccptr->next;
		free(dccptr);
		dccptr=*parent;
		continue;
done_leof:
		dccsend(dccptr->lfd, &dccptr->lsendq);
		close(dccptr->lfd);
		dccptr->lfd = -1;

		if(dccptr->rfd >= 0 && sbuf_getlength(&dccptr->rsendq) > 0)
			goto next_dcc;
		goto done_err;
done_reof:
		dccsend(dccptr->rfd, &dccptr->rsendq);
		close(dccptr->rfd);
		dccptr->rfd = -1;

		if(dccptr->lfd >= 0 && sbuf_getlength(&dccptr->lsendq) > 0)
			goto next_dcc;
		goto done_err;

	}

}

void initclient(fd_set *rfds, fd_set *wfds)
{
	struct cliententry *cptr;
	int length;
		
	highfd = s_sock;
	FD_ZERO(rfds);
	FD_ZERO(wfds);


	for(cptr=headclient;cptr;cptr=cptr->next)
	{
		/* handle write set first */
		if(cptr->loc.fd > -1)
		{
			if((length = sbuf_getlength(&cptr->loc.sendq)))
			{
				if(length > HIGHOVL)
				{
					if(cptr->srv.fd >= 0)
						cptr->srv.flags |= FLAGDRAIN;
				}
				
				if(length < LOWOVL)
				{
					if(cptr->srv.fd >= 0)
						cptr->srv.flags &= ~FLAGDRAIN;
				}
			
				FD_SET(cptr->loc.fd, wfds);
				if(cptr->loc.fd > highfd)
					highfd = cptr->loc.fd;
			}
		}

		if(cptr->flags & FLAGCONNECTED  && cptr->srv.fd > -1)
		{
			if((length = sbuf_getlength(&cptr->srv.sendq)))
			{
				if(length > HIGHOVL)
				{
					if(cptr->loc.fd >= 0)
						cptr->loc.flags |= FLAGDRAIN;
				}
				
				if(length < LOWOVL)
				{
					if(cptr->loc.fd >= 0)
						cptr->loc.flags &= ~FLAGDRAIN;
				}
				FD_SET(cptr->srv.fd, wfds);
				if(cptr->srv.fd > highfd)
					highfd = cptr->srv.fd;
			}
		}

		/* now set read(s) if not draining */
		if(cptr->loc.fd > -1 && !(cptr->loc.flags & FLAGDRAIN))
		{
			FD_SET(cptr->loc.fd, rfds);
			if (cptr->loc.fd > highfd)
			{
				highfd = cptr->loc.fd;
			}
		}
		
		if(cptr->flags & FLAGCONNECTED && cptr->srv.fd > -1 && !(cptr->srv.flags & FLAGDRAIN) )
		{
			FD_SET(cptr->srv.fd,rfds);
			if (cptr->srv.fd > highfd)
			{
				highfd = cptr->srv.fd;
			}
		}
	}
}

char mline[512];
int scanclient (struct cliententry *cptr, fd_set * rfds)
{
	int f;
	int res;

	if(cptr->loc.fd >= 0 && FD_ISSET(cptr->loc.fd, rfds))
	{
		res = recv(cptr->loc.fd, allbuf, PACKETBUFF, 0);
		if(res == -1)
		{
			if(errno == EINTR || errno == EAGAIN)
				goto cr_out;

			if(cptr->flags & FLAGDOCKED)
			{
				if(cptr->srv.fd == -1)
				{
					close(cptr->loc.fd);
					cptr->loc.fd = -1;
					return KILLCURRENTUSER;
				}
				close(cptr->loc.fd);
				cptr->loc.fd=DOCKEDFD;
				return 0;
			}
			else
				return KILLCURRENTUSER;
		}
		
		if(res == 0)
		{
			if(cptr->flags & FLAGDOCKED)
			{
				if(cptr->srv.fd == -1)
				{
					close(cptr->loc.fd);
					cptr->loc.fd = -1;
					return KILLCURRENTUSER;
				}
				close(cptr->loc.fd);
				cptr->loc.fd=DOCKEDFD;
				return 0;
			}
			else
				return KILLCURRENTUSER;
		}
		sbuf_put(&cptr->loc.recvq, allbuf, res);

		for(;;)
		{
			res = sbuf_getmsg(&cptr->loc.recvq, mline, 512);
			if(res <= 0)
				break;
			f = handleclient(cptr, CLIENT, res - 1, mline);
			if(f)
				return f; 			
		}
	}
cr_out:

	
	if(cptr->srv.fd >= 0 && FD_ISSET(cptr->srv.fd, rfds))
	{
		res = recv(cptr->srv.fd, allbuf, PACKETBUFF, 0);
		if(res == -1)
		{
			if(errno == EINTR || errno == EAGAIN)
				goto sr_out;
				
			if(cptr->flags & FLAGKEEPALIVE)
				return SERVERDIED;
			else
				return KILLCURRENTUSER;		
		}
		
		if(res == 0)
		{
			if(cptr->flags & FLAGKEEPALIVE)
				return SERVERDIED;
			else
				return KILLCURRENTUSER;
		}
		sbuf_put(&cptr->srv.recvq, allbuf, res);

		for(;;)
		{
			res = sbuf_getmsg(&cptr->srv.recvq, mline, 512);
			if(res <= 0)
				break;
			f = handleclient(cptr, SERVER, res - 1, mline);
			if(f)
				return f; 			
		}

	}
sr_out:
	return 0;
}


void chkclient(fd_set *rfds, fd_set *wfds)
{
	int p;
	struct cliententry *cptr;


	for(cptr=headclient;cptr;cptr=cptr->next)
	{
		p=scanclient(cptr,rfds);
		if(p)
			goto han_err;
		
		if(cptr->loc.fd >= 0 && FD_ISSET(cptr->loc.fd, wfds))
		{
			p = send_queued(&cptr->loc);
			if(p == -1)
			{
				p = KILLCURRENTUSER;
				goto han_err;
			}

		}

		if(cptr->srv.fd >= 0 && FD_ISSET(cptr->srv.fd, wfds))
		{
			p = send_queued(&cptr->srv);
			if(p == -1)
			{
				p = SERVERDIED;
				goto han_err;
			}
			if(cptr->flags & FLAGCONNECTING)
			{
				cptr->flags &= ~FLAGCONNECTING;
				tprintf(&cptr->loc, "NOTICE AUTH :Suceeded connection\n");
				logprint(jack, "(%i) %s!%s@%s connected to %s\n", cptr->loc.fd, cptr->nick, cptr->uname, cptr->fromip, cptr->onserver);
			}
		}

han_err:
		if(p == KILLCURRENTUSER) /* self death */
		{
		
			wipeclient(cptr);			
			return;
		}
		if(p == SERVERDIED) /* keep alive */
		{
//			page_free(&cptr->page_server);
			sbuf_clear(&cptr->srv.sendq);
			sbuf_clear(&cptr->srv.recvq);
			tprintf(&cptr->loc, "NOTICE AUTH :IRC quit, KeepAlive here.\n", cptr->nick);
			cptr->flags &= ~FLAGCONNECTING;
			if(cptr->flags & FLAGCONNECTED)
			{
				if( cptr->srv.fd != -1)
				{
					close(cptr->srv.fd);
					cptr->srv.fd=-1;
				}
				cptr->flags &= ~FLAGCONNECTED;
			}
		}
	}
  	return;
}

int addon_client(int citizen, struct sockaddr_in *nin)
{
	int p,r;
	struct in_addr faddr;
	struct cliententry *cptr;

	if (jack->maxusers)
	{
		if (countfds (headclient) + 1 > jack->maxusers)
		{
			return -1;
		}
	}

	p = sizeof(struct sockaddr_in);
	r=getpeername (citizen, (struct sockaddr *)nin, &p);
	if(r)
	{
		return -1;
	}
	if (!connokay (nin, jack))
	{
		return -1;
	}

	for(cptr=headclient;cptr;cptr=cptr->next)
	{
		if((cptr->loc.fd == -1) && (cptr->srv.fd == -1))
			break;
	}

	if(cptr == NULL)
	{
		cptr = (struct cliententry *) pmalloc( sizeof( struct cliententry ));
		if( cptr == NULL)
		{
			return -1;
		}

		memset(cptr, 0, sizeof(struct cliententry));
		cptr->prev=NULL;
		cptr->next=headclient;

		if( headclient != NULL)
		{
			headclient->prev=cptr;
		}
		headclient=cptr;	
	}
			
	cptr->loc.fd = citizen;
	cptr->srv.fd = -1;
		
	cptr->flags=0;
	cptr->headchan=NULL;
		
	strncpy(cptr->vhost, jack->vhostdefault, HOSTLEN);
	cptr->vhost[HOSTLEN]='\0';
	strcpy(cptr->nick,"UNKNOWN");
	faddr = nin->sin_addr;
	strncpy (cptr->fromip, gethost (&faddr), HOSTLEN);
	cptr->fromip[HOSTLEN]='\0';

	if (jack->dpassf == 0)
	{
		cptr->flags |= FLAGPASS;
	}
	sbuf_claim(&cptr->loc.recvq);
	sbuf_claim(&cptr->loc.sendq);
	sbuf_claim(&cptr->srv.recvq);
	sbuf_claim(&cptr->srv.sendq);
//	cptr->blen=0;
		
	return 0;
}


int initproxy (confetti * jr)
{
	int opt;
	int res;
	int family;
	struct sockaddr_in sin4;
	struct sockaddr_in6 sin6;
	struct hostent *he;

	family = AF_INET;

	if(*jr->dhost)
	{
		res = inet_pton(AF_INET, jr->dhost, &sin4.sin_addr);
		if(res == 0)
		{
			res = inet_pton(AF_INET6, jr->dhost, &sin6.sin6_addr);
			if(res != 0)
			{
				family = AF_INET6;
			}
			else
			{
				he = gethostbyname(jr->dhost);
				if (he)
					memcpy (&sin4.sin_addr, he->h_addr, he->h_length);
				else
					sin4.sin_addr.s_addr = INADDR_ANY;
			}
		}
	}	

	if(family == AF_INET)
	{
		memset(&sin4, 0, sizeof(sin4));
		sin4.sin_family = AF_INET;
		sin4.sin_port = htons(jr->dport);
	}
	else /* if(family == AF_INET6) */
	{
		memset(&sin6, 0, sizeof(sin6));
		sin6.sin6_family = AF_INET6;
		sin6.sin6_port = htons(jr->dport);
	}

	s_sock = socket (family, SOCK_STREAM, 0);
	if (s_sock < 0)
	{
		return SOCKERR;
	}

	opt = 1;
	setsockopt (s_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof (opt));


	if(family == AF_INET)
		res = bind(s_sock, (struct sockaddr *)&sin4, sizeof(sin4));
	else /* if(family == AF_INET6) */
		res = bind(s_sock, (struct sockaddr *)&sin6, sizeof(sin6));

	if(res == -1)
	{
		close (s_sock);
		return BINDERR;
	}

	if (listen (s_sock, 10) < 0)
	{
		return LISTENERR;
	}

	return 0;
}

int ircproxy (confetti * jr)
{
	int p,r,nfd;
	struct sockaddr_in nin;
	socklen_t ninlen;
	fd_set rfds;
	fd_set wfds;
	jack = jr;
	
	for(;;)
	{
		initclient(&rfds,&wfds);
		initpdcc(&rfds, &wfds);
		initldcc(&rfds);
		FD_SET(s_sock, &rfds);
		if ((p = select (highfd + 1, &rfds, &wfds, (fd_set *) 0, NULL)) < 0)
		{
			if(errno == ENOMEM)
			{
				bnckill (SELECTERR);
			}
			bnckill(FATALITY);
		}
		if (FD_ISSET (s_sock, &rfds))
		{
			ninlen = sizeof(nin);
			nfd = accept (s_sock, (struct sockaddr *) &nin, &ninlen);
			if(nfd >= 0)
			{
				r=addon_client(nfd,&nin);
				if(r == -1)
				{
					close(nfd);
				}
			}
		}
		chkldcc(&rfds);
		chkpdcc(&rfds,&wfds);
		chkclient(&rfds,&wfds);
	}
	return 0;
}


