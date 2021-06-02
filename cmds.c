#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
                     
#include "common.h"

#define MAXMOTDLINE 512
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
extern int mytoi(char *buf);
extern unsigned char touppertab[];
extern unsigned char tolowertab[];

unsigned char motdb[MAXMOTDLINE];

char *helplist[] =
{
  "COMMANDS are /quote:     ex.) /quote conn bnc.irc.net 6667",
  "MAIN <Supervisor password>",
  "  Identifies you as a supervisor, enabling admin commands",
  "VIP [new virtual host]",
  "  /quote VIP alone will list current vhosts in the config file",
  "IDENT <newident>",
  "  If your shell has identwd installed, bnc will take advantage of its features, changing your ident",
  "KEEPALIVE",
  "  returns you to bnc if a shell closes you (EXPERIMENTAL)",
  "CONN <server address> [port] [pass]",
  "  Connects you to a real irc server",
  "VDF",
  "  Switches your vhost back to the config default",
  "VN",
  "  Switches your vhost to the shells default",

  NULL
};
char *helplista[] =  
{
  "-----ADMIN *ONLY* COMMANDS----",
  "BWHO",
  "  Lists all clients using BNC",
  "BKILL <FD>",
  "  Closes one of the clients with that specified FD",
  "DIE",
  "  Shuts down BNC",
  "BDIE",
  "  alternative shutdown",
  "ADDHOST <type> <address>",
  "  Adds an allow for an IP to use your BNC",
  "LISTHOST",
  "  Lists allowed IP hosts",
  NULL
};




BUILTIN_COMMAND(cmd_quit);
BUILTIN_COMMAND(cmd_nick);
BUILTIN_COMMAND(cmd_pass);
BUILTIN_COMMAND(cmd_user);
BUILTIN_COMMAND(cmd_help);
BUILTIN_COMMAND(cmd_main);
BUILTIN_COMMAND(cmd_conn);
BUILTIN_COMMAND(cmd_ident);
BUILTIN_COMMAND(cmd_vn);
BUILTIN_COMMAND(cmd_vdf);
BUILTIN_COMMAND(cmd_vip);
BUILTIN_COMMAND(cmd_who);
BUILTIN_COMMAND(cmd_die);
BUILTIN_COMMAND(cmd_bdie);
BUILTIN_COMMAND(cmd_bkill);
BUILTIN_COMMAND(cmd_addhost);
BUILTIN_COMMAND(cmd_listhost);
BUILTIN_COMMAND(cmd_keepalive);
BUILTIN_COMMAND(cmd_rawecho);
BUILTIN_COMMAND(cmd_bmsg);
BUILTIN_COMMAND(cmd_prefixrawecho);
BUILTIN_COMMAND(cmd_dock);
BUILTIN_COMMAND(cmd_resume);
BUILTIN_COMMAND(cmd_resumealive);
BUILTIN_COMMAND(cmd_dumpll);
BUILTIN_COMMAND(cmd_bypass);

BUILTIN_COMMAND(srv_nick);
BUILTIN_COMMAND(srv_tellnick);
BUILTIN_COMMAND(srv_join);
BUILTIN_COMMAND(srv_kick);
BUILTIN_COMMAND(srv_part);
BUILTIN_COMMAND(srv_ping);
BUILTIN_COMMAND(srv_endmotd);

cmdstruct serverbnccmds[] =
{
	{ "NICK", srv_nick, FLAGCONNECTED, FLAGNONE },
	{ "PING", srv_ping, FLAGCONNECTED | FLAGDOCKED, FLAGNONE },
	{ "004", srv_tellnick, FLAGCONNECTED, FLAGNONE },
	{ "JOIN", srv_join, FLAGCONNECTED, FLAGNONE },
	{ "KICK", srv_kick, FLAGCONNECTED, FLAGNONE },
	{ "PART", srv_part, FLAGCONNECTED, FLAGNONE },
	{ "376", srv_endmotd, FLAGCONNECTED, FLAGNONE },
	{NULL, NULL, 0,0}
};

cmdstruct clientbnccmds[] =
{
	{ "QUIT", cmd_quit, FLAGNONE, FLAGCONNECTED },
	{ "PASS", cmd_pass, FLAGNONE, FLAGCONNECTED | FLAGPASS },
	{ "NICK", cmd_nick, FLAGNONE, FLAGCONNECTED },
 	{ "USER", cmd_user, FLAGNONE, FLAGCONNECTED },
 	{ "HELP", cmd_help, FLAGPASS, FLAGCONNECTED },
 	{ "BNCHELP", cmd_help, FLAGPASS, FLAGNONE },
 	{ "MAIN", cmd_main, FLAGNONE, FLAGNONE },
 	{ "CONN", cmd_conn, FLAGBASED, FLAGNONE },
 	{ "IDENT", cmd_ident, FLAGPASS, FLAGNONE },
 	{ "VN", cmd_vn, FLAGPASS, FLAGNONE },
 	{ "VDF", cmd_vdf, FLAGPASS, FLAGNONE },
 	{ "VIP", cmd_vip, FLAGPASS, FLAGNONE },
 	{ "BWHO", cmd_who, FLAGSUPER, FLAGNONE },
 	{ "DIE", cmd_die, FLAGSUPER, FLAGNONE },
 	{ "BDIE", cmd_bdie, FLAGSUPER, FLAGNONE },
 	{ "BKILL", cmd_bkill, FLAGSUPER, FLAGNONE },
 	{ "ADDHOST", cmd_addhost, FLAGSUPER, FLAGNONE },
 	{ "LISTHOST", cmd_listhost, FLAGSUPER, FLAGNONE },
 	{ "KEEPALIVE", cmd_keepalive, FLAGPASS, FLAGNONE },
 	{ "RAWECHO", cmd_rawecho, FLAGPASS, FLAGNONE },
 	{ "BMSG", cmd_bmsg, FLAGPASS, FLAGNONE },
 	{ "PRE", cmd_prefixrawecho , FLAGPASS | FLAGBASED, FLAGNONE },
 	{ "DOCK", cmd_dock, FLAGCONNECTED, FLAGNONE },
 	{ "DETACH", cmd_dock, FLAGCONNECTED, FLAGNONE },
 	{ "RESUME", cmd_resume, FLAGPASS, FLAGCONNECTED },
 	{ "RESUME", cmd_resumealive, FLAGPASS, FLAGNONE },
 	{ "DUMPLL", cmd_dumpll, FLAGNONE, FLAGNONE },
 	{ "BYPASS", cmd_bypass, FLAGCONNECTED, FLAGNONE },
	{ NULL, NULL, 0, 0 }
};

int remnl (char *buf, int size)
{
	int p;

	for (p = 0; p < size; p++)
	{
		if (buf[p] == '\0')
		{
			return p;
		}
		if((buf[p] == '\n') || (buf[p] == '\r'))
		{
			buf[p] = '\0';
			return p;
		}
	}
	return p;
}


int irc_strcasecmp(const char *s1, const char *s2)
{
	int x;
	
	if(s1 == NULL)
	{
		return 1;
	}
	if(s2 == NULL)
	{
		return 1;
	}

	for(;*s1 && *s2; s1++, s2++)
	{
		x = touppertab[(unsigned char)*s1] - touppertab[(unsigned char)*s2];
		
		if(x)
		{
			return x;
		}
	}
	/* should not need toupper or anything, since they both should be 0 */
	if(*s1 == *s2)
	{
		return 0;
	}
	return 1;
}
int wipechans(struct cliententry *list_ptr)
{
	struct chanentry *ochan;
	struct chanentry *chanlist;
	
	chanlist=list_ptr->headchan;
	while(chanlist)
	{
		ochan=chanlist;
		chanlist=chanlist->next;
		
		free(ochan);
	}
	list_ptr->headchan=NULL;
	return 0;
}

struct chanentry *findchan(struct chanentry *chanlist, char *chan)
{
	while(chanlist)
	{
		if(!irc_strcasecmp(chanlist->chan,chan))
		{
			return chanlist;
		}
		chanlist=chanlist->next;
	}
	return NULL;
}

int wipechan(struct cliententry *list_ptr, char *chan)
{
	struct chanentry *chanlist;

	if(list_ptr == NULL)
	{
		return 1;
	}
	
	chanlist=findchan(list_ptr->headchan,chan);

	if(chanlist == NULL)
	{
		return 1;
	}
	
	if(chanlist->prev)
	{
		chanlist->prev->next = chanlist->next;
	}
	else
	{
		list_ptr->headchan=chanlist->next;
	}
			
	if(chanlist->next)
	{
		chanlist->next->prev=chanlist->prev;
	}
			
	free(chanlist);
	return 0;
}


int getnickuserhost(char **argv,char *buf,char *fix)
{
	int p,c;
	c=0;
	argv[0]=buf;

	for(p=0;buf[p];p++)
	{
		if(buf[p] == '!')
		{
			buf[p]='\0';
			fix[c++]='!';
			argv[1]=&buf[p+1];
		}
		if(buf[p] == '@')
		{
			buf[p]='\0';
			fix[c++]='@';
			argv[2]=&buf[p+1];
		}
	}
	return c;
}

int ismenuh(char *prefix, char *nick)
{
	int p,repc,c,f,m;
	char repv[3];
	char *nuh[3];

	if(prefix == NULL)
	{
		return 0;
	}

	m=0;
	c=strlen(prefix);
	
	repc = getnickuserhost(nuh, prefix, repv);
	
	
	if(!strncasecmp(nick, nuh[0],NICKLEN))
	{
		m=1;
	}
	f=0;
	for(p=0;p<c;p++)
	{
		if( prefix[p] == '\0' )
		{
			if(repc > 0)
			{
				prefix[p]=repv[f++];
				repc--;
			}
		}
	}
	return m;
}

int irc_connect(struct cliententry *list_ptr, char *server, u_short port, char *pass)
{
	int cs,r;
	
	list_ptr->flags &= ~FLAGAUTOCONN;
	list_ptr->susepass=0;
	
	if(list_ptr->flags & FLAGCONNECTED)
	{
		r=sockprint(list_ptr->fd, "NOTICE AUTH :Disconnecting old\n", server, port);
		if(r < 0)
		{
			return KILLCURRENTUSER;
		}
		if(list_ptr->sfd > -1)
		{
			close(list_ptr->sfd);
			list_ptr->sfd=-1;
		}
		list_ptr->flags &= ~FLAGCONNECTED;
	}
	
	r=sockprint(list_ptr->fd, "NOTICE AUTH :Making reality through %s port %i\n", server, port);
	if(r < 0)
	{
		return KILLCURRENTUSER;
	}
	cs = do_connect (list_ptr->vhost, server, port, list_ptr->uname);
	if (cs < 0)
	{
		if (cs == -9)
		{
			r=sockprint(list_ptr->fd, "NOTICE AUTH :Failed Connection (Supplied Vhost is not on this system)\n");
			
		}
		else
		{
			r=sockprint(list_ptr->fd, "NOTICE AUTH :Failed Connection\n");
		}
		if(r < 0)
		{
			return KILLCURRENTUSER;
		}
		return -1;
	}
	r=sockprint(list_ptr->fd, "NOTICE AUTH :Suceeded connection\n");
	if(r < 0)
	{
		return KILLCURRENTUSER;
	}
	logprint(jack, "(%i) %s!%s@%s connected to %s", list_ptr->fd, list_ptr->nick, list_ptr->uname, list_ptr->fromip, server);

	if (pass != NULL)
	{
		r=sockprint(cs, "PASS :%s\n", pass);
		if(r < 0)
		{
			return KILLCURRENTUSER;
		}
	}
	strncpy (list_ptr->onserver, server, HOSTLEN);
	list_ptr->onserver[HOSTLEN]='\0';
	strncpy (list_ptr->sid, server, HOSTLEN);
	list_ptr->sid[HOSTLEN]='\0';
	r=sockprint(cs, "NICK %s\n", list_ptr->nick);
	if(r < 0)
	{
		return KILLCURRENTUSER;
	}
	r=sockprint(cs, "USER %s \"%s\" \"%s\" :%s\n", list_ptr->uname, list_ptr->fromip, list_ptr->onserver, list_ptr->realname);
	if(r < 0)
	{
		return KILLCURRENTUSER;
	}
	list_ptr->sfd = cs;
	list_ptr->flags |= FLAGCONNECTED;
	return 0;

}



int handlepclient (struct cliententry *list_ptr, int fromwho, int pargc, char **pargv, char *prefix)
{
	int p,f,r,w;
	FILE *motdf;
	cmdstruct *bnccmds;

	f=0;
	p=0;
	w=1;
	if(fromwho == CLIENT)
	{
		bnccmds=clientbnccmds;
	}
	else
	{
		bnccmds=serverbnccmds;
	}

	while(bnccmds[p].name != NULL)
	{
		if(!strcasecmp(pargv[0],bnccmds[p].name))
		{
			/* lets check flags */
			if( (bnccmds[p].flags_on & list_ptr->flags) == bnccmds[p].flags_on)
			{
				if( (bnccmds[p].flags_off & ~list_ptr->flags) == bnccmds[p].flags_off)
				{
					w=0;
					f=bnccmds[p].func(list_ptr, prefix, pargc, pargv);
					break;
				}
			}
		}
		p++;
	}
	if(f > 0)
	{
		return f;
	}	

	if( (list_ptr->flags & ( FLAGNICK | FLAGUSER | FLAGPASS)) != ( FLAGNICK | FLAGUSER | FLAGPASS) )
	{
		return w;
	}

	if( !( list_ptr->flags & FLAGBASED ))
	{
		list_ptr->flags |= FLAGBASED;
		if (list_ptr->flags & FLAGAUTOCONN)
		{
			/* handle the autoconn stuff, disabled for now */
			if (list_ptr->susepass)
			{
				r=irc_connect(list_ptr, list_ptr->autoconn, list_ptr->sport, list_ptr->autopass);
			}
			else
			{
				r=irc_connect(list_ptr, list_ptr->autoconn, list_ptr->sport, NULL);
			}
			if(r > 1)
			{
				return r;
			}
 			
			return w;

		}
		r=sockprint(list_ptr->fd, "NOTICE AUTH :Welcome to BNC " VERSION ", the irc proxy\n");

		if(r < 0)
		{
			return KILLCURRENTUSER;
		}		
		if( (list_ptr->flags & FLAGSUPER) == 0)
		{      
			motdf = fopen (jack->motdf, "r");
			if(motdf != NULL)
			{
				while (!feof (motdf))
				{
					memset(motdb,0,MAXMOTDLINE);
					fgets (motdb,MAXMOTDLINE, motdf);
					motdb[MAXMOTDLINE]='\0';
					p=remnl (motdb,MAXMOTDLINE);
					motdb[MAXMOTDLINE]='\0';
				
					if(p > 0)
					{
						r=sockprint(list_ptr->fd, "NOTICE AUTH :-*- %s\n", motdb);
						if(r < 0)
						{
							return KILLCURRENTUSER;
						}
					}
				}
				fclose (motdf);
			}
		}
		r=sockprint(list_ptr->fd, "NOTICE AUTH :Level two, lets connect to something real now\n");
		if(r < 0)
		{
			return KILLCURRENTUSER;
		}		
	        r=sockprint(list_ptr->fd, "NOTICE AUTH :type /quote conn [server] <port> <pass> to connect\n");
		if(r < 0)
		{
			return KILLCURRENTUSER;
		}	        
		r=sockprint(list_ptr->fd, "NOTICE AUTH :type /quote help for basic list of commands and usage\n");
		if(r < 0)
		{
			return KILLCURRENTUSER;
		}
	}
	return w;
}

BUILTIN_COMMAND(srv_ping) 
{
	int r;
	if(pargc < 2)
	{
		return 0;
	}
	r=sockprint(list_ptr->sfd, "PONG :%s\n", pargv[1] );
	if(r < 0)
	{
		return SERVERDIED;
	}
	return 0; /* we don't wanna forward anything when docked */
	
}

BUILTIN_COMMAND(srv_part)
{
	int m;
	if(pargc < 2)
	{
		return FORWARDCMD;
	}
	m=ismenuh(prefix,list_ptr->nick);
	if(!m) /* its not me, so forget it */
	{
		return FORWARDCMD;
	}
	wipechan(list_ptr,pargv[1]);
	return FORWARDCMD;
}

BUILTIN_COMMAND(srv_kick)
{
	if(pargc < 3)
	{
		return FORWARDCMD;
	}
	if(!strncasecmp(list_ptr->nick, pargv[2] ,NICKLEN))
	{
		wipechan(list_ptr, pargv[1]);
	}

	return FORWARDCMD;
}

BUILTIN_COMMAND(srv_endmotd)
{
	int r;
	struct chanentry *chanlist;
	if(list_ptr->docked == 1) /* ok resuming is almost done */
	{
		list_ptr->docked = 0;
		chanlist=list_ptr->headchan;
		while(chanlist)
		{
			r=sockprint(list_ptr->fd,":%s!%s@%s JOIN %s\n",list_ptr->nick, list_ptr->uname, list_ptr->fromip, chanlist->chan );
			if(r < 0)
			{
				return KILLCURRENTUSER;
			}
			
			r=sockprint(list_ptr->sfd,"NAMES %s\n",chanlist->chan );
			if(r < 0)
			{
				return KILLCURRENTUSER;
			}
			
			
			chanlist=chanlist->next;
		}
	}
	return FORWARDCMD;
}

BUILTIN_COMMAND(srv_join)
{
	int r,p,m;
	struct chanentry *chan;
	if(pargc < 2)
	{
		return FORWARDCMD;
	}
	m=ismenuh(prefix,list_ptr->nick);
	if(!m)
	{
		return FORWARDCMD;
	}
	
	chan=findchan(list_ptr->headchan,pargv[1]);
	if(chan != NULL)
	{
		return FORWARDCMD;
	}
	
	p=strlen(pargv[1]);
	chan=pmalloc(sizeof(struct chanentry)+p+1);
	memset(chan->chan,0,p+1);

	for(r=0;p>0;p--)
	{
		chan->chan[r]=pargv[1][r];
		r++;
	}
	chan->prev=0;
	chan->next=list_ptr->headchan;
	if(list_ptr->headchan)
	{
		list_ptr->headchan->prev=chan;
	}
	list_ptr->headchan=chan;	
	
	return FORWARDCMD;
}

BUILTIN_COMMAND(srv_nick)
{
	int p,repc,c,f;
	char repv[3];
	char *nuh[3];
	char *arf="UNKNOWN";
	nuh[0]=arf;
	nuh[1]=arf;
	nuh[2]=arf;
	
	if(pargc < 2)
	{
		return 0;
	}
	if(prefix == NULL)
	{
		return 0;
	}
	c=strlen(prefix);
	
	repc = getnickuserhost(nuh, prefix, repv);
	
	
	if(!strncasecmp(list_ptr->nick, nuh[0],NICKLEN))
	{
		strncpy( list_ptr->nick, pargv[1], NICKLEN);
		list_ptr->nick[NICKLEN]='\0';
	}
	f=0;
	for(p=0;p<c;p++)
	{
		if( prefix[p] == '\0' )
		{
			if(repc > 0)
			{
				prefix[p]=repv[f++];
				repc--;
			}
		}
	}
	
 	return FORWARDCMD;
	
}

BUILTIN_COMMAND(srv_tellnick)
{
	if(pargc < 2)
	{
		return 0;
	}
	strncpy(list_ptr->nick,pargv[1],NICKLEN);
	list_ptr->nick[NICKLEN]='\0';
	if(prefix != NULL)
	{
		strncpy(list_ptr->sid,prefix,HOSTLEN);
		list_ptr->sid[HOSTLEN]='\0';
	}
	return FORWARDCMD;
}

BUILTIN_COMMAND(cmd_resumealive)
{
	list_ptr->flags &= ~FLAGDOCKED;
	return 0;
}

BUILTIN_COMMAND(cmd_resume)
{
	int r,sfd,m;
	struct cliententry *client;
	if(pargc < 3)
	{
		r=sockprint(list_ptr->fd,"NOTICE %s :Syntax, /quote resume dock_fd pass\n",list_ptr->nick);
		if(r < 0)
		{
			return KILLCURRENTUSER;
		}
		
		return 0;
	}
	sfd=mytoi(pargv[1]);
	client = getclient(headclient, sfd);
	m=0;
	if(client != NULL)
	{
		m=1;
		if(!(client->flags & FLAGDOCKED))
		{
			m=0;
		}
	}
	
	if(!m)
	{
		r=sockprint(list_ptr->fd,"NOTICE %s :Docked fd not found\n",list_ptr->nick);
		if(r < 0)
		{
			return KILLCURRENTUSER;
		}
		return 0;
	}
	
	if(!strncasecmp(client->autopass,pargv[2],PASSLEN))
	{
		r=sockprint(list_ptr->fd,"NOTICE %s :-*- Resuming session\n",list_ptr->nick);
		if(r < 0)
		{
			return KILLCURRENTUSER;
		}


		if( strncasecmp(client->nick, list_ptr->nick, NICKLEN) )
		{
			r=sockprint(list_ptr->fd,":%s@%s!%s NICK :%s\n",list_ptr->nick,list_ptr->uname,list_ptr->fromip, client->nick);
			if(r < 0)
			{
				return KILLCURRENTUSER;
			}
		}
		
		r=sockprint(list_ptr->fd, ":%s 001 %s :Welcome to a resumed bnc session\n", client->sid, client->nick);
		if(r < 0)
		{
			return KILLCURRENTUSER;
		}
		r=sockprint(list_ptr->fd, ":%s 002 %s :your host is %s, running an irc server\n", client->sid, client->nick, client->sid);
		if(r < 0)
		{
			return KILLCURRENTUSER;
		}
		r=sockprint(list_ptr->fd, ":%s 003 %s :%s runs docked bnc\n", client->sid, client->nick, client->sid);
		if(r < 0)
		{
			return KILLCURRENTUSER;
		}
		r=sockprint(list_ptr->fd, ":%s 004 %s %s 234123 _____ ______\n", client->sid, client->nick, client->sid);
		if(r < 0)
		{
			return KILLCURRENTUSER;
		}
		r=sockprint(client->sfd,"LUSERS\nMOTD\n");
		if(r < 0)
		{
			return SERVERDIED;
		}
		
		client->docked=1;
		client->fd=list_ptr->fd;
		client->flags &= ~FLAGDOCKED;
		list_ptr->fd=-1;
		list_ptr->sfd=-1;
		return KILLCURRENTUSER;
	
	}
	else
	{
		r=sockprint(list_ptr->fd,"NOTICE %s :incorrect resume pass\n",list_ptr->nick);
		if(r < 0)
		{
			return KILLCURRENTUSER;
		}
	}
	return 0;	
}

BUILTIN_COMMAND(cmd_dock)
{
	int r;
	if(pargc < 2)
	{
		r=sockprint(list_ptr->fd,"NOTICE %s :/quote DOCK pass\n",list_ptr->nick);
		if(r < 0)
		{
			return KILLCURRENTUSER;
		}
		return 0;
	}
	r=sockprint(list_ptr->fd,"NOTICE %s :To resume, /quote resume %i %s\n",list_ptr->nick,list_ptr->sfd,pargv[1]);
	if(r < 0)
	{
		return KILLCURRENTUSER;
	}
	strncpy(list_ptr->autopass, pargv[1], PASSLEN);
	list_ptr->autopass[PASSLEN]='\0';
	list_ptr->flags |= FLAGDOCKED;
	if(!(list_ptr->flags & FLAGKEEPALIVE))
	{
		close(list_ptr->fd);
		list_ptr->fd=DOCKEDFD;
	}
	return 0;
}

BUILTIN_COMMAND(cmd_quit)
{
	return KILLCURRENTUSER;
}

BUILTIN_COMMAND(cmd_dumpll)
{
	int r;
	struct cliententry *client_ptr;

	client_ptr = headclient;
	r=sockprint(list_ptr->fd, "NOTICE AUTH :Dumping Links.\n");
	if(r < 0)
	{
		return KILLCURRENTUSER;
	}
	while (client_ptr != NULL)
	{
		r=sockprint(list_ptr->fd, "NOTICE AUTH :%p<= %p => %p :(%i,%i)\n", client_ptr->prev, client_ptr, client_ptr->next, client_ptr->fd, client_ptr->sfd);
		if(r < 0)
		{
			return KILLCURRENTUSER;
		}
		client_ptr = client_ptr->next;
	}
	r=sockprint(list_ptr->fd, "NOTICE AUTH :End of Linked list.\n");
	if(r < 0)
	{
		return KILLCURRENTUSER;
	}
	
	return 0;
}

BUILTIN_COMMAND(cmd_rawecho)
{
	int r;
	if(pargc < 2)
	{
		return 0;
	}
	r=sockprint(list_ptr->fd,"%s\n",pargv[1]);
	if(r < 0)
	{
		return KILLCURRENTUSER;
	}
	return 0;
}

BUILTIN_COMMAND(cmd_prefixrawecho)
{
	int r;
	if(pargc < 2)
		return 0;
	if(prefix == NULL)
	{
		r=sockprint(list_ptr->fd,":%s!%s@%s %s\n", list_ptr->nick, list_ptr->uname, list_ptr->fromip, pargv[1]);
	}
	else
	{
		r=sockprint(list_ptr->fd,":%s %s\n", prefix, pargv[1]);
	}
	if(r < 0)
		return KILLCURRENTUSER;
	return 0;
}



BUILTIN_COMMAND(cmd_bypass)
{
	int r;
	if(pargc < 2)
	{
		return 0;
	}
	r=sockprint(list_ptr->sfd,"%s\n",pargv[1]);
	if(r < 0)
	{
		return SERVERDIED;
	}
	return 0;
}


BUILTIN_COMMAND(cmd_nick)
{
	int r;
	if(pargc < 2)
	{
		return 0;
	}
	strncpy (list_ptr->nick, pargv[1], NICKLEN);
	list_ptr->nick[NICKLEN]=0;
	list_ptr->flags |= FLAGNICK;
	if( (list_ptr->flags & FLAGPASS) == 0)
	{
		r=sockprint(list_ptr->fd,"NOTICE AUTH :You need to say /quote PASS <password>\n");
		if(r < 0)
		{
			return KILLCURRENTUSER;
		}
	}
	return 0;
}
BUILTIN_COMMAND(cmd_pass)
{
	int p,r;
	int iswhite;
	int sargc;
	char *sargv[4];
	
	if (pargc < 2)
	{
		return 0;
	}
	
	p = 0;
	sargc=1;
	sargv[0]=pargv[1];
	
	iswhite=0;
	for(p=0; pargv[1][p] ; p++)
	{
		if(sargc > 3)
		{
			iswhite=0; /* just be mean */
		}

		if(iswhite)
		{
			if( pargv[1][p] != ':' )
			{
				iswhite=0;
				sargv[sargc++]=&pargv[1][p];
			}
		}
		else 
		{
			if( pargv[1][p] == ':' )
			{
				iswhite=1;
				pargv[1][p]='\0';
			}
		}
	
	}

/*	
	printf("Pass line gave %i args\n",sargc);
	for(p=0;p<sargc;p++)
	{
		printf("(%i) %s\n",p,sargv[p]); 
	}
*/
	

	/* go back to the old way for now */
	if(passwordokay (sargv[0], jack->dpass))
	{
		list_ptr->flags |= FLAGPASS;
		if(sargc > 1)
		{
			list_ptr->flags |= FLAGAUTOCONN;
			strncpy (list_ptr->autoconn, sargv[1], HOSTLEN);
			list_ptr->autoconn[HOSTLEN]='\0';
			list_ptr->sport=jack->cport;
			if(sargc > 2) /* contains port */
			{
				list_ptr->sport=mytoi(sargv[2]);
			}
			if(sargc > 3) /* contains server pass */
			{
				strncpy (list_ptr->autopass, sargv[3], PASSLEN);
				list_ptr->autopass[PASSLEN]='\0';
				list_ptr->susepass=1;
			}
		}
	
		
		return 0;
	}
	list_ptr->pfails++;
	if (list_ptr->pfails > 2)
	{
		return KILLCURRENTUSER;
	}
	logprint(jack, "Failed pass from %s password %s", list_ptr->fromip, sargv[0]);
	if( list_ptr->flags & FLAGNICK )
	{
		r=sockprint(list_ptr->fd, "NOTICE AUTH :Failed Pass!!\n");
		if(r < 0)
		{
			return KILLCURRENTUSER;
		}
	}
	return 0;
}
BUILTIN_COMMAND(cmd_user)
{
	if (pargc < 5)
	{
		return 0;
	}

	strncpy (list_ptr->realname, pargv[4], REALLEN);
	list_ptr->realname[REALLEN]='\0';	

	strncpy (list_ptr->uname, pargv[1], USERLEN);
	list_ptr->uname[USERLEN]='\0';
	  
	list_ptr->flags |= FLAGUSER;
	return 0;
}
BUILTIN_COMMAND(cmd_help)
{
	int p,r;
	for (p = 0; helplist[p] != NULL; p++)
	{
		r=sockprint(list_ptr->fd,"NOTICE AUTH :*** %s\n", helplist[p]);
		if(r < 0)
		{
			return KILLCURRENTUSER;
		}
	}
	if(list_ptr->flags & FLAGSUPER)
	{
		for (p = 0; helplista[p] != NULL; p++)
		{
			r=sockprint(list_ptr->fd,"NOTICE AUTH :*** %s\n", helplista[p]);
			if(r < 0)
			{
				return KILLCURRENTUSER;
			}
		}
	}
	r=sockprint(list_ptr->fd, "NOTICE AUTH :*** For a detailed explanation of commands, consult the file README,\n");
	if(r < 0)
	{
		return KILLCURRENTUSER;
	}
	return 0;
}

BUILTIN_COMMAND(cmd_main)
{
	int r;
	if (pargc < 2)
	{
		return 0;
	}
	if (passwordokay (pargv[1], jack->spass))
	{
		list_ptr->flags |= FLAGSUPER;
		list_ptr->flags |= FLAGPASS;
		r=sockprint(list_ptr->fd, "NOTICE AUTH :Welcome Supervisor!!\n");
		if(r < 0)
		{
			return KILLCURRENTUSER;
		}
	    return 0;
	}
	logprint(jack, "Failed MAIN from %s", list_ptr->fromip);
	r=sockprint(list_ptr->fd, "NOTICE AUTH :Failed Main!!\n");
	if(r < 0)
	{
		return KILLCURRENTUSER;
	}

	return 0;
}
BUILTIN_COMMAND(cmd_conn)
{
	int cport,r;
	

	if (pargc < 2)
	{
		return 0;
	}
	if (pargc > 2)
	{
		cport = mytoi (pargv[2]);
	}
	else
	{
		cport = jack->cport;
	}
	if (pargc > 3)
	{
		r=irc_connect(list_ptr, pargv[1], cport, pargv[3]);
	}
	else
	{
		r=irc_connect(list_ptr, pargv[1], cport, NULL);
	}
	if(r > 1)
	{
		return r;
	}
	return 0;
}

BUILTIN_COMMAND(cmd_ident)
{
	int r;
	if (pargc < 2)
	{
		r=sockprint(list_ptr->fd, "NOTICE AUTH :current ident is %s\n", list_ptr->uname);
		if(r < 0)
		{
			return KILLCURRENTUSER;
		}
		return 0;
	}
	r=sockprint(list_ptr->fd, "NOTICE AUTH :changing ident from %s to %s\n", list_ptr->uname, pargv[1]);
	if(r < 0)
	{
		return KILLCURRENTUSER;
	}
	strncpy (list_ptr->uname, pargv[1], USERLEN);
	list_ptr->uname[USERLEN]='\0';
	return 0;
}
BUILTIN_COMMAND(cmd_vn)
{
	int r;
	r=sockprint(list_ptr->fd, "NOTICE AUTH :Nulling out Vhost to system internal default\n");
	if(r < 0)
	{
		return KILLCURRENTUSER;
	}
	memset (list_ptr->vhost, '\0',HOSTLEN);
	return 0;
}

BUILTIN_COMMAND(cmd_vdf)
{
	int  r;
	if (strlen (jack->vhostdefault) < 1)
	{
		r=sockprint(list_ptr->fd, "NOTICE AUTH :Switching Vhost back to default\n");
	}
	else
	{
		r=sockprint(list_ptr->fd, "NOTICE AUTH :Switching Vhost back to default (%s)\n", jack->vhostdefault);
	}
	if(r < 0)
	{
		return KILLCURRENTUSER;
	}
	strncpy (list_ptr->vhost, jack->vhostdefault, HOSTLEN);
	list_ptr->vhost[HOSTLEN]='\0';
	return 0;
}
BUILTIN_COMMAND(cmd_vip)
{
	int f,r;
	struct vhostentry *vhost_ptr;
	struct hostent *he;
	struct in_addr addr;
	
	f = 0;
	if (pargc > 1)
	{
		he = gethostbyname (pargv[1]);
		if (he)
		{
			memcpy (&addr.s_addr, he->h_addr, sizeof (addr.s_addr));
			strncpy (list_ptr->vhost, pargv[1], HOSTLEN);
			list_ptr->vhost[HOSTLEN]='\0';
			r=sockprint(list_ptr->fd, "NOTICE AUTH :Switching Vhost to %s (%s)\n", list_ptr->vhost, inet_ntoa (addr));
		}
		else
		{
			r=sockprint(list_ptr->fd, "NOTICE AUTH :Vhost %s invalid\n", pargv[1]);
		}
		if(r < 0)
		{
			return KILLCURRENTUSER;
		}

	}
	else
	{	
		if (strlen (list_ptr->vhost) != 0)
		{
			r=sockprint(list_ptr->fd, "NOTICE AUTH :Default Vhost: %s\n", list_ptr->vhost);
		}
		else
		{
			r=sockprint(list_ptr->fd, "NOTICE AUTH :Default Vhost: -SYSTEM DEFAULT-\n");
		}
		if(r < 0)
		{
			return KILLCURRENTUSER;
		}
		r=sockprint(list_ptr->fd, "NOTICE AUTH :Listing Vhosts\n");
		if(r < 0)
		{
			return KILLCURRENTUSER;
		}
		vhost_ptr=jack->vhostlist;
	      
		while (vhost_ptr != NULL)
		{
			r=sockprint(list_ptr->fd, "NOTICE AUTH : (%i) %s\n", f, vhost_ptr->vhost);
			if(r < 0)
			{
				return KILLCURRENTUSER;
			}
			f++;
			vhost_ptr = vhost_ptr->next;
		}
		r=sockprint(list_ptr->fd, "NOTICE AUTH :End of Vhost list\n");
		if(r < 0)
		{
			return KILLCURRENTUSER;
		}
	}
	return 0;
}
BUILTIN_COMMAND(cmd_who)
{
	int r;
	int nchans;
	char chans[128+1];
	
	
	struct cliententry *client_ptr;
	char st[11];

	client_ptr = headclient;
	r=sockprint(list_ptr->fd, "NOTICE AUTH :Listing users.\n");
	if(r < 0)
	{
		return KILLCURRENTUSER;
	}
	while (client_ptr != NULL)
	{
		thestat(st,11, client_ptr);
		if( client_ptr->flags & FLAGCONNECTED )
		{
			nchans=chanlist(chans,128,client_ptr);
			if( client_ptr->flags & FLAGDOCKED )
			{
				r=sockprint(list_ptr->fd, "NOTICE AUTH :(DOCKED FD %i Status: %s)[%s@%s] on server (%s) %s\n", client_ptr->sfd, st, client_ptr->nick, client_ptr->fromip, client_ptr->sid, client_ptr->onserver);
			}
			else
			{
				r=sockprint(list_ptr->fd, "NOTICE AUTH :(FD %i Status: %s)[%s@%s] on server (%s) %s\n", client_ptr->fd, st, client_ptr->nick, client_ptr->fromip, client_ptr->sid, client_ptr->onserver);
			}
			if(r < 0)
			{
				return KILLCURRENTUSER;
			}
			if(nchans)
			{
				r=sockprint(list_ptr->fd, "NOTICE AUTH :CHANLIST: %s\n", chans );
			}
			
		}
		else
		{
			r=sockprint(list_ptr->fd, "NOTICE AUTH :(FD %i Status: %s)[%s@%s] \n", client_ptr->fd, st, client_ptr->nick, client_ptr->fromip);
		}
		if(r < 0)
		{
			return KILLCURRENTUSER;
		}
		client_ptr = client_ptr->next;
	}
	r=sockprint(list_ptr->fd, "NOTICE AUTH :End of user list.\n");
	if(r < 0)
	{
		return KILLCURRENTUSER;
	}
	return 0;
}
BUILTIN_COMMAND(cmd_bdie)
{
	int r;
	r=sockprint(list_ptr->fd, "NOTICE AUTH :Shutting it down....\n");
	if(r < 0)
	{
		return KILLCURRENTUSER;
	}
	logprint(jack,"Shutdown called by %s@%s",list_ptr->nick,list_ptr->fromip);
	bnckill(FATALITY);
	return 0;
}
BUILTIN_COMMAND(cmd_die)
{
	int r;
	r=sockprint(list_ptr->fd, "NOTICE AUTH :Shutting it down....\n");
	if(r < 0)
	{
		return KILLCURRENTUSER;
	}
	logprint(jack,"Shutdown called by %s@%s",list_ptr->nick,list_ptr->fromip);
	bewmstick ();
	return 0;
}

BUILTIN_COMMAND(cmd_bmsg)
{
	int p,r;
	struct cliententry *client_ptr;
	
	if (pargc < 3)
	{
		return 0;
	}
	p = mytoi (pargv[1]);
	if(p < 1)
	{
		r=sockprint(list_ptr->fd, "NOTICE AUTH :invalid bmsg arguments\n");
		if(r < 0)
		{
			return KILLCURRENTUSER;
		}
		return 0;
	}
	
	client_ptr = getclient(headclient,p);
	if( client_ptr == NULL)
	{
		r=sockprint(list_ptr->fd, "NOTICE AUTH :No such FD %i\n", p);		          
		if(r < 0)
		{
			return KILLCURRENTUSER;
		}
		return 0;
	}
	r=sockprint(client_ptr->fd, "NOTICE AUTH :%i BMSG %s\n",list_ptr->fd,pargv[2]);
/*
	if(r < 0)
	{
		return KILLCURRENTUSER;
	}
 */
	return 0;
	
}

BUILTIN_COMMAND(cmd_bkill)
{
	int p,r;
	struct cliententry *client_ptr;

	if (pargc < 2)
	{
		return 0;
	}
	p = mytoi (pargv[1]);
	if(p < 1)
	{
		r=sockprint(list_ptr->fd, "NOTICE AUTH :invalid bkill argument\n");
		if(r < 0)
		{
			return KILLCURRENTUSER;
		}
		return 0;
	}
	client_ptr = getclient(headclient,p);
	if( client_ptr == NULL)
	{
		r=sockprint(list_ptr->fd, "NOTICE AUTH :No such FD %i\n", p);		          
		if(r < 0)
		{
			return KILLCURRENTUSER;
		}
		return 0;
	}

	if(p == list_ptr->fd)
	{
		r=sockprint(list_ptr->fd, "NOTICE AUTH :Suicide is painful\n");
		if(r < 0)
		{
			return KILLCURRENTUSER;
		}
		return KILLCURRENTUSER;
	}
	r=sockprint(list_ptr->fd, "NOTICE AUTH :Killed %i\n", p);
	if(r < 0)
	{
		return KILLCURRENTUSER;
	}
	logprint(jack, "BKILL to %s@%s", client_ptr->nick, client_ptr->fromip);

	if(client_ptr->prev == NULL)
		headclient=client_ptr->next;
	else
		client_ptr->prev->next=client_ptr->next;
	
	wipeclient(client_ptr);
	                                                                                
	return 0;
}

BUILTIN_COMMAND(cmd_addhost)
{
	accesslist *na;
	
	if (pargc < 3)
	{
		return 0;
	}
	if (mytoi (pargv[1]) > 2)
	{
		return 0;
	}
	na = pmalloc (sizeof (accesslist));
	na->type = mytoi (pargv[1]);
	strncpy (na->addr, pargv[2], HOSTLEN);
	na->addr[HOSTLEN]='\0';
	na->next = NULL;
	add_access (jack, na);
	logprint(jack, "ADDHOST %s", pargv[2]);
	return 0;
}

BUILTIN_COMMAND(cmd_listhost)
{
	int r;
	accesslist *na;
	int i;
	
	for (na = jack->alist, i = 1; na; na = na->next, i++)
	{
		r=sockprint(list_ptr->fd, "NOTICE AUTH :#%i: (t:%i) %s\r\n", i, (int) na->type, na->addr);
		if(r < 0)
		{
			return KILLCURRENTUSER;
		}
	}
	return 0;
}

BUILTIN_COMMAND(cmd_keepalive)
{
	int r;
	
	if( !(list_ptr->flags & FLAGKEEPALIVE ))
	{
		list_ptr->flags |= FLAGKEEPALIVE;
		r=sockprint(list_ptr->fd, "NOTICE AUTH :Enabling KeepAlive\n");
	}
	else
	{
		list_ptr->flags &= ~FLAGKEEPALIVE;
		r=sockprint(list_ptr->fd, "NOTICE AUTH :Disabling KeepAlive\n");
	}
	if(r < 0)
	{
		return KILLCURRENTUSER;
	}
	return 0;
}