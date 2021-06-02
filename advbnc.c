#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
extern h_errno;

int pt,mu,dp,cu;
char ps[20];

int
do_connect(char *hostname, u_short port)
{
	int s;
	struct hostent *he;
	struct sockaddr_in sin;

	memset(&sin, 0, sizeof(struct sockaddr_in));
	sin.sin_family = AF_INET;
	sin.sin_port = htons(port);
	sin.sin_addr.s_addr = inet_addr(hostname);
	if(sin.sin_addr.s_addr == INADDR_NONE)
	{
		he = gethostbyname(hostname);
		if(!he)
			return -1;
		memcpy(&sin.sin_addr, he->h_addr, he->h_length);
	}
	s = socket(AF_INET, SOCK_STREAM, 0);
	if(s < 0)
		return -2;
	if(connect(s, (struct sockaddr *)&sin, sizeof(sin)) < 0)
	{
		close(s);
		return -2;
	}
	return s;
}

void
server(int s)
 {
	int sock_c=-1;
	int maxfd = s+1;
	char buffer[1024];
	char user[1024];
	char nick[100];
	int u,n,p;
	fd_set rset;
	u_short myport;
	char *cmd, *server, *port,*nck,*pass;
	char myserver[1024];
	u=1;
	n=1;
	p=1;
	cu++;
	while((u+n+p)&&3){
		memset(buffer, 0, 1023);
		if(read(s, buffer, 1024) <= 0)
		{
			close(s);
			cu--;
			return;
		}
		if(!strncasecmp(buffer, "USER ", 5)){
			strcpy(user, buffer);
			u=0;
		}
		else
		if(!strncasecmp(buffer, "NICK ", 5)){
			strcpy(nick,buffer);
			nck=NULL;
			strtok(nick," ");
			nck=strtok(NULL," \n\r");
			if(nck)
			  n=0;
		}
		else                                       
	        if(!strncasecmp(buffer, "PASS ", 5)){
	                strcpy(myserver,buffer);
	                pass=NULL;
	                strtok(myserver," ");
	                pass=strtok(NULL," \n\r");
	                if(pass){
	                	if(!strncmp(pass,ps,strlen(ps)))
	                	  p=0;
	                }
	        }
        }
        sprintf(buffer, "NOTICE %s :*** Welcome to IRC Jumperbot- Your in the temperal limbo that requires you connect to a real server\n",nck);
	write(s, buffer, strlen(buffer));
        sprintf(buffer, "NOTICE %s :type /quote conn [server] <port> to connect\n",nck);
	write(s, buffer, strlen(buffer));
	while(1)
	{
		FD_ZERO(&rset);
		FD_SET(s, &rset);
		if(sock_c >= 0)
			FD_SET(sock_c, &rset);
		if(sock_c > s)
			maxfd = sock_c + 1;
		else
			maxfd = s + 1;
		select(maxfd, &rset, NULL, NULL, NULL);
		memset(buffer, 0, 1024);
		if(FD_ISSET(s, &rset))
		{
			if(read(s, buffer, 1023) <= 0)
			{
				close(s);
				close(sock_c);
				return;
			}
			if(sock_c >= 0)
			{
				write(sock_c, buffer, strlen(buffer));
			}
			else
			{
				cmd = NULL;
				server = NULL;
				port = NULL;
				cmd = strtok(buffer, " ");
				if(!cmd)
					continue;
				if(strcasecmp(cmd, "conn"))
					continue;
				server = strtok(NULL, " \n\r");
				if(!server)
					continue;
				strcpy(myserver, server);
				port = strtok(NULL, " \n\r");
				if(!port) {
					myport=dp;
				}
				else
				  myport = atoi(port);
				sock_c = do_connect(myserver, myport);
				if(sock_c < 0)
					sock_c = -1;
				write(sock_c, user, strlen(user));
				sprintf(buffer,"NICK %s\n",nck);
				write(sock_c, buffer, strlen(buffer));
				memset(buffer, 0, 1024);
				read(sock_c, buffer, 1023);
				if(strlen(buffer) > 0)
				{
					write(s, buffer, strlen(buffer));
				}
								continue;
			}
		}
		if(sock_c >= 0)
		{
			if(FD_ISSET(sock_c, &rset))
			{
				memset(buffer, 0, 1024);
				if(read(sock_c, buffer, 1023) <= 0)
				{
					close(s);
					close(sock_c);
					cu--;
					return;
				}
				write(s, buffer, strlen(buffer));
			}
		}
	}
}


loadconf(){
  FILE *conf;
  char *confcmd,*confval;
  char line[100];
  int  howfar;
  
  if((conf=fopen("bnc.conf","r")) == NULL) {
  	printf("***Ack! No config file (bnc.conf).\n");
  	return 1;
  }
  howfar=0;
  while(!feof(conf)){
        memset(line,0,100);
 	fgets(line,100,conf);
 	howfar++;
 	if(!strlen(line))
 	  continue;
	if(strncmp(line,"#",1)){
	        confcmd=NULL;
	        confval=NULL;
		confcmd=strtok(line,":");
		if(!confcmd)
		  continue;
 		confval=strtok(NULL,"\n");
 		if(!confval)
 		  continue;
//  		printf("var %s = %s\n",confcmd,confval);
 		if(!strcasecmp(confcmd,"pt"))
 			pt=atoi(confval);
 		else
  		if(!strcasecmp(confcmd,"mu"))
 			mu=atoi(confval);
 		else
 		if(!strcasecmp(confcmd,"dp"))
 			dp=atoi(confval);
 		else
 		if(!strcasecmp(confcmd,"ps"))
 			strcpy(ps,confval);
 		else
 		printf("Config line %i rejected-what weirdo told you '%s' goes in my config file?\n",howfar,confcmd);
 		
 	}
  }                     
  
  fclose(conf);
  return 0;     
}
				
int
main(int argc, char *argv[])
{
	int a_sock = 0;
	int s_sock = 0;
       	struct sockaddr_in sin;
	int sinlen;
	int mypid;
        
        pt=6667;
        mu=100;
        dp=6667;
        cu=0;
	strcpy(ps,"default");
	printf("\nIrc Proxy v0.0.970804 GNU project (C) 1997-98\n");
	printf("Coded by James Seter\n");
	
	if(loadconf()) {
		printf("***Using defaults(Not recommended)\n");
	}
        printf("--Configuration:\n");
        printf("    Daemon port......:%u\n    Password.........:%s\n    Maxusers.........:%u\n    Default conn port:%u\n",pt,ps,mu,dp);
         
	s_sock = socket(AF_INET, SOCK_STREAM, 0);
	if(s_sock < 0)
	{
		perror("socket");
		exit(0);
	}
	memset(&sin, 0, sizeof(struct sockaddr_in));
	sin.sin_family = AF_INET;
        sin.sin_port = htons(pt);
        sin.sin_addr.s_addr = INADDR_ANY;
	if(bind(s_sock, (struct sockaddr *)&sin, sizeof(struct sockaddr_in)) < 0)
	{
		perror("bind");
		exit(0);
	}
         if(listen(s_sock, 10) < 0)
	{
		perror("listen");
		exit(0);
	}
	while(1)
	{
		sinlen = sizeof(sin);
		close(a_sock);
		a_sock = accept(s_sock, (struct sockaddr *)&sin, &sinlen);
		if(a_sock < 0)
		{
			perror("accept");
			continue;
		}
		switch(fork())
		{
			case -1:
				continue;
			case 0:
				if(cu>=mu)
				  exit(0);
				server(a_sock);
				exit(0);
		}
		while(waitpid(-1, NULL, (int) NULL) > 0) ;
	}
	}

