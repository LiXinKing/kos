
/*
 * Get physical address of first (actually last :-) free page, and mark it
 * used. If no free pages left, return 0.
 */
#include <mm.h>
unsigned long get_free_page(void)
{	
	
	volatile int i=0;
	unsigned long addr;
	while(mem_map[i++]!=0)
	{
		if(i>PAGING_PAGES)
		{
		printf("out of memory");
		while(1);
		}
	}
	mem_map[i-1]=1;
	addr=LOW_MEM+(i-1)*4096;
	addr=phy_to_vir(addr);
	i=0;
	while(i<1024)
	{
		
		*(volatile unsigned long *)(addr+i*4)=0x0;
		i++;
	}
	addr=vir_to_phy(addr);
//	printf("%X\n",addr);
//	int j=0;
//	while(j<1024){
//	*((volatile unsigned long *)(addr))=0x123456;
//	j++;
//	addr=addr+4;
//	}

//	addr=LOW_MEM+i*4096;
//	printf("aaa:%X\n",addr);
//	__asm__(
//		"mov r0,#0x0\n"
//		"mov r1,#1024\n"
//"loop:	str r0,[%0]\n"
//		"add %0,%0,#4\n"
//		"sub r1,r1,#1\n"
//		"cmp r1,#0\n"
//		"bne loop"
//		:
//		:"r"(addr)
//		:
//			);
//	printf("ok?\n");
//	unsigned long tmp;
//	j=0;
//	int k=0;
//	int h=0;
//	int u=0;
//	while(j<10){
//	tmp=*(volatile unsigned long *)(addr);
//	printf("%X\n",tmp);
//	if(tmp==0)k++;
//	else if(tmp==0x123456)h++;
//	else u++;
//	j++;
//	addr=addr-4;
//			}
//	printf("k:%d\nh:%d\n",k,h);
//	printf("ok\n");

//	while(1);
	printf("get_page out :      %X\n",addr);
	return addr;
}
/*unsigned long get_big_page(void)
{	
	
	int i=0,j=0;
	unsigned long addr;
	while((mem_map[i++]!=0)&(mem_map[i++]!=0)&(mem_map[i++]!=0)(mem_map[i++]!=0))
	{
		if(i>PAGING_PAGES)
		{
		printf("out of memory");
		while(1);
		}
	}

	mem_map[i--]=1;
	mem_map[i--]=1;
	mem_map[i--]=1;
	mem_map[i--]=1;
	addr=LOW_MEM+i*4096;
	while(j<4096)
	{
		*(volatile unsigned long *)(addr+j*4)=0x0;
		j++;
	}

	return addr;
}
*/
