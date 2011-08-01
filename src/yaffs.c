/*
 * Flickernoise
 * Copyright (C) 2010, 2011 Sebastien Bourdeauducq
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <assert.h>
#include <rtems.h>
#include <rtems/libio.h>
#include <rtems/seterr.h>
#include <rtems/userenv.h>
#include <bsp/milkymist_flash.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <yaffs/rtems_yaffs.h>
#include <yaffs/yaffs_packedtags2.h>

#include "yaffs.h"

#define MAXIMUM_YAFFS_MOUNTS 2

struct yaffs_softc {
	struct yaffs_dev *dev;
	dev_t flashdev;
	unsigned int size;
	unsigned int blocksize;
	rtems_yaffs_os_handler free_os_context;
};

/* WARNING: Those I/O helpers:
 * - do not open or close the device
 * - set iop to NULL
 * The device driver must be able to support this.
 */

static rtems_status_code my_ioctl(dev_t dev, int request, void *p)
{
	rtems_libio_ioctl_args_t args;

	args.iop = NULL;
	args.command = request;
	args.buffer = p;
	return rtems_io_control(rtems_filesystem_dev_major_t(dev), rtems_filesystem_dev_minor_t(dev), &args);
}

static rtems_status_code my_read(dev_t dev, void *buffer, int len, int offset)
{
	rtems_libio_rw_args_t args;
	
	args.iop = NULL;
	args.offset = offset;
	args.buffer = buffer;
	args.count = len;
	args.flags = 0;
	return rtems_io_read(rtems_filesystem_dev_major_t(dev), rtems_filesystem_dev_minor_t(dev), &args);
}

static rtems_status_code my_write(dev_t dev, const void *buffer, int len, int offset)
{
	rtems_libio_rw_args_t args;

	args.iop = NULL;
	args.offset = offset;
	args.buffer = (void *)buffer;
	args.count = len;
	args.flags = 0;
	return rtems_io_write(rtems_filesystem_dev_major_t(dev), rtems_filesystem_dev_minor_t(dev), &args);
}

/* Flash access functions */

#define NOR_CHUNK_DATA_SIZE 512
#define NOR_CHUNK_TAGS_SIZE 16
#define NOR_CHUNK_WHOLE_SIZE (NOR_CHUNK_DATA_SIZE+NOR_CHUNK_TAGS_SIZE)

static unsigned int chunk_address(struct yaffs_dev *dev, int c)
{
	struct yaffs_softc *sc = (struct yaffs_softc *)dev->driver_context;
	unsigned int chunks_per_block = dev->param.chunks_per_block;
	return sc->blocksize*(c/chunks_per_block)
		+ NOR_CHUNK_WHOLE_SIZE*(c%chunks_per_block);
}

static int write_chunk_tags(struct yaffs_dev *dev, int nand_chunk, const u8 *data, const struct yaffs_ext_tags *tags)
{
	struct yaffs_softc *sc = (struct yaffs_softc *)dev->driver_context;
	unsigned int address;
	
	//printf("%s %d (data=%p tags=%p)\n", __func__, nand_chunk, data, tags);
	address = chunk_address(dev, nand_chunk);
	if(data)
		my_write(sc->flashdev, data, NOR_CHUNK_DATA_SIZE, address);
	if(tags) {
		struct yaffs_packed_tags2_tags_only x;
		yaffs_pack_tags2_tags_only(&x, tags);
		my_write(sc->flashdev, &x, NOR_CHUNK_TAGS_SIZE, address+NOR_CHUNK_DATA_SIZE);
	}
	return YAFFS_OK;
}

static int read_chunk_tags(struct yaffs_dev *dev, int nand_chunk, u8 *data, struct yaffs_ext_tags *tags)
{
	struct yaffs_softc *sc = (struct yaffs_softc *)dev->driver_context;
	unsigned int address;
	
	//printf("%s %d (data=%p tags=%p)\n", __func__, nand_chunk, data, tags);
	address = chunk_address(dev, nand_chunk);
	if(data)
		my_read(sc->flashdev, data, NOR_CHUNK_DATA_SIZE, address);
	if(tags) {
		struct yaffs_packed_tags2_tags_only x;
		my_read(sc->flashdev, &x, NOR_CHUNK_TAGS_SIZE, address+NOR_CHUNK_DATA_SIZE);
		yaffs_unpack_tags2_tags_only(tags, &x);
	}
	return YAFFS_OK;
}

static int bad_block(struct yaffs_dev *dev, int blockId)
{
	struct yaffs_ext_tags tags;
	int chunk_nr;

	chunk_nr = blockId * dev->param.chunks_per_block;

	read_chunk_tags(dev, chunk_nr, NULL, &tags);
	tags.block_bad = 1;
	write_chunk_tags(dev, chunk_nr, NULL, &tags);
	
	return YAFFS_OK;
}

static int query_block(struct yaffs_dev *dev, int blockId, enum yaffs_block_state *state, u32 *seq_number)
{
	struct yaffs_ext_tags tags;
	int chunk_nr;

	*seq_number = 0;

	chunk_nr = blockId * dev->param.chunks_per_block;

	read_chunk_tags(dev, chunk_nr, NULL, &tags);
	if(tags.block_bad)
		*state = YAFFS_BLOCK_STATE_DEAD;
	else if(!tags.chunk_used)
		*state = YAFFS_BLOCK_STATE_EMPTY;
	else if(tags.chunk_used) {
		*state = YAFFS_BLOCK_STATE_NEEDS_SCAN;
		*seq_number = tags.seq_number;
	}
	
	return YAFFS_OK;
}

static int erase(struct yaffs_dev *dev, int blockId)
{
	struct yaffs_softc *sc = dev->driver_context;

	my_ioctl(sc->flashdev, FLASH_ERASE_BLOCK, (void *)(blockId*sc->blocksize));
	
	return YAFFS_OK;
}

static int initialise(struct yaffs_dev *dev)
{
	return YAFFS_OK;
}

static int mount_sema_created;
static rtems_id mount_sema;
static struct yaffs_softc *current_mounts[MAXIMUM_YAFFS_MOUNTS];

static void unmount_handler(struct yaffs_dev *dev, void *os_context)
{
	struct yaffs_softc *softc = dev->driver_context;
	int i;
	
	rtems_semaphore_obtain(mount_sema, RTEMS_WAIT, RTEMS_NO_TIMEOUT);
	
	for(i=0;i<MAXIMUM_YAFFS_MOUNTS;i++) {
		if(current_mounts[i] == softc) {
			current_mounts[i] = NULL;
			break;
		}
	}
	
	yaffs_flush_whole_cache(dev);
	yaffs_deinitialise(dev);
	softc->free_os_context(dev, os_context);
	free(softc);
	free(dev);
	
	rtems_semaphore_release(mount_sema);
}

static int flush_task_running;
static rtems_id flush_task_id;

static rtems_task flush_task(rtems_task_argument argument)
{
	int i;
	struct yaffs_softc *sc;
	rtems_yaffs_default_os_context *os_context;
	
	while(1) {
		rtems_task_wake_after(10*100);
		rtems_semaphore_obtain(mount_sema, RTEMS_WAIT, RTEMS_NO_TIMEOUT);
		for(i=0;i<MAXIMUM_YAFFS_MOUNTS;i++) {
			sc = current_mounts[i];
			if(sc != NULL) {
				os_context = sc->dev->os_context;
				os_context->os_context.lock(sc->dev, os_context);
				yaffs_flush_whole_cache(sc->dev);
				os_context->os_context.unlock(sc->dev, os_context);
			}
		}
		rtems_semaphore_release(mount_sema);
	}
}

int yaffs_initialize(rtems_filesystem_mount_table_entry_t *mt_entry, const void *data)
{
	int i;
	int index;
	struct yaffs_dev *dev;
	struct yaffs_param *param;
	struct yaffs_softc *softc;
	rtems_yaffs_default_os_context *os_context;
	struct stat st;
	rtems_yaffs_mount_data md;
	rtems_status_code sc1, sc2;
	int r;
	
	if(!mount_sema_created) {
		sc1 = rtems_semaphore_create(
			rtems_build_name('Y', 'A', 'F', 'M'),
			1,
			RTEMS_LOCAL
				| RTEMS_BINARY_SEMAPHORE
				| RTEMS_INHERIT_PRIORITY
				| RTEMS_PRIORITY,
			0,
			&mount_sema
		);
		if(sc1 != RTEMS_SUCCESSFUL) {
			errno = ENOMEM;
			return -1;
		}
		mount_sema_created = 1;
	}
	
	rtems_semaphore_obtain(mount_sema, RTEMS_WAIT, RTEMS_NO_TIMEOUT);
	index = -1;
	for(i=0;i<MAXIMUM_YAFFS_MOUNTS;i++) {
		if(current_mounts[i] == NULL) {
			index = i;
			break;
		}
	}
	rtems_semaphore_release(mount_sema);
	if(index == -1) {
		errno = ENOMEM;
		return -1;
	}
	
	dev = malloc(sizeof(struct yaffs_dev));
	if(dev == NULL) {
		errno = ENOMEM;
		return -1;
	}
	memset(dev, 0, sizeof(struct yaffs_dev));
	
	softc = malloc(sizeof(struct yaffs_softc));
	if(softc == NULL) {
		errno = ENOMEM;
		free(dev);
		return -1;
	}
	softc->dev = dev;
	
	if(stat(mt_entry->dev, &st) < 0) {
		errno = EIO;
		free(softc);
		free(dev);
		return -1;
	}
	softc->flashdev = st.st_rdev;
	sc1 = my_ioctl(softc->flashdev, FLASH_GET_SIZE, &softc->size);
	sc2 = my_ioctl(softc->flashdev, FLASH_GET_BLOCKSIZE, &softc->blocksize);
	if((sc1 != RTEMS_SUCCESSFUL)||(sc2 != RTEMS_SUCCESSFUL)) {
		errno = EIO;
		free(softc);
		free(dev);
		return -1;
	}
	
	os_context = malloc(sizeof(rtems_yaffs_default_os_context));
	if(os_context == NULL) {
		errno = ENOMEM;
		free(softc);
		free(dev);
		return -1;
	}
	r = rtems_yaffs_initialize_default_os_context(os_context);
	if(r == -1) {
		free(os_context);
		free(softc);
		free(dev);
		return -1;
	}
	softc->free_os_context = os_context->os_context.unmount;
	os_context->os_context.unmount = unmount_handler;
	
	/* set parameters */
	dev->driver_context = softc;
	dev->os_context = os_context;
	dev->read_only = 0;

	param = &(dev->param);
	param->name = mt_entry->dev;
	
	param->start_block = 0;
	param->end_block = softc->size/softc->blocksize - 1;
	param->chunks_per_block = softc->blocksize/NOR_CHUNK_WHOLE_SIZE;
	param->total_bytes_per_chunk = NOR_CHUNK_WHOLE_SIZE;
	param->n_reserved_blocks = 5;
	param->n_caches = 15;
	param->inband_tags = 1;
	param->is_yaffs2 = 1;
	param->no_tags_ecc = 1;
	
	/* set callbacks */
	param->write_chunk_tags_fn = write_chunk_tags;
	param->read_chunk_tags_fn = read_chunk_tags;
	param->bad_block_fn = bad_block;
	param->query_block_fn = query_block;
	param->erase_fn = erase;
	param->initialise_flash_fn = initialise;
	
	md.dev = dev;
	r = rtems_yaffs_mount_handler(mt_entry, &md);
	if(r == -1) {
		errno = ENOMEM;
		softc->free_os_context(dev, os_context);
		free(softc);
		free(dev);
		return -1;
	}
	
	current_mounts[index] = softc;
	if(!flush_task_running) {
		sc1 = rtems_task_create(rtems_build_name('F', 'L', 'S', 'H'), 220, 256*1024,
			RTEMS_PREEMPT | RTEMS_NO_TIMESLICE | RTEMS_NO_ASR,
			0, &flush_task_id);
		assert(sc1 == RTEMS_SUCCESSFUL);
		sc1 = rtems_task_start(flush_task_id, flush_task, 0);
		assert(sc1 == RTEMS_SUCCESSFUL);
		flush_task_running = 1;
	}
	
	return 0;
}
