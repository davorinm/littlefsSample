/*
 * Copyright (c) 2019 Peter Bigot Consulting, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Sample which uses the filesystem API with littlefs */

#include <stdio.h>

#include <zephyr.h>
#include <device.h>
#include <fs/fs.h>
#include <fs/littlefs.h>
#include <storage/flash_map.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(dataStorage);

/* Matches LFS_NAME_MAX */
#define MAX_PATH_LEN		255
#define MOUNT_POINT			"/lfs"
#define PROC_DATA			"/procData"
#define PROC_DATA_FILE		MOUNT_POINT PROC_DATA

FS_LITTLEFS_DECLARE_DEFAULT_CONFIG(storage);
static struct fs_mount_t lfs_storage_mnt = {
	.type = FS_LITTLEFS,
	.fs_data = &storage,
	.storage_dev = (void *)FLASH_AREA_ID(storage),
	.mnt_point = MOUNT_POINT,
};
struct fs_mount_t *mp = &lfs_storage_mnt;

#define DATA_SIMULATE_DELAY 100
static struct k_work_delayable data_simulate_timer;

uint32_t data_file_index = 0;

typedef struct proc_data_t {
	int16_t a;
	int16_t b;
	int16_t c;
	int16_t d;
	int16_t e;
	int16_t f;
	int16_t g;
	int16_t h;
	int16_t i;
	int8_t j;
	int8_t k;
	uint32_t l;
	uint64_t m;
	uint8_t n[6];
	uint8_t o;
} proc_data_t;

static int writeData(proc_data_t data) {
	int rc;
	int rc2;
	struct fs_file_t file;
	const char * fname = PROC_DATA_FILE;

	fs_file_t_init(&file);

	rc = fs_open(&file, fname, FS_O_CREATE | FS_O_RDWR);
	if (rc < 0) {
		LOG_ERR("Open data file error: %d", rc);
		return rc;
	}

	// Seek to file pointer
	rc = fs_seek(&file, data_file_index, FS_SEEK_SET); 
	if (rc < 0) {
		LOG_ERR("Seek data file to pointer error: %d, %d", data_file_index, rc);
		goto close;
	}

	rc = fs_write(&file, &data, sizeof(data));
	LOG_DBG("write new data to: %d, ret: %d", data_file_index, rc);

close:
	rc2 = fs_close(&file);
	if (rc2 < 0) {
		LOG_ERR("File close error: %d", rc2);
		return rc2;
	}

	return rc;
}

static void dataStorage_addData(proc_data_t data) {
	int rc;

	rc = writeData(data);
	if (rc == -ENOSPC) {
		LOG_ERR("data write error, no more space left: %d", rc);

		// Set file pointer to begining
		data_file_index = 0;

		// Write data again
		rc = writeData(data);
		if(rc < 0) {
			LOG_ERR("data write #2 error: %d", rc);
		} else {
			LOG_DBG("data write #2 sucess: %d", rc);

			// Advance file pointer
			data_file_index += rc;
		}
	} else if(rc < 0) {
		LOG_ERR("data write error: %d", rc);
	} else {
		// Advance file pointer
		data_file_index += rc;
	}
}

static void data_simulate(struct k_work *work) {
	proc_data_t data = {
		.a = 1,
		.b = 2,
		.c = 3,
		.d = 4,
		.e = 5,
		.f = 6,
		.g = 7,
		.h = 8,
		.i = 9,
		.j = 10,
		.k = 11,
	};
	
	dataStorage_addData(data);

	k_work_reschedule(&data_simulate_timer, K_MSEC(DATA_SIMULATE_DELAY));
}

static int dataStorageInit(bool wipeFlash) {
	int rc;

	// Mount file system
	rc = fs_mount(mp);
	if (rc < 0) {
		LOG_ERR("FAIL: mount id %" PRIuPTR " at %s: %d", (uintptr_t)mp->storage_dev, mp->mnt_point, rc);
		return -1;
	}
	LOG_DBG("%s mount: %d", mp->mnt_point, rc);

	return 0;
}

void main(void) {
	dataStorageInit(false);	

	k_work_init_delayable(&data_simulate_timer, data_simulate);
	k_work_reschedule(&data_simulate_timer, K_MSEC(DATA_SIMULATE_DELAY));
}
