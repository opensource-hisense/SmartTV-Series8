/*
 * LZO decompressor for the Linux kernel. Code borrowed from the lzo
 * implementation by Markus Franz Xaver Johannes Oberhumer.
 *
 * Add parallel decompress & reading, porting to uboot.
 * Copyright (C) 2011-2012
 * Joe.C, MediaTek Inc <yingjoe.chen@mediatek.com>
 *
 * Linux kernel adaptation:
 * Copyright (C) 2009
 * Albin Tonnerre, Free Electrons <albin.tonnerre@free-electrons.com>
 *
 * Original code:
 * Copyright (C) 1996-2005 Markus Franz Xaver Johannes Oberhumer
 * All Rights Reserved.
 *
 * lzop and the LZO library are free software; you can redistribute them
 * and/or modify them under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.
 * If not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * Markus F.X.J. Oberhumer
 * <markus@oberhumer.com>
 * http://www.oberhumer.com/opensource/lzop/
 */

#include <linux/types.h>
#include <linux/lzo.h>
#include <linux/string.h>
#include <malloc.h>

#include <asm/unaligned.h>
#include "lzo/lzodefs.h"

static const unsigned char lzop_magic[] = {
	0x89, 0x4c, 0x5a, 0x4f, 0x00, 0x0d, 0x0a, 0x1a, 0x0a };

#define LZO_BLOCK_SIZE        (256*1024l)
#define HEADER_HAS_FILTER      0x00000800L

static inline int parse_header(u8 *input, u8 *skip)
{
	int l;
	u8 *parse = input;
	u8 level = 0;
	u16 version;

	/* read magic: 9 first bits */
	for (l = 0; l < 9; l++) {
		if (*parse++ != lzop_magic[l])
			return 0;
	}
	/* get version (2bytes), skip library version (2),
	 * 'need to be extracted' version (2) and
	 * method (1) */
	version = get_unaligned_be16(parse);
	parse += 7;
	if (version >= 0x0940)
		level = *parse++;
	if (get_unaligned_be32(parse) & HEADER_HAS_FILTER)
		parse += 8; /* flags + filter info */
	else
		parse += 4; /* flags */

	/* skip mode and mtime_low */
	parse += 8;
	if (version >= 0x0940)
		parse += 4;	/* skip mtime_high */

	l = *parse++;
	/* don't care about the file name, and skip checksum */
	parse += l + 4;

	*skip = parse - input;
	return 1;
}

#define COPY4(dst, src)	\
		put_unaligned(get_unaligned((const u32 *)(src)), (u32 *)(dst))

#define error(str)  printf("%s\n", str)
# ifndef unlikely
#  define unlikely(x)	x
# endif

#define unlzo_check_input_size(size)    \
        if (in_len < (size))            \
        {				\
		ret = fill(fill_data);  \
		if (ret < 0)		\
                    return LZO_E_INPUT_OVERRUN;\
                in_len += ret;		\
        }

int lzo1x_decompress_block(const unsigned char *in, int src_len, int *pin_len,
				int (*fill) (void *fill_data),
				void *fill_data,
			   unsigned char *out, size_t *out_len);


// input:     input buffer
// in_len:    init input len (start from input buffer). Return bytes read.
// fill:      fill function to read more data.
// fill_data: will pass this data to fill function.
// output:    output buffer
// posp:      return output length
int unlzo_read(u8 *input, int in_len,
				int (*fill) (void *fill_data),
				void *fill_data,
				u8 *output, int *posp)
{
	u8 skip = 0, r = 0;
	u32 src_len, dst_len;
	size_t tmp;
	u8 *in_buf, *out_buf;
	int ret = -1;

	out_buf = output;

	in_buf = input;

	if (*posp)
		*posp = 0;

        // header size. assume < 128.
        unlzo_check_input_size(128);

	if (!parse_header(input, &skip)) {
		error("invalid header");
		goto exit_2;
	}
	in_buf += skip;
	in_len -= skip;

	for (;;) {
	        // Make sure we have 4 bytes for dst_len
	        unlzo_check_input_size(4);

		/* read uncompressed block size */
		dst_len = get_unaligned_be32(in_buf);
		in_buf += 4;
		in_len -= 4;

		/* exit if last block */
		if (dst_len == 0) {
			break;
		}

		if (dst_len > LZO_BLOCK_SIZE) {
			error("dest len longer than block size");
			goto exit_2;
		}

	        // make sure we have more then 9 bytes. 8bytes for remaining 
	        // block header, and 1 byte for first byte in block to make
	        // sure we have at lest 1 byte when calling lzo1x_decompress_block 
	        unlzo_check_input_size(9);

		/* read compressed block size, and skip block checksum info */
		src_len = get_unaligned_be32(in_buf);
		in_buf += 8;
		in_len -= 8;

		if (src_len <= 0 || src_len > dst_len) {
			error("file corrupted");
			goto exit_2;
		}

		/* decompress */
		tmp = dst_len;

		/* When the input data is not compressed at all,
		 * lzo1x_decompress_safe will fail, so call memcpy()
		 * instead */
		if (unlikely(dst_len == src_len))
		{
		        size_t cleft, num;
		        u8 *tin, *tout;
		        
		        tin = in_buf;
		        tout = out_buf;
		        cleft = dst_len;
		        while (cleft)
		        {
		            // Read some data if no data left.
		            if (!in_len)
                                unlzo_check_input_size(cleft);

		            num = (in_len < cleft) ? in_len : cleft;
		            memcpy(tin, tout, num);
		            tin += num;
		            tout += num;
		            in_len -= num;
		            cleft -= num;
		            *posp += num;
                        }
                }
		else {
			r = lzo1x_decompress_block((u8 *) in_buf, src_len, &in_len,
						fill, fill_data,
						out_buf, &tmp);

			if (r != LZO_E_OK || dst_len != tmp) {
				error("Compressed data violation");
				goto exit_2;
			}

			in_len -= src_len;
		}

		out_buf += dst_len;
		*posp += dst_len;
		in_buf += src_len;
	}

	ret = 0;
exit_2:
	return ret;
}

static inline int fetch_more_data(const unsigned char *ip, const unsigned char **ip_end, int *pin_len, int size_need,
				int (*fill) (void *fill_data),
				void *fill_data)
{
	int size_left = *ip_end - ip;
	int ret;

	while (size_left < size_need)
	{
		ret = fill(fill_data);
		if (ret <= 0)
                    return 1;

                size_left += ret;
                *ip_end += ret;
                *pin_len += ret;
	}

	return 0;
}

#define HAVE_IP(x, ip_end, ip) (((size_t)(ip_end - ip) < (x)) && \
		(((ipsrc_end - ip) < (x)) || fetch_more_data(ip, &ip_end, pin_len, x, fill, fill_data)))
#define HAVE_OP(x, op_end, op) ((size_t)(op_end - op) < (x))
#define HAVE_LB(m_pos, out, op) (m_pos < out || m_pos >= op)


int lzo1x_decompress_block(const unsigned char *in, int src_len, int *pin_len,
				int (*fill) (void *fill_data),
				void *fill_data,
			   unsigned char *out, size_t *out_len)
{
	const unsigned char * const ipsrc_end = in + src_len;
	const unsigned char * ip_end = in + *pin_len;
	unsigned char * const op_end = out + *out_len;
	const unsigned char *ip = in, *m_pos;
	unsigned char *op = out;
	size_t t;

	*out_len = 0;

	if (*ip > 17) {
		t = *ip++ - 17;
		if (t < 4)
			goto match_next;
		if (HAVE_OP(t, op_end, op))
			goto output_overrun;
		if (HAVE_IP(t + 1, ip_end, ip))
			goto input_overrun;
		do {
			*op++ = *ip++;
		} while (--t > 0);
		goto first_literal_run;
	}

	while (!HAVE_IP(1, ip_end, ip)) {
		t = *ip++;
		if (t >= 16)
			goto match;
		if (t == 0) {
			if (HAVE_IP(1, ip_end, ip))
				goto input_overrun;
			while (*ip == 0) {
				t += 255;
				ip++;
				if (HAVE_IP(1, ip_end, ip))
					goto input_overrun;
			}
			t += 15 + *ip++;
		}
		if (HAVE_OP(t + 3, op_end, op))
			goto output_overrun;
		if (HAVE_IP(t + 4, ip_end, ip))
			goto input_overrun;

		COPY4(op, ip);
		op += 4;
		ip += 4;
		if (--t > 0) {
			if (t >= 4) {
				do {
					COPY4(op, ip);
					op += 4;
					ip += 4;
					t -= 4;
				} while (t >= 4);
				if (t > 0) {
					do {
						*op++ = *ip++;
					} while (--t > 0);
				}
			} else {
				do {
					*op++ = *ip++;
				} while (--t > 0);
			}
		}

first_literal_run:
		if (HAVE_IP(2, ip_end, ip))
			goto input_overrun;

		t = *ip++;
		if (t >= 16)
			goto match;
		m_pos = op - (1 + M2_MAX_OFFSET);
		m_pos -= t >> 2;
		m_pos -= *ip++ << 2;

		if (HAVE_LB(m_pos, out, op))
			goto lookbehind_overrun;

		if (HAVE_OP(3, op_end, op))
			goto output_overrun;
		*op++ = *m_pos++;
		*op++ = *m_pos++;
		*op++ = *m_pos;

		goto match_done;

		do {
match:
			if (t >= 64) {
				if (HAVE_IP(1, ip_end, ip))
					goto input_overrun;
				m_pos = op - 1;
				m_pos -= (t >> 2) & 7;
				m_pos -= *ip++ << 3;
				t = (t >> 5) - 1;
				if (HAVE_LB(m_pos, out, op))
					goto lookbehind_overrun;
				if (HAVE_OP(t + 3 - 1, op_end, op))
					goto output_overrun;
				goto copy_match;
			} else if (t >= 32) {
				t &= 31;
				if (t == 0) {
					if (HAVE_IP(1, ip_end, ip))
						goto input_overrun;
					while (*ip == 0) {
						t += 255;
						ip++;
						if (HAVE_IP(1, ip_end, ip))
							goto input_overrun;
					}
					t += 31 + *ip++;
				}
				if (HAVE_IP(2, ip_end, ip))
					goto input_overrun;
				m_pos = op - 1;
				m_pos -= get_unaligned_le16(ip) >> 2;
				ip += 2;
			} else if (t >= 16) {
				m_pos = op;
				m_pos -= (t & 8) << 11;

				t &= 7;
				if (t == 0) {
					if (HAVE_IP(1, ip_end, ip))
						goto input_overrun;
					while (*ip == 0) {
						t += 255;
						ip++;
						if (HAVE_IP(1, ip_end, ip))
							goto input_overrun;
					}
					t += 7 + *ip++;
				}
				if (HAVE_IP(2, ip_end, ip))
					goto input_overrun;
				m_pos -= get_unaligned_le16(ip) >> 2;
				ip += 2;
				if (m_pos == op)
					goto eof_found;
				m_pos -= 0x4000;
			} else {
				if (HAVE_IP(1, ip_end, ip))
					goto input_overrun;
				m_pos = op - 1;
				m_pos -= t >> 2;
				m_pos -= *ip++ << 2;

				if (HAVE_LB(m_pos, out, op))
					goto lookbehind_overrun;
				if (HAVE_OP(2, op_end, op))
					goto output_overrun;

				*op++ = *m_pos++;
				*op++ = *m_pos;
				goto match_done;
			}

			if (HAVE_LB(m_pos, out, op))
				goto lookbehind_overrun;
			if (HAVE_OP(t + 3 - 1, op_end, op))
				goto output_overrun;

			if (t >= 2 * 4 - (3 - 1) && (op - m_pos) >= 4) {
				COPY4(op, m_pos);
				op += 4;
				m_pos += 4;
				t -= 4 - (3 - 1);
				do {
					COPY4(op, m_pos);
					op += 4;
					m_pos += 4;
					t -= 4;
				} while (t >= 4);
				if (t > 0)
					do {
						*op++ = *m_pos++;
					} while (--t > 0);
			} else {
copy_match:
				*op++ = *m_pos++;
				*op++ = *m_pos++;
				do {
					*op++ = *m_pos++;
				} while (--t > 0);
			}
match_done:
			t = ip[-2] & 3;
			if (t == 0)
				break;
match_next:
			if (HAVE_OP(t, op_end, op))
				goto output_overrun;
			if (HAVE_IP(t + 1, ip_end, ip))
				goto input_overrun;

			*op++ = *ip++;
			if (t > 1) {
				*op++ = *ip++;
				if (t > 2)
					*op++ = *ip++;
			}

			t = *ip++;
		} while (!HAVE_IP(1, ip_end, ip));
	}

	*out_len = op - out;
	return LZO_E_EOF_NOT_FOUND;

eof_found:
	*out_len = op - out;
	return (ip == ipsrc_end ? LZO_E_OK :
		(ip < ipsrc_end ? LZO_E_INPUT_NOT_CONSUMED : LZO_E_INPUT_OVERRUN));
input_overrun:
	*out_len = op - out;
	return LZO_E_INPUT_OVERRUN;

output_overrun:
	*out_len = op - out;
	return LZO_E_OUTPUT_OVERRUN;

lookbehind_overrun:
	*out_len = op - out;
	return LZO_E_LOOKBEHIND_OVERRUN;
}
