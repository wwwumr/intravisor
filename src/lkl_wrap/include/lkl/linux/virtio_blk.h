#ifndef _LKL_LINUX_VIRTIO_BLK_H
#define _LKL_LINUX_VIRTIO_BLK_H
/* This header is BSD licensed so anyone can use the definitions to implement
 * compatible drivers/servers.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of IBM nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL IBM OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE. */
#include <lkl/linux/types.h>
#include <lkl/linux/virtio_ids.h>
#include <lkl/linux/virtio_config.h>
#include <lkl/linux/virtio_types.h>

/* Feature bits */
#define LKL_VIRTIO_BLK_F_SIZE_MAX	1	/* Indicates maximum segment size */
#define LKL_VIRTIO_BLK_F_SEG_MAX	2	/* Indicates maximum # of segments */
#define LKL_VIRTIO_BLK_F_GEOMETRY	4	/* Legacy geometry available  */
#define LKL_VIRTIO_BLK_F_RO		5	/* Disk is read-only */
#define LKL_VIRTIO_BLK_F_BLK_SIZE	6	/* Block size of disk is available*/
#define LKL_VIRTIO_BLK_F_TOPOLOGY	10	/* Topology information is available */
#define LKL_VIRTIO_BLK_F_MQ		12	/* support more than one vq */
#define LKL_VIRTIO_BLK_F_DISCARD	13	/* DISCARD is supported */
#define LKL_VIRTIO_BLK_F_WRITE_ZEROES	14	/* WRITE ZEROES is supported */
#define LKL_VIRTIO_BLK_F_SECURE_ERASE	16 /* Secure Erase is supported */

/* Legacy feature bits */
#ifndef VIRTIO_BLK_NO_LEGACY
#define LKL_VIRTIO_BLK_F_BARRIER	0	/* Does host support barriers? */
#define LKL_VIRTIO_BLK_F_SCSI	7	/* Supports scsi command passthru */
#define LKL_VIRTIO_BLK_F_FLUSH	9	/* Flush command supported */
#define LKL_VIRTIO_BLK_F_CONFIG_WCE	11	/* Writeback mode available in config */
/* Old (deprecated) name for LKL_VIRTIO_BLK_F_FLUSH. */
#define LKL_VIRTIO_BLK_F_WCE LKL_VIRTIO_BLK_F_FLUSH
#endif /* !VIRTIO_BLK_NO_LEGACY */

#define LKL_VIRTIO_BLK_ID_BYTES	20	/* ID string length */

struct lkl_virtio_blk_config {
	/* The capacity (in 512-byte sectors). */
	__lkl__virtio64 capacity;
	/* The maximum segment size (if LKL_VIRTIO_BLK_F_SIZE_MAX) */
	__lkl__virtio32 size_max;
	/* The maximum number of segments (if LKL_VIRTIO_BLK_F_SEG_MAX) */
	__lkl__virtio32 seg_max;
	/* geometry of the device (if LKL_VIRTIO_BLK_F_GEOMETRY) */
	struct lkl_virtio_blk_geometry {
		__lkl__virtio16 cylinders;
		__lkl__u8 heads;
		__lkl__u8 sectors;
	} geometry;

	/* block size of device (if LKL_VIRTIO_BLK_F_BLK_SIZE) */
	__lkl__virtio32 blk_size;

	/* the next 4 entries are guarded by LKL_VIRTIO_BLK_F_TOPOLOGY  */
	/* exponent for physical block per logical block. */
	__lkl__u8 physical_block_exp;
	/* alignment offset in logical blocks. */
	__lkl__u8 alignment_offset;
	/* minimum I/O size without performance penalty in logical blocks. */
	__lkl__virtio16 min_io_size;
	/* optimal sustained I/O size in logical blocks. */
	__lkl__virtio32 opt_io_size;

	/* writeback mode (if LKL_VIRTIO_BLK_F_CONFIG_WCE) */
	__lkl__u8 wce;
	__lkl__u8 unused;

	/* number of vqs, only available when LKL_VIRTIO_BLK_F_MQ is set */
	__lkl__virtio16 num_queues;

	/* the next 3 entries are guarded by LKL_VIRTIO_BLK_F_DISCARD */
	/*
	 * The maximum discard sectors (in 512-byte sectors) for
	 * one segment.
	 */
	__lkl__virtio32 max_discard_sectors;
	/*
	 * The maximum number of discard segments in a
	 * discard command.
	 */
	__lkl__virtio32 max_discard_seg;
	/* Discard commands must be aligned to this number of sectors. */
	__lkl__virtio32 discard_sector_alignment;

	/* the next 3 entries are guarded by LKL_VIRTIO_BLK_F_WRITE_ZEROES */
	/*
	 * The maximum number of write zeroes sectors (in 512-byte sectors) in
	 * one segment.
	 */
	__lkl__virtio32 max_write_zeroes_sectors;
	/*
	 * The maximum number of segments in a write zeroes
	 * command.
	 */
	__lkl__virtio32 max_write_zeroes_seg;
	/*
	 * Set if a LKL_VIRTIO_BLK_T_WRITE_ZEROES request may result in the
	 * deallocation of one or more of the sectors.
	 */
	__lkl__u8 write_zeroes_may_unmap;

	__lkl__u8 unused1[3];

	/* the next 3 entries are guarded by LKL_VIRTIO_BLK_F_SECURE_ERASE */
	/*
	 * The maximum secure erase sectors (in 512-byte sectors) for
	 * one segment.
	 */
	__lkl__virtio32 max_secure_erase_sectors;
	/*
	 * The maximum number of secure erase segments in a
	 * secure erase command.
	 */
	__lkl__virtio32 max_secure_erase_seg;
	/* Secure erase commands must be aligned to this number of sectors. */
	__lkl__virtio32 secure_erase_sector_alignment;

} __attribute__((packed));

/*
 * Command types
 *
 * Usage is a bit tricky as some bits are used as flags and some are not.
 *
 * Rules:
 *   LKL_VIRTIO_BLK_T_OUT may be combined with LKL_VIRTIO_BLK_T_SCSI_CMD or
 *   LKL_VIRTIO_BLK_T_BARRIER.  LKL_VIRTIO_BLK_T_FLUSH is a command of its own
 *   and may not be combined with any of the other flags.
 */

/* These two define direction. */
#define LKL_VIRTIO_BLK_T_IN		0
#define LKL_VIRTIO_BLK_T_OUT	1

#ifndef VIRTIO_BLK_NO_LEGACY
/* This bit says it's a scsi command, not an actual read or write. */
#define LKL_VIRTIO_BLK_T_SCSI_CMD	2
#endif /* VIRTIO_BLK_NO_LEGACY */

/* Cache flush command */
#define LKL_VIRTIO_BLK_T_FLUSH	4

/* Get device ID command */
#define LKL_VIRTIO_BLK_T_GET_ID    8

/* Discard command */
#define LKL_VIRTIO_BLK_T_DISCARD	11

/* Write zeroes command */
#define LKL_VIRTIO_BLK_T_WRITE_ZEROES	13

/* Secure erase command */
#define LKL_VIRTIO_BLK_T_SECURE_ERASE	14

#ifndef VIRTIO_BLK_NO_LEGACY
/* Barrier before this op. */
#define LKL_VIRTIO_BLK_T_BARRIER	0x80000000
#endif /* !VIRTIO_BLK_NO_LEGACY */

/*
 * This comes first in the read scatter-gather list.
 * For legacy virtio, if LKL_VIRTIO_F_ANY_LAYOUT is not negotiated,
 * this is the first element of the read scatter-gather list.
 */
struct lkl_virtio_blk_outhdr {
	/* VIRTIO_BLK_T* */
	__lkl__virtio32 type;
	/* io priority. */
	__lkl__virtio32 ioprio;
	/* Sector (ie. 512 byte offset) */
	__lkl__virtio64 sector;
};

/* Unmap this range (only valid for write zeroes command) */
#define LKL_VIRTIO_BLK_WRITE_ZEROES_FLAG_UNMAP	0x00000001

/* Discard/write zeroes range for each request. */
struct lkl_virtio_blk_discard_write_zeroes {
	/* discard/write zeroes start sector */
	__lkl__le64 sector;
	/* number of discard/write zeroes sectors */
	__lkl__le32 num_sectors;
	/* flags for this range */
	__lkl__le32 flags;
};

#ifndef VIRTIO_BLK_NO_LEGACY
struct lkl_virtio_scsi_inhdr {
	__lkl__virtio32 errors;
	__lkl__virtio32 data_len;
	__lkl__virtio32 sense_len;
	__lkl__virtio32 residual;
};
#endif /* !VIRTIO_BLK_NO_LEGACY */

/* And this is the final byte of the write scatter-gather list. */
#define LKL_VIRTIO_BLK_S_OK		0
#define LKL_VIRTIO_BLK_S_IOERR	1
#define LKL_VIRTIO_BLK_S_UNSUPP	2
#endif /* _LKL_LINUX_VIRTIO_BLK_H */
