
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "compat.h"
#include "filelist.h"

void insert_entry(struct file_p **, const char *);
void destroy_data(void **p);

void insert_file(struct file_p **filep, const char *filename) {

	struct file_p *behindfile=NULL;

#if 0
	{
		struct file_p *next=*filep;
		while (next!=NULL) {
			printf("%s (%p %p) data: %p\n", next->filename, next->prevfile,
			    next->nextfile, next->data);
			next=next->nextfile;
		}
	}
#endif

	/* first node and ..-node are inserted at start */
	if (*filep==NULL || strcmp(filename, "/..")==0 || strcmp(filename, (*filep)->filename)<0) {
		struct file_p *hfile=NULL;

		insert_entry(&hfile, filename);
		hfile->nextfile=*filep;
		if (*filep!=NULL) (*filep)->prevfile=hfile;
		*filep=hfile;
		return;
	}

	behindfile = *filep;
	while (behindfile->nextfile!=NULL &&
	    (strcmp(filename, behindfile->filename)>0 ||
	    strcmp(behindfile->filename, "/..")==0)) {
		behindfile = behindfile->nextfile;
	}

	/* scroll one back if less */
	if (strcmp(filename, behindfile->filename)<0) {
		assert(behindfile->prevfile!=NULL);
		behindfile=behindfile->prevfile;
	}
	insert_entry(&behindfile, filename);
}

/*
   This procedure inserts a new entry BEHIND an existing one.
   If there is no entry (for first entry)
   */

void insert_entry(struct file_p **filep,const char *filename) {

	struct file_p *next;

#ifdef VERBOSE
	fprintf(stderr,"Inserting element '%s' ", filename);
#endif
	next=malloc(sizeof(struct file_p));
	if (next==NULL) {
		fprintf(stderr,"Out of memory.\n");
		exit(1);
	}
	memset(next, 0, sizeof(struct file_p));

	strlcpy(next->filename, filename, sizeof(next->filename));
	next->data=NULL;

	if (*filep==NULL) {
		*filep=next;
		return;
	} else {
		/* set pointers in new node */
		next->nextfile=(*filep)->nextfile;
		next->prevfile=*filep;

		/* update predessor */
		(*filep)->nextfile=next;

		/* update backlink in successor */
		if (next->nextfile!=NULL) {
			next->nextfile->prevfile=next;
		}
	}
}

void seekfirst_file(struct file_p **filep) {
	if (*filep==NULL) return;
	while ((*filep)->prevfile!=NULL) *filep=(*filep)->prevfile;
}

void seeklast_file(struct file_p **filep) {
	if (*filep==NULL) return;
	while ((*filep)->nextfile!=NULL) *filep=(*filep)->nextfile;
}

void delete_all(struct file_p **filep) {

	struct file_p *next;

#ifdef VERBOSE
	fprintf(stderr,"filelist: delete all\n");
#endif

	seekfirst_file(filep);

	while (*filep!=NULL) {
		next=(*filep)->nextfile;
		destroy_data(&((*filep)->data));
		free(*filep);
		*filep=next;
	}
}

void delete_all_data(struct file_p *filep) {

#ifdef VERBOSE
	fprintf(stderr,"filelist: delete all data\n");
#endif

	seekfirst_file(&filep);

	while (filep!=NULL) {
		destroy_data(&(filep->data));
		filep=filep->nextfile;
	}
}

int is_file_dir(struct file_p *filep, unsigned int nr) {
	char *c;

	c=get_file(filep,nr);
	if (c==NULL) return 1;

	return c[0]=='/' ? 1 : 0;
}

char *get_file(struct file_p *filep, unsigned int nr) {

	unsigned int i=0;

#ifdef VERBOSE
	fprintf(stderr,"get_file %d ",nr);
#endif

	for (i=0; i<nr && filep!=NULL; i++, filep=filep->nextfile);

#ifdef VERBOSE
	fprintf(stderr,"OK '%s'\n",filep==NULL ? "NULL" : filep->filename);
#endif
	return filep==NULL ? NULL : filep->filename;
}

void *get_data(struct file_p *filep, unsigned int nr) {

	unsigned int i=0;

#ifdef VERBOSE
	fprintf(stderr,"get_data %d ",nr);
#endif
	for (i=0; i<nr && filep!=NULL; i++, filep=filep->nextfile);

#ifdef VERBOSE
	fprintf(stderr,"OK '%s'\n",filep==NULL ? "NULL" : filep->filename);
#endif
	return filep==NULL ? NULL : filep->data;
}

void set_data(struct file_p *filep, unsigned int nr, void *data) {

	unsigned int i=0;

#ifdef VERBOSE
	fprintf(stderr,"set_data %d ",nr);
#endif
	for (i=0; i<nr && filep!=NULL; i++, filep=filep->nextfile);

#ifdef VERBOSE
	fprintf(stderr,"OK '%s'\n",filep==NULL ? "NULL" : filep->filename);
#endif

	if (filep->data!=NULL) printf("filelist warning: Setting data, but "
	    "it isn't cleared, yet.");

	filep->data=data;
}
