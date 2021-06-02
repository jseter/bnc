#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <stdarg.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#include <netdb.h>
#include <pwd.h>

#include "config.h"
#include "sbuf.h"
#include "struct.h"
#include "send.h"

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



#ifndef HAVE_SNPRINTF
#ifdef HAVE_VSNPRINTF
int snprintf(char *str, size_t n, const char *format,...)
{
  int p;
  va_list ap;
  va_start (ap, format);
  p = vsnprintf(char *str, size_t n, const char *format,
  va_end(ap);
  return p;
}
#else
#define BADBUF 4096
char mybuf[BADBUF+1];		/* dumb but oh well, attempting securety : */

int snprintf (char *str, size_t n, const char *format,...)
{
  int p;
  va_list ap;

  va_start (ap, format);

  p = vsprintf (mybuf, format, ap);
  va_end (ap);
  if(n > BADBUF)
  {
    n = BADBUF;
  }
  strncpy (str, mybuf, n);
  str[n]='\0';
  if (p > n)
  {
    p = n;
  }
  return p;
}

int vsnprintf ( char *str, size_t n, const char *format, va_list ap )
{
  int p;
  p = vsprintf (mybuf, format, ap);
  if(n > BADBUF)
  {
    n = BADBUF;
  }
  strncpy (str, mybuf, n);
  str[n]='\0';
  if (p > n)
  {
    p = n;
  }
  return p;
}
#endif
#endif


int xsockprint(int fd,const char *format,...)
{
  int p;
  va_list ap;
  va_start(ap,format);
  if(fd == DOCKEDFD)
  {
  	return 1; 
  }
  
  p = vsnprintf(buffer, PACKETBUFF, format, ap);
  va_end(ap);
  p = send(fd, buffer, p, 0);
  return p;
}


int logprint(confetti *jr,const char *format,...)
{
  int p;
  va_list ap;
  va_start(ap,format);
  p = vsnprintf(buffer, PACKETBUFF, format, ap);
  va_end(ap);
  bnclog(jr,buffer);
  return p;
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

	if(!strncmp(encr, pass, 10))
		return 1;

	return 0;
}


int connokay (struct sockaddr_in *sa, confetti * jr)
{
	struct hostent *hp;
	accesslist *na;

	hp = gethostbyaddr ((char *) &sa->sin_addr, sizeof (struct in_addr), AF_INET);
	logprint(jack, "Connection from %s", inet_ntoa (sa->sin_addr));

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

int do_connect(char *vhostname, char *hostname, u_short port, char *uname, int *status)
{
	int res;
	int s;
	int f;
	int st;
	struct hostent *he;
	struct sockaddr_in sin;
	
	st = 0;

	f = 1;
	s = socket (AF_INET, SOCK_STREAM, 0);
	if (s < 0)
	{
		return -2;
	}
	
	res = setnonblock(s);
	if(res == -1)
	{
		close(s);
		return -2;
	}
		
	memset (&sin, 0, sizeof (struct sockaddr_in));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = INADDR_ANY;
	
	if(vhostname)
	{
		res = inet_aton(vhostname, &sin.sin_addr);
		if(res == 0)
		{
			he = gethostbyname(vhostname);
			if (he)
				memcpy (&sin.sin_addr, he->h_addr, he->h_length);
			else
				sin.sin_addr.s_addr = INADDR_ANY;
		}
	}
	
//	printf("vhost: %s %s \n", vhostname, inet_ntoa(sin.sin_addr));

	if (bind (s, (struct sockaddr *) &sin, sizeof (struct sockaddr_in)) < 0)
	{
		return -9;
	}
	memset (&sin, 0, sizeof (struct sockaddr_in));
	sin.sin_family = AF_INET;
	sin.sin_port = htons (port);
	sin.sin_addr.s_addr = INADDR_ANY;

	res = inet_aton(hostname, &sin.sin_addr);
	if(res == 0)
	{
		he = gethostbyname(hostname);
		if (he)
			memcpy (&sin.sin_addr, he->h_addr, he->h_length);
		else
			sin.sin_addr.s_addr = INADDR_ANY;
	}
	
//	printf("host: %s %s \n", hostname, inet_ntoa(sin.sin_addr));

	if (jack->identwd)
	{
		identwd_lock(&sin, port);
	}

	res = connect (s, (struct sockaddr *) &sin, sizeof (sin));
	if ( res == -1)
	{
		switch(errno)
		{
			case EINPROGRESS:
				st = 1;
				break;
			default:
				close(s);
				return -2;
		}
	}

	if (jack->identwd)
	{
		identwd_unlock(s, &sin, port, uname);
	}
	if(status)
		*status = st;
	return s;
}


/* trust the caller of handleclient to filter \0 \n and \r
 * expects buflen be the len of the string, not counting the \0
 * also expects a \0 to be prepended.
 */ 

int handleclient (struct cliententry *cptr, int fromwho, int buflen, char *buf)
{
	int p, f, r;
	char *prefix;
	char *pargv[10];
	char repv[10];
	
	int pargc;
	int repc;
	int iswhite;
	f=0;

        prefix=NULL;
	pargv[0]=buf;
	pargc=1;
	repc=0;
	
	iswhite=0; 
	p=0;

	if(buf[p] == ':')
	{
		prefix=&buf[p+1];
		pargc=0;
		p++;
	}
	
	for(;p<buflen;p++)
	{
		if(iswhite) /* inside the whitespace */
		{
			if( buf[p] == ':' )
			{
				pargv[pargc]=&buf[p+1];
				pargc++;
				break; /* hit a string, meaning done */
			}
			if(buf[p] != ' ')
			{
				iswhite=0; /* no longer white */
				pargv[pargc]=&buf[p];
				pargc++;
				if(pargc > 9)
				{
					break;
				}
			}
		}
		else
		{
		        if(buf[p] == ' ' )
			{
				repv[repc++]=buf[p]; /* this shouldn't ever overflow */
				buf[p]='\0';
				iswhite=1;
			}
		}
	}
 	f=handlepclient(cptr, fromwho, pargc, pargv, prefix);
	if((cptr->flags & FLAGCONNECTED) && (f == FORWARDCMD))
	{
		f=0;
		for(p=0;p<buflen;p++)
		{
			if( buf[p] == '\0' )
			{
				if(repc > 0)
				{
					buf[p]=repv[f++];
					repc--;
				}
			}
		}
		if(fromwho == CLIENT)
		{
			tprintf(&cptr->srv, "%s\n", buf);
		}
		else
		{
			/* don't forward anything if its docked */
			r=1;
			if( !(cptr->flags & FLAGDOCKED))
			{
				tprintf(&cptr->loc, "%s\n", buf);
			}
		}
		f=0;
	}
	return f;
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
	int f,r,m;
	int res;

	m=0;
	if(cptr->loc.fd >= 0)
	{
		m=(FD_ISSET (cptr->loc.fd, rfds));
	}
	if(m)
	{
		r = recv (cptr->loc.fd, allbuf, PACKETBUFF, 0);
		if(r <= 0)
		{
			if(cptr->flags & FLAGDOCKED)
			{
				if(cptr->srv.fd == -1)
				{
					close(cptr->loc.fd);
					cptr->loc.fd = -1;
					return KILLCURRENTUSER;
				}
				close(cptr->loc.fd); /* wipe that dude */
				cptr->loc.fd=DOCKEDFD;
				return 0;
			}
			else
			{
				return KILLCURRENTUSER;
			}
		}
		sbuf_put(&cptr->loc.recvq, allbuf, r);
	}

	for(;;)
	{
		res = sbuf_getmsg(&cptr->loc.recvq, mline, 512);
		if(res <= 0)
			break;
		f = handleclient(cptr, CLIENT, res - 1, mline);
		if(f > 1)
			return f; 			
	}

	if(!(cptr->flags & FLAGCONNECTED))
	{
		return 0;
	}
	
	if(cptr->srv.fd == -1)
	{
		cptr->flags &= ~FLAGCONNECTED;
		return 0;
	}	
	
	
	
	if(FD_ISSET (cptr->srv.fd, rfds))
	{
		r = recv (cptr->srv.fd, allbuf, PACKETBUFF, 0);
		if (r <= 0)
		{
			if(cptr->flags & FLAGKEEPALIVE)
				return SERVERDIED;
			else
				return KILLCURRENTUSER;
		}
		sbuf_put(&cptr->srv.recvq, allbuf, r);

		for(;;)
		{
			res = sbuf_getmsg(&cptr->srv.recvq, mline, 512);
			if(res <= 0)
				break;
			f = handleclient(cptr, SERVER, res - 1, mline);
			if(f > 1)
				return f; 			
		}

	}
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
				logprint(jack, "(%i) %s!%s@%s connected to %s", cptr->loc.fd, cptr->nick, cptr->uname, cptr->fromip, cptr->onserver);
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
	int f;

	s_sock = socket (AF_INET, SOCK_STREAM, 0);
	if (s_sock < 0)
	{
		return SOCKERR;
	}
	memset (&muhsin, 0, sizeof (struct sockaddr_in));

	muhsin.sin_family = AF_INET;
	muhsin.sin_port = htons (jr->dport);
	muhsin.sin_addr.s_addr = INADDR_ANY;
	f = 1;
	setsockopt (s_sock, SOL_SOCKET, SO_REUSEADDR, &f, sizeof (f));

	if (bind (s_sock, (struct sockaddr *) &muhsin, sizeof (struct sockaddr_in)) < 0)
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
	int ninlen;
	fd_set rfds;
	fd_set wfds;
	jack = jr;
	
	for(;;)
	{
		initclient(&rfds,&wfds);
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
			nfd = accept (s_sock, (struct sockaddr *) &nin, &ninlen);
			r=addon_client(nfd,&nin);
			if(r == -1)
			{
				close(nfd);
			}
		}
		chkclient(&rfds,&wfds);
	}
	return 0;
}


