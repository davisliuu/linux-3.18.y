#ifndef __STDAFX_H__
#define __STDAFX_H__

/* Function prototypes. */
typedef enum {
	need_more,      /* block not completed,
			 * need more input or more output */
	block_done,     /* block flush performed */
	finish_started, /* finish started, need only more
			 * output at next deflate */
	finish_done     /* finish done, accept no more
			 * input or output */
} block_state;

typedef block_state (*compress_func)(deflate_state *s, int flush);

/* Compression function. Returns the block state after the call. */
typedef struct config_s {
	ush good_length; /* reduce lazy search above this match length */
	ush max_lazy;    /* do not perform lazy search above
			  * this match length */
	ush nice_length; /* quit search above this match length */
	ush max_chain;
	compress_func func;
} config;

#define Freq fc.freq
#define Code fc.code
#define Dad  dl.dad
#define Len  dl.len
#define zmemzero(dest, len) memset(dest, 0, len)

/* rank Z_BLOCK between Z_NO_FLUSH and Z_PARTIAL_FLUSH */
#define RANK(f) (((f) << 1) - ((f) > 4 ? 9 : 0))

/* Output a short LSB first on the stream.
 * IN assertion: there is enough room in pendingBuf.
 */
#define put_short(s, w) { \
	put_byte(s, (uch)((w) & 0xff)); \
	put_byte(s, (uch)((ush)(w) >> 8)); \
}

#ifdef NO_CONFLICT_HASH
static ush hash2[1 << (DEF_MEM_LEVEL+7)];
#endif

/*----------------deflate trees-------------------------------------*/
void bi_windup(deflate_state *s);
void copy_block(deflate_state *s, char *buf, unsigned len, int header);
void pqdownheap(deflate_state *s, ct_data *tree, int k);
void gen_bitlen(deflate_state *s, tree_desc *desc);
void gen_codes(ct_data *tree, int max_code, ush *bl_count);
void build_tree(deflate_state *s, tree_desc *desc);
void scan_tree(deflate_state *s, ct_data *tree, int max_code);
int build_bl_tree(deflate_state *s);
void compress_block(deflate_state *s, const ct_data *ltree,
		const ct_data *dtree);
void send_all_trees(deflate_state *s, int lcodes,
		int dcodes, int blcodes);

void  init_block(deflate_state *s);
void  _tr_init(deflate_state *s);
void  _tr_flush_block(deflate_state *s, char *buf,
		unsigned long stored_len, int last);
void  bi_flush(deflate_state *s);
void  _tr_flush_bits(deflate_state *s);
void  _tr_align(deflate_state *s);
void  _tr_stored_block(deflate_state *s, char *buf,
		unsigned long stored_len, int last);

/*Initialize the hash table (avoiding 64K overflow for 16 bit systems).
 * prev[] will be initialized on the fly.
 */
#define CLEAR_HASH(s) \
	do {\
		s->head[s->hash_size-1] = NIL; \
		zmemzero((Byte *)s->head, \
		(unsigned)(s->hash_size-1) * sizeof(*s->head));\
	} while (0)

void load_lz_mid_inf(z_streamp_gz strm, struct def_lz_rm *drm);
void save_lz_mid_inf(z_streamp_gz strm, struct def_lz_rm *drm);
void get_lz_result(deflate_state *s, int last);
void getLzResult(const defalte_interface *inter, unsigned int *lz_result,
		struct def_lz_rm *drm, int task_flag);
int  getBufDatas(deflate_state *s, unsigned int *lz_result,
		struct def_lz_rm *drm, struct def_huf_rm *hrm);

void lm_init(deflate_state *s);
int ZEXPORT deflateResetKeep(z_streamp_gz strm);
int ZEXPORT deflateReset(z_streamp_gz strm);
void flush_pending(z_streamp_gz strm);
block_state deflate_slow(deflate_state *s, int flush);
int read_buf(z_streamp_gz strm, Byte *buf, unsigned size);
void fill_window(deflate_state *s);
uInt longest_match(deflate_state *s, unsigned cur_match);

int ZEXPORT deflateInit_(z_streamp_gz strm, int level,
		const char *version, int stream_size);
int ZEXPORT deflateInit2_(z_streamp_gz strm, int level, int method,
		int windowBits, int memLevel,
		int strategy, const char *version,
		int stream_size);

#define deflateInit(strm, level) \
	deflateInit_((strm), (level), ZLIB_VERSION, (int)sizeof(z_stream_gz))
#define deflateInit2(strm, level, method, windowBits, memLevel, strategy) \
	deflateInit2_((strm), (level), (method), (windowBits), (memLevel),\
			(strategy), ZLIB_VERSION, (int)sizeof(z_stream_gz))

int ZEXPORT deflate_lz(z_streamp_gz strm, int flush);
int ZEXPORT deflate_huf_block(unsigned char *in, unsigned int *lz_result,
		struct def_lz_rm *drm, struct def_huf_rm *hrm);

int ZEXPORT deflateEnd(z_streamp_gz strm);

/*use the method of finding the most similar paragraph
 *now realise the most simple method:consider the last
 *paragraph as the most similar paragraph
 */
block_state deflate_replace(deflate_state *s);

/*use the method of finding the most similar paragraph
 *now realise a simple method:consider the last paragraph
 *as the most similar paragraph;and compare every field
 */
block_state deflate_replace1(deflate_state *s);
/*return:0.SUCCESS or 1.ERROR*/
/*the available input file size is considered as 64M,
  but can send part of it and hold on the result*/
/*task_flag:0,begin;1,middle;2,end;*/
int def_lz(unsigned char *in, unsigned int in_file_len,
		unsigned int *lz_result, struct def_lz_rm *drm,
		int cprLevel, int task_flag);
/*return:0.SUCCESS or 1.ERROR or 2.Task SUCCESS or 3.LAST BLOCK ERROR*/
int def_huf_block(unsigned char *in, unsigned int *lz_result,
		struct def_lz_rm *drm, struct def_huf_rm *hrm);

/*---------------inflate functions-------------*/
int ZEXPORT inflateResetKeep(z_streamp_gz strm);
int ZEXPORT inflateReset(z_streamp_gz strm);
int ZEXPORT inflateReset2(z_streamp_gz strm, int windowBits);
void fixedtables(struct inflate_state *state);
int translate_code(code codes, unsigned int *code_len,
		unsigned int *extra_len, unsigned int *value);
int translate_code_table(const code *start_table,
		const code *end_table, Byte *result_table);
int translate_lens_table(const code *start_table,
		const code *end_table, unsigned int *result_table);
int translate_dist_table(const code *start_table,
		const code *end_table, unsigned int *result_table);
void translate_static_lens_table(const code *len_table,
		unsigned int *result_table);
void translate_static_dist_table(const code *dist_table,
		unsigned int *result_table);
int inflate_table(codetype type, unsigned short *lens,
		unsigned codes, code **table,
		unsigned *bits, unsigned short *work);
int updatewindow(z_streamp_gz strm, const Byte *end, unsigned copy);

int ZEXPORT inflateInit_(z_streamp_gz strm,
		const char *version, int stream_size);
int ZEXPORT inflateInit2_(z_streamp_gz strm, int  windowBits,
		const char *version, int stream_size);
#define inflateInit(strm) inflateInit_((strm), ZLIB_VERSION, \
		(int)sizeof(z_stream_gz))
#define inflateInit2(strm, windowBits) inflateInit2_((strm), \
		(windowBits), ZLIB_VERSION,\
		(int)sizeof(z_stream_gz))

int ZEXPORT inflate(z_streamp_gz strm, int flush);
int inflateEnd(z_streamp_gz zcpr);

#endif /*__STDAFX_H__*/
