/*******************************************************************************
* file     sfds.c
* author   mackgim
* version  V1.0.0
* date
* brief ： simple flash data storage
*******************************************************************************/


#ifndef SFDS_H__
#define SFDS_H__


#include <stdint.h>
#include <stdbool.h>


#ifdef __cplusplus
extern "C" {
#endif


#define FDS_PAGE_SIZE FLASH_USER_PAGE_SIZE


#define FDS_RECORD_KEY_DIRTY		(0x0000)
#define FDS_FILE_ID_INVALID			(0xFFFF)
#define FDS_ERASED_WORD				(0xFFFFFFFF)
#define FDS_GC_TAG					(0xf42e8723ac53d034)
#define FDS_ERASED_DOUBLE_WORD		(0xFFFFFFFFFFFFFFFF)

#define	FDS_HEADER_FILE_OFFSET		(0)
#define	FDS_HEADER_RECORDID_OFFSET	(2)
#define	FDS_HEADER_GCTAG_OFFSET		(4)


#define	FDS_HEADER_FILE_SIZE		(2)
#define	FDS_HEADER_RECORDID_SIZE	(2)
#define	FDS_HEADER_GCTAG_SIZE		(2)
#define FDS_HEADER_CRC_SIZE			(3)
#define FDS_HEADER_SIZE				(6) // Size of the whole header, in 4-byte words.



	// Page types.
#define FDS_PAGE_TAG_SIZE       (2) // Page tag size, in 4-byte words.
#define FDS_PAGE_TAG_WORD_0     (0) // Offset of the first word in the page tag from the page address.
#define FDS_PAGE_TAG_WORD_1     (1) // Offset of the second word in the page tag from the page address.	

// Page tag constants
#define FDS_PAGE_TAG_MAGIC      (0xDEADC0DE)
#define FDS_PAGE_TAG_SWAP       (0xF11E01FF)
#define FDS_PAGE_TAG_DATA       (0xF11E01FE)


	typedef struct
	{
		/**@brief Write bytes to flash. */
		uint8_t(*write)(uint32_t dest, void const* p_src, uint32_t len);
		/**@brief Erase flash pages. */
		uint8_t(*erase)(uint32_t start_addr, uint32_t end_addr);

		uint32_t(*crc_calc)(uint32_t*, uint32_t, bool);
	} __SFDS_API_TypeDef;

	typedef struct
	{
		__SFDS_API_TypeDef* api;
		uint32_t StartAddr;
		uint32_t EndAddr;
	} __SFDS_HANDLE_TypeDef;


	/**@brief   The record metadata as stored in flash.
	 * @warning Do not edit or reorder the fields in this structure.
	 */
	typedef struct
	{
		uint16_t FileID;    //!< The ID of the file that the record belongs to.
		uint16_t DataLen;	//数据长度, in words
		uint16_t RecordKey;	//
		uint16_t Reserved;  //
		uint32_t RecordID;	//记录ID
		uint32_t CRC32;		//校验码
		uint64_t GcTag;	//是否垃圾标记

	} __SFDS_HEADER_TypeDef;

	typedef enum
	{
		FDS_HEADER_VALID,
		// Valid header.
		FDS_HEADER_DIRTY,
		// Header is incomplete, or record has been deleted.
		FDS_HEADER_CORRUPT  // Header contains corrupt information, not related to CRC.
	} __SFDS_HEADER_STATUS_TypeDef;

	typedef enum
	{
		FDS_PAGE_DATA,         // Page is ready for storage.
		FDS_PAGE_SWAP,         // Page is reserved for garbage collection.
		FDS_PAGE_DIRTY,			//page is dirty.
	} __SFDS_PAGE_TYPE_TypeDef;


	typedef struct
	{
		uint32_t				const* Paddr;		// The address of the page.
		uint32_t                WriteOffset;     // The page write offset, in 4-byte words.
	} __SFDS_PAGE_TypeDef;

	typedef struct
	{
		uint32_t				const* Paddr;		// The address of the page.
		uint32_t                WriteOffset;     // The page write offset, in 4-byte words.
	} __SFDS_SWAP_PAGE_TypeDef;



	typedef struct
	{
		uint32_t const* Pheader;          //!< The unique record ID.
		uint32_t const* Pdata;           //!< The last known location of the record in flash.
	} __SFDS_RECORD_DESC_TypeDef;


	typedef struct
	{
		uint16_t FileID;            //!< The ID of the file that the record belongs to.
		uint16_t RecordKey;                //!< The record key.
		struct
		{
			void     const* Ptr;
			uint32_t         Lenth;
		} Data;
	} __SFDS_RECORD_TypeDef;



	/**@brief   File system statistics. */
	typedef struct
	{
		uint16_t PagesAvailable;    //!< The number of pages available.
		uint16_t Percent;			//
		uint32_t ValidRecords;      //!< The number of valid records.
		uint32_t DirtyRecords;      //!< The number of deleted ("dirty") records.


		/**@brief The number of words written to flash, including those reserved for future writes. */
		uint32_t WordsUsed;

		/**@brief The largest number of free contiguous words in the file system.
		 *
		 * This number indicates the largest record that can be stored by FDS.
		 * It takes into account all reservations for future writes.
		 */
		uint16_t LargestContig;

		/**@brief The largest number of words that can be reclaimed by garbage collection.
		 *
		 * The actual amount of space freed by garbage collection might be less than this value if
		 * records are open while garbage collection is run.
		 */
		uint32_t FreeableWords;

		/**@brief Filesystem corruption has been detected.
		 *
		 * One or more corrupted records were detected. FDS will heal the filesystem automatically
		 * next time garbage collection is run, but some data may be lost.
		 *
		 * @note: This flag is unrelated to CRC failures.
		 */
		bool Corruption;
	} __SFDS_STAT_TypeDef;


	uint8_t sfds_init(__SFDS_API_TypeDef* api);
	uint8_t sfds_record_read(__SFDS_RECORD_TypeDef* rp);
	uint8_t sfds_record_write(__SFDS_RECORD_TypeDef* rp);
	uint8_t sfds_stat(__SFDS_STAT_TypeDef* const p_stat);
	uint8_t sfds_gc(void);
#ifdef __cplusplus
}
#endif

#endif // FDS_H__
