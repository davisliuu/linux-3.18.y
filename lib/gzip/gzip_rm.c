#include <linux/mm.h>
#include <linux/unistd.h>
#include <linux/slab.h>

#include "global.h"
#include "crc32.h"
#include "stdafx.h"

ush hash_size;
uInt DEPTH;

#define GZIP_HEAD_LEN	10
unsigned char gzip_head[GZIP_HEAD_LEN]={0x1f, 0x8b, 0x08, 0x00, 0, 0, 0, 0, 0, 0x03};

static int total_alloc_mem_size = 0;
void * gzip_alloc(unsigned int size)
{
	total_alloc_mem_size += size;
	return kmalloc(size, GFP_KERNEL);
}

int gzip_defalte(void *dst, unsigned int *dst_len, const void *src, unsigned int len)
{
	unsigned int i;
	int cprLevel=6;
	int ret=Z_OK;
	unsigned char* CprPtr;
	uInt *lz_result;
	struct def_lz_rm  *drm;
	struct def_huf_rm *hrm;

	drm = (struct def_lz_rm  *)gzip_alloc(sizeof(struct def_lz_rm));
	if (NULL == drm) {
		printk("alloc drm failed!");
		return -1;
	}

	hrm = (struct def_huf_rm *)gzip_alloc(sizeof(struct def_huf_rm));
	if (NULL == hrm) {
		printk("alloc hrm failed!");
		return -1;
	}

	/* Add gzip header*/
	memcpy(dst, gzip_head, GZIP_HEAD_LEN);
	CprPtr = (unsigned char *)dst + GZIP_HEAD_LEN;

	memset(drm, 0, sizeof(struct def_lz_rm));
	memset(hrm,0,sizeof(struct def_huf_rm));

	lz_result =  (uInt *) gzip_alloc(len * sizeof(uInt));
	if (NULL == lz_result) {
		printk("kmalloc lz_result failed!\n");
		return -1;
	}

	memset(lz_result, 0, len);

	if (!src || !dst) {
		printk("src or dst is NULL!\n");
		return -1;
	}

	//lz deflate
	ret = def_lz((unsigned char *)src, len, lz_result, drm, cprLevel, 2);


	if(ret != 0){
		printk("ERROR:deflate lz\n");
		return 1;
	}

	while(ret == 0){
		ret = def_huf_block((unsigned char *)src, lz_result, drm, hrm);
		for(i = 0; i < hrm->def_out_lens; ++i) {
			CprPtr[i+hrm->total_out-hrm->def_out_lens] = hrm->def_out[i];
		}
	}

	*dst_len = hrm->total_out + GZIP_HEAD_LEN;

	kfree(lz_result);
	kfree(drm);
	kfree(hrm);

	total_alloc_mem_size = 0;

	if(ret != 2)
	{
		return 1;
	}

	return 0;
}
