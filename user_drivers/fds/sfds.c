/*******************************************************************************
* file     sfds.c
* author   mackgim
* version  V1.0.0
* date
* brief ： simple flash data storage
*******************************************************************************/
#include "sfds.h"
#include "standard_lib.h"
#include "platform.h"


#ifdef DEBUG
#if 1
#define fwkpf(...) kprint(__VA_ARGS__)
#define fwnpf(...) nprint(__VA_ARGS__)
#else
#define fwkpf(...)
#define fwnpf(...)
#endif
#else
#define fwkpf(...)
#define fwnpf(...)
#endif


#pragma region 功能
static __SFDS_HANDLE_TypeDef	   sSfdsHd;

static __SFDS_PAGE_TypeDef           sPages[FLASH_USER_DATA_PAGES];
static __SFDS_SWAP_PAGE_TypeDef      sSwapPage;
static uint32_t sLastRecordID = 0;


static bool header_has_next(__SFDS_HEADER_TypeDef const* p_hdr, uint32_t const* p_page_end)
{
	uint32_t const* const p_hdr32 = (uint32_t*)p_hdr;
	return ((p_hdr32 < p_page_end)
		&& (*p_hdr32 != FDS_ERASED_WORD));  // Check last to be on the safe side (dereference)
}

// Jump to the next header.
static __SFDS_HEADER_TypeDef const* header_jump(__SFDS_HEADER_TypeDef const* const p_hdr)
{
	return (__SFDS_HEADER_TypeDef*)((uint32_t*)p_hdr + FDS_HEADER_SIZE + p_hdr->DataLen);
}

static __SFDS_HEADER_STATUS_TypeDef header_check(__SFDS_HEADER_TypeDef const* p_hdr, uint32_t const* p_page_end)
{
	if (((uint32_t*)header_jump(p_hdr) > p_page_end))
	{
		// The length field would jump across the page boundary.
		// FDS won't allow writing such a header, therefore it has been corrupted.
		return FDS_HEADER_CORRUPT;
	}

	// It is important to also check for the record ID to be non-erased.
	// It might happen that during GC, when records are copied in one operation,
	// the device powers off after writing the first two words of the header.
	// In that case the record would be considered valid, but its ID would
	// corrupt the file system.
	if ((p_hdr->FileID == FDS_FILE_ID_INVALID)
		|| (p_hdr->RecordKey == FDS_RECORD_KEY_DIRTY)
		|| (p_hdr->RecordID == FDS_ERASED_WORD)
		|| (p_hdr->GcTag != FDS_ERASED_DOUBLE_WORD))
	{
		return FDS_HEADER_DIRTY;
	}

	return FDS_HEADER_VALID;
}


static __SFDS_PAGE_TYPE_TypeDef page_identify(uint32_t const* const p_page_addr, uint32_t len)
{
	//fwkpf("addr=0x%x\r\n", (unsigned int)p_page_addr);
	//Clock_Wait(100);
	//fwkpf("addr1=0x%x, addr2=0x%x\r\n", (unsigned int)&p_page_addr[FDS_PAGE_TAG_WORD_0], (unsigned int)&p_page_addr[FDS_PAGE_TAG_WORD_1]);
	if ((p_page_addr[FDS_PAGE_TAG_WORD_0] == FDS_PAGE_TAG_MAGIC) && (p_page_addr[FDS_PAGE_TAG_WORD_1] == FDS_PAGE_TAG_DATA))
	{
		return FDS_PAGE_DATA;
	}
	else
	{
		for (uint32_t i = 0; i < (len); i++)
		{
			//swap page不全是0xffffffff,flash空间被污染
			if (p_page_addr[i] != FDS_ERASED_WORD)
			{
				return FDS_PAGE_DIRTY;
			}
		}
		return FDS_PAGE_SWAP;
	}
}


static uint8_t pages_are_standardized(void)
{
	uint8_t pages = 0, swap_pages = 0, dirty_pages = 0;
	for (uint8_t i = 0; i < FLASH_USER_PHY_PAGES; i++)
	{
		uint32_t const* const p_page_addr = (uint32_t*)sSfdsHd.StartAddr + (i * FDS_PAGE_SIZE);
		__SFDS_PAGE_TYPE_TypeDef type = page_identify(p_page_addr, FDS_PAGE_SIZE);
		switch (type)
		{
		case FDS_PAGE_DATA:
		{
			pages++;
		}
		break;
		case FDS_PAGE_SWAP:
		{
			swap_pages++;
		}
		break;
		default:
		{
			dirty_pages++;
		}
		break;
		}
	}

	if ((swap_pages == 1) && (pages == FLASH_USER_DATA_PAGES))
	{
		//是标准化的数据flash空间
		return STD_SUCCESS;
	}

	fwkpf("it is not a standardized space\r\n");
	return STD_FAILED;
}


// Tags a page as data, i.e, ready for storage.
static uint8_t page_tag_write_data(uint32_t* const p_page_addr)
{
	// The tag needs to be statically allocated since it is not buffered by fstorage.
	static uint32_t const page_tag_data[] = { FDS_PAGE_TAG_MAGIC, FDS_PAGE_TAG_DATA };
	return sSfdsHd.api->write((uint32_t)p_page_addr, page_tag_data, FDS_PAGE_TAG_SIZE * sizeof(uint32_t));
}


static void page_scan(uint32_t const* p_addr, uint32_t* const words_written)
{
	uint32_t const* const p_page_end = p_addr + FDS_PAGE_SIZE;

	p_addr += FDS_PAGE_TAG_SIZE;
	*words_written = FDS_PAGE_TAG_SIZE;

	__SFDS_HEADER_TypeDef const* p_header = (__SFDS_HEADER_TypeDef*)p_addr;

	while (header_has_next(p_header, p_page_end))
	{
		__SFDS_HEADER_STATUS_TypeDef hdr = header_check(p_header, p_page_end);

		if (hdr == FDS_HEADER_VALID)
		{
			// Update the latest (largest) record ID.
			if (p_header->RecordID > sLastRecordID)
			{
				sLastRecordID = p_header->RecordID;
			}
		}
		else
		{

			if (hdr == FDS_HEADER_CORRUPT)
			{
				// It could happen that a record has a corrupt header which would set a
				// wrong offset for this page. In such cases, update this value to its maximum,
				// to ensure that no new records will be written to this page and to enable
				// correct statistics reporting by fds_stat().
				*words_written = FDS_PAGE_SIZE;

				// We can't continue to scan this page.
				return;
			}
		}

		*words_written += (FDS_HEADER_SIZE + p_header->DataLen);
		p_header = header_jump(p_header);
	}
}


static uint8_t pages_update(void)
{
	uint16_t page_count = 0;

	for (uint16_t i = 0; i < FLASH_USER_PHY_PAGES; i++)
	{
		uint32_t const* const p_page_addr = (uint32_t*)sSfdsHd.StartAddr + (i * FDS_PAGE_SIZE);
		__SFDS_PAGE_TYPE_TypeDef type = page_identify(p_page_addr, FDS_PAGE_SIZE);

		switch (type)
		{
		case FDS_PAGE_DATA:
		{
			sPages[page_count].Paddr = p_page_addr;
			page_scan(p_page_addr, &sPages[page_count].WriteOffset);

			fwkpf("page%u, start=0x%x, offset=0x%x, rest=%u\r\n", page_count,
				(unsigned int)p_page_addr, (unsigned int)sPages[page_count].WriteOffset, (unsigned int)(FDS_PAGE_SIZE - sPages[page_count].WriteOffset));
			page_count++;
		}
		break;
		case FDS_PAGE_SWAP:
		{
			sSwapPage.Paddr = p_page_addr;
			sSwapPage.WriteOffset = 0;
		}
		break;
		default:
		{
			fwkpf("shouldn't happen\r\n");
		}
		return STD_FAILED;
		}
	}

	return STD_SUCCESS;
}

static uint8_t page_init(void)
{
	if (pages_are_standardized() != STD_SUCCESS)
	{
		uint8_t ret = sSfdsHd.api->erase(sSfdsHd.StartAddr, sSfdsHd.EndAddr);
		if (ret != STD_SUCCESS)
		{
			return ret;
		}

		//更新数据page的magic word
		for (uint8_t i = 0; i < FLASH_USER_DATA_PAGES; i++)
		{
			uint32_t* const p_page_addr = (uint32_t*)sSfdsHd.StartAddr + (i * FDS_PAGE_SIZE);
			uint8_t ret = page_tag_write_data(p_page_addr);
			if (ret != STD_SUCCESS)
			{
				return ret;
			}
		}
	}

	return pages_update();

}



#pragma region 查找

static bool record_find_next(uint32_t const* const p_page_addr, uint32_t const** p_record)
{
	uint32_t const* p_page_end = (p_page_addr + FDS_PAGE_SIZE);


	// If this is the first call on this page, start searching from its beginning.
// Otherwise, jump to the next record.
	__SFDS_HEADER_TypeDef const* p_header = (__SFDS_HEADER_TypeDef*)(*p_record);

	if (p_header != NULL)
	{
		p_header = header_jump(p_header);
	}
	else
	{
		p_header = (__SFDS_HEADER_TypeDef*)(p_page_addr + FDS_PAGE_TAG_SIZE);
	}

	// Read records from the page until:
	// - a valid record is found or
	// - the last record on a page is found

	while (header_has_next(p_header, p_page_end))
	{
		//fwkpf("p_header=0x%x\r\n", (unsigned int)p_header);
		switch (header_check(p_header, p_page_end))
		{
		case FDS_HEADER_VALID:
			*p_record = (uint32_t*)p_header;
			return true;

		case FDS_HEADER_DIRTY:
			p_header = header_jump(p_header);
			break;

		case FDS_HEADER_CORRUPT:
			// We can't reliably jump over this record.
			// There is nothing more we can do on this page.
			return false;
		}
	}

	// No more valid records on this page.
	return false;
}

static uint8_t record_find(uint16_t const* p_file_id, uint16_t const* p_record_key, __SFDS_RECORD_DESC_TypeDef* p_desc)
{
	for (uint8_t i = 0; i < FLASH_USER_DATA_PAGES; i++)
	{
		uint32_t const* const p_page_addr = sPages[i].Paddr;
		uint32_t const* p_record = NULL;

		while (record_find_next(p_page_addr, &p_record))
		{
			__SFDS_HEADER_TypeDef const* p_header = (__SFDS_HEADER_TypeDef*)p_record;
			if ((p_file_id != NULL) &&
				(p_header->FileID != *p_file_id))
			{
				continue;
			}

			if ((p_record_key != NULL) &&
				(p_header->RecordKey != *p_record_key))
			{
				continue;
			}

			uint32_t crc = sSfdsHd.api->crc_calc((uint32_t*)p_header, FDS_HEADER_CRC_SIZE * sizeof(uint32_t), true);
			crc = sSfdsHd.api->crc_calc((uint32_t*)p_header + FDS_HEADER_SIZE, p_header->DataLen * sizeof(uint32_t), false);

			if (crc != p_header->CRC32)
			{
				fwkpf("key=0x%x, crc error, 0x%x-0x%x\r\n", p_header->RecordKey, (unsigned int)crc, (unsigned int)p_header->CRC32);
				continue;
			}
			// Record found; update the descriptor.
			p_desc->Pdata = (uint32_t*)p_header + FDS_HEADER_SIZE;
			p_desc->Pheader = (uint32_t*)p_header;
			
			//fwkpf("found:header=0x%x, data=0x%x, key=0x%x, len=0x%x\r\n", (unsigned int)p_desc->Pheader,
			//	(unsigned int)p_desc->Pdata, (unsigned int)p_header->RecordKey, (unsigned int)p_header->DataLen);

			return true;
		}

		p_record = NULL;
	}

	return false;
}


#pragma endregion


#pragma region 写流程函数

// NOTE: Must be called from within a critical section.
static bool page_has_space(uint16_t page, uint16_t length_words)
{
	length_words += sPages[page].WriteOffset;
	//length_words += sPages[page].words_reserved;
	return (length_words <= FDS_PAGE_SIZE);
}


// Reserve space on a page.
// NOTE: this function takes into the account the space required for the record header.
static uint8_t write_space_reserve(uint16_t length_words, uint16_t* p_page)
{
	uint16_t const total_len_words = length_words + FDS_HEADER_SIZE;

	if (total_len_words > FDS_PAGE_SIZE - FDS_PAGE_TAG_SIZE)
	{
		return STD_FAILED;
	}

	for (uint16_t page = 0; page < FLASH_USER_DATA_PAGES; page++)
	{
		if (page_has_space(page, total_len_words))
		{

			*p_page = page;

			//m_pages[page].words_reserved += total_len_words;
			return STD_SUCCESS;
		}
	}


	return STD_NO_SPACE;
}



static uint8_t record_header_write_file(__SFDS_HEADER_TypeDef const* header, uint32_t* const p_addr)
{
	return sSfdsHd.api->write((uint32_t)(p_addr + FDS_HEADER_FILE_OFFSET), &header->FileID, FDS_HEADER_FILE_SIZE * sizeof(uint32_t));
}

static uint8_t record_header_write_recordid(__SFDS_HEADER_TypeDef const* header, uint32_t* const p_addr)
{
	return sSfdsHd.api->write((uint32_t)(p_addr + FDS_HEADER_RECORDID_OFFSET), &header->RecordID, FDS_HEADER_RECORDID_SIZE * sizeof(uint32_t));
}

static uint8_t record_header_write_gctag(uint32_t* const p_addr)
{
	uint64_t gc_tag = FDS_GC_TAG;
	return sSfdsHd.api->write((uint32_t)(p_addr + FDS_HEADER_GCTAG_OFFSET), &gc_tag, sizeof(gc_tag));
}

static uint8_t record_data_write(uint32_t* const p_addr, uint32_t const* p_src, uint32_t len)
{
	return sSfdsHd.api->write((uint32_t)(p_addr + FDS_HEADER_SIZE), p_src, len * sizeof(uint32_t));
}


#pragma endregion


#pragma region 查找记录状态
// Retrieve statistics about dirty records on a page.
static void records_stat(uint16_t   page,
	uint32_t* p_valid_records,
	uint32_t* p_dirty_records,
	uint32_t* p_freeable_words,
	bool* p_corruption)
{
	__SFDS_HEADER_TypeDef const* p_header = (__SFDS_HEADER_TypeDef*)(sPages[page].Paddr + FDS_PAGE_TAG_SIZE);
	uint32_t     const* const p_page_end = (sPages[page].Paddr + FDS_PAGE_SIZE);

	while (header_has_next(p_header, p_page_end))
	{
		switch (header_check(p_header, p_page_end))
		{
		case FDS_HEADER_DIRTY:
			*p_dirty_records += 1;
			*p_freeable_words += FDS_HEADER_SIZE + p_header->DataLen;
			p_header = header_jump(p_header);
			break;

		case FDS_HEADER_VALID:
			*p_valid_records += 1;
			p_header = header_jump(p_header);
			break;

		case FDS_HEADER_CORRUPT:
		{
			*p_dirty_records += 1;
			*p_freeable_words += (p_page_end - (uint32_t*)p_header);
			*p_corruption = true;
			// We can't continue on this page.
			return;
		}

		default:
			break;
		}
	}
}

#pragma endregion


uint8_t sfds_init(__SFDS_API_TypeDef* api)
{
	sSfdsHd.StartAddr = FLASH_SPACE_USER_BEGIN_ADDR;
	sSfdsHd.EndAddr = FLASH_SPACE_USER_END_ADDR;
	sSfdsHd.api = api;

	uint8_t ret = page_init();
	if (ret != STD_SUCCESS)
	{
		fwkpf("failed to init page\r\n");
		return ret;
	}

	fwkpf("ok\r\n");
	return STD_SUCCESS;
}


uint8_t sfds_record_read(__SFDS_RECORD_TypeDef* rp)
{
	__SFDS_RECORD_DESC_TypeDef desc = { 0 };
	uint8_t ret = record_find(&rp->FileID, &rp->RecordKey, &desc);
	if (ret)
	{
		__SFDS_HEADER_TypeDef const* p_header = (__SFDS_HEADER_TypeDef*)desc.Pheader;
		rp->Data.Ptr = desc.Pdata;
		rp->Data.Lenth = p_header->DataLen * sizeof(uint32_t);
		return STD_SUCCESS;
	}

	return STD_FAILED;
}


uint8_t sfds_record_write(__SFDS_RECORD_TypeDef* rp)
{

	if ((rp->FileID == FDS_FILE_ID_INVALID) ||
		(rp->RecordKey == FDS_RECORD_KEY_DIRTY))
	{
		return STD_INVALID_ARG;
	}
	uint32_t length_words = rp->Data.Lenth / 4;
	uint16_t page = 0;
	uint8_t ret = write_space_reserve(length_words, &page);
	if (ret != STD_SUCCESS)
	{
		fwkpf("failed space, ret=0x%x\r\n", ret);
		return ret;
	}

	__SFDS_RECORD_DESC_TypeDef desc = { 0 };
	uint8_t old_exist = record_find(&rp->FileID, &rp->RecordKey, &desc);

	//构建数据
	__SFDS_HEADER_TypeDef write_header;
	write_header.FileID = rp->FileID;
	write_header.DataLen = length_words;
	write_header.RecordKey = rp->RecordKey;
	write_header.Reserved = 0x0;
	sLastRecordID++;
	if (sLastRecordID == FDS_ERASED_WORD)
	{
		sLastRecordID = 1;
	}
	write_header.RecordID = sLastRecordID;
	sSfdsHd.api->crc_calc((uint32_t*)&write_header, FDS_HEADER_CRC_SIZE * sizeof(uint32_t), true);
	write_header.CRC32 = sSfdsHd.api->crc_calc((uint32_t*)rp->Data.Ptr, rp->Data.Lenth, false);

	uint32_t* p_write_addr = (uint32_t*)(sPages[page].Paddr + sPages[page].WriteOffset);
	sPages[page].WriteOffset += length_words + FDS_HEADER_SIZE;
	//
	ret = record_header_write_file(&write_header, p_write_addr);
	if (ret != STD_SUCCESS)
	{
		fwkpf("failed to wrtie file\r\n");
		return ret;
	}

	ret = record_data_write(p_write_addr, rp->Data.Ptr, length_words);
	if (ret != STD_SUCCESS)
	{
		fwkpf("failed to wrtie data\r\n");
		return ret;
	}

	ret = record_header_write_recordid(&write_header, p_write_addr);
	if (ret != STD_SUCCESS)
	{
		fwkpf("failed to wrtie recordid\r\n");
		return ret;
	}

	if (old_exist)
	{
		ret = record_header_write_gctag((uint32_t* const)desc.Pheader);
		if (ret != STD_SUCCESS)
		{
			fwkpf("failed to wrtie gc tag\r\n");
			return ret;
		}
	}
	fwkpf("ok\r\n");
	return STD_SUCCESS;
}


uint8_t sfds_stat(__SFDS_STAT_TypeDef* const p_stat)
{
	uint16_t const words_in_page = FDS_PAGE_SIZE;
	// The largest number of free contiguous words on any page.
	uint16_t       contig_words = 0;

	uint32_t total_words = FLASH_USER_DATA_PAGES * FDS_PAGE_SIZE;
	if (p_stat == NULL)
	{
		return STD_INVALID_ARG;
	}

	memset(p_stat, 0x00, sizeof(__SFDS_STAT_TypeDef));

	p_stat->PagesAvailable = FLASH_USER_PHY_PAGES;

	for (uint16_t page = 0; page < FLASH_USER_DATA_PAGES; page++)
	{
		uint32_t const words_used = sPages[page].WriteOffset;

		if (page_identify(sPages[page].Paddr, FDS_PAGE_SIZE) == FDS_PAGE_DIRTY)
		{
			p_stat->PagesAvailable--;
		}

		p_stat->WordsUsed += words_used;

		contig_words = (words_in_page - words_used);
		if (contig_words > p_stat->LargestContig)
		{
			p_stat->LargestContig = contig_words;
		}

		records_stat(page,
			&p_stat->ValidRecords,
			&p_stat->DirtyRecords,
			&p_stat->FreeableWords,
			&p_stat->Corruption);
	}
	p_stat->Percent = p_stat->WordsUsed * 100 / total_words;


	return STD_SUCCESS;
}


uint8_t sfds_gc(void)
{
	//转移数据到swap空间
	for (uint8_t i = 0; i < FLASH_USER_DATA_PAGES; i++)
	{
		uint32_t const* const p_page_addr = sPages[i].Paddr;
		uint32_t const* p_record = NULL;

		while (record_find_next(p_page_addr, &p_record))
		{
			__SFDS_HEADER_TypeDef const* p_header = (__SFDS_HEADER_TypeDef*)p_record;

			uint32_t crc = sSfdsHd.api->crc_calc((uint32_t*)p_header, FDS_HEADER_CRC_SIZE * sizeof(uint32_t), true);
			crc = sSfdsHd.api->crc_calc((uint32_t*)p_header + FDS_HEADER_SIZE, p_header->DataLen * sizeof(uint32_t), false);

			if (crc != p_header->CRC32)
			{
				fwkpf("key=0x%x, crc error, 0x%x-0x%x\r\n", p_header->RecordKey, (unsigned int)crc, (unsigned int)p_header->CRC32);
				continue;
			}

			//fwkpf("found:header=0x%x, data=0x%x,key=0x%x, len=0x%x\r\n", (unsigned int)p_header,
			//	(unsigned int)((uint32_t*)p_header + FDS_HEADER_SIZE), (unsigned int)p_header->RecordKey, (unsigned int)p_header->DataLen);


			//写入swap page
			uint32_t* const p_swap_page_addr = (uint32_t* const)sSwapPage.Paddr;
			__SFDS_PAGE_TYPE_TypeDef type = page_identify(p_swap_page_addr, FDS_PAGE_SIZE);

			if (type == FDS_PAGE_DATA)
			{

			}
			else if (type == FDS_PAGE_SWAP)
			{
				page_tag_write_data(p_swap_page_addr);
				sSwapPage.WriteOffset += FDS_PAGE_TAG_SIZE;
			}
			else
			{
				sSfdsHd.api->erase((uint32_t)p_swap_page_addr, (uint32_t)(p_swap_page_addr + FDS_PAGE_SIZE));
				page_tag_write_data(p_swap_page_addr);
				sSwapPage.WriteOffset += FDS_PAGE_TAG_SIZE;
			}

			uint32_t length_words = p_header->DataLen;
			uint32_t* p_write_addr = (uint32_t*)(p_swap_page_addr + sSwapPage.WriteOffset);
			sSwapPage.WriteOffset += length_words + FDS_HEADER_SIZE;
			//
			uint8_t ret = record_header_write_file(p_header, p_write_addr);
			if (ret != STD_SUCCESS)
			{
				fwkpf("failed to wrtie file\r\n");
				continue;
			}

			ret = record_data_write(p_write_addr, (uint32_t*)p_header + FDS_HEADER_SIZE, length_words);
			if (ret != STD_SUCCESS)
			{
				fwkpf("failed to wrtie data\r\n");
				continue;
			}

			ret = record_header_write_recordid(p_header, p_write_addr);
			if (ret != STD_SUCCESS)
			{
				fwkpf("failed to wrtie recordid\r\n");
				continue;
			}
			fwkpf("swap, write done, addr=0x%x, key=0x%x\r\n", (unsigned int)p_write_addr, (unsigned int)p_header->RecordKey);
		}
		p_record = NULL;
	}

	//转换分区
	uint32_t const* last_page_addr = sSwapPage.Paddr;
	uint32_t last_offset = sSwapPage.WriteOffset;
	for (uint8_t i = 0; i < FLASH_USER_DATA_PAGES; i++)
	{
		uint32_t const* const p_page_addr = sPages[i].Paddr;
		sSfdsHd.api->erase((uint32_t)p_page_addr, (uint32_t)(p_page_addr + FDS_PAGE_SIZE));
		sPages[i].WriteOffset = 0;


		uint32_t const* temp_page_addr = sPages[i].Paddr;
		uint32_t temp_offset = sPages[i].WriteOffset;

		sPages[i].Paddr = last_page_addr;
		sPages[i].WriteOffset = last_offset;


		last_page_addr = temp_page_addr;
		last_offset = temp_offset;
	}

	sSwapPage.Paddr = last_page_addr;
	sSwapPage.WriteOffset = last_offset;


	//生成data page
	for (uint8_t i = 0; i < FLASH_USER_DATA_PAGES; i++)
	{
		__SFDS_PAGE_TYPE_TypeDef type = page_identify(sPages[i].Paddr, FDS_PAGE_SIZE);

		switch (type)
		{
		case FDS_PAGE_DATA:
			break;
		case FDS_PAGE_SWAP:
		{
			uint32_t* const p_page_addr = (uint32_t* const)sPages[i].Paddr;
			page_tag_write_data(p_page_addr);
			sPages[i].WriteOffset += FDS_PAGE_TAG_SIZE;
		}
		break;
		default:
		{
			uint32_t* const p_page_addr = (uint32_t* const)sPages[i].Paddr;
			sSfdsHd.api->erase((uint32_t)p_page_addr, (uint32_t)(p_page_addr + FDS_PAGE_SIZE));
			sPages[i].WriteOffset = 0;
			page_tag_write_data(p_page_addr);
			sPages[i].WriteOffset += FDS_PAGE_TAG_SIZE;
		}
		break;
		}
	}

#ifdef DEBUG

	for (uint8_t i = 0; i < FLASH_USER_DATA_PAGES; i++)
	{
		fwkpf("data page: addr=0x%x, offset=0x%x\r\n", (unsigned int)sPages[i].Paddr, (unsigned int)sPages[i].WriteOffset);
	}
	fwkpf("swap page: addr=0x%x, offset=0x%x\r\n", (unsigned int)sSwapPage.Paddr, (unsigned int)sSwapPage.WriteOffset);

#endif



	return STD_SUCCESS;
}

#pragma endregion



/*******************************************************************************
END
*******************************************************************************/


