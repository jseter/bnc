#define VERSION "v2.8.6"
#define PACKETBUFF 1024

#define KILLCURRENTUSER 100
#define SERVERDIED 300
#define FORWARDCMD 1

#define DOCKEDFD -10

#define BUFSIZE 512
#define PASSLEN 16
#define HOSTLEN 63
#define NICKLEN 30
#define USERLEN 10
#define REALLEN 50
#define AUTOCON 1024
#define FILELEN 256

#define FLAGNONE 0
#define FLAGSUPER 1
#define FLAGPASS 2
#define FLAGUSER 4
#define FLAGNICK 8
#define FLAGBASED 16
#define FLAGAUTOCONN 32
#define FLAGCONNECTED 64
#define FLAGKEEPALIVE 128
#define FLAGDOCKED 256
#define FLAGCONNECTING 512

#define CLIENT 0
#define SERVER 1

struct chanentry
{
	struct chanentry *next;
	struct chanentry *prev;
	char chan[1]; 
};

struct lsock
{
	int fd;
	int flags;
	struct sbuf sendq;
	struct sbuf recvq;
};


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
	char sid[HOSTLEN +1]; /* for docking purposes */
	char autoconn[HOSTLEN + 1];
	char autopass[PASSLEN + 1]; /* replaced with docking pass when docked */
	int sport;
	int susepass;
	unsigned int flags;
	int docked;
//	int fd;
//	int sfd;
	int pfails;

	struct lsock srv;
	struct lsock loc;

//	int blen;
//	char biff[BUFSIZE];
//	int slen;
//	char siff[BUFSIZE];
//	PAGE page_client;
//	PAGE page_server;
	struct chanentry *headchan;
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
	unsigned int dport;
	unsigned int maxusers;
	unsigned int cport;
	
	int dpassf;
	int identwd;
	int logf;
	int has_alist;
	int usemotd;
	int mtype;
	
	FILE *logfile;
	FILE *identlie;
	struct vhostentry *vhostlist;
	accesslist *alist, *alist_end;

	char vhostdefault[HOSTLEN+1];
	char spass[PASSLEN+1];
	char dpass[PASSLEN+1];
	char pidfile[FILELEN];
	char motdf[FILELEN+1];
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
