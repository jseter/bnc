#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <ctype.h>

#include "config.h"
#include "sbuf.h"
#include "struct.h"
#include "send.h"

unsigned int req = 0;
extern void *pmalloc(size_t size);
#define CONFCMD(x) int x(confetti * jr, int pargc, char **pargv)
CONFCMD(conf_listen);
CONFCMD(gen_c);
CONFCMD(gen_d);
CONFCMD(gen_p);
CONFCMD(gen_s);
CONFCMD(gen_v);
CONFCMD(gen_x);
CONFCMD(gen_l);
CONFCMD(gen_m);
CONFCMD(gen_w);
CONFCMD(gen_a);
CONFCMD(gen_b);
CONFCMD(gen_i);
CONFCMD(conf_listen);
CONFCMD(conf_password);
CONFCMD(conf_allow);

struct confcmd
{
	char *name;
	CONFCMD((*func));
};

struct confcmd confcmdlist[] =
{
	{"C",gen_c},
	{"D",gen_d},
	{"P",gen_p},
	{"S",gen_s},
	{"V",gen_v},
	{"X",gen_x},
	{"L",gen_l},
	{"M",gen_m},
	{"W",gen_w},
	{"A",gen_a},
	{"B",gen_b},
	{"I",gen_i},
	{"LISTEN",conf_listen},
	{"PASSWORD",conf_password},
	{"ADMINPASS",gen_s},
	{"ALLOW",conf_allow},
	{"VHOST",gen_v},
	{"DEFAULTVHOST",gen_x},
	{"PIDFILE",gen_p},
	{"MOTDFILE",gen_m},
	{"USEIDENTWD",gen_w},
	{"LOGFILE",gen_l},
	{NULL,NULL}
};


int mytoi(char *buf)
{
	int acum;
	int p;
	acum=0;
	for(p=0;buf[p];p++)
	{
		if( isdigit( buf[p] ))
		{
			acum*=10;
			acum+= ( buf[p] - '0' );
		}
	}
	return acum;
}

void add_access (confetti * jr, accesslist * a)
{
	if (!jr->has_alist)
	{
		jr->alist = jr->alist_end = a;
		jr->has_alist++;
	}
	else
	{
		jr->alist_end->next = a;
		jr->alist_end = a;
	}
}
CONFCMD(conf_password)
{
	if(pargc < 2)
	{
		return -1;
	}
	
	strncpy (jr->dpass, pargv[1], PASSLEN);
	jr->dpass[PASSLEN]='\0';
	jr->dpassf = 1;	
	return 0;
}

CONFCMD(conf_listen)
{
	if(pargc < 2)
	{
		return -1;
	}
	jr->dport = (mytoi(pargv[1]) % 65535);
	jr->maxusers=0;
	req|=1;
	if(pargc < 3)
	{
		return 0;
	}
	jr->maxusers=mytoi(pargv[2]);
	
	return 0;
}

CONFCMD(conf_allow)
{
	accesslist *na;
	int p;

	if (pargc < 2)
	{
		return 0;
	}

	for(p=1;p<pargc;p++)
	{
		na = pmalloc (sizeof (accesslist));
		na->type = 1;
		strncpy (na->addr, pargv[p], HOSTLEN);
		na->addr[HOSTLEN]='\0';
		na->next = NULL;
		add_access (jr, na);
	}
	return 0;
}

CONFCMD(gen_c)
{
	if(pargc < 2)
	{
		return 0;
	}
	jr->cport = mytoi(pargv[1]);
	if(jr->cport < 0)
	{
		jr->cport=0;
	}
	if(jr->cport > 65535)
	{
		jr->cport = 65535;
	}
	return 0;
}

CONFCMD(gen_d)
{
	if (pargc < 3)
	{
		return 0;
	}
	req |= 1;
	jr->dport = mytoi (pargv[1]);
	jr->maxusers = mytoi (pargv[2]);
	if (pargc > 3)
	{
		strncpy (jr->dpass, pargv[3], PASSLEN);
        	jr->dpass[PASSLEN]='\0';
        	jr->dpassf = 1;	
	}
		return 0;
}
CONFCMD(gen_p)
{
	if (pargc < 2)
	{
		return 0;
	}
	strncpy (jr->pidfile, pargv[1], FILELEN);
	jr->pidfile[FILELEN]='\0';
	return 0;
}

CONFCMD(gen_s)
{
	if (pargc < 2)
	{
		return 0;
	}
	strncpy (jr->spass, pargv[1], PASSLEN);
	jr->spass[PASSLEN]='\0';
	req |= 2;
	return 0;
	
}

CONFCMD(gen_v)
{
	struct vhostentry *vhost_ptr;
	
	if (pargc < 2)
	{
	  return 0;
	}
	vhost_ptr = (struct vhostentry *) pmalloc (sizeof (struct vhostentry));
	if (vhost_ptr == NULL)
	{
	  return 0;
	}
	vhost_ptr->next=jr->vhostlist;
	jr->vhostlist=vhost_ptr;
	
	strncpy (vhost_ptr->vhost, pargv[1], HOSTLEN);
	vhost_ptr->vhost[HOSTLEN]='\0';
	return 0;	
}

CONFCMD(gen_x)
{
	if (pargc < 2)
	{
		return 0;
	}
	strncpy (jr->vhostdefault, pargv[1], HOSTLEN);
	jr->vhostdefault[HOSTLEN]='\0';
	return 0;
}

CONFCMD(gen_l)
{
	if (pargc < 2)
	{
		return 0;
	}
	if ((jr->logfile = fopen (pargv[1], "ab")) != NULL)
	{
		jr->logf = 1;
	}
	return 0;
}

CONFCMD(gen_m)
{
	if (pargc < 2)
	{
		return 0;
	}
	strncpy (jr->motdf, pargv[1], 256);
	jr->motdf[256]='\0';
	jr->usemotd = 1;
	return 0;
}

CONFCMD(gen_w)
{
	if (pargc < 2)
	{
		jr->identwd = 1;
		return 0;
	}
	jr->identwd = mytoi (pargv[1]);
	return 0;
}

CONFCMD(gen_a)
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
	add_access (jr, na);
	return 0;
}

CONFCMD(gen_b)
{
	if(pargc < 2)
	{
		return 0;
	}
	jr->mtype=mytoi(pargv[1]);
	if(jr->mtype > 1)
	{
		jr->mtype=1;
	}
	printf("mtype is %i\n",jr->mtype);
	return 0;
}

CONFCMD(gen_i)
{
	if (pargc < 2)
	{
		jr->logfile = fopen (".ident", "ab");
	}
	else
	{
		jr->logfile = fopen (pargv[1], "ab");
	}
	return 0;
}



/******************************


	printf ("--Option line in error:(%i)\n", pargc);
	for (p = 0; p < pargc; p++)
	{
		printf ("    %i:%s\n", p, pargv[p]);
        }

*******************************/

int confoption(confetti * jr, int pargc, char **pargv)
{
	int p;
	for(p=0;confcmdlist[p].name;p++)
	{
		if(!strcasecmp(confcmdlist[p].name,pargv[0]))
		{
			return confcmdlist[p].func(jr,pargc,pargv);
		}
	}
	printf ("Skipping unknown Field in conf file: (%s)\n", pargv[0]);
	return -1;
}

int loadconf (char *fname, confetti * jr)
{
	FILE *src;
	unsigned char linbuff[1024];
	char *pargv[100];
	char *tmp;
	int p;
	int err;
	int line;

	req = 0;
	if ((src = fopen (fname, "rb")) == NULL)
	{
		return CONFNOTFOUND;
	}

	line=0;
	while (!feof (src))
	{
		if ((fgets (linbuff, 1024, src)) == NULL)
		{
			break;
		}
		line++;
		p = 0;
		tmp = strtok (linbuff, ": \n\r,");
		if (tmp != NULL)
		{
			if(tmp[0] == '#')
			{
				continue;
			}
			pargv[p++] = tmp;
			while ((tmp = strtok (NULL, ": \n\r,")) != NULL)
			{
				pargv[p++] = tmp;
				if (p > 100)
				{
					break;
				}
			}
			err = confoption (jr, p, pargv);
			if(err == -1)
			{
				printf ("--Option line in error:(%i)\n", line);
			}
		}
	}
	fclose (src);
	if (req != 3)
	{
		return CONFREQNOTHAPPY;
	}
	return 0;
}

unsigned char tolowertab[] =
{
		0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xa,
		0xb, 0xc, 0xd, 0xe, 0xf, 0x10, 0x11, 0x12, 0x13, 0x14,
		0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d,
		0x1e, 0x1f,
		' ', '!', '"', '#', '$', '%', '&', 0x27, '(', ')',
		'*', '+', ',', '-', '.', '/',
		'0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
		':', ';', '<', '=', '>', '?',
		'@', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i',
		'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's',
		't', 'u', 'v', 'w', 'x', 'y', 'z', '{', '|', '}', '~',
		'_',
		'`', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i',
		'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's',
		't', 'u', 'v', 'w', 'x', 'y', 'z', '{', '|', '}', '~',
		0x7f,
		0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89,
		0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f,
		0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99,
		0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f,
		0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9,
		0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf,
		0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9,
		0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf,
		0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9,
		0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf,
		0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9,
		0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf,
		0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9,
		0xea, 0xeb, 0xec, 0xed, 0xee, 0xef,
		0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9,
		0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff
};

unsigned char touppertab[] =
{
		0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xa,
		0xb, 0xc, 0xd, 0xe, 0xf, 0x10, 0x11, 0x12, 0x13, 0x14,
		0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d,
		0x1e, 0x1f,
		' ', '!', '"', '#', '$', '%', '&', 0x27, '(', ')',
		'*', '+', ',', '-', '.', '/',
		'0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
		':', ';', '<', '=', '>', '?',
		'@', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I',
		'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S',
		'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '[', '\\', ']', '^',
		0x5f,
		'`', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I',
		'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S',
		'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '[', '\\', ']', '^',
		0x7f,
		0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89,
		0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f,
		0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99,
		0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f,
		0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9,
		0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf,
		0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9,
		0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf,
		0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9,
		0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf,
		0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9,
		0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf,
		0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9,
		0xea, 0xeb, 0xec, 0xed, 0xee, 0xef,
		0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9,
		0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff
};
