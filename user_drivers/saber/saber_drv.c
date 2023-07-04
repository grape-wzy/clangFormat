/*******************************************************************************
 * file     saber_drv.c
 * author   mackgim
 * version  V1.0.0
 * date
 * brief ：
 *******************************************************************************/

/* Includes ------------------------------------------------------------------*/

#include "saber_drv.h"
#include "saber_config.h"
#include "standard_lib.h"

#ifdef DEBUG
#if 1
#define sabkpf(...) kprint(__VA_ARGS__)
#define sabnpf(...) nprint(__VA_ARGS__)
#define sabtpf(...) tprint(__VA_ARGS__)
#else
#define sabkpf(...)
#define sabnpf(...)
#define sabtpf(...)
#endif
#else
#define sabkpf(...)
#define sabnpf(...)
#define sabtpf(...)
#endif

#define SABER_FIFO_MAX_SIZE (512)
static __SABER_OPCODE_TypeDef sSaberOpCode = SAVER_OPCODE_NULL;

#pragma region lowlevel write and read

static uint8_t sWriteBuff[SABER_FIFO_MAX_SIZE];

uint8_t saber_write(__GYRO_ACC_HW_DRIVER_TypeDef *handle, uint16_t addr, uint16_t channel, uint8_t *buff, uint16_t buffsize)
{
	sWriteBuff[0] = (uint8_t)channel;
	sWriteBuff[1] = (buffsize & 0xff);
	sWriteBuff[2] = ((buffsize >> 8) & 0xff);
	memcpy(&sWriteBuff[3], buff, buffsize);

#ifdef DEBUG

	uint8_t ret = handle->transmit_receive(sWriteBuff, sWriteBuff, buffsize + 3, 1000);
	if (ret != STD_SUCCESS)
	{
		kprint("failed to write channel 0x%x\r\n", channel);
	}
	return ret;
#else
	return handle->transmit_receive(sWriteBuff, sWriteBuff, buffsize + 3, 1000);
#endif
}

uint8_t saber_read(__GYRO_ACC_HW_DRIVER_TypeDef *handle, uint16_t addr, uint16_t channel, uint8_t *buff, uint16_t buffsize)
{

	uint8_t data[3];
	data[0] = (uint8_t)channel;
	data[1] = (buffsize & 0xff);
	data[2] = ((buffsize >> 8) & 0xff);
#ifdef DEBUG1
	nprint("read: ");
	for (uint16_t i = 0; i < 3; i++)
	{
		nprint("0x%x,", data[i]);
	}
#endif
	uint8_t ret = handle->transmit_receive(data, data, 3, 1000);
	if (ret != STD_SUCCESS)
	{
		return ret;
	}
	ret = handle->transmit_receive(buff, buff, buffsize, 1000);
	if (ret != STD_SUCCESS)
	{
		kprint("failed to read channel 0x%x\r\n", channel);
	}

#ifdef DEBUG1
	nprint("read: ");
	for (uint16_t i = 0; i < (buffsize); i++)
	{
		nprint("0x%x,", buff[i]);
	}
	nprint("\r\n");
#endif
	return ret;
}

#pragma endregion

#pragma region 测量数据组装
// 根据头文件定义，组装数据包结构配置
uint32_t saber_assemble_packet_config(uint8_t *data)
{
	uint32_t payload_len = 0;
	for (uint16_t i = 0; i < sizeof(packet_session) / sizeof(uint16_t); i++)
	{
		if (packet_session[i] & 0x8000)
		{
			data[payload_len++] = 0xff;
			data[payload_len++] = 0xff;
			data[payload_len++] = (uint8_t)(packet_session[i] & 0xff);
			data[payload_len++] = (uint8_t)((packet_session[i] >> 8) & 0xff);
		}
	}

	return payload_len;
}

// 获取测量数据长度
uint16_t saber_get_packet_len(uint8_t *data)
{
	uint16_t i = 0, j = 0, temp = 0, p_len = 0;
	const uint16_t packet_single_len[42] =
		{7, 9, 9, 9, 15, 15, 15, 0, 15, 15, 15, 15, 15, 15, 7, 4, 0, 0, 0, 0, 0,
		 0, 0, 0, 19, 19, 39, 0, 0, 0, 15, 15, 7, 0, 9, 0, 0, 0, 7, 7, 0, 43};
	kprint("l=%u\r\n", data[5] / 4);
	for (i = 0; i < data[5] / 4; i++)
	{
		temp = 9 + i * 4;
		if ((data[temp] & 0x80) == 0x80)
		{
			uint16_t config = (((uint16_t)data[temp] << 8) & 0xff00) | data[temp - 1];

			// for (j = 0; i < 42; j++)
			for (j = 0; j < 42; j++)
			{
				if (config == packet_session[j])
				{
					sabkpf("%u-%u - 0x%x-%u\r\n", i, j, config & 0x7fff, packet_single_len[j]);
					p_len += packet_single_len[j];
					break;
				}
			}
		}
	}
	return p_len;
}

#pragma endregion

#pragma region 发送并回复

// 获取测量数据
uint8_t saber_read_measure_data(void *handle, uint8_t *output, uint16_t *output_size)
{
	__GYRO_ACC_HANDLE_TypeDef *gah = (__GYRO_ACC_HANDLE_TypeDef *)handle;
	uint8_t ret = STD_FAILED;
	__SABER_FIFO_TypeDef res = {0, 0};
	__SABER_FIFO_TypeDef res1 = {0, 0};
#if 0
	ret = saber_read(gah->hw_drv, SABER_I2C_ADDR, SABER_FIFO_CHANNEL, (uint8_t*)&res, sizeof(res));
	if (ret != STD_SUCCESS)
	{
		sabkpf("failed to read\r\n");
		goto failed;
	}
	if (res.measure_unread == 0)
	{
		sabkpf("measure_unread = 0\r\n");
		ret = STD_NODATA;
		goto failed;
	}
#else
	uint8_t count1 = 0;

	do
	{
		ret = saber_read(gah->hw_drv, SABER_I2C_ADDR, SABER_FIFO_CHANNEL, (uint8_t *)&res, sizeof(res));

		if ((ret == STD_SUCCESS) && (res.measure_unread != 0) && (res.measure_unread < 256))
		{
			// for (uint32_t i = 0; i < 0x1f; i++)
			//{
			//	__NOP();
			// }

			ret = saber_read(gah->hw_drv, SABER_I2C_ADDR, SABER_FIFO_CHANNEL, (uint8_t *)&res1, sizeof(res1));
			if ((ret == STD_SUCCESS) && (res1.measure_unread == res.measure_unread))
			{
				break;
			}
			else
			{
				sabtpf("two unread , %u-%u\r\n", (unsigned int)res.measure_unread, (unsigned int)res1.measure_unread);
			}
		}
		if (ret != STD_SUCCESS)
		{
			sabtpf("failed to read, %u\r\n", count1);
			ret = STD_FAILED;
		}
		else
		// if (res.measure_unread == 0)
		{
			sabtpf("measure_unread = 0, %u-%u\r\n", (unsigned int)res.response_unread, count1);
			ret = STD_NODATA;
		}

		count1++;
		if (count1 == 4)
		{
			goto failed;
		}

	} while (1);
#endif

	res.measure_unread = 83;
	// tprint("%u\r\n", (unsigned int)res.measure_unread);
	ret = saber_read(gah->hw_drv, SABER_I2C_ADDR, SABER_MEASURE_CHANNEL, (uint8_t *)output, res.measure_unread);
	// ret = saber_read(gah->hw_drv, SABER_I2C_ADDR, SABER_MEASURE_CHANNEL, (uint8_t*)output, output_size);
	if (ret != STD_SUCCESS)
	{
		sabkpf("failed to read\r\n");
		goto failed;
	}

	if ((output[0] != 0x41) || (output[1] != 0x78) || (output[res.measure_unread - 1] != 0x6D))
	{
		sabkpf("not a frame-%u\r\n", (unsigned int)res.measure_unread);
		ret = STD_FAILED;
		goto failed;
	}
	if (res.measure_unread != (uint32_t)(output[5] + 8))
	{
		sabkpf("size error - %u-%u\r\n", (unsigned int)res.measure_unread, (output[5] + 8));
		ret = STD_FAILED;
		goto failed;
	}

	uint8_t bcc = check_bcc(output, res.measure_unread - 2);
	if (bcc != output[res.measure_unread - 2])
	{
		ret = STD_ERROR;
		sabkpf("[saber]: crc wrong\r\n");
		sabkpf("[saber]: frame - size=%u, cb=0x%x, dc=0x%x,", (unsigned int)res.measure_unread, bcc, output[res.measure_unread - 2]);
		goto failed;
	}
	*output_size = res.measure_unread;
	ret = STD_SUCCESS;
failed:
	// sabkpf("[saber]: failed to read data, ret=%u\r\n", ret);
#ifdef DEBUG
{
	static uint8_t first = 0;

	if ((ret != STD_SUCCESS) && (res.measure_unread > 0) && (res.measure_unread < 1024))
	{
		sabkpf("error data (size=%u):", (unsigned int)res.measure_unread);
		for (uint16_t i = 0; i < res.measure_unread; i++)
		{
			sabnpf("0x%x-", output[i]);
		}
		sabnpf("\r\n");
	}
	else if ((first == 0) && (ret == STD_SUCCESS))
	{
		first = 1;
		sabkpf("right data (size=%u):", (unsigned int)res.measure_unread);
		for (uint16_t i = 0; i < res.measure_unread; i++)
		{
			sabnpf("0x%x-", output[i]);
		}
		sabnpf("\r\n");
	}
	// else
	//{
	//	sabkpf("error %u,  (size=%u)\r\n", ret, (unsigned int)res.measure_unread);
	// }
}
#if 0
	static uint32_t count = 0;
	count++;
	if (count == 100)
	{
		count = 0;

		if ((ret == STD_SUCCESS) && (res.measure_unread < 1024))
		{
			sabkpf("ok data (size=%u):", (unsigned int)res.measure_unread);
			for (uint16_t i = 0; i < res.measure_unread; i++)
			{
				sabnpf("0x%x-", output[i]);
			}
			sabnpf("\r\n");
		}
	}
#endif
#endif
	return ret;
}

// 读取saber缓存中的应答数据
uint8_t saber_drv_read_fifo(void *handle, uint8_t *output, uint16_t *output_size)
{
	__GYRO_ACC_HANDLE_TypeDef *gah = (__GYRO_ACC_HANDLE_TypeDef *)handle;
	__SABER_FIFO_TypeDef res;
	gah->hw_drv->enable_irq(0);
	uint8_t ret = saber_read(gah->hw_drv, SABER_I2C_ADDR, SABER_FIFO_CHANNEL, (uint8_t *)&res, sizeof(res));

	if (ret != STD_SUCCESS)
	{
		sabkpf("failed to read fifo, ret=%u\r\n", ret);
		goto failed;
	}
	if ((res.response_unread >= SABER_FIFO_MAX_SIZE) || (res.response_unread == 0))
	{
		sabkpf("wrong size(max 1024), r=%u, ret=%u\r\n", (unsigned int)res.response_unread, ret);
		ret = STD_FAILED;
		goto failed;
	}
	Clock_Wait(1);
	ret = saber_read(gah->hw_drv, SABER_I2C_ADDR, SABER_RESPONSE_CHANNEL, (uint8_t *)output, res.response_unread);
	if (ret != STD_SUCCESS)
	{
		sabkpf("failed to read data, ret=%u\r\n", ret);
		goto failed;
	}

	if ((output[0] != 0x41) || (output[1] != 0x78) || (output[res.response_unread - 1] != 0x6D))
	{
		sabkpf("data is not a frame, ");
		for (uint16_t i = 0; i < res.response_unread; i++)
		{
			sabnpf("0x%x,", output[i]);
		}
		sabnpf("\r\n");
		ret = STD_FAILED;
		goto failed;
	}

	uint8_t bcc = check_bcc(output, res.response_unread - 2);
	if (bcc != output[res.response_unread - 2])
	{
		ret = STD_ERROR;
		sabkpf("crc error, ret=%u\r\n", ret);
		goto failed;
	}
	// sabkpf("read, size=%u\r\n", (unsigned int)res.response_unread);
	*output_size = (uint16_t)res.response_unread;
	ret = STD_SUCCESS;
failed:
	gah->hw_drv->enable_irq(1);
	return ret;
}

// 发送并获取返回数据
uint8_t saber_drv_send_msg(void *handle, uint8_t *input, uint16_t input_size, uint8_t *output, uint16_t *output_size)
{
	__GYRO_ACC_HANDLE_TypeDef *gah = (__GYRO_ACC_HANDLE_TypeDef *)handle;
	gah->hw_drv->enable_irq(0);
	uint8_t ret = saber_write(gah->hw_drv, SABER_I2C_ADDR, SABER_MESSAGE_CHANNEL, input, input_size);
	__SABER_OPCODE_TypeDef lastoc = sSaberOpCode;
	sSaberOpCode = SAVER_OPCODE_WAIT_FOR_RSP;
	gah->hw_drv->enable_irq(1);
	if (ret != STD_SUCCESS)
	{
		sabkpf("failed to write, ret=%u\r\n", ret);
		goto ending;
	}
	UTIL_SEQ_WaitEvt(1 << CFG_IDLEEVT_SABER_WAIT_ID);
	ret = saber_drv_read_fifo(handle, output, output_size);
ending:
	sSaberOpCode = lastoc;
	return ret;
}
#pragma endregion

#pragma region 唤醒操作

// 回应唤醒状态
uint8_t saber_ack_wakeup(void *handle)
{
	__GYRO_ACC_HANDLE_TypeDef *gah = (__GYRO_ACC_HANDLE_TypeDef *)handle;

	uint8_t ret;
	uint8_t iconfig[8], index = 0;
	iconfig[index++] = 0x41;					/*    0x41 0x78 is our protocol header   */
	iconfig[index++] = 0x78;					/*                                       */
	iconfig[index++] = 0xff;					/*    0xff is our protocol maddr         */
	iconfig[index++] = 0x01;					/*    0x01 is our protocol class ID      */
	iconfig[index++] = 0x81;					/*    0x02 is our protocol message ID    */
	iconfig[index++] = 0x00;					/*    0x00 is our protocol payload length*/
	iconfig[index] = check_bcc(iconfig, index); /*this is our protocol CRC, you can use the function*/
	index++;									/*check_bcc to get the value                        */
	iconfig[index++] = 0x6d;					/*    0x6d is our protocol tail          */

	gah->hw_drv->enable_irq(0);
	ret = saber_write(gah->hw_drv, SABER_I2C_ADDR, SABER_MESSAGE_CHANNEL, iconfig, index);
	gah->hw_drv->enable_irq(1);
	if (ret != STD_SUCCESS)
	{
		sabkpf("failed\r\n");
		return ret;
	}
	sabkpf("ok\r\n");
	return STD_SUCCESS;
}

// 等待saber初始化完成
uint8_t saber_wait_for_wakeup(void *handle)
{
	//__GYRO_ACC_HANDLE_TypeDef* gah = (__GYRO_ACC_HANDLE_TypeDef*)handle;
	sSaberOpCode = SAVER_OPCODE_WAIT_FOR_RSP;

	uint8_t data[512];
	uint16_t datasize = 0;
	uint8_t ret;
	while (1)
	{
		// sabtpf("again\r\n");
		UTIL_SEQ_WaitEvt(1 << CFG_IDLEEVT_SABER_WAIT_ID);
		// sabtpf("here\r\n");
		ret = saber_drv_read_fifo(handle, data, &datasize);

		if (ret != STD_SUCCESS)
		{
			// sabkpf("failed to read\r\n");
			continue;
		}
		if ((data[3] != 0x1) || (data[4] != 0x01))
		{
#ifdef DEBUG
			sabtpf("saber not wakeup, ");
			for (uint16_t i = 0; i < datasize; i++)
			{
				sabnpf("0x%x,", data[i]);
			}
			sabnpf("\r\n");
#endif
			continue;
		}

		sabtpf("saber wakeup\r\n");
		sSaberOpCode = SAVER_OPCODE_NULL;
		saber_ack_wakeup(handle);
		break;
	} // while (1);

	return STD_SUCCESS;
}

#pragma endregion

#pragma region 设置操作

// 设置odr，采样频率
uint8_t saber_set_packet_rate(void *handle, uint16_t odr)
{
	uint8_t iconfig[24], index = 0;
	iconfig[index++] = 0x41;
	iconfig[index++] = 0x78;
	iconfig[index++] = 0xff;
	iconfig[index++] = 0x04;
	iconfig[index++] = 0x10;
	iconfig[index++] = 0x02;
	iconfig[index++] = (uint8_t)(odr & 0xff);
	iconfig[index++] = (uint8_t)((odr >> 8) & 0xff);
	;
	iconfig[index] = check_bcc(iconfig, index);
	index++;
	iconfig[index++] = 0x6d;

	// sSaberRsp.CID = 0x4;
	// sSaberRsp.MID = 0x90;
	uint8_t output[512];
	uint16_t outputsize = 0;
	uint8_t ret = saber_drv_send_msg(handle, iconfig, index, output, &outputsize);

	if (ret != STD_SUCCESS)
	{
		sabkpf("failed to set odr\r\n");
		return STD_FAILED;
	}

	if ((output[3] != 0x04) && (output[4] != 0x90))
	{
		kprint("failed , cid=0x%x, mid=0x%x\r\n", output[3], output[4]);
		return STD_FAILED;
	}
	sabkpf("ok\r\n");
	return STD_SUCCESS;
}

// 获取odr，采样频率
uint8_t saber_get_packet_rate(void *handle, uint16_t *value)
{
	uint8_t iconfig[24], index = 0;
	iconfig[index++] = 0x41;
	iconfig[index++] = 0x78;
	iconfig[index++] = 0xff;
	iconfig[index++] = 0x04;
	iconfig[index++] = 0x11;
	iconfig[index++] = 0x0;
	iconfig[index] = check_bcc(iconfig, index);
	index++;
	iconfig[index++] = 0x6d;

	// sSaberRsp.CID = 0x4;
	// sSaberRsp.MID = 0x90;
	uint8_t output[512];
	uint16_t outputsize = 0;
	uint8_t ret = saber_drv_send_msg(handle, iconfig, index, output, &outputsize);

	if (ret != STD_SUCCESS)
	{
		sabkpf("failed to set odr\r\n");
		return STD_FAILED;
	}

	if ((output[3] != 0x04) && (output[4] != 0x91))
	{
		kprint("failed , cid=0x%x, mid=0x%x\r\n", output[3], output[4]);
		return STD_FAILED;
	}
	uint16_t odr = ((((uint16_t)output[7] << 8) & 0xff00) | output[6]);
	sabkpf("odr=%u\r\n", odr);
	if (value != NULL)
	{
		*value = odr;
	}
	return STD_SUCCESS;
}

// 设置数据帧结构
uint8_t saber_set_packet_config(void *handle)
{
	uint8_t iconfig[240], index = 0, payload_len = 0;
	iconfig[index++] = 0x41;
	iconfig[index++] = 0x78;
	iconfig[index++] = 0xff;
	iconfig[index++] = 0x06;
	iconfig[index++] = 0x0a;
	payload_len = saber_assemble_packet_config(&iconfig[6]);
	iconfig[index++] = payload_len;
	index += payload_len;
	iconfig[index] = check_bcc(iconfig, index);
	index++;
	iconfig[index++] = 0x6d;

	// #ifdef DEBUG
	//	sabkpf("[saber]: message(send data packet) - ");
	//	for (uint16_t i = 0; i < index; i++)
	//	{
	//		sabkpf("0x%x,", iconfig[i]);
	//	}
	//	sabkpf("\r\n");
	// #endif

	uint8_t output[512];
	uint16_t outputsize = 0;
	uint8_t ret = saber_drv_send_msg(handle, iconfig, index, output, &outputsize);

	if (ret != STD_SUCCESS)
	{
		sabkpf("failed to set data packet\r\n");
		return STD_FAILED;
	}

	if ((output[3] != 0x06) && (output[4] != 0x8a))
	{
		kprint("failed , cid=0x%x, mid=0x%x\r\n", output[3], output[4]);
		return STD_FAILED;
	}
	sabkpf("ok\r\n");
	return STD_SUCCESS;
}

// 获取数据帧结构和长度
uint8_t saber_get_packet_config(void *handle, uint16_t *value)
{
	uint8_t iconfig[24], index = 0;

	iconfig[index++] = 0x41;
	iconfig[index++] = 0x78;
	iconfig[index++] = 0xff;
	iconfig[index++] = 0x06;
	iconfig[index++] = 0x0b;
	iconfig[index++] = 0x00;
	iconfig[index] = check_bcc(iconfig, index);
	index++;
	iconfig[index++] = 0x6d;

	uint8_t output[512];
	uint16_t outputsize = 0;
	uint8_t ret = saber_drv_send_msg(handle, iconfig, index, output, &outputsize);

	if (ret != STD_SUCCESS)
	{
		sabkpf("failed to get data packet\r\n");
		return STD_FAILED;
	}

	if ((output[3] != 0x06) && (output[4] != 0x8b))
	{
		kprint("failed , cid=0x%x, mid=0x%x\r\n", output[3], output[4]);
		return STD_FAILED;
	}
	// sabkpf("ok\r\n");
	uint16_t l = saber_get_packet_len(output);
	if (value != NULL)
	{
		*value = l;
	}
	sabkpf("measure length = %u\r\n", l);
	return STD_SUCCESS;
}

// 获取就绪信号设置
uint8_t saber_get_drdy_conf(void *handle, uint16_t *value)
{
	uint8_t iconfig[24], index = 0;
	iconfig[index++] = 0x41;
	iconfig[index++] = 0x78;
	iconfig[index++] = 0xff;
	iconfig[index++] = 0x06;
	iconfig[index++] = 0x07;
	iconfig[index++] = 0x00;
	iconfig[index] = check_bcc(iconfig, index);
	index++;
	iconfig[index++] = 0x6d;

	uint8_t output[512];
	uint16_t outputsize = 0;
	uint8_t ret = saber_drv_send_msg(handle, iconfig, index, output, &outputsize);

	if (ret != STD_SUCCESS)
	{
		sabkpf("failed to get drdy\r\n");
		return STD_FAILED;
	}

	if ((output[3] != 0x06) && (output[4] != 0x87))
	{
		kprint("failed , cid=0x%x, mid=0x%x\r\n", output[3], output[4]);
		return STD_FAILED;
	}
	// sabkpf("ok\r\n");

	uint16_t drdy = ((((uint16_t)output[7] << 8) & 0xff00) | output[6]);
	sabkpf("drdy=0x%x\r\n", drdy);
	if (value != NULL)
	{
		*value = drdy;
	}
	return STD_SUCCESS;
}

// 设置算法引擎
uint8_t saber_set_fusion_mode(void *handle, uint8_t mode)
{
	if (mode > 3)
	{
		sabkpf("failed ,mode>%u\r\n", mode);
		return STD_FAILED;
	}
	uint8_t iconfig[24], index = 0;
	iconfig[index++] = 0x41;
	iconfig[index++] = 0x78;
	iconfig[index++] = 0xff;
	iconfig[index++] = 0x04;
	iconfig[index++] = 0x09;
	iconfig[index++] = 0x01;
	iconfig[index++] = mode;
	iconfig[index] = check_bcc(iconfig, index);
	index++;
	iconfig[index++] = 0x6d;

	uint8_t output[512];
	uint16_t outputsize = 0;
	uint8_t ret = saber_drv_send_msg(handle, iconfig, index, output, &outputsize);

	if (ret != STD_SUCCESS)
	{
		sabkpf("failed to set odr\r\n");
		return STD_FAILED;
	}

	if ((output[3] != 0x4) && (output[4] != 0x89))
	{
		kprint("failed , cid=0x%x, mid=0x%x\r\n", output[3], output[4]);
		return STD_FAILED;
	}
	sabkpf("ok\r\n");
	return STD_SUCCESS;
}

// 获取算法引擎
uint8_t saber_get_fusion_mode(void *handle, uint8_t *value)
{
	uint8_t iconfig[24], index = 0;
	iconfig[index++] = 0x41;
	iconfig[index++] = 0x78;
	iconfig[index++] = 0xff;
	iconfig[index++] = 0x04;
	iconfig[index++] = 0x0A;
	iconfig[index++] = 0x00;
	iconfig[index] = check_bcc(iconfig, index);
	index++;
	iconfig[index++] = 0x6d;

	uint8_t output[512];
	uint16_t outputsize = 0;
	uint8_t ret = saber_drv_send_msg(handle, iconfig, index, output, &outputsize);

	if (ret != STD_SUCCESS)
	{
		sabkpf("failed to get drdy\r\n");
		return STD_FAILED;
	}

	if ((output[3] != 0x4) && (output[4] != 0x8a))
	{
		kprint("failed , cid=0x%x, mid=0x%x\r\n", output[3], output[4]);
		return STD_FAILED;
	}

#ifdef DEBUG
	uint8_t mode = output[6];
	static const uint8_t sConstFusionMode[][30] = {
		"Sophisticate Without Magnet",
		"Sophisticate With Magnet",
		"Swfit Without Magnet",
		"Swfit With Magnet",
		"NULL",
	};
	sabkpf("fusion mode=0x%x, %s\r\n", mode, sConstFusionMode[mode > 3 ? 4 : mode]);
#endif
	if (value != NULL)
	{
		*value = output[6];
	}
	return STD_SUCCESS;
}

// 转到测量状态
uint8_t saber_switch_to_measure(void *handle)
{
	uint8_t iconfig[24], index = 0;
	iconfig[index++] = 0x41;
	iconfig[index++] = 0x78;
	iconfig[index++] = 0xff;
	iconfig[index++] = 0x01;
	iconfig[index++] = 0x03;
	iconfig[index++] = 0x00;
	iconfig[index] = check_bcc(iconfig, index);
	index++;
	iconfig[index++] = 0x6d;

	uint8_t output[512];
	uint16_t outputsize = 0;
	uint8_t ret = saber_drv_send_msg(handle, iconfig, index, output, &outputsize);

	if (ret != STD_SUCCESS)
	{
		sabkpf("failed\r\n");
		return STD_FAILED;
	}
	if ((output[3] != 0x1) && (output[4] != 0x83))
	{
		kprint("failed , cid=0x%x, mid=0x%x\r\n", output[3], output[4]);
		return STD_FAILED;
	}
	return STD_SUCCESS;
}
// 转到配置状态
uint8_t saber_switch_to_config(void *handle)
{
	uint8_t iconfig[8], index = 0;
	iconfig[index++] = 0x41;					/*    0x41 0x78 is our protocol header   */
	iconfig[index++] = 0x78;					/*                                       */
	iconfig[index++] = 0xff;					/*    0xff is our protocol maddr         */
	iconfig[index++] = 0x01;					/*    0x01 is our protocol class ID      */
	iconfig[index++] = 0x02;					/*    0x02 is our protocol message ID    */
	iconfig[index++] = 0x00;					/*    0x00 is our protocol payload length*/
	iconfig[index] = check_bcc(iconfig, index); /*this is our protocol CRC, you can use the function*/
	index++;									/*check_bcc to get the value                        */
	iconfig[index++] = 0x6d;					/*    0x6d is our protocol tail          */

	uint8_t output[512];
	uint16_t outputsize = 0;
	uint8_t ret = saber_drv_send_msg(handle, iconfig, index, output, &outputsize);

	if (ret != STD_SUCCESS)
	{
		sabkpf("failed\r\n");
		return STD_FAILED;
	}
	if ((output[3] != 0x1) && (output[4] != 0x82))
	{
		kprint("failed , cid=0x%x, mid=0x%x\r\n", output[3], output[4]);
		return STD_FAILED;
	}
	return STD_SUCCESS;
}

#pragma endregion

#pragma region 接口

uint8_t saber_drv_init(void *handle)
{
	__GYRO_ACC_HANDLE_TypeDef *gah = (__GYRO_ACC_HANDLE_TypeDef *)handle;
	sSaberOpCode = SAVER_OPCODE_NULL;
	gah->hw_drv->pwr_ctrl(0);
	Clock_Wait(20);
	gah->hw_drv->pwr_ctrl(1);
	gah->hw_drv->init();
	gah->hw_drv->enable_irq(0);
	gah->hw_drv->reset(0);
	Clock_Wait(100);
	gah->hw_drv->reset(1);
	// Clock_Wait(100);
	gah->hw_drv->enable_irq(1);
	sabkpf("start to init saber\r\n");
	uint8_t ret = saber_wait_for_wakeup(handle);
	if (ret != STD_SUCCESS)
	{
		sabkpf("failed to wait for wakeup\r\n");
		goto ending;
	}
	Clock_Wait(100);

	uint8_t mode;
	ret = saber_get_fusion_mode(handle, &mode);
	if (ret != STD_SUCCESS)
	{
		goto ending;
	}
	if (mode != gah->config->Mode)
	{
		ret = saber_set_fusion_mode(handle, gah->config->Mode);
		if (ret != STD_SUCCESS)
		{
			goto ending;
		}
		ret = saber_get_fusion_mode(handle, &mode);
		if (ret != STD_SUCCESS)
		{
			goto ending;
		}
		if (mode != gah->config->Mode)
		{
			sabkpf("failed to set mode\r\n");
		}
	}

	sabkpf("set packet rate %u\r\n", gah->config->DataRate);
	ret = saber_set_packet_rate(handle, gah->config->DataRate);
	if (ret != STD_SUCCESS)
	{
		goto ending;
	}

	ret = saber_get_packet_rate(handle, NULL);
	if (ret != STD_SUCCESS)
	{
		goto ending;
	}
	ret = saber_set_packet_config(handle);
	if (ret != STD_SUCCESS)
	{
		goto ending;
	}
	// ret = saber_get_packet_config(handle, &sSaberResultLenth);
	// if (ret != STD_SUCCESS)
	//{
	//	goto ending;
	// }
	ret = saber_get_drdy_conf(handle, NULL);
	if (ret != STD_SUCCESS)
	{
		goto ending;
	}

	sabkpf("ok\r\n");
ending:
	return ret;
}

uint8_t saber_drv_deinit(void *handle)
{
	__GYRO_ACC_HANDLE_TypeDef *gah = (__GYRO_ACC_HANDLE_TypeDef *)handle;
	sSaberOpCode = SAVER_OPCODE_NULL;

	gah->hw_drv->enable_irq(0);
	gah->hw_drv->reset(0);
	gah->hw_drv->deinit();
	gah->hw_drv->pwr_ctrl(0);

	sabkpf("ok\r\n");
	return STD_SUCCESS;
}

uint8_t saber_drv_start(void *handle)
{
	uint8_t ret = saber_switch_to_measure(handle);
	if (ret == STD_SUCCESS)
	{
		sSaberOpCode = SAVER_OPCODE_WAIT_FOR_DATA;
		sabkpf("ok\r\n");
	}
	return ret;
}

uint8_t saber_drv_stop(void *handle)
{

	sSaberOpCode = SAVER_OPCODE_NULL;
	uint8_t ret = saber_switch_to_config(handle);
	sabkpf("ok\r\n");
	return ret;
}

uint8_t saber_drv_read_raw(void *handle, uint8_t *value, uint16_t size)
{
#ifdef DEBUG
	static uint32_t sSaberResultCount = 0;
	sSaberResultCount++;
#endif

	__SABER_RESULT_TypeDef *ouput = (__SABER_RESULT_TypeDef *)value;
	uint8_t raw[256];
	uint16_t raw_size = 0;
	uint8_t ret = saber_read_measure_data(handle, raw, &raw_size);
	if (ret != STD_SUCCESS)
	{
		sabkpf("failed\r\n");
		return ret;
	}
	uint8_t *data = &raw[6];
	__SABER_PACKET_SESSION_HEAD_TypeDef psh;
	uint16_t l = 0;

	if (raw_size == 0)
	{
		sabkpf("size=0\r\n");
		return STD_FAILED;
	}
	uint16_t data_size = raw_size - 8; // 8的组成, 2-帧头，3-命令字，1-长度,1-校验码,1-帧尾
	do
	{
		memcpy((char *)&psh, data, sizeof(psh));
		if (psh.length == 0)
		{
			sabkpf("see the ghost - 0x%x-0x%x, %u\r\n", (unsigned int)&raw[0], (unsigned int)data, (unsigned int)sSaberResultCount);
			return STD_NODATA;
		}

		// if (!(psh.session & 0x8000))
		//{
		//	sabkpf("[saber]: enable - see the ghost - %u\r\n", (unsigned int)sSaberResultCount);

		//	if ((psh.session == 0) && (psh.length == 0))
		//	{
		//		return STD_NODATA;
		//	}
		//	continue;
		//}
		switch (psh.session & 0x7fff)
		{
		case SESSION_NAME_RAW_ACC:
		{
			int16_t x[3];
			memcpy((uint8_t *)&x[0], &data[3], 6);
			ouput->Acc[0] = (float)x[0];
			ouput->Acc[1] = (float)x[1];
			ouput->Acc[2] = (float)x[2];
		}
		break;
		case SESSION_NAME_RAW_GYRO:
		{
			int16_t x[3];
			memcpy((uint8_t *)&x[0], &data[3], 6);
			ouput->Gyro[0] = (float)x[0];
			ouput->Gyro[1] = (float)x[1];
			ouput->Gyro[2] = (float)x[2];
		}
		break;
		case SESSION_NAME_RAW_MAG:
		{
			int16_t x[3];
			memcpy((uint8_t *)&x[0], &data[3], 6);
			ouput->Mag[0] = (float)x[0];
			ouput->Mag[1] = (float)x[1];
			ouput->Mag[2] = (float)x[2];
		}
		break;
		case SESSION_NAME_QUAT:
		{

			memcpy((uint8_t *)ouput->q, &data[3], 16);
		}
		break;
		case SESSION_NAME_EULER:
		{
			memcpy((uint8_t *)ouput->e, &data[3], 12);
		}
		break;
		case SESSION_NAME_CAL_ACC:
		{
			memcpy((uint8_t *)ouput->Acc, &data[3], 12);
		}
		break;
		case SESSION_NAME_CAL_GYRO:
		{
			memcpy((uint8_t *)ouput->Gyro, &data[3], 12);
		}
		break;
		case SESSION_NAME_CAL_MAG:
		{
			memcpy((uint8_t *)ouput->Mag, &data[3], 12);
		}
		break;
		case SESSION_NAME_KAL_ACC:
		{
			memcpy((uint8_t *)ouput->Acc, &data[3], 12);
		}
		break;
		case SESSION_NAME_KAL_GYRO:
		{
			memcpy((uint8_t *)ouput->Gyro, &data[3], 12);
		}
		break;
		case SESSION_NAME_KAL_MAG:
		{
			memcpy((uint8_t *)ouput->Mag, &data[3], 12);
		}
		break;
		case SESSION_NAME_TEMPERATURE:
		{
			memcpy((uint8_t *)&ouput->Temperature, &data[3], 4);
			// sabkpf("t=%u\r\n", psh.length);
		}
		break;
		default:
		{
			sabkpf("[saber]: session - see the ghost 0x%x\r\n", psh.session);
		}
		break;
		}

		data += psh.length + sizeof(psh);
		l += psh.length + sizeof(psh);
		if (l >= data_size)
		{
			break;
		}
	} while (1);

	return STD_SUCCESS;
}

uint8_t saber_drv_isr_handle(void *handle)
{
	__GYRO_ACC_HANDLE_TypeDef *gah = (__GYRO_ACC_HANDLE_TypeDef *)handle;

	if (gah->hw_drv->get_ready())
	{

		if (sSaberOpCode == SAVER_OPCODE_WAIT_FOR_RSP)
		{
			// sabkpf("here\r\n");
			UTIL_SEQ_SetEvt(1 << CFG_IDLEEVT_SABER_WAIT_ID);
		}
		return sSaberOpCode;
	}
	else
	{
		return SAVER_OPCODE_NULL;
	}
}

uint8_t saber_drv_test(void *handle)
{

	return STD_SUCCESS;
}

#pragma endregion

/*******************************************************************************
END
*******************************************************************************/
