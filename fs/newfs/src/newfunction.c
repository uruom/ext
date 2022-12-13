#include "../include/newfs.h"

extern struct custom_options newfs_options; 
extern struct newfs_super super;


char* newfs_get_fname(const char* path) {
    char ch = '/';
    char *q = strrchr(path, ch) + 1;
    return q;
}
/**
 * @brief 计算路径的层级
 * exm: /av/c/d/f
 * -> lvl = 4
 * @param path 
 * @return int 
 */
int newfs_calc_lvl(const char * path) {
    // char* path_cpy = (char *)malloc(strlen(path));
    // strcpy(path_cpy, path);
    char* str = path;
    int   lvl = 0;
    if (strcmp(path, "/") == 0) {
        return lvl;
    }
    while (*str != NULL) {
        if (*str == '/') {
            lvl++;
        }
        str++;
    }
    return lvl;
}
/**
 * @brief 驱动读
 * 
 * @param offset 
 * @param out_content 
 * @param size 
 * @return int 
 */
int newfs_driver_read(int offset, uint8_t *out_content, int size) {
    /* 按 1024B 读取*/
    int      offset_aligned = NEWFS_ROUND_DOWN(offset, NEWFS_BLK_SZ());
    int      bias           = offset - offset_aligned;
    int      size_aligned   = NEWFS_ROUND_UP((size + bias), NEWFS_BLK_SZ());
    uint8_t* temp_content   = (uint8_t*)malloc(size_aligned);
    uint8_t* cur            = temp_content;
    // lseek(NEWFS_DRIVER(), offset_aligned, SEEK_SET);
    ddriver_seek(NEWFS_DRIVER(), offset_aligned, SEEK_SET);
    while (size_aligned != 0)
    {
        // read(NEWFS_DRIVER(), cur, NEWFS_IO_SZ());
        ddriver_read(NEWFS_DRIVER(), cur, NEWFS_IO_SZ());
        cur          += NEWFS_IO_SZ();
        size_aligned -= NEWFS_IO_SZ();   
    }
    memcpy(out_content, temp_content + bias, size);
    free(temp_content);
    return NEWFS_ERROR_NONE;
}
/**
 * @brief 驱动写
 * 
 * @param offset 
 * @param in_content 
 * @param size 
 * @return int 
 */
int newfs_driver_write(int offset, uint8_t *in_content, int size) {
    int      offset_aligned = NEWFS_ROUND_DOWN(offset, NEWFS_BLK_SZ());
    int      bias           = offset - offset_aligned;
    int      size_aligned   = NEWFS_ROUND_UP((size + bias), NEWFS_BLK_SZ());
    uint8_t* temp_content   = (uint8_t*)malloc(size_aligned);
    uint8_t* cur            = temp_content;
    newfs_driver_read(offset_aligned, temp_content, size_aligned);
    memcpy(temp_content + bias, in_content, size);
    
    // lseek(super.driver_fd, offset_aligned, SEEK_SET);
    ddriver_seek(super.driver_fd, offset_aligned, SEEK_SET);
    while (size_aligned != 0)
    {
        // write(super.driver_fd, cur, NEWFS_IO_SZ());
        ddriver_write(super.driver_fd, cur, NEWFS_IO_SZ());
        cur          += NEWFS_IO_SZ();
        size_aligned -= NEWFS_IO_SZ();   
    }

    free(temp_content);
    return NEWFS_ERROR_NONE;
}
/**
 * @brief 为一个inode分配dentry的bro，采用头插法
 * 
 * @param inode 
 * @param dentry 
 * @return int 
 */
int newfs_alloc_dentry(struct newfs_inode* inode, struct newfs_dentry* dentry) {
    if (inode->dentrys == NULL) {
        inode->dentrys = dentry;
    }
    else {
        dentry->brother = inode->dentrys;
        inode->dentrys = dentry;
    }
    inode->dir_cnt++;
    return inode->dir_cnt;
}
/**
 * @brief 分配一个inode，占用位图
 * 
 * @param dentry 该dentry指向分配的inode
 * @return newfs_inode
 */
struct newfs_inode* newfs_alloc_inode(struct newfs_dentry * dentry) {
    struct newfs_inode* inode;
    int byte_cursor  = 0; 
    int bit_cursor   = 0; 
    int ino_cursor   = 0;
    int bno_cursor   = 0;
    int data_blk_cnt = 0;
    boolean is_find_free_entry = FALSE;
    boolean is_enough_data_blk = FALSE;

    /* 从索引位图中取空闲 */
    for (byte_cursor = 0; byte_cursor < NEWFS_BLKS_SZ(super.map_inode_blks); 
         byte_cursor++)
    {
        for (bit_cursor = 0; bit_cursor < UINT8_BITS; bit_cursor++) {
            if((super.map_inode[byte_cursor] & (0x1 << bit_cursor)) == 0) {    
                                                      /* 当前ino_cursor位置空闲 */
                super.map_inode[byte_cursor] |= (0x1 << bit_cursor);
                is_find_free_entry = TRUE;           
                break;
            }
            ino_cursor++;
        }
        if (is_find_free_entry) {
            break;
        }
    }

    if (!is_find_free_entry || ino_cursor == super.max_ino)
        return -NEWFS_ERROR_NOSPACE;

    /* 先分配一个 inode */
    inode = (struct newfs_inode*)malloc(sizeof(struct newfs_inode));
    inode->ino  = ino_cursor; 
    inode->size = 0;

    /* 从数据位图中取空闲，要取出若干个 */
    for (byte_cursor = 0; byte_cursor < NEWFS_BLKS_SZ(super.map_data_blks); 
         byte_cursor++)
    {
        for (bit_cursor = 0; bit_cursor < UINT8_BITS; bit_cursor++) {
            if((super.map_data[byte_cursor] & (0x1 << bit_cursor)) == 0) {    
                                                      /* 当前ino_cursor位置空闲 */
                super.map_data[byte_cursor] |= (0x1 << bit_cursor);

                /* 找到的块号立刻记入 inode，并判断找够了没 */
                inode->bno[data_blk_cnt++] = bno_cursor;
                if(data_blk_cnt == NEWFS_DATA_PER_FILE){
                    is_enough_data_blk = TRUE;
                    break;
                }
            }
            bno_cursor++;
        }
        if (is_enough_data_blk) {
            break;
        }
    }

    if (!is_enough_data_blk || bno_cursor == super.max_data)
        return -NEWFS_ERROR_NOSPACE;

    /* dentry指向inode */
    dentry->inode = inode;
    dentry->ino   = inode->ino;
    
    /* inode指回dentry */
    inode->dentry = dentry;
    
    inode->dir_cnt = 0;
    inode->dentrys = NULL;
    
    /* 对于文件，还需要预分配 pointer，指向内存中的随机块 */
    if (NEWFS_IS_REG(inode)) {
        int p_count = 0;
        for(p_count = 0; p_count < NEWFS_DATA_PER_FILE; p_count++){
            inode->block_pointer[p_count] = (uint8_t *)malloc(NEWFS_BLK_SZ());
        }
    }
    if (NEWFS_IS_REG(inode)) {
        inode->data = (uint8_t *)malloc(NEWFS_BLKS_SZ(NEWFS_DATA_PER_FILE));
    }

    return inode;
}
/**
 * @brief 将内存inode及其下方结构全部刷回磁盘
 * 
 * @param inode 
 * @return int 
 */
// int newfs_sync_inode(struct newfs_inode * inode) {
//     struct newfs_inode_d  inode_d;
//     struct newfs_dentry*  dentry_cursor;
//     struct newfs_dentry_d dentry_d;
//     int ino             = inode->ino;
//     inode_d.ino         = ino;
//     inode_d.size        = inode->size;
//     inode_d.ftype       = inode->dentry->ftype;
//     inode_d.dir_cnt     = inode->dir_cnt;
//     int blk_cnt = 0;
//     // for(blk_cnt = 0; blk_cnt < NEWFS_DATA_PER_FILE; blk_cnt++)
//     //     inode_d.bno[blk_cnt] = inode->bno[blk_cnt]; /* 数据块的块号也要赋值 */

//     int offset, offset_limit;  /* 用于密集写回 dentry */
    
//     /* inode 非密集写回，间隔一个 BLK */
//     if (newfs_driver_write(NEWFS_INO_OFS(ino), (uint8_t *)&inode_d, 
//                      sizeof(struct newfs_inode_d)) != NEWFS_ERROR_NONE) {
//         NEWFS_DBG("[%s] io error\n", __func__);
//         return -NEWFS_ERROR_IO;
//     }
//                                                       /* Cycle 1: 写 INODE */
//                                                       /* Cycle 2: 写 数据 */
//     if (NEWFS_IS_DIR(inode)) {      
//         blk_cnt = 0;  
//         offset = NEWFS_DATA_OFS(ino);                  
//         dentry_cursor = inode->dentrys;
//         /* dentry 要存满 4 个不连续的 blk 块 */
//         while(dentry_cursor != NULL && blk_cnt < NEWFS_DATA_PER_FILE){
//             offset = NEWFS_DATA_OFS(inode->bno[blk_cnt]); // dentry 从 inode 分配的首个数据块开始存
//             offset_limit = NEWFS_DATA_OFS(inode->bno[blk_cnt] + 1);
//             /* 写满一个 blk 时换到下一个 bno */
//             while (dentry_cursor != NULL)
//             {
//                 memcpy(dentry_d.fname, dentry_cursor->fname, NEWFS_MAX_FILE_NAME);
//                 dentry_d.ftype = dentry_cursor->ftype;
//                 dentry_d.ino = dentry_cursor->ino;
//                 /* dentry 密集写回 */
//                 if (newfs_driver_write(offset, (uint8_t *)&dentry_d, 
//                                     sizeof(struct newfs_dentry_d)) != NEWFS_ERROR_NONE) {
//                     NEWFS_DBG("[%s] io error\n", __func__);
//                     return -NEWFS_ERROR_IO;                     
//                 }
                
//                 if (dentry_cursor->inode != NULL) {
//                     newfs_sync_inode(dentry_cursor->inode); /* 递归 */
//                 }

//                 dentry_cursor = dentry_cursor->brother; /* 深搜 */
//                 offset += sizeof(struct newfs_dentry_d);
//                 if(offset + sizeof(struct newfs_dentry_d) > offset_limit)
//                     break;
//             }
//             blk_cnt++; /* 访问下一个指向的数据块 */
//         }
//     }
//     else if (NEWFS_IS_REG(inode)) {
//         for(blk_cnt = 0; blk_cnt < NEWFS_DATA_PER_FILE; blk_cnt++){
//             if (newfs_driver_write(NEWFS_DATA_OFS(inode->bno[blk_cnt]), 
//                     inode->block_pointer[blk_cnt], NEWFS_BLK_SZ()) != NEWFS_ERROR_NONE) {
//                 NEWFS_DBG("[%s] io error\n", __func__);
//                 return -NEWFS_ERROR_IO;
//             }
//         }
//     }
//     return NEWFS_ERROR_NONE;
// }

int newfs_sync_inode(struct newfs_inode * inode) {
    struct newfs_inode_d  inode_d;
    struct newfs_dentry*  dentry_cursor;
    struct newfs_dentry_d dentry_d;
    int ino             = inode->ino;
    inode_d.ino         = ino;
    inode_d.size        = inode->size;
    memcpy(inode_d.target_path, inode->target_path, NEWFS_MAX_FILE_NAME);
    inode_d.ftype       = inode->dentry->ftype;
    inode_d.dir_cnt     = inode->dir_cnt;
    int offset;
    
    if (newfs_driver_write(NEWFS_INO_OFS(ino), (uint8_t *)&inode_d, 
                     sizeof(struct newfs_inode_d)) != NEWFS_ERROR_NONE) {
        NEWFS_DBG("[%s] io error\n", __func__);
        return -NEWFS_ERROR_IO;
    }
                                                      /* Cycle 1: 写 INODE */
                                                      /* Cycle 2: 写 数据 */
    if (NEWFS_IS_DIR(inode)) {                          
        dentry_cursor = inode->dentrys;
        offset        = NEWFS_DATA_OFS(ino);
        while (dentry_cursor != NULL)
        {
            memcpy(dentry_d.fname, dentry_cursor->fname, NEWFS_MAX_FILE_NAME);
            dentry_d.ftype = dentry_cursor->ftype;
            dentry_d.ino = dentry_cursor->ino;
            if (newfs_driver_write(offset, (uint8_t *)&dentry_d, 
                                 sizeof(struct newfs_dentry_d)) != NEWFS_ERROR_NONE) {
                NEWFS_DBG("[%s] io error\n", __func__);
                return -NEWFS_ERROR_IO;                     
            }
            
            if (dentry_cursor->inode != NULL) {
                newfs_sync_inode(dentry_cursor->inode);
            }

            dentry_cursor = dentry_cursor->brother;
            offset += sizeof(struct newfs_dentry_d);
        }
    }
    else if (NEWFS_IS_REG(inode)) {
        if (newfs_driver_write(NEWFS_DATA_OFS(ino), inode->data, 
                             NEWFS_BLKS_SZ(NEWFS_DATA_PER_FILE)) != NEWFS_ERROR_NONE) {
            NEWFS_DBG("[%s] io error\n", __func__);
            return -NEWFS_ERROR_IO;
        }
    }
    return NEWFS_ERROR_NONE;
}
/**
 * @brief 
 * 
 * @param dentry dentry指向ino，读取该inode
 * @param ino inode唯一编号
 * @return struct newfs_inode* 
 */
// struct newfs_inode* newfs_read_inode(struct newfs_dentry * dentry, int ino) {
//     struct newfs_inode* inode = (struct newfs_inode*)malloc(sizeof(struct newfs_inode));
//     struct newfs_inode_d inode_d;
//     struct newfs_dentry* sub_dentry; /* 指向 子dentry 数组 */
//     struct newfs_dentry_d dentry_d;
//     int    blk_cnt = 0; /* 用于读取多个 bno */
//     int    dir_cnt = 0, offset, offset_limit; /* 用于读取目录项 不连续的 dentrys */

//     if (newfs_driver_read(NEWFS_INO_OFS(ino), (uint8_t *)&inode_d, 
//                         sizeof(struct newfs_inode_d)) != NEWFS_ERROR_NONE) {
//         NEWFS_DBG("[%s] io error\n", __func__);
//         return NULL;
//     }
//     inode->dir_cnt = 0;
//     inode->ino = inode_d.ino;
//     inode->size = inode_d.size;
//     memcpy(inode->target_path, inode_d.target_path, NEWFS_MAX_FILE_NAME);
//     inode->dentry = dentry;     /* 指回父级 dentry*/
//     inode->dentrys = NULL;
//     for(blk_cnt = 0; blk_cnt < NEWFS_DATA_PER_FILE; blk_cnt++)
//         inode->bno[blk_cnt] = inode_d.bno[blk_cnt]; /* 数据块的块号也要赋值 */
    
//     if (NEWFS_IS_DIR(inode)) {
//         dir_cnt = inode_d.dir_cnt;
//         blk_cnt = 0; /* 离散的块号 */
        
//         while(dir_cnt != 0){
//             offset = NEWFS_DATA_OFS(inode->bno[blk_cnt]); // dentry 从 inode 分配的首个数据块开始存
//             offset_limit = NEWFS_DATA_OFS(inode->bno[blk_cnt] + 1);
//             /* 写满一个 blk 时换到下一个 bno */
//             while (offset + sizeof(struct newfs_dentry_d) < offset_limit)
//             {
//                 if (newfs_driver_read(offset, (uint8_t *)&dentry_d, 
//                                     sizeof(struct newfs_dentry_d)) != NEWFS_ERROR_NONE) {
//                     NEWFS_DBG("[%s] io error\n", __func__);
//                     return NULL;                    
//                 }
                
//                 sub_dentry = new_dentry(dentry_d.fname, dentry_d.ftype);
//                 sub_dentry->parent = inode->dentry;
//                 sub_dentry->ino    = dentry_d.ino; 
//                 newfs_alloc_dentry(inode, sub_dentry);

//                 offset += sizeof(struct newfs_dentry_d);
//                 dir_cnt--;
//                 if(dir_cnt == 0) 
//                     break;  /* 减到 0 后提前退出 */
//             }
//             blk_cnt++; /* 访问下一个指向的数据块 */
//         }
//     }
//     else if (NEWFS_IS_REG(inode)) {
//         for(blk_cnt = 0; blk_cnt < NEWFS_DATA_PER_FILE; blk_cnt++){
//             inode->block_pointer[blk_cnt] = (uint8_t *)malloc(NEWFS_BLK_SZ()); /* 只分配一个块 */
//             if (newfs_driver_read(NEWFS_DATA_OFS(inode->bno[blk_cnt]), inode->block_pointer[blk_cnt], 
//                                 NEWFS_BLK_SZ()) != NEWFS_ERROR_NONE) {
//                 NEWFS_DBG("[%s] io error\n", __func__);
//                 return NULL;                    
//             }
//         }
//     }
//     return inode;
// }
struct newfs_inode* newfs_read_inode(struct newfs_dentry * dentry, int ino) {
    struct newfs_inode* inode = (struct newfs_inode*)malloc(sizeof(struct newfs_inode));
    struct newfs_inode_d inode_d;
    struct newfs_dentry* sub_dentry;
    struct newfs_dentry_d dentry_d;
    int    dir_cnt = 0, i;
    if (newfs_driver_read(NEWFS_INO_OFS(ino), (uint8_t *)&inode_d, 
                        sizeof(struct newfs_inode_d)) != NEWFS_ERROR_NONE) {
        NEWFS_DBG("[%s] io error\n", __func__);
        return NULL;                    
    }
    inode->dir_cnt = 0;
    inode->ino = inode_d.ino;
    inode->size = inode_d.size;
    memcpy(inode->target_path, inode_d.target_path, NEWFS_MAX_FILE_NAME);
    inode->dentry = dentry;
    inode->dentrys = NULL;
    if (NEWFS_IS_DIR(inode)) {
        dir_cnt = inode_d.dir_cnt;
        for (i = 0; i < dir_cnt; i++)
        {
            if (newfs_driver_read(NEWFS_DATA_OFS(ino) + i * sizeof(struct newfs_dentry_d), 
                                (uint8_t *)&dentry_d, 
                                sizeof(struct newfs_dentry_d)) != NEWFS_ERROR_NONE) {
                NEWFS_DBG("[%s] io error\n", __func__);
                return NULL;                    
            }
            sub_dentry = new_dentry(dentry_d.fname, dentry_d.ftype);
            sub_dentry->parent = inode->dentry;
            sub_dentry->ino    = dentry_d.ino; 
            newfs_alloc_dentry(inode, sub_dentry);
        }
    }
    else if (NEWFS_IS_REG(inode)) {
        inode->data = (uint8_t *)malloc(NEWFS_BLKS_SZ(NEWFS_DATA_PER_FILE));
        if (newfs_driver_read(NEWFS_DATA_OFS(ino), (uint8_t *)inode->data, 
                            NEWFS_BLKS_SZ(NEWFS_DATA_PER_FILE)) != NEWFS_ERROR_NONE) {
            NEWFS_DBG("[%s] io error\n", __func__);
            return NULL;                    
        }
    }
    return inode;
}
/**
 * @brief 
 * 
 * @param inode 
 * @param dir [0...]
 * @return struct newfs_dentry* 
 */
struct newfs_dentry* newfs_get_dentry(struct newfs_inode * inode, int dir) {
    struct newfs_dentry* dentry_cursor = inode->dentrys;
    int    cnt = 0;
    while (dentry_cursor)
    {
        if (dir == cnt) {
            return dentry_cursor;
        }
        cnt++;
        dentry_cursor = dentry_cursor->brother;
    }
    return NULL;
}
/**
 * @brief 
 * path: /qwe/ad  total_lvl = 2,
 *      1) find /'s inode       lvl = 1
 *      2) find qwe's dentry 
 *      3) find qwe's inode     lvl = 2
 *      4) find ad's dentry
 *
 * path: /qwe     total_lvl = 1,
 *      1) find /'s inode       lvl = 1
 *      2) find qwe's dentry
 * 
 * @param path 
 * @return struct newfs_inode* 
 */
struct newfs_dentry* newfs_lookup(const char * path, boolean* is_find, boolean* is_root) {
    struct newfs_dentry* dentry_cursor = super.root_dentry;
    struct newfs_dentry* dentry_ret = NULL;
    struct newfs_inode*  inode; 
    int   total_lvl = newfs_calc_lvl(path);
    int   lvl = 0;
    boolean is_hit;
    char* fname = NULL;
    char* path_cpy = (char*)malloc(sizeof(path));
    *is_root = FALSE;
    strcpy(path_cpy, path);

    if (total_lvl == 0) {                           /* 根目录 */
        *is_find = TRUE;
        *is_root = TRUE;
        dentry_ret = super.root_dentry;
    }
    fname = strtok(path_cpy, "/");       
    while (fname)
    {   
        lvl++;
        if (dentry_cursor->inode == NULL) {           /* Cache机制，如果没有可能是被换出了 */
            newfs_read_inode(dentry_cursor, dentry_cursor->ino);
        }
        inode = dentry_cursor->inode;

        if (NEWFS_IS_REG(inode) && lvl < total_lvl) {
            NEWFS_DBG("[%s] not a dir\n", __func__);
            dentry_ret = inode->dentry;
            break;
        }
        if (NEWFS_IS_DIR(inode)) {
            dentry_cursor = inode->dentrys;
            is_hit        = FALSE;

            while (dentry_cursor)
            {
                if (memcmp(dentry_cursor->fname, fname, strlen(fname)) == 0) {
                    is_hit = TRUE;
                    break;
                }
                dentry_cursor = dentry_cursor->brother; /* 遍历目录下的子文件 */
            }
            
            if (!is_hit) {
                *is_find = FALSE;
                NEWFS_DBG("[%s] not found %s\n", __func__, fname);
                dentry_ret = inode->dentry;
                break;
            }

            if (is_hit && lvl == total_lvl) {
                *is_find = TRUE;
                dentry_ret = dentry_cursor;
                break;
            }
        }
        fname = strtok(NULL, "/"); 
    }

    if (dentry_ret->inode == NULL) {
        dentry_ret->inode = newfs_read_inode(dentry_ret, dentry_ret->ino);
    }
    
    return dentry_ret;
}
/**
 * @brief 挂载newfs, Layout 如下
 * 
 * Layout
 * | Super | Inode Map | Data Map | Data |
 * 
 *  BLK_SZ = 2 * IO_SZ
 * 
 * 每个Inode占用一个Blk
 * @param options 
 * @return int 
 */
int newfs_mount(struct custom_options options){
    int                 ret = NEWFS_ERROR_NONE;
    int                 driver_fd;
    struct newfs_super_d  newfs_super_d;    /* 临时存放 driver 读出的超级块 */
    struct newfs_dentry*  root_dentry;
    struct newfs_inode*   root_inode;

    int                 super_blks;

    int                 inode_num;
    int                 data_num;
    int                 map_inode_blks;
    int                 map_data_blks;
    
    boolean             is_init = FALSE;

    super.is_mounted = FALSE;

    // driver_fd = open(options.device, O_RDWR);
    driver_fd = ddriver_open(options.device);
    printf("s\n");
    if (driver_fd < 0) {
        return driver_fd;
    }

    super.driver_fd = driver_fd;
    ddriver_ioctl(NEWFS_DRIVER(), IOC_REQ_DEVICE_SIZE,  &super.sz_disk);  /* 请求查看设备大小 4MB */
    ddriver_ioctl(NEWFS_DRIVER(), IOC_REQ_DEVICE_IO_SZ, &super.sz_io);    /* 请求设备IO大小 512B */
    super.sz_blk = super.sz_io * 2;                                 /* EXT2文件系统实际块大小 1024B */

    root_dentry = new_dentry("/", NEWFS_DIR);

    /* 读取 super 到临时空间 */
    if (newfs_driver_read(NEWFS_SUPER_OFS, (uint8_t *)(&newfs_super_d), 
                        sizeof(struct newfs_super_d)) != NEWFS_ERROR_NONE) {
        return -NEWFS_ERROR_IO;
    }   
                                                      /* 读取super */
    if (newfs_super_d.magic_num != NEWFS_MAGIC_NUM) {     /* 幻数无，重建整个磁盘 */
                                                      /* 估算各部分大小 */
        // super_blks = NEWFS_ROUND_UP(sizeof(struct newfs_super_d), NEWFS_BLK_SZ()) / NEWFS_BLK_SZ();  

        // inode_num  =  NEWFS_DISK_SZ() / ((NEWFS_DATA_PER_FILE + NEWFS_INODE_PER_FILE) * NEWFS_IO_SZ());


        // map_inode_blks = NEWFS_ROUND_UP(NEWFS_ROUND_UP(inode_num, UINT32_BITS), NEWFS_IO_SZ()) 
        //                  / NEWFS_IO_SZ();
        
        /* 规定各部分大小 */
        super_blks      = NEWFS_SUPER_BLKS;
        map_inode_blks  = NEWFS_MAP_INODE_BLKS;
        map_data_blks   = NEWFS_MAP_DATA_BLKS;
        inode_num       = NEWFS_INODE_BLKS;
        data_num        = NEWFS_DATA_BLKS;
        // int q=NEWFS_DISK_SZ() / ((NEWFS_DATA_PER_FILE + 1) * NEWFS_IO_SZ());
        // if(q!=15){
        //     return 32323;
        // }
                                                      /* 布局layout */
        // int p=NEWFS_ROUND_UP(NEWFS_ROUND_UP(q, UINT32_BITS), NEWFS_IO_SZ()) 
        //                  / NEWFS_IO_SZ();
        // printf("\n p=%d q=%d \n",p,q);
        // if(p<=10){
        //     return 32333;
        // }
        super.max_ino = inode_num;
        super.max_data = data_num;

        newfs_super_d.magic_num = NEWFS_MAGIC_NUM;
        
        newfs_super_d.map_inode_offset = NEWFS_SUPER_OFS + NEWFS_BLKS_SZ(super_blks);
        newfs_super_d.map_data_offset  = newfs_super_d.map_inode_offset + NEWFS_BLKS_SZ(map_inode_blks);

        newfs_super_d.inode_offset = newfs_super_d.map_data_offset + NEWFS_BLKS_SZ(map_data_blks);
        newfs_super_d.data_offset  = newfs_super_d.inode_offset + NEWFS_BLKS_SZ(inode_num);

        newfs_super_d.map_inode_blks  = map_inode_blks;
        newfs_super_d.map_data_blks   = map_data_blks;

        newfs_super_d.sz_usage        = 0;

        is_init = TRUE;
    }
    super.sz_usage   = newfs_super_d.sz_usage;      /* 建立 in-memory 结构 */
    
    super.map_inode = (uint8_t *)malloc(NEWFS_BLKS_SZ(newfs_super_d.map_inode_blks));
    super.map_inode_blks = newfs_super_d.map_inode_blks;
    super.map_inode_offset = newfs_super_d.map_inode_offset;

    super.map_data = (uint8_t *)malloc(NEWFS_BLKS_SZ(newfs_super_d.map_data_blks));
    super.map_data_blks = newfs_super_d.map_data_blks;
    super.map_data_offset = newfs_super_d.map_data_offset;

    super.inode_offset = newfs_super_d.inode_offset;
    super.data_offset = newfs_super_d.data_offset;

    /* 读取两个位图到内存空间 */
    printf("1\n");
    if (newfs_driver_read(newfs_super_d.map_inode_offset, (uint8_t *)(super.map_inode), 
                        NEWFS_BLKS_SZ(newfs_super_d.map_inode_blks)) != NEWFS_ERROR_NONE) {
        return -NEWFS_ERROR_IO;
    }
    printf("2\n");
    if (newfs_driver_read(newfs_super_d.map_data_offset, (uint8_t *)(super.map_data), 
                        NEWFS_BLKS_SZ(newfs_super_d.map_data_blks)) != NEWFS_ERROR_NONE) {
        return -NEWFS_ERROR_IO;
    }
    printf("3\n");
    if (is_init) {                                    /* 如果进行了重建，则分配根节点 */
        root_inode = newfs_alloc_inode(root_dentry);    
        newfs_sync_inode(root_inode);  /* 将重建后的 根inode 写回磁盘 */
    }
    printf("4\n");
    /* 如果磁盘有数据，则先读入根结点，其他暂时不读 (Cache) */
    root_inode            = newfs_read_inode(root_dentry, NEWFS_ROOT_INO); 
    root_dentry->inode    = root_inode;
    super.root_dentry = root_dentry;
    super.is_mounted  = TRUE;

    newfs_dump_map();
    return ret;
}
void newfs_dump_map() {
    int byte_cursor = 0;
    int bit_cursor = 0;

    for (byte_cursor = 0; byte_cursor < NEWFS_BLKS_SZ(super.map_inode_blks); 
         byte_cursor+=4)
    {
        for (bit_cursor = 0; bit_cursor < UINT8_BITS; bit_cursor++) {
            printf("%d ", (super.map_inode[byte_cursor] & (0x1 << bit_cursor)) >> bit_cursor);   
        }
        printf("\t");

        for (bit_cursor = 0; bit_cursor < UINT8_BITS; bit_cursor++) {
            printf("%d ", (super.map_inode[byte_cursor + 1] & (0x1 << bit_cursor)) >> bit_cursor);   
        }
        printf("\t");
        
        for (bit_cursor = 0; bit_cursor < UINT8_BITS; bit_cursor++) {
            printf("%d ", (super.map_inode[byte_cursor + 2] & (0x1 << bit_cursor)) >> bit_cursor);   
        }
        printf("\t");
        
        for (bit_cursor = 0; bit_cursor < UINT8_BITS; bit_cursor++) {
            printf("%d ", (super.map_inode[byte_cursor + 3] & (0x1 << bit_cursor)) >> bit_cursor);   
        }
        printf("\n");
    }
}
// int newfs_mount(struct custom_options options){
//     int                 ret = NEWFS_ERROR_NONE;
//     int                 driver_fd;
//     struct newfs_super_d  newfs_super_d; 
//     struct newfs_dentry*  root_dentry;
//     struct newfs_inode*   root_inode;

//     int                 inode_num;
//     int                 map_inode_blks;
    
//     int                 super_blks;
//     boolean             is_init = FALSE;

//     super.is_mounted = FALSE;

//     // driver_fd = open(options.device, O_RDWR);
//     driver_fd = ddriver_open(options.device);

//     if (driver_fd < 0) {
//         return driver_fd;
//     }

//     super.driver_fd = driver_fd;
//     ddriver_ioctl(NEWFS_DRIVER(), IOC_REQ_DEVICE_SIZE,  &super.sz_disk);
//     ddriver_ioctl(NEWFS_DRIVER(), IOC_REQ_DEVICE_IO_SZ, &super.sz_io);
    
//     root_dentry = new_dentry("/", NEWFS_DIR);

//     if (newfs_driver_read(NEWFS_SUPER_OFS, (uint8_t *)(&newfs_super_d), 
//                         sizeof(struct newfs_super_d)) != NEWFS_ERROR_NONE) {
//         return -NEWFS_ERROR_IO;
//     }   
//                                                       /* 读取super */
//     if (newfs_super_d.magic_num != NEWFS_MAGIC_NUM) {     /* 幻数无 */
//                                                       /* 估算各部分大小 */
//         super_blks = NEWFS_ROUND_UP(sizeof(struct newfs_super_d), NEWFS_IO_SZ()) / NEWFS_IO_SZ();

//         inode_num  =  NEWFS_DISK_SZ() / ((NEWFS_DATA_PER_FILE + 1) * NEWFS_IO_SZ());

//         map_inode_blks = NEWFS_ROUND_UP(NEWFS_ROUND_UP(inode_num, UINT32_BITS), NEWFS_IO_SZ()) 
//                          / NEWFS_IO_SZ();
//         super.max_ino = inode_num;

//         super.max_data = NEWFS_DATA_BLKS;

//         newfs_super_d.magic_num = NEWFS_MAGIC_NUM;
//         newfs_super_d.map_data_offset  = newfs_super_d.map_inode_offset + NEWFS_BLKS_SZ(map_inode_blks);

//         newfs_super_d.map_data_blks   = NEWFS_MAP_DATA_BLKS;

//         newfs_super_d.sz_usage        = 0;

//         is_init = TRUE;
        
//                                                       /* 布局layout */
//         super.max_ino = (inode_num - super_blks - map_inode_blks); 
//         newfs_super_d.map_inode_offset = NEWFS_SUPER_OFS + NEWFS_BLKS_SZ(super_blks);
//         newfs_super_d.data_offset = newfs_super_d.map_inode_offset + NEWFS_BLKS_SZ(map_inode_blks);
//         newfs_super_d.map_inode_blks  = map_inode_blks;
//         newfs_super_d.sz_usage    = 0;
//         NEWFS_DBG("inode map blocks: %d\n", map_inode_blks);
//         is_init = TRUE;
//     }

    
//     super.map_data = (uint8_t *)malloc(NEWFS_BLKS_SZ(newfs_super_d.map_data_blks));
//     super.map_data_blks = newfs_super_d.map_data_blks;
//     super.map_data_offset = newfs_super_d.map_data_offset;

//     super.inode_offset = newfs_super_d.inode_offset;
//     super.data_offset = newfs_super_d.data_offset;

//     super.sz_usage   = newfs_super_d.sz_usage;      /* 建立 in-memory 结构 */
    
//     super.map_inode = (uint8_t *)malloc(NEWFS_BLKS_SZ(newfs_super_d.map_inode_blks));
//     super.map_inode_blks = newfs_super_d.map_inode_blks;
//     super.map_inode_offset = newfs_super_d.map_inode_offset;
//     super.data_offset = newfs_super_d.data_offset;

//     if (newfs_driver_read(newfs_super_d.map_inode_offset, (uint8_t *)(super.map_inode), 
//                         NEWFS_BLKS_SZ(newfs_super_d.map_inode_blks)) != NEWFS_ERROR_NONE) {
//         return -NEWFS_ERROR_IO;
//     }

//     if (is_init) {                                    /* 分配根节点 */
//         root_inode = newfs_alloc_inode(root_dentry);
//         newfs_sync_inode(root_inode);
//     }
    
//     root_inode            = newfs_read_inode(root_dentry, NEWFS_ROOT_INO);
//     root_dentry->inode    = root_inode;
//     super.root_dentry = root_dentry;
//     super.is_mounted  = TRUE;

//     // newfs_dump_map();
//     return ret;
// }
/**
 * @brief 
 * 
 * @return int 
 */
int newfs_umount() {
    struct newfs_super_d  newfs_super_d; 

    if (!super.is_mounted) {
        return NEWFS_ERROR_NONE;
    }

    newfs_sync_inode(super.root_dentry->inode);     /* 从根节点向下刷写节点 */
                                                    
    newfs_super_d.magic_num           = NEWFS_MAGIC_NUM;
    newfs_super_d.sz_usage            = super.sz_usage;

    newfs_super_d.map_inode_blks      = super.map_inode_blks;
    newfs_super_d.map_inode_offset    = super.map_inode_offset;
    newfs_super_d.map_data_blks       = super.map_data_blks;
    newfs_super_d.map_data_offset     = super.map_data_offset;

    newfs_super_d.inode_offset        = super.inode_offset;
    newfs_super_d.data_offset         = super.data_offset;
    

    if (newfs_driver_write(NEWFS_SUPER_OFS, (uint8_t *)&newfs_super_d, 
                     sizeof(struct newfs_super_d)) != NEWFS_ERROR_NONE) {
        return -NEWFS_ERROR_IO;
    }

    if (newfs_driver_write(newfs_super_d.map_inode_offset, (uint8_t *)(super.map_inode), 
                         NEWFS_BLKS_SZ(newfs_super_d.map_inode_blks)) != NEWFS_ERROR_NONE) {
        return -NEWFS_ERROR_IO;
    }

    // if (newfs_driver_write(newfs_super_d.map_data_offset, (uint8_t *)(super.map_data), 
    //                      NEWFS_BLKS_SZ(newfs_super_d.map_data_blks)) != NEWFS_ERROR_NONE) {
    //     return -NEWFS_ERROR_IO;
    // }

    free(super.map_inode);
    free(super.map_data);
    ddriver_close(NEWFS_DRIVER());

    return NEWFS_ERROR_NONE;
}