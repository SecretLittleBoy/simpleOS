#include "syscall.h"
#include "proc.h"
#include "stddef.h"
#include "printk.h"
extern struct task_struct *current;

// 返回读取的字符数
int64_t sys_read(unsigned int fd, char *buf, uint64_t count) {
    int64_t ret;
    struct file *target_file = &(current->files[fd]);
    if (target_file->opened) {
        ret = target_file->read(target_file, buf, count);
    } else {
        printk("file not open\n");
        ret = ERROR_FILE_NOT_OPEN;
    }
    return ret;
}

// 返回打印的字符数
int64_t sys_write(unsigned int fd, const char *buf, uint64_t count) {
    int64_t ret;
    struct file *target_file = &(current->files[fd]);
    if (target_file->opened) {
        ret = target_file->write(target_file, buf, count);
    } else {
        printk("file not open\n");
        ret = ERROR_FILE_NOT_OPEN;
    }
    return ret;
}



long sys_getpid() {
    return current->pid;
}