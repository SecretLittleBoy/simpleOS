#include "syscall.h"
#include "proc.h"
#include "stddef.h"
#include "printk.h"
extern struct task_struct *current;

// 返回打印的字符数
long sys_write(unsigned int fd, const char *buf, size_t count) {
    if (fd == 1) {
        size_t i;
        for (i = 0; i < count; i++) {
            if(buf[i] == '\0')
                break;
            putc(buf[i]);
        }
        return i;
    } else {
        return 0;
    }
}

long sys_getpid() {
    return current->pid;
}