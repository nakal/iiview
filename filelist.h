
#ifndef IIVIEW_FILELIST_INCLUDED
#define IIVIEW_FILELIST_INCLUDED

#include <limits.h>

struct file_p {
	struct file_p *nextfile;
	struct file_p *prevfile;
	void *data;
	char filename[PATH_MAX];
};

extern struct file_p *file;

extern void insert_file(struct file_p**,const char *);
extern void seekfirst_file(struct file_p**);
extern void seeklast_file(struct file_p**);
extern void delete_all(struct file_p**);
extern void delete_all_data(struct file_p *filep);
extern int is_file_dir(struct file_p *, unsigned int);
extern char *get_file(struct file_p *, unsigned int);
extern void *get_data(struct file_p *, unsigned int);
extern void set_data(struct file_p *, unsigned int,void *);

#endif

