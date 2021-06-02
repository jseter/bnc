
struct line
{
	struct line *next;
	struct line *prev;
	char line[0];
};

struct line_page
{
	struct line *head;
	struct line *tail;
};


#define PAGE struct line_page
#define LINE struct line


#ifndef _MTYPE_H
extern PAGE *page_alloc(void);
extern int page_free(PAGE *page_ptr);
extern int page_append_line(PAGE *page_ptr, char *line);
#endif
#define _MTYPE_H



