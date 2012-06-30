#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <time.h>

#define ROOT_INODE 1

struct partitionInfo
{
	long sectors; //sectors count (including the unused block)
	int offset; //offset from start of file
} partitionInfo[4];


// 磁盘上的索引节点(i 节点)数据结构。
typedef struct d_inode
{
  unsigned short i_mode;	// 文件类型和属性(rwx 位)。
  unsigned short i_uid;		// 用户id（文件拥有者标识符）。
  unsigned long i_size;		// 文件大小（字节数）。
  unsigned long i_time;		// 修改时间（自1970.1.1:0 算起，秒）。
  unsigned char i_gid;		// 组id(文件拥有者所在的组)。
  unsigned char i_nlinks;	// 链接数（多少个文件目录项指向该i 节点）。
  unsigned short i_zone[9];	// 直接(0-6)、间接(7)或双重间接(8)逻辑块号。
// zone 是区的意思，可译成区段，或逻辑块。
}d_inode;

// 磁盘上超级块结构。上面125-132 行完全一样。
struct d_super_block
{
  unsigned short s_ninodes;	// 节点数。
  unsigned short s_nzones;	// 逻辑块数。
  unsigned short s_imap_blocks;	// i 节点位图所占用的数据块数。
  unsigned short s_zmap_blocks;	// 逻辑块位图所占用的数据块数。
  unsigned short s_firstdatazone;	// 第一个数据逻辑块。
  unsigned short s_log_zone_size;	// log(数据块数/逻辑块)。（以2 为底）。
  unsigned long s_max_size;	// 文件最大长度。
  unsigned short s_magic;	// 文件系统魔数。
};

// 文件目录项结构。
#define NAME_LEN 14		// 名字长度值
struct dir_entry
{
  unsigned short inode;		// i 节点。
  char name[NAME_LEN];		// 文件名。
};

int cur_part_offfset = 0;
char current_path_name[1024]={0};
FILE* fp = 0;
//int indentLevel = 0;
d_inode currentNode;
d_inode rootNode;
int inodeStartPos = 0;
int currentNodeIndex = -1;
void get_partition_info();
struct d_inode readInode(unsigned short inodeIndex);
void showDirectory(const struct d_inode* pInode, const int startPos, int recursive, int depth);
void showFileInfo(const d_inode* pInode, const char* name, int nodeId, int showColumn);
void getSubItems(const struct d_inode* pInode, struct dir_entry* pDirs);
void get_modeStr(unsigned short i_mode, char res[]);
unsigned short _bmap(const struct d_inode* pInode, unsigned short block);
void run();
void showDirItems(const struct d_inode* pInode, const char* name, int nodeId, int nameOnly);
int inodeFromName(const char* name, int length, const d_inode* parent);
int inodeFromPathName(const char* pathName);
void makeFullPath(const char* pathName, char fullPath[]);
void argFromCmd(const char* command, char arg[]);
void trim_str(char* str);
void setCurrentNode(int nodeId, d_inode node);
int parseImage(int partition);
int isImageFile();
int isMinixSystem(int partition);
void getParentPath(const char* currentPath, char parentPath[]);
void showFileInfoHead();

int main(int argc, char* argv[])
{
	char* filename = 0;
	int i=0;
	int count = 0;
	int ok = 0;
	if(argc < 2)
	{
		char buf[256];
		printf("Input image file name: ");
		scanf("%s", buf);
		fp = fopen(buf, "rb");
	}
	else
		fp = fopen(argv[1], "rb");
	if(!fp)
	{
		printf("Failed to open the input file\n");
		return -1;
	}
	if(!isImageFile())
	{
		printf("Not a hard disk image file\n");
		return -1;
	}

	get_partition_info();
	count = 0;
	for(i=0; i<4; i++)
	{
		if(partitionInfo[i].sectors && isMinixSystem(i))
		{
			printf("Minix System found on partition: %d\n", i+1);
			count++;
		}
	}
	if(count == 0)
	{
		printf("No Minix System found on any partition.\n");
		return -1;
	}	

	if(count > 1)
	{
		int partitionId;
		printf("Select a partition, possible values are: ");
		for(i=0; i<4; i++)
		{
			if(partitionInfo[i].sectors)
				printf("%d ", i + 1);
		}
		printf("\nInput your choice: ");
		scanf("%d", &partitionId);
		partitionId--;
		if(partitionId >=0 && partitionId < 4)
			ok = parseImage(partitionId);
		else
		{
			printf("Input id is invalid.\n");
			return -1;
		}
	}
	else//only one minix partition
	{
		for(i=0; i<4; i++)
			if(partitionInfo[i].sectors)
			{
				ok = parseImage(i);
				break;
			}
	}

	//run command
	if(!ok)
		return -1;

	run();

	fclose(fp);
	return 0;
} 

int isImageFile()
{
	unsigned char mbrFlag[2] = {0, 0};
	fseek(fp, 510, 0);
	return (fread(mbrFlag, 2, 1, fp) == 1) && mbrFlag[0] == (unsigned char)0x55 
		&& mbrFlag[1] == (unsigned char)0xaa;
}

int isMinixSystem(int partition)
{
	unsigned char c;
	int pos = 512 - 64 - 2 + partition * 16 + 4;
	fseek(fp, pos, 0);
	return fread(&c, 1, 1, fp) == 1 && c == (unsigned char)0x81;
}


void get_partition_info()
{
	int i=0;
	unsigned char head_end = 0;
	unsigned char systemId;
	for(  i=0; i<4; i++)
	{
		int start_pos = 512 - 64 - 2 + i * 16;

		fseek(fp, start_pos + 4, 0);
		fread(&systemId, 1, 1, fp);
		if(systemId == (char)0)
		{
			partitionInfo[i].sectors = 0;
			continue;
		}
 
		fseek(fp, start_pos + 8, 0);
		fread(&partitionInfo[i].offset, 4, 1, fp); 
		fread(&partitionInfo[i].sectors, 4, 1, fp); 
		printf("partition %d: %d sectors = %dM, offset = %d sector(s)\n", i + 1, partitionInfo[i].sectors, (partitionInfo[i].sectors << 9) >> 20, partitionInfo[i].offset);
	}
}


 
int parseImage(int partition)
{
	int i=0;
	int sectorsBefore = 0;
	struct d_super_block sb;
	char modeStr[11] = {0};
 
	if(partitionInfo[partition].sectors == 0)
		return 0;

	cur_part_offfset = partitionInfo[partition].offset * 512;
	fseek(fp, cur_part_offfset + 1024, 0);
	
   //read super block
   fread(&sb, sizeof(sb), 1, fp);
   printf("super block info for partition %d:\n", partition + 1);
   printf("\ts_ninodes=%d, s_nzones=%d, s_imap_blocks=%d, s_zmap_blocks=%d, s_firstdatazone=%d, s_log_zone_size=%d, s_max_size=%d, s_magic=0x%02x\n", 
	   sb.s_ninodes, sb.s_nzones, sb.s_imap_blocks, sb.s_zmap_blocks, sb.s_firstdatazone, sb.s_log_zone_size, sb.s_max_size, sb.s_magic);
   if(sb.s_magic != 0x137F)
   {
	   printf("File system is not minix 1.0\n");
	   return 0;
   }
 
   //read root inode
    inodeStartPos = cur_part_offfset + 1024 + ( 1 + sb.s_imap_blocks + sb.s_zmap_blocks) * 1024;
	rootNode = readInode(ROOT_INODE);
	currentNode = rootNode;
	strcpy(current_path_name, "/");
	currentNodeIndex = ROOT_INODE;
	return 1;
}

// 以下这些是st_mode 值的符号名称。
// 文件类型：
#define S_IFMT 00170000		// 文件类型（8 进制表示）。
#define S_IFREG 0100000		// 常规文件。
#define S_IFBLK 0060000		// 块特殊（设备）文件，如磁盘dev/fd0。
#define S_IFDIR 0040000		// 目录文件。
#define S_IFCHR 0020000		// 字符设备文件。
#define S_IFIFO 0010000		// FIFO 特殊文件。
 

#define S_ISREG(m) (((m) & S_IFMT) == S_IFREG)	// 测试是否常规文件。
#define S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)	// 是否目录文件。
#define S_ISCHR(m) (((m) & S_IFMT) == S_IFCHR)	// 是否字符设备文件。
#define S_ISBLK(m) (((m) & S_IFMT) == S_IFBLK)	// 是否块设备文件。
#define S_ISFIFO(m) (((m) & S_IFMT) == S_IFIFO)	// 是否FIFO 特殊文件。

void showFileInfo(const d_inode* pInode, const char* name, int nodeId, int showColumn)
{
	char modeStr[11] = {0};
	char time_buf[64] = {0};
	if(showColumn)
		showFileInfoHead();
	get_modeStr(pInode->i_mode, modeStr);
	strcpy(time_buf, ctime(&pInode->i_time));
	time_buf[strlen(time_buf) - 1] = 0;	//remove '\n'
	printf("%10s\t%4d\t%4d\t%4d\t%4d\t%10d\t%32s\t%s\n", modeStr, pInode->i_nlinks,
	pInode->i_uid, pInode->i_gid, nodeId, pInode->i_size, time_buf, name);
}

void showFileInfoHead()
{
	printf("%10s\t%4s\t%4s\t%4s\t%4s\t%10s\t%32s\t%s\n","mode","nlinks", "uid", 
		"gid","inode", "size", "time", "name");
}

void showDirItems(const struct d_inode* pInode, const char* name, int nodeId, int nameOnly)
{
	int i; 
	int itemCount, realCount;
	struct dir_entry* pDirs = 0;
	int size = 0;
	if(!S_ISDIR(pInode->i_mode))  
	{
	 	if(strcmp(name, current_path_name) != 0)	//ignore current directory
		{
			if(nameOnly)
				printf("%s\n", name);
			else
				showFileInfo(pInode, name, nodeId, 1);
		}
		return;
	}

  if(!nameOnly)
		showFileInfoHead();
	realCount = 0;
	itemCount =  pInode->i_size/sizeof(struct dir_entry);
	pDirs = (struct dir_entry*)malloc(pInode->i_size);
	getSubItems(pInode, pDirs);
	for(i=0; i<itemCount; i++)
	{
		struct d_inode inode;
		if(pDirs[i].name[0] == '.' && pDirs[i].name[1] == 0
			|| pDirs[i].name[0] == '.' && pDirs[i].name[1] == '.' && pDirs[i].name[2] == 0)
			continue;
		if(pDirs[i].inode < ROOT_INODE)
			continue;
		inode = readInode(pDirs[i].inode);
		if(nameOnly)
		{
			printf("%s\t", pDirs[i].name);
			if(i == itemCount - 1)
				printf("\n");
		}
		else
			showFileInfo(&inode, pDirs[i].name, pDirs[i].inode, 0);
		size += inode.i_size;
		realCount++;
	}
	printf("\n%d items, total size = %d bytes = %0.2fK = %0.2fM\n", 
		realCount, size, (float)size/1024, (float)size/1024/1024);
	free(pDirs);
	pDirs = 0;
}

struct d_inode readInode(unsigned short inodeIndex)
{
	int offset = 0;
	struct d_inode inode;
	if( currentNodeIndex >= ROOT_INODE)	//filter initialization
	{
		if(inodeIndex == ROOT_INODE)
			return rootNode;
		if(inodeIndex == currentNodeIndex)
			return currentNode;
	}
	offset = inodeStartPos + (inodeIndex - ROOT_INODE) * sizeof(struct d_inode);
	fseek(fp, offset, 0);
	fread(&inode, sizeof(inode), 1, fp);
	return inode;
}

void getSubItems(const struct d_inode* pInode, struct dir_entry* pDirs)
{
	int itemCount = pInode->i_size/sizeof(struct dir_entry);
	int i=0;
	const int itemsPerBlock = 1024 / sizeof(struct dir_entry);
	int itemsRemain = itemCount;
	int nextBlock = 0;
	while(itemsRemain > 0)
	{
		int diskBlock = _bmap(pInode, nextBlock++);
		int itemsToRead = itemsRemain > itemsPerBlock ? itemsPerBlock : itemsRemain;
		fseek(fp, diskBlock * 1024 + cur_part_offfset, 0); 
		fread(pDirs + (itemCount - itemsRemain), sizeof(struct dir_entry), itemsToRead, fp);
		itemsRemain -= itemsToRead;
	}
}

void get_modeStr(unsigned short i_mode, char res[])
{
	int i=0;
	if(S_ISREG(i_mode))
		res[0] = '-';
	else if(S_ISDIR(i_mode))
		res[0] = 'd';
	else if(S_ISCHR(i_mode))
		res[0] = 'c';
	else if(S_ISBLK(i_mode))
		res[0] = 'b';
	else if(S_ISFIFO(i_mode))
		res[0] = 'f';
	else
		res[0] = '?';
	
	for(i=1; i<=9; i++)
		res[i] = '-';

	if(i_mode & 00400)
		res[1] = 'r';
	if(i_mode & 00200)
		res[2] = 'w';
	if(i_mode & 00100)
		res[3] = 'x';

	if(i_mode & 00040)
		res[4] = 'r';
	if(i_mode & 00020)
		res[5] = 'w';
	if(i_mode & 00010)
		res[6] = 'x';

	if(i_mode & 00004)
		res[7] = 'r';
	if(i_mode & 00002)
		res[8] = 'w';
	if(i_mode & 00001)
		res[9] = 'x';
}

unsigned short _bmap(const struct d_inode* pInode, unsigned short block)
{
	unsigned short block1 = 0, block2 = 0;
	int pos = 0;
	if(block < 7)
		return pInode->i_zone[block];
	block -= 7;

	if(block < 512)
	{
		block1 = pInode->i_zone[7];
		 pos = block1 * 1024 + block * 2;
		fseek(fp, pos + cur_part_offfset, 0);
		fread(&block1, 2, 1, fp);
		return block1;
	}

	block -= 512;
	block1 = pInode->i_zone[8];
	pos = block1 * 1024 + block / 512 * 2;
	fseek(fp, pos + cur_part_offfset, 0);
	fread(&block2, 2, 1, fp);
	pos = block2 * 1024 + block % 512 * 2;	
	fseek(fp, pos + cur_part_offfset, 0);
	fread(&block2, 2, 1, fp);
	return block2;
} 

int inodeFromPathName(const char* pathName)
{
    const char* start = 0, *end = 0;
	d_inode parentNode = currentNode;
	int nodeId = currentNodeIndex;
	if(!pathName[0])
		return currentNodeIndex;

	if(strcmp(pathName, ".") == 0)
		return currentNodeIndex;

	if(strcmp(pathName, "..") == 0)
	{
		//root dir's parent is itself
		if(currentNodeIndex == ROOT_INODE)
			return ROOT_INODE;

		//current_path_name will never end with '/'
		end = current_path_name + strlen(current_path_name) - 1;
		while(end >= current_path_name)
		{
			if(*end == '/')
			{
				if(end > current_path_name)
				{
					char path[1024] = {0};
					strncpy(path, current_path_name, end - current_path_name);
					path[end - current_path_name] = 0;
					return inodeFromPathName(path);
				}
				else
					return ROOT_INODE; //return root node
			}
			end--;
		}

		return -1;	//no parent node
	}

	if(pathName[0] == '/')
	{
		parentNode = rootNode;
		nodeId = ROOT_INODE;
		pathName++;
	}
	end = start = pathName;
	while(*end)
	{
		if(*end == '/')
		{
			nodeId = inodeFromName(start, end - start, &parentNode);
			if(nodeId < ROOT_INODE)
				return -1;
			parentNode = readInode(nodeId);
			start = ++end;
		}
		else
			end++;
	}
	if( end == start) //for path such as '/etc/'
		return nodeId;
	return inodeFromName(start, end - start, &parentNode);
}

int inodeFromName(const char* name, int length, const d_inode* parent)
{
	int i;
	int itemCount = parent->i_size / sizeof(struct dir_entry);
	struct dir_entry* pDirs = (struct dir_entry*)malloc(parent->i_size);
	getSubItems(parent, pDirs);
	for(i =0; i<itemCount; i++)
	{
		if(pDirs[i].inode < ROOT_INODE)	//ignore invalid entries
			continue;
		if(strlen(pDirs[i].name) == length && strncmp(pDirs[i].name, name, length) == 0)
			return pDirs[i].inode;
	}
	free(pDirs);
	pDirs = 0;
	return -1;
}

void run()
{
	char cmdBuf[1024];
	char cmdArg[1024];
	printf("Now we are in root directory, type 'help' for help.\n");
	getchar();//clear buf
	while(1)
	{
		printf("[%s]$ ", current_path_name);
		fgets(cmdBuf, sizeof(cmdBuf), stdin);
		trim_str(cmdBuf);
		if(!cmdBuf[0])
			continue;
		if(strcmp(cmdBuf, "exit") == 0)
			break;
		if(strncmp(cmdBuf,"cd ", 3) == 0 || strncmp(cmdBuf, "ls ", 3) == 0 || strncmp(cmdBuf, "ll ", 3) == 0
			|| strncmp(cmdBuf, "lld ", 4) == 0)
		{
			char fullName[1024];
			int nodeId = -1;
			int fullNameLength = 0;
			argFromCmd(cmdBuf, cmdArg);
			nodeId = inodeFromPathName(cmdArg);
			if(nodeId < ROOT_INODE)
			{
				printf("Invalid path: %s\n", cmdArg);
				continue;
			}
			d_inode node = readInode(nodeId);
			makeFullPath(cmdArg, fullName);
			fullNameLength = strlen(fullName);
			if(fullNameLength > 1 && fullName[fullNameLength - 1] == '/')
				fullName[fullNameLength - 1] = 0;
			if(strncmp(cmdBuf,"cd ", 3) == 0)
			{
				if(!S_ISDIR(node.i_mode))  
				{
					printf("%s is not a directory\n", fullName);
					continue;
				}
				strcpy(current_path_name,  fullName);
				//change current path
				setCurrentNode(nodeId, node);
				continue;
			}
			else if(strncmp(cmdBuf, "ls ", 3) == 0)
			{
				showDirItems(&node, cmdArg, nodeId, 1);
				continue;
			}
			else if(strncmp(cmdBuf, "ll ", 3) == 0)
			{
				showDirItems(&node, cmdArg, nodeId,0);
				continue;
			}
			else if(strncmp(cmdBuf, "lld ", 4) == 0)
			{
				showFileInfo(&node, cmdArg, nodeId, 1);
				continue;
			}
		}
		if(strcmp(cmdBuf, "ls") == 0)
		{
			showDirItems(&currentNode, current_path_name, currentNodeIndex, 1);
			continue;
		}
		if(strcmp(cmdBuf, "ll") == 0)
		{
			showDirItems(&currentNode, current_path_name, currentNodeIndex, 0);
			continue;
		}

		if(strncmp(cmdBuf, "help", 3) == 0)
		{
			printf("supported commands are:\n");
			printf("1. cd <path>\n");
			printf("2. ls [file]\n");
			printf("3. ll [file]\n");
			printf("4. lld <dir>\n");
			printf("5. exit\n");
			printf("6. help\n");
			continue;
		}

		printf("Unknown command. Type 'help' to get help.\n");
	}
}

void trim_str(char* str)
{
	char* start = str, *end = str + strlen(str) - 1;
	char c;
	while(start <= end && ( (c = *start)== ' ' ||  c== '\t' || c== '\n'))
		start++;
	while(end >= start && ( (c = *end)== ' ' ||  c== '\t' || c== '\n'))
		end--;
	if(end < start)
		str[0] = 0;
	else
	{
		strncpy(str, start, end - start + 1);
		str[end - start + 1] = 0;
	}
}

void argFromCmd(const char* command, char arg[])
{
	arg[0] = 0;
	while(*command && *command != ' ' &&  *command != '\t')
		command++;
	while(*command && (*command == ' ' ||  *command == '\t'))
		command++;	
	strcpy(arg, command);
	trim_str(arg);
}


void  getParentPath(const char* currentPath, char parentPath[])
{
 		const char* end = currentPath + strlen(currentPath) - 1;
		if(*end == '/' && !*(end+1))
		{
			parentPath[0] = '/';
			parentPath[1] = 0;
			return;
		}
		if(*end == '/')
			end--;
		while(end >= currentPath)
		{
			if(*end == '/')
			{
				if(end > currentPath)
				{
					strncpy(parentPath, currentPath, end - currentPath);
					parentPath[end - currentPath] = 0;
					return;
				}
				else
				{
					strcpy(parentPath, "/");
					return;
				}
			}
			end--;
		}
		
		return;
}

void setCurrentNode(int nodeId, d_inode node)
{
	if(nodeId == currentNodeIndex)
		return;
	
	currentNode = node;
	currentNodeIndex =nodeId; 
}

void makeFullPath(const char* pathName, char fullPath[])
{
	if(strcmp(pathName, ".") == 0)
	{
		strcpy(fullPath, current_path_name); 
		return;
	}

	if(strcmp(pathName, "..") == 0)
	{
		getParentPath(current_path_name, fullPath);
		return;
	}


	if(pathName[0] == '/')
	{
		strcpy(fullPath, pathName);
		return;
	}
	else
	{
		int length = strlen(pathName);
		int offset = 0;
		strcpy(fullPath, current_path_name);
		while(strncmp(pathName + offset, "../", 3) == 0)
		{
			getParentPath(fullPath, fullPath);
			offset += 3;
		}

		if(fullPath[1]) //append '/' for non root path
			strcat(fullPath, "/");
		strcat(fullPath, pathName + offset);
		return;
	}
}
