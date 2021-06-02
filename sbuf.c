#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sbuf.h"

// static const char *version = "1.0.0";

#define voidnext(ptr) (*(void **)(ptr))
// #define voiddata(ptr) (((char *)ptr) + sizeof(*ptr))
#define voiddata(ptr) (char *)(((void **)(ptr)) + 1)
#define voiddatum(ptr) (void *)(((void **)(ptr)) + 1)

static void *sbuf_cache;

static void *sbuf_alloc(void)
{
	void *res;
	if(sbuf_cache == NULL)
		return malloc( SBUFBLOCK );
	res = sbuf_cache;
	sbuf_cache = voidnext(sbuf_cache);
	return res;
}
static void sbuf_free(void *ptr)
{
	voidnext(ptr) = sbuf_cache;
	sbuf_cache = ptr;
}

int sbuf_claim(struct sbuf *record)
{
	bzero(record, sizeof(*record));
	return 0;
}


int sbuf_put(struct sbuf *record, const void *message, size_t length)
{
	register void *bucket;
	register void *data;
	register size_t chunk;
	register size_t offset;

	/* If there is free space in the last bucket, skip allocation */
	offset = (record->length + record->offset) % SBUFMTU;
	if(offset)
	{
		bucket = record->tail;
		data = voiddata(bucket) + offset;
		chunk = SBUFMTU - offset;
		if(chunk > length)
			chunk = length;
		goto putblk;
	}
	
	while(length)
	{
		/* Grow the sbuf */
		bucket = sbuf_alloc();
		if(bucket == NULL)
			return -1;
		voidnext(bucket) = NULL;
		if(record->tail)
			voidnext(record->tail) = bucket;
		else
			record->head = bucket;

		record->tail = bucket;

		chunk = length > SBUFMTU ? SBUFMTU : length;
		data = voiddata(bucket);
putblk:
		/* Copy the block into the buffer */
		memcpy( data, message, chunk);
		message = (void *)(((unsigned char *)message) + chunk);
		record->length += chunk;
		length -= chunk;
	}
	return 0;
}

int sbuf_delete(struct sbuf *record, size_t length)
{
	register void *bucket;
	register size_t chunk;
	
	if(length > record->length)
		length = record->length;
	
	while(length)
	{
		chunk = SBUFMTU - record->offset;
		if(chunk > length)
			chunk = length;
		length -= chunk;
		record->offset += chunk;
		record->length -= chunk;
		
		if( record->offset == SBUFMTU)
		{
			bucket = record->head;
			record->offset = 0;
			record->head = voidnext(bucket);
			sbuf_free(bucket);
			if( record->length == 0 )
				record->tail = NULL;
		}
	}
	if(record->length == 0 && record->offset > 0)
	{
		bucket = record->head;
		record->offset = 0;
		record->head = NULL;
		record->tail = NULL;
		sbuf_free(bucket);
	}
	return 0;
}

int sbuf_clear(struct sbuf *record)
{
	register void *bucket;
	
	while(record->head)
	{
		bucket = record->head;
		record->head = voidnext(bucket);
		sbuf_free(bucket);
	}
	record->tail = NULL;
	record->length = 0;
	record->offset = 0;
	return 0;
}

void *sbuf_pagemap(struct sbuf *record, size_t *length)
{
	if(record->head == NULL)
		return NULL;
	if(record->length == 0)
		return NULL;
	
	*length = SBUFMTU - record->offset;
	if(*length > record->length)
		*length = record->length;

	return (void *)(voiddata(record->head) + record->offset);
}

void *sbuf_statemap(struct sbufstate *state, size_t *length)
{
	if(state->chunk == 0)
		return NULL;
	*length = state->chunk;
	return state->data;
}


int sbuf_nextchunk(struct sbufstate *state)
{
	if(state->length == 0)
		return -1;
	state->bucket = voidnext(state->bucket);
	state->chunk = SBUFMTU;
	if(state->chunk > state->length)
		state->chunk = state->length;
	state->length -= state->chunk;
	state->data = (void *)voiddata(state->bucket);

	return 0;
}

int sbuf_firstchunk(struct sbuf *record, struct sbufstate *state)
{
	if(record->length == 0)
		return -1;

	state->length = record->length;
	state->bucket = record->head;
	state->chunk = SBUFMTU - record->offset;
	if(state->chunk > record->length)
		state->chunk = record->length;
	state->length -= state->chunk;
	state->data = (void *)(voiddata(state->bucket) + record->offset);
	return 0;
}

int sbuf_getmsg(struct sbuf *record, char *buf, size_t length)
{
	struct sbufstate state;
	int res;
	char *d;
	char *s;
	char *eos;
	char *eod;
	int nlcount;
	size_t rlength;
	size_t tlength;
	
	nlcount = 0;
	tlength = 0;
	d = buf;
	eod = buf + length;
	bzero(&state, sizeof(state));
	res = sbuf_firstchunk(record, &state);
	while(res == 0)
	{
		s = sbuf_statemap(&state, &rlength);
		eos = s + rlength;
		for(;s < eos; s++)
		{
			if((*s == '\r') || (*s == '\n'))
			{
				nlcount++;
				continue;
			}
			if(*s == '\0')
			{
				/* no nul byte allowed in input */
				continue;
			}
			
			if(nlcount)
				goto gotline;
			
			tlength++;
			if(d < eod)
				*d++ = *s;
		}
	
		res = sbuf_nextchunk(&state);
	}

	if(nlcount)
		goto gotline;
	return 0;
gotline:
	sbuf_delete(record, nlcount + tlength);
	if(d >= eod)
		d = eod-1;
	*d++ = '\0';
	return d - buf;
}


int sbuf_gettag(struct sbuf *record, char *buf, size_t length)
{
	struct sbufstate state;
	int res;
	char *d;
	char *s;
	char *eos;
	char *eod;
	int esc;
	int nl;
	int cmt;
	size_t rlength;
	size_t tlength;
	
	cmt = 0;
	esc = 0;
	tlength = 0;
	nl = 1;
	
	d = buf;
	eod = buf + length;
	
	bzero(&state, sizeof(state));
	res = sbuf_firstchunk(record, &state);
	while(res == 0)
	{
		s = sbuf_statemap(&state, &rlength);
		eos = s + rlength;
		for(;s < eos; s++)
		{
			tlength++;
			if(cmt)
			{
				if((*s == '\r') || (*s == '\n'))
				{
					nl = 1;
					continue;
				}
				
				if(nl == 1)
					cmt = 0;
				/* fall through */
			}
			if(nl && (*s == '#'))
			{
				cmt = 1;
				nl = 0;
			}
			if(esc)
			{
				esc = 0;
				if(d < eod)
					*d++ = *s;
				continue;
			}
			
			if(*s == '\\')
			{
				esc = 1;
				continue;
			}
			if(nl && ((*s == ' ') || (*s == '\t')))
				continue;
				
			if((*s == '\r') || (*s == '\n'))
			{
				if(nl == 0)
				{
					if(d < eod)
						*d++ = ' ';
				}
				nl = 1;
				continue;
			}
			nl = 0;
			if(*s == ';')
				goto gotline;
			if(d < eod)
				*d++ = *s;
		}
		res = sbuf_nextchunk(&state);
	}

	return 0;
gotline:
	sbuf_delete(record, tlength);
	if(d >= eod)
		d = eod-1;
	*d++ = '\0';
	return d - buf;
}
