#ifndef K_FILE
#define K_FILE

#include <linux/fs.h>
#include <asm/segment.h>
#include <asm/uaccess.h>
#include <linux/buffer_head.h>

// for flags and mode, see http://man7.org/linux/man-pages/man2/open.2.html

struct file *file_open(const char *path, int flags, int rights);
int file_read(struct file *file, unsigned long long offset, unsigned char *data, unsigned int size);
int file_write(struct file *file, unsigned long long offset, unsigned char *data, unsigned int size);
void file_close(struct file *file);
int file_sync(struct file *file);

#endif
