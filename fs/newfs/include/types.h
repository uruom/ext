#ifndef _TYPES_H_
#define _TYPES_H_


#define NEWFS_ROUND_DOWN(value, round)    (value % round == 0 ? value : (value / round) * round)
// #define NEWFS_ROUND_DOWN(value, round)    (value % round == 0 ? value : (value / round) * round)
#define NEWFS_ROUND_UP(value, round)      (value % round == 0 ? value : (value / round + 1) * round)
#define NEWFS_DATA_PER_FILE       16
#define NEWFS_INODE_PER_FILE      1
#define UINT32_BITS             32
#define UINT8_BITS              8
#define NEWFS_MAX_FILE_NAME       128
#define NEWFS_ERROR_NOSPACE       ENOSPC
#define NEWFS_BLKS_SZ(blks)               (blks * NEWFS_IO_SZ())
typedef enum newfs_file_type {
    NEWFS_REG_FILE,
    NEWFS_DIR,
    NEWFS_SYM_LINK
} NEWFS_FILE_TYPE;

struct custom_options {
	const char*        device;
};

struct newfs_super {
    uint32_t magic;
    int      fd;
    /* TODO: Define yourself */
    int     driver_fd;
    int     sz_io;
    int     sz_disk;
    int     sz_usage;
    int     max_ino;
    int     map_inode;
    int     map_inode_blks;
    int     data_offset;

    // bool true=1,false = 0
    int     is_mounted;
    struct newfs_dentry* root_dentry;
};

struct newfs_inode {
    uint32_t ino;
    /* TODO: Define yourself */
    int     size;
    int     dir_cnt;
    struct  newfs_dentry* dentry;
    struct  newfs_dentry* dentrys;
    char               target_path[SFS_MAX_FILE_NAME];
    uint8_t*    data;
};

struct newfs_dentry {
    char     name[MAX_NAME_LEN];
    uint32_t ino;
    /* TODO: Define yourself */
    
    struct  newfs_dentry* parent;
    struct  newfs_dentry* brother;
    struct  newfs_inode* inode;
    NEWFS_FILE_TYPE ftype;
};
struct newfs_super_d {
    uint32_t           magic_num;
    int                sz_usage;

    int                max_ino;
    int                map_inode_blks;
    int                map_inode_offset;
    int                data_offset;
};
static inline struct newfs_dentry* new_dentry(char * fname, NEWFS_FILE_TYPE ftype) {
    struct newfs_dentry * dentry = (struct newfs_dentry *)malloc(sizeof(struct newfs_dentry));
    memset(dentry, 0, sizeof(struct newfs_dentry));
    SFS_ASSIGN_FNAME(dentry, fname);
    dentry->ftype   = ftype;
    dentry->ino     = -1;
    dentry->inode   = NULL;
    dentry->parent  = NULL;
    dentry->brother = NULL;                                            
}


#endif /* _TYPES_H_ */