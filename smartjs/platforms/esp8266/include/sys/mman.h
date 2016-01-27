#ifndef _MMAN_H_
#define _MMAN_H_

#define MAP_PRIVATE 1
#define PROT_READ 1
#define MAP_FAILED ((void *) (-1))

void *mmap(void *addr, size_t len, int prot, int flags, int fd, off_t offset);

#endif
