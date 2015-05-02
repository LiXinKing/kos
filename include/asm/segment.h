


extern inline unsigned char get_usr_byte(const char * addr)
{
	unsigned register char _v;
	extern struct task_struct *current;
	unsigned long _tmp = current->nr*0x4000000+(unsigned long)addr;
	__asm__ ("ldrb %0,[%1] \n":"=r" (_v):"r" (_tmp));
	return _v;
}

extern inline unsigned short get_usr_word(const unsigned short *addr)
{
	unsigned short _v;
	extern struct task_struct *current;
	unsigned long _tmp = current->nr*0x4000000+(unsigned long)addr;
	__asm__ ("ldrh %0,[%1] \n" :"=r" (_v):"r" (_tmp));
	return _v;
}


extern inline unsigned long get_usr_long(const unsigned long *addr)
{
	unsigned long _v;
	extern struct task_struct *current;
	unsigned long _tmp = current->nr*0x4000000+(unsigned long)addr;
	__asm__ ("ldr %0,[%1] \n" :"=r" (_v):"r" (_tmp));

	return _v;
}




extern inline void put_usr_byte(char val,char *addr)
{
	unsigned long _tmp = current->nr*0x4000000+(unsigned long)addr;

	__asm__ ("strb %0,[%1]\n"::"r" (val),"r" (_tmp));
}

extern inline void put_usr_word(short val,short * addr)
{
	unsigned long _tmp = current->nr*0x4000000+(unsigned long)addr;
	__asm__ ("strh %0,[%1]\n"::"r" (val),"r" (_tmp));
}

extern inline void put_usr_long(unsigned long val,unsigned long * addr)
{
	unsigned long _tmp = current->nr*0x4000000+(unsigned long)addr;
	__asm__ ("str %0,[%1]\n"::"r" (val),"r" (_tmp));
}


