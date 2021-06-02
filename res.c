#include <stdio.h>
#include <stdlib.h>   
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <sys/types.h>   
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <arpa/nameser.h>
#include <resolv.h>
#include <netdb.h> 
#include <fcntl.h>
#include <errno.h>

#include "res.h"
#include "common.h"

int _getshort(char *);
int _getlong(char *);

struct pres_answer *headpresanswer=NULL;
struct pres_id *headpresid=NULL;
int pres_activequeries = 0;

struct pres_answer *pres_getanswerid(int id)
{
	struct pres_answer *listrev;
	for(listrev=headpresanswer;listrev != NULL; listrev=listrev->next)
	{
		if(listrev->id == id)
		{
			return listrev;
		}
	}
	return NULL;
}

int pres_releaseanswerid(struct pres_answer *listrev)
{
	if(listrev == NULL)
		return -1;
	if(listrev->prev == NULL)
	{
		headpresanswer=listrev->next;
		if(listrev->next != NULL)
			listrev->next->prev=NULL;
		
	}
	else
	{
		listrev->prev->next=listrev->next;
		if(listrev->next != NULL)
			listrev->next->prev=listrev->prev;
		
	}

	free(listrev);
	return 0;	
}

int str_revdnsq(char *buf, char *host, int maxlen)
{
	int len;
	int r;
	int x;
	unsigned char *cp;
	register char *eob;
	register char *bob;
	register char *sl,*sr;
	
	char c;
	
	eob = buf + maxlen;
	bob = buf;

	len=0;
	cp = (unsigned char *)host;
	cp += 3;
	
//	printf("got here\n");
	for(x=0; x < 4; x++, cp--)
	{
		sl=bob;
		for(r = *cp; r > 0; r /= 10)
		{
			c = (r % 10) + '0';
			if(bob >= eob)
				return -1;

			*bob=c;
			bob++;
			len++;
		}
		
		if(sl == bob)
		{
			if(bob >= eob)
				return -1;
			*bob='0';
			bob++;
			len++;
		}
		sr = bob - 1;
		for(; sl < sr; sl++, sr--)
		{
			c = *sr;
			*sr = *sl;
			*sl = c;
		}
		*bob='.';
		bob++;
		len++;
	}

	for(cp="in-addr.arpa.";*cp;cp++)
	{
		if(bob > eob)
			return -1;
		*bob=*cp;
		bob++;
		len++;
	}
	*bob='\0';	
	
	
	return len;
}


struct timeval *pres_getnextevent(struct timeval *thetime)
{
	int events;
	static struct timeval ndelay;
	struct timeval *delay;
	struct pres_id *list;
	struct timeval tmptv;

	events=0;
	pres_activequeries=0;
	delay = NULL;
	for(list=headpresid;list != NULL; list=list->next)
	{
		
		if((thetime->tv_sec >= RETRYDNSSEND(list->lsend)) || (list->lsend == 0) )
			pres_activequeries=1;
		else
		{
			tmptv.tv_sec = RETRYDNSSEND(list->lsend) - thetime->tv_sec;
//ph			printf("lsend is %i %i %i\n", tmptv.tv_sec, list->lsend, thetime->tv_sec);
			if(delay == NULL)
			{
				events++;
				delay=&ndelay;
				ndelay.tv_sec=tmptv.tv_sec;
				if(ndelay.tv_sec < 1)
					delay->tv_sec=1;
				ndelay.tv_usec=0;
				continue;
			}
			if( tmptv.tv_sec <= ndelay.tv_sec )
			{
				events++;
				ndelay.tv_sec=tmptv.tv_sec;
				if(ndelay.tv_sec < 1)
					ndelay.tv_sec=1;
				ndelay.tv_sec=0;
//ph				printf("%i setting delay to %i\n", __LINE__, delay->tv_sec);
			}
		}
		tmptv.tv_sec = list->ettl - thetime->tv_sec;
		if(tmptv.tv_sec <= 0)
		{
			struct pres_answer *listrev;
			
//ph			printf("expiring dns\n");
			if(list->prev == NULL)
			{
				if(list->next == NULL)
				{
					headpresid=NULL;
				}
				else
				{
					headpresid=list->next;
					list->next->prev=NULL;
				}
			}
			else
			{
				if(list->next == NULL)
				{
					list->prev->next=NULL;
				}
				else
				{
					list->prev->next=list->next;
					list->next->prev=list->prev;	
				}
			}
			list->next=list->prev=NULL;
			
			listrev = (struct pres_answer *)malloc(sizeof(struct pres_answer));
			if(listrev == NULL)
				return -1;
			listrev->id=list->id;
			listrev->ttl=list->id;
			listrev->flags = FLAGPRESBAD;
			listrev->hostname[0]='\0';
			listrev->prev=NULL;
			listrev->next=headpresanswer;
			if(headpresanswer != NULL)
				headpresanswer->prev=listrev;
			headpresanswer=listrev;			
			free(list);
		}
		else
		{
			
			if(delay == NULL)
			{
				events++;
				delay=&ndelay;
				ndelay.tv_sec=tmptv.tv_sec;
				if(ndelay.tv_sec < 1)
					delay->tv_sec=1;
				ndelay.tv_usec=0;
				continue;
			}
			if( tmptv.tv_sec <= ndelay.tv_sec )
			{
				events++;
				ndelay.tv_sec=tmptv.tv_sec;
				if(ndelay.tv_sec < 1)
					ndelay.tv_sec=1;
				ndelay.tv_sec=0;
//ph				printf("%i setting delay to %i\n", __LINE__, delay->tv_sec);
			}
			
		}
	}
	
	return delay;
}

void dumpb(unsigned char *buf,int lena)
{
	int cnt1,cnt2;
	unsigned char c;
	int lines=0;
	int ofs=0;
	lines=(lena/16)+1;

	printf("output from %10p\n", buf);	
	for(cnt1=0;cnt1<lines;cnt1++)
	{
		printf("%10p: ", buf + (cnt1*16));
		for(cnt2=0;cnt2<16;cnt2++)
		{
			ofs = (cnt1*16)+cnt2;
			if(ofs >= lena)
			{
				printf("__");
			}
			else
			{
				c=buf[ofs];
				printf("%02X",c);
			}
			if(cnt2==7) putchar('-');
				else putchar(' ');
		}
		printf("  ");
		for(cnt2=0;cnt2<16;cnt2++)
		{
			ofs = (cnt1*16)+cnt2;
			if(ofs >= lena)
			{
				putchar('_');
			}
			else
			{
				c=buf[ofs];
				if((c<32) || (c>126) ) putchar('.');
				else putchar(c);
			}
		}
		putchar('\n');
	}
}



int pres_init(void)
{
	int fd;
	int flags;
	int res;
	int nl;
	struct sockaddr_in sin;
	
	fd = socket(PF_INET, SOCK_DGRAM, 0);
	if(fd == -1)
		return -1;
	/* set non-blocking */
#ifdef O_NONBLOCK
	flags = fcntl(fd, F_GETFL, 0);
	if(flags == -1)
		flags = 0;
	res = fcntl(fd, F_SETFL, flags | O_NONBLOCK);
#else
	flags = 1;
	res = ioctl(fd, FIOBIO, &flags);
#endif			
	if(res == -1)
	{
		close(fd);
		return -1;
	}
	memset(&sin, 0, sizeof(struct sockaddr_in));
	sin.sin_family = AF_INET;
	sin.sin_port = 0;
	sin.sin_addr.s_addr = INADDR_ANY;
	res = bind(fd, (struct sockaddr *)&sin, sizeof(struct sockaddr_in));
	if(res == -1)
	{
		close(fd);
		return -1;
	}
	
	nl = sizeof(struct sockaddr_in);
#if 0
	res = getsockname(fd, (struct sockaddr *)&sin, &nl);
	if(res == -1)
	{
		close(fd);
		return -1;
	}
//ph	printf("dns listening on port %i\n", ntohs(sin.sin_port));
#endif
	res = res_init();	
	if(res == -1)
	{
		close(fd);
		return -1;
	}
	if (!_res.nscount)
	{
		_res.nscount = 1;   
		_res.nsaddr_list[0].sin_addr.s_addr = inet_addr("127.0.0.1");
	}
	return fd;
}




int pres_dumpbuf(int fd)
{
	struct pres_id *plist;
	int p;
	int cx = time(NULL);
	int ret;
	int count;
	int sent;

	count=0;
	sent=0;
//ph	printf("pres_dumpbuf(%i)\n", fd);
	for(plist=headpresid; plist != NULL; plist=plist->next)
	{
		if(cx > plist->ettl)
		{
//ph			printf("dns timeout on %i\n", plist->id);
			continue;
		}
		sent=0;
		for(p=0; p < _res.nscount; p++)
		{
			ret = sendto(fd, plist->packet, plist->plen, 0, (struct sockaddr *)&(_res.nsaddr_list[p]), sizeof(struct sockaddr_in));
			if(ret != -1)
				sent++;
		}

//ph		printf("sucessfully dumped out %i\n", plist->id);
		plist->lsend = cx;
	}
	return sent ? sent : -1;
}

int pres_send(int fd, int id, const void *buf, int len, int flags)
{
	struct pres_id *plist;
	int cx;
	cx=time(NULL);
	
	plist = (struct pres_id *)malloc(sizeof(struct pres_id));
	if(plist == NULL)
		return -1;
	plist->packet = (char *)malloc(len);
	if(plist->packet == NULL)
	{
		free(plist);
		return -1;
	}
	memcpy(plist->packet, buf, len);
	plist->lsend=0;
	plist->flags=flags;
	plist->ettl = cx + DNSTIMEOUT;
	plist->plen=len;
	plist->id=id;
	plist->next=headpresid;
	if(headpresid != NULL) headpresid->prev=plist;
	plist->prev=NULL;
	headpresid=plist;
	
//	dumpb(plist->packet, plist->plen);
	return 0;
}


int pres_query(int fd, char *dname, int class, int type, int flags )
{
	HEADER *dns = NULL;
	static char buf[PACKETSZ];
	int len, ns, ret;
	int id;
	
	len=ns=ret=0;

//ph	printf("attempting to start resolving %s.\n", dname);

	memset((char *)buf, 0, PACKETSZ);
	len = res_mkquery(QUERY, dname, class, type, NULL, 0, NULL, buf, PACKETSZ);
	if(len == -1)
	{
//ph		printf("unable to build query\n");
		return -1;
	}
	dns = (HEADER *)buf;
	id = ntohs(dns->id);

	ret = pres_send(fd, id,  buf, sizeof(HEADER) + len, flags);
	if(ret == -1)
	{
//ph		printf("could not send dns request\n");
		return -1;
	}
	
	return id;
}


int pres_namequery(int fd, char *host)
{
	return pres_query(fd, host, C_IN, T_A, 0);	
}


int pres_revquery(int fd, char *host)
{
	int len;
	static char ipbuf[32];
	len = str_revdnsq(ipbuf, host, 32);
	if(len == -1)
		return -1;
	return pres_query(fd, ipbuf, C_IN, T_PTR, FLAGPRESREV);
}



int pres_recv(int fd)
{
	struct pres_id *plist;
	int found;
	
	struct hostent he;
	HEADER *hptr;
	static char buf[PACKETSZ];
	static char exp[HOSTLEN+1];
	char *eob;
	char *pbuf;
	struct in_addr dr;
	struct sockaddr_in from;
	socklen_t fromlen;
	int len, ns, ret, ans;
	int n;
	int type;
	int ttl;
	int dlen;
	int class;
	int p;

	len=ns=ret=ans=0;

	fromlen=sizeof(struct sockaddr_in);
	ret = recvfrom(fd, buf, PACKETSZ, 0, (struct sockaddr *)&from, &fromlen);
//	printf("packet from %s\n", inet_ntoa((struct sockaddr)from.sin_addr.s_addr));
	if(ret == -1)
	{
//ph		printf("unable to read data\n");
		return 0;
	}
	if(ret == 0)
	{
//ph		printf("zero len data\n");
		return 0;	
	}

	found=0;
	for(p = 0; p < _res.nscount; p++)
	{
		if( (unsigned int) _res.nsaddr_list[p].sin_addr.s_addr == (unsigned int) from.sin_addr.s_addr)
		{
			found = 1;
			break;
		}		
		
	}
	if(found != 1)
	{
//ph		printf("rogue packet\n");
		return -1;
	}
	
//ph	printf("got reply\n");
//	dumpb(buf, ret);

	eob=buf+ret;

	hptr = (HEADER *)buf;
	
	hptr->id = ntohs(hptr->id);
	hptr->ancount = ntohs(hptr->ancount);
	hptr->qdcount = ntohs(hptr->qdcount);
	hptr->nscount = ntohs(hptr->nscount);
	hptr->arcount = ntohs(hptr->arcount);
//ph	printf("get_res:id = %d rcode = %d ancount = %d qdcount = %d arcount = %d\n",
//ph		hptr->id, hptr->rcode, hptr->ancount, hptr->qdcount, hptr->arcount);

	for(plist=headpresid; plist != NULL; plist=plist->next)
	{
//ph		printf("comparing %i %i\n", plist->id, hptr->id);
		if(plist->id == hptr->id)
		{
			/* lets unlink it */
			if(plist->prev == NULL)
			{
				if(plist->next == NULL)
				{
					headpresid=NULL;
				}
				else
				{
					headpresid=plist->next;
					plist->next->prev=NULL;
				}
			}
			else
			{
				if(plist->next == NULL)
				{
					plist->prev->next=NULL;
				}
				else
				{
					plist->prev->next=plist->next;
					plist->next->prev=plist->prev;	
				}
			}
			plist->next=plist->prev=NULL;
			break;
		}
	
	}
	if(plist == NULL)
		return -1;
	free(plist);
	plist=NULL;

	pbuf = buf + sizeof(HEADER);
	
	for(; hptr->qdcount > 0; hptr->qdcount--)
	{
//ph		printf("currently at %10p\n", pbuf);
//ph		printf("reading question\n");
		n = dn_skipname(pbuf, eob);
		if(n == -1)
			break;
		else
			pbuf += n + QFIXEDSZ;
	}
	
//ph	printf("doing ancount %i\n", hptr->ancount);
	while(hptr->ancount-- > 0)
	{
//ph		printf("currently at %10p\n", pbuf);
		n = dn_expand(buf, eob, pbuf, exp, sizeof(exp));
		exp[HOSTLEN] = '\0';
		if(n <= 0)
		{
//ph			printf("did not expand\n");
			break;
		}		
		pbuf += n;
		type = (int) _getshort(pbuf);
		pbuf += sizeof(short);
		class = (int) _getshort(pbuf);
		pbuf += sizeof(short);
		
		ttl = _getlong(pbuf);
		pbuf += sizeof(long);
		
		dlen = (int) _getshort(pbuf);
		pbuf += sizeof(short);
		
		
	
		len = strlen(exp);

//ph		printf("type %i\n", type);
		switch(type)
		{
			case T_A:
			{
				he.h_length =  dlen;
				if(ans == 1)
					he.h_addrtype = (class == C_IN) ? AF_INET : AF_UNSPEC;
				
				if(dlen != sizeof(dr))
				{
//ph					printf("bad IP len (%d)\n", dlen);
					return -2;
				}
				
				memcpy((char *)&dr, pbuf, sizeof(dr));
//ph				printf("got ip # %s for %s\n", inet_ntoa(dr), exp);
				pbuf += dlen;
				break;
				
			}
			case T_CNAME:
			{
//ph				printf("got cname %s\n", exp);
				pbuf += dlen;
				break;
			}
			case T_PTR:
			{
				struct pres_answer *listrev;
//ph				printf("handling a reverse dns\n");
				n = dn_expand(buf, eob, pbuf, exp, HOSTLEN);
				if(n == -1)
				{
					return -2;
				}
				exp[HOSTLEN]='\0';
//				pbuf += n; /* who should i trust? */
				
				listrev = (struct pres_answer *)malloc(sizeof(struct pres_answer));
				if(listrev == NULL)
					return -1;
				listrev->flags=FLAGPRESREV;
				listrev->id=hptr->id;
				listrev->ttl=hptr->id;
				memcpy(listrev->hostname, exp, HOSTLEN);
				listrev->hostname[HOSTLEN]='\0';
				listrev->prev=NULL;
				listrev->next=headpresanswer;
				if(headpresanswer != NULL)
					headpresanswer->prev=listrev;
				headpresanswer=listrev;
//ph				printf("reversed to %s\n",exp);
				pbuf += dlen;
				break;
			}
			default:
			{
				pbuf += dlen;
//ph				printf("Unknown type\n");
				break;
			}
			
		}
		
	}

//ph	printf("done hacking away\n");
	return 1;
}




int pres_zactivequeries(void)
{
	struct pres_id *plist;
	int cx=time(NULL);
	for(plist=headpresid; plist != NULL; plist=plist->next)
	{
		if(cx > plist->ettl)
			continue;
		if(cx > (plist->lsend + DNSRETRY))
			return 1; /* only if we need to send some dns */
	}
	return 0;
}
