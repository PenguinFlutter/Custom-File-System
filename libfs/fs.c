#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include "disk.h"
#include "fs.h"


#define FAT_EOC 0xFFFF

//superblock struct
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

//root directory entry struct
typedef struct 
{
	uint8_t file_name[FS_FILENAME_LEN];
	uint32_t file_size;
	uint16_t first_block_index;
	uint8_t unused_padding[10];
}__attribute__((packed)) root_directory;

//FAT struct
typedef struct
{

    uint16_t *fat_arr;

}__attribute__((packed)) FAT;

//file descriptor struct
typedef struct {
	root_directory *rootAccess;
	size_t offset;
} file_descriptor;

//intialize objects
superblock super_block;
FAT fat;
root_directory root_arr[FS_FILE_MAX_COUNT];
file_descriptor fds[FS_OPEN_MAX_COUNT];

int fs_mount(const char *diskname)
{
    //attempt to open disk
	if (block_disk_open(diskname) == -1) 
	{
		return -1;
	}

    //read superblock from disk's block
	if (block_read(0, &super_block) == -1) 
	{
		return -1;
	}

    // check if superblock signature matches expected signature
	if (memcmp(super_block.signature, "ECS150FS", 8) != 0) 
	{
		return -1;
	}

    //Check if number of virtual blocks matches disk's block count
	if (block_disk_count() != super_block.total_virtual_blocks) 
	{
        return -1;
    }

    //calculate size of FAT and number of blocks
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

    //set first element of FAT array to indicate EOC
    fat.fat_arr[0] = FAT_EOC;

    //read root directory data from block and store it in root_arr
	if (block_read(super_block.root_directory_index, root_arr) == -1)
	{
		return -1;
	}
	
    //success
	return 0;
}

int fs_umount(void)
{
    //write superblock 
	if (block_write(0, &super_block) == -1) 
	{
		return -1;
	}

    //Write FAT data blocks
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

//print details about superblock,FAT,root_dir
int fs_info(void)
{
	printf("FS Info:\n");
    printf("total_blk_count=%i\n", super_block.total_virtual_blocks);
    printf("fat_blk_count=%i\n", super_block.FAT_blocks);
    printf("rdir_blk=%i\n", super_block.root_directory_index);
    printf("data_blk=%i\n", super_block.data_start_index);
    printf("data_blk_count=%i\n", super_block.data_blocks);

    int fat_count = 0;
    for (int i = 1; i < super_block.data_blocks; i++) {
        if (fat.fat_arr[i] == 0) {
            fat_count++;
        }
    }

	printf("fat_free_ratio=%i/%i\n", fat_count, super_block.data_blocks);

    int root_count = 0;
    for (int j = 0; j < FS_FILE_MAX_COUNT; j++) {
        if (root_arr[j].file_name[0] == '\0') {
            root_count++;
        }
    }

    printf("rdir_free_ratio=%i/%i\n", root_count, FS_FILE_MAX_COUNT);

    return 0;
}


int fs_create(const char *filename)
{
	// check if signature matches
	if (strncmp(super_block.signature, "ECS150FS", 8) != 0) 
    {
    	return -1;
	}

    //check if filename and length
	if (filename == NULL || strlen(filename) > FS_FILENAME_LEN) 
    {
		return -1;
	}

	//if file already exists
    for (int i = 0; i < FS_FILE_MAX_COUNT; ++i) 
    {
        if (strcmp((char*)root_arr[i].file_name, filename) == 0)
        {
            return -1;  
        }
    }

	//find an empty index in root directory
	int emptyIndex = -1;
	for (int i = 0; i < FS_FILE_MAX_COUNT; i++) 
    {
		if (root_arr[i].file_name[0] == '\0') 
        {
			emptyIndex = i;
			break;
		}
	}

	//intialize file entry in root directory
	strncpy((char*)root_arr[emptyIndex].file_name, filename, FS_FILENAME_LEN);
	root_arr[emptyIndex].file_size = 0;
	root_arr[emptyIndex].first_block_index = FAT_EOC;

    //success
	return 0;

}

int fs_delete(const char *filename)
{
    //check if signature matches
	if (strncmp(super_block.signature, "ECS150FS", 8) != 0) 
    {
    	return -1;
	}

    //check filename and length of file
	if (filename == NULL || strlen(filename) > FS_FILENAME_LEN) 
    {
		return -1;
	}

	//find file in root directory
	int fileIndex = -1;
	for (int i = 0; i < FS_FILE_MAX_COUNT; i++) 
    {
		if (strcmp((char*)root_arr[i].file_name, filename) == 0) 
        {
			fileIndex = i;
			break;
		}
	}

	if (fileIndex == -1) {
		return -1;
	}

	uint16_t first_blockIndex= root_arr[fileIndex].first_block_index;

	//update FAT to free blocks with given file
	while (first_blockIndex != FAT_EOC) 
    {
		uint16_t next_blockIndex = fat.fat_arr[first_blockIndex];
		fat.fat_arr[first_blockIndex] = 0;
		first_blockIndex = next_blockIndex;
	}

	//clean up the file entry in root directory
	root_arr[fileIndex].file_name[0] = '\0';
	root_arr[fileIndex].file_size = 0;
	root_arr[fileIndex].first_block_index = FAT_EOC;

	return 0; //success
}

int fs_ls(void)
{
	printf("FS Ls:\n");
	for (int i = 0; i < FS_FILE_MAX_COUNT; i++) 
    {
		if (root_arr[i].file_name[0] != '\0') //only print file entries that aren't empty
        {
			printf("file: %s, size: %d, data_blk: %d\n", root_arr[i].file_name, root_arr[i].file_size, root_arr[i].first_block_index);
		}
	}
    return 0; //success
}

int fs_open(const char *filename)
{
    //check if signature matches
	if (strncmp(super_block.signature, "ECS150FS", 8) != 0) 
    {
        return -1;
    }
    
	// Find an available file descriptor
    int fd;
    for (fd = 0; fd < FS_OPEN_MAX_COUNT; fd++) 
    {
        if (fds[fd].rootAccess == NULL) {
            break;
        }
    }

    // No available file descriptors
    if (fd == FS_OPEN_MAX_COUNT) 
    {
        return -1;
    }

    // Find the file in the root directory
    int fileIndex = -1;
    for (int i = 0; i < FS_FILE_MAX_COUNT; i++) 
    {
        if (strcmp((char*)root_arr[i].file_name, filename) == 0) 
        {
            fileIndex = i;
            break;
        }
    }

    //if file isn't found
    if (fileIndex == -1) 
    {
        return -1;
    } else if (fileIndex >= FS_FILE_MAX_COUNT) { //if file index reached or passed max count
        return -1;
    }

    // Initialize file descriptor
    fds[fd].rootAccess = &root_arr[fileIndex];
    fds[fd].offset = 0;

    return fd;
}

int fs_close(int fd)
{
    //check if signature matches
	if (strncmp(super_block.signature, "ECS150FS", 8) != 0) 
    {
        return -1;
    }
    //check if signature matches
    if (strncmp(super_block.signature, "ECS150FS", 8) != 0) {
        return -1;
    }
    
    //check fd
    if (fd < 0 || fd >= FS_OPEN_MAX_COUNT) {
        return -1;
    } else if (fds[fd].rootAccess == NULL) {
        return -1;
    }

    //reset fd
    fds[fd].rootAccess = NULL;
    fds[fd].offset = 0;

	return 0; //sucess
}

int fs_stat(int fd)
{
    //check if signature matches
	if (strncmp(super_block.signature, "ECS150FS", 8) != 0) 
    {
        return -1;
    }
    int size = 0;
	// Check if the file descriptor is valid
    if (fd < 0 || fd >= FS_OPEN_MAX_COUNT) 
    {
        return -1;
	} 

    else if (fds[fd].rootAccess == NULL) 
    {
		return -1;
	}

    //get file size from fd
	size = fds[fd].rootAccess->file_size;

    return size;
}

int fs_lseek(int fd, size_t offset)
{
    // check if signature matches
	if (strncmp(super_block.signature, "ECS150FS", 8) != 0) {
		return -1;
	}

    //check fd
	if (fd < 0) 
    {
		return -1;
	} 
    
    else if (fd >= FS_OPEN_MAX_COUNT) 
    {
		return -1;
	}
    
    else if (fds[fd].rootAccess == NULL) 
    {
		return -1;
	} 
    
    else if (fds[fd].rootAccess->file_size < offset) 
    {
		return -1;
	}

    //set offsett for given fd
 	fds[fd].offset = offset;

    return 0; //success
}

// Helper functions for fs_read() and fs_write()
int find_data_block_index(root_directory *fileEntry, size_t fileOffset) 
{
    uint16_t curBlockIndex = fileEntry->first_block_index;

    while (fileOffset >= BLOCK_SIZE) 
    {
        if (curBlockIndex == FAT_EOC) 
        {
            return FAT_EOC;  // Reached end of file
        }

        curBlockIndex = fat.fat_arr[curBlockIndex];
        fileOffset -= BLOCK_SIZE;
    }

    return curBlockIndex;
}

int allocate_new_data_block(root_directory *fileEntry) 
{
    uint16_t curBlockIndex = fileEntry->first_block_index;
    uint16_t prevBlockIndex = FAT_EOC;

    while (curBlockIndex != FAT_EOC) 
    {
        prevBlockIndex = curBlockIndex;
        curBlockIndex = fat.fat_arr[curBlockIndex];
    }

    for (int i = 0; i < super_block.data_blocks; i++) 
    {
        if (fat.fat_arr[i] == 0) 
        {
            fat.fat_arr[i] = FAT_EOC;
            if (prevBlockIndex == FAT_EOC) 
            {
                fileEntry->first_block_index = i;
            } else {
                fat.fat_arr[prevBlockIndex] = i;
            }

            return 0; //success

        }
    }

    return -1; 
}

int fs_write(int fd, void *buf, size_t count) 
{
    //check fd
    if (fd < 0 || fd >= FS_OPEN_MAX_COUNT) 
    {
        return -1;
    }

    if (fds[fd].rootAccess == NULL) 
    {
        return -1;
    }

    //get file entry, offset, and current block index from fd
    root_directory *fileEntry = fds[fd].rootAccess;
    size_t fileOffset = fds[fd].offset;
    uint16_t curBlockIndex = find_data_block_index(fileEntry, fileOffset);

    // Allocate new data blocks if needed
    while (curBlockIndex == FAT_EOC && count > 0) 
    {
        if (allocate_new_data_block(fileEntry) == -1) 
        {
            return -1;  // Failed to allocate a new data block
        }
        curBlockIndex = find_data_block_index(fileEntry, fileOffset);
    }

    //read current data block into bounce buffer
    char bounceBuffer[BLOCK_SIZE];
    if (block_read(super_block.data_start_index + curBlockIndex, bounceBuffer) == -1) 
    {
        return -1;
    }

    //calculate number of bytes to write in current data block
    size_t writeBytes = count;
    if (writeBytes > BLOCK_SIZE - fileOffset % BLOCK_SIZE) 
    {
        writeBytes = BLOCK_SIZE - fileOffset % BLOCK_SIZE;
    }

    memcpy(bounceBuffer + fileOffset % BLOCK_SIZE, buf, writeBytes);

    //write updated bounce buffer back to current block
    if (block_write(super_block.data_start_index + curBlockIndex, bounceBuffer) == -1) 
    {
        return -1;
    }

    // Update the fd's offset
    fds[fd].offset += writeBytes;

    // Update the file size if necessary
    if (fds[fd].offset > fileEntry->file_size) 
    {
        fileEntry->file_size = fds[fd].offset;
    }

    return writeBytes;
}

int fs_read(int fd, void *buf, size_t count)
{
    // Check if the fd is valid
    if (fd < 0 || fd >= FS_OPEN_MAX_COUNT) 
    {
        return -1;
    }

    if (fds[fd].rootAccess == NULL) 
    {
        return -1;
    }

    //get file entry, size, and current offset from fd
    root_directory *fileEntry = fds[fd].rootAccess;
    size_t fileSize = fs_stat(fd);
    size_t fileOffset = fds[fd].offset;

    if (fileOffset >= fileSize) 
    {
        return 0; // No more bytes to read
    }

    //calculate remaining bytes in file from offset
    size_t remainBytes = fileSize - fileOffset;

    // Find current block index and read block into bounce buffer
    char bounceBuffer[BLOCK_SIZE];
    uint16_t curBlockIndex = find_data_block_index(fileEntry, fileOffset);
    if (block_read(super_block.data_start_index + curBlockIndex, bounceBuffer) == -1) {
        return -1;
    }

    // bytes to read from current block
    size_t readBytes = count;
    if (readBytes > remainBytes) 
    {
        readBytes = remainBytes;
    }

    memcpy(buf, bounceBuffer + fileOffset % BLOCK_SIZE, readBytes);

    // update fd's offset
    fds[fd].offset += readBytes;

    return readBytes;
}