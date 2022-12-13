#ifndef _TYPES_H_
#define _TYPES_H_


typedef int          boolean;
typedef uint16_t     flag16;

#define MAX_NAME_LEN    128     
// #define NEWFS_SUPER_OFS           0
// #define NEWFS_ERROR_NONE          0
// #define NEWFS_ROOT_INO            0
// #define NEWFS_ERROR_IO            EIO 
// #define NEWFS_MAGIC_NUM           0x52415453
// #define NEWFS_IO_SZ()                     (super.sz_io)
// #define NEWFS_DRIVER()                      (super.driver_fd)

// #define NEWFS_ROUND_DOWN(value, round)    (value % round == 0 ? value : (value / round) * round)
// // #define NEWFS_ROUND_DOWN(value, round)    (value % round == 0 ? value : (value / round) * round)
// #define NEWFS_ROUND_UP(value, round)      (value % round == 0 ? value : (value / round + 1) * round)
// #define NEWFS_DATA_PER_FILE       16
// #define NEWFS_INODE_PER_FILE      1
// #define UINT32_BITS             32
// #define UINT8_BITS              8
// #define NEWFS_MAX_FILE_NAME       128
// #define NEWFS_ERROR_NOSPACE       ENOSPC
// #define NEWFS_BLKS_SZ(blks)               (blks * NEWFS_IO_SZ())
typedef enum newfs_file_type {
    NEWFS_REG_FILE,
    NEWFS_DIR,
    NEWFS_SYM_LINK
} NEWFS_FILE_TYPE;

#define TRUE                    1
#define FALSE                   0
#define UINT32_BITS             32
#define UINT8_BITS              8

#define NEWFS_MAGIC_NUM           0x12345678  /* Define by yourself */
#define NEWFS_SUPER_OFS           0
#define NEWFS_ROOT_INO            0


#define NEWFS_SUPER_BLKS          1
#define NEWFS_MAP_INODE_BLKS      1
#define NEWFS_MAP_DATA_BLKS       1
#define NEWFS_INODE_BLKS          512
#define NEWFS_DATA_BLKS           2048

#define NEWFS_ERROR_NONE          0
#define NEWFS_ERROR_ACCESS        EACCES
#define NEWFS_ERROR_SEEK          ESPIPE     
#define NEWFS_ERROR_ISDIR         EISDIR
#define NEWFS_ERROR_NOSPACE       ENOSPC
#define NEWFS_ERROR_EXISTS        EEXIST
#define NEWFS_ERROR_NOTFOUND      ENOENT
#define NEWFS_ERROR_UNSUPPORTED   ENXIO
#define NEWFS_ERROR_IO            EIO     /* Error Input/Output */
#define NEWFS_ERROR_INVAL         EINVAL  /* Invalid Args */

#define NEWFS_MAX_FILE_NAME       128
#define NEWFS_DATA_PER_FILE       4
#define NEWFS_DEFAULT_PERM        0777     /* 全权限打开 */

#define NEWFS_IOC_MAGIC           'S'
#define NEWFS_IOC_SEEK            _IO(NEWFS_IOC_MAGIC, 0)

#define NEWFS_FLAG_BUF_DIRTY      0x1
#define NEWFS_FLAG_BUF_OCCUPY     0x2


#define NEWFS_INODE_PER_FILE      1

#define NEWFS_IO_SZ()                     (super.sz_io)       /* 512B*/
#define NEWFS_BLK_SZ()                    (super.sz_blk)      /* 1024B*/
#define NEWFS_DISK_SZ()                   (super.sz_disk)     /* 4MB */
#define NEWFS_DRIVER()                    (super.driver_fd)

#define NEWFS_ROUND_DOWN(value, round)    ((value) % (round) == 0 ? (value) : ((value) / (round)) * (round))
#define NEWFS_ROUND_UP(value, round)      ((value) % (round) == 0 ? (value) : ((value) / (round) + 1) * (round))

#define NEWFS_BLKS_SZ(blks)               ((blks) * NEWFS_BLK_SZ())
#define NEWFS_ASSIGN_FNAME(pnewfs_dentry, _fname) memcpy(pnewfs_dentry->fname, _fname, strlen(_fname))
#define NEWFS_INO_OFS(ino)                (super.inode_offset + (ino) * NEWFS_BLK_SZ())
#define NEWFS_DATA_OFS(bno)               (super.data_offset + (bno) * NEWFS_BLK_SZ())

// #define NEWFS_INO_OFS(ino)                (super.data_offset + ino * NEWFS_BLKS_SZ((\
                                        // NEWFS_INODE_PER_FILE + NEWFS_DATA_PER_FILE)))
// #define NEWFS_DATA_OFS(ino)               (NEWFS_INO_OFS(ino) + NEWFS_BLKS_SZ(NEWFS_INODE_PER_FILE))

#define NEWFS_IS_DIR(pinode)              (pinode->dentry->ftype == NEWFS_DIR)
#define NEWFS_IS_REG(pinode)              (pinode->dentry->ftype == NEWFS_REG_FILE)
#define NEWFS_IS_SYM_LINK(pinode)             (pinode->dentry->ftype == NEWFS_SYM_LINK)

struct newfs_dentry;
struct newfs_inode;
struct newfs_super;




struct custom_options {
	const char*        device;
};

struct newfs_super {
    // uint32_t magic;
    // int      fd;
    /* TODO: Define yourself */

    int                 driver_fd;          /* 打开的磁盘句柄 */
    int                 sz_io;              /* 512B */
    int                 sz_blk;             /* 1024B */
    int                 sz_disk;            /* 4MB */
    int                 sz_usage;           /* ioctl 相关信息 */

    int                 max_ino;
    int                 max_data;

    uint8_t*            map_inode;          /* 指向 inode 位图的内存起点 */ 
    int                 map_inode_blks;     /* inode 位图占用的块数 */
    int                 map_inode_offset;   /* inode 位图在磁盘上的偏移 */

    uint8_t*            map_data;           /* 指向 data 位图的内存起点 */ 
    int                 map_data_blks;      /* data 位图占用的块数 */
    int                 map_data_offset;    /* data 位图在磁盘上的偏移 */

    int                 inode_offset;       /* 索引结点的偏移 */
    int                 data_offset;        /* 数据块的偏移*/

    boolean             is_mounted;

    struct newfs_dentry*  root_dentry;
    // int     driver_fd;
    // int     sz_io;
    // int     sz_disk;
    // int     sz_usage;
    // int     max_ino;
    // int     map_inode;
    // int     map_inode_blks;
    // int     data_offset;

    // // bool true=1,false = 0
    // int     is_mounted;
    // struct newfs_dentry* root_dentry;
};

struct newfs_inode {
    uint32_t ino;
    /* TODO: Define yourself */
    // int     size;
    // int     dir_cnt;
    // struct  newfs_dentry* dentry;
    // struct  newfs_dentry* dentrys;
    // char               target_path[NEWFS_MAX_FILE_NAME];
    // uint8_t*    data;
    char                target_path[NEWFS_MAX_FILE_NAME];
    int                 size;                               /* 文件已占用空间 */
    int                 dir_cnt;
    struct newfs_dentry*  dentry;                             /* 指向该 inode 的 父dentry */
    struct newfs_dentry*  dentrys;                            /* 如果是 DIR，指向其所有子项（用链表串接）*/
    uint8_t*            block_pointer[NEWFS_DATA_PER_FILE];   /* 如果是 FILE，指向 4 个数据块，四倍结构 */
    int                 bno[NEWFS_DATA_PER_FILE];  
    uint8_t*           data;             
};

struct newfs_dentry {
    // char     name[MAX_NAME_LEN];
    uint32_t ino;
    /* TODO: Define yourself */
    
    // struct  newfs_dentry* parent;
    // struct  newfs_dentry* brother;
    // struct  newfs_inode* inode;
    // NEWFS_FILE_TYPE ftype;
     char                fname[NEWFS_MAX_FILE_NAME];
    struct newfs_dentry*  parent;             /* 父亲 Inode 的 dentry */
    struct newfs_dentry*  brother;            /* 下一个兄弟 Inode 的 dentry */
    // uint32_t            ino;
    struct newfs_inode*   inode;              /* 指向inode */
    NEWFS_FILE_TYPE       ftype;
};
// struct newfs_super_d {
//     uint32_t           magic_num;
//     int                sz_usage;

//     int                max_ino;
//     int                map_inode_blks;
//     int                map_inode_offset;
//     int                data_offset;
// };



// 
// static inline struct newfs_dentry* new_dentry(char * fname, NEWFS_FILE_TYPE ftype) {
//     struct newfs_dentry * dentry = (struct newfs_dentry *)malloc(sizeof(struct newfs_dentry));
//     memset(dentry, 0, sizeof(struct newfs_dentry));
//     SFS_ASSIGN_FNAME(dentry, fname);
//     dentry->ftype   = ftype;
//     dentry->ino     = -1;
//     dentry->inode   = NULL;
//     dentry->parent  = NULL;
//     dentry->brother = NULL;                                            
// }
static inline struct newfs_dentry* new_dentry(char * fname, NEWFS_FILE_TYPE ftype) {
    struct newfs_dentry * dentry = (struct newfs_dentry *)malloc(sizeof(struct newfs_dentry)); /* dentry 在内存空间也是随机分配 */
    memset(dentry, 0, sizeof(struct newfs_dentry));
    NEWFS_ASSIGN_FNAME(dentry, fname);
    dentry->ftype   = ftype;
    dentry->ino     = -1;
    dentry->inode   = NULL;
    dentry->parent  = NULL;
    dentry->brother = NULL;
    return dentry;                                            
}

struct newfs_super_d {
    uint32_t    magic_num;
    int         sz_usage;
    int         map_ino;
    int         map_inode_blks;     /* inode 位图占用的块数 */
    int         map_inode_offset;   /* inode 位图在磁盘上的偏移 */

    int         map_data_blks;      /* data 位图占用的块数 */
    int         map_data_offset;    /* data 位图在磁盘上的偏移 */

    int         inode_offset;       /* 索引结点的偏移 */
    int         data_offset;        /* 数据块的偏移*/
};

struct newfs_inode_d {
    uint32_t        ino;            /* 在inode位图中的下标 */
    int             size;           /* 文件已占用空间 */
    int             dir_cnt;
    NEWFS_FILE_TYPE   ftype;  
    char            target_path[NEWFS_MAX_FILE_NAME];
    int             bno[NEWFS_DATA_PER_FILE];       /* 数据块在磁盘中的块号 */
};

struct newfs_dentry_d {
    char            fname[NEWFS_MAX_FILE_NAME];
    NEWFS_FILE_TYPE   ftype;
    uint32_t        ino;            /* 指向的 ino 号 */
};
#endif /* _TYPES_H_ */