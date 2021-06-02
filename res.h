#define HOSTLEN 63
#define DNSTIMEOUT 30
#define DNSRETRY 5
#define RETRYDNSSEND(x) (x+DNSRETRY)
#define FLAGPRESREV 1
#define FLAGPRESBAD 128
struct pres_id
{
	struct pres_id *next;
	struct pres_id *prev;
	int flags;
	int id;
	time_t lsend;
	time_t ettl;
	int plen;
	char *packet;
};

struct pres_answer
{
	struct pres_answer *next;
	struct pres_answer *prev;
	int id;
	int ttl;
	int flags;
	struct sockaddr_in sin;
	char hostname[HOSTLEN+1];
};

int pres_init(void);
int pres_dumpbuf(int fd);
int pres_send(int fd, int id, const void *buf, int len, int flags);
int pres_query(int fd, char *dname, int class, int type, int flags );
int pres_namequery(int fd, char *host);
int pres_revquery(int fd, char *host);
int pres_recv(int fd);
struct pres_answer *pres_getanswerid(int id);
int pres_releaseanswerid(struct pres_answer *listrev);
struct timeval *pres_getnextevent(struct timeval *thetime);
