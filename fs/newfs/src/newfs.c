#include "newfs.h"

/******************************************************************************
* SECTION: 瀹忓畾涔�
*******************************************************************************/
#define OPTION(t, p)        { t, offsetof(struct custom_options, p), 1 }

/******************************************************************************
* SECTION: 鍏ㄥ眬鍙橀噺
*******************************************************************************/
static const struct fuse_opt option_spec[] = {		/* 鐢ㄤ簬FUSE鏂囦欢绯荤粺瑙ｆ瀽鍙傛暟 */
	OPTION("--device=%s", device),
	FUSE_OPT_END
};

struct custom_options newfs_options;			 /* 鍏ㄥ眬閫夐」 */
struct newfs_super super; 
/******************************************************************************
* SECTION: FUSE鎿嶄綔瀹氫箟
*******************************************************************************/
static struct fuse_operations operations = {
	.init = newfs_init,						 /* mount鏂囦欢绯荤粺 */		
	.destroy = newfs_destroy,				 /* umount鏂囦欢绯荤粺 */
	.mkdir = newfs_mkdir,					 /* 寤虹洰褰曪紝mkdir */
	.getattr = newfs_getattr,				 /* 鑾峰彇鏂囦欢灞炴€э紝绫讳技stat锛屽繀椤诲畬鎴� */
	.readdir = newfs_readdir,				 /* 濉厖dentrys */
	.mknod = newfs_mknod,					 /* 鍒涘缓鏂囦欢锛宼ouch鐩稿叧 */
	.write = NULL,								  	 /* 鍐欏叆鏂囦欢 */
	.read = NULL,								  	 /* 璇绘枃浠� */
	.utimens = newfs_utimens,				 /* 淇敼鏃堕棿锛屽拷鐣ワ紝閬垮厤touch鎶ラ敊 */
	.truncate = NULL,						  		 /* 鏀瑰彉鏂囦欢澶у皬 */
	.unlink = NULL,							  		 /* 鍒犻櫎鏂囦欢 */
	.rmdir	= NULL,							  		 /* 鍒犻櫎鐩綍锛� rm -r */
	.rename = NULL,							  		 /* 閲嶅懡鍚嶏紝mv */

	.open = NULL,							
	.opendir = NULL,
	.access = NULL
};
/******************************************************************************
* SECTION: 蹇呭仛鍑芥暟瀹炵幇
*******************************************************************************/
/**
 * @brief 鎸傝浇锛坢ount锛夋枃浠剁郴缁�
 * 
 * @param conn_info 鍙拷鐣ワ紝涓€浜涘缓绔嬭繛鎺ョ浉鍏崇殑淇℃伅 
 * @return void*
 */
void* newfs_init(struct fuse_conn_info * conn_info) {
	/* TODO: 鍦ㄨ繖閲岃繘琛屾寕杞� */
	// newfs?
	// SFS_ERROR_NONE=0
	// 涓轰粈涔堣繖涓笢瑗挎槸鍙互鐢ㄧ殑鍟婏紝涓嶈瑕佽嚜宸卞啓锛�
    if (newfs_mount(newfs_options) != 0) {
		// SFS_DBG("[%s] mount error\n", __func__);
		printf("init error\n");
		while(1);
		// 杩欐槸涓簱鍚э紝搴旇鑳界敤
		fuse_exit(fuse_get_context()->fuse);
		return NULL;
	} 
	/* 涓嬮潰鏄竴涓帶鍒惰澶囩殑绀轰緥 */
	super.fd = ddriver_open(newfs_options.device);
	
	return NULL;
}

/**
 * @brief 鍗歌浇锛坲mount锛夋枃浠剁郴缁�
 * 
 * @param p 鍙拷鐣�
 * @return void
 */
void newfs_destroy(void* p) {
	// 鍚屼笂闈㈤偅涓紵
	if (sfs_umount() != 0) {
		printf("destory error\n");
		while(1);
		fuse_exit(fuse_get_context()->fuse);
		return;
	}
	/* TODO: 鍦ㄨ繖閲岃繘琛屽嵏杞� */
	ddriver_close(super.fd);

	return;
}

/**
 * @brief 鍒涘缓鐩綍
 * 
 * @param path 鐩稿浜庢寕杞界偣鐨勮矾寰�
 * @param mode 鍒涘缓妯″紡锛堝彧璇伙紵鍙啓锛燂級锛屽彲蹇界暐
 * @return int 0鎴愬姛锛屽惁鍒欏け璐�
 */
int newfs_mkdir(const char* path, mode_t mode) {
	(void)mode;
	// bool is_find, is_root
	int is_find, is_root;
	char* fname;
	struct sfs_dentry* last_dentry = sfs_lookup(path, &is_find, &is_root);
	struct sfs_dentry* dentry;
	struct sfs_inode*  inode;

	if (is_find) {
		// SFS_ERROR_EXISTS=EEXIST=17
		return -17;
	}

	if (SFS_IS_REG(last_dentry->inode)) {
		// SFS_ERROR_UNSUPPORTED=ENXIO = 6
		return -6;
	}

	fname  = sfs_get_fname(path);
	dentry = new_dentry(fname, SFS_DIR); 
	dentry->parent = last_dentry;
	inode  = sfs_alloc_inode(dentry);
	sfs_alloc_dentry(last_dentry->inode, dentry);
	
	// return SFS_ERROR_NONE;
	/* TODO: 瑙ｆ瀽璺緞锛屽垱寤虹洰褰� */
	return 0;
}

/**
 * @brief 鑾峰彇鏂囦欢鎴栫洰褰曠殑灞炴€э紝璇ュ嚱鏁伴潪甯搁噸瑕�
 * 
 * @param path 鐩稿浜庢寕杞界偣鐨勮矾寰�
 * @param newfs_stat 杩斿洖鐘舵€�
 * @return int 0鎴愬姛锛屽惁鍒欏け璐�
 */
int newfs_getattr(const char* path, struct stat * newfs_stat) {
	/* TODO: 瑙ｆ瀽璺緞锛岃幏鍙朓node锛屽～鍏卬ewfs_stat锛屽彲鍙傝€�/fs/simplefs/sfs.c鐨剆fs_getattr()鍑芥暟瀹炵幇 */
	return 0;
}

/**
 * @brief 閬嶅巻鐩綍椤癸紝濉厖鑷砨uf锛屽苟浜ょ粰FUSE杈撳嚭
 * 
 * @param path 鐩稿浜庢寕杞界偣鐨勮矾寰�
 * @param buf 杈撳嚭buffer
 * @param filler 鍙傛暟璁茶В:
 * 
 * typedef int (*fuse_fill_dir_t) (void *buf, const char *name,
 *				const struct stat *stbuf, off_t off)
 * buf: name浼氳澶嶅埗鍒癰uf涓�
 * name: dentry鍚嶅瓧
 * stbuf: 鏂囦欢鐘舵€侊紝鍙拷鐣�
 * off: 涓嬩竴娆ffset浠庡摢閲屽紑濮嬶紝杩欓噷鍙互鐞嗚В涓虹鍑犱釜dentry
 * 
 * @param offset 绗嚑涓洰褰曢」锛�
 * @param fi 鍙拷鐣�
 * @return int 0鎴愬姛锛屽惁鍒欏け璐�
 */
int newfs_readdir(const char * path, void * buf, fuse_fill_dir_t filler, off_t offset,
			    		 struct fuse_file_info * fi) {
    /* TODO: 瑙ｆ瀽璺緞锛岃幏鍙栫洰褰曠殑Inode锛屽苟璇诲彇鐩綍椤癸紝鍒╃敤filler濉厖鍒癰uf锛屽彲鍙傝€�/fs/simplefs/sfs.c鐨剆fs_readdir()鍑芥暟瀹炵幇 */
    return 0;
}

/**
 * @brief 鍒涘缓鏂囦欢
 * 
 * @param path 鐩稿浜庢寕杞界偣鐨勮矾寰�
 * @param mode 鍒涘缓鏂囦欢鐨勬ā寮忥紝鍙拷鐣�
 * @param dev 璁惧绫诲瀷锛屽彲蹇界暐
 * @return int 0鎴愬姛锛屽惁鍒欏け璐�
 */
int newfs_mknod(const char* path, mode_t mode, dev_t dev) {
	/* TODO: 瑙ｆ瀽璺緞锛屽苟鍒涘缓鐩稿簲鐨勬枃浠� */
	return 0;
}

/**
 * @brief 淇敼鏃堕棿锛屼负浜嗕笉璁﹖ouch鎶ラ敊 
 * 
 * @param path 鐩稿浜庢寕杞界偣鐨勮矾寰�
 * @param tv 瀹炶返
 * @return int 0鎴愬姛锛屽惁鍒欏け璐�
 */
int newfs_utimens(const char* path, const struct timespec tv[2]) {
	(void)path;
	return 0;
}
/******************************************************************************
* SECTION: 閫夊仛鍑芥暟瀹炵幇
*******************************************************************************/
/**
 * @brief 鍐欏叆鏂囦欢
 * 
 * @param path 鐩稿浜庢寕杞界偣鐨勮矾寰�
 * @param buf 鍐欏叆鐨勫唴瀹�
 * @param size 鍐欏叆鐨勫瓧鑺傛暟
 * @param offset 鐩稿鏂囦欢鐨勫亸绉�
 * @param fi 鍙拷鐣�
 * @return int 鍐欏叆澶у皬
 */
int newfs_write(const char* path, const char* buf, size_t size, off_t offset,
		        struct fuse_file_info* fi) {
	/* 閫夊仛 */
	return size;
}

/**
 * @brief 璇诲彇鏂囦欢
 * 
 * @param path 鐩稿浜庢寕杞界偣鐨勮矾寰�
 * @param buf 璇诲彇鐨勫唴瀹�
 * @param size 璇诲彇鐨勫瓧鑺傛暟
 * @param offset 鐩稿鏂囦欢鐨勫亸绉�
 * @param fi 鍙拷鐣�
 * @return int 璇诲彇澶у皬
 */
int newfs_read(const char* path, char* buf, size_t size, off_t offset,
		       struct fuse_file_info* fi) {
	/* 閫夊仛 */
	return size;			   
}

/**
 * @brief 鍒犻櫎鏂囦欢
 * 
 * @param path 鐩稿浜庢寕杞界偣鐨勮矾寰�
 * @return int 0鎴愬姛锛屽惁鍒欏け璐�
 */
int newfs_unlink(const char* path) {
	/* 閫夊仛 */
	return 0;
}

/**
 * @brief 鍒犻櫎鐩綍
 * 
 * 涓€涓彲鑳界殑鍒犻櫎鐩綍鎿嶄綔濡備笅锛�
 * rm ./tests/mnt/j/ -r
 *  1) Step 1. rm ./tests/mnt/j/j
 *  2) Step 2. rm ./tests/mnt/j
 * 鍗筹紝鍏堝垹闄ゆ渶娣卞眰鐨勬枃浠讹紝鍐嶅垹闄ょ洰褰曟枃浠舵湰韬�
 * 
 * @param path 鐩稿浜庢寕杞界偣鐨勮矾寰�
 * @return int 0鎴愬姛锛屽惁鍒欏け璐�
 */
int newfs_rmdir(const char* path) {
	/* 閫夊仛 */
	return 0;
}

/**
 * @brief 閲嶅懡鍚嶆枃浠� 
 * 
 * @param from 婧愭枃浠惰矾寰�
 * @param to 鐩爣鏂囦欢璺緞
 * @return int 0鎴愬姛锛屽惁鍒欏け璐�
 */
int newfs_rename(const char* from, const char* to) {
	/* 閫夊仛 */
	return 0;
}

/**
 * @brief 鎵撳紑鏂囦欢锛屽彲浠ュ湪杩欓噷缁存姢fi鐨勪俊鎭紝渚嬪锛宖i->fh鍙互鐞嗚В涓轰竴涓�64浣嶆寚閽堬紝鍙互鎶婅嚜宸辨兂淇濆瓨鐨勬暟鎹粨鏋�
 * 淇濆瓨鍦╢h涓�
 * 
 * @param path 鐩稿浜庢寕杞界偣鐨勮矾寰�
 * @param fi 鏂囦欢淇℃伅
 * @return int 0鎴愬姛锛屽惁鍒欏け璐�
 */
int newfs_open(const char* path, struct fuse_file_info* fi) {
	/* 閫夊仛 */
	return 0;
}

/**
 * @brief 鎵撳紑鐩綍鏂囦欢
 * 
 * @param path 鐩稿浜庢寕杞界偣鐨勮矾寰�
 * @param fi 鏂囦欢淇℃伅
 * @return int 0鎴愬姛锛屽惁鍒欏け璐�
 */
int newfs_opendir(const char* path, struct fuse_file_info* fi) {
	/* 閫夊仛 */
	return 0;
}

/**
 * @brief 鏀瑰彉鏂囦欢澶у皬
 * 
 * @param path 鐩稿浜庢寕杞界偣鐨勮矾寰�
 * @param offset 鏀瑰彉鍚庢枃浠跺ぇ灏�
 * @return int 0鎴愬姛锛屽惁鍒欏け璐�
 */
int newfs_truncate(const char* path, off_t offset) {
	/* 閫夊仛 */
	return 0;
}


/**
 * @brief 璁块棶鏂囦欢锛屽洜涓鸿鍐欐枃浠舵椂闇€瑕佹煡鐪嬫潈闄�
 * 
 * @param path 鐩稿浜庢寕杞界偣鐨勮矾寰�
 * @param type 璁块棶绫诲埆
 * R_OK: Test for read permission. 
 * W_OK: Test for write permission.
 * X_OK: Test for execute permission.
 * F_OK: Test for existence. 
 * 
 * @return int 0鎴愬姛锛屽惁鍒欏け璐�
 */
int newfs_access(const char* path, int type) {
	/* 閫夊仛: 瑙ｆ瀽璺緞锛屽垽鏂槸鍚﹀瓨鍦� */
	return 0;
}	
/******************************************************************************
* SECTION: FUSE鍏ュ彛
*******************************************************************************/
int main(int argc, char **argv)
{
    int ret;
	printf("test\n")
	struct fuse_args args = FUSE_ARGS_INIT(argc, argv);

	newfs_options.device = strdup("TODO: 杩欓噷濉啓浣犵殑ddriver璁惧璺緞");

	if (fuse_opt_parse(&args, &newfs_options, option_spec, NULL) == -1)
		return -1;
	
	ret = fuse_main(args.argc, args.argv, &operations, NULL);
	fuse_opt_free_args(&args);
	return ret;
}