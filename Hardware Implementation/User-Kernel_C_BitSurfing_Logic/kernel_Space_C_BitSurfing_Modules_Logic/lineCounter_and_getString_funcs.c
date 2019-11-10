/* a file linecounter function lineCounter() */
/* an idea of line to string to unsigned short implementation getString() */
/* both can be used instead of kgetline function, in case we don't want foreign code */

#include <linux/init.h> /* for module_init() and module_exit() */
#include <linux/module.h> /* for MODULE_<any> */
#include <linux/kernel.h> /* for KERN_INFO */
#include <linux/slab.h> /* for kmalloc() */
/* from git repo, for kgetline() and other I/O */
#include <linux/unistd.h>
#include <linux/syscalls.h>
#include <linux/fs.h>
#include <asm/uaccess.h>

typedef struct codeBookData{
  size_t arrayLength;
  unsigned short *arrayPtr;
}node_t;

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

unsigned short lineCounter(char *filename){
	struct file *fin = NULL;
	mm_segment_t oldfs;
	char chr = 0;
	size_t accumLine = 0;
	loff_t offset = 0;

	oldfs = get_fs();
    set_fs(get_ds());
    fin = filp_open(filename, O_RDONLY, S_IRUSR | S_IWUSR);
    
    if(IS_ERR(fin) || (fin == NULL))
    	return(0);

    while(vfs_read(fin, &chr, 1, &offset)>0){
    	if(chr=='\n')
    		accumLine++;
    }
    set_fs(oldfs);
    filp_close(fin,NULL);
    printk(KERN_INFO "Goodbye, World : %zu\n", accumLine);
    return(accumLine);
}

unsigned short getString(char *filename){
	struct file *fin = NULL;
	mm_segment_t oldfs;
	char chr = 0;
	size_t accumLine = 0;
	loff_t offset = 0;

	oldfs = get_fs();
    set_fs(get_ds());
    fin = filp_open(filename, O_RDONLY, S_IRUSR | S_IWUSR);
    
    if(IS_ERR(fin) || (fin == NULL))
    	return(0);

    while(vfs_read(fin, &chr, 1, &offset)>0){
    	if((chr!='\n')&&(accumLine==0)){
    		printk(KERN_INFO "%c\n", chr);
    	}
    	if(chr=='\n')
    		accumLine++;
    }
    set_fs(oldfs);
    filp_close(fin,NULL);
    printk(KERN_INFO "Goodbye, World : %zu\n", accumLine);
    return(accumLine);
}

static int __init bitSurfing_Receiver_init(void){
  /* Load CodeBook and randomly choose a message of interest */
  node_t *codebookPtr = NULL;

  lineCounter("codebook1.txt");
  getString("codebook1.txt");

  return(0);
}

static void __exit bitSurfing_Receiver_exit(void){
  printk(KERN_INFO "Goodbye, World!\n");
}

module_init(bitSurfing_Receiver_init);
module_exit(bitSurfing_Receiver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Dimitris");
MODULE_DESCRIPTION("BitSurfing Receiving Node");
MODULE_VERSION("1.0");