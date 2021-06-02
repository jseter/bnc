#include <stdio.h>
#include <stdlib.h>

#define _MTYPE_H
#include "mtype.h"

PAGE *page_alloc(void)
{
	return (PAGE *) malloc(sizeof(PAGE));
}

int page_free(PAGE *page_ptr)
{
	LINE *line_ptr;
	
	if(page_ptr == NULL)
	{
		return -1;
	}

	for(line_ptr=page_ptr->head;line_ptr;line_ptr=page_ptr->head)
	{
		page_ptr->head=line_ptr->next;
		free(line_ptr);
	}
	page_ptr->head=page_ptr->tail=NULL;
	return 0;
};


int page_append_line(PAGE *page_ptr, char *line)
{
	LINE *line_ptr;
	int len;
	
	if(page_ptr == NULL)
	{
		return -1;
	}
	
	for(len=0;line[len];len++);
	line_ptr=(LINE *)malloc(sizeof(LINE) + (sizeof(char) * (len+1))   );
	if(line_ptr == NULL)
	{
		return -1;
	}
	
	for(len=0;line[len];len++)
	{
		line_ptr->line[len]=line[len];
	}
	line_ptr->line[len]='\0';
	
	line_ptr->prev=page_ptr->tail;
	line_ptr->next=NULL;
	if(page_ptr->head == NULL)
	{
		page_ptr->head=line_ptr;
		page_ptr->tail=line_ptr;
	}
	else
	{
		page_ptr->tail->next=line_ptr;
		page_ptr->tail=line_ptr;
	}
	return 0;
}
