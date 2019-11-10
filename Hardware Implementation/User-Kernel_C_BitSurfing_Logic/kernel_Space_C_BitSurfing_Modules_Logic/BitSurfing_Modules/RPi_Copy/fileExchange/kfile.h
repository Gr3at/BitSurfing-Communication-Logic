/*
** kfile.h
**
** Made by xsyann (kopen() - kclose() - kread()) - edited by Gr3at (added kwrite() - binary2short() - OpenAndStoreCodebookToVector() - pickRandomMsgToSend())
** Contact <contact@xsyann.com>
**
** Started on  Fri Mar 14 12:03:30 2014 xsyann
** Last update Fri Apr 18 22:19:06 2014 xsyann
*/

#ifndef                 __KFILE_H__
#define                 __KFILE_H__

typedef struct codeBookData{
  size_t arrayLength;
  unsigned short *arrayPtr;
}node_t;

struct file *kopen(const char *filename, int flags, umode_t mode);
void kclose(struct file *f);
ssize_t kread(struct file *file, char __user *buf, size_t count);
ssize_t kwrite(struct file *file, char __user *buf, size_t count);
unsigned short binary2short(char*);
node_t * OpenAndStoreCodebookToVector(char*);
unsigned short pickRandomMsgToSend(node_t*);

#endif                  /* __KFILE_H__ */
