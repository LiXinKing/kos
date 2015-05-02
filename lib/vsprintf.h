/*
 * Use local definitions of C library macros and functions
 * NOTE: The function implementations may not be as efficient
 * as an inline or assembly code implementation provided by a
 * native C library.
 */

#include "types.h"

#ifndef va_arg

#ifndef _VALIST
#define _VALIST
typedef char *va_list;
#endif /* _VALIST */

/*
 * Storage alignment properties
 */
#define	 NATIVE_INT		 int
#define  _AUPBND         (sizeof (NATIVE_INT) - 1)
#define  _ADNBND         (sizeof (NATIVE_INT) - 1)

/*
 * Variable argument list macro definitions
 */

#define _bnd(X, bnd)    (((sizeof (X)) + (bnd)) & (~(bnd)))
#define va_arg(ap, T)   (*(T *)(((ap) += (_bnd (T, _AUPBND))) - (_bnd (T,_ADNBND))))
#define va_end(ap)      (void) 0
#define va_start(ap, A) (void) ((ap) = (((char *) &(A)) + (_bnd (A,_AUPBND))))

#endif /* va_arg */
#define __xh "r1"
#define __xl "r0"

#define __asmeq(x, y)  ".ifnc " x "," y " ; .err ; .endif\n\t"

#define do_div(n,base)						\
({								\
	register unsigned int __base      asm("r4") = base;	\
	register unsigned long long __n   asm("r0") = n;	\
	register unsigned long long __res asm("r2");		\
	register unsigned int __rem       asm(__xh);		\
	asm(	__asmeq("%0", __xh)				\
		__asmeq("%1", "r2")				\
		__asmeq("%2", "r0")				\
		__asmeq("%3", "r4")				\
		"bl	__do_div64"				\
		: "=r" (__rem), "=r" (__res)			\
		: "r" (__n), "r" (__base)			\
		: "ip", "lr", "cc");				\
	n = __res;						\
	__rem;							\
})
unsigned long simple_strtoul(const char *cp,char **endp,unsigned int base);
long simple_strtol(const char *cp,char **endp,unsigned int base);
unsigned long long simple_strtoull(const char *cp,char **endp,unsigned int base);
long long simple_strtoll(const char *cp,char **endp,unsigned int base);
//static int  skip_atoi(const char **s);
//char * number(char * buf, char * end, long long num, int base, int size, int precision, int type);
int vsnprintf(char *buf, size_t size, const char *fmt, va_list args);
int snprintf(char * buf, size_t size, const char *fmt, ...);
int vsprintf(char *buf, const char *fmt, va_list args);
int sprintf(char * buf, const char *fmt, ...);
int vsscanf(const char * buf, const char * fmt, va_list args);
int sscanf(const char * buf, const char * fmt, ...);
