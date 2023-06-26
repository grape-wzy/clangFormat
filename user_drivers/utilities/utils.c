/*******************************************************************************
* file    utils.c
* author  mackgim
* version 1.0.0
* date
* brief   utils
*******************************************************************************/

#include "utils.h"
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
//#include <math.h>
//#include <stdlib.h>
//#include <stdarg.h>
// 整型转字符串函数
uint32_t uint_to_char(uint32_t input, uint8_t* output)
{
	uint32_t number = input;
	int32_t number_count = 0;
	while (number)
	{
		number /= 10;
		number_count++;
	}
	number = input;
	for (int32_t i = number_count - 1; i >= 0; i--)
	{
		output[i] = (uint8_t)((number % 10) | 48);
		number /= 10;
	}

	output[number_count] = '\0';
	return number_count;
}

// 判断是否所有值都是同一个值
uint8_t all_is_same(uint32_t* buff, uint32_t count, uint32_t cmp)
{
	uint32_t error = 0;
	for (uint32_t i = 0; i < count; i++)
	{
		if (buff[i] == cmp)
		{
			error++;
		}
	}

	if (error == count)
	{
		return true;
	}
	return false;
}


uint8_t check_bcc(uint8_t* data, uint32_t size)
{
	uint8_t bcc_num = 0;
	uint32_t i = 0;
	while (i < size)
	{
		bcc_num ^= *data;
		data++;
		i++;
	}
	return bcc_num;
}


uint32_t bytes_to_str(uint8_t* byte, uint32_t size, uint8_t* str)
{

	uint32_t l = 0;
	for (uint32_t i = 0; i < size; i++)
	{
		sprintf((char*)&str[l], "%02X", byte[i]);
		l = strlen((const char*)str);
	}

	return l;
}