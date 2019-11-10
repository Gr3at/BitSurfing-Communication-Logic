/*
** kfile.c
**
** Made by xsyann - edited by Gr3at (added kwrite())
** Contact <contact@xsyann.com>
**
** Started on  Fri Mar 14 12:00:51 2014 xsyann
** Last update Fri Apr 18 22:22:37 2014 xsyann
*/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/unistd.h>
#include <linux/syscalls.h>
#include <linux/slab.h>
#include "kfile.h"
#include "gpioStuff.h"

#include <linux/random.h> /* for get_random_bytes() */

struct file *kopen(const char *filename, int flags, umode_t mode){
	struct file *f = NULL;
	mm_segment_t oldfs;

	oldfs = get_fs();
	set_fs(get_ds());
	f = filp_open(filename, flags, mode);
	set_fs(oldfs);

	if (IS_ERR(f) || (f == NULL))
		return NULL;
	return f;
}

void kclose(struct file *f){
	filp_close(f, NULL);
}

ssize_t kread(struct file *file, char __user *buf, size_t count){
	mm_segment_t oldfs;
	ssize_t ret;

	oldfs = get_fs();
	set_fs(get_ds());

	ret = kernel_read(file, buf, count, &file->f_pos);
	//ret = vfs_read(file, buf, count, &file->f_pos); // valid for kernels prior to 4.14

	set_fs(oldfs);
	return ret;
}

ssize_t kwrite(struct file *file, char __user *buf, size_t count){
	mm_segment_t oldfs;
	ssize_t ret;

	oldfs = get_fs();
	set_fs(get_ds());

	//ret = vfs_write(file, buf, count, &file->f_pos); // for kernels before 4.14
	ret = kernel_write(file, buf, count, &file->f_pos);

	set_fs(oldfs);
	return ret;
}

unsigned short binary2short(char* s){
	char *p = s;
	unsigned short r = 0;
  	unsigned char c;

  	while(p && *p){
    	c = *p++;
	    if(c == '0') r = (r<<1);
	    else if(c == '1') r = (r<<1)+1;
	    else break;
	}
	return(r);
}

node_t * OpenAndStoreCodebookToVector(char *filename){
  size_t fileLines = 0;
  size_t dummy = 0;
  unsigned short *cbPtr = NULL;
  node_t* codeBPtr = (node_t*)kmalloc(sizeof(node_t), GFP_KERNEL);

  struct file *fin;
  char *line = NULL;
  size_t len = 0;
  ssize_t read = 0;
  fin = kopen(filename, O_RDONLY, 0);
  if(fin == NULL)
    return(NULL);
    
  while((read = kgetline(&line, &len, fin)) != -1)
    fileLines++;
  kclose(fin);

  cbPtr = (unsigned short *)kmalloc(fileLines*sizeof(unsigned short), GFP_KERNEL);
  if(cbPtr==NULL){
    printk("Couldn't allocate memmory\n");
    return(NULL);
  }
  fin = kopen(filename, O_RDONLY, 0);
  if(fin == NULL)
    return(NULL);
  while(((read = kgetline(&line, &len, fin)) != -1) && (dummy < fileLines))
    cbPtr[dummy++] = binary2short(line);

  if(line != NULL)
    kfree(line);
  kclose(fin);
  codeBPtr->arrayPtr = cbPtr;
  codeBPtr->arrayLength = fileLines;
  printk(KERN_INFO "%zu\n", codeBPtr->arrayLength);
  return(codeBPtr);
}

unsigned short pickRandomMsgToSend(node_t *cBPtr){
  unsigned int randInt;
  get_random_bytes(&randInt, sizeof(randInt));
  size_t randomIndex = (randInt % (cBPtr->arrayLength)) + 0;
  return(cBPtr->arrayPtr[randomIndex]);
}

