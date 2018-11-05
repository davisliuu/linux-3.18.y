#include "global.h"
#include "trees.h"
#include "crc32.h"
#include "stdafx.h"
#include <linux/slab.h>
#include <linux/vmalloc.h>

voidpf zcalloc(voidpf opaque, unsigned items,
		unsigned size)
{
	if (opaque) items += size - size; /* make compiler happy */
	return (voidpf)gzip_alloc(items * size);
}

void  zcfree(voidpf opaque, voidpf ptr)
{
	kfree(ptr);
	if (opaque) return; /* make compiler happy */
}

extern ush hash_size;
extern uInt DEPTH;
#define ZALLOC(strm,items,size) (*((strm)->zalloc))((strm)->opaque, (items), (size));
#define ZFREE(strm, addr) (*((strm)->zfree))((strm)->opaque, (voidpf)(addr));

unsigned long crc32(unsigned long crc, const unsigned char *buf, unsigned int len)
{
	if (buf == Z_NULL)
		return 0UL;

	crc = crc ^ 0xffffffffUL;

	while (len >= 8) {
		DO8;
		len -= 8;
	}

	if (len) {
		do {
			DO1;
		} while (--len);
	}

	return crc ^ 0xffffffffUL;
}

/* To be used only when the state is known to be valid */
static_tree_desc  static_l_desc =
{static_ltree, extra_lbits, LITERALS+1, L_CODES, MAX_BITS};

static_tree_desc  static_d_desc =
{static_dtree, extra_dbits, 0,          D_CODES, MAX_BITS};

static_tree_desc  static_bl_desc =
{(const ct_data *)0, extra_blbits, 0,   BL_CODES, MAX_BL_BITS};

/* ===========================================================================
 * Update a hash value with the given input byte
 * IN  assertion: all calls to to UPDATE_HASH are made with consecutive
 *    input characters, so that a running hash key can be computed from the
 *    previous key instead of complete recalculation each time.
 */
#ifndef HASH_TEST
#ifndef TWO_HASH_TEST
#ifndef NO_CONFLICT_HASH
#define UPDATE_HASH(s,h,c) (h = (((h)<<s->hash_shift) ^ (c)) & s->hash_mask)
#define INSERT_STRING(s, str, match_head) \
	(UPDATE_HASH(s, s->ins_h, s->window[(str) + (MIN_MATCH-1)]), \
	 match_head = s->prev[(str) & s->w_mask] = s->head[s->ins_h], \
	 s->head[s->ins_h] = (ush)(str))

#else
void UPDATE_HASH(deflate_state *s, uInt *h,uInt *h1,uInt str)
{
	if (3 == hash_size) {
		*h1 = ((s->window[(str)] << 8) ^ (s->window[(str) + 1] << 4));
		*h  = ((s->window[(str) + 2]));
	} else if(4 == hash_size) {
		*h1 = ((s->window[(str)] << 10) ^ (s->window[(str) + 1] << 6));
		*h  = ((s->window[(str) + 2] << 2) ^ (s->window[(str) + 3]));
	} else if (5 == hash_size) {
		*h1 = ((s->window[(str)] << 10) ^ (s->window[(str) + 1] << 6));
		*h  = ((s->window[(str) + 2] << 2) ^ (s->window[(str) + 3] << 1));
		*h  = ((*h) ^ (s->window[(str) + 4]));
	} else if (6 == hash_size) {
		*h1 = ((s->window[(str)] << 10) ^ (s->window[(str) + 1] << 6));
		*h  = ((s->window[(str) + 2] << 3) ^ (s->window[(str) + 3] << 2));
		*h  = (*h) ^ (*h1);
		*h1 = ((s->window[(str) + 4] << 1)
				^ (s->window[(str) + 5]));
	} else if (7 == hash_size) {
		*h1 = ((s->window[(str)] << 10)
				^ (s->window[(str) + 1] << 6));
		*h  = ((s->window[(str) + 2] << 4)
				^ (s->window[(str) + 3] << 3));
		*h  = (*h) ^ (*h1);
		*h1 = ((s->window[(str) + 4] << 2) ^ (s->window[(str) + 5] << 1)
				^ (s->window[(str) + 6]));
	} else if (8 == hash_size) {
		*h1 = ((s->window[(str)] << 10) ^ (s->window[(str) + 1] << 6));
		*h  = ((s->window[(str) + 2] << 5) ^ (s->window[(str) + 3] << 4));
		*h  = (*h) ^ (*h1);
		*h1 = ((s->window[(str) + 4] << 3) ^ (s->window[(str) + 5] << 2)
				^ (s->window[(str) + 6] << 1) ^ (s->window[(str) + 7]));
	} else {
		*h1 = ((s->window[(str)] << 10) ^ (s->window[(str) + 1] << 7));
		*h  = ((s->window[(str) + 2] << 6) ^ (s->window[(str) + 3] << 5));
		*h  = (*h) ^ (*h1);
		*h1 = ((s->window[(str) + 4] << 4) ^ (s->window[(str) + 5] << 3)
				^ (s->window[(str) + 6] << 2)
				^ (s->window[(str) + 7] << 1)
				^ (s->window[(str) + 8]));
	}

	*h  = ((*h) ^ (*h1)) & (4 * 1024 - 1);
	*h1 = ((s->window[(str)] << 1) ^ (s->window[(str) + 1]))&(16 - 1);
}

void INSERT_STRING(deflate_state *s, uInt str, uInt *match_head)
{
	UPDATE_HASH(s, &(s->ins_h), &(s->hash_value), str);
	if ((ush)s->hash_value != hash2[s->ins_h]) {
		if ((ush)str - s->head[s->ins_h] >= DEPTH) {
			*match_head = NIL;
			s->prev[(str) & s->w_mask] = 0;
			s->head[s->ins_h] = (ush)(str);
			hash2[s->ins_h] = s->hash_value;
		} else {
			*match_head = s->prev[(str) & s->w_mask] = s->head[s->ins_h];
			s->head[s->ins_h] = (ush)(str);
			hash2[s->ins_h] = s->hash_value;
		}
	}
	else
	{
		*match_head = s->prev[(str) & s->w_mask] = s->head[s->ins_h];
		s->head[s->ins_h] = (ush)(str);
	}
}
#endif

#else
#define UPDATE_HASH(s,h,c) (h = (((h)<<s->hash_shift) ^ (c)) & s->hash_mask)
#define UPDATE_HASH2(s,h,h1,c1,c2,c3)\
	(((h) = (c1)),\
	 (UPDATE_HASH(s,h,c2)),\
	 (UPDATE_HASH(s,h,c3)),\
	 ((h) = (h) ^ (h1)))
#define INSERT_STRING(s, str, match_head, match_head2)\
	(UPDATE_HASH(s, s->ins_h, s->window[(str) + (MIN_MATCH-1)]), \
	 UPDATE_HASH2(s, s->hash_value, s->ins_h, s->window[(str) + (MIN_MATCH)],\
		 s->window[(str) + (MIN_MATCH)+1], s->window[(str) + (MIN_MATCH)+2]),\
	 match_head = s->prev[(str) & s->w_mask] = s->head[s->ins_h], \
	 s->head[s->ins_h] = (ush)(str),\
	 match_head2 = hash2_prev[(str) & s->w_mask] = hash2_head[s->hash_value], \
	 hash2_head[s->hash_value] = (ush)(str))
#endif
#else

/*
 *used for 2K+2K
 */
uInt getnum(uInt c)
{
	if (c >= 48 && c <= 57)
		return (c - 46);
	else if (c == 46)
		return 1;
	else if (c == 124)
		return 12;
	else
		return 0;
}


void UPDATE_HASH(deflate_state *s, uInt h, uInt c1, uInt c2, uInt c3)
{
	uInt mask = (s->hash_mask + 1) / 16 -1;
	s->hash_value = ((s->hash_value << s->hash_shift) ^ (c1)) & mask;
	s->ins_h = s->hash_value;
	uInt a = getnum(c3);
	uInt b = getnum(c2);
	uInt c = getnum(c1);
	if(a*b*c!=0){
		s->ins_h = a + ((b-1)*12) + ((c-1)*144) +mask;
	}
}

void INSERT_STRING(deflate_state *s,  uInt str,  uInt *match_head) {
	UPDATE_HASH(s, s->ins_h, s->window[(str) + (MIN_MATCH-1)],
			s->window[(str) + 1], s->window[(str)]);
	*match_head = NULL;
	*match_head = s->prev[(str) & s->w_mask] = s->head[s->ins_h];
	s->head[s->ins_h] = (ush)(str);
}
#endif

# define _tr_tally_lit(s, c, flush) \
{ uch cc = (c); \
	s->d_buf[s->last_lit] = 0; \
	s->l_buf[s->last_lit++] = cc; \
	s->dyn_ltree[cc].Freq++; \
	flush = (s->last_lit == s->lit_bufsize-1); \
}
# define _tr_tally_dist(s, distance, length, flush) \
{ uch len = (length); \
	ush dist = (distance); \
	s->d_buf[s->last_lit] = dist; \
	s->l_buf[s->last_lit++] = len; \
	dist--; \
	s->dyn_ltree[_length_code[len]+LITERALS+1].Freq++; \
	s->dyn_dtree[d_code(dist)].Freq++; \
	flush = (s->last_lit == s->lit_bufsize-1); \
}

/* ===========================================================================
 * Flush the current block, with given end-of-file flag.
 * IN assertion: strstart is set to the end of the current match.
 */

#define FLUSH_BLOCK_ONLY(s, last) { \
	_tr_flush_block(s, (s->block_start >= 0L ? \
				(char *)&s->window[(unsigned)s->block_start] : \
				(char *)Z_NULL), \
			(uLong)((long)s->strstart - s->block_start), \
			(last)); \
	s->block_start = s->strstart; \
	flush_pending(s->strm); \
	Tracev((stderr,"[FLUSH]")); \
}

/* Same but force premature exit if necessary. */
/*#define FLUSH_BLOCK(s, last) { \
  FLUSH_BLOCK_ONLY(s, last); \
  if (s->strm->avail_out == 0) return (last) ? finish_started : need_more; \
  }*/

/*deflate functions*/
#define TRY_FREE(s, p) {if (NULL != p) ZFREE(s, p);}
#define d_code(dist) \
	((dist) < 256 ? _dist_code[dist] : _dist_code[256+((dist)>>7)])
/* Mapping from a distance to a distance code. dist is the distance - 1 and
 * must not have side effects. _dist_code[256] and _dist_code[257] are never
 * used.
 */

const config gzip_configuration_table[10] = {
	/*      good lazy nice chain */
	/* 0 */ {0,    0,  0,    0, deflate_slow}, /* store only */
	/* 1 */ {4,    3,  8,    2, deflate_slow}, /* max speed, no lazy matches */
	/* 2 */ {4,    3,  8,    4, deflate_slow},
	/* 3 */ {4,    3, 16,    8, deflate_slow}, /* 8->4, nice 16->6,good 4->6->12*/
	/* 4 */ {4,    4, 16,   16, deflate_slow}, /* lazy matches */
	/* 5 */ {8,   16, 32,   32, deflate_slow},
	/* 6 */ {8,   3, 128, 128, deflate_slow},
	/* 7 */ {8,   32, 128, 256, deflate_slow},
	/* 8 */ {32, 128, 258, 1024, deflate_slow},
	/* 9 */ {32,   3, 258, 4096, deflate_slow}}; /* max compression */

int def_lz(unsigned char* in, unsigned int in_file_len,
		unsigned int *lz_result, struct def_lz_rm *drm,
		int cprLevel, int task_flag)
{

	z_stream_gz zcpr;
	defalte_interface *inter;
	int ret=Z_OK;

	memset(&zcpr,0,sizeof(z_stream_gz));

	deflateInit(&zcpr,cprLevel);

	zcpr.next_in   = in;
	zcpr.avail_in  = in_file_len;
	zcpr.next_out  = (uch *)gzip_alloc(100);
	zcpr.avail_out = 100;
	if (task_flag != 0)
		load_lz_mid_inf(&zcpr,drm);

	/*the main deflate function*/
	/*modify for new requirement*/
	/*if the char and the length/distance >= 8160,
	 *need to watch the line of the lz result:when
	 *it is the times of 8, a block end
	 */
	ret=deflate_lz(&zcpr,Z_FINISH);
	kfree(zcpr.next_out);
	drm->total_in       += zcpr.total_in;
	drm->in_file_size   += in_file_len;
	drm->crc_out        = zcpr.adler;

	inter = zcpr.inter;
	getLzResult(inter, lz_result, drm, task_flag);

	if (ret != Z_STREAM_END) {
		pr_err("LZ ERROR: %d!\n", ret);
		deflateEnd(&zcpr);
		return 1;
	}

	save_lz_mid_inf(&zcpr,drm);
	deflateEnd(&zcpr);
	return 0;
}

int def_huf_block(unsigned char* in, unsigned int *lz_result,
		struct def_lz_rm *drm, struct def_huf_rm *hrm)
{
	int ret = 0;
	ret = deflate_huf_block(in,lz_result,drm,hrm);

	if (hrm->next_line_num == drm->lz_result_line) {
		drm->total_block_num = hrm->cur_block_num;
		if (ret == 1)
			return 2;
		else
			return 3;
	} else {
		if (ret == 1)
			return 0;
		else
			return 1;
	}
}

/*load lz middle information to strm*/
void load_lz_mid_inf(z_streamp_gz strm, struct def_lz_rm *drm)
{
	int i;

	deflate_state* state   = strm->state;
	state->ins_h           = drm->ins_h;
	state->match_length    = drm->match_length;
	state->match_available = drm->match_available;
	state->prev_match      = drm->prev_match;
	state->strstart        = drm->strstart;
	state->match_start     = drm->match_start;
	state->lookahead       = drm->lookahead;
	state->prev_length     = drm->prev_length;
	state->insert          = drm->insert;
	state->high_water      = drm->high_water;
	state->last_lit        = drm->last_lit;
	strm->inter->last_lit  = drm->last_lit;
	strm->adler            = drm->crc_out;
	strm->next_in         += drm->total_in;

	for (i = 0; i < (1 << MAX_WBITS); ++i) {
		state->prev[i] = drm->prev[i];
	}

	for (i = 0; i < (1 << (MAX_WBITS+1)); ++i) {
		state->window[i] = drm->window[i];
	}

	for (i = 0; i< (1 << (DEF_MEM_LEVEL + 7)); ++i) {
		state->head[i] = drm->head[i];
	}
}

/*save lz middle information to drm*/
void save_lz_mid_inf(z_streamp_gz strm, struct def_lz_rm *drm)
{
	int i;

	deflate_state* state = strm->state;
	drm->ins_h           = state->ins_h;
	drm->match_length    = state->match_length;
	drm->match_available = state->match_available;
	drm->prev_match      = state->prev_match;
	drm->strstart        = state->strstart;
	drm->match_start     = state->match_start;
	drm->lookahead       = state->lookahead;
	drm->prev_length     = state->prev_length;
	drm->insert          = state->insert;
	drm->high_water      = state->high_water;

	for (i = 0; i < (1 << MAX_WBITS); ++i) {
		drm->prev[i] = state->prev[i];
	}

	for (i = 0; i < (1 << (MAX_WBITS+1)); ++i) {
		drm->window[i] = state->window[i];
	}

	for (i = 0; i< (1 << (DEF_MEM_LEVEL+7)); ++i) {
		drm->head[i] = state->head[i];
	}
}

void getLzResult(const defalte_interface *inter,
		unsigned int *lz_result,
		struct def_lz_rm *drm, int task_flag)
{
	unsigned int total_deal_in = 0;
	unsigned int i = 0;
	unsigned int j = 0;
	unsigned int max;
	int connect_num;
	int left_bits;
	const uch *l_buf;
	const ush *d_buf;

	for (j = 0; j < inter->block_num; ++j) {
		l_buf = &inter->l_buf[j * HUFFMAN_BLOCK_SIZE];
		d_buf = &inter->d_buf[j * HUFFMAN_BLOCK_SIZE];

		i = 0;
		if (j == 0 && (drm->last_lit != 0)) {
			i = drm->last_lit;
		}

		max = (j == inter->block_num - 1)?inter->last_lit:(HUFFMAN_BLOCK_SIZE-1);

		if (drm->lz_result_line > 1) {
			if (((lz_result[drm->lz_result_line - 1] >> 30)
						& 0x03) == 0) {
				connect_num = (lz_result[drm->lz_result_line - 1] >> 24)
						& 0x03;
				while (connect_num < 3 && d_buf[i] == 0 && i < max) {
					left_bits = connect_num * 8;
					lz_result[drm->lz_result_line -1] += (l_buf[i++] << left_bits)
							+ (1 << 24) + (1 << 26);
					connect_num++;
					total_deal_in++;
				}
			}
		}

		for (; i < max;) {
			if (d_buf[i] != 0) {
				lz_result[drm->lz_result_line] = (l_buf[i]+3) + (d_buf[i] << 9)
						+ (1 << 24) + (2 << 26) + (1 << 30);
				if(l_buf[i] > 1) lz_result[drm->lz_result_line] += (1<<28);
				total_deal_in += l_buf[i]+3;
				i++;
			} else {
				lz_result[drm->lz_result_line] = l_buf[i++]
						+ (1 << 24) + (1 << 26);
				total_deal_in++;
				connect_num = (lz_result[drm->lz_result_line] >> 24)&0x03;
				while (connect_num < 3 && d_buf[i] == 0 && i < max) {
					left_bits = connect_num *8;
					lz_result[drm->lz_result_line] += (l_buf[i++] << left_bits)
						+ (1 << 24) + (1 << 26);
					connect_num++;
					total_deal_in++;
				}
			}

			(drm->lz_result_line) += 1;
		}
	}

	drm->total_block_num  +=  inter->block_num -1;
	drm->last_lit          =  inter->last_lit;

	if (task_flag == 2 && j == inter->block_num) {
		lz_result[(drm->lz_result_line) -1] += (1 << 29);
		lz_result[drm->lz_result_line++] = (7 << 29)
				+ (drm->crc_out &0xffff);
		lz_result[drm->lz_result_line++] = (7 << 29)
				+ ((drm->crc_out >> 16)&0xffff);
		lz_result[drm->lz_result_line++] = (5 << 29)
				+ (drm->in_file_size);
		drm->total_block_num  += 1;
	}
}


int ZEXPORT deflateEnd(z_streamp_gz strm)
{
	int status;

	if (strm == Z_NULL || strm->state == Z_NULL)
		return Z_STREAM_ERROR;

	status = strm->state->status;
	if (status != INIT_STATE &&
			status != EXTRA_STATE &&
			status != NAME_STATE &&
			status != COMMENT_STATE &&
			status != HCRC_STATE &&
			status != BUSY_STATE &&
			status != FINISH_STATE) {
		return Z_STREAM_ERROR;
	}

	/* Deallocate in reverse order of allocations: */
	TRY_FREE(strm, strm->state->pending_buf);
	TRY_FREE(strm, strm->state->head);
	TRY_FREE(strm, strm->state->prev);
	TRY_FREE(strm, strm->inter);

	ZFREE(strm, strm->state);
	strm->state = Z_NULL;

	return status == BUSY_STATE ? Z_DATA_ERROR : Z_OK;
}

int ZEXPORT deflateInit_(z_streamp_gz strm, int level,
		const char *version, int stream_size)
{
	return deflateInit2_(strm, level, Z_DEFLATED, MAX_WBITS, DEF_MEM_LEVEL,
			Z_DEFAULT_STRATEGY, version, stream_size);
}


int ZEXPORT deflateInit2_(z_streamp_gz strm, int  level, int  method,
		int windowBits, int memLevel,
		int strategy, const char *version,
		int stream_size)
{
	deflate_state *s;
	defalte_interface *inter;
	int wrap = 2;
	static const char my_version[] = ZLIB_VERSION;

	ush *overlay;
	/* We overlay pending_buf and d_buf+l_buf.
	 * This works since the average
	 * output size for (length,distance) codes is <= 24 bits.
	 */

	if (version == Z_NULL
			|| version[0] != my_version[0]
			|| stream_size != sizeof(z_stream_gz)) {
		return Z_VERSION_ERROR;
	}

	if (strm == Z_NULL)
		return Z_STREAM_ERROR;

	strm->msg = Z_NULL;

	if (strm->zalloc == (alloc_func)0) {
#ifdef Z_SOLO
		return Z_STREAM_ERROR;
#else
		strm->zalloc = zcalloc;
		strm->opaque = (voidpf)0;
#endif
	}

	if (strm->zfree == (free_func)0)
#ifdef Z_SOLO
		return Z_STREAM_ERROR;
#else
	strm->zfree = zcfree;
#endif
	if (level == Z_DEFAULT_COMPRESSION)
		level = 6;

	if (memLevel < 1 || memLevel > MAX_MEM_LEVEL
			|| method != Z_DEFLATED
			|| windowBits < 8
			|| windowBits > 15
			|| level < 0
			|| level > 9
			|| strategy < 0
			|| strategy > Z_FIXED) {
		return Z_STREAM_ERROR;
	}

	if (windowBits == 8)
		windowBits = 9;  /* until 256-byte
				    window bug fixed */
	s = (deflate_state *)ZALLOC(strm, 1,
			sizeof(deflate_state));
	if (s == Z_NULL)
		return Z_MEM_ERROR;
	strm->state = (struct internal_state *)s;
	s->strm = strm;
	s->in_off = 0;
	inter = (defalte_interface*)ZALLOC(strm, 1,
			sizeof(defalte_interface));
	if (inter == Z_NULL)
		return Z_MEM_ERROR;
	memset(inter,0,sizeof(defalte_interface));
	strm->inter = inter;

	s->wrap = wrap;
	s->w_bits = windowBits;
	s->w_size = 1 << s->w_bits;
	s->w_mask = s->w_size - 1;

	s->hash_bits = memLevel + 7;
	s->hash_size = 1 << s->hash_bits;
	s->hash_mask = s->hash_size - 1;
	s->hash_shift =  ((s->hash_bits+MIN_MATCH-1) / MIN_MATCH);

	s->window = (Byte *)ZALLOC(strm, s->w_size, 2*sizeof(Byte));
	s->prev   = (ush *)ZALLOC(strm, s->w_size, sizeof(ush));
	s->head   = (ush *)ZALLOC(strm, s->hash_size, sizeof(ush));

	s->high_water = 0; /* nothing written to s->window yet */

	s->lit_bufsize = 1 << (memLevel + 6); /* 8K elements by default */

	overlay = (ush *)ZALLOC(strm, s->lit_bufsize, sizeof(ush)+2);
	s->pending_buf = (uch *) overlay;
	s->pending_buf_size = (unsigned long)s->lit_bufsize
				* (sizeof(ush) + 2L);

	if (s->window == Z_NULL || s->prev == Z_NULL ||
			s->head == Z_NULL ||
			s->pending_buf == Z_NULL) {
		s->status = FINISH_STATE;
		deflateEnd (strm);
		return Z_MEM_ERROR;
	}

	s->d_buf = inter->d_buf;
	s->l_buf = inter->l_buf;

	s->level = level;
	s->strategy = strategy;
	s->method = (Byte)method;

	return deflateReset(strm);
}

int ZEXPORT deflateReset(z_streamp_gz strm)
{
	int ret;

	ret = deflateResetKeep(strm);
	if (ret == Z_OK)
		lm_init(strm->state);
	return ret;
}

int ZEXPORT deflateResetKeep(z_streamp_gz strm)
{
	deflate_state *s;

	if (strm == Z_NULL || strm->state == Z_NULL ||
			strm->zalloc == (alloc_func)0 ||
			strm->zfree == (free_func)0) {
		return Z_STREAM_ERROR;
	}

	strm->total_in = strm->total_out = 0;
	strm->msg = Z_NULL; /* use zfree if we ever
			       allocate msg dynamically */

	s = (deflate_state *)strm->state;
	s->pending = 0;
	s->pending_out = s->pending_buf;

	if (s->wrap < 0) {
		s->wrap = -s->wrap; /* was made negative
				       by deflate(..., Z_FINISH); */
	}
	s->status = s->wrap ? INIT_STATE : BUSY_STATE;
	strm->adler = crc32(0L, Z_NULL, 0);
	s->last_flush = Z_NO_FLUSH;

	_tr_init(s);

	return Z_OK;
}

/* ===================================================================
 * Flush as much pending output as possible. All deflate() output goes
 * through this function so some applications may wish to modify it
 * to avoid allocating a large strm->next_out buffer and copying into it.
 * (See also read_buf()).
 */
void flush_pending(z_streamp_gz strm)
{
	unsigned len;
	deflate_state *s = strm->state;

	_tr_flush_bits(s);
	len = s->pending;
	if (len > strm->avail_out)
		len = strm->avail_out;
	if (len == 0)
		return;

	zmemcpy(strm->next_out, s->pending_out, len);
	strm->next_out  += len;
	s->pending_out  += len;
	strm->total_out += len;
	strm->avail_out  -= len;
	s->pending -= len;
	if (s->pending == 0) {
		s->pending_out = s->pending_buf;
	}
}

/* ==================================================================
 * Read a new buffer from the current input stream, update the adler32
 * and total number of bytes read.  All deflate() input goes through
 * this function so some applications may wish to modify it to avoid
 * allocating a large strm->next_in buffer and copying from it.
 * (See also flush_pending()).
 */
int read_buf(z_streamp_gz strm, Byte *buf, unsigned size)
{
	unsigned len = strm->avail_in;

	if (len > size) len = size;
	if (len == 0) return 0;

	strm->avail_in  -= len;

	zmemcpy(buf, strm->next_in, len);
	strm->adler = crc32(strm->adler, buf, len);

	strm->next_in  += len;
	strm->total_in += len;

	return (int)len;
}

/* =======================================================
 * Fill the window when the lookahead becomes insufficient.
 * Updates strstart and lookahead.
 *
 * IN assertion: lookahead < MIN_LOOKAHEAD
 * OUT assertions: strstart <= window_size-MIN_LOOKAHEAD
 *    At least one byte has been read, or avail_in == 0; reads are
 *    performed for at least two bytes (required for the zip translate_eol
 *    option -- not supported here).
 */
void fill_window(deflate_state *s)
{
	register unsigned n, m;
	register ush *p;
	unsigned more;    /* Amount of free space
			     at the end of the window. */
	uInt wsize = s->w_size;

	Assert(s->lookahead < MIN_LOOKAHEAD, "already enough lookahead");

	do {
		more = (unsigned)(s->window_size - (uLong)s->lookahead - (uLong)s->strstart);

		/* Deal with !@#$% 64K limit: */
		if (sizeof(int) <= 2) {
			if (more == 0 && s->strstart == 0 && s->lookahead == 0) {
				more = wsize;

			} else if (more == (unsigned)(-1)) {
				/* Very unlikely, but possible on 16 bit machine if
				 * strstart == 0 && lookahead == 1 (input done a byte at time)
				 */
				more--;
			}
		}

		/* If the window is almost full and there is insufficient lookahead,
		 * move the upper half to the lower one to make room in the upper half.
		 */
		if (s->strstart >= wsize+MAX_DIST(s)) {

			zmemcpy(s->window, s->window+wsize, (unsigned)wsize);
			s->match_start -= wsize;
			s->strstart    -= wsize; /* we now have strstart >= MAX_DIST */
			s->block_start -= (long) wsize;

			s->in_off      += wsize;
			/* Slide the hash table (could be avoided with 32 bit values
			   at the expense of memory usage). We slide even when level == 0
			   to keep the hash table consistent if we switch back to level > 0
			   later. (Using level 0 permanently is not an optimal usage of
			   zlib, so we don't care about this pathological case.)
			 */
			n = s->hash_size;
			p = &s->head[n];
			do {
				m = *--p;
				*p = (ush)(m >= wsize ? m-wsize : NIL);
#ifdef NO_CONFLICT_HASH
				hash2[n] = (ush)(m >= wsize ? hash2[n] : 0);
#endif
			} while (--n);

			n = wsize;
#ifndef FASTEST
			p = &s->prev[n];
			do {
				m = *--p;
				*p = (ush)(m >= wsize ? m-wsize : NIL);
				/* If n is not on any hash chain, prev[n] is garbage but
				 * its value will never be used.
				 */
			} while (--n);
#endif
			more += wsize;
		}

		if (s->strm->avail_in == 0)
			break;

		/* If there was no sliding:
		 *    strstart <= WSIZE+MAX_DIST-1 && lookahead <= MIN_LOOKAHEAD - 1 &&
		 *    more == window_size - lookahead - strstart
		 * => more >= window_size - (MIN_LOOKAHEAD-1 + WSIZE + MAX_DIST-1)
		 * => more >= window_size - 2*WSIZE + 2
		 * In the BIG_MEM or MMAP case (not yet supported),
		 *   window_size == input_size + MIN_LOOKAHEAD  &&
		 *   strstart + s->lookahead <= input_size => more >= MIN_LOOKAHEAD.
		 * Otherwise, window_size == 2*WSIZE so more >= 2.
		 * If there was sliding, more >= WSIZE. So in all cases, more >= 2.
		 */
		Assert(more >= 2, "more < 2");

		n = read_buf(s->strm, s->window + s->strstart + s->lookahead, more);
		s->lookahead += n;

		/* Initialize the hash value now that we have some input: */
		if (s->lookahead + s->insert >= MIN_MATCH) {
			uInt str = s->strstart - s->insert;
			s->ins_h =  s->window[str];
#ifndef HASH_TEST
#ifndef TWO_HASH_TEST
			s->hash_value = s->ins_h;
			s->hash_value = (((s->hash_value)<<s->hash_shift)
					^ (s->window[str + 1])) & s->hash_mask;
#else
			UPDATE_HASH(s, s->ins_h, s->window[str + 1]);
#endif
#else
			UPDATE_HASH(s, s->ins_h, s->window[str + 1],
					s->window[str], 97);
			s->hash_value = s->ins_h;
#endif
#if MIN_MATCH != 3
			Call UPDATE_HASH() MIN_MATCH-3 more times
#endif
				while (s->insert) {
#ifndef HASH_TEST
#ifndef TWO_HASH_TEST

#ifndef NO_CONFLICT_HASH
					UPDATE_HASH(s, s->ins_h, s->window[str + MIN_MATCH -1]);
#else
					UPDATE_HASH(s,&(s->ins_h),&(s->hash_value),str);
#endif

#else
					UPDATE_HASH(s, s->ins_h, s->window[str + MIN_MATCH -1]);
					UPDATE_HASH2(s, s->hash_value, s->ins_h, s->window[str + MIN_MATCH],
							s->window[str + MIN_MATCH+1], s->window[str + MIN_MATCH+2]);
					hash2_prev[str & s->w_mask] = hash2_head[s->ins_h];
					hash2_head[s->hash_value] = (ush)str;
#endif
#else
					UPDATE_HASH(s, s->ins_h, s->window[str + MIN_MATCH -1],
							s->window[str + 1], s->window[str]);
#endif
#ifndef FASTEST
					s->prev[str & s->w_mask] = s->head[s->ins_h];
#endif
					s->head[s->ins_h] = (ush)str;
#ifdef NO_CONFLICT_HASH
					hash2[s->ins_h] = s->hash_value;
#endif
					str++;
					s->insert--;
					if (s->lookahead + s->insert < MIN_MATCH)
						break;
				}
		}
		/* If the whole input has less than MIN_MATCH bytes, ins_h is garbage,
		 * but this is not important since only literal bytes will be emitted.
		 */

	} while (s->lookahead < MIN_LOOKAHEAD && s->strm->avail_in != 0);

	/* If the WIN_INIT bytes after the end of the current data have never been
	 * written, then zero those bytes in order to avoid memory check reports of
	 * the use of uninitialized (or uninitialised as Julian writes) bytes by
	 * the longest match routines.  Update the high water mark for the next
	 * time through here.  WIN_INIT is set to MAX_MATCH since the longest match
	 * routines allow scanning to strstart + MAX_MATCH, ignoring lookahead.
	 */
	if (s->high_water < s->window_size) {
		uLong curr = s->strstart + (uLong)(s->lookahead);
		uLong init;

		if (s->high_water < curr) {
			/* Previous high water mark below current data -- zero WIN_INIT
			 * bytes or up to end of window, whichever is less.
			 */
			init = s->window_size - curr;
			if (init > WIN_INIT)
				init = WIN_INIT;
			zmemzero(s->window + curr, (unsigned)init);
			s->high_water = curr + init;
		} else if (s->high_water < (uLong)curr + WIN_INIT) {
			/* High water mark at or above current data, but below current data
			 * plus WIN_INIT -- zero out to current data plus WIN_INIT, or up
			 * to end of window, whichever is less.
			 */
			init = (uLong)curr + WIN_INIT - s->high_water;
			if (init > s->window_size - s->high_water)
				init = s->window_size - s->high_water;
			zmemzero(s->window + s->high_water, (unsigned)init);
			s->high_water += init;
		}
	}

	Assert((uLong)s->strstart <= s->window_size - MIN_LOOKAHEAD,
			"not enough room for search");
}
#ifdef TWO_HASH_TEST

/* current match */
uInt longest_match2(deflate_state *s, unsigned cur_match)
{
	unsigned chain_length = s->max_chain_length;    /* max hash chain length */
	register Byte *scan  = s->window + s->strstart; /* current string */
	register Byte *match;                           /* matched string */
	register int len;                               /* length of current match */
	int best_len = s->prev_length;                  /* best match length so far */
	int nice_match = s->nice_match;                 /* stop if match long enough */
	unsigned limit = s->strstart > (unsigned)MAX_DIST(s) ?
		s->strstart - (unsigned)MAX_DIST(s) : NIL;
	/* Stop when cur_match becomes <= limit. To simplify the code,
	 * we prevent matches with the string of window index 0.
	 */
	ush *prev = hash2_prev;
	uInt wmask = s->w_mask;

#ifdef UNALIGNED_OK
	/* Compare two bytes at a time. Note: this is not always beneficial.
	 * Try with and without -DUNALIGNED_OK to check.
	 */
	register Bytef *strend = s->window + s->strstart + MAX_MATCH - 1;
	register ush scan_start = *(ushf*)scan;
	register ush scan_end   = *(ushf*)(scan+best_len - 1);
#else
	register Byte *strend  = s->window + s->strstart + MAX_MATCH;
	register Byte scan_end1  = scan[best_len - 1];
	register Byte scan_end   = scan[best_len];
#endif

	/* The code is optimized for HASH_BITS >= 8 and MAX_MATCH-2 multiple of 16.
	 * It is easy to get rid of this optimization if necessary.
	 */
	Assert(s->hash_bits >= 8 && MAX_MATCH == 258, "Code too clever");

	/* Do not waste too much time if we already have a good match: */
	if (s->prev_length >= s->good_match) {
		chain_length >>= 2;
	}

	/* Do not look for matches beyond the end of the input. This is necessary
	 * to make deflate deterministic.
	 */
	if ((uInt)nice_match > s->lookahead)
		nice_match = s->lookahead;

	Assert((uLong)s->strstart <= s->window_size-MIN_LOOKAHEAD, "need lookahead");
	Assert(cur_match < s->strstart, "no future");

	while ((cur_match)> limit
			&& (0 != chain_length--)
			&& cur_match != 16*1024)
	{
		Assert(cur_match < s->strstart, "no future");
		match = s->window + cur_match;

		/* Skip to next match if the match length cannot increase
		 * or if the match length is less than 2.  Note that the checks below
		 * for insufficient lookahead only occur occasionally for performance
		 * reasons.  Therefore uninitialized memory will be accessed, and
		 * conditional jumps will be made that depend on those values.
		 * However the length of the match is limited to the lookahead, so
		 * the output of deflate is not affected by the uninitialized values.
		 */
#if (defined(UNALIGNED_OK) && MAX_MATCH == 258)
		/* This code assumes sizeof(unsigned short) == 2. Do not use
		 * UNALIGNED_OK if your compiler uses a different size.
		 */
		if (*(ushf*)(match+best_len - 1) != scan_end ||
				*(ushf*)match != scan_start)
			continue;

		/* It is not necessary to compare scan[2] and match[2] since they are
		 * always equal when the other bytes match, given that the hash keys
		 * are equal and that HASH_BITS >= 8. Compare 2 bytes at a time at
		 * strstart+3, +5, ... up to strstart+257. We check for insufficient
		 * lookahead only every 4th comparison; the 128th check will be made
		 * at strstart+257. If MAX_MATCH-2 is not a multiple of 8, it is
		 * necessary to put more guard bytes at the end of the window, or
		 * to check more often for insufficient lookahead.
		 */
		Assert(scan[2] == match[2], "scan[2]?");
		scan++;
		match++;
		do {
		} while (*(ushf*)(scan += 2) == *(ushf*)(match += 2) &&
				*(ushf*)(scan += 2) == *(ushf*)(match += 2) &&
				*(ushf*)(scan += 2) == *(ushf*)(match += 2) &&
				*(ushf*)(scan += 2) == *(ushf*)(match += 2) &&
				scan < strend);
		/* The funny "do {}" generates better code on most compilers */

		/* Here, scan <= window+strstart+257 */
		Assert(scan <= s->window+(unsigned)(s->window_size - 1), "wild scan");
		if (*scan == *match)
			scan++;

		len = (MAX_MATCH - 1) - (int)(strend-scan);
		scan = strend - (MAX_MATCH - 1);

#else /* UNALIGNED_OK */

		if (match[best_len] != scan_end ||
			match[best_len - 1] != scan_end1 ||
			*match != *scan ||
			*++match != scan[1]) {
			cur_match = (prev[cur_match & wmask]);
			continue;
		}

		/* The check at best_len-1 can be removed because it will be made
		 * again later. (This heuristic is not always a win.)
		 * It is not necessary to compare scan[2] and match[2] since they
		 * are always equal when the other bytes match, given that
		 * the hash keys are equal and that HASH_BITS >= 8.
		 */
		scan += 2;
		match++;
		Assert(*scan == *match, "match[2]?");

		/* We check for insufficient lookahead only every 8th comparison;
		 * the 256th check will be made at strstart+258.
		 */
		do {
		} while (*++scan == *++match && *++scan == *++match &&
				*++scan == *++match && *++scan == *++match &&
				*++scan == *++match && *++scan == *++match &&
				*++scan == *++match && *++scan == *++match &&
				scan < strend);

		Assert(scan <= s->window+(unsigned)(s->window_size - 1), "wild scan");

		len = MAX_MATCH - (int)(strend - scan);
		scan = strend - MAX_MATCH;

#endif /* UNALIGNED_OK */
		if (len > best_len) {
			s->match_start = cur_match;
			best_len = len;
			if (len >= nice_match)
				break;
#ifdef UNALIGNED_OK
			scan_end = *(ushf*)(scan+best_len - 1);
#else
			scan_end1  = scan[best_len - 1];
			scan_end   = scan[best_len];
#endif
		}

		cur_match = (prev[cur_match & wmask]);
	}

	if ((uInt)best_len <= s->lookahead)
		return (uInt)best_len;
	return s->lookahead;
}
#endif

/* current match */
uInt longest_match(deflate_state *s, unsigned cur_match)
{
	unsigned chain_length = s->max_chain_length;    /* max hash chain length */
	register Byte *scan  = s->window + s->strstart; /* current string */
	register Byte *match;                           /* matched string */
	register int len = 0;                           /* length of current match */
	int best_len = s->prev_length;                  /* best match length so far */
	int nice_match = s->nice_match;                 /* stop if match long enough */
	unsigned limit = s->strstart > (unsigned)MAX_DIST(s) ?
		s->strstart - (unsigned)MAX_DIST(s) : NIL;
	/* Stop when cur_match becomes <= limit. To simplify the code,
	 * we prevent matches with the string of window index 0.
	 */
	ush *prev = s->prev;
	uInt wmask = s->w_mask;

#ifdef OPTMIZATION
#ifndef CONTINUE_CONNECT
	best_len = 0;
#else
	best_len = 0;
#endif
#endif

#ifdef UNALIGNED_OK
	/* Compare two bytes at a time. Note: this is not always beneficial.
	 * Try with and without -DUNALIGNED_OK to check.
	 */
	register Bytef *strend = s->window + s->strstart + MAX_MATCH - 1;
	register ush scan_start = *(ushf*)scan;
	register ush scan_end   = *(ushf*)(scan+best_len-1);
#else
	register Byte *strend  = s->window + s->strstart + MAX_MATCH;
	register Byte scan_end1  = scan[best_len-1];
	register Byte scan_end   = scan[best_len];
#endif

	/* The code is optimized for HASH_BITS >= 8 and MAX_MATCH-2 multiple of 16.
	 * It is easy to get rid of this optimization if necessary.
	 */
	Assert(s->hash_bits >= 8 && MAX_MATCH == 258, "Code too clever");

	/* Do not waste too much time if we already have a good match: */
#ifndef OPTMIZATION
	if (s->prev_length >= s->good_match) {
		chain_length >>= 2;
	}
#endif
	/* Do not look for matches beyond the end of the input. This is necessary
	 * to make deflate deterministic.
	 */
	if ((uInt)nice_match > s->lookahead)
		nice_match = s->lookahead;

	Assert((uLong)s->strstart <= s->window_size-MIN_LOOKAHEAD, "need lookahead");
	Assert(cur_match < s->strstart, "no future");

	while ((cur_match)> limit
			&& cur_match != (1<<MAX_WBITS)
			&& (0 != chain_length--))
	{
		Assert(cur_match < s->strstart, "no future");
		match = s->window + cur_match;

		/* Skip to next match if the match length cannot increase
		 * or if the match length is less than 2.  Note that the checks below
		 * for insufficient lookahead only occur occasionally for performance
		 * reasons.  Therefore uninitialized memory will be accessed, and
		 * conditional jumps will be made that depend on those values.
		 * However the length of the match is limited to the lookahead, so
		 * the output of deflate is not affected by the uninitialized values.
		 */
#if (defined(UNALIGNED_OK) && MAX_MATCH == 258)
		/* This code assumes sizeof(unsigned short) == 2. Do not use
		 * UNALIGNED_OK if your compiler uses a different size.
		 */
		if (*(ushf*)(match+best_len-1) != scan_end ||
				*(ushf*)match != scan_start)
			continue;

		/* It is not necessary to compare scan[2] and match[2] since they are
		 * always equal when the other bytes match, given that the hash keys
		 * are equal and that HASH_BITS >= 8. Compare 2 bytes at a time at
		 * strstart+3, +5, ... up to strstart+257. We check for insufficient
		 * lookahead only every 4th comparison; the 128th check will be made
		 * at strstart+257. If MAX_MATCH-2 is not a multiple of 8, it is
		 * necessary to put more guard bytes at the end of the window, or
		 * to check more often for insufficient lookahead.
		 */
		Assert(scan[2] == match[2], "scan[2]?");
		scan++;
		match++;
		do {
		} while (*(ushf*)(scan+=2) == *(ushf*)(match+=2) &&
				*(ushf*)(scan+=2) == *(ushf*)(match+=2) &&
				*(ushf*)(scan+=2) == *(ushf*)(match+=2) &&
				*(ushf*)(scan+=2) == *(ushf*)(match+=2) &&
				scan < strend);
		/* The funny "do {}" generates better code on most compilers */

		/* Here, scan <= window+strstart+257 */
		Assert(scan <= s->window+(unsigned)(s->window_size-1), "wild scan");
		if (*scan == *match)
			scan++;

		len = (MAX_MATCH - 1) - (int)(strend-scan);
		scan = strend - (MAX_MATCH-1);

#else /* UNALIGNED_OK */

#ifdef OPTMIZATION
		if (*match != *scan) {
			hash_conflict_num++;
			cur_match = (prev[cur_match & wmask]);
			len =0;
			last_node_match_length[len]++;
			continue;
		}

		if (*++match != scan[1]) {
			hash_conflict_num++;
			cur_match = (prev[cur_match & wmask]);
			len =1;
			last_node_match_length[len]++;
			continue;
		}

		if (match[1] != scan[2]) {
			hash_conflict_num++;
			cur_match = (prev[cur_match & wmask]);
			len =2;
			last_node_match_length[len]++;
			continue;
		}

#else
		if (*match != *scan || *++match != scan[1]
				|| match[1] != scan[2]) {
			cur_match = (prev[cur_match & wmask]);
			len =0;
			continue;
		}
#endif

		/* The check at best_len-1 can be removed because it will be made
		 * again later. (This heuristic is not always a win.)
		 * It is not necessary to compare scan[2] and match[2] since they
		 * are always equal when the other bytes match, given that
		 * the hash keys are equal and that HASH_BITS >= 8.
		 */
		scan += 2;
		match++;
		Assert(*scan == *match, "match[2]?");

		/* We check for insufficient lookahead only every 8th comparison;
		 * the 256th check will be made at strstart+258.
		 */
		do {
		} while (*++scan == *++match && *++scan == *++match &&
				*++scan == *++match && *++scan == *++match &&
				*++scan == *++match && *++scan == *++match &&
				*++scan == *++match && *++scan == *++match &&
				scan < strend);

		Assert(scan <= s->window + (unsigned)(s->window_size - 1), "wild scan");

		len = MAX_MATCH - (int)(strend - scan);
#ifdef OPTMIZATION
#ifndef CONTINUE_CONNECT
		if (len > nice_match)
			len = nice_match;
#else
#ifdef LAST_OPTMIZATION
		if (len > nice_match)
			len = nice_match;
#endif
#endif
		last_node_match_length[len]++;
#else
#endif
		scan = strend - MAX_MATCH;

#endif /* UNALIGNED_OK */
		if (len > best_len) {
			s->match_start = cur_match;
			best_len = len;
			if (len >= nice_match) {
				break;
			}
#ifdef UNALIGNED_OK
			scan_end = *(ushf*)(scan+best_len - 1);
#else
			scan_end1 = scan[best_len - 1];
			scan_end  = scan[best_len];
#endif
		}

		cur_match = (prev[cur_match & wmask]);

	}
#ifdef OPTMIZATION
	if (((cur_match)<= limit || cur_match == (1<<MAX_WBITS))
			&& (len < nice_match) && (0 != chain_length))
		last_node_match_length[0]++;
#endif
	if (best_len < len)
		best_len = len;
	if ((uInt)best_len <= s->lookahead)
		return (uInt)best_len;
	return s->lookahead;
}

void get_lz_result(deflate_state *s, int last)
{
	if (!last) {
		s->d_buf += (s->last_lit+1);
		s->l_buf += (s->last_lit+1);
		s->last_lit = 0;
	}

	s->block_start = s->strstart;
}

void getPara(deflate_state *s, Byte *prev_para,
		Byte **para, uInt prev_len, uInt *curr_len)
{
	int pos = 0;

	*para = prev_para + prev_len;
	do {
		pos++;
		if (pos == s->strm->avail_in)
			break;
	} while ((*para)[pos-1] != (Byte)('\n'));

	*curr_len = pos;
}

int putOutChars(deflate_state *s, Byte *dataSet, uInt out_len, int *bflush)
{
	uInt i;

	for (i = 0; i < out_len; ++i) {
		_tr_tally_lit(s, dataSet[i], (*bflush));
		s->strm->avail_in -= 1;

		if (0 == s->strm->avail_in) {
			get_lz_result(s, 1);
			return 1;
		}

		if (*bflush) {
			get_lz_result(s, 0);
			(*bflush) = 0;
		}
	}

	return 0;
}

int putOutLens(deflate_state *s, uInt distance, uInt match_len, int *bflush)
{
	uInt j;

	if (match_len > 258) {
		uInt count = match_len / 258
			+ ((match_len % 258 == 0)?0:1);
		uInt average_len = match_len / count;
		for (j = 0; j < count; j++) {
			_tr_tally_dist(s, distance,
					(j == count - 1)?
					(match_len - average_len*(count-1)- MIN_MATCH):\
					(average_len - MIN_MATCH),
					(*bflush));
			s->strm->avail_in -= (j == count - 1)?\
					     (match_len - average_len*(count-1)):(average_len);
			if (0 == s->strm->avail_in) {
				get_lz_result(s, 1);
				return 1;
			}

			if (1 == (*bflush)) {
				get_lz_result(s, 0);
				(*bflush) = 0;
			}
		}
	} else {
		_tr_tally_dist(s, distance,
				match_len - MIN_MATCH,
				(*bflush));
		s->strm->avail_in -=  match_len;
		if (0 == s->strm->avail_in) {
			get_lz_result(s, 1);
			return 1;
		}

		if (1 == (*bflush)) {
			get_lz_result(s, 0);
			(*bflush) = 0;
		}
	}

	return 0;
}

block_state deflate_replace(deflate_state *s)
{
	uInt i;
	int  bflush; /* set if current block
			must be flushed */
	Byte *prevPara = s->strm->next_in;
	Byte *currPara = NULL;
	uInt prev_len = 0;
	uInt curr_len = 0;
	uInt ret = 0;
	uInt compare_num;
	uInt match_len;
	bool end_flag;

	for (;;) {
		getPara(s,prevPara, &currPara, prev_len, &curr_len);
		(s->strm)->adler = crc32(s->strm->adler, currPara, curr_len);
		s->strm->total_in += curr_len;
		compare_num = (curr_len >= prev_len)?prev_len:curr_len;
		match_len = 0;

		for (i = 0; i < compare_num; ++i) {
			end_flag = 0;

			if (prevPara[i] == currPara[i]) {
				end_flag = 0;
				match_len++;
			} else {
				end_flag =1;
			}

			if (1 == end_flag) {
				if (match_len >= MIN_MATCH) {
					putOutLens(s, prev_len, match_len, &bflush);
					ret = putOutChars(s, currPara + i, 1, &bflush);
					if (1 == ret)
						return finish_done;
					match_len = 0;
				} else {
					ret = putOutChars(s, currPara + i - match_len,
							match_len + 1, &bflush);
					if (1 == ret)
						return finish_done;
					match_len = 0;
				}
			} else if (i == compare_num - 1) {
				if (match_len >= MIN_MATCH) {
					ret = putOutLens(s, prev_len, match_len, &bflush);
					if (1 == ret)
						return finish_done;
					match_len = 0;
				} else {
					ret = putOutChars(s, currPara + i + 1 - match_len,
							match_len, &bflush);
					if (1 == ret)
						return finish_done;
					match_len = 0;
				}
			}
		}

		if (curr_len > compare_num) {
			ret = putOutChars(s, currPara + compare_num,
					curr_len - compare_num, &bflush);
			if (1 == ret)
				return finish_done;
		}

		if (0 == s->strm->avail_in)
			break;
		prevPara = currPara;
		prev_len = curr_len;
	}

	return finish_done;
}

void skipUntilSepa(Byte *para, uInt para_len,
		uInt *para_index,uch byte)
{
	uInt i = *para_index;
	for ( ; i < para_len; ++i) {
		if(para[i] == byte)
			break;
	}

	*para_index = i;
}


block_state deflate_replace1(deflate_state *s)
{
	int  bflush; /* set if current block
			must be flushed */
	Byte *prevPara = s->strm->next_in;
	Byte *currPara = NULL;
	uInt prev_len = 0;
	uInt curr_len = 0;
	uInt ret = 0;
	Byte byte = '|';
	uInt match_len = 0;
	uInt i = 0;
	uInt j = 0;
	uInt jump = 0;
	bool end_flag;

	for (;;) {
		getPara(s,prevPara, &currPara, prev_len, &curr_len);
		(s->strm)->adler = crc32(s->strm->adler, currPara, curr_len);
		s->strm->total_in += curr_len;

		match_len = 0;
		i = 0;
		j = 0;
		jump = 0;

		for ( ; i < curr_len; ++i) {
			end_flag = 0;

			if (prevPara[j] == currPara[i]) {
				end_flag = 0;
				match_len++;
			} else {
				end_flag =1;
				jump = 0;

				if (byte == prevPara[j]) {
					jump =1;
				}

				if (byte == currPara[i]) {
					jump =2;
				}
			}

			if (1 == end_flag) {
				if (match_len >= MIN_MATCH) {
					putOutLens(s, prev_len-j + i, match_len, &bflush);
				} else {
					ret = putOutChars(s, currPara + i - match_len,
							match_len, &bflush);
				}

				if (1 == jump) {
					uInt prev_index = i;
					skipUntilSepa(currPara, curr_len, &i,byte);
					ret = putOutChars(s, currPara + prev_index,
							i - prev_index, &bflush);
					if (1 == ret)
						return finish_done;
					if (i == curr_len)
						break;
					match_len = 1;
				} else if (2 == jump) {
					skipUntilSepa(prevPara, prev_len, &j,byte);
					if (j == prev_len) {
						ret = putOutChars(s, currPara + i, 1, &bflush);
						if (1 == ret)
							return finish_done;
						break;
					}

					match_len = 1;
				} else {
					ret = putOutChars(s, currPara + i, 1, &bflush);
					match_len = 0;
					if (1 == ret)
						return finish_done;
				}
			} else if (i + 1 == curr_len || j+1 >= prev_len) {
				if (match_len >= MIN_MATCH) {
					ret = putOutLens(s, prev_len - j + i, match_len, &bflush);
					if (1 == ret)
						return finish_done;
					match_len = 0;
				} else {
					ret = putOutChars(s, currPara + i + 1 - match_len,
							match_len, &bflush);
					if (1 == ret)
						return finish_done;
					match_len = 0;
				}

				break;
			}
			++j;
		}

		if (i < curr_len - 1) {
			ret = putOutChars(s, currPara + i + 1,
					curr_len - 1 - i, &bflush);
			if (1 == ret)
				return finish_done;
		}

		if (0 == s->strm->avail_in)
			break;
		prevPara = currPara;
		prev_len = curr_len;
	}

	return finish_done;
}

/* =============================================================
 * Same as above, but achieves better compression. We use a lazy
 * evaluation for matches: a match is finally adopted only if there is
 * no better match at the next window position.
 */
#ifdef LAST_OPTMIZATION
void spliceResult(z_streamp_gz strm)
{
	unsigned int i = 0;
	unsigned int j = 0;
	unsigned int k = 0;
	unsigned int index = 0;
	deflate_state *s = strm->state;
	defalte_interface *inter = strm->inter;
	s->l_buf = inter->l_buf;
	s->d_buf = inter->d_buf;
	s->last_lit = 0;
	ush prev_d = 0;
	uch prev_l = 0;
	bool flush = 0;

	for (j = 0; j < inter->block_num; ++j) {
		uch *l_buf = &inter->l_buf[j * HUFFMAN_BLOCK_SIZE];
		ush *d_buf = &inter->d_buf[j * HUFFMAN_BLOCK_SIZE];

		unsigned int max = (j == inter->block_num - 1)?\
				   inter->last_lit:(HUFFMAN_BLOCK_SIZE-1);

		for (i = 0; i < max; ++i) {
			if (d_buf[i] == 0) {
				if (prev_d != 0) {
					_tr_tally_dist(s, prev_d, prev_l, flush);
					if (flush) {
						s->last_lit = 0;
						s->l_buf += HUFFMAN_BLOCK_SIZE;
						s->d_buf += HUFFMAN_BLOCK_SIZE;
					}
					match_length[prev_l + 3]++;
					prev_d = 0;
					prev_l = 0;
				}

				_tr_tally_lit(s, l_buf[i], flush);
				if (flush) {
					s->last_lit = 0;
					s->l_buf += HUFFMAN_BLOCK_SIZE;
					s->d_buf += HUFFMAN_BLOCK_SIZE;
				}
			} else {
				match_length[l_buf[i] + 3]--;

				if (prev_d == 0) {
					prev_d = d_buf[i];
					prev_l = l_buf[i];
				} else if (prev_d == d_buf[i]) {
					if (prev_l + l_buf[i] + 6 > 258) {
						_tr_tally_dist(s, prev_d, prev_l, flush);
						if (flush) {
							s->last_lit = 0;
							s->l_buf += HUFFMAN_BLOCK_SIZE;
							s->d_buf += HUFFMAN_BLOCK_SIZE;
						}

						match_length[prev_l + 3]++;
						prev_d = d_buf[i];
						prev_l = l_buf[i];
					} else
						prev_l += l_buf[i] +3;
				} else {
					_tr_tally_dist(s, prev_d, prev_l, flush);

					if (flush) {
						s->last_lit = 0;
						s->l_buf += HUFFMAN_BLOCK_SIZE;
						s->d_buf += HUFFMAN_BLOCK_SIZE;
					}

					match_length[prev_l + 3]++;
					prev_d = d_buf[i];
					prev_l = l_buf[i];
				}
			}
		}
	}
}
#endif

#ifndef OPTMIZATION
block_state deflate_slow(deflate_state *s, int flush)
{
	uInt hash_head;  /* head of hash chain */
#ifdef TWO_HASH_TEST
	uInt hash_head2; /* head of hash chain */
#endif
	int  bflush;     /* set if current block must be flushed */

	/* Process the input block. */
	for (;;) {
		/* Make sure that we always have enough lookahead, except
		 * at the end of the input file. We need MAX_MATCH bytes
		 * for the next match, plus MIN_MATCH bytes to insert the
		 * string following the next match.
		 */
		if (s->lookahead < MIN_LOOKAHEAD) {
			fill_window(s);
			if (s->lookahead < MIN_LOOKAHEAD && flush == Z_NO_FLUSH)
				return need_more;

			if (s->lookahead == 0)
				break; /* flush the current block */
		}

		/* Insert the string window[strstart .. strstart+2] in the
		 * dictionary, and set hash_head to the head of the hash chain:
		 */
		hash_head = NIL;
		if (s->lookahead >= MIN_MATCH) {
#ifndef HASH_TEST
#ifndef TWO_HASH_TEST
#ifndef NO_CONFLICT_HASH
			INSERT_STRING(s, s->strstart, hash_head);
#else
			INSERT_STRING(s, s->strstart, &hash_head);
#endif
#else
			INSERT_STRING(s, s->strstart, hash_head, hash_head2);
#endif
#else
			INSERT_STRING(s, s->strstart, &hash_head);
#endif
		}

		/* Find the longest match, discarding those <= prev_length */
		s->prev_length = s->match_length;
		s->prev_match = s->match_start;
		s->match_length = MIN_MATCH-1;

#ifdef TWO_HASH_TEST
		if (hash_head2 != NIL && s->prev_length < s->max_lazy_match &&
				s->strstart - hash_head <= MAX_DIST(s)) {
			/* To simplify the code, we prevent matches with the string
			 * of window index 0 (in particular we have to avoid a match
			 * of the string with itself at the start of the input file).
			 */
			s->match_length = longest_match2 (s, hash_head2);
		}

		if (s->match_length < 3) {
#endif
			if (hash_head != NIL && s->prev_length < s->max_lazy_match &&
					s->strstart - hash_head <= MAX_DIST(s)) {
				/* To simplify the code, we prevent matches with the string
				 * of window index 0 (in particular we have to avoid a match
				 * of the string with itself at the start of the input file).
				 */
				s->match_length = longest_match(s, hash_head);

				if (s->match_length <= 5 && (s->strategy == Z_FILTERED
#if TOO_FAR <= 32767
					|| (s->match_length == MIN_MATCH &&
					s->strstart - s->match_start > TOO_FAR &&
					s->level > 9)
#endif
					)) {

					/* If prev_match is also MIN_MATCH, match_start is garbage
					 * but we will ignore the current match anyway.
					 */
					s->match_length = MIN_MATCH - 1;
				}
			}
#ifdef TWO_HASH_TEST
		}
#endif
		/* If there was a match at the previous step and the current
		 * match is not better, output the previous match */
		if (s->prev_length >= MIN_MATCH && s->match_length <= s->prev_length) {
			uInt max_insert = s->strstart + s->lookahead - MIN_MATCH;
			/* Do not insert strings in hash table beyond this. */

			_tr_tally_dist(s, s->strstart -1 - s->prev_match,
					s->prev_length - MIN_MATCH, bflush);
			/* Insert in hash table all strings up to the end of the match.
			 * strstart-1 and strstart are already inserted. If there is not
			 * enough lookahead, the last two strings are not inserted in
			 * the hash table.
			 */
			s->lookahead -= s->prev_length - 1;
			s->prev_length -= 2;
			do {
				if (++s->strstart <= max_insert) {
#ifndef HASH_TEST
#ifndef TWO_HASH_TEST

#ifndef NO_CONFLICT_HASH
					INSERT_STRING(s, s->strstart, hash_head);
#else
					INSERT_STRING(s, s->strstart, &hash_head);
#endif

#else
					INSERT_STRING(s, s->strstart, hash_head, hash_head2);
#endif
#else
					INSERT_STRING(s, s->strstart, &hash_head);
#endif
				}
			} while (--s->prev_length != 0);

			s->match_available = 0;
			s->match_length = MIN_MATCH-1;
			s->strstart++;

			if (bflush) {
				get_lz_result(s, 0);
				bflush = 0;
			}

		} else if (s->match_available) {
			/* If there was no match at the previous position, output a
			 * single literal. If there was a match but the current match
			 * is longer, truncate the previous match to a single literal.
			 */
			Tracevv((stderr, "%c", s->window[s->strstart - 1]));
			_tr_tally_lit(s, s->window[s->strstart - 1], bflush);
			if (bflush) {
				get_lz_result(s, 0);
				bflush = 0;
			}

			s->strstart++;
			s->lookahead--;
			if (s->strm->avail_out == 0)
				return need_more;
		} else {
			/* There is no previous match to compare with, wait for
			 * the next step to decide.
			 */
			s->match_available = 1;
			s->strstart++;
			s->lookahead--;
		}
	}

	Assert (flush != Z_NO_FLUSH, "no flush?");
	if (s->match_available) {
		Tracevv((stderr, "%c", s->window[s->strstart - 1]));
		_tr_tally_lit(s, s->window[s->strstart - 1], bflush);
		s->match_available = 0;
	}

	s->insert = s->strstart < MIN_MATCH-1 ? s->strstart : MIN_MATCH-1;
	if (flush == Z_FINISH) {
		get_lz_result(s, 1);
		bflush = 0;
		return finish_done;
	}
	if (s->last_lit) {
		get_lz_result(s, 0);
		bflush = 0;
	}

	pr_err("can not be here!\n");

	return block_done;
}
#else
#ifndef CONTINUE_CONNECT
block_state deflate_slow(deflate_state *s, int flush)
{
	uInt hash_head = NIL; /* head of hash chain */
	int  bflush;          /* set if current block must be flushed */

	/* Process the input block. */
	for (;;) {
		/* Make sure that we always have enough lookahead, except
		 * at the end of the input file. We need MAX_MATCH bytes
		 * for the next match, plus MIN_MATCH bytes to insert the
		 * string following the next match.
		 */
		if (s->lookahead < MIN_LOOKAHEAD) {
			fill_window(s);
			if (s->lookahead < MIN_LOOKAHEAD && flush == Z_NO_FLUSH) {
				return need_more;
			}

			if (s->lookahead == 0) {
				s->prev_length = s->match_length;
				s->prev_match = s->match_start;
				if (s->prev_length>=6) {
					_tr_tally_dist(s,
						s->strstart - s->prev_length - s->prev_match,
						s->prev_length - MIN_MATCH, bflush);
				}

				/*for static*/
				match_length_value = s->prev_length;
				match_length[match_length_value]++;
				node_match_length[match_length_value] += jump_back_num;
				jump_back_num = 0;

				if (bflush) {
					get_lz_result(s, 0);
					bflush = 0;
				}

				break; /* flush the current block */
			}
		}

		/* Insert the string window[strstart .. strstart+2] in the
		 * dictionary, and set hash_head to the head of the hash chain:
		 */
		if (hash_head == NIL || s->strstart - hash_head > MAX_DIST(s)) {
#ifndef NO_CONFLICT_HASH
			INSERT_STRING(s, s->strstart-2, hash_head);
			INSERT_STRING(s, s->strstart-1, hash_head);
			INSERT_STRING(s, s->strstart, hash_head);
#else
			INSERT_STRING(s, s->strstart-2, &hash_head);
			INSERT_STRING(s, s->strstart-1, &hash_head);
			INSERT_STRING(s, s->strstart, &hash_head);
#endif
		} else {
			if (s->lookahead >= MIN_MATCH) {
#ifndef NO_CONFLICT_HASH
				INSERT_STRING(s, s->strstart - 5, hash_head);
				INSERT_STRING(s, s->strstart - 4, hash_head);
				INSERT_STRING(s, s->strstart - 3, hash_head);
				INSERT_STRING(s, s->strstart - 2, hash_head);
				INSERT_STRING(s, s->strstart - 1, hash_head);
				INSERT_STRING(s, s->strstart, hash_head);
#else
				INSERT_STRING(s, s->strstart - 5, &hash_head);
				INSERT_STRING(s, s->strstart - 4, &hash_head);
				INSERT_STRING(s, s->strstart - 3, &hash_head);
				INSERT_STRING(s, s->strstart - 2, &hash_head);
				INSERT_STRING(s, s->strstart - 1, &hash_head);
				INSERT_STRING(s, s->strstart, &hash_head);
#endif
			}
		}

		/* Find the longest match, discarding those <= prev_length.
		 */
		s->prev_length = s->match_length;
		s->prev_match = s->match_start;
		s->match_length = 0;
		uInt char_flag = 0;
		if (hash_head != NIL && s->strstart - hash_head <= MAX_DIST(s)) {
			/* To simplify the code, we prevent matches with the string
			 * of window index 0 (in particular we have to avoid a match
			 * of the string with itself at the start of the input file).
			 */

			jump_back_num_tmp = 1;

			s->match_length = longest_match(s, hash_head);

			/*for static*/
			if(jump_back_num_tmp > (int)s->max_chain_length)
				jump_back_num_tmp =  (int)s->max_chain_length;
			g_total_match += jump_back_num_tmp;
		}

		if (s->prev_length >= 6 && s->prev_length % 6 == 0
				&& (s->prev_match + s->prev_length != s->match_start
					|| s->prev_length == 258)) {
			if(s->strstart < s->prev_length+s->prev_match )
				s->prev_match -= (1<<MAX_WBITS);
			_tr_tally_dist(s, s->strstart -s->prev_length - s->prev_match,
					s->prev_length - MIN_MATCH, bflush);
			match_length_value = s->prev_length;
			match_length[match_length_value]++;
			node_match_length[match_length_value] += jump_back_num;
			jump_back_num = 0;

			s->prev_length =  0;
			if (bflush) {
				get_lz_result(s, 0);
				bflush = 0;
			}
		}

		if (s->prev_length < 6) {
			if (s->match_length == 6) {
				s->lookahead -= 6;
				s->strstart  += 6;
				jump_back_num += jump_back_num_tmp;
			} else {
				uInt char_num = 6;
				if (hash_head == NIL || s->strstart - hash_head > MAX_DIST(s)) {
					g_not_need_jump_back++;
					jump_back_num = 0;
					char_num = 1;
					char_flag = 0;
				} else if (s->match_length >= MIN_MATCH) {
					_tr_tally_dist(s, s->strstart - s->match_start,
							s->match_length - MIN_MATCH, bflush);

					match_length_value = s->match_length;
					match_length[match_length_value]++;
					jump_back_num = jump_back_num_tmp;
					node_match_length[match_length_value] += jump_back_num;
					jump_back_num = 0;
					char_flag = 2;

					s->lookahead -= s->match_length;
					s->strstart  += s->match_length;
					char_num = 6 - s->match_length;
					s->match_length = 0;

					if (bflush) {
						get_lz_result(s, 0);
						bflush = 0;
					}
				} else {
					char_flag = 1;
					jump_back_num = 0;
					if (jump_back_num_tmp == 1)
						g_error_result++;
					else {
						g_length_number++;
						g_error_match += jump_back_num_tmp;
					}
				}

				for (uInt num = 0; num < char_num; ++num) {
					if (s->lookahead > 0) {
						_tr_tally_lit(s, s->window[s->strstart], bflush);
						match_length[char_flag]++;

						s->strstart++;
						s->lookahead--;
						if (bflush) {
							get_lz_result(s, 0);
							bflush = 0;
						}
					}
				}
			}
		} else {
			jump_back_num += jump_back_num_tmp;
			if (s->match_length == 6) {
				s->lookahead -= 6;
				s->strstart  += 6;
				s->match_length += s->prev_length;
				s->match_start  -= s->prev_length;
			} else {
				s->prev_length += s->match_length;
				if (s->strstart < (s->prev_length-s->match_length) + s->prev_match)
					s->prev_match -= (1<<MAX_WBITS);
				_tr_tally_dist(s, s->strstart - (s->prev_length-s->match_length)
						- s->prev_match,
						s->prev_length - MIN_MATCH, bflush);
				match_length_value = s->prev_length;
				match_length[match_length_value]++;
				node_match_length[match_length_value] += jump_back_num;
				jump_back_num = 0;
				char_flag = 2;

				if (bflush) {
					get_lz_result(s, 0);
					bflush = 0;
				}
				s->lookahead -= s->match_length;
				s->strstart  += s->match_length;
				for (uInt num = 0; num < 6 - s->match_length; ++num) {
					if (s->lookahead > 0) {
						_tr_tally_lit(s, s->window[s->strstart], bflush);
						match_length[char_flag]++;
						if (bflush) {
							get_lz_result(s, 0);
							bflush = 0;
						}
						s->strstart++;
						s->lookahead--;
					}
				}
			}
		}
	}

	s->insert = s->strstart < MIN_MATCH-1 ?\
		    s->strstart : MIN_MATCH - 1;
	if (flush == Z_FINISH) {
		get_lz_result(s, 1);
		bflush = 0;
		return finish_done;
	}

	if (s->last_lit) {
		get_lz_result(s, 0);
		bflush = 0;
	}

	pr_err("can not be here!\n");
	return block_done;
}
#else
#ifndef LAST_OPTMIZATION
block_state deflate_slow(deflate_state *s, int flush)
{
	uInt hash_head = NIL;  /* head of hash chain */
	int  bflush;           /* set if current block must be flushed */
	uInt need_deal_num = 0;

	/* Process the input block. */
	for (;;) {
		/* Make sure that we always have enough lookahead, except
		 * at the end of the input file. We need MAX_MATCH bytes
		 * for the next match, plus MIN_MATCH bytes to insert the
		 * string following the next match.
		 */
		if (s->lookahead < MIN_LOOKAHEAD) {
			fill_window(s);
			if (s->lookahead < MIN_LOOKAHEAD && flush == Z_NO_FLUSH)
				return need_more;

			if (s->lookahead == 0)
				break; /* flush the current block */
		}

		/* Insert the string window[strstart .. strstart+2] in the
		 * dictionary, and set hash_head to the head of the hash chain:
		 */

		if (hash_head == NIL || s->strstart - hash_head > MAX_DIST(s)) {
#ifndef NO_CONFLICT_HASH
			INSERT_STRING(s, s->strstart-2, hash_head);
			INSERT_STRING(s, s->strstart-1, hash_head);
			INSERT_STRING(s, s->strstart, hash_head);
#else
			INSERT_STRING(s, s->strstart, &hash_head);
#endif
		} else {
			if (s->lookahead >= MIN_MATCH) {
#ifndef NO_CONFLICT_HASH
				INSERT_STRING(s, s->strstart - 5, hash_head);
				INSERT_STRING(s, s->strstart - 4, hash_head);
				INSERT_STRING(s, s->strstart - 3, hash_head);
				INSERT_STRING(s, s->strstart - 2, hash_head);
				INSERT_STRING(s, s->strstart - 1, hash_head);
				INSERT_STRING(s, s->strstart, hash_head);
#else
				INSERT_STRING(s, s->strstart - 5, &hash_head);
				INSERT_STRING(s, s->strstart - 4, &hash_head);
				INSERT_STRING(s, s->strstart - 3, &hash_head);
				INSERT_STRING(s, s->strstart - 2, &hash_head);
				INSERT_STRING(s, s->strstart - 1, &hash_head);
				INSERT_STRING(s, s->strstart, &hash_head);
#endif
			}
		}


		/* Find the longest match,
		   discarding those <= prev_length.*/
		s->match_length = 0;
		uInt char_flag = 0;
		jump_back_num_tmp = 0;
		if (hash_head != NIL && s->strstart - hash_head <= MAX_DIST(s)) {
			/* To simplify the code, we prevent matches with the string
			 * of window index 0 (in particular we have to avoid a match
			 * of the string with itself at the start of the input file).
			 */

			jump_back_num_tmp = 1;

			s->match_length = longest_match (s, hash_head);

			/*for static*/
			if (jump_back_num_tmp > (int)s->max_chain_length)
				jump_back_num_tmp =  (int)s->max_chain_length;
			g_total_match += jump_back_num_tmp;

			if (0 == need_deal_num) {
				/*save output*/
				s->prev_length = s->match_length;
				s->prev_match = s->match_start;
				need_deal_num = s->match_length / 6
					+ ((s->match_length % 6) == 0?0:1);

			}

			if (0 == need_deal_num)
				need_deal_num =1;
		}

		if (need_deal_num == 0) {
			g_not_need_jump_back++;/*hash not exits*/
			char_flag = 0;/*empty hash*/
			jump_back_num = 0;
			for (uInt num = 0; num < 1; ++num) {
				if (s->lookahead > 0) {
					_tr_tally_lit(s, s->window[s->strstart], bflush);
					match_length[char_flag]++;
					if (bflush) {
						get_lz_result(s, 0);
						bflush = 0;
					}

					s->strstart++;
					s->lookahead--;
				}
			}
		} else if (need_deal_num == 1) {
			uInt tmp_compare_num = (s->prev_length <= 6)?s->prev_length: \
					       s->prev_length - (s->prev_length - 1) / 6 * 6;
			uInt jump_num = 6;
			if (tmp_compare_num + 3 >= s->match_length) {
				jump_back_num += jump_back_num_tmp;
				if (s->prev_length >= MIN_MATCH) {
					uInt dis_tmp =(s->prev_length - 1) / 6 * 6;

					if (s->strstart < s->prev_match)
						s->prev_match -= (1 << MAX_WBITS);
					_tr_tally_dist(s, s->strstart - dis_tmp - s->prev_match,
							s->prev_length - MIN_MATCH, bflush);
					/*for static*/
					match_length_value = s->prev_length;
					match_length[match_length_value]++;
					node_match_length[match_length_value] += jump_back_num;
					jump_back_num = 0;

					if (bflush) {
						get_lz_result(s, 0);
						bflush = 0;
					}

					s->lookahead -= (s->prev_length - dis_tmp);
					s->strstart  += (s->prev_length - dis_tmp);
					jump_num = 6 - (s->prev_length - dis_tmp);
					char_flag = 2;/*distance*/
				} else {
					/*error match*/
					if (jump_back_num_tmp == 1)
						g_error_result++;/*not match :old nodes*/
					else {
						g_length_number++;/*error match num*/
						g_error_match += jump_back_num;/*error macth nodes*/
					}

					char_flag = 1;/*error match*/
					jump_back_num = 0;
				}
			} else {
				uInt dis_tmp = (s->prev_length - 1) / 6 * 6;
				if(s->strstart < s->prev_match )
					s->prev_match -= (1 << MAX_WBITS);
				_tr_tally_dist(s, s->strstart - dis_tmp - s->prev_match,
						dis_tmp - MIN_MATCH, bflush);
				/*for static*/
				match_length_value = dis_tmp;
				match_length[match_length_value]++;
				node_match_length[match_length_value] += jump_back_num;
				jump_back_num = jump_back_num_tmp;

				if (bflush) {
					get_lz_result(s, 0);
					bflush = 0;
				}

				s->prev_length = 0;

				/*deal the current match result*/
				if (s->match_length <= 6) {
					if (s->match_length >= MIN_MATCH) {
						_tr_tally_dist(s, s->strstart - s->match_start,
								s->match_length - MIN_MATCH, bflush);
						/*for static*/
						match_length_value = s->match_length;
						match_length[match_length_value]++;
						node_match_length[match_length_value] += jump_back_num;

						char_flag = 2;/*distance*/
					} else {
						/*error match*/
						if (jump_back_num_tmp == 1) {
							g_error_result++;/*not match :old nodes*/
						} else {
							g_length_number++;/*error match num*/
							g_error_match += jump_back_num;/*error macth nodes*/
						}

						char_flag = 1;/*error match*/
						jump_back_num = 0;
					}

					s->strstart += s->match_length;
					s->lookahead -= s->match_length;
					jump_num = 6 - s->match_length;
					jump_back_num = 0;
				} else {
					s->prev_length = s->match_length;
					s->prev_match = s->match_start;
					need_deal_num = s->match_length / 6
						+ ((s->match_length % 6) == 0?0:1);
					jump_num = 0;
					s->strstart += 6;
					s->lookahead -= 6;
				}
			}

			need_deal_num--;
			for (uInt num = 0; num < jump_num; ++num) {
				if (s->lookahead > 0) {
					_tr_tally_lit(s, s->window[s->strstart], bflush);
					match_length[char_flag]++;
					if (bflush) {
						get_lz_result(s, 0);
						bflush = 0;
					}

					s->strstart++;
					s->lookahead--;
				}
			}
		} else {
			s->strstart += 6;
			s->lookahead -= 6;
			need_deal_num--;
			jump_back_num += jump_back_num_tmp;
		}
	}

	s->insert = s->strstart < MIN_MATCH-1 ? s->strstart : MIN_MATCH-1;

	if (flush == Z_FINISH) {
		get_lz_result(s, 1);
		bflush = 0;
		return finish_done;
	}

	if (s->last_lit) {
		get_lz_result(s, 0);
		bflush = 0;
	}

	pr_err("can not be here!\n");
	return block_done;
}
#else
block_state deflate_slow(deflate_state *s, int flush)
{
	uInt hash_head = NIL; /* head of hash chain */
	int  bflush;          /* set if current block must be flushed */
	uInt need_deal_num = 0;
	uInt one_step_num = 6;

	/* Process the input block. */
	for (;;) {
		/* Make sure that we always have enough lookahead, except
		 * at the end of the input file. We need MAX_MATCH bytes
		 * for the next match, plus MIN_MATCH bytes to insert the
		 * string following the next match.
		 */
		if (s->lookahead < MIN_LOOKAHEAD) {
			fill_window(s);
			if (s->lookahead < MIN_LOOKAHEAD
					&& flush == Z_NO_FLUSH) {
				return need_more;
			}

			if (s->lookahead == 0)
				break; /* flush the current block */
		}

		/* Insert the string window[strstart .. strstart+2] in the
		 * dictionary, and set hash_head to the head of the hash chain:
		 */

		if (hash_head == NIL || s->strstart - hash_head > MAX_DIST(s)) {
#ifndef NO_CONFLICT_HASH
			INSERT_STRING(s, s->strstart-1, hash_head);
			INSERT_STRING(s, s->strstart, hash_head);
#else
			INSERT_STRING(s, s->strstart-1, &hash_head);
			INSERT_STRING(s, s->strstart, &hash_head);
#endif
		} else {
			if (s->lookahead >= MIN_MATCH) {
#ifndef NO_CONFLICT_HASH
				INSERT_STRING(s, s->strstart - 5, hash_head);
				INSERT_STRING(s, s->strstart - 4, hash_head);
				INSERT_STRING(s, s->strstart - 3, hash_head);
				INSERT_STRING(s, s->strstart - 2, hash_head);
				INSERT_STRING(s, s->strstart - 1, hash_head);
				INSERT_STRING(s, s->strstart, hash_head);
#else
				INSERT_STRING(s, s->strstart - 5, &hash_head);
				INSERT_STRING(s, s->strstart - 4, &hash_head);
				INSERT_STRING(s, s->strstart - 3, &hash_head);
				INSERT_STRING(s, s->strstart - 2, &hash_head);
				INSERT_STRING(s, s->strstart - 1, &hash_head);
				INSERT_STRING(s, s->strstart, &hash_head);
#endif
			}
		}

		/* Find the longest match, discarding those <= prev_length.
		 */
		s->match_length = 0;
		uInt char_flag = 0;
		jump_back_num_tmp = 0;
		if (hash_head != NIL && s->strstart - hash_head <= MAX_DIST(s)) {
			/* To simplify the code, we prevent matches with the string
			 * of window index 0 (in particular we have to avoid a match
			 * of the string with itself at the start of the input file).
			 */

			jump_back_num_tmp = 1;

			s->match_length = longest_match (s, hash_head);

			/*for static*/
			jump_back_num_tmp = (int)s->max_chain_length;
			g_total_match += jump_back_num_tmp;

			if (0 == need_deal_num) {
				/*save output*/
				s->prev_length = s->match_length;
				s->prev_match = s->match_start;
				need_deal_num = s->match_length / one_step_num
					+ ((s->match_length % one_step_num) == 0?0:1);

			}

			if (0 == need_deal_num)
				need_deal_num = 1;
		}

		if (need_deal_num == 0) {
			g_not_need_jump_back++;
			char_flag = 0;
			jump_back_num = 0;
			for (uInt num = 0; num < 2; ++num) {
				if (s->lookahead > 0) {
					_tr_tally_lit(s, s->window[s->strstart], bflush);
					match_length[char_flag]++;
					if (bflush) {
						get_lz_result(s, 0);
						bflush = 0;
					}

					s->strstart++;
					s->lookahead--;
				}
			}
		} else if (need_deal_num == 1) {
			uInt tmp_compare_num = (s->prev_length <= one_step_num)?s->prev_length:\
					       s->prev_length - (s->prev_length - 1) /
					       one_step_num * one_step_num;
			uInt jump_num = one_step_num;

			if (tmp_compare_num+3 >= s->match_length) {
				jump_back_num += jump_back_num_tmp;
				if (s->prev_length >= MIN_MATCH) {
					uInt dis_tmp =(s->prev_length - 1) /
						one_step_num * one_step_num;
					if (s->strstart < s->prev_match)
						s->prev_match -= (1 << MAX_WBITS);
					_tr_tally_dist(s, s->strstart - dis_tmp - s->prev_match,
							s->prev_length - MIN_MATCH, bflush);
					/*for static*/
					match_length_value = s->prev_length;
					match_length[match_length_value]++;
					node_match_length[match_length_value] += jump_back_num;
					jump_back_num = 0;

					if (bflush) {
						get_lz_result(s, 0);
						bflush = 0;
					}

					s->lookahead -= (s->prev_length - dis_tmp);
					s->strstart  += (s->prev_length - dis_tmp);
					jump_num = one_step_num - (s->prev_length - dis_tmp);
					char_flag = 2;/*distance*/
				} else {
					/*error match*/
					if (jump_back_num_tmp == 1) {
						g_error_result++;/*not match :old nodes*/
					} else {
						g_length_number++;/*error match num*/
						g_error_match += jump_back_num;/*error macth nodes*/
					}

					char_flag = 1;/*error match*/
					jump_back_num = 0;
				}
			} else {
				uInt dis_tmp = (s->prev_length - 1) / one_step_num
					* one_step_num;
				if (s->strstart < s->prev_match)
					s->prev_match -= (1 << MAX_WBITS);
				_tr_tally_dist(s, s->strstart - dis_tmp - s->prev_match,
						dis_tmp - MIN_MATCH, bflush);
				/*for static*/
				match_length_value = dis_tmp;
				match_length[match_length_value]++;
				node_match_length[match_length_value] += jump_back_num;
				jump_back_num = jump_back_num_tmp;

				if (bflush) {
					get_lz_result(s, 0);
					bflush = 0;
				}

				s->prev_length = 0;

				/*deal the current match result*/
				if (s->match_length <= one_step_num) {
					if (s->match_length >= MIN_MATCH) {
						_tr_tally_dist(s, s->strstart- s->match_start,
								s->match_length - MIN_MATCH, bflush);
						if (bflush) {
							get_lz_result(s, 0);
							bflush = 0;
						}

						/*for static*/
						match_length_value = s->match_length;
						match_length[match_length_value]++;
						node_match_length[match_length_value] += jump_back_num;
						char_flag = 2;/*distance*/
					} else {
						if (jump_back_num_tmp == 1) {
							g_error_result++;
						} else {
							g_length_number++;
							g_error_match += jump_back_num;
						}

						char_flag = 1;/*error match*/
						jump_back_num = 0;
					}

					s->strstart += s->match_length;
					s->lookahead -= s->match_length;
					jump_num = one_step_num
						- s->match_length;
					jump_back_num = 0;
				} else {
					s->prev_length = s->match_length;
					s->prev_match = s->match_start;
					need_deal_num = s->match_length
						/ one_step_num
						+ ((s->match_length
							% one_step_num)
						== 0?0:1);
					jump_num = 0;
					s->strstart += one_step_num;
					s->lookahead -= one_step_num;
				}
			}

			need_deal_num--;
			for (uInt num = 0; num < jump_num; ++num) {
				if (s->lookahead > 0) {
					_tr_tally_lit(s, s->window[s->strstart],
							bflush);
					match_length[char_flag]++;
					if (bflush) {
						get_lz_result(s, 0);
						bflush = 0;
					}
					s->strstart++;
					s->lookahead--;
				}
			}
		} else {
			s->strstart += one_step_num;
			s->lookahead -= one_step_num;
			need_deal_num--;
			jump_back_num += jump_back_num_tmp;
		}
	}
	s->insert = s->strstart < MIN_MATCH-1 ? s->strstart : MIN_MATCH-1;
	if (flush == Z_FINISH) {
		get_lz_result(s, 1);
		bflush = 0;
		return finish_done;
	}
	if (s->last_lit) {
		get_lz_result(s, 0);
		bflush = 0;
	}
	printf("ERROR: can not be here!\n");
	return block_done;
}
#endif
#endif
#endif

int ZEXPORT deflate_lz(z_streamp_gz strm, int flush)
{
	int old_flush; /* value of flush param
			  for previous deflate call */
	deflate_state *s;
	block_state bstate;

	if (strm == Z_NULL || strm->state == Z_NULL ||
			flush > Z_BLOCK || flush < 0) {
		return Z_STREAM_ERROR;
	}
	s = strm->state;

	if ((strm->next_in == Z_NULL && strm->avail_in != 0) ||
			(s->status == FINISH_STATE && flush != Z_FINISH)) {
		/*ERR_RETURN(strm, Z_STREAM_ERROR);*/
		return Z_STREAM_ERROR;
	}

	s->strm = strm; /* just in case */
	old_flush = s->last_flush;
	s->last_flush = flush;

	/* Write the header */
	if (s->status == INIT_STATE)
		s->status = BUSY_STATE;

	if (strm->avail_in == 0
			&& RANK(flush) <= RANK(old_flush)
			&& flush != Z_FINISH) {
		/*ERR_RETURN(strm, Z_BUF_ERROR);*/
		return Z_BUF_ERROR;
	}

	/* User must not provide more input after the first FINISH: */
	if (s->status == FINISH_STATE && strm->avail_in != 0) {
		/*ERR_RETURN(strm, Z_BUF_ERROR);*/
		return Z_BUF_ERROR;
	}

	/* Start a new block or continue the current one.
	 */
	if (strm->avail_in != 0 || s->lookahead != 0 ||
			(flush != Z_NO_FLUSH && s->status != FINISH_STATE)) {

		/*RM, modified for logic only use
		  deflate_slow and deflate_fast*/
		if (s->strategy == Z_HUFFMAN_ONLY
				|| s->strategy == Z_RLE
				|| s->level <= 0) {
			pr_err("STRATEGY ,logic only use deflate slow!\n");
			return Z_STREAM_ERROR;
		}

#ifdef NO_CONFLICT_HASH
		for (p_i = 0; p_i < (1 << (DEF_MEM_LEVEL + 7)); p_i++)
			hash2[p_i] = 0;
#endif

		bstate = (*(gzip_configuration_table[s->level].func))(s, flush);

		if (bstate == finish_started || bstate == finish_done)
			s->status = FINISH_STATE;

		if (bstate == need_more || bstate == finish_started) {
			/* avoid BUF_ERROR next call, see above */
			if (strm->avail_out == 0)
				s->last_flush = -1;

			return Z_OK;
		}

		if (bstate == block_done) {
			pr_err("can not be here, because flush is const Z_FINISH!\n");
			return Z_STREAM_ERROR;
		}
	}

	(strm->inter)->block_num = (s->l_buf - (strm->inter)->l_buf)
					/ (HUFFMAN_BLOCK_SIZE) + 1;
	(strm->inter)->last_lit  = s->last_lit;
	if ((strm->inter)->block_num > HUFFMAN_BLOCK_NUM) {
		pr_err("the block num error!\n");
		return Z_STREAM_ERROR;
	}
#ifdef LAST_OPTMIZATION
	spliceResult(strm);
	(strm->inter)->block_num = (s->l_buf - (strm->inter)->l_buf)
					/ (HUFFMAN_BLOCK_SIZE) + 1;
	(strm->inter)->last_lit  = s->last_lit;

	if ((strm->inter)->block_num > HUFFMAN_BLOCK_NUM) {
		pr_err("the block num error!\n");
		return Z_STREAM_ERROR;
	}

	fprintf(static_file, "\n");
	for (int p_i = 0; p_i < 259; p_i++)
		pr_err("match_length[%8d] = %14d\n",
				p_i, match_length[p_i]);
#endif
	return Z_STREAM_END;
}

/*the char and len/distance need to >= 8160,
 *watch the line of lz result: need to be the times of 8
 */
int getBufDatas(deflate_state *s, unsigned int *lz_result,
		struct def_lz_rm *drm, struct def_huf_rm *hrm)
{
	unsigned int  i = 0;
	uInt next_char;
	unsigned int flag;
	unsigned int lit_num;
	unsigned int dis_num;
	unsigned int total_deal_in = 0;
	unsigned long lz_result_line = hrm->next_line_num;

	s->last_lit = 8160;

	for (; i < s->last_lit
		|| (i >= s->last_lit && lz_result_line % 8 != 0);) {
		if (lz_result_line+3 == drm->lz_result_line) {
			i += (1 << 31);/*the end of file*/
			break;
		}

		flag    = lz_result[lz_result_line] >> 30;
		lit_num = (lz_result[lz_result_line] >> 24) & 0x03;
		dis_num = (lz_result[lz_result_line] >> 26) & 0x03;
		if (flag == 0) {
			if (lit_num != dis_num || lit_num == 0) {
				pr_err("lit_num = 0x%x, dis_num = 0x%x\n",
						lit_num, dis_num);
				pr_err("lz result is error!\n");
				return 1;
			}

			next_char = lz_result[lz_result_line];
			while (lit_num != 0) {
				s->l_buf[i] = (next_char & 0xff);
				s->d_buf[i] = 0;
				next_char = next_char >> 8;
				total_deal_in++;
				lit_num--;
				i++;
			}
		} else if (flag == 1) {
			if (dis_num != 2 || lit_num != 1) {
				pr_err("lz result is error!\n");
				return 1;
			}

			s->l_buf[i] = (uch)(lz_result[lz_result_line] & 0x1ff)
					- 3;
			s->d_buf[i] = (ush)((lz_result[lz_result_line] >> 9)
					& 0x7fff);
			total_deal_in += s->l_buf[i] + 3;
			i++;
		} else {
			pr_err("lz result is error!\n");
			return 1;
		}

		lz_result_line++;
	}

	s->last_lit = i;
	if (((lz_result[lz_result_line-1] >> 29) & 0x01) == 1) {
		if ((lz_result[lz_result_line] >> 29) != 7) {
			pr_err("lz result crc is error!\n");
			return 1;
		}

		lz_result_line++;
		if ((lz_result[lz_result_line] >> 29) != 7) {
			pr_err("lz result crc is error!\n");
			return 1;
		}

		lz_result_line++;
		if ((lz_result[lz_result_line] >> 29) != 5) {
			pr_err("lz result input file size is error!\n");
			return 1;
		}

		lz_result_line++;
	}

	hrm->next_line_num = lz_result_line;
	hrm->block_start += total_deal_in;
	return 0;
}

/*modified
 *the char and len/distance need to >= 8160,
 *watch the line of lz result: need to be the times of 8
 */
int ZEXPORT deflate_huf_block(unsigned char *in, unsigned int *lz_result,
		struct def_lz_rm *drm, struct def_huf_rm *hrm)
{
	unsigned int i;
	unsigned int j;
	int last;
	z_stream_gz zcpr;
	int ret = Z_OK;
	deflate_state *s;

	if (in == NULL || lz_result == NULL || drm == NULL || hrm == NULL)
		return Z_STREAM_ERROR;

	memset(&zcpr, 0, sizeof(z_stream_gz));

	deflateInit(&zcpr, 6); /*level is not really used here*/
	if (zcpr.inter == NULL) {
		deflateEnd(&zcpr);
		return Z_STREAM_ERROR;
	}

	zcpr.next_in         = in;
	zcpr.avail_in        = drm->in_file_size;
	zcpr.next_out        = hrm->def_out;
	zcpr.avail_out       = DEFLATE_BLOCK_SIZE;
	zcpr.total_in        = drm->total_in;
	zcpr.adler           = drm->crc_out;

	s  = zcpr.state;
	s->bi_buf         =   hrm->bi_buf;
	s->bi_valid       =   hrm->bi_valid;
	s->btype          =   &hrm->btype;
	s->bl_desc.heap   =   hrm->bl_heap;
	s->l_desc.heap    =   hrm->ll_heap;
	s->d_desc.heap    =   hrm->d_heap;
	init_block(s);
	TRY_FREE(&zcpr, zcpr.state->window);

	last = 0;

	/*get window*/
	s->block_start = hrm->block_start;
	s->window = &in[s->block_start];

	/*get l_buf d_buf*/
	ret = getBufDatas(s, lz_result, drm, hrm);
	s->strstart = hrm->block_start - s->block_start;
	s->block_start = 0;
	last = (s->last_lit >> 31) & 0x01;
	s->last_lit -= (s->last_lit & 0x80000000);
	if (ret != 0) {
		deflateEnd(&zcpr);
		return Z_STREAM_END;
	}

	/*get frequency*/
	for (j = 0; j < s->last_lit; ++j) {
		uch len  = s->l_buf[j];
		ush dist = s->d_buf[j];

		if (dist == 0) {
			s->dyn_ltree[len].Freq++;
		} else {
			s->dyn_ltree[_length_code[len] + LITERALS + 1].Freq++;
			s->dyn_dtree[d_code(dist-1)].Freq++;
		}
	}

	FLUSH_BLOCK_ONLY(s, last);
	hrm->bi_buf   =   s->bi_buf;
	hrm->bi_valid =   s->bi_valid;

	for (i = 0; i < L_CODES; ++i)
		hrm->table_code[i] = (s->dyn_ltree[i].fc.code << 16)
			+ (s->dyn_ltree[i].dl.len);
	for (i = 0; i < D_CODES; ++i)
		hrm->table_code[i+L_CODES] = (s->dyn_dtree[i].fc.code<<16)
			+ (s->dyn_dtree[i].dl.len);
	for (i = 0; i < BL_CODES; ++i)
		hrm->table_code[i+L_CODES+D_CODES] = (s->bl_tree[i].fc.code<<16)
			+ (s->bl_tree[i].dl.len);

	if (s->strm->avail_out == 0) {
		hrm->def_out_lens = s->strm->total_out;
		hrm->total_out += s->strm->total_out;
		pr_err("need more space!\n");
		deflateEnd(&zcpr);
		return Z_OK;
	}

	(hrm->cur_block_num)++;

	if (last) {
		if (hrm->next_line_num != drm->lz_result_line) {
			hrm->def_out_lens = s->strm->total_out;
			hrm->total_out += s->strm->total_out;
			pr_err("the total lz result line number is error!\n");
			deflateEnd(&zcpr);
			return Z_STREAM_ERROR;
		}

		Assert(zcpr.avail_out > 0, "bug2");
		if (s->wrap <= 0) {
			hrm->def_out_lens = s->strm->total_out;
			hrm->total_out += s->strm->total_out;
			deflateEnd(&zcpr);
			return Z_STREAM_END;
		}

		/* Write the trailer */
		if (s->wrap == 2) {
			put_byte(s, (Byte)(zcpr.adler & 0xff));
			put_byte(s, (Byte)((zcpr.adler >> 8) & 0xff));
			put_byte(s, (Byte)((zcpr.adler >> 16) & 0xff));
			put_byte(s, (Byte)((zcpr.adler >> 24) & 0xff));
			put_byte(s, (Byte)(zcpr.total_in & 0xff));
			put_byte(s, (Byte)((zcpr.total_in >> 8) & 0xff));
			put_byte(s, (Byte)((zcpr.total_in >> 16) & 0xff));
			put_byte(s, (Byte)((zcpr.total_in >> 24) & 0xff));
		}

		flush_pending(&zcpr);

		/* If avail_out is zero, the application will call deflate again
		 * to flush the rest.
		 */
		if (s->wrap > 0)
			s->wrap = -s->wrap; /* write the trailer only once! */
		ret = s->pending;
		hrm->def_out_lens = s->strm->total_out;
		hrm->total_out += s->strm->total_out;
		deflateEnd(&zcpr);
		return ret != 0 ? Z_OK : Z_STREAM_END;
	}

	hrm->def_out_lens = s->strm->total_out;
	hrm->total_out += s->strm->total_out;
	deflateEnd(&zcpr);
	return Z_STREAM_END;
}

/* ===========================================================================
 * Initialize the "longest match" routines for a new zlib stream
 */
void lm_init(deflate_state *s)
{
	s->window_size = (uLong)2L * s->w_size;

	CLEAR_HASH(s);

	/* Set the default configuration parameters:
	 */
	s->max_lazy_match   = gzip_configuration_table[s->level].max_lazy;
	s->good_match       = gzip_configuration_table[s->level].good_length;
	s->nice_match       = gzip_configuration_table[s->level].nice_length;
	s->max_chain_length = gzip_configuration_table[s->level].max_chain;

	s->strstart = 0;
	s->block_start = 0L;
	s->lookahead = 0;
	s->insert = 0;
	s->match_length = s->prev_length = MIN_MATCH-1;
	s->match_available = 0;
	s->ins_h = 0;
}

/*==============================================================*/

/*===================tree functions=============================*/
#define send_bits(s, value, length) \
do { \
	int len = length;\
	if (s->bi_valid > (int)Buf_size - len) {\
		int val = value;\
		s->bi_buf |= (ush)val << s->bi_valid;\
		put_short(s, s->bi_buf);\
		s->bi_buf = (ush)val >> (Buf_size - s->bi_valid);\
		s->bi_valid += len - Buf_size;\
	} else {\
		s->bi_buf |= (ush)(value) << s->bi_valid;\
		s->bi_valid += len;\
	} \
} while (0)

#define send_code(s, c, tree) send_bits(s, tree[c].Code, tree[c].Len)

/* ===========================================================================
 * Initialize a new block.
 */
void init_block(deflate_state *s)
{
	int n; /* iterates over tree elements */

	/* Initialize the trees. */
	for (n = 0; n < L_CODES;  n++)
		s->dyn_ltree[n].Freq = 0;
	for (n = 0; n < D_CODES;  n++)
		s->dyn_dtree[n].Freq = 0;
	for (n = 0; n < BL_CODES; n++)
		s->bl_tree[n].Freq = 0;

	s->dyn_ltree[END_BLOCK].Freq = 1;
	s->opt_len = s->static_len = 0L;
	s->last_lit = s->matches = 0;
}

/* ===========================================================================
 * Initialize the tree data structures for a new zlib stream.
 */
void _tr_init(deflate_state *s)
{
	s->l_desc.dyn_tree = s->dyn_ltree;
	s->l_desc.stat_desc = &static_l_desc;

	s->d_desc.dyn_tree = s->dyn_dtree;
	s->d_desc.stat_desc = &static_d_desc;

	s->bl_desc.dyn_tree = s->bl_tree;
	s->bl_desc.stat_desc = &static_bl_desc;

	s->bi_buf = 0;
	s->bi_valid = 0;
#ifdef DEBUG
	s->compressed_len = 0L;
	s->bits_sent = 0L;
#endif

	/* Initialize the first block of the first file: */
	init_block(s);
}

/* ===========================================================================
 * Flush the bit buffer, keeping at most 7 bits in it.
 */
void bi_flush(deflate_state *s)
{
	if (s->bi_valid == 16) {
		put_short(s, s->bi_buf);
		s->bi_buf = 0;
		s->bi_valid = 0;
	} else if (s->bi_valid >= 8) {
		put_byte(s, (Byte)s->bi_buf);
		s->bi_buf >>= 8;
		s->bi_valid -= 8;
	}
}

/* ===========================================================================
 * Flush the bits in the bit buffer to pending output (leaves at most 7 bits)
 */
void _tr_flush_bits(deflate_state *s)
{
	bi_flush(s);
}

/* ===========================================================================
 * Send one empty static block to give enough lookahead for inflate.
 * This takes 10 bits, of which 7 may remain in the bit buffer.
 */
void _tr_align(deflate_state *s)
{
	send_bits(s, STATIC_TREES<<1, 3);
	send_code(s, END_BLOCK, static_ltree);
#ifdef DEBUG
	s->compressed_len += 10L; /* 3 for block type, 7 for EOB */
#endif
	bi_flush(s);
}

/* ===========================================================================
 * Flush the bit buffer and align the output on a byte boundary
 */
void bi_windup(deflate_state *s)
{
	if (s->bi_valid > 8) {
		put_short(s, s->bi_buf);
	} else if (s->bi_valid > 0)
		put_byte(s, (Byte)s->bi_buf);

	s->bi_buf = 0;
	s->bi_valid = 0;
#ifdef DEBUG
	s->bits_sent = (s->bits_sent+7) & ~7;
#endif
}

/* ===========================================================================
 * Copy a stored block, storing first the length and its
 * one's complement if requested.
 * the input data
 * its length
 * true if block header must be written
 */
void copy_block(deflate_state *s, char *buf, unsigned len, int header)
{
	bi_windup(s);        /* align on byte boundary */

	if (header) {
		put_short(s, (ush)len);
		put_short(s, (ush)~len);
#ifdef DEBUG
		s->bits_sent += 2 * 16;
#endif
	}
#ifdef DEBUG
	s->bits_sent += (uLong)len << 3;
#endif
	while (len--)
		put_byte(s, *buf++);
}

/* ===========================================================================
 * Send a stored block
 * input block
 * length of input block
 * one if this is the last block for a file
 */
void _tr_stored_block(deflate_state *s, char *buf, uLong stored_len, int last)
{
	send_bits(s, (STORED_BLOCK<<1)+last, 3);    /* send block type */
#ifdef DEBUG
	s->compressed_len = (s->compressed_len + 3 + 7) & (uLong)~7L;
	s->compressed_len += (stored_len + 4) << 3;
#endif
	copy_block(s, buf, (unsigned)stored_len, 1); /* with header */
}

/* ===========================================================================
 * Compares to subtrees, using the tree depth as tie breaker when
 * the subtrees have equal frequency. This minimizes the worst case length.
 */
#define smaller(tree, n, m, depth) \
	(tree[n].Freq < tree[m].Freq || \
	 (tree[n].Freq == tree[m].Freq && depth[n] <= depth[m]) ||\
	 (tree[n].Freq == tree[m].Freq && depth[n] == depth[m] \
	  && n < m))


/* ===========================================================================
 * Restore the heap property by moving down the tree starting at node k,
 * exchanging a node with the smallest of its two sons if necessary, stopping
 * when the heap property is re-established (each father smaller than its
 * two sons).
 * the tree to restore
 * node to move down
 */
void pqdownheap(deflate_state *s, ct_data *tree, int k)
{
	int v = s->heap[k];
	int j = k << 1;  /* left son of k */

	while (j <= s->heap_len) {
		/* Set j to the smallest of the two sons: */
		if (j < s->heap_len && smaller(tree, s->heap[j+1],
				s->heap[j], s->depth)) {
			j++;
		}
		/* Exit if v is smaller than both sons */
		if (smaller(tree, v, s->heap[j], s->depth))
			break;

		/* Exchange v with the smallest son */
		s->heap[k] = s->heap[j];
		k = j;

		/* And continue down the tree, setting j to the left son of k */
		j <<= 1;
	}
	s->heap[k] = v;
}

/* ===========================================================================
 * Remove the smallest element from the heap and recreate the heap with
 * one less element. Updates heap and heap_len.
 */
#define pqremove(s, tree, top) \
{\
	top = s->heap[SMALLEST]; \
	s->heap[SMALLEST] = s->heap[s->heap_len--];\
	pqdownheap(s, tree, SMALLEST);\
}

/* ===========================================================================
 * Compute the optimal bit lengths for a tree and update the total bit length
 * for the current block.
 * IN assertion: the fields freq and dad are set, heap[heap_max] and
 *    above are the tree nodes sorted by increasing frequency.
 * OUT assertions: the field len is set to the optimal bit length, the
 *     array bl_count contains the frequencies for each bit length.
 *     The length opt_len is updated; static_len is also updated if stree is
 *     not null.
 */
void gen_bitlen_tmp(deflate_state *s, tree_desc *desc)
{
	ct_data *tree        = desc->dyn_tree;
	int max_code         = desc->max_code;
	const ct_data *stree = desc->stat_desc->static_tree;
	const int *extra     = desc->stat_desc->extra_bits;
	int base             = desc->stat_desc->extra_base;
	int max_length       = desc->stat_desc->max_length;
	int h;              /* heap index */
	int n, m;           /* iterate over the tree elements */
	int bits;           /* bit length */
	int xbits;          /* extra bits */
	ush f;              /* frequency */
	int overflow = 0;   /* number of elements with bit length too large */
	int total_leave_nodes;
	int curr_bits;
	int curr_father_nodes;
	int need_father_nodes;
	int last_bl_count;

	for (bits = 0; bits <= MAX_BITS; bits++)
		s->bl_count[bits] = 0;

	/* In a first pass, compute the optimal bit lengths (which may
	 * overflow in the case of the bit length tree).
	 */
	tree[s->heap[s->heap_max]].Len = 0; /* root of the heap */
	total_leave_nodes = (HEAP_SIZE-s->heap_max+1)/2;
	curr_bits = 1;
	curr_father_nodes = 0;
	need_father_nodes = 0;
	last_bl_count = 0;

	for (h = s->heap_max+1; h < HEAP_SIZE; h++) {
		n = s->heap[h];
		bits = tree[tree[n].Dad].Len + 1;

		if (bits > max_length)
			bits = max_length;

		tree[n].Len = (ush)bits;

		/* We overwrite tree[n].Dad which is no longer needed */
		if (bits != curr_bits) {
			total_leave_nodes -= s->bl_count[curr_bits];
			need_father_nodes = total_leave_nodes /
				(1 << (15 - curr_bits));

			if (total_leave_nodes > need_father_nodes
					* (1 << (15 - curr_bits)))
				need_father_nodes++;

			if (need_father_nodes > curr_father_nodes) {
				overflow = 1;
				need_father_nodes -= curr_father_nodes;
				s->bl_count[curr_bits] -= need_father_nodes;
				s->bl_count[curr_bits+1] += 2*need_father_nodes;
				last_bl_count += need_father_nodes;
			}

			curr_father_nodes = 0;
			curr_bits = bits;
		}

		/* not a leaf node */
		if (n > max_code) {
			curr_father_nodes++;
			continue;
		}

		s->bl_count[bits]++;
		xbits = 0;

		if (n >= base)
			xbits = extra[n-base];

		f = tree[n].Freq;
		s->opt_len += (uLong)f * (bits + xbits);

		if (stree)
			s->static_len += (uLong)f * (stree[n].Len + xbits);
	}

	if (overflow == 0)
		return;
	s->bl_count[max_length] -= last_bl_count;

	Trace((stderr, "\nbit length overflow\n"));

	/* This happens for example on obj2 and pic of the Calgary corpus */
	/* Now recompute all bit lengths, scanning in increasing frequency.
	 * h is still equal to HEAP_SIZE. (It is simpler to reconstruct all
	 * lengths instead of fixing only the wrong ones. This idea is taken
	 * from 'ar' written by Haruhiko Okumura.)
	 */
	for (bits = max_length; bits != 0; bits--) {
		n = s->bl_count[bits];
		while (n != 0) {
			m = s->heap[--h];
			if (m > max_code)
				continue;
			if ((unsigned) tree[m].Len != (unsigned) bits) {
				Trace((stderr, "code %d bits %d->%d\n",
					m, tree[m].Len, bits));
				s->opt_len += ((long)bits - (long)tree[m].Len)
					* (long)tree[m].Freq;
				tree[m].Len = (ush)bits;
			}
			n--;
		}
	}
}

void gen_bitlen(deflate_state *s, tree_desc *desc)
{
	ct_data *tree        = desc->dyn_tree;
	int max_code         = desc->max_code;
	const ct_data *stree = desc->stat_desc->static_tree;
	const int *extra     = desc->stat_desc->extra_bits;
	int base             = desc->stat_desc->extra_base;
	int max_length       = desc->stat_desc->max_length;
	int h;              /* heap index */
	int n, m;           /* iterate over the tree elements */
	int bits;           /* bit length */
	int xbits;          /* extra bits */
	ush f;              /* frequency */
	int overflow = 0;   /* number of elements with bit length too large */

	for (bits = 0; bits <= MAX_BITS; bits++)
		s->bl_count[bits] = 0;

	/* In a first pass, compute the optimal bit lengths (which may
	 * overflow in the case of the bit length tree).
	 */
	tree[s->heap[s->heap_max]].Len = 0; /* root of the heap */

	for (h = s->heap_max + 1; h < HEAP_SIZE; h++) {
		n = s->heap[h];
		bits = tree[tree[n].Dad].Len + 1;
		if (bits > max_length) {
			bits = max_length;
			overflow++;
		}

		tree[n].Len = (ush)bits;

		/* We overwrite tree[n].Dad which
		   is no longer needed */
		if (n > max_code)/* not a leaf node */
			continue;

		s->bl_count[bits]++;
		xbits = 0;
		if (n >= base)
			xbits = extra[n-base];
		f = tree[n].Freq;
		s->opt_len += (uLong)f * (bits + xbits);
		if (stree)
			s->static_len += (uLong)f * (stree[n].Len + xbits);
	}
	if (overflow == 0)
		return;

	Trace((stderr, "\nbit length overflow\n"));
	/* This happens for example on obj2 and pic of the Calgary corpus */

	/* Find the first bit length which could increase: */
	do {
		bits = max_length-1;
		while (s->bl_count[bits] == 0)
			bits--;
		s->bl_count[bits]--;      /* move one leaf down the tree */
		s->bl_count[bits+1] += 2; /* move one overflow
					     item as its brother */
		s->bl_count[max_length]--;
		/* The brother of the overflow item also moves one step up,
		 * but this does not affect bl_count[max_length]
		 */
		overflow -= 2;
	} while (overflow > 0);

	/* Now recompute all bit lengths, scanning in increasing frequency.
	 * h is still equal to HEAP_SIZE. (It is simpler to reconstruct all
	 * lengths instead of fixing only the wrong ones. This idea is taken
	 * from 'ar' written by Haruhiko Okumura.)
	 */
	for (bits = max_length; bits != 0; bits--) {
		n = s->bl_count[bits];
		while (n != 0) {
			m = s->heap[--h];
			if (m > max_code)
				continue;
			if ((unsigned) tree[m].Len != (unsigned) bits) {
				Trace((stderr, "code %d bits %d->%d\n",
					m, tree[m].Len, bits));
				s->opt_len += ((long)bits - (long)tree[m].Len)
					*(long)tree[m].Freq;
				tree[m].Len = (ush)bits;
			}
			n--;
		}
	}
}

/* ===========================================================================
 * Reverse the first len bits of a code, using straightforward code (a faster
 * method would use a table)
 * IN assertion: 1 <= len <= 15
 * the value to invert
 * its bit length
 */
unsigned bi_reverse(unsigned code, int len)
{
	register unsigned res = 0;

	do {
		res |= code & 1;
		code >>= 1, res <<= 1;
	} while (--len > 0);
	return res >> 1;
}


/* ===========================================================================
 * Generate the codes for a given tree and bit counts (which need not be
 * optimal).
 * IN assertion: the array bl_count contains the bit length statistics for
 * the given tree and the field len is set for all tree elements.
 * OUT assertion: the field code is set for all tree elements of non
 *     zero code length.
 * the tree to decorate
 * largest code with non zero frequency
 * number of codes at each bit length
 */
void gen_codes(ct_data *tree, int max_code, ush *bl_count)
{
	ush next_code[MAX_BITS + 1]; /* next code value for each bit length */
	ush code = 0;              /* running code value */
	int bits;                  /* bit index */
	int n;                     /* code index */

	/* The distribution counts are first used to generate the code values
	 * without bit reversal.
	 */
	for (bits = 1; bits <= MAX_BITS; bits++) {
		code = (code + bl_count[bits - 1]) << 1;
		next_code[bits] = code;
	}

	/* Check that the bit counts in bl_count are consistent. The last code
	 * must be all ones.
	 */
	Assert(code + bl_count[MAX_BITS] - 1 == (1 << MAX_BITS) - 1,
			"inconsistent bit counts");
	Tracev((stderr, "\ngen_codes: max_code %d ", max_code));

	for (n = 0;  n <= max_code; n++) {
		int len = tree[n].Len;

		if (len == 0)
			continue;
		/* Now reverse the bits */
		tree[n].Code = bi_reverse(next_code[len]++, len);
		Tracecv(tree != static_ltree,
			(stderr, "\nn %3d %c l %2d c %4x (%x) ",
			n, (isgraph(n) ? n : ' '), len, tree[n].Code,
			next_code[len]-1));
	}
}

void sort_heap(deflate_state *s, ct_data *tree)
{
	int heap_idx = 1;
	int min_index = SMALLEST;
	int n = 0;

	while ((SMALLEST + heap_idx) <= s->heap_len) {
		if (s->heap[min_index] > s->heap[SMALLEST + heap_idx]
			&& tree[s->heap[min_index]].Freq ==
			tree[s->heap[SMALLEST + heap_idx]].Freq) {
			min_index = SMALLEST + heap_idx;
		}

		heap_idx++;
	}

	if (min_index != SMALLEST) {
		n = s->heap[min_index];
		s->heap[min_index] = s->heap[SMALLEST];
		s->heap[SMALLEST] = n;
	}
}

/* ===========================================================================
 * Construct one Huffman tree and assigns the code bit strings and lengths.
 * Update the total bit length for the current block.
 * IN assertion: the field freq is set for all tree elements.
 * OUT assertions: the fields len and code are set to the optimal bit length
 *     and corresponding code. The length opt_len is updated; static_len is
 *     also updated if stree is not null. The field max_code is set.
 */
void build_tree(deflate_state *s, tree_desc *desc)
{
	int i;
	ct_data *tree         = desc->dyn_tree;
	const ct_data *stree  = desc->stat_desc->static_tree;
	int elems             = desc->stat_desc->elems;
	int n, m;          /* iterate over heap elements */
	int max_code = -1; /* largest code with non zero frequency */
	int node;          /* new node being created */

	/* Construct the initial heap,
	 * with least frequent element in
	 * heap[SMALLEST]. The sons of heap[n]
	 * are heap[2*n] and heap[2*n+1].
	 * heap[0] is not used.
	 */
	s->heap_len = 0, s->heap_max = HEAP_SIZE;

	for (n = 0; n < elems; n++) {
		if (tree[n].Freq != 0) {
			s->heap[++(s->heap_len)] = max_code = n;
			s->depth[n] = 0;
		} else {
			tree[n].Len = 0;
		}
	}

	/* The pkzip format requires that at least
	 * one distance code exists,
	 * and that at least one bit
	 * should be sent even if there is only one
	 * possible code. So to avoid special
	 * checks later on we force at least
	 * two codes of non zero frequency.
	 */
	while (s->heap_len < 2) {
		s->heap[++(s->heap_len)] =
			(max_code < 2 ? ++max_code : 0);
		node = s->heap[++(s->heap_len)];
		tree[node].Freq = 1;
		s->depth[node] = 0;
		s->opt_len--;

		if (stree)
			s->static_len -= stree[node].Len;
		/* node is 0 or 1 so it does not have extra bits */
	}

	desc->max_code = max_code;

	/* The elements heap[heap_len/2+1 .. heap_len]
	 * are leaves of the tree,
	 * establish sub-heaps of increasing lengths:
	 */
	for (n = s->heap_len/2; n >= 1; n--)
		pqdownheap(s, tree, n);

	/* Construct the Huffman tree by repeatedly combining the least two
	 * frequent nodes.
	 */
	node = elems;/* next internal node of the tree */
	do {
		sort_heap(s, tree);

		/* n = node of least frequency */
		pqremove(s, tree, n);
		sort_heap(s, tree);

		/* m = node of next least frequency */
		m = s->heap[SMALLEST];

		/* keep the nodes sorted by frequency */
		s->heap[--(s->heap_max)] = n;
		s->heap[--(s->heap_max)] = m;

		/* Create a new node father of n and m */
		tree[node].Freq = tree[n].Freq + tree[m].Freq;
		s->depth[node] = (uch)((s->depth[n] >= s->depth[m] ?
					s->depth[n] : s->depth[m]) + 1);
		tree[n].Dad = tree[m].Dad = (ush)node;
#ifdef DUMP_BL_TREE
		if (tree == s->bl_tree) {
			pr_info("\nnode %d(%d), sons %d(%d) %d(%d)",
					node, tree[node].Freq, n,
					tree[n].Freq, m, tree[m].Freq);
		}
#endif
		/* and insert the new node in the heap */
		s->heap[SMALLEST] = node++;
		pqdownheap(s, tree, SMALLEST);

	} while (s->heap_len >= 2);

	s->heap[--(s->heap_max)] = s->heap[SMALLEST];

	/*add for RM*/
	i = 0;
	for (; i < HEAP_SIZE - 2 - s->heap_max; ++i)
		(desc->heap)[i] = (tree[s->heap[HEAP_SIZE-i-1]].fc.freq << 16)
			+ s->heap[HEAP_SIZE-i-1];
	for (; i < 2 * elems - 1; ++i)
		(desc->heap)[i] = 0;

	/* At this point, the fields freq and dad are set. We can now
	 * generate the bit lengths.
	 */
	gen_bitlen(s, (tree_desc *)desc);

	/* The field len is now set, we can generate the bit codes */
	gen_codes((ct_data *)tree, max_code, s->bl_count);
}

/* ============================================
 * Scan a literal or distance tree to determine
 * the frequencies of the codes
 * in the bit length tree.
 * the tree to be scanned and its largest code
 * of non zero frequency
 */
void scan_tree(deflate_state *s, ct_data *tree, int max_code)
{
	int n;                     /* iterates over all tree elements */
	int prevlen = -1;          /* last emitted length */
	int curlen;                /* length of current code */
	int nextlen = tree[0].Len; /* length of next code */
	int count = 0;             /* repeat count of the current code */
	int max_count = 7;         /* max repeat count */
	int min_count = 4;         /* min repeat count */

	if (nextlen == 0)
		max_count = 138, min_count = 3;

	tree[max_code+1].Len = (ush)0xffff; /* guard */

	for (n = 0; n <= max_code; n++) {
		curlen = nextlen; nextlen = tree[n + 1].Len;
		if (++count < max_count && curlen == nextlen)
			continue;
		else if (count < min_count)
			s->bl_tree[curlen].Freq += count;
		else if (curlen != 0) {
			if (curlen != prevlen)
				s->bl_tree[curlen].Freq++;
			s->bl_tree[REP_3_6].Freq++;
		} else if (count <= 10)
			s->bl_tree[REPZ_3_10].Freq++;
		else
			s->bl_tree[REPZ_11_138].Freq++;

		count = 0;
		prevlen = curlen;
		if (nextlen == 0) {
			max_count = 138;
			min_count = 3;
		} else if (curlen == nextlen) {
			max_count = 6;
			min_count = 3;
		} else {
			max_count = 7;
			min_count = 4;
		}
	}
}

/* ==============================================
 * Construct the Huffman tree for the bit lengths
 * and return the index in bl_order of the last
 * bit length code to send.
 */
int build_bl_tree(deflate_state *s)
{
	int max_blindex;  /* index of last bit length code
			     of non zero freq */

	/* Determine the bit length frequencies
	   for literal and distance trees */
	scan_tree(s, (ct_data *)s->dyn_ltree, s->l_desc.max_code);
	scan_tree(s, (ct_data *)s->dyn_dtree, s->d_desc.max_code);

	/* Build the bit length tree: */
	build_tree(s, (tree_desc *)(&(s->bl_desc)));
	/* opt_len now includes the length of the tree
	 * representations, except
	 * the lengths of the bit lengths codes and
	 * the 5+5+4 bits for the counts.
	 */

	/* Determine the number of bit length codes to send. The pkzip format
	 * requires that at least 4 bit length codes be sent. (appnote.txt says
	 * 3 but the actual value used is 4.)
	 */
	for (max_blindex = BL_CODES-1; max_blindex >= 3; max_blindex--) {
		if (s->bl_tree[bl_order[max_blindex]].Len != 0)
			break;
	}
	/* Update opt_len to include the bit length tree and counts */
	s->opt_len += 3*(max_blindex+1) + 5+5+4;
	Tracev((stderr, "\ndyn trees: dyn %ld, stat %ld",
				s->opt_len, s->static_len));

	return max_blindex;
}

/* ==============================================
 * Send the block data compressed using the given
 * Huffman trees * literal tree * distance tree
 */
void compress_block(deflate_state *s,
		const ct_data *ltree, const ct_data *dtree)
{
	unsigned dist;      /* distance of matched string */
	int lc;             /* match length or unmatched char (if dist == 0) */
	unsigned lx = 0;    /* running index in l_buf */
	unsigned code;      /* the code to send */
	int extra;          /* number of extra bits to send */

	if (s->last_lit != 0) {
		do {
			dist = s->d_buf[lx];
			lc = s->l_buf[lx++];
			if (dist == 0) {

				/* send a literal byte */
				send_code(s, lc, ltree);
				Tracecv(isgraph(lc),
					(stderr, " '%c' ", lc));
			} else {
				/* Here, lc is the match length - MIN_MATCH */
				code = _length_code[lc];

				/* send the length code */
				send_code(s, code+LITERALS + 1, ltree);
				extra = extra_lbits[code];
				if (extra != 0) {
					lc -= base_length[code];

					/* send the extra length bits */
					send_bits(s, lc, extra);
				}

				dist--; /* dist is now the match distance - 1 */
				code = d_code(dist);
				Assert(code < D_CODES, "bad d_code");

				/* send the distance code */
				send_code(s, code, dtree);
				extra = extra_dbits[code];
				if (extra != 0) {
					dist -= base_dist[code];

					/* send the extra distance bits */
					send_bits(s, dist, extra);
				}
			} /* literal or match pair ? */

			/* Check that the overlay between
			   pending_buf and d_buf+l_buf is ok: */
			Assert((uInt)(s->pending) < s->lit_bufsize + 2 * lx,
					"pendingBuf overflow");

		} while (lx < s->last_lit);
	}

	send_code(s, END_BLOCK, ltree);
}

/* ===========================================================================
 * Send a literal or distance tree in compressed form, using the codes in
 * bl_tree.* the tree to be scanned * and its largest code of non zero frequency
 */
void send_tree(deflate_state *s, ct_data *tree, int max_code)
{
	int n;                     /* iterates over all tree elements */
	int prevlen = -1;          /* last emitted length */
	int curlen;                /* length of current code */
	int nextlen = tree[0].Len; /* length of next code */
	int count = 0;             /* repeat count of the current code */
	int max_count = 7;         /* max repeat count */
	int min_count = 4;         /* min repeat count */

	/* tree[max_code+1].Len = -1; */
	/* guard already set */
	if (nextlen == 0)
		max_count = 138, min_count = 3;

	for (n = 0; n <= max_code; n++) {
		curlen = nextlen; nextlen = tree[n+1].Len;
		if (++count < max_count && curlen == nextlen) {
			continue;
		} else if (count < min_count) {
			do {
				send_code(s, curlen, s->bl_tree);
			} while (--count != 0);

		} else if (curlen != 0) {
			if (curlen != prevlen) {
				send_code(s, curlen, s->bl_tree);
				count--;
			}

			Assert(count >= 3 && count <= 6, " 3_6?");
			send_code(s, REP_3_6, s->bl_tree);
			send_bits(s, count-3, 2);

		} else if (count <= 10) {
			send_code(s, REPZ_3_10, s->bl_tree);
			send_bits(s, count-3, 3);

		} else {
			send_code(s, REPZ_11_138, s->bl_tree);
			send_bits(s, count-11, 7);
		}

		count = 0;
		prevlen = curlen;
		if (nextlen == 0) {
			max_count = 138;
			min_count = 3;
		} else if (curlen == nextlen) {
			max_count = 6;
			min_count = 3;
		} else {
			max_count = 7;
			min_count = 4;
		}
	}
}

/* ===========================================================================
 * Send the header for a block using dynamic Huffman trees: the counts, the
 * lengths of the bit length codes, the literal tree and the distance tree.
 * IN assertion: number of codes for each tree lcodes >= 257, dcodes >= 1,
 * blcodes >= 4.
 */
void send_all_trees(deflate_state *s, int lcodes, int dcodes, int blcodes)
{
	int rank;                    /* index in bl_order */

	Assert(lcodes >= 257 && dcodes >= 1 && blcodes >= 4,
			"not enough codes");
	Assert(lcodes <= L_CODES && dcodes <= D_CODES
			&& blcodes <= BL_CODES,
			"too many codes");
	Tracev((stderr, "\nbl counts: "));
	send_bits(s, lcodes-257, 5); /* not +255 as stated in appnote.txt */
	send_bits(s, dcodes-1,   5);
	send_bits(s, blcodes-4,  4); /* not -3 as stated in appnote.txt */
	for (rank = 0; rank < blcodes; rank++) {
		Tracev((stderr, "\nbl code %2d ", bl_order[rank]));
		send_bits(s, s->bl_tree[bl_order[rank]].Len, 3);
	}
	Tracev((stderr, "\nbl tree: sent %ld", s->bits_sent));

	send_tree(s, (ct_data *)s->dyn_ltree, lcodes-1); /* literal tree */
	Tracev((stderr, "\nlit tree: sent %ld", s->bits_sent));

	send_tree(s, (ct_data *)s->dyn_dtree, dcodes-1); /* distance tree */
	Tracev((stderr, "\ndist tree: sent %ld", s->bits_sent));
}

/* ===========================================================================
 * Determine the best encoding for the current block: dynamic trees, static
 * trees or store, and output the encoded block to the zip file.
 * input block, or NULL if too old
 * length of input block
 * one if this is the last block for a file
 */
void _tr_flush_block(deflate_state *s, char *buf, uLong stored_len, int last)
{
	uLong opt_lenb, static_lenb; /* opt_len and static_len in bytes */
	int max_blindex = 0;  /* index of last bit length
				 code of non zero freq */

	/* Construct the literal and distance trees */
	build_tree(s, (tree_desc *)(&(s->l_desc)));
	Tracev((stderr, "\nlit data: dyn %ld, stat %ld", s->opt_len,
				s->static_len));

	build_tree(s, (tree_desc *)(&(s->d_desc)));
	Tracev((stderr, "\ndist data: dyn %ld, stat %ld", s->opt_len,
				s->static_len));
	/* At this point, opt_len and static_len are the total bit lengths of
	 * the compressed block data, excluding the tree representations.
	 */

	/* Build the bit length tree for the above two trees, and get the index
	 * in bl_order of the last bit length code to send.
	 */
	max_blindex = build_bl_tree(s);

	/* Determine the best encoding. Compute the block lengths in bytes. */
	opt_lenb = (s->opt_len+3+7)>>3;
	static_lenb = (s->static_len+3+7)>>3;

	Tracev((stderr, "\nopt %lu(%lu) stat %lu(%lu) stored %lu lit %u ",
				opt_lenb, s->opt_len, static_lenb,
				s->static_len, stored_len,
				s->last_lit));

	if (static_lenb <= opt_lenb)
		opt_lenb = static_lenb;

	if (stored_len+4 <= opt_lenb && buf != (char *)0) {
		/* 4: two words for the lengths */
		/* The test buf != NULL is only necessary if
		 * LIT_BUFSIZE > WSIZE.
		 * Otherwise we can't have processed more than
		 * WSIZE input bytes since
		 * the last block flush, because compression would have been
		 * successful. If LIT_BUFSIZE <= WSIZE, it is never too late to
		 * transform a block into a stored block.
		 */
		*(s->btype) = 0;
		_tr_stored_block(s, buf, stored_len, last);

	} else if (s->strategy == Z_FIXED || static_lenb == opt_lenb) {
		*(s->btype) = 1;
		send_bits(s, (STATIC_TREES<<1)+last, 3);
		compress_block(s, (const ct_data *)static_ltree,
				(const ct_data *)static_dtree);
#ifdef DEBUG
		s->compressed_len += 3 + s->static_len;
#endif
	} else {
		*(s->btype) = 2;
		send_bits(s, (DYN_TREES<<1)+last, 3);
		send_all_trees(s, s->l_desc.max_code+1, s->d_desc.max_code+1,
				max_blindex+1);
		compress_block(s, (const ct_data *)s->dyn_ltree,
				(const ct_data *)s->dyn_dtree);
#ifdef DEBUG
		s->compressed_len += 3 + s->opt_len;
#endif
	}
	Assert(s->compressed_len == s->bits_sent, "bad compressed size");
	/* The above check is made mod 2^32, for files larger than 512 MB
	 * and uLong implemented on 32 bits.
	 */

	if (last) {
		bi_windup(s);
#ifdef DEBUG
		s->compressed_len += 7;  /* align on byte boundary */
#endif
	}
	Tracev((stderr, "\ncomprlen %lu(%lu) ", s->compressed_len >> 3,
				s->compressed_len - 7 * last));
}
/*==============================================================*/
