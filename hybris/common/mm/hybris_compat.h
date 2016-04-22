#ifndef HYBRIS_ANDROID_MM_COMPAT_H_
#define HYBRIS_ANDROID_MM_COMPAT_H_

size_t strlcpy(char *dest, const char *src, size_t size);
size_t strlcat(char *dst, const char *src, size_t size);

#define ELF_ST_BIND(x)          ((x) >> 4)

#ifndef PAGE_SIZE
#define PAGE_SIZE 4096
#endif

#define PAGE_MASK (~(PAGE_SIZE - 1))

/*
 * From bionic/libc/include/elf.h
 *
 * Android compressed rel/rela sections
 */
#define DT_ANDROID_REL (DT_LOOS + 2)
#define DT_ANDROID_RELSZ (DT_LOOS + 3)

#define DT_ANDROID_RELA (DT_LOOS + 4)
#define DT_ANDROID_RELASZ (DT_LOOS + 5)

#endif
