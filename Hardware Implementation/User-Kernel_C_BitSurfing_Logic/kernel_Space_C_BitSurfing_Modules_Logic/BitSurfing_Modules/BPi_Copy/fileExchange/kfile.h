/*
** kfile.h
**
** Made by xsyann (kopen() - kclose() - kread()) - edited by Gr3at (added kwrite() - binary2short() - OpenAndStoreCodebookToVector() - pickRandomMsgToSend())
** Contact <contact@xsyann.com>
**
*/

#ifndef                 __KFILE_H__
#define                 __KFILE_H__

#define ISCODEBOOK 1
#define ISHASHWORD 0

typedef struct codeBookData{
  size_t arrayLength;
  unsigned short *arrayPtr;
}node_t;

struct file *kopen(const char *filename, int flags, umode_t mode);
void kclose(struct file *f);
ssize_t kread(struct file *file, char __user *buf, size_t count);
ssize_t kwrite(struct file *file, char __user *buf, size_t count);
unsigned short binary2short(char*);
node_t * OpenAndStoreCodebookToVector(char*, unsigned char);

#endif                  /* __KFILE_H__ */
