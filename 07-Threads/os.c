#include <stddef.h>
#include <stdint.h>
#include "reg.h"
#include "threads.h"

/*define input limitation*/
#define MAX 50

/* USART TXE Flag
 * This flag is cleared when data is written to USARTx_DR and
 * set when that data is transferred to the TDR
 */
#define USART_FLAG_TXE	((uint16_t) 0x0080)

/*USART RXNE Flag
 * When it is set, received data is ready to be read
 */
#define USART_FLAG_RXNE	((uint16_t) 0x0020)

int lock = 0;

void usart_init(void)
{
	*(RCC_APB2ENR) |= (uint32_t) (0x00000001 | 0x00000004);
	*(RCC_APB1ENR) |= (uint32_t) (0x00020000);

	/* USART2 Configuration, Rx->PA3, Tx->PA2 */
	*(GPIOA_CRL) = 0x00004B00;
	*(GPIOA_CRH) = 0x44444444;
	*(GPIOA_ODR) = 0x00000000;
	*(GPIOA_BSRR) = 0x00000000;
	*(GPIOA_BRR) = 0x00000000;

	*(USART2_CR1) = 0x0000000C;
	*(USART2_CR2) = 0x00000000;
	*(USART2_CR3) = 0x00000000;
	*(USART2_CR1) |= 0x2000;
}

void print_str(const char *str)
{
	while (*str) {
		while (!(*(USART2_SR) & USART_FLAG_TXE));
		*(USART2_DR) = (*str & 0xFF);
		str++;
	}
}

void print_char(const char *str)
{
	if(*str){
		while (!(*(USART2_SR) & USART_FLAG_TXE));
		*(USART2_DR) = (*str & 0xFF);
	}
}

char get_char()
{
		while (!(*(USART2_SR) & USART_FLAG_RXNE));
		return *(USART2_DR)  & 0xFF;
}

int fib(int number)
{
	if(number==0) return 0;
	int result, zero=0;
	asm volatile("push {r3, r4, r5, r6}");
	asm ("subs r4, %[num], %[int0]"::[num] "r" (number), [int0]"r" (zero));
	asm ("mov r3, #-1\n"
					"mov r4,#1\n"
					"mov r6,#0\n"
					"mov r5,#0\n"
			".Loop:\n"
					"add r5,r4,r3\n"
					"mov r3,r4\n"
					"mov r4,r5\n"

					"add r6,r6,#1\n"
					"cmp r0,r6\n"
					"bcs .Loop"
					);
	asm ("mov %[fibresult], r4":[fibresult]"=r"(result));
	asm volatile("pop {r3, r4, r5, r6 }	");

	return result;
}

char *strtok(char *str, char *symbol)
{
	static char *tmp;
	if(str!=NULL)
	{
		tmp = str;
		while((*tmp )!= *symbol)
			 tmp++;
		*tmp = '\0';
	}else{
		tmp++;
		if((*tmp)=='\0') return NULL;
		str = tmp;
		while((*tmp != '\0') || (*tmp )!= *symbol)
		{
			if(*tmp == '\0') break;
			 tmp++;
		}
		*tmp = '\0';
	}
	return str;
}

int strcmp(const char *str1, const char *str2)
{
	char c1, c2;
	while(*str1)
	{
		c1 = *str1++;
		c2 = *str2++;
		if(c1!=c2) return 1;
	}
	return 0;
}

int strlen(char *s)
{
	int i;
	for(i=0; s[i]!='\0'; i++);
	return i;
}

void reverse(char *s)
{
	int i, j;
	for(i=0, j=strlen(s)-1; i<j; i++,j-- )
	{
		char c = s[i];
		s[i] = s[j];
		s[j] = c;
	}
}

int atoi(char *s)
{
	int sum = 0,i;
	for(i=0; s[i]!='\0'; i++)
		sum = sum*10 + s[i] - '0';
	return sum;
}

void itoa(int n, char s[])
{
	int flag = 1;
	if(n<0)
	{
		n = -n;
		flag = 0;
	}
	int i=0;
	while(n!=0)
	{
		s[i++] = n%10 + '0';
		n = n/10;
	}
	if(!flag)
		s[i++] = '-';
	s[i] = '\0';
	reverse(s);
	print_str(s);
	print_str("\n");
}

void command_fib(void *number)
{
	while(1)
	{
		while(lock == 0);
		char *num = (char *)number;
		int result = fib(atoi(num));
		itoa(result, num);
		lock = 0;
	}
}

void command(char *cmd)
{
	char *c = strtok((char *)cmd, " ");
	if(!strcmp(c, "fib"))
	{
		c = strtok(NULL, " ");
		if (thread_create(command_fib, (void *)  c) == -1)
			print_str("Thread command_fib  creation failed\r\n");
		lock = 1;
	}
}

void shell(void *userdata)
{
	char buffer[MAX];
	int index = 0;
	while(1){
		while(lock==1);
		print_str("din@din-Inspiron-N3010$ ");
		for(index=0; index<MAX; index++)
		{
			buffer[index] = get_char();
	
			/*detect ENTER (13)*/
			if(buffer[index] == 13)
			{
				print_str("\n");
				buffer[index] = '\0';
				command(buffer);
				break;
			}
			print_char(&buffer[index]);
		}
	}
}


/* 72MHz */
#define CPU_CLOCK_HZ 72000000

/* 100 ms per tick. */
#define TICK_RATE_HZ 10

int main(void)
{
	const char *str1 = "Task1_shell";

	usart_init();

	if (thread_create(shell, (void *) str1) == -1)
		print_str("Thread shell creation failed\r\n");

	/* SysTick configuration */
	*SYSTICK_LOAD = (CPU_CLOCK_HZ / TICK_RATE_HZ) - 1UL;
	*SYSTICK_VAL = 0;
	*SYSTICK_CTRL = 0x07;

	thread_start();

	return 0;
}
