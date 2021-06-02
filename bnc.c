#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#ifdef HAVE_SYS_TIME_H
#	include <sys/time.h>
#	ifdef TIME_WITH_SYS_TIME
#		include <time.h>
#	endif
#else
#	include <time.h>
#endif

#include <unistd.h>
#include "config.h"
#include "sbuf.h"
#include "struct.h"
#include "send.h"


#ifndef VERSION
#define VERSION "v?.?.?"
#endif


char *myself;

confetti bncconf;
char logbuf[PACKETBUFF];
int foreman;

extern int logprint(confetti *jr,const char *format,...);
extern int loadconf (char *fname, confetti * jr);
extern int ircproxy (confetti * jr);
extern int initproxy (confetti * jr);

extern char buffer[];


int bnclog (confetti * jr, char *logbuff)
{
  static long tata;
  int p;
  char tmpa[40];

  if (jr->logf != 1)
    return 0;
  tata = time ((long *) 0);
  snprintf (tmpa, 40, "%s", ctime (&tata));
  for (p = 0; p < 40; p++)
  {
    switch (tmpa[p])
    {
      case '\n':
      case '\r':
      case '\0':
	{
	  tmpa[p] = '\0';
	  break;
	}
    }
  }
  fprintf (jr->logfile, "%s: %s\n", tmpa, logbuff);
  fflush (jr->logfile);
  return 0;
}
struct deathwish
{
	int reason;
	char *message;
};

struct deathwish bncerr[] =
{
  { CONFNOTFOUND, "The config file was not found"},
  { CONFREQNOTHAPPY, "Configuration file did not satisfy all requirments"},
  { FATALITY, "Fatal error such as lack of ram occured"},
  { SOCKERR, "Unable to open socket"},
  { BINDERR, "Unable to bind to socket"},
  { LISTENERR, "Unable to open socket for listen"},
  { BACKGROUND, "Successfully went into the background"},
  { SELECTERR, "Select call returned in error" },
  { DOWNER, "Shut down by Supervisor" },
  { KILLED, "I wuz shot down!" },
  { 0, "Died for an unknown reason" }
};


void bnckill (int reason)
{
	int p;
	char *reply;
	for(p=0;;p++)
	{
		if((reason == bncerr[p].reason) || (bncerr[p].reason == 0))
		{
			reply=bncerr[p].message;
			break;
		}
	}

	if (foreman)
		printf ("Exit %s{%i} :%s.\n", myself, reason, reply);

	logprint(&bncconf,"Exit %s{%i} :%s.\n", myself, reason, reply);
	exit (reason);
}

void *pmalloc(size_t size)
{
	void *s;
	s=malloc(size);
	if(s == NULL)
	{
		bnckill(FATALITY);
	
	}
	return s;
}

int main (int argc, char **argv)
{
	int tmps;
	char *avhd;
	char *pars;
	char *conffile;
	FILE *mylife;
	
	struct vhostentry *hockum;

	myself = argv[0];
	foreman = 1;
  
  
  	
	if (argc > 1)
	{
		conffile = argv[1];
	}
	else
	{
		conffile=buffer;
		strncpy (conffile, argv[0], PACKETBUFF);
		strncat (conffile, ".conf", PACKETBUFF);
		conffile[PACKETBUFF]='\0';
	}
	printf ("Irc Proxy " VERSION " GNU project (C) 1998-99\n");
	printf ("Coded by James Seter :bugs-> (Pharos@refract.com) or IRC pharos on efnet\n");
	printf ("--Using conf file %s\n", conffile);

	memset (&bncconf, 0, sizeof (bncconf));
	bncconf.cport = 6667;
	bncconf.mtype=1;
	
	pars = strrchr (argv[0], '/');
	if (pars == NULL)
		pars = argv[0];
	else if (strlen (pars) > 1)
		pars++;

	strcpy (bncconf.pidfile, "./pid.");
	strncat (bncconf.pidfile, pars, 256);

	strcpy (bncconf.dpass, "-NONE-");
	bncconf.dpassf = 0;
	bncconf.identwd = 0;
	avhd = getenv ("IRC_HOST");
	if (avhd != NULL)
	{
		strncpy (bncconf.vhostdefault, avhd, HOSTLEN);
		bncconf.vhostdefault[HOSTLEN]='\0';
	}
	tmps = loadconf (conffile, &bncconf);
	if(tmps)
	{
		bnckill (tmps);
	}

	printf ("--Configuration:\n");
	printf ("    Daemon port......:%u\n    Maxusers.........:%u\n    Default conn port:%u\n    Pid File.........:%s\n",
		bncconf.dport, bncconf.maxusers, bncconf.cport, bncconf.pidfile);
	
	if (bncconf.vhostdefault[0] == '\0')
	{
		printf ("    Vhost Default....:-SYSTEM DEFAULT-\n");
	}
	else
	{
		printf ("    Vhost Default....:%s\n", bncconf.vhostdefault);
	}
	
	if (bncconf.vhostlist != NULL)
	{
		hockum = bncconf.vhostlist;
		while (hockum != NULL)
		{
			printf ("    Vhost entry......:%s\n", hockum->vhost);
			hockum = hockum->next;
		}
	}
	tmps = initproxy (&bncconf);
	if(tmps)
	{
		bnckill (tmps);
	}
	signal (SIGHUP, SIG_IGN);
	signal (SIGPIPE,SIG_IGN);
	
	switch (tmps = fork ())
	{
		case -1:
		{
			bnckill (FORKERR);
		}
		case 0:
		{
			foreman = 0;		/* this is a child process printing should no longer be allowed */
			setsid ();
			break;
		}
		default:
		{
			printf ("    Process Id.......:%i\n", tmps);
			bnckill (BACKGROUND);
		}
	}
	
	if ((mylife = fopen (bncconf.pidfile, "wb")) != NULL)
	{
		fprintf (mylife, "%i\n", getpid ());
		fclose (mylife);
	}
	logprint(&bncconf, "BNC started. pid %i", getpid ());
	tmps = ircproxy (&bncconf);
	if(tmps)
	{
		bnckill (tmps);
	}
	return 0;
}
