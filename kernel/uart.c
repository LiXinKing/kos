#include <uart.h>

void uart_init()
{
	/*  配置引脚
	 *	GPA0CON[1] = UART_0_TXD
	 *	GPA0CON[0] = UART_0_RXD
	 */
	GPA0CON = 0x22222222;
	GPA1CON=0x2222;
	/*
	 *	设置UART0的数据格式为:8个数据位，
	 *	一个停止位，无奇偶校验
	 */
	ULCON0 = (0 << 3)|(0 << 2)|(0x3 << 0);

	/*
	 *	设置UART0的时钟源为PCLK_PSYS = 66.7MHz，由时钟分频器产生
	 *	发送、接收均采用查询方式
	 */
	UCON0 = (0 << 10)|(0x1 << 2)|(0x1 << 0);

	/*  禁止UART0  FIFO！！！！！！！！！！！
	 *  这里如果使能的话实验效果差别相当大，
	 *  读者可以尝试修改为UFCON0 = (1 << 0);
     *	思考为什么效果会这样？
	 */
	UFCON0 = (0 << 0);

	/* 无流控 */
	UMCON0 = (0 << 4);

	/*
	 *	波特率 = 115200bps
	 *	分频系数 = ( PCLK_PSYS / (Baud * 16)) - 1；
	 *	分频系数 = UBRDIVn寄存器的值 + (UDIVSLOTn寄存器中1的个数) / 16;
	 *	For example:
	 *		PCLKP =66.7MHz，波特率设为115200
	 *		分频系数 = ( 66700000 / (115200 * 16)) - 1 = 35.2；
	 *		所以：UBRDIVn寄存器的值 = 35；
	 *		所以UDIVSLOTn寄存器的值的1的个数为2，
	 *		根据S5PV210手册p880可知 UDIVSLOTn= 0x808；
	 */

	UBRDIV0 = 0x23;
	UDIVSLOT0 = 0x1;
	return;
}

void putc(unsigned char c)
{
	/*直到发送缓存为空，不为空时一直等待*/
	while(!(UTRSTAT0 & (1 << 2)));

	/*向UTXH0 寄存器中写入数据，UART会自动将它发送出去*/
	UTXH0 = c;
	return;
}

unsigned char getc()
{
	/*直到发送缓存不为空，为空时一直等待*/
	while(!(UTRSTAT0 & (1 << 0)));
	/*直接读取URXH0 寄存器，即可获得接收到的数据*/
	return URXH0;
}
//这里getc和putc与另外两个输入和输出的函数是一模一样的，为了兼容，暂且不改了
