/*************************************************************************
	> File Name: memory.h
	> Author: jinlx
	> Mail: a1307468@126.com 
	> Created Time: 2014年07月07日 星期一 22时55分24秒
 ************************************************************************/

extern int free_page_tables(unsigned long from,unsigned long size,int n);
extern int copy_page_tables(unsigned long from,unsigned long to,long size,int n);

