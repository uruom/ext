#include <newfs.h>


/**
 * @brief 閿熸枻鎷烽敓鏂ゆ嫹sfs, Layout 閿熸枻鎷烽敓鏂ゆ嫹
 * 
 * Layout
 * | Super | Inode Map | Data |
 * 
 * IO_SZ = BLK_SZ
 * 
 * 姣忛敓鏂ゆ嫹Inode鍗犻敓鏂ゆ嫹涓€閿熸枻鎷稡lk
 * @param options 
 * @return int 
 */
int newfs_mount(struct custom_options options){
    int                 ret = 0;
    int                 driver_fd;
    struct newfs_super_d  newfs_super_d; 
    struct newfs_dentry*  root_dentry;
    struct newfs_inode*   root_inode;

    int                 inode_num;
    int                 map_inode_blks;
    
    int                 super_blks;
    // boolean             is_init = FALSE;
    // 0=false,1=true
    int                 is_init = 0;


    super.is_mounted = 0;

    // driver_fd = open(options.device, O_RDWR);
    // ddriver_open 閿熸枻鎷烽敓鏂ゆ嫹閿熸枻鎷烽敓鎺ワ讣鎷疯矊閿熸枻鎷烽敓鏂ゆ嫹閿熷彨锟�
    driver_fd = ddriver_open(options.device);

    if (driver_fd < 0) {
        return driver_fd;
    }

    super.driver_fd = driver_fd;
    // 閿熸澃鐧告嫹閿熸枻鎷疯浆閿熸枻鎷烽敓鏂ゆ嫹define閿熸枻鎷峰簲閿熸枻鎷锋病閿熸枻鎷烽敓鏂ゆ嫹
    ddriver_ioctl(SFS_DRIVER(), NEW_IOC_REQ_DEVICE_SIZE,  &super.sz_disk);
    ddriver_ioctl(SFS_DRIVER(), NEW_IOC_REQ_DEVICE_IO_SZ, &super.sz_io);
    
    root_dentry = new_dentry("/", NEWFS_DIR);

    if (sfs_driver_read(NEWFS_SUPER_OFS, (uint8_t *)(&newfs_super_d), 
                        sizeof(struct newfs_super_d)) != NEWFS_ERROR_NONE) {
        return -NEWFS_ERROR_IO;
    }   
                                                      /* 閿熸枻鎷峰彇super */
    if (newfs_super_d.magic_num != NEWFS_MAGIC_NUM) {     /* 閿熸枻鎷烽敓鏂ゆ嫹閿熸枻鎷� */
                                                      /* 閿熸枻鎷烽敓鏂ゆ嫹閿熸枻鎷烽敓鏂ゆ嫹鎵ч敓鍙拷 */
        super_blks = NEWFS_ROUND_UP(sizeof(struct newfs_super_d), NEWFS_IO_SZ()) / super.io_sz;// SFS_IO_SZ();

        // inode_num  =  SFS_DISK_SZ() / ((SFS_DATA_PER_FILE + SFS_INODE_PER_FILE) * SFS_IO_SZ());
        inode_num  =  super.disk_sz / ((NEWFS_DATA_PER_FILE + NEWFS_INODE_PER_FILE) * super.io_sz);
        
        map_inode_blks = NEWFS_ROUND_UP(NEWFS_ROUND_UP(inode_num, UINT32_BITS), super.io_sz)//SFS_IO_SZ() 
                         / super.io_sz;//SFS_IO_SZ()
        
                                                      /* 閿熸枻鎷烽敓鏂ゆ嫹layout */
        super.max_ino = (inode_num - super_blks - map_inode_blks); 
        newfs_super_d.map_inode_offset = NEWFS_SUPER_OFS + NEWFS_BLKS_SZ(super_blks);
        newfs_super_d.data_offset = newfs_super_d.map_inode_offset + NEWFS_BLKS_SZ(map_inode_blks);
        newfs_super_d.map_inode_blks  = map_inode_blks;
        newfs_super_d.sz_usage    = 0;
        printf("magic running\n");
        // SFS_DBG("inode map blocks: %d\n", map_inode_blks);
        is_init = 1;
    }
    super.sz_usage   = newfs_super_d.sz_usage;      /* 閿熸枻鎷烽敓鏂ゆ嫹 in-memory 閿熺粨鏋� */
    
    super.map_inode = (uint8_t *)malloc(SFS_BLKS_SZ(newfs_super_d.map_inode_blks));
    super.map_inode_blks = newfs_super_d.map_inode_blks;
    super.map_inode_offset = newfs_super_d.map_inode_offset;
    super.data_offset = newfs_super_d.data_offset;

    if (newfs_driver_read(newfs_super_d.map_inode_offset, (uint8_t *)(super.map_inode), 
                        NEWFS_BLKS_SZ(newfs_super_d.map_inode_blks)) != NEWFS_ERROR_NONE) {
        return -NEWFS_ERROR_IO;
    }

    if (is_init) {                                    /* 閿熸枻鎷烽敓鏂ゆ嫹閿熸枻鎷疯瘶閿燂拷 */
        root_inode = newfs_alloc_inode(root_dentry);
        sfs_sync_inode(root_inode);
    }
    
    root_inode            = sfs_read_inode(root_dentry, NEWFS_ROOT_INO);
    root_dentry->inode    = root_inode;
    super.root_dentry = root_dentry;
    super.is_mounted  = 1;

    sfs_dump_map();
    return ret;
}

int newfs_driver_read(int offset, uint8_t *out_content, int size) {
    int      offset_aligned = NEWFS_ROUND_DOWN(offset, NEWFS_IO_SZ());
    int      bias           = offset - offset_aligned;
    int      size_aligned   = NEWFS_ROUND_UP((size + bias), NEWFS_IO_SZ());
    uint8_t* temp_content   = (uint8_t*)malloc(size_aligned);
    uint8_t* cur            = temp_content;
    // lseek(SFS_DRIVER(), offset_aligned, SEEK_SET);
    ddriver_seek(NEWFS_DRIVER(), offset_aligned, SEEK_SET);
    while (size_aligned != 0)
    {
        // read(SFS_DRIVER(), cur, SFS_IO_SZ());
        ddriver_read(NEWFS_DRIVER(), cur, NEWFS_IO_SZ());
        cur          += NEWFS_IO_SZ();
        size_aligned -= NEWFS_IO_SZ();   
    }
    memcpy(out_content, temp_content + bias, size);
    free(temp_content);
    return NEWFS_ERROR_NONE;
}

struct newfs_inode* newfs_alloc_inode(struct newfs_dentry * dentry) {
    struct newfs_inode* inode;
    int byte_cursor = 0; 
    int bit_cursor  = 0; 
    int ino_cursor  = 0;
    // 0=false,1=true
    int is_find_free_entry = 0;

    for (byte_cursor = 0; byte_cursor < NEWFS_BLKS_SZ(super.map_inode_blks); 
         byte_cursor++)
    {
        for (bit_cursor = 0; bit_cursor < UINT8_BITS; bit_cursor++) {
            if((super.map_inode[byte_cursor] & (0x1 << bit_cursor)) == 0) {    
                                                      /* 瑜版挸澧爄no_cursor娴ｅ秶鐤嗙粚娲＝ */
                super.map_inode[byte_cursor] |= (0x1 << bit_cursor);
                is_find_free_entry = 1;           
                break;
            }
            ino_cursor++;
        }
        if (is_find_free_entry==1) {
            break;
        }
    }

    if (is_find_free_entry==0 || ino_cursor == super.max_ino)
        return -NEWFS_ERROR_NOSPACE;

    inode = (struct newfs_inode*)malloc(sizeof(struct newfs_inode));
    inode->ino  = ino_cursor; 
    inode->size = 0;
                                                      /* dentry閹稿洤鎮渋node */
    dentry->inode = inode;
    dentry->ino   = inode->ino;
                                                      /* inode閹稿洤娲杁entry */
    inode->dentry = dentry;
    
    inode->dir_cnt = 0;
    inode->dentrys = NULL;
    
    if (NEWFS_IS_REG(inode)) {
        inode->data = (uint8_t *)malloc(NEWFS_BLKS_SZ(NEWFS_DATA_PER_FILE));
    }

    return inode;
}

int newfs_sync_inode(struct newfs_inode * inode) {
    struct newfs_inode_d  inode_d;
    struct newfs_dentry*  dentry_cursor;
    struct newfs_dentry_d dentry_d;
    int ino             = inode->ino;
    inode_d.ino         = ino;
    inode_d.size        = inode->size;
    inode_d.ftype       = inode->dentry->ftype;
    inode_d.dir_cnt     = inode->dir_cnt;
    int offset;
    
    if (sfs_driver_write(SFS_INO_OFS(ino), (uint8_t *)&inode_d, 
                     sizeof(struct sfs_inode_d)) != SFS_ERROR_NONE) {
        SFS_DBG("[%s] io error\n", __func__);
        return -SFS_ERROR_IO;
    }
                                                      /* Cycle 1: 闁告劧鎷� INODE */
                                                      /* Cycle 2: 闁告劧鎷� 闁轰胶澧楀畵锟� */
    if (SFS_IS_DIR(inode)) {                          
        dentry_cursor = inode->dentrys;
        offset        = SFS_DATA_OFS(ino);
        while (dentry_cursor != NULL)
        {
            memcpy(dentry_d.fname, dentry_cursor->fname, SFS_MAX_FILE_NAME);
            dentry_d.ftype = dentry_cursor->ftype;
            dentry_d.ino = dentry_cursor->ino;
            if (sfs_driver_write(offset, (uint8_t *)&dentry_d, 
                                 sizeof(struct sfs_dentry_d)) != SFS_ERROR_NONE) {
                SFS_DBG("[%s] io error\n", __func__);
                return -SFS_ERROR_IO;                     
            }
            
            if (dentry_cursor->inode != NULL) {
                sfs_sync_inode(dentry_cursor->inode);
            }
