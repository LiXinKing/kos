#include <types.h>
#include <movi.h>
#include <smdkv210single.h>
#include <s5pc110.h>


//使用的是irom中的固化的函数，地址为0xD0037f98
//同样0xd0037488为系统启动通道的固化数值，是选择通道一还是通道二，通道二只有在系统第一次启动失败的时候才会启用
typedef u32(*copy_sd_mmc_to_mem)
(u32 channel, u32 start_block, u16 block_size, u32 *trg, u32 init);

static void delay(void)
{
	int i=0;
	while(i<400000)i++;
}
void movi_bl2_copy(void)
{	

	ulong ch;
#if defined(CONFIG_EVT1)
	ch = *(volatile u32 *)(0xD0037488);
	copy_sd_mmc_to_mem copy_bl2 =
	    (copy_sd_mmc_to_mem) (*(u32 *) (0xD0037F98));

#else
	ch = *(volatile u32 *)(0xD003A508);
	copy_sd_mmc_to_mem copy_bl2 =
	    (copy_sd_mmc_to_mem) (*(u32 *) (0xD003E008));
#endif
	u32 ret;
	if (ch == 0xEB000000) {
		ret = copy_bl2(0, 1, total_size,
			sd_copy_base, 0);
		

	}
	else if (ch == 0xEB200000) {
		ret = copy_bl2(2, 1, total_size,
			sd_copy_base, 0);
		
	}
	else
		return;

	if (ret == 0)
			while(1);
	else
		return;
}

