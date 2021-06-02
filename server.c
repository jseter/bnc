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
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <pwd.h>

#include "common.h"
#define ENDLESSLOOP for(;;)


extern int h_errno;
extern char logbuf[];
extern int bnclog (confetti * jr, char *logbuff);
extern void add_access (confetti *, accesslist *);
extern int handlepclient (struct cliententry *list_ptr, int fromwho, int pargc, char **pargv, char *prefix);
extern void bnckill (int reason);
extern int remnl (char *buf, int size);

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



struct cliententry *headclient;
confetti *jack;

char *hmsg[]=
{
	"NOTICE AUTH :",
	":bnc!bnc@bnc.com PRIVMSG %s :",
	NULL
};


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


int sockprint(int fd,const char *format,...)
{
  int p;
  va_list ap;
  va_start(ap,format);
  p = vsnprintf(buffer, PACKETBUFF, format, ap);
  va_end(ap);
  p = send(fd, buffer, p, 0);
  return p;
}

int bncmsg(int fd, char *nick, const char *format,...)
{
  int p, c;
  va_list ap;
  
  c=snprintf(buffer, PACKETBUFF, hmsg[jack->mtype], nick);
  va_start(ap,format);
  
  p = vsnprintf(&buffer[c], PACKETBUFF - c, format, ap);
  va_end(ap);
  p = send(fd, buffer, p+c, 0);
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


int thestat(char *buf,int len, struct cliententry *list_ptr)
{
	int p,d;
	char *st[2] = 
	{
		"spunback",
		"SPUNBACK",
	};
	d=list_ptr->flags;
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

      st = (char *) malloc( sizeof(char) * strlen(wld) + 1);

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
  char salt[3];
  char *crypt ();

  if (pass[0] == '+')
  {
    pass = &pass[1];

    salt[0] = pass[0];
    salt[1] = pass[1];
    salt[2] = 0;
    encr = crypt (s, salt);
  }
  else
  {
    encr = s;
  }

  if (!strncmp (encr, pass, 10))
    return 1;
  else
    return 0;
}

int connokay (struct sockaddr_in sa, confetti * jr)
{
	struct hostent *hp = gethostbyaddr ((char *) &sa.sin_addr, sizeof (sa.sin_addr), AF_INET);

	accesslist *na;

	logprint(jack, "Connection from %s", inet_ntoa (sa.sin_addr));

	if (!jr->has_alist)
		return 1;
	for (na = jr->alist; na; na = na->next)
	{
		if (na->type == 1)
		{
			if (!match (inet_ntoa (sa.sin_addr), na->addr))
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




int wipeclient(struct cliententry *list_ptr)
{
	if(list_ptr->fd > -1)
	{
		close(list_ptr->fd);
	}
	if(list_ptr->sfd > -1)
	{
		close(list_ptr->sfd);
	}
	free(list_ptr);
	return 0;
}

int freeclientlist (struct cliententry *list_ptr)
{
  struct cliententry *hold_ptr;

  while (list_ptr != NULL)
  {
    hold_ptr = list_ptr->next;
    wipeclient(list_ptr);
    list_ptr = hold_ptr;
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

struct cliententry *getclient (struct cliententry *list_ptr, int nfd)
{
	while (list_ptr != NULL)
	{
		if (list_ptr->fd == nfd)
		{
			return list_ptr;
		}
		if (list_ptr->sfd == nfd)
		{
			return list_ptr;
		}
		list_ptr = list_ptr->next;
	}
	return NULL;
}

int countfds (struct cliententry *list_ptr)
{
	int p;

	p = 0;
	while (list_ptr != NULL)
	{
		list_ptr = list_ptr->next;
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

int do_connect (char *vhostname, char *hostname, u_short port, char *uname)
{
	int s;
	int f;
	struct hostent *he;
	struct sockaddr_in sin;

	f = 1;
	s = socket (AF_INET, SOCK_STREAM, 0);
	if (s < 0)
	{
		return -2;
	}
	memset (&sin, 0, sizeof (struct sockaddr_in));
	sin.sin_family = AF_INET;

	if( (vhostname == NULL))
	{
		f = 0;
	}
	else
	{
		if (strlen (vhostname) < 1)
		{
			f = 0;
		}
	}

	sin.sin_addr.s_addr = (f ? inet_addr (vhostname) : INADDR_ANY);
	if ((sin.sin_addr.s_addr == -1) && f)
	{
		he = gethostbyname (vhostname);
		if (he)
		{
			memcpy (&sin.sin_addr, he->h_addr, he->h_length);
		}
	}
	else
	{
		sin.sin_addr.s_addr = INADDR_ANY;
	}

	if (bind (s, (struct sockaddr *) &sin, sizeof (struct sockaddr_in)) < 0)
	{
		return -9;
	}
	memset (&sin, 0, sizeof (struct sockaddr_in));
	sin.sin_family = AF_INET;
	sin.sin_port = htons (port);
	sin.sin_addr.s_addr = inet_addr (hostname);
	if (sin.sin_addr.s_addr == -1)
	{
		he = gethostbyname (hostname);
		if (!he)
		{
			return -1;
		}
		memcpy (&sin.sin_addr, he->h_addr, he->h_length);
	}
	if (jack->identwd)
	{
		identwd_lock(&sin, port);
	}

	if (connect (s, (struct sockaddr *) &sin, sizeof (sin)) < 0)
	{
		close(s);
		return -2;
	}
	if (jack->identwd)
	{
		identwd_unlock(s, &sin, port, uname);
	}
	return s;
}


/* trust the caller of handleclient to filter \0 \n and \r
 * expects buflen be the len of the string, not counting the \0
 * also expects a \0 to be prepended.
 */ 

int handleclient (struct cliententry *list_ptr, int fromwho, int buflen, char *buf)
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
 	f=handlepclient(list_ptr, fromwho, pargc, pargv, prefix);
	if((list_ptr->flags & FLAGCONNECTED) && (f == 1))
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
			r = sockprint(list_ptr->sfd,"%s\n",buf);
		}
		else
		{
			r = sockprint(list_ptr->fd,"%s\n",buf);
		}
		if(r < 1)
		{
			return SERVERDIED;
		}
		f=0;
	}
	return f;
}


void initclient (struct cliententry *list_ptr, fd_set * nation)
{
	highfd = s_sock;
	FD_ZERO (nation);
	FD_SET (s_sock, nation);
	while (list_ptr != NULL)
	{
		FD_SET(list_ptr->fd, nation);
		if (list_ptr->fd > highfd)
		{
			highfd = list_ptr->fd;
		}
		if(list_ptr->flags & FLAGCONNECTED)
		{
			if( list_ptr->sfd > -1)
			{
				FD_SET(list_ptr->sfd,nation);
				if (list_ptr->sfd > highfd)
				{
					highfd = list_ptr->sfd;
				}
			}	
		}
		list_ptr = list_ptr->next;
	}
}


int scanclient (struct cliententry *list_ptr, fd_set * nation)
{
	int p,c,f,r;

	if (FD_ISSET (list_ptr->fd, nation))
	{
		r = recv (list_ptr->fd, allbuf, PACKETBUFF, 0);
		if(r <= 0)
		{
			return KILLCURRENTUSER;
		}
		for (p = 0; p < r; p++)
		{
			c=allbuf[p];
			switch(c)
			{
				case '\0':
				{
					break;
				}
				case '\r':
				case '\n':
				{
				        if(list_ptr->blen < BUFSIZE)
				        {
						list_ptr->biff[list_ptr->blen]=0;
						c=list_ptr->blen;
					}
					else
					{
						list_ptr->biff[BUFSIZE-1]=0;
						c=BUFSIZE-1;
					}
					list_ptr->blen=0;
					if(c > 0 )
					{
						f = handleclient (list_ptr, CLIENT, c, list_ptr->biff);
						if( f > 1)
						{
							return f;
						}
					}
					break;
				}
				default:
				{
				        if(list_ptr->blen < (BUFSIZE-1))
				        {
					  list_ptr->biff[list_ptr->blen++]=c;
					}
					break;
				}
			}
		}	
	}
	if(!(list_ptr->flags & FLAGCONNECTED))
	{
		return 0;
	}
	
	if(FD_ISSET (list_ptr->sfd, nation))
	{
		r = recv (list_ptr->sfd, allbuf, PACKETBUFF, 0);
		if (r <= 0)
		{
			if(list_ptr->flags & FLAGKEEPALIVE)
			{
				return SERVERDIED;
			}
			else
			{
				return KILLCURRENTUSER;
			}
		}
		/* new shiz */
		for (p = 0; p < r; p++)
		{
			c=allbuf[p];
			switch(c)
			{
				case '\0':
				{
					break;
				}
				case '\r':
				case '\n':
				{
				        if(list_ptr->slen < BUFSIZE)
				        {
						list_ptr->siff[list_ptr->slen]=0;
						c=list_ptr->slen;
					}
					else
					{
						list_ptr->siff[BUFSIZE-1]=0;
						c=BUFSIZE-1;
					}
					list_ptr->slen=0;
					if(c > 0 )
					{
						f = handleclient (list_ptr, SERVER, c, list_ptr->siff);
						if( f > 1)
						{
							return f;
						}
					}
					break;
				}
				default:
				{
				        if(list_ptr->slen < (BUFSIZE-1))
				        {
					  list_ptr->siff[list_ptr->slen++]=c;
					}
					break;
				}
			}
		}	
		/* end of new shiz */
		
		
/*		r=send (list_ptr->fd, allbuf, r, 0);
		if(r < 0)
		{
			return KILLCURRENTUSER;
		}
*/

	}
	return 0;
}

struct cliententry *chkclient (struct cliententry *list_ptr, fd_set * nation)
{
	int p,r;

	while (list_ptr != NULL)
	{
		p=scanclient(list_ptr,nation);

		if(p == KILLCURRENTUSER) /* self death */
		{
			return list_ptr;
		}
		if(p == SERVERDIED) /* keep alive */
		{
			r=sockprint(list_ptr->fd, "NOTICE AUTH :IRC quit, KeepAlive here.\n", list_ptr->nick);
			if(r < 0)
			{
				return list_ptr;
			}			
			if(list_ptr->flags & FLAGCONNECTED)
			{
				if( list_ptr->sfd > -1)
				{
					close(list_ptr->sfd);
					list_ptr->sfd=-1;
				}
				list_ptr->flags &= ~FLAGCONNECTED;
			}
		}
		list_ptr = list_ptr->next;
	}
  	return NULL;
}

void doclient(struct cliententry *list_ptr, fd_set * nation)
{
	int citizen, p,r;
	struct in_addr faddr;
	struct cliententry *bob;

	if (FD_ISSET (s_sock, nation))
	{
		citizen = accept (s_sock, (struct sockaddr *) &muhsin, &sinlen);
		if (citizen >= 0)
		{
			if (jack->maxusers)
			{
				if (countfds (headclient) + 1 > jack->maxusers)
				{
					close (citizen);
					return;
				}
			}
			p = sizeof (muhsin);
			r=getpeername (citizen, (struct sockaddr *) &muhsin, &p);
			if(r)
			{
				close(citizen);
				return;
			}
			if (!connokay (muhsin, jack))
			{
				close (citizen);
				return;
			}
			bob = (struct cliententry *) malloc( sizeof( struct cliententry ));
			memset(bob, 0, sizeof(struct cliententry));
			if( bob == NULL)
			{
				close (citizen);
				return;
			}
			bob->fd = citizen;
			bob->sfd = -1;
			bob->prev=NULL;
			bob->next=headclient;
			if( headclient != NULL)
				headclient->prev=bob;
			headclient=bob;
			strncpy(bob->vhost, jack->vhostdefault, HOSTLEN);
			bob->vhost[HOSTLEN]='\0';
			strcpy(bob->nick,"UNKNOWN");

			faddr = muhsin.sin_addr;
			strncpy (bob->fromip, gethost (&faddr), HOSTLEN);
			bob->fromip[HOSTLEN]='\0';

			if (jack->dpassf == 0)
			{
				bob->flags |= FLAGPASS;
			}
			bob->blen=0;
		}
	}
	/* normal connection stuff */
	bob = chkclient (list_ptr, nation);
	if(bob == NULL)
		return;

	if(bob->prev == NULL)
		headclient=bob->next;
	else
		bob->prev->next=bob->next;

	wipeclient(bob);	
		
}


int findbad(struct cliententry *list_ptr, fd_set * nation)
{
	int p;

	struct timeval thearf;
	thearf.tv_sec=0;
	thearf.tv_usec=0;
	while (list_ptr != NULL)
	{
		FD_ZERO(nation);
		FD_SET(list_ptr->fd, nation);
		
		if((p=select(list_ptr->fd+1, nation, (fd_set *) 0, (fd_set *) 0, &thearf )) < 0)
		{
			if(errno == EBADF)
			{ /* this is the bad boy. */
				if(list_ptr->prev == NULL)
					headclient=list_ptr->next;
				else
					list_ptr->prev->next=list_ptr->next;
					
				wipeclient(list_ptr);
				return 1;
			}
		
		}
		
		if(list_ptr->flags & FLAGCONNECTED)
		{
			if( list_ptr->sfd > -1)
			{
				FD_ZERO(nation);
				FD_SET(list_ptr->sfd, nation);
				if((p=select(list_ptr->fd+1, nation, (fd_set *) 0, (fd_set *) 0, &thearf)) < 0)
				{
					if(errno == EBADF)
					{ /* this is the bad boy. */
						close(list_ptr->sfd);
						list_ptr->sfd=-1;
						return 1;
					}
				}
			}	
		}
		list_ptr = list_ptr->next;
	}
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
  int p;
  fd_set nation;

  jack = jr;
  ENDLESSLOOP
  {
    initclient(headclient, &nation);
    if ((p = select (highfd + 1, &nation, (fd_set *) 0, (fd_set *) 0, NULL)) < 0)
    {
    	if(errno == EBADF)
    	{
    		p=findbad(headclient,&nation);
    	}
    	if(errno == ENOMEM)
    	{
		bnckill (SELECTERR);
	}
    }
    doclient (headclient, &nation);
  }
  return 0;
}


