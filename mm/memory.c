#include <types.h>
#include <s5pc110.h>
#include <stdio.h>
#include <mm.h>


unsigned char mem_map [ PAGING_PAGES ] = {0,};
unsigned long HIGH_MEMORY = 0;
//已经从物理地址转换成线性地址的值,相当于返回了0
static unsigned long  get_pgtb1()
{
	unsigned long pgt1;

	__asm__ __volatile__("mrc p15,0,r0,c2,c0,0\n"  "bic r0,r0,#0xF0000000\n""mov %0,r0\n" :"=r"(pgt1)
						::"memory");

	return pgt1;
//	return 0;
}
unsigned long create_map(unsigned long paddr)
{
	

}


void copy_page(unsigned long from, unsigned long to)
{
	from=phy_to_vir(from);
	to=phy_to_vir(to);
	unsigned long *from_p=(volatile unsigned long *)from;
	unsigned long *to_p=(volatile unsigned long *)to;
	int i=0;
	while(i<1024)
	{
	*to_p= *from_p;
	to_p++;
	from_p++;
	i++;
	}

}

void free_page(unsigned long addr)
{
	if(addr < LOW_MEM)return;
	if(addr >= HIGH_MEMORY)
		printf("try to free noneexistent page");
	addr-=LOW_MEM;
	addr>>=12;
	if(mem_map[addr]--)return;
	mem_map[addr]=0;
		printf("try to free free page ");

}
static unsigned long check_dir(unsigned long dir)
{
	if(dir&0x3)//四个字节对齐
		printf("In function check_dir,not alingment.. should panic\n");
	unsigned long *pg_base;
	unsigned long *page_table;

	pg_base=dir & 0xfffffff0;
	if((*(pg_base)&1))page_table=*(pg_base) & (0xfffff000);	
	else if((*(pg_base+1)&1))page_table=*(pg_base+1) & (0xfffff000);
	else if((*(pg_base+2)&1))page_table=*(pg_base+2) & (0xfffff000);
	else if((*(pg_base+3)&1))page_table=*(pg_base+3) & (0xfffff000);
	else return 0;	/* Out of memory, see freeing */
	return page_table;
}
//线性地址的free，注意的是我们会保留一级页表
int free_page_tables(unsigned long from,unsigned long size)
{
	unsigned long *pg_table;
	unsigned long * dir,nr;
	unsigned long *pg_base;
	unsigned long pgt1;

	if (from & 0xfffff)
		printf("free_page_tables called with wrong alignment");
	//第一MB不能释放
	//第一个64MB和2.5GB以上的线性地址不能用
	if ((from<0x24000000)|(from>0xd0000000))
		printf("Trying to free up swapper memory space");
		//多出一点就要多free一个page
		//Arm里后两位为0的话，代表这个也表是无效的
		//一级目录正常情况下后两位是01，而二级页目录正常情况下后两位是11
		//每一个一级页目录相代表的是1M的大小,超过了1MB就free掉两个page
//	pgt1=mem_base+n*1024*16;
	pgt1=0;
	
	size = (size + 0xfffff) >> 20;
	dir = (unsigned long *) (((from>>18) & 0x3ffc)+pgt1);
; /* _pg_dir = 0 */
	for ( ; size-->0 ; dir++) {
	//一级页表肯定会存在,常驻内存
		if (!(1 & *dir))
			continue;
		pg_table = (unsigned long *) (0xfffffc00 & *dir);
		for (nr=0 ; nr<256 ; nr++) {
			if (*pg_table) {
				if (3 & *pg_table)
					free_page(0xfffff000 & *pg_table);
					//表示在mem_map数组的标志位清零了
			//	else
			//		swap_free(*pg_table >> 1);
				*pg_table = 0;
			}
			pg_table++;
		}
		*dir = 0;
		if(check_dir(dir)==0)
		free_page(0xfffff000 & *dir);

	//	pg_base=(dir & 0xfffffff0);
		//free掉二级映射表所在的地址,一页内存可以放置能映射到4M的二级内存页表
	//	if((*(pg_base)==0)&(*(pg_base+1)==0)&(*(pg_base+2)==0)&(*(pg_base+3)==0))

	}
	return 0;
}

int copy_page_tables(unsigned long from,unsigned long to,long size,int n)
{
	unsigned long * from_page_table;
	unsigned long * to_page_table;
	unsigned long this_page;
	unsigned long * from_dir, * to_dir;
	unsigned long new_page;
	unsigned long nr;
	unsigned long *pg_base;
	unsigned long pgt_from;
	unsigned long pgt_to;

	unsigned long  num;

	if ((from&0xfffff) || (to&0xfffff))
		printf("copy_page_tables called with wrong alignment\n");
//	while(1)get_free_page();
	pgt_from=get_pgtb1();
//	pgt_to=pg_dir_base+n*16*1024;
	pgt_to=pgt_from;
	printf("copy_page_table pgt:    %X\n",pgt_from);
	from_dir = (unsigned long *) (((from>>18) & 0x3ffc)+pgt_from); /* _pg_dir = 0 */
	to_dir = (unsigned long *) (((to>>18) & 0x3ffc)+pgt_to);
	size = ((unsigned) (size+0xfffff)) >> 20;

	for( ; size-->0 ; from_dir++,to_dir++) {
		if (1 & *to_dir)
			printf("copy_page_tables: already exist\n");
		//	panic("copy_page_tables: already exist");
		if (!(1 & *from_dir))
			continue;

		if((to_page_table=check_dir(to_dir))==0)
			if (!(to_page_table = (unsigned long *) get_free_page()))
			return -1;	/* Out of memory, see freeing */
		
	//	pg_base=todir & 0xfffffff0;
	//	if((*(pg_base)&1))to_page_table=*(pg_base) & (0xfffff000);	
	//	else if((*(pg_base+1)&1))to_page_table=*(pg_base+1) & (0xfffff000);
	//	else if((*(pg_base+2)&1))to_page_table=*(pg_base+2) & (0xfffff000);
	//	else if((*(pg_base+3)&1))to_page_table=*(pg_base+3) & (0xfffff000);

		num=(to>>20)&3;
		to_page_table=((unsigned long) to_page_table)|(num<<10);
		*to_dir = ((unsigned long) to_page_table) | (num<<10) | (1<<5) | 1;
		
		from_page_table = (unsigned long *) (0xfffffc00 & *from_dir);
		from_page_table=(unsigned long *)phy_to_vir(from_page_table);
		to_page_table=(unsigned long *)phy_to_vir(to_page_table);
		printf("size:   %X\n",size);
		printf("to_page_table:   %X\n",to_page_table);
		printf("from_page_table:   %X\n",from_page_table);
		nr =256;
		for ( ; nr-- > 0 ; from_page_table++,to_page_table++) {
			this_page = *from_page_table;
			if (!this_page)
				continue;
			//下面这种情况表示该页是在交换设备中，除了存在为，其他位是在交换设备中位置的标号
		/*	if (!(3 & this_page)) {
				if (!(new_page = get_free_page()))
					return -1;
				read_swap_page(this_page>>2, (char *) new_page);
				*to_page_table = this_page;
				//这个dirty表示如果这一页重新交换出去要重新写入到交换设备
				*from_page_table = new_page | (PAGE_DIRTY | 7);
				continue;
			}*/
			//两个共用了所以设置为只读
			this_page = this_page & ~(1<<4);
			*to_page_table = this_page;
			//两种情况：一种是从内核copy到另外的一个进程中
			//一种是：从非内核进程1到非内核进程2
			//第一种情况不会设置from的页面为只读，即给内核读写权限，但把to设置为只读
			//言外之意就是to在写的时候只能重新分配页面
			//第二种情况是把to和from都设置为只读，那么谁先写谁就写时复制重新分配页面
			if (this_page > LOW_MEM) {
				*from_page_table = this_page;
				this_page -= LOW_MEM;
				this_page >>= 12;
				printf("this_page:     %X\n",this_page);
				mem_map[this_page]++;
			}
		}
	}
//	while(1);
	return 0;
}

/*
 * This function puts a page in memory at the wanted address.
 * It returns the physical address of the page gotten, 0 if
 * out of memory (either when trying to access page-table or
 * page.)
 */
/*
 *  Well, here is one of the most complicated functions in mm. It
 * copies a range of linerar addresses by copying only the pages.
 * Let's hope this is bug-free, 'cause this one I don't want to debug :-)

*/
static unsigned long put_page(unsigned long page,unsigned long address)
{	
	unsigned long *page_tb;
	unsigned long pgt1;
	unsigned long tmp;
	unsigned long  num;
	if (page < LOW_MEM || page >= HIGH_MEMORY)
		printf("Trying to put page %p at %p\n",page,address);
	if (mem_map[(page-LOW_MEM)>>12] != 1)
		printf("mem_map disagrees with %p at %p\n",page,address);
	
	pgt1=get_pgtb1();
	//__asm__ __volatile__("mrc p15,0,r0,c2,c0,0\n" "mov %0,r0" :"=r"(pgt1)
	//					::"memory");
	page_tb = ( unsigned long *)(pgt1+((address >> 18) & 0x3ffc));
	if ((*page_tb)&1)
		page_tb = (unsigned long *) ((0xfffffc00 & *page_tb));
	else {
		if((tmp=check_dir(page_tb))==0)
			if (!(tmp=get_free_page()))
				return 0;
		num=(address>>20)&3;
		tmp=tmp | (num<<10);
		*page_tb = tmp | 1 | (1 << 5);
		page_tb = (unsigned long *) tmp;
	}
	page_tb=phy_to_vir(page_tb);
	page_tb += (address >> 10) & 0x3fc;
	*(volatile unsigned long *)page_tb=page| (0xE) | (1<<4)|(1<<5);
     //   mcr	p15, 0, r0, c8, c7, 0   @ invalidate TLBs，使原来的TLB中内容无效
    // mcr	p15, 0, r0, c7, c5, 0   @ invalidate icache，使原来的icache中的内容无效
//	__asm__ __volatile__("mcr p15,0,r0,c8,c7,0\n" "mcr p15,0,r0,c7,c5,0\n" :::);
	return page;

}




void do_no_page(char *name,int domain,unsigned long address)
{
	int nr[4];
	unsigned long *tmp;
	unsigned long page;
	int block,i;
	struct m_inode * inode;
	unsigned long pgt1;

	//__asm__ __volatile__("mrc p15,0,r0,c2,c0,0\n" "mov %0,r0" :"=r"(pgt1)
	//					::"memory");
	//获取二级映射表的物理位置
//	page = *(unsigned long *)(pgt1+((address >> 18) & 0xffc));
//		page &= 0xfffffc00;
//		page += (address >> 10) & 0xffc;
//		tmp = (unsigned long *) page;
//		*tmp=get_free_page();
	printf("address:   %X\n",address);
		page=get_free_page();
		printf("page: %X\n",page);
		if(put_page(page,address));
	//	printf("%X\n",*(volatile unsigned long *)(0x23e48000));
		return;

/*	 unsigned long addr=0x2f200000;
	int j=0;
	int k=0;
	int h=0;
	 unsigned long tmp1;
	while(j<1024){
	*((volatile unsigned long *)(addr))=0x123456;
	j++;
	addr=addr+4;
	}
	addr=0x2f200000;
	int m=0;
	while(m<1024){
	tmp1=*(volatile unsigned long *)(addr);
	if(tmp1==0)k++;
	else if(tmp1==0x123456)h++;
	addr=addr+4;
	m++;
			}
	printf("k=%d\n h=%d\n",k,h);
*/
	
//	printf("bbbb:%X\n",*tmp);
/*	m=0;
	k=0;
	h=0;
	addr=0x2f200000;
	while(m<1024){
	tmp1=*(volatile unsigned long *)(addr);
	if(tmp1==0)k++;
	else if(tmp1==0x123456)h++;
	addr=addr+4;
	m++;
			}
	printf("k=%d\n h=%d\n",k,h);
		while(1);
		*/

}

void do_no_table(char *name,int domain,unsigned long address)
{
	unsigned long page;
	unsigned long pgt1;
	unsigned long *table;
	unsigned long *table_pg;
//	unsigned long table_offset;
//	unsigned long tmp;
	unsigned long  num;
	//__asm__ __volatile__("mrc p15,0,r0,c2,c0,0\n" "mov %0,r0" :"=r"(pgt1)
	//					::"memory");
	//获取二级映射表的物理位置
//	page = *(unsigned long *)(pgt1+((address >> 18) & 0xffc));
//		page &= 0xfffffc00;
//		page += (address >> 10) & 0xffc
//		tmp = (unsigned long *) page;
//		*tmp=get_free_page();
	pgt1=get_pgtb1();	
	printf("pgt1    %X\n",pgt1);
//每次设置4MB的页面。。刚好放满一个page
	table=(volatile unsigned long *)(pgt1+((address >> 18) & 0x3ffc));
	num=(address >> 20)&3;

	if(table_pg=check_dir(table))
	{
		*table=(*table_pg)|(num<<10)|1;
		return;
	
	}
	page=get_free_page();
	printf("table address      %X\n",address);
	printf("ppppp1:   %X\n",page);
	*table=page|(num<<10)|1;
	return;


/*	table_offset=((address >> 20) & 0xfff)>>2;
	table_pg = (volatile unsigned long *)(pgt1+(table_offset<<4));
	int j=0;
	while(j<4)
	{
		if((*table_pg)&1)break;
		table_pg++;
		j++;
	}
	if((*table_pg)&1)
	{
		*table=(*table_pg&0xFFFFF000)|(num<<10)|1;
		return;
	}
	page=get_free_page();
	*table=page|(num<<10)|1;*/


//	*table=page;
//	table++;
//	*table=page+1024;
//	table++;
//	*table=page+2048;
//	table++;
//	*table=page+3072;
		//printf("page: %X\n",page);
}

void un_wp_page(unsigned long * table_entry)
{
	unsigned long old_page,new_page;
		printf("table entry11111 :     %X\n",*table_entry);

	old_page = 0xfffff000 & *table_entry;
	//mem_map==1表示是一个独占的内存模块，说明原来的那个共享的进程已经分配了新的页面
//	printf("table_entry:      %X\n",old_page);
	if (old_page >= LOW_MEM && mem_map[MAP_NR(old_page)]==1) {
	//变成可写了
		*table_entry |= (1<<4);
		return;
	}
	if (!(new_page=get_free_page()))
	//	oom();//内存不够了
		printf("no memory!\n");
		//原页面是共享的时候，mem_map会>1
	if (old_page >= LOW_MEM)
		mem_map[MAP_NR(old_page)]--;
	//从一个物理地址到另外一个物理地址
	copy_page(old_page,new_page);
	*table_entry = new_page | (0xE)| (1<<4)|(1<<5);
	invalidate();
}	
void do_page_fault(char *name,int domain,unsigned long address)
{	
	unsigned long pgt1;
	unsigned long *ptg_dir;
	printf("In do_pege_fault fuction handle\n");
	
	//超出了人物的最大地址范围了（64MB）
//	if (address - current->start_code > TASK_SIZE) {
//		printk("Bad things happen: page error in do_wp_page\n\r");
//		do_exit(SIGSEGV);
//					}

	pgt1=get_pgtb1();
	un_wp_page((unsigned long *)
			(((address>>10) & 0x3fc) + phy_to_vir(0xfffffc00 &
			*((unsigned long *) ( ((address>>18) &0x3ffc) +pgt1)))));
	invalidate();

}


void write_verify(unsigned long address)
{
	unsigned long page;
	unsigned long pgt1;
	pgt1=get_pgtb1();
	//页目录项不存在则返回
	if (!( (page = *((unsigned long *) (((address>>18) & 0x3ffc)+pgt1)) )&1))
		return;
	page &= 0xfffffc00;
	page=phy_to_vir(page);
	page += ((address>>10) & 0x3fc);
	//注意：由于验证的需要这里的存在切不可写是相对于pl1而言的，实际上应该是相对于pl0而言的
	if ((0x23f & *(unsigned long *) page) == 0x2e)  
	/* non-writeable, present 存在但是不可以写*/
		//存在不可写执行写时复制。。下面这个函数就是执行写时复制。。当存在多个共享时
		//就会为这个页表分配一个新的内存，并且减少原页的共享计数
		un_wp_page((unsigned long *) page);
	return;
}


void get_empty_page(unsigned long address)
{
	unsigned long tmp;

	if (!(tmp=get_free_page()) || !put_page(tmp,address)) {
		free_page(tmp);		/* 0 is ok - ignored */
	//	oom();
		printf("out of memory\n");
	}
}

//address是逻辑地址即段转换前的地址因此需要加上每个进程的起始逻辑地址
//而上面的address是线性地址
/*static int try_to_share(unsigned long address, struct task_struct * p)
{
	unsigned long from;
	unsigned long to;
	unsigned long from_page;
	unsigned long to_page;
	unsigned long phys_addr;
	unsigned long *pg_base;
	unsigned long ptg1;
	unsigned long num;
	ptg1=get_pgtb1();
//当前进程和进程p在指定线性地址的内容
	from_page = to_page = (((address>>18) & 0x3ffc));
	from_page += ((p->start_code>>18) & 0x3ffc);
	to_page += ((current->start_code>>18) & 0x3ffc);
	from_page+=ptg1;
	to_page+=ptg1;
// is there a page-directory at from? 
	from = *(unsigned long *) from_page;
	if (!(from & 1))
		return 0;
	from &= 0xfffffc00;
	from_page = from + ((address>>10) & 0x3fc);
	phys_addr = *(unsigned long *) from_page;
// is the page clean and present? 
//	只有不dirty和存在的页面才会被共享
//	暂时没有dirty位

	if ((phys_addr & 0x3) != 0x01)
		return 0;
	phys_addr &= 0xfffff000;
	if (phys_addr >= HIGH_MEMORY || phys_addr < LOW_MEM)
		return 0;
	to = *(unsigned long *) to_page;
	if (!(to & 1))
	{
	//	pg_base=to_page & 0xfffffff0;
	//	if((*(pg_base)!=0))to=*(pg_base) & (0xfffff000);	
	//	else if((*(pg_base+1)!=0))to=*(pg_base+1) & (0xfffff000);
	//	else if((*(pg_base+2)!=0))to=*(pg_base+2) & (0xfffff000);
	//	else if((*(pg_base+3)!=0))to=*(pg_base+3) & (0xfffff000);
		num=(address>>20)&3;
		if((to=check_dir(to_page))==0)
			if (to = get_free_page())
				*(unsigned long *) to_page = to | (num << 10) | 1;
			else
			//oom();
				printf("no memory,try_to_share fuction\n");
	       	}
	to = *(unsigned long *)to_page;
	to_page = to + ((address>>10) & 0x3fc);
	//要分享的进程p的对应先行地址已经存在的话，就无法try to share了
	if (3 & *(unsigned long *) to_page)
		//panic("try_to_share: to_page already exists");
		printf("panic in try to share\n");
// share them: write-protect 、
//	分享的时候，原页要开启页保护，以便可以写时复制

	*(unsigned long *) from_page |= (1 << 9);
	*(unsigned long *) to_page = *(unsigned long *) from_page;

	phys_addr -= LOW_MEM;
	phys_addr >>= 12;
	mem_map[phys_addr]++;
	return 1;
}
*/
static int share_page()
{
	printf("something not completed in fuction share_page\n");
	return 1;

}


//主存开始：start_mem 结束：end_mem
void mem_init(long start_mem, long end_mem)
{
	int i;
//暂时设置成15MB对应的主存区加上显存和虚拟内存区
	HIGH_MEMORY = end_mem;
	for (i=0 ; i<PAGING_PAGES ; i++)
		mem_map[i] = USED;
	i = MAP_NR(start_mem);
	end_mem -= start_mem;
	end_mem >>= 12;
	printf("i:%d\n",i);
	printf("i:%d\n",i);
	printf("end_mem:%d\n",end_mem);
	//初始化真实适用的主存区。。去掉用于显存和虚拟盘的内存
	while (end_mem-->0)
		mem_map[i++]=0;
}

