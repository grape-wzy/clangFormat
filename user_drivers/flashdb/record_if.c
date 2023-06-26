/*******************************************************************************
* file    record_if.c
* author  mackgim
* version 1.0.0
* date
* brief   flash recorder 保存关键数据
*******************************************************************************/


#include "record_if.h"
#include "crc32.h"
#include "platform.h"
#include "standard_lib.h"
#include <flashdb.h>


#pragma region 函数


#pragma endregion

#pragma region 变量
/* Flag to check fds initialization. */
static struct fdb_kvdb kvdb = { 0 };
static uint32_t boot_count = 0;
static time_t boot_time[10] = { 0, 1, 2, 3 };

#pragma endregion

#pragma region 功能


static struct fdb_default_kv_node default_kv_table[] = {
		{"username", "armink", 0}, /* string KV */
		{"password", "123456", 0}, /* string KV */
		{"boot_count", &boot_count, sizeof(boot_count)}, /* int type KV */
		{"boot_time", &boot_time, sizeof(boot_time)},    /* int array type KV */
};

void recorder_lock(void)
{

}

void recorder_unlock(void)
{

}

//产品信息初始化
uint8_t recorder_init(void)
{
	//struct fdb_default_kv default_kv;

	//default_kv.kvs = default_kv_table;
	//default_kv.num = sizeof(default_kv_table) / sizeof(default_kv_table[0]);
	/* set the lock and unlock function if you want */
	fdb_kvdb_control(&kvdb, FDB_KVDB_CTRL_SET_LOCK, (void*)recorder_lock);
	fdb_kvdb_control(&kvdb, FDB_KVDB_CTRL_SET_UNLOCK, (void*)recorder_unlock);
	/* Key-Value database initialization
	 *
	 *       &kvdb: database object
	 *       "env": database name
	 * "fdb_kvdb1": The flash partition name base on FAL. Please make sure it's in FAL partition table.
	 *              Please change to YOUR partition name.
	 * &default_kv: The default KV nodes. It will auto add to KVDB when first initialize successfully.
	 *        NULL: The user data if you need, now is empty.
	 */
	fdb_err_t result;
	//result = fdb_kvdb_init(&kvdb, "env", "fdb_kvdb1", &default_kv, NULL);

	result = fdb_kvdb_init(&kvdb, "env", "fdb_kvdb1", NULL, NULL);
	if (result != FDB_NO_ERR) {
		kprint("failed to init fdb\r\n");
		return STD_FAILED;
	}
	kprint("ok\r\n\r\n");
	return STD_SUCCESS;
}


uint8_t recorder_save(uint8_t index, const void* buff, uint32_t size)
{
	struct fdb_blob blob;
	fdb_err_t result = fdb_kv_set_blob(&kvdb, "temp", fdb_blob_make(&blob, buff, size));
	if (result != FDB_NO_ERR)
	{
		kprint("failed, ret=0x%x\r\n", result);
		return STD_FAILED;
	}
	//kprint("ok\r\n");
	return STD_SUCCESS;
}

uint8_t recorder_read(uint8_t index, void* buff, uint32_t size)
{
	struct fdb_blob blob;
	fdb_kv_get_blob(&kvdb, "temp", fdb_blob_make(&blob, buff, size));
	if (blob.saved.len <= 0)
	{
		kprint("failed, len=%d\r\n", blob.saved.len);
		return  STD_FAILED;
	}
	//kprint("ok\r\n");
	return STD_SUCCESS;
}
#pragma endregion
