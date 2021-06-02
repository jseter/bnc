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

#ifdef HAVE_SSL
#include <openssl/crypto.h>
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#endif

#include "sbuf.h"
#include "struct.h"
#include "send.h"
#include "ctcp.h"

extern struct ldcc *headldcc;


int ctsrv_dcc(struct cliententry *cptr, char *prefix, char *to, int fromwho);

struct ctcmd
{
	char *name;
	int (*func)(struct cliententry *cptr, char *prefix, char *to, int fromwho);
	unsigned int flags_on;
	unsigned int flags_off;
};

struct ctcmd ctsrvtab[] =
{
#ifdef DCC_PROXY
	{ "DCC", ctsrv_dcc, FLAGCONNECTED, FLAGNONE },
#endif
	{ NULL, NULL, 0, 0}
};

struct ctcmd ctcmdtab[] =
{
	{ NULL, NULL, 0, 0}
};














char ctparbuf[512];
char *ctcur;
char *ctend;

int ct_load(char *msg)
{
	ctcur = ctparbuf;
	ctend = ctcur + 511;

	if(*msg != '\001')
		return FORWARDCMD;
	for(msg++;*msg; msg++)
	{
		if(*msg == '\001')
			break;
		if(ctcur > ctend)
			return FORWARDCMD;
		*ctcur++ = *msg;
	}
	ctend = ctcur;
	ctcur = ctparbuf;
	*ctend = '\0';
	return 0;
}

char *ct_getargstr(void)
{
	char *str;
	
	if(ctcur >= ctend)
		return NULL;
	str = ctcur;
	for(; ctcur < ctend; ctcur++)
	{
		if(*ctcur == ' ')
		{
			*ctcur = '\0';
			for(ctcur++;ctcur < ctend && *ctcur == ' '; ctcur++);
			break;
		}
	}

	return str;
}

char *ct_getqargstr(void)
{
	char *str;
	char *dest;
	int inq;
	
	if(ctcur >= ctend)
		return NULL;
	str = ctcur;
	dest = ctcur;
	inq = 0;

	for(; ctcur < ctend; ctcur++)
	{
		if(inq)
		{
			if(*ctcur == '"')
			{
				inq=0;
				continue;
			}
			*dest++ = *ctcur;
			continue;
		}
		else
		{
			if(*ctcur == '"')
			{
				inq = 1;
				continue;
			}
			if(*ctcur == ' ')
			{
				*dest = '\0';
				for(ctcur++;ctcur < ctend && *ctcur == ' '; ctcur++);
				break;
			}
			*dest++ = *ctcur;
		}
	}
	*dest = '\0';

	return str;	

}

char *ct_getallstr(void)
{
	char *str;
	if(ctcur >= ctend)
		return NULL;
	str = ctcur;
	ctcur = ctend;
	return str;	
}



int ct_handle(struct cliententry *cptr, char *prefix, char *to, char *msg, int fromwho)
{
	int res;
	char *cmdname;
	struct ctcmd *ctl;

//	printf("got here\n");
	res = ct_load(msg);
	if(res)
		return res;
	cmdname = ct_getargstr();
	if(cmdname == NULL)
		return FORWARDCMD;

//	printf("got ctcp '%s'\n", cmdname);
	switch(fromwho)
	{
		case CLIENT:
			ctl = ctcmdtab;
			break;
		case SERVER:
			ctl = ctsrvtab;
			break;
		default:
			return FORWARDCMD;
	}

	for(; ctl->name; ctl++)
	{
		if(!strcasecmp(cmdname, ctl->name))
		{
			if((ctl->flags_on & cptr->flags) == ctl->flags_on)
				if((ctl->flags_off & ~cptr->flags) == ctl->flags_off)
					return ctl->func(cptr, prefix, to, fromwho);
		}
	}
	
	
	
	return FORWARDCMD;
}


struct ctdcc
{
	char *name;
	int (*func)(struct cliententry *cptr, char *prefix, char *to, int fromwho);
};

int ctdcc_send(struct cliententry *cptr, char *prefix, char *to, int fromwho);
struct ctdcc ctdcctab[] =
{
	{ "SEND", ctdcc_send },
	{ NULL, NULL}
};

int ctdcc_send(struct cliententry *cptr, char *prefix, char *to, int fromwho)
{
	int res;
	char *fname;
	char *ip;
	char *port;
	char *len;
	struct ldcc *lptr;
	struct lsock *mptr;
	
	struct sockaddr_in sin;
	struct sockaddr_in lsin;
	socklen_t lsinlen;

	
	fname = ct_getqargstr();
	if(fname == NULL)
		return FORWARDCMD;
	ip = ct_getargstr();
	if(ip == NULL)
		return FORWARDCMD;
	port = ct_getargstr();
	if(ip == NULL)
		return FORWARDCMD;
	len = ct_getargstr();
	/* optional */

	res = inet_aton(ip, &sin.sin_addr);
	if(res == 0)
		return FORWARDCMD;

	sin.sin_port = htons(atoi(port));
	lptr = malloc(sizeof(*lptr));
	if(lptr == NULL)
		return FORWARDCMD;
		
	memset(lptr, 0, sizeof(*lptr));
	sin.sin_family = AF_INET;
	lptr->sin = *(struct sockaddr *)&sin;
	lptr->sinlen = sizeof(sin);

	
	lptr->fd = socket(AF_INET, SOCK_STREAM, 0);
	if(lptr->fd == -1)
	{
		free(lptr);
		return FORWARDCMD;
	}

	lsinlen = sizeof(lsin);

	res = getsockname(cptr->srv.fd, (struct sockaddr *)&lsin, &lsinlen);
	if(res == -1)
		lsin.sin_addr.s_addr = INADDR_ANY;
	lsin.sin_family = AF_INET;
	lsin.sin_port = htons(0);
	
	res = bind (lptr->fd, (struct sockaddr *) &lsin, sizeof (struct sockaddr_in));
	if(res == -1)
	{
		close(lptr->fd);
		free(lptr);
		return FORWARDCMD;
		
	}

	lsinlen = sizeof(lsin);
	res = getsockname(lptr->fd, (struct sockaddr *)&lsin, &lsinlen);
	if(res == -1)
	{
		close(lptr->fd);
		free(lptr);
		return FORWARDCMD;
	}

	res = listen(lptr->fd, 1);
	if(res == -1)
	{
		close(lptr->fd);
		free(lptr);
		return FORWARDCMD;
	}

//	close(lptr->fd);
//	free(lptr);

	lptr->next = headldcc;
	headldcc = lptr;
	
//	printf("%s %s DCC SEND '%s' '%s(%s)' '%s' '%s'\n", prefix, to, fname, ip, inet_ntoa(sin.sin_addr), port, len);

//	printf("%s %i\n", inet_ntoa(lsin.sin_addr), ntohs(lsin.sin_port));

	if(fromwho == CLIENT)
		mptr = &cptr->srv;
	else
		mptr = &cptr->loc;
		
	if(prefix)
		tprintf(mptr, ":%s ", prefix);

	tprintf(mptr, "PRIVMSG %s :\001DCC SEND \"%s\" %lu %hu", to, fname, ntohl(*(unsigned long *)&lsin.sin_addr), ntohs(lsin.sin_port));
	if(len)
		tprintf(mptr, " %s", len);
	tprintf(mptr, "\001\r\n");
	return 0;	
	
//	return FORWARDCMD;
}

#ifdef DCC_PROXY
int ctsrv_dcc(struct cliententry *cptr, char *prefix, char *to, int fromwho)
{
	char *cmdname;
	struct ctdcc *ctd;
	cmdname = ct_getargstr();

//	printf("DCC command '%s'\n", cmdname);
	for(ctd = ctdcctab; ctd->name; ctd++)
	{
		if(!strcasecmp(ctd->name, cmdname))
			return ctd->func(cptr,prefix, to, fromwho);
	}
	return FORWARDCMD;
}
#endif
