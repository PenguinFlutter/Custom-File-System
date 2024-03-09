# ECS150_Proj3

## Summary 

The purpose of this project is to implement the support of a simple file system called ECS150-FS, which operates based on a FAT (File Allocation Table) structure and accommodates up to 128 files within a single root directory. 

Similar to physical hard drives divided into sectors, the virtual disk is conceptually divided into blocks. The initial software layer involved in this file system's implementation is the block API, which facilitates operations such as opening or closing the virtual disk and reading or writing entire blocks from it.

Above the block layer lies the FS layer, responsible for managing the file system's operations. Through the FS layer, users can mount a virtual disk, enumerate files contained within it, add or remove files, perform file read and write operations, and more.

## Implementation

### Phase 1:

#### Superblock
The implementation of the `superblock` struct, represents the first block of the file system. In order to mimick its internal format, we initialized the following:

`signature`: An array of characters with a length of 8, identifying the file system "ECS150FS".

`total_virtual_blocks`: A 16-bit unsigned integer indicating the total number of virtual blocks in the file system.

`root_directory_index`: A 16-bit unsigned integer representing the index of the root directory within the file system. 

`data_start_index`: A 16-bit unsigned integer representing the index where the actual data blocks start within the file system. 

`data_blocks`: A 16-bit unsigned integer indicating the total number of data blocks in the file system.

`FAT_blocks`: An 8-bit unsigned integer representing the number of File Allocation Table (FAT) blocks in the file system.

`unused_padding`: An array of 4079 bytes, used for padding to ensure that the structure is a specific size or alignment.

#### File Allocation Table (FAT)
The implementation of the `FAT` struct represents the File Allocation Table (FAT).
`fat_arr`: A pointer to 16-bit unsigned integer array which holds the FAT entries.

#### Root Directory
The implementation of a `root_directory` struct, represents an array of 128 entires after the File Allocation Table (FAT):

`file_name`: An 8-bit unsigned integer array, representing the name of the file. 

`file_size`: A 32-bit unsigned integer representing the size of the file.

`first_block_index`: A 16-bit unsigned integer representing the index of the first block.
`unused_padding`: An array of 10 bytes, used for padding to ensure that the structure is a specific size or alignment. 

#### fs_mount()
The `fs_mount()` function first opens the specific virtual disk named "read to be used" and reads the superblock from block 0 using `block_read()`. Upon successful completion, it calculates the size of the FAT based on the number of data blocks and block size. This function also determines the number of blocks occupied by the FAT and initializes the FAT array within the FAT structure by dynamically allocating memory using `malloc()`. It then reads the FAT from the disk into the allocated memory in chunks of `BLOCK_SIZE`, iterating over the number of FAT blocks and reading each block using `block_read()`. The first element of the FAT array is set to indicate the End of Chain (EOC). Finally, it reads the root directory data from the block specified in the superblock's `root_directory_index` and stores it in `root_arr`.

#### fs_umount()
The `fs_umount` function is designed to unmountthe file system. It begins by writing the `superblock` back to the disk at block 0 and then proceeds to write the FAT data blocks sequentially. For each FAT data block, it calculates the appropriate offset in the FAT array and writes the corresponding block to the disk. After writing the FAT data blocks, it frees the memory allocated for the FAT array. Next, it writes the root directory data back to the disk at the block specified in the superblock's `root_directory_index`. Finally, the function closes the disk using `block_disk_close`.

#### fs_info()
The `fs_info` function is responsible for providing information about the file system. It prints various parameters obtained from the superblock, such as the total number of blocks, the number of FAT blocks, the block index of the root directory, the index of the first data block, and the total number of data blocks. It also calculates the number of free blocks in the FAT and the number of free entries in the root directory then prints them out. 

### Phase 2
The `fs_create` function is designed to create a new file within the file system. Initially, it verifies if the file system is mounted by checking the signature in the superblock. Following this, it scans the root directory array to confirm whether a file with the same name already exists. Upon finding an available slot in the root directory, it initializes the file entry by copying the filename, setting the file size to 0, and assigning the first block index to the `FAT_EOC`.







https://stackoverflow.com/questions/65894320/could-someone-explain-to-me-what-uint64-t-is-doing-exactly
https://clickhouse.com/docs/en/sql-reference/data-types/int-uint
https://github.com/Ell4iot/simple-file-system/blob/main/libfs/fs.c
https://www.geeksforgeeks.org/difference-d-format-specifier-c-language/
https://cplusplus.com/reference/cstring/memcmp/