#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#include "common.h"
unsigned int req = 0;


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

int confoption (confetti * jr, int pargc, char **pargv)
{
  int p;
  struct vhostentry *vhost_ptr;

  switch (pargv[0][0])
  {
    case '#':
      {
	return 0;
      }
    case 'C':
      {
	if (pargc < 2)
	  return 0;
	jr->cport = atoi (pargv[1]);
	if (jr->cport < 1025)
	  jr->cport = 1025;
	else if (jr->cport > 65534)
	  jr->cport = 65534;
	return 0;
      }
    case 'd':
    case 'D':
      {
	if (pargc < 3)
	  break;
	req |= 1;
	jr->dport = atoi (pargv[1]);
	jr->maxusers = atoi (pargv[2]);
	if (pargc > 3)
	{
	  strncpy (jr->dpass, pargv[3], PASSLEN);
	  jr->dpass[PASSLEN]='\0';
	  jr->dpassf = 1;

	}
	return 0;
      }
    case 'P':
    case 'p':
      {
	if (pargc < 2)
	  break;
	strncpy (jr->pidfile, pargv[1], 256);
	jr->pidfile[256]='\0';
	return 0;
      }
    case 'S':
    case 's':
      {
	if (pargc < 2)
	  break;
	strncpy (jr->spass, pargv[1], PASSLEN);
	jr->spass[PASSLEN]='\0';
	req |= 2;
	return 0;

      }
    case 'v':
    case 'V':
      {
	if (pargc < 2)
	{
	  break;
	}
	vhost_ptr = (struct vhostentry *) malloc (sizeof (struct vhostentry));
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
    case 'x':
    case 'X':
      {
	if (pargc < 2)
	  break;
	strncpy (jr->vhostdefault, pargv[1], HOSTLEN);
	jr->vhostdefault[HOSTLEN]='\0';
	return 0;
      }
    case 'l':
    case 'L':
      {
	if (pargc < 2)
	  break;
	if ((jr->logfile = fopen (pargv[1], "ab")) != NULL)
	  jr->logf = 1;
	return 0;
      }
    case 'm':
    case 'M':
      {
	if (pargc < 2)
	  break;
	strncpy (jr->motdf, pargv[1], 256);
	jr->motdf[256]='\0';
	jr->usemotd = 1;
	return 0;
      }
    case 'w':
    case 'W':
      {
	if (pargc < 2)
	  break;
	jr->identwd = atoi (pargv[1]);
	return 0;
      }
    case 'a':
    case 'A':
      {
	accesslist *na;

	if (pargc < 3)
	  break;
	if (atoi (pargv[1]) > 2)
	  break;
	na = malloc (sizeof (accesslist));
	na->type = atoi (pargv[1]);
	strncpy (na->addr, pargv[2], HOSTLEN);
	na->addr[HOSTLEN]='\0';
	na->next = NULL;
	add_access (jr, na);
	return 0;
      }
    case 'i':
    case 'I':
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
    default:
      {
	printf ("Skipping unknown Field in conf file: (%c)\n", pargv[0][0]);
	return 0;
      }

  }

  printf ("--Option line in error:(%i)\n", pargc);
  for (p = 0; p < pargc; p++)
  {
    printf ("    %i:%s\n", p, pargv[p]);

  }
  return 0;



}



int loadconf (char *fname, confetti * jr)
{
  FILE *src;
  unsigned char linbuff[1024];
  char *pargv[100];
  char *tmp;
  int p;

  req = 0;
  if ((src = fopen (fname, "rb")) == NULL)
  {
    return CONFNOTFOUND;
  }
  while (!feof (src))
  {
    if ((fgets (linbuff, 1024, src)) == NULL)
    {
      break;
    }
    p = 0;
    tmp = strtok (linbuff, ": \n\r,");
    if (tmp != NULL)
    {
      pargv[p++] = tmp;
      while ((tmp = strtok (NULL, ": \n\r,")) != NULL)
      {
	pargv[p++] = tmp;
	if (p > 100)
	  break;
      }
      if (confoption (jr, p, pargv))
	return FATALITY;
    }


  }
  fclose (src);
  if (req != 3)
  {
    return CONFREQNOTHAPPY;
  }
  return 0;

}
