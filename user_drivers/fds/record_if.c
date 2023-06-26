/*******************************************************************************
* file    record_if.c
* author  mackgim
* version 1.0.0
* date
* brief   flash recorder 保存关键数据
*******************************************************************************/
//  Each page(4096,or,0x1000) is divided into 8 blocks(512,or,0x200)

#include "record_if.h"
#include "crc32.h"
#include "sfds.h"
#include "standard_lib.h"
#include "flash_driver.h"


/**@brief Macro for calling error handler function if supplied error code any other than NRF_SUCCESS.
 *
 * @param[in] ERR_CODE Error code supplied to the error handler.
 */

#define APP_ERROR_CHECK(ERR_CODE)                           \
    do                                                      \
    {                                                       \
        if (ERR_CODE != STD_SUCCESS)                  \
        {                                                   \
		    kprint("failed,line=%d\r\n", __LINE__);				\
            return STD_FAILED;								\
        }                                                   \
    } while (0)

 //122可以保存119的长度
#pragma region 函数

#pragma endregion

#pragma region 变量
/* Flag to check fds initialization. */
static bool volatile m_fds_initialized;
static bool volatile m_fds_gced;

#pragma endregion

#pragma region 功能
//产品信息初始化
uint8_t recorder_init(void)
{
	log_flush();

	static __SFDS_API_TypeDef api = {
		.write = flash_ll_write,
		.erase = flash_ll_erase,
		.crc_calc = crc32_accumulate,
	};

	uint8_t rc = sfds_init(&api);
	APP_ERROR_CHECK(rc);


	recorder_stat();

	kprint("ok\r\n\r\n");
	return STD_SUCCESS;
}

uint8_t recorder_erase_all(void)
{
	//flash_bounds_set();
	return STD_SUCCESS;
}
#pragma endregion


#pragma region 各种信息的保存

static uint16_t sRecordID[][2] = RECORDER_ID_SET;

//保存、读取
uint8_t recorder_save(uint8_t index, void* buff, uint32_t size)
{
	__SFDS_RECORD_TypeDef record =
	{
		.FileID = sRecordID[index][0],
		.RecordKey = sRecordID[index][1],
		.Data.Ptr = buff,
		/* The length of a record is always expressed in 4-byte units (words). */
		.Data.Lenth = (size),
	};
	uint8_t ret = STD_SUCCESS;

	do
	{

		ret = sfds_record_write(&record);

		if (ret == STD_NO_SPACE)
		{
			sfds_gc();
		}
		else
		{
			break;
		}
	} while (1);

#if DEBUG
	recorder_stat();
#endif
	return ret;
}

uint8_t recorder_read(uint8_t index, void* buff, uint32_t size)
{
	__SFDS_RECORD_TypeDef record =
	{
		.FileID = sRecordID[index][0],
		.RecordKey = sRecordID[index][1],
		.Data.Ptr = NULL,
		/* The length of a record is always expressed in 4-byte units (words). */
		.Data.Lenth = 0,
	};

	uint8_t ret = sfds_record_read(&record);
	if (ret == STD_SUCCESS)
	{

		if (buff != NULL)
		{
			uint32_t l = MIN(record.Data.Lenth, size);
			memcpy(buff, record.Data.Ptr, l);
#ifdef DEBUG
			const uint8_t sRecordName[][20] = {"product", "algo", "account", "time", "ble", "skt", };
			kprint("name=%s, id=0x%x, data addr=0x%x, size=%u\r\n", (char *)&sRecordName[index], (unsigned int)sRecordID[index][1], (unsigned int)record.Data.Ptr, (unsigned int)record.Data.Lenth);

#endif // DEBUG

			}
	}

	return ret;
}


#pragma endregion

#pragma region 其他功能



//返回可以操作的空间(uint32_t)
uint32_t recorder_stat(void)
{
	__SFDS_STAT_TypeDef stat = { 0 };

	uint8_t rc = sfds_stat(&stat);
	APP_ERROR_CHECK(rc);

	nprint("fds: %u pages, %u valid records, %u dirty records, %u largest_contig. %u%% percent used\r\n", (unsigned int)stat.PagesAvailable,
		(unsigned int)stat.ValidRecords, (unsigned int)stat.DirtyRecords, (unsigned int)stat.LargestContig, (unsigned int)stat.Percent);
	return stat.LargestContig;
}


uint8_t recorder_gc(void)
{
	return sfds_gc();
}
#pragma endregion