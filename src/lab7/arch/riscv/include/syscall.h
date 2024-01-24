#ifndef _SYSCALL_H_
#define _SYSCALL_H_

#define SYS_WRITE 64
#define SYS_GETPID 172

#include "stddef.h"
#include "defs.h"
long sys_read(unsigned int fd, char *buf, size_t count);
long sys_write(unsigned int fd, const char *buf, size_t count);
long sys_getpid();

int64_t sys_openat(int dfd, const char *filename, int flags);
int64_t sys_lseek(int fd, int64_t offset, int whence);

#endif