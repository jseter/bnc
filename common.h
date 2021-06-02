#include "config.h"

#define PACKETBUFF 1024

#define KILLCURRENTUSER 100
#define SERVERDIED 300
#define FORWARDCMD 1

#define BUFSIZE 512
#define PASSLEN 16
#define HOSTLEN 63
#define NICKLEN 30
#define USERLEN 10
#define REALLEN 50
#define AUTOCON 1024

#define FLAGNONE 0
#define FLAGSUPER 1
#define FLAGPASS 2
#define FLAGUSER 4
#define FLAGNICK 8
#define FLAGBASED 16
#define FLAGAUTOCONN 32
#define FLAGCONNECTED 64
#define FLAGKEEPALIVE 128

#define CLIENT 0
#define SERVER 1


struct cliententry
{
  struct cliententry *next;
  struct cliententry *prev;
  char nick[NICKLEN + 1];
  char uname[USERLEN + 1];
  char fromip[HOSTLEN + 1];
  char realname[REALLEN + 1];
  char vhost[HOSTLEN + 1];
  char onserver[HOSTLEN + 1];
  char autoconn[HOSTLEN + 1];
  char autopass[PASSLEN + 1];
  int sport;
  int susepass;
  unsigned int flags;
  int fd;
  int sfd;
  int pfails;
  int blen;
  char biff[BUFSIZE];
  int slen;
  char siff[BUFSIZE];

};

#define BUILTIN_COMMAND(x) int x(struct cliententry *list_ptr, char *prefix, int pargc, char **pargv)

typedef struct
{
   char *name;
   int (*func)(struct cliententry *, char *, int, char **);
   unsigned int flags_on;
   unsigned int flags_off;
} cmdstruct;


struct vhostentry
{
    struct vhostentry  *next;
    char vhost[HOSTLEN+1];   
    
};

struct alist_struct {
	char type;
	char addr[HOSTLEN+1];
	struct alist_struct *next;
};
typedef struct alist_struct accesslist;

typedef struct
{
    char pidfile[256];
    unsigned int dport;
    unsigned int maxusers;
    char dpass[PASSLEN+1];
    int dpassf;
    unsigned int cport;
    int identwd;
    int logf;
    FILE *logfile;
    FILE *identlie;
    struct vhostentry *vhostlist;
    accesslist *alist, *alist_end;
    int has_alist;
    char vhostdefault[HOSTLEN+1];
    char spass[PASSLEN+1];
    int usemotd;
    int mtype;
    char motdf[256];
} confetti;

#define CONFNOTFOUND 1
#define CONFREQNOTHAPPY 2
#define FATALITY 255
#define SOCKERR 3
#define BINDERR 4
#define LISTENERR 5
#define FORKERR 6
#define BACKGROUND 7
#define SELECTERR 8
#define KILLED 9
#define DOWNER 10
