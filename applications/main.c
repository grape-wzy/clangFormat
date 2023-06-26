/*******************************************************************************
* file    main.c
* author  mackgim
* version 1.0.0
* date
* brief   主函数
*******************************************************************************/

#include "main.h"


int main()
{

	platform_init();

	kprint("\r\n");
	kprint("----run----\r\n");
	kprint("\r\n");

	while (1)
	{
		itask_proc();
	}
}

