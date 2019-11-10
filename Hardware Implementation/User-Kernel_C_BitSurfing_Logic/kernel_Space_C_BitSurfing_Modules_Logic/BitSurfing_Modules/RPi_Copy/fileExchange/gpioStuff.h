/*
** gpioStuff.h
**
** Made by xsyann (kgetline() - edited by Gr3at (added raiseFlag(), gpioEventDetected(), searchValidMsgInBuffer(), msgToSentInBuffer(), init_gpio(), free_gpio())
**
** Started on  Fri Mar 14 21:27:27 2014 xsyann
** Last update Fri Apr 18 22:20:12 2014 xsyann
*/

#ifndef                 __GPIOSTUFF_H__
#define                 __GPIOSTUFF_H__

#define MIN_CHUNK       64
#define BUFFER_SIZE     128

ssize_t kgetline(char **lineptr, size_t *n, struct file *f);
void raiseFlag(void);
bool gpioEventDetected(void);
unsigned short searchValidMsgInBuffer(unsigned int);
bool msgToSentInBuffer(unsigned short, unsigned int);
int init_gpio(void);
void free_gpio(void);

#endif                  /* __GPIOSTUFF_H__ */
