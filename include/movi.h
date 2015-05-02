#ifndef __MOVI_H__
#define __MOVI_H__
#include "smdkv210single.h"
#ifndef CONFIG_GENERIC_MMC

	/*
	这个config_generic_mmc应该是定义了的，删除了无用的信息
		*/
#else	/* !CONFIG_GENERIC_MMC */

#define total_size (512*2)//512KB,1024 sectors

#define MAGIC_NUMBER_MOVI	(0x24564236)

#define MOVI_TOTAL_BLKCNT       *((volatile unsigned int*)(SDMMC_BLK_SIZE))


#define SS_SIZE			(8 * 1024)

#if defined(CONFIG_EVT1)
#define eFUSE_SIZE		(1 * 512)	// 512 Byte eFuse, 512 Byte reserved
#else
#define eFUSE_SIZE		(1 * 1024)	// 1 kB eFuse, 1 KB reserved
#endif /* CONFIG_EVT1 */


#define MOVI_BLKSIZE		(1<<9) /* 512 bytes */

/* partition information */
#define PART_SIZE_BL		(512 * 1024)
#define PART_SIZE_KERNEL	(4 * 1024 * 1024)
#define PART_SIZE_ROOTFS	(26 * 1024 * 1024)

#define MOVI_LAST_BLKPOS	(MOVI_TOTAL_BLKCNT - (eFUSE_SIZE / MOVI_BLKSIZE))


#define MOVI_BL1_BLKCNT		(SS_SIZE / MOVI_BLKSIZE)	/* 8KB */
#define MOVI_ENV_BLKCNT		(CFG_ENV_SIZE / MOVI_BLKSIZE)	/* 16KB */
#define MOVI_BL2_BLKCNT		(PART_SIZE_BL / MOVI_BLKSIZE)	/* 512KB */
#define MOVI_ZIMAGE_BLKCNT	(PART_SIZE_KERNEL / MOVI_BLKSIZE)	/* 4MB */

/* Change writing block position at fused chip */
#if defined(CONFIG_EVT1)
#define MOVI_BL2_POS		((eFUSE_SIZE / MOVI_BLKSIZE) + MOVI_BL1_BLKCNT + MOVI_ENV_BLKCNT)
#endif

// #define MOVI_ZIMAGE_POS		(MOVI_BL2_POS - MOVI_ZIMAGE_BLKCNT)
#define MOVI_ROOTFS_BLKCNT	(PART_SIZE_ROOTFS / MOVI_BLKSIZE)

/*
 *
 * start_blk: start block number for image
 * used_blk: blocks occupied by image
 * size: image size in bytes
 * attribute: attributes of image
 *            0x1: u-boot parted (BL1)
 *            0x2: u-boot (BL2)
 *            0x4: kernel
 *            0x8: root file system
 *            0x10: environment area
 *            0x20: reserved
 * description: description for image
 * by scsuh
 */
typedef struct member {
	uint start_blk;
	uint used_blk;
	uint size;
	uint attribute; /* attribute of image */
	char description[16];
} member_t; /* 32 bytes */

/*
 * magic_number: 0x24564236
 * start_blk: start block number for raw area
 * total_blk: total block number of card
 * next_raw_area: add next raw_area structure
 * description: description for raw_area
 * image: several image that is controlled by raw_area structure
 * by scsuh
 */
typedef struct raw_area {
	uint magic_number; /* to identify itself */
	uint start_blk; /* compare with PT on coherency test */
	uint total_blk;
	uint next_raw_area; /* should be sector number */
	char description[16];
	member_t image[15];
} raw_area_t; /* 512 bytes */

#endif /* !CONFIG_GENERIC_MMC */

#endif /*__MOVI_H__*/
