#ifndef VIDEO_H
#define VIDEO_H

#include <stdint.h>

void paging_map_region(uint32_t virt, uint32_t phys, uint32_t size);
void video_init(uint32_t boot_magic, uint32_t mbi_addr);
void video_activate_after_paging(void);
int video_is_graphics(void);
void video_clear(void);
void video_putchar(char c);
void printk(const char *txt);

#endif
