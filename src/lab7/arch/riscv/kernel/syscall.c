#include "syscall.h"
#include "proc.h"
#include "stddef.h"
#include "printk.h"
#include <fat32.h>
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

int64_t sys_openat(int dfd, const char *filename, int flags) {
    int fd = -1;
    // Find an available file descriptor first
    for (int i = 0; i < PGSIZE / sizeof(struct file); i++) {
        if (!current->files[i].opened) {
            fd = i;
            break;
        }
    }
    // Do actual open
    file_open(&(current->files[fd]), filename, flags);
    return fd;
}

int64_t sys_lseek(int fd, int64_t offset, int whence) {
    int64_t ret;
    struct file *target_file = &(current->files[fd]);
    if (target_file->opened) {
        /* todo: indirect call */
        if (target_file->perms & FILE_READABLE) {
            ret = target_file->lseek(target_file, offset, whence);
        } else {
            printk("file not readable\n");
            ret = ERROR_FILE_NOT_OPEN;
        }
    } else {
        printk("file not open\n");
        ret = ERROR_FILE_NOT_OPEN;
    }
    return ret;
}

int64_t sys_close(unsigned int fd) {
    int64_t ret;
    struct file *target_file = &(current->files[fd]);
    if (target_file->opened) {
        target_file->opened = 0;
        ret = 0;
    } else {
        printk("file not open\n");
        ret = ERROR_FILE_NOT_OPEN;
    }
    return ret;
}