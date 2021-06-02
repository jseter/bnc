struct ldcc
{
	struct ldcc *next;
	struct sockaddr sin;
	socklen_t sinlen;
	int fd;
};

struct pdcc
{
	struct pdcc *next;
	int lfd;
	int rfd;
	int flags;
	struct sbuf lsendq;
	struct sbuf rsendq;
};

int ct_handle(struct cliententry *cptr, char *prefix, char *to, char *msg, int fromwho);
