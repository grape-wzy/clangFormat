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
#include "fds.h"
#include "platform.h"
#include "standard_lib.h"



/**@brief Macro for calling error handler function if supplied error code any other than NRF_SUCCESS.
 *
 * @param[in] ERR_CODE Error code supplied to the error handler.
 */
#define APP_ERROR_CHECK(ERR_CODE)                           \
    do                                                      \
    {                                                       \
        const uint32_t LOCAL_ERR_CODE = (ERR_CODE);         \
        if (LOCAL_ERR_CODE != NRF_SUCCESS)                  \
        {                                                   \
		    kprint("failed,line=%d\r\n", __LINE__);				\
            return STD_FAILED;								\
        }                                                   \
    } while (0)

 //122可以保存119的长度
#pragma region 函数
static void recorder_evt_handler(fds_evt_t const* p_evt);
uint8_t recorder_gc(void);
uint32_t recorder_get_free_space(void);
uint8_t recorder_ll_save(fds_record_t* rp);
uint8_t recorder_ll_read(fds_record_t* rp, uint8_t* data, uint32_t size);
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
	ret_code_t rc;
	fds_register(recorder_evt_handler);

	rc = fds_init();
	APP_ERROR_CHECK(rc);

	while (!m_fds_initialized)
	{
		//power_manage();
	}

	recorder_get_free_space();

	kprint("ok\r\n\r\n");
	return STD_SUCCESS;
}

uint8_t recorder_erase_all(void)
{
	//flash_bounds_set();
	return STD_SUCCESS;
}
#pragma endregion

#pragma region 回调

static char const* fds_evt_str[] =
{
	"FDS_EVT_INIT",
	"FDS_EVT_WRITE",
	"FDS_EVT_UPDATE",
	"FDS_EVT_DEL_RECORD",
	"FDS_EVT_DEL_FILE",
	"FDS_EVT_GC",
};

const char* fds_err_str(ret_code_t ret)
{
	/* Array to map FDS return values to strings. */
	static char const* err_str[] =
	{
		"FDS_ERR_OPERATION_TIMEOUT",
		"FDS_ERR_NOT_INITIALIZED",
		"FDS_ERR_UNALIGNED_ADDR",
		"FDS_ERR_INVALID_ARG",
		"FDS_ERR_NULL_ARG",
		"FDS_ERR_NO_OPEN_RECORDS",
		"FDS_ERR_NO_SPACE_IN_FLASH",
		"FDS_ERR_NO_SPACE_IN_QUEUES",
		"FDS_ERR_RECORD_TOO_LARGE",
		"FDS_ERR_NOT_FOUND",
		"FDS_ERR_NO_PAGES",
		"FDS_ERR_USER_LIMIT_REACHED",
		"FDS_ERR_CRC_CHECK_FAILED",
		"FDS_ERR_BUSY",
		"FDS_ERR_INTERNAL",
	};

	return err_str[ret - NRF_ERROR_FDS_ERR_BASE];
}

static void recorder_evt_handler(fds_evt_t const* p_evt)
{
	if (p_evt->result == NRF_SUCCESS)
	{
		//kprint("Event: %s received (NRF_SUCCESS)\r\n",
		//	fds_evt_str[p_evt->id]);
	}
	else
	{
		kprint("Error, Event: %s received (%s)\r\n",
			fds_evt_str[p_evt->id],
			fds_err_str(p_evt->result));
	}

	switch (p_evt->id)
	{
	case FDS_EVT_INIT:
		if (p_evt->result == NRF_SUCCESS)
		{
			m_fds_initialized = true;
			kprint("init\r\n");
		}
		break;

	case FDS_EVT_WRITE:
	{
		if (p_evt->result == NRF_SUCCESS)
		{
			kprint("write complete, ");
			nprint("Record ID:0x%04x,", (unsigned int)p_evt->write.record_id);
			nprint("File ID:0x%04x,", (unsigned int)p_evt->write.file_id);
			nprint("Record key:0x%04x\r\n", (unsigned int)p_evt->write.record_key);
		}
	}
	break;

	case FDS_EVT_DEL_RECORD:
	{
		if (p_evt->result == NRF_SUCCESS)
		{
			kprint("delete complete, ");
			nprint("Record ID:0x%04x,", (unsigned int)p_evt->write.record_id);
			nprint("File ID:0x%04x,", (unsigned int)p_evt->write.file_id);
			nprint("Record key:0x%04x\r\n", (unsigned int)p_evt->write.record_key);
		}
		//		m_delete_all.pending = false;
	}
	break;
	case FDS_EVT_GC:
	{
		kprint("gc complete\r\n");
		m_fds_gced = true;
	}
	break;

	default:
		break;
	}
}

#pragma endregion

#pragma region 各种信息的保存

static uint16_t sRecordID[][2] = RECORDER_ID_SET;

//保存、读取
uint8_t recorder_save(uint8_t index, void* buff, uint32_t size)
{
	fds_record_t record =
	{
		.file_id = sRecordID[index][0],
		.key = sRecordID[index][1],
		.data.p_data = buff,
		/* The length of a record is always expressed in 4-byte units (words). */
		.data.length_words = (size / sizeof(uint32_t)),
	};
	return recorder_ll_save(&record);
}

uint8_t recorder_read(uint8_t index, void* buff, uint32_t size)
{
	fds_record_t record =
	{
		.file_id = sRecordID[index][0],
		.key = sRecordID[index][1],
		.data.p_data = NULL,
		/* The length of a record is always expressed in 4-byte units (words). */
		.data.length_words = 0,
	};
	return recorder_ll_read(&record, buff, size);
}


#pragma endregion

#pragma region 其他功能

uint8_t recorder_gc(void)
{
	m_fds_gced = false;
	ret_code_t ret = fds_gc();
	APP_ERROR_CHECK(ret);
	while (!m_fds_gced) {}
	kprint("OK\r\n");
	return STD_SUCCESS;
}

//返回可以操作的空间(uint32_t)
uint32_t recorder_get_free_space(void)
{
	fds_stat_t stat = { 0 };

	ret_code_t rc = fds_stat(&stat);
	APP_ERROR_CHECK(rc);

	nprint("flash have %d valid records, %d dirty records, %d largest_contig.\r\n", stat.valid_records, stat.dirty_records, stat.largest_contig);
	//kprint("%d dirty records.\r\n", stat.dirty_records);
	//kprint("%d words_reserved.\r\n", stat.words_reserved);
	//kprint("%d words_used.\r\n", stat.words_used);
	//kprint("%d largest_contig.\r\n", stat.largest_contig);
	//kprint("%d freeable_words.\r\n", stat.freeable_words);
	//kprint("ok\r\n");
	if (stat.largest_contig < 5)
	{
		return 0;
	}
	return stat.largest_contig - 4;
}

uint8_t recorder_ll_save(fds_record_t* rp)
{
	ret_code_t ret;

	//查询剩余空间
	uint32_t freespace = recorder_get_free_space();
	if (rp->data.length_words > freespace)
	{
		kprint("full\r\n");
		recorder_gc();
		recorder_get_free_space();
	}

	fds_record_desc_t desc = { 0 };
	fds_find_token_t tok = { 0 };

	ret = fds_record_find(rp->file_id, rp->key, &desc, &tok);
	if (ret == NRF_SUCCESS)
	{
		ret = fds_record_update(&desc, rp);
		if (ret != NRF_SUCCESS)
		{
			kprint("failed to update, 0x%x\r\n", (unsigned int)ret);
			return STD_FAILED;
		}
		kprint("ok to update\r\n");
	}
	else if (ret == FDS_ERR_NOT_FOUND)
	{

		ret = fds_record_write(&desc, rp);
		if (ret != NRF_SUCCESS)
		{
			kprint("failed to write, 0x%x\r\n", (unsigned int)ret);
			return STD_FAILED;
		}
		kprint("ok to write\r\n");
	}
	else
	{

		kprint("failed to find, 0x%x", (unsigned int)ret);
		return STD_FAILED;
	}

	return STD_SUCCESS;
}

uint8_t recorder_ll_read(fds_record_t* rp, uint8_t* data, uint32_t size)
{
	ret_code_t ret;

	fds_record_desc_t desc = { 0 };
	fds_find_token_t tok = { 0 };

	ret = fds_record_find(rp->file_id, rp->key, &desc, &tok);
	if (ret == NRF_SUCCESS)
	{
		fds_flash_record_t config = { 0 };
		ret = fds_record_open(&desc, &config);
		APP_ERROR_CHECK(ret);
		//header是12个字节
		if (data != NULL)
		{
			memcpy(data, config.p_data, size);
		}
		kprint("headerp=0x%x, datap=0x%x\r\n", (unsigned int)config.p_header, (unsigned int)config.p_data);
		kprint("record_key=0x%x, length_words=%u, file_id=0x%x,record_id=0x%x\r\n",
			(unsigned int)config.p_header->record_key,
			(unsigned int)config.p_header->length_words,
			(unsigned int)config.p_header->file_id,
			(unsigned int)config.p_header->record_id);
		ret = fds_record_close(&desc);
		APP_ERROR_CHECK(ret);
		kprint("ok\r\n");
		return STD_SUCCESS;
	}
	else
	{
		kprint("no data\r\n");
		return STD_FAILED;
	}


}
#pragma endregion