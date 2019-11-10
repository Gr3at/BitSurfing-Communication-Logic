#ifndef                 __GPIOSTUFF_H__
#define                 __GPIOSTUFF_H__

#define MIN_CHUNK       64
#define BUFFER_SIZE     128

ssize_t kgetline(char **lineptr, size_t *n, struct file *f);
void raiseFlag(void);
bool gpioEventDetected(void);
bool gpioEventDetected_v2(void);
unsigned short searchValidMsgInBuffer(unsigned int);
bool msgToSentInBuffer(unsigned short, unsigned int);
int init_gpio(void);
void free_gpio(void);

#endif                  /* __GPIOSTUFF_H__ */
