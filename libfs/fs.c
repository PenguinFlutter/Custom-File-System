#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "disk.h"
#include "fs.h"

#define FAT_EOC 0xFFFF

typedef struct 
{
	char signature[8];
	uint16_t total_virtual_blocks;
	uint16_t root_directory_index;
	uint16_t data_start_index;
	uint16_t data_blocks;
	uint8_t FAT_blocks;
	uint8_t unused_padding[4079];
}__attribute__((packed)) superblock;

typedef struct 
{
	uint8_t file_name[FS_FILENAME_LEN];
	uint32_t file_size;
	uint16_t first_data_index;
	uint8_t unused_padding[10];
}__attribute__((packed)) root_directory;

typedef struct
{
    uint16_t *fat_arr;

}__attribute__((packed)) FAT;

superblock super_block;
FAT fat;
root_directory root_arr[FS_FILE_MAX_COUNT];

/* TODO: Phase 1 */

int fs_mount(const char *diskname)
{
	if (block_disk_open(diskname) == -1) 
	{
		return -1;
	}

	if (block_read(0, &super_block) == -1) 
	{
		return -1;
	}

	if (memcmp(super_block.signature, "ECS150FS", 8) != 0) 
	{
		return -1;
	}

	if (block_disk_count() != super_block.total_virtual_blocks) 
	{
        return -1;
    }

	int FAT_size = super_block.data_blocks * 2;
	int num_FAT_block = (FAT_size / BLOCK_SIZE);
    if ((FAT_size * 2) % BLOCK_SIZE > 0) 
	{
        num_FAT_block++;
	}
    
	// Initialize FAT array
    fat.fat_arr = (uint16_t*)malloc(super_block.FAT_blocks * BLOCK_SIZE * sizeof(uint16_t));

    for (int data_blocks = 0; data_blocks < num_FAT_block; data_blocks++)
    {
        if (block_read(data_blocks + 1, fat.fat_arr + BLOCK_SIZE * data_blocks) == -1)
        {
            return -1;
        }
    }
    fat.fat_arr[0] = FAT_EOC;
	if (block_read(super_block.root_directory_index, root_arr) == -1)
	{
		return -1;
	}
	
	return 0;
    // Root directory creation
       /* TODO: Phase 1 */
}

int fs_umount(void)
{
	if (block_write(0, &super_block) == -1) 
	{
		return -1;
	}

	for (int data_blocks = 0; data_blocks < super_block.FAT_blocks; data_blocks++)
    {
        if (block_write(data_blocks + 1, fat.fat_arr + data_blocks * BLOCK_SIZE) == -1) 
		{
            return -1;
        }
    }

    free(fat.fat_arr);
	
	if (block_write(super_block.root_directory_index, root_arr) == -1) 
	{
		return -1;
	}

	if (block_disk_close() == -1)
	{
		return -1;
	}

	return 0; 
}

int fs_info(void)
{
	printf("FS Info:\n");
    printf("total_blk_count=%i\n", super_block.total_virtual_blocks);
    printf("fat_blk_count=%i\n", super_block.FAT_blocks);
    printf("rdir_blk=%i\n", super_block.root_directory_index);
    printf("data_blk=%i\n", super_block.data_start_index);
    printf("data_blk_count=%i\n", super_block.data_blocks);

    int fat_count = 0;
    for (int i = 1; i < super_block.data_blocks; i++) 
	{
        if (fat.fat_arr[i] == 0) 
		{
            fat_count++;
        }
    }

	printf("fat_free_ratio=%i/%i\n", fat_count, super_block.data_blocks);

    int root_count = 0;
    for (int i = 0; i < FS_FILE_MAX_COUNT; i++) 
	{
        if (root_arr[i].file_name[0] == '\0') 
		{
            root_count++;
        }
    }

    printf("rdir_free_ratio=%i/%i\n", root_count, FS_FILE_MAX_COUNT);

    return 0;
	/* TODO: Phase 1 */
}

int fs_create(const char *filename)
{
	/* TODO: Phase 2 */
}

int fs_delete(const char *filename)
{
	/* TODO: Phase 2 */
}

int fs_ls(void)
{
	/* TODO: Phase 2 */
}

int fs_open(const char *filename)
{
	/* TODO: Phase 3 */
}

int fs_close(int fd)
{
	/* TODO: Phase 3 */
}

int fs_stat(int fd)
{
	/* TODO: Phase 3 */
}

int fs_lseek(int fd, size_t offset)
{
	/* TODO: Phase 3 */
}

int fs_write(int fd, void *buf, size_t count)
{
	/* TODO: Phase 4 */
}

int fs_read(int fd, void *buf, size_t count)
{
	/* TODO: Phase 4 */
}

