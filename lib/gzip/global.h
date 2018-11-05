#ifndef __GLOBAL_H__
#define __GLOBAL_H__
/* constants */

#define Z_NO_FLUSH      0
#define Z_PARTIAL_FLUSH 1
#define Z_SYNC_FLUSH    2
#define Z_FULL_FLUSH    3
#define Z_FINISH        4
#define Z_BLOCK         5
#define Z_TREES         6
/* Allowed flush values; see deflate()
   and inflate() below for details */

#define Z_OK            0
#define Z_STREAM_END    1
#define Z_NEED_DICT     2
#define Z_ERRNO        (-1)
#define Z_STREAM_ERROR (-2)
#define Z_DATA_ERROR   (-3)
#define Z_MEM_ERROR    (-4)
#define Z_BUF_ERROR    (-5)
#define Z_VERSION_ERROR (-6)

#define HUFFMAN_BLOCK_SIZE (8*1024)
#define HUFFMAN_BLOCK_NUM  (8*1024)
#define TOTAL_LENS         (2*512*1024)
#define DEFLATE_BLOCK_SIZE (HUFFMAN_BLOCK_SIZE * 3)

#define LENGTH_CODES 29
/* number of length codes, not counting
   the special END_BLOCK code */

#define LITERALS  256
/* number of literal bytes 0..255 */

#define L_CODES (LITERALS+1+LENGTH_CODES)
/* number of Literal or Length codes,
   including the END_BLOCK code */

#define D_CODES   30
/* number of distance codes */

#define BL_CODES  19
/* number of codes used to
   transfer the bit lengths */

#define HEAP_SIZE (2*L_CODES+1)
/* maximum heap size */

#define MAX_BITS 15
/* All codes must not exceed MAX_BITS bits */

#define Buf_size 16
/* size of bit buffer in bi_buf */

#define Z_NULL  0  /* for initializing
		      zalloc, zfree, opaque */

#define MAXBITS 15

#ifndef ZEXTERN
#  define ZEXTERN extern
#endif

#ifndef ZEXPORT
#  define ZEXPORT
#endif

#define ZLIB_VERSION "1.2.8"

#ifndef MAX_WBITS
#ifdef CONFIG_ARCH_HI3516CV300
#define MAX_WBITS 13 /* 8K LZ77 window */
#else
#define MAX_WBITS 15 /* 32K LZ77 window */
#endif
#endif

#ifndef NO_GZIP
#  define GUNZIP
#endif

#    define zmemcpy memcpy

#ifndef RM
#  define RM
#endif

/* Diagnostic functions */
#ifdef DEBUG
#include <linux/unistd.h>
extern int ZLIB_INTERNAL z_verbose;
extern void ZLIB_INTERNAL z_error OF((char *m));
#  define Assert(cond,msg) {if(!(cond)) z_error(msg);}
#  define Trace(x) {if (z_verbose>=0) fprintf x ;}
#  define Tracev(x) {if (z_verbose>0) fprintf x ;}
#  define Tracevv(x) {if (z_verbose>1) fprintf x ;}
#  define Tracec(c,x) {if (z_verbose>0 && (c)) fprintf x ;}
#  define Tracecv(c,x) {if (z_verbose>1 && (c)) fprintf x ;}
#else
#  define Assert(cond,msg)
#  define Trace(x)
#  define Tracev(x)
#  define Tracevv(x)
#  define Tracec(c,x)
#  define Tracecv(c,x)
#endif

typedef unsigned char  Byte;  /* 8 bits */
typedef unsigned int   uInt;  /* 16 bits or more */
typedef unsigned long  uLong; /* 32 bits or more */
typedef unsigned short ush;
typedef unsigned char  uch;
typedef void  *voidpf;
typedef voidpf (*alloc_func)(voidpf opaque, uInt items, uInt size);
typedef void   (*free_func)(voidpf opaque, voidpf address);

typedef struct ct_data_s {
	union {
		ush  freq;       /* frequency count */
		ush  code;       /* bit string */
	} fc;
	union {
		ush  dad;        /* father node in Huffman tree */
		ush  len;        /* length of bit string */
	} dl;
}ct_data;

typedef struct static_tree_desc_s{
	const ct_data *static_tree;  /* static tree or NULL */
	const int *extra_bits;       /* extra bits for each code or NULL */
	int     extra_base;          /* base index for extra_bits */
	int     elems;               /* max number of elements in the tree */
	int     max_length;          /* max bit length for the codes */
}static_tree_desc;

typedef struct tree_desc_s {
	ct_data *dyn_tree;           /* the dynamic tree */
	int     max_code;            /* largest code with non zero frequency */
	static_tree_desc *stat_desc; /* the corresponding static tree */
	unsigned int  *heap;       /* the lit/length freq (0~285)*2 or
				      the distance freq (0~29)*2 or the
				      code of the length of freq (0~19)*2*/
}tree_desc;

struct internal_state;
struct rm_interface;

typedef struct gz_stream_s {
	Byte     *next_in;     /* next input byte */
	uInt     avail_in;  /* number of bytes available at next_in */
	uLong    total_in;  /* total number of input bytes read so far */

	Byte     *next_out; /* next output byte should be put there */
	uInt     avail_out; /* remaining free space at next_out */
	uLong    total_out; /* total number of bytes output so far */

	char         *msg;  /* last error message, NULL if no error */
	struct internal_state  *state; /* not visible by applications */

	struct rm_interface *inter; /*input and output for rm*/

	alloc_func zalloc;  /* used to allocate the internal state */
	free_func  zfree;   /* used to free the internal state */
	voidpf     opaque;  /* private data object passed to zalloc and zfree */

	uLong   adler;      /* adler32 value of the uncompressed data */
	uLong   reserved;   /* reserved for future use */
}z_stream_gz;

typedef z_stream_gz  *z_streamp_gz;

typedef struct rm_interface{
	unsigned char   l_buf[TOTAL_LENS];
	unsigned short  d_buf[TOTAL_LENS];
	unsigned int    block_num;    /* the true huffman block number*/
	unsigned int    last_lit; /* the last position of valid lit in l_buf*/
}defalte_interface;

typedef struct internal_state {
	z_streamp_gz strm;      /* pointer back to this zlib stream */
	int   status;        /* as the name implies */
	Byte  *pending_buf;  /* output still pending */
	uLong  pending_buf_size; /* size of pending_buf */
	Byte  *pending_out;  /* next pending byte to output to the stream */
	uInt   pending;      /* nb of bytes in the pending buffer */
	int   wrap;          /* bit 0 true for zlib, bit 1 true for gzip */
	Byte  method;        /* can only be DEFLATED */
	int   last_flush;    /* value of flush param for previous deflate call */

	/* used by deflate.c: */

	uInt  w_size;        /* LZ77 window size (32K by default) */
	uInt  w_bits;        /* log2(w_size)  (8..16) */
	uInt  w_mask;        /* w_size - 1 */

	Byte  *window;
	/* Sliding window. Input bytes are read into the second half of the window,
	 * and move to the first half later to keep a dictionary of at least wSize
	 * bytes. With this organization, matches are limited to a distance of
	 * wSize-MAX_MATCH bytes, but this ensures that IO is always
	 * performed with a length multiple of the block size. Also, it limits
	 * the window size to 64K, which is quite useful on MSDOS.
	 * To do: use the user input buffer as sliding window.
	 */

	uLong window_size;
	/* Actual size of window: 2*wSize, except when the user input buffer
	 * is directly used as sliding window.
	 */

	ush *prev;
	/* Link to older string with same hash index. To limit the size of this
	 * array to 64K, this link is maintained only for the last 32K strings.
	 * An index in this array is thus a window index modulo 32K.
	 */

	ush *head; /* Heads of the hash chains or NIL. */

	uInt  ins_h;          /* hash index of string to be inserted */
	uInt  hash_size;      /* number of elements in hash table */
	uInt  hash_bits;      /* log2(hash_size) */
	uInt  hash_mask;      /* hash_size-1 */

	uInt  hash_shift;
	/* Number of bits by which ins_h must be shifted at each input
	 * step. It must be such that after MIN_MATCH steps, the oldest
	 * byte no longer takes part in the hash key, that is:
	 *   hash_shift * MIN_MATCH >= hash_bits
	 */

	long block_start;
	/* Window position at the beginning of the current output block. Gets
	 * negative when the window is moved backwards.
	 */

	uInt match_length;           /* length of best match */
	unsigned prev_match;             /* previous match */
	int match_available;         /* set if previous match exists */
	uInt strstart;               /* start of string to insert */
	uInt match_start;            /* start of matching string */
	uInt lookahead;              /* number of valid bytes ahead in window */

	uInt prev_length;
	/* Length of the best match at previous step. Matches not greater than this
	 * are discarded. This is used in the lazy match evaluation.
	 */

	uInt max_chain_length;
	/* To speed up deflation, hash chains are never searched beyond this
	 * length.  A higher limit improves compression ratio but degrades the
	 * speed.
	 */

	uInt max_lazy_match;
	/* Attempt to find a better match only when the current match is strictly
	 * smaller than this value. This mechanism is used only for compression
	 * levels >= 4.
	 */
#define max_insert_length  max_lazy_match
	/* Insert new strings in the hash table only if the match length is not
	 * greater than this length. This saves time but degrades compression.
	 * max_insert_length is used only for compression levels <= 3.
	 */

	int level;    /* compression level (1..9) */
	int strategy; /* favor or force Huffman coding*/

	uInt good_match;
	/* Use a faster search when the previous match is longer than this */

	int nice_match; /* Stop searching when current match exceeds this */

	/* used by trees.c: */
	/* Didn't use ct_data typedef below to suppress compiler warning */
	struct ct_data_s dyn_ltree[HEAP_SIZE];   /* literal and length tree */
	struct ct_data_s dyn_dtree[2 * D_CODES + 1]; /* distance tree */
	struct ct_data_s bl_tree[2 * BL_CODES + 1];  /* Huffman tree for bit lengths */

	struct tree_desc_s l_desc;	/* desc. for literal tree */
	struct tree_desc_s d_desc;	/* desc. for distance tree */
	struct tree_desc_s bl_desc;	/* desc. for bit length tree */

	ush bl_count[MAX_BITS+1];
	/* number of codes at each bit
	   length for an optimal tree */

	int heap[2*L_CODES+1]; /* heap used to build the Huffman trees */
	int heap_len;          /* number of elements in the heap */
	int heap_max;          /* element of largest frequency */
	/* The sons of heap[n] are heap[2*n] and heap[2*n+1].
	 * heap[0] is not used.
	 * The same heap array is used to build all trees.
	 */

	uch depth[2*L_CODES+1];
	/* Depth of each subtree used as tie breaker
	 * for trees of equal frequency
	 */

	uch  *l_buf;	/* buffer for literals or lengths */

	uInt  lit_bufsize;
	/* Size of match buffer for literals/lengths.  There are 4 reasons for
	 * limiting lit_bufsize to 64K:
	 *   - frequencies can be kept in 16 bit counters
	 *   - if compression is not successful for the first block, all input
	 *     data is still in the window so we can still emit a stored block even
	 *     when input comes from standard input.  (This can also be done for
	 *     all blocks if lit_bufsize is not greater than 32K.)
	 *   - if compression is not successful for a file smaller than 64K, we can
	 *     even emit a stored file instead of a stored block (saving 5 bytes).
	 *     This is applicable only for zip (not gzip or zlib).
	 *   - creating new Huffman trees less frequently may not provide fast
	 *     adaptation to changes in the input data statistics. (Take for
	 *     example a binary file with poorly compressible code followed by
	 *     a highly compressible string table.) Smaller buffer sizes give
	 *     fast adaptation but have of course the overhead of transmitting
	 *     trees more frequently.
	 *   - I can't count above 4
	 */

	uInt last_lit;      /* running index in l_buf */

	ush *d_buf;
	/* Buffer for distances. To simplify the code,
	 * d_buf and l_buf have
	 * the same number of elements.
	 * To use different lengths, an extra flag
	 * array would be necessary.
	 */

	uLong opt_len;	 /* bit length of current block with optimal trees */
	uLong static_len;/* bit length of current block with static trees */
	uInt matches;    /* number of string matches in current block */
	uInt insert;     /* bytes at end of window left to insert */

	ush bi_buf;
	/* Output buffer. bits are inserted starting at the bottom (least
	 * significant bits).
	 */
	int bi_valid;
	/* Number of valid bits in bi_buf.  All bits above the last valid bit
	 * are always zero.
	 */

	uLong high_water;
	/* High water mark offset in window for initialized bytes -- bytes above
	 * this are set to zero in order to avoid memory check warnings when
	 * longest match routines access bytes past the input.  This is then
	 * updated to the new high water mark.
	 */

	uInt in_off;
	/* the offset has been dealed,can not find in window*/
	unsigned int    *w_off;
	/* if copy, the offset in infile*/
	unsigned int    *w_len;
	/* if copy, the copy len*/
	unsigned char   *btype;
	/* the deflate mode:store, static, dynamic*/
	uInt hash_value;/*add for hash*/
} deflate_state;

/*for inflate*/
/* Possible inflate modes between inflate() calls */
typedef enum {
	HEAD,       /* i: waiting for magic header */
	FLAGS,      /* i: waiting for method and flags (gzip) */
	TIME,       /* i: waiting for modification time (gzip) */
	OS,         /* i: waiting for extra flags and operating system (gzip) */
	EXLEN,      /* i: waiting for extra length (gzip) */
	EXTRA,      /* i: waiting for extra bytes (gzip) */
	NAME,       /* i: waiting for end of file name (gzip) */
	COMMENT,    /* i: waiting for end of comment (gzip) */
	HCRC,       /* i: waiting for header crc (gzip) */
	DICTID,     /* i: waiting for dictionary check value */
	DICT,       /* waiting for inflateSetDictionary() call */
	TYPE,       /* i: waiting for type bits, including last-flag bit */
	TYPEDO,     /* i: same, but skip check to exit inflate on new block */
	BEND,       /* i: RM, exit inflate on new block*/
	STORED,     /* i: waiting for stored size (length and complement) */
	COPY_,      /* i/o: same as COPY below, but only first time in */
	COPY,       /* i/o: waiting for input or output to copy stored block */
	TABLE,      /* i: waiting for dynamic block table lengths */
	LENLENS,    /* i: waiting for code length code lengths */
	CODELENS,   /* i: waiting for length/lit and distance code lengths */
	LEN_,       /* i: same as LEN below, but only first time in */
	LEN,        /* i: waiting for length/lit/eob code */
	LENEXT,     /* i: waiting for length extra bits */
	DIST,       /* i: waiting for distance code */
	DISTEXT,    /* i: waiting for distance extra bits */
	MATCH,      /* o: waiting for output space to copy string */
	LIT,        /* o: waiting for output space to write literal */
	CHECK,      /* i: waiting for 32-bit check value */
	LENGTH,     /* i: waiting for 32-bit length (gzip) */
	DONE,       /* finished check, done -- remain here until reset */
	BAD,        /* got a data error -- remain here until reset */
	MEM,        /* got an inflate() memory error -- remain here until reset */
	SYNC        /* looking for synchronization bytes to restart inflate() */
} inflate_mode;

/* Structure for decoding tables.  Each entry provides either the
   information needed to do the operation requested by the code that
   indexed that table entry, or it provides a pointer to another
   table that indexes more bits of the code.  op indicates whether
   the entry is a pointer to another table, a literal, a length or
   distance, an end-of-block, or an invalid code.  For a table
   pointer, the low four bits of op is the number of index bits of
   that table.  For a length or distance, the low four bits of op
   is the number of extra bits to get after the code.  bits is
   the number of bits in this code or part of the code to drop off
   of the bit buffer.  val is the actual byte to output in the case
   of a literal, the base length or distance, or the offset from
   the current table to the next table.  Each entry is four bytes. */
typedef struct {
	unsigned char op;           /* operation, extra bits, table bits */
	unsigned char bits;         /* bits in this part of the code */
	unsigned short val;         /* offset in table or code value */
} code;
/* op values as set by inflate_table():
   00000000 - literal
   0000tttt - table link, tttt != 0 is the number of table index bits
   0001eeee - length or distance, eeee is the number of extra bits
   01100000 - end of block
   01000000 - invalid code
 */
static const code lenfix[512] = {
	{96,7,0},{0,8,80},{0,8,16},{20,8,115},{18,7,31},{0,8,112},{0,8,48},
	{0,9,192},{16,7,10},{0,8,96},{0,8,32},{0,9,160},{0,8,0},{0,8,128},
	{0,8,64},{0,9,224},{16,7,6},{0,8,88},{0,8,24},{0,9,144},{19,7,59},
	{0,8,120},{0,8,56},{0,9,208},{17,7,17},{0,8,104},{0,8,40},{0,9,176},
	{0,8,8},{0,8,136},{0,8,72},{0,9,240},{16,7,4},{0,8,84},{0,8,20},
	{21,8,227},{19,7,43},{0,8,116},{0,8,52},{0,9,200},{17,7,13},{0,8,100},
	{0,8,36},{0,9,168},{0,8,4},{0,8,132},{0,8,68},{0,9,232},{16,7,8},
	{0,8,92},{0,8,28},{0,9,152},{20,7,83},{0,8,124},{0,8,60},{0,9,216},
	{18,7,23},{0,8,108},{0,8,44},{0,9,184},{0,8,12},{0,8,140},{0,8,76},
	{0,9,248},{16,7,3},{0,8,82},{0,8,18},{21,8,163},{19,7,35},{0,8,114},
	{0,8,50},{0,9,196},{17,7,11},{0,8,98},{0,8,34},{0,9,164},{0,8,2},
	{0,8,130},{0,8,66},{0,9,228},{16,7,7},{0,8,90},{0,8,26},{0,9,148},
	{20,7,67},{0,8,122},{0,8,58},{0,9,212},{18,7,19},{0,8,106},{0,8,42},
	{0,9,180},{0,8,10},{0,8,138},{0,8,74},{0,9,244},{16,7,5},{0,8,86},
	{0,8,22},{64,8,0},{19,7,51},{0,8,118},{0,8,54},{0,9,204},{17,7,15},
	{0,8,102},{0,8,38},{0,9,172},{0,8,6},{0,8,134},{0,8,70},{0,9,236},
	{16,7,9},{0,8,94},{0,8,30},{0,9,156},{20,7,99},{0,8,126},{0,8,62},
	{0,9,220},{18,7,27},{0,8,110},{0,8,46},{0,9,188},{0,8,14},{0,8,142},
	{0,8,78},{0,9,252},{96,7,0},{0,8,81},{0,8,17},{21,8,131},{18,7,31},
	{0,8,113},{0,8,49},{0,9,194},{16,7,10},{0,8,97},{0,8,33},{0,9,162},
	{0,8,1},{0,8,129},{0,8,65},{0,9,226},{16,7,6},{0,8,89},{0,8,25},
	{0,9,146},{19,7,59},{0,8,121},{0,8,57},{0,9,210},{17,7,17},{0,8,105},
	{0,8,41},{0,9,178},{0,8,9},{0,8,137},{0,8,73},{0,9,242},{16,7,4},
	{0,8,85},{0,8,21},{16,8,258},{19,7,43},{0,8,117},{0,8,53},{0,9,202},
	{17,7,13},{0,8,101},{0,8,37},{0,9,170},{0,8,5},{0,8,133},{0,8,69},
	{0,9,234},{16,7,8},{0,8,93},{0,8,29},{0,9,154},{20,7,83},{0,8,125},
	{0,8,61},{0,9,218},{18,7,23},{0,8,109},{0,8,45},{0,9,186},{0,8,13},
	{0,8,141},{0,8,77},{0,9,250},{16,7,3},{0,8,83},{0,8,19},{21,8,195},
	{19,7,35},{0,8,115},{0,8,51},{0,9,198},{17,7,11},{0,8,99},{0,8,35},
	{0,9,166},{0,8,3},{0,8,131},{0,8,67},{0,9,230},{16,7,7},{0,8,91},
	{0,8,27},{0,9,150},{20,7,67},{0,8,123},{0,8,59},{0,9,214},{18,7,19},
	{0,8,107},{0,8,43},{0,9,182},{0,8,11},{0,8,139},{0,8,75},{0,9,246},
	{16,7,5},{0,8,87},{0,8,23},{64,8,0},{19,7,51},{0,8,119},{0,8,55},
	{0,9,206},{17,7,15},{0,8,103},{0,8,39},{0,9,174},{0,8,7},{0,8,135},
	{0,8,71},{0,9,238},{16,7,9},{0,8,95},{0,8,31},{0,9,158},{20,7,99},
	{0,8,127},{0,8,63},{0,9,222},{18,7,27},{0,8,111},{0,8,47},{0,9,190},
	{0,8,15},{0,8,143},{0,8,79},{0,9,254},{96,7,0},{0,8,80},{0,8,16},
	{20,8,115},{18,7,31},{0,8,112},{0,8,48},{0,9,193},{16,7,10},{0,8,96},
	{0,8,32},{0,9,161},{0,8,0},{0,8,128},{0,8,64},{0,9,225},{16,7,6},
	{0,8,88},{0,8,24},{0,9,145},{19,7,59},{0,8,120},{0,8,56},{0,9,209},
	{17,7,17},{0,8,104},{0,8,40},{0,9,177},{0,8,8},{0,8,136},{0,8,72},
	{0,9,241},{16,7,4},{0,8,84},{0,8,20},{21,8,227},{19,7,43},{0,8,116},
	{0,8,52},{0,9,201},{17,7,13},{0,8,100},{0,8,36},{0,9,169},{0,8,4},
	{0,8,132},{0,8,68},{0,9,233},{16,7,8},{0,8,92},{0,8,28},{0,9,153},
	{20,7,83},{0,8,124},{0,8,60},{0,9,217},{18,7,23},{0,8,108},{0,8,44},
	{0,9,185},{0,8,12},{0,8,140},{0,8,76},{0,9,249},{16,7,3},{0,8,82},
	{0,8,18},{21,8,163},{19,7,35},{0,8,114},{0,8,50},{0,9,197},{17,7,11},
	{0,8,98},{0,8,34},{0,9,165},{0,8,2},{0,8,130},{0,8,66},{0,9,229},
	{16,7,7},{0,8,90},{0,8,26},{0,9,149},{20,7,67},{0,8,122},{0,8,58},
	{0,9,213},{18,7,19},{0,8,106},{0,8,42},{0,9,181},{0,8,10},{0,8,138},
	{0,8,74},{0,9,245},{16,7,5},{0,8,86},{0,8,22},{64,8,0},{19,7,51},
	{0,8,118},{0,8,54},{0,9,205},{17,7,15},{0,8,102},{0,8,38},{0,9,173},
	{0,8,6},{0,8,134},{0,8,70},{0,9,237},{16,7,9},{0,8,94},{0,8,30},
	{0,9,157},{20,7,99},{0,8,126},{0,8,62},{0,9,221},{18,7,27},{0,8,110},
	{0,8,46},{0,9,189},{0,8,14},{0,8,142},{0,8,78},{0,9,253},{96,7,0},
	{0,8,81},{0,8,17},{21,8,131},{18,7,31},{0,8,113},{0,8,49},{0,9,195},
	{16,7,10},{0,8,97},{0,8,33},{0,9,163},{0,8,1},{0,8,129},{0,8,65},
	{0,9,227},{16,7,6},{0,8,89},{0,8,25},{0,9,147},{19,7,59},{0,8,121},
	{0,8,57},{0,9,211},{17,7,17},{0,8,105},{0,8,41},{0,9,179},{0,8,9},
	{0,8,137},{0,8,73},{0,9,243},{16,7,4},{0,8,85},{0,8,21},{16,8,258},
	{19,7,43},{0,8,117},{0,8,53},{0,9,203},{17,7,13},{0,8,101},{0,8,37},
	{0,9,171},{0,8,5},{0,8,133},{0,8,69},{0,9,235},{16,7,8},{0,8,93},
	{0,8,29},{0,9,155},{20,7,83},{0,8,125},{0,8,61},{0,9,219},{18,7,23},
	{0,8,109},{0,8,45},{0,9,187},{0,8,13},{0,8,141},{0,8,77},{0,9,251},
	{16,7,3},{0,8,83},{0,8,19},{21,8,195},{19,7,35},{0,8,115},{0,8,51},
	{0,9,199},{17,7,11},{0,8,99},{0,8,35},{0,9,167},{0,8,3},{0,8,131},
	{0,8,67},{0,9,231},{16,7,7},{0,8,91},{0,8,27},{0,9,151},{20,7,67},
	{0,8,123},{0,8,59},{0,9,215},{18,7,19},{0,8,107},{0,8,43},{0,9,183},
	{0,8,11},{0,8,139},{0,8,75},{0,9,247},{16,7,5},{0,8,87},{0,8,23},
	{64,8,0},{19,7,51},{0,8,119},{0,8,55},{0,9,207},{17,7,15},{0,8,103},
	{0,8,39},{0,9,175},{0,8,7},{0,8,135},{0,8,71},{0,9,239},{16,7,9},
	{0,8,95},{0,8,31},{0,9,159},{20,7,99},{0,8,127},{0,8,63},{0,9,223},
	{18,7,27},{0,8,111},{0,8,47},{0,9,191},{0,8,15},{0,8,143},{0,8,79},
	{0,9,255}
};

static const code distfix[32] = {
	{16,5,1},{23,5,257},{19,5,17},{27,5,4097},{17,5,5},{25,5,1025},
	{21,5,65},{29,5,16385},{16,5,3},{24,5,513},{20,5,33},{28,5,8193},
	{18,5,9},{26,5,2049},{22,5,129},{64,5,0},{16,5,2},{23,5,385},
	{19,5,25},{27,5,6145},{17,5,7},{25,5,1537},{21,5,97},{29,5,24577},
	{16,5,4},{24,5,769},{20,5,49},{28,5,12289},{18,5,13},{26,5,3073},
	{22,5,193},{64,5,0}
};
/* Maximum size of the dynamic table.  The maximum number of code structures is
   1444, which is the sum of 852 for literal/length codes and 592 for distance
   codes.  These values were found by exhaustive searches using the program
   examples/enough.c found in the zlib distribtution.  The arguments to that
   program are the number of symbols, the initial root table size, and the
   maximum bit length of a code.  "enough 286 9 15" for literal/length codes
   returns returns 852, and "enough 30 6 15" for distance codes returns 592.
   The initial root table size (9 or 6) is found in the fifth argument of the
   inflate_table() calls in inflate.c and infback.c.  If the root table size is
   changed, then these maximum sizes would be need to be recalculated and
   updated.主表+子表，具体的待会再看 */
#define ENOUGH_LENS 852
#define ENOUGH_DISTS 592
#define ENOUGH (ENOUGH_LENS+ENOUGH_DISTS)

/* Type of code to build for inflate_table() */
typedef enum {
	CODES,
	LENS,
	DISTS
} codetype;

/*added for RM, applied by inflate*/
struct inflate_interface{
	unsigned char mode;	/* stored:0 static huffman:1 dynamic huffman:2 */
	unsigned char *lenlen;	/* length table for inflating length */
	unsigned int  *lenchar;	/* length and char table */
	unsigned int  *distance;/* distance table */
	unsigned int  bit_pos;	/* bit possition of the first bit need to deal*/
	unsigned long crc_out;	/* crc32 :not last block, 4nBytes; last block, actual Bytes*/

	unsigned int  *huffman_result;	/* the result after huffman inflating[8*1024]*/

	/*added for new requirement at 2013/12/21*/
	unsigned int *lenchar_addr;
	unsigned int *dis_addr;
	unsigned int lenchar_addr_index;
	unsigned int dis_addr_index;
};

/* state maintained between inflate() calls.  Approximately 10K bytes. */
struct inflate_state {
	inflate_mode mode;          /* current inflate mode */
	int last;                   /* true if processing last block */
	int wrap;                   /* bit 0 true for zlib, bit 1 true for gzip */
	int havedict;               /* true if dictionary provided */
	int flags;                  /* gzip header method and flags (0 if zlib) */
	unsigned dmax;              /* zlib header max distance (INFLATE_STRICT) */
	unsigned long check;        /* protected copy of check value */
	unsigned long total;        /* protected copy of output count */
	/* sliding window */
	unsigned int  wbits;             /* log base 2 of requested window size */
	unsigned int  wsize;             /* window size or zero if not using window */
	unsigned int  whave;             /* valid bytes in the window */
	unsigned int  wnext;             /* window write index */
	unsigned char *window;      /* allocated sliding window, if needed */
	/* bit accumulator */
	unsigned long hold;         /* input bit accumulator */
	unsigned int  bits;              /* number of bits in "in" */
	/* for string and stored block copying */
	unsigned int  length;            /* literal or length of data to copy */
	unsigned int  offset;            /* distance back to copy string from */
	/* for table and code decoding */
	unsigned int  extra;             /* extra bits needed */
	/* fixed and dynamic code tables */
	code const *lencode;        /* starting table for length/literal codes */
	code const *distcode;       /* starting table for distance codes */
	unsigned int lenbits;           /* index bits for lencode */
	unsigned int distbits;          /* index bits for distcode */
	/* dynamic table building */
	unsigned int ncode;             /* number of code length code lengths */
	unsigned int nlen;              /* number of length code lengths */
	unsigned int ndist;             /* number of distance code lengths */
	unsigned int have;              /* number of code lengths in lens[] */
	code *next;                 /* next available space in codes[] */
	unsigned short lens[320];   /* temporary storage for code lengths */
	unsigned short work[288];   /* work area for code table building */
	code codes[ENOUGH];         /* space for code tables */
	int sane;                   /* if false, allow invalid distance too far */
	int back;                   /* bits back of last unprocessed length/lit */
	unsigned int was;               /* initial length of match */
};





/* default windowBits for decompression. MAX_WBITS is for compression only */
#ifndef DEF_WBITS
#  define DEF_WBITS MAX_WBITS
#endif

/* Reverse the bytes in a 32-bit value */
#define ZSWAP32(q) ((((q) >> 24) & 0xff) + (((q) >> 8) & 0xff00) + \
		(((q) & 0xff00) << 8) + (((q) & 0xff) << 24))

/*****************inflate************************************/
/* Load registers with state in inflate() for speed */
#define LOAD() \
	do { \
		put = strm->next_out; \
		left = strm->avail_out; \
		next = strm->next_in; \
		have = strm->avail_in; \
		hold = state->hold; \
		bits = state->bits; \
	} while (0)

/* Restore state from registers in inflate() */
#define RESTORE() \
	do { \
		strm->next_out = put; \
		strm->avail_out = left; \
		strm->next_in = next; \
		strm->avail_in = have; \
		state->hold = hold; \
		state->bits = bits; \
	} while (0)

/* Remove zero to seven bits as needed to go to a byte boundary */
#define BYTEBITS() \
	do { \
		hold >>= bits & 7; \
		bits -= bits & 7; \
	} while (0)

/* Get a byte of input into the bit accumulator, or return from inflate()
   if there is no input available. */
#define PULLBYTE() \
	do { \
		if (have == 0) goto inf_leave; \
		have--; \
		hold += (unsigned long)(*next++) << bits; \
		bits += 8; \
	} while (0)

/* Assure that there are at least n bits in the bit accumulator.  If there is
   not enough available input to do that, then return from inflate(). */
#define NEEDBITS(n) \
	do { \
		while (bits < (unsigned)(n)) \
		PULLBYTE(); \
	} while (0)

/* Return the low n bits of the bit accumulator (n < 16) */
#define BITS(n) \
	((unsigned)hold & ((1U << (n)) - 1))

/* Remove n bits from the bit accumulator */
#define DROPBITS(n) \
	do { \
		hold >>= (n); \
		bits -= (unsigned)(n); \
	} while (0)

/* Clear the input bit accumulator */
#define INITBITS() \
	do { \
		hold = 0; \
		bits = 0; \
	} while (0)

/* check function to use adler32() for zlib or crc32() for gzip */
#ifdef GUNZIP
/*#  define UPDATE(check, buf, len) \
  (state->flags ? crc32(check, buf, len) : adler32(check, buf, len))*/
#  define UPDATE(check, buf, len) crc32(check, buf, len)
#else
#  define UPDATE(check, buf, len) adler32(check, buf, len)
#endif

#define Z_NO_COMPRESSION         0
#define Z_BEST_SPEED             1
#define Z_BEST_COMPRESSION       9
#define Z_DEFAULT_COMPRESSION  (-1)
/* compression levels */

#define Z_FILTERED            1
#define Z_HUFFMAN_ONLY        2
#define Z_RLE                 3
#define Z_FIXED               4
#define Z_DEFAULT_STRATEGY    0
/* compression strategy; see deflateInit2() below for details */

#define Z_DEFLATED   8
/* The deflate compression method (the only one supported in this version) */

/* Maximum value for memLevel in deflateInit2 */

#define MAX_MEM_LEVEL 9
#define DEF_MEM_LEVEL 7    /*RM, hash 16K, buff 8K*/

#define MIN_MATCH  3
#define MAX_MATCH  258
/* The minimum and maximum match lengths */

#define INIT_STATE    42
#define EXTRA_STATE   69
#define NAME_STATE    73
#define COMMENT_STATE 91
#define HCRC_STATE   103
#define BUSY_STATE   113
#define FINISH_STATE 666
/* Stream status */


#define NIL 0
#define TOO_FAR 4096

/* Output a byte on the stream.
 * IN assertion: there is enough room in pending_buf.
 */
#define put_byte(s, c) {s->pending_buf[s->pending++] = (c);}

#define STORED_BLOCK 0
#define STATIC_TREES 1
#define DYN_TREES    2
/* The three kinds of block type */
#define MIN_LOOKAHEAD (511)
/* Minimum amount of lookahead, except at the end of the input file.
 * See deflate.c for comments about the MIN_MATCH+1.
 */

#define MAX_DIST(s)  ((s)->w_size-MIN_LOOKAHEAD)
/* In order to simplify the code, particularly on 16 bit machines, match
 * distances are limited to MAX_DIST instead of WSIZE.
 */

#define WIN_INIT MAX_MATCH
/* Number of bytes after end of data in window to initialize in order to avoid
   memory checker errors from longest match routines */
struct def_lz_rm
{
	unsigned int   in_file_size;  /* the size of input file*/
	unsigned int   lz_result_line;/* the total line number of lz result*/
	unsigned int   total_in;      /* the total dealted input datas*/
	unsigned int   crc_out;       /* the crc value after lz*/
	unsigned int   total_block_num; /* total huffman block number*/
	unsigned int   last_lit;	/* the last huffman block have last_lit liters*/

	/*used for deflate lz for part of task file*/
	unsigned short  prev[1 << MAX_WBITS];
	/* Link to older string with same hash index.
	 * To limit the size of this
	 * array to 64K, this link is maintained
	 * only for the last 32K strings.
	 * An index in this array is thus a window
	 * index modulo 32K.
	 */
	unsigned short  head[1 << (DEF_MEM_LEVEL + 7)];	/* Heads of the hash chains or NIL. */
	unsigned int    ins_h;	/* hash index of string to be inserted */
	unsigned int    match_length;	/* length of best match */
	unsigned        prev_match;	/* previous match */
	int             match_available;/* set if previous match exists */
	unsigned int    strstart;	/* start of string to insert */
	unsigned int    match_start;	/* start of matching string */
	unsigned int    lookahead;	/* number of valid bytes ahead in window */
	unsigned int    prev_length;
	/* Length of the best match at previous step. Matches not greater than this
	 * are discarded. This is used in the lazy match evaluation.
	 */
	unsigned int    insert;    /* bytes at end of window left to insert */
	unsigned int    high_water;
	unsigned char   window[1 << (MAX_WBITS + 1)]; /* allocated sliding window, if needed */
	/* High water mark offset in window for initialized bytes -- bytes above
	 * this are set to zero in order to avoid memory check warnings when
	 * longest match routines access bytes past the input.  This is then
	 * updated to the new high water mark.
	 */
};

struct def_huf_rm
{
	unsigned int    next_line_num;	/* need to deal lz result at next_line_num, start 0*/
	unsigned int    cur_block_num;	/* current block number, start from 0*/
	unsigned int    total_out;	/* until this time, the total output bytes of huffman blocks*/
	unsigned int    def_out_lens;	/* the number of output after deflate huffman block*/
	unsigned int    block_start;	/* appoint to the infile, means the start of current block*/
	unsigned short  bi_buf;
	/* Output buffer. bits are inserted starting at the bottom (least
	 * significant bits).
	 */
	int             bi_valid;
	/* Number of valid bits in bi_buf.  All bits above the last valid bit
	 * are always zero.
	 */
	unsigned char   def_out[DEFLATE_BLOCK_SIZE];	/* the result of huffman result*/
	unsigned int    ll_heap[2 * L_CODES - 1];	/* the lit/length freq (0~285)*2*/
	unsigned int    d_heap[2 * D_CODES - 1];	/* the distance freq (0~29)*2*/
	unsigned int    bl_heap[2 * BL_CODES - 1];	/* the code of the length of freq (0~19)*2*/
	unsigned int    table_code[335];
	/* the lit/length code 0~285,the distance code 286~315,
	   the code of the length of code 316~334*/
	unsigned char   btype;	/* stored:0 static huffman:1 dynamic huffman:2 */
};

extern void * gzip_alloc(unsigned int size);

#endif /*__GLOBAL_H__*/
