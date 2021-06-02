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
	char nick[1024];
	fd_set rset;
	u_short myport;
	char *cmd, *server, *port;
	char myserver[1024];

	memset(buffer, 0, 1023);
	if(read(s, buffer, 1024) <= 0)
	{
		close(s);
		return;
	}
	if(!strncasecmp(buffer, "USER", 4))
		strcpy(user, buffer);
	else
		strcpy(nick, buffer);
	memset(buffer, 0, 1024);
	if(read(s, buffer, 1023) <= 0)
	{
		close(s);
		return;
	}
	if(!strncasecmp(buffer, "USER", 4))
		strcpy(user, buffer);
	else
		strcpy(nick, buffer);
        sprintf(buffer, "NOTICE usr :*** Welcome to IRC Jumperbot- Your in the temperal limbo that requires you connect to a real server\n");
	write(s, buffer, strlen(buffer));
        sprintf(buffer, "NOTICE usr :type /quote conn [server] <port> to connect\n");
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
				if(!port)
					port = "6667";
				myport = atoi(port);
				sock_c = do_connect(myserver, myport);
				if(sock_c < 0)
					sock_c = -1;
				write(sock_c, user, strlen(user));
				write(sock_c, nick, strlen(nick));
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
					return;
				}
				write(s, buffer, strlen(buffer));
			}
		}
	}
}


				
int
main(int argc, char *argv[])
{
	int a_sock = 0;
	int s_sock = 0;
        int dport;
	struct sockaddr_in sin;
	int sinlen;
	int mypid;
        if(argc >= 2) dport=atoi(argv[1]); else dport=6667;
        printf("IRC jumpPort v1.0.2 GNU project (C) 1997-98\n");
        printf("Coded by Brian Mitchell(halflife) & James Seter(Pharos)\n");
        printf("Binding on port %d\n",dport);
        
	s_sock = socket(AF_INET, SOCK_STREAM, 0);
	if(s_sock < 0)
	{
		perror("socket");
		exit(0);
	}
	memset(&sin, 0, sizeof(struct sockaddr_in));
	sin.sin_family = AF_INET;
        sin.sin_port = htons(dport);
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
				server(a_sock);
				exit(0);
		}
		while(waitpid(-1, NULL, (int) NULL) > 0) ;
	}
	}

