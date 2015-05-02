#ifndef _MM_H
#define _MM_H

#define PAGE_SIZE 4096


extern unsigned long get_free_page(void);
#define  pg_dir_base 0x20000000

#define MemOffset 0x20000000

#define Mem_Base 0x00000000
#define Mem_Limited 0x100000

/* these are not to be changed without changing head.s etc */
#define LOW_MEM 0x20100000
extern unsigned long HIGH_MEMORY;
#define PAGING_MEMORY (63*1024*1024)
#define PAGING_PAGES (PAGING_MEMORY>>12)
#define MAP_NR(addr) (((addr)-LOW_MEM)>>12)
#define USED 100
#define phy_to_vir(x) ((long)x-MemOffset)
#define vir_to_phy(x) ((long)x+MemOffset)
extern unsigned char mem_map [ PAGING_PAGES ];


#define invalidate() \
 __asm__(  \
	"mcr	p15, 0, r0, c7, c5, 0\n" \
    "mcr	p15, 0, r0, c8, c7, 0\n" \
	)


#endif
