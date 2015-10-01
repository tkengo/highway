#ifndef _GLIBC_H_
#define _GLIBC_H_

void *glibc_rawmemchr(const void *s, int c_in);
void *glibc_memrchr(const void *s, int c_in, size_t n);

#endif // _GLIBC_H_
