/* $Id: sbuf.h,v 1.2 2002/03/18 15:27:25 muddev Exp $
 * "$Revision: 1.2 $
 *
 * $Log: sbuf.h,v $
 * Revision 1.2  2002/03/18 15:27:25  muddev
 * modified cvs strings to comments for now
 *
*/
#define SBUFBLOCK 2048
#define SBUFMTU (SBUFBLOCK - sizeof(void *))

struct sbuf
{
	size_t length;
	size_t offset;
	void *head;
	void *tail;
};

struct sbufstate
{
	size_t length;
	size_t chunk;
	void *bucket;
	void *data;
};
#define sbuf_getlength(ptr) ((ptr)->length)

int sbuf_claim(struct sbuf *record);
int sbuf_put(struct sbuf *record, const void *message, size_t length);
int sbuf_delete(struct sbuf *record, size_t length);
int sbuf_clear(struct sbuf *record);
void *sbuf_pagemap(struct sbuf *record, size_t *length);
void *sbuf_statemap(struct sbufstate *state, size_t *length);
int sbuf_nextchunk(struct sbufstate *state);
int sbuf_firstchunk(struct sbuf *record, struct sbufstate *state);
int sbuf_getmsg(struct sbuf *record, char *buf, size_t length);
int sbuf_gettag(struct sbuf *record, char *buf, size_t length);
