#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
                     
#include "common.h"

extern int mytoi(char *buf);
extern void bnckill (int reason);
extern int bewmstick (void);
extern int sockprint(int fd,const char *format,...);
extern int bncmsg(int fd, char *nick, const char *format,...);
extern int logprint(confetti *jr,const char *format,...);
extern int passwordokay (char *s, char *pass);
extern int do_connect (char *vhostname, char *hostname, u_short port, char *uname);
extern int thestat(char *buf,int len, struct cliententry *list_ptr);
extern struct cliententry *getclient(struct cliententry *list_ptr, int nfd);
extern void add_access (confetti *, accesslist *);
extern int wipeclient(struct cliententry *list_ptr);
extern confetti *jack;
extern int chanlist(char *buf,int len, struct cliententry *client);
extern struct cliententry *headclient;
extern void *pmalloc(size_t size);
extern unsigned char touppertab[];
extern unsigned char tolowertab[];


BUILTIN_CTCPCMD(svrctcp_version);

ctcpfuncs ctcpcmds[] =
{
	{ "VERSION", svrctcp_version, FLAGCONNECTED | FLAGDOCKED, FLAGNONE },
	{ NULL, NULL, 0, 0 }
};

int handlectcp(struct cliententry *list_ptr, unsigned char *fromwho, unsigned char *prefix, int pargc, unsigned char **pargv)
{
	int r;
	ctcpfuncs *ctcpcmd;
	
	r=FORWARDCMD;
	for(ctcpcmd=ctcpcmds;ctcpcmd->name;ctcpcmd++)
	{
		if(!strcasecmp(pargv[0],ctcpcmd->name))
		{
			if( (ctcpcmd->flags_on & list_ptr->flags ) == ctcpcmd->flags_on )
			{
				if( (ctcpcmd->flags_off & ~list_ptr->flags ) == ctcpcmd->flags_off)
				{
					r=ctcpcmd->func(list_ptr, fromwho, prefix, pargc, pargv);
				}
			}
			break;
		}
	}
	
	return r;
}


BUILTIN_CTCPCMD(svrctcp_version)
{
	int r;
	char *s="Docked BNC client";
	if(list_ptr->version == NULL)
	{
		list_ptr->version = s;
	}
	r=sockprint(list_ptr->sfd, "NOTICE %s :\001VERSION %s\001\n", fromwho, list_ptr->version);	
	if(r < 0)
	{
		return KILLCURRENTUSER;
	}
	return 0;
}