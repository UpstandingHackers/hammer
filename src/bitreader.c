/* Bit-parsing operations for Hammer.
 * Copyright (C) 2012  Meredith L. Patterson, Dan "TQ" Hirsch
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, version 2.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <stdint.h>
#include <stdio.h>
#include "internal.h"
#include "hammer.h"
#include "test_suite.h"

#define LSB(range) (0:range)
#define MSB(range) (1:range)
#define LDB(range,i) (((i)>>LSB(range))&((1<<(MSB(range)-LSB(range)+1))-1))


int64_t h_read_bits(HInputStream* state, int count, char signed_p) {
  // BUG: Does not 
  int64_t out = 0;
  int offset = 0;
  int final_shift = 0;
  int64_t msb = ((signed_p ? 1LL:0) << (count - 1)); // 0 if unsigned, else 1 << (nbits - 1)
  
  
  // overflow check...
  int bits_left = (state->length - state->index); // well, bytes for now
  if (bits_left <= 64) { // Large enough to handle any valid count, but small enough that overflow isn't a problem.
    // not in danger of overflowing, so add in bits
    // add in number of bits...
    bits_left = (bits_left << 3) - state->bit_offset - state->margin;
    if (bits_left < count) {
      if (state->endianness & BYTE_BIG_ENDIAN)
	final_shift = count - bits_left;
      else
	final_shift = 0;
      count = bits_left;
      state->overrun = true;
    } else
      final_shift = 0;
  }
  
  if ((state->bit_offset & 0x7) == 0 && (count & 0x7) == 0 && (state->margin == 0)) {
    // fast path
    if (state->endianness & BYTE_BIG_ENDIAN) {
      while (count > 0) {
	count -= 8;
	out = (out << 8) | state->input[state->index++];
      }
    } else {
      int i;
      for (i = 0; count > 0; i += 8) {
	count -= 8;
	out |= (int64_t)state->input[state->index++] << i;
      }
    }
  } else {
    while (count) {
      int segment, segment_len;
      // Read a segment...
      if (state->endianness & BIT_BIG_ENDIAN) {
	if (count + state->bit_offset + state->margin >= 8) {
	  segment_len = 8 - state->bit_offset - state->margin;
	  segment = (state->input[state->index] >> state->margin) & ((1 << segment_len) - 1);
	  state->index++;
	  state->bit_offset = 0;
	  state->margin = 0;
	} else {
	  segment_len = count;
	  state->bit_offset += count;
	  segment = (state->input[state->index] >> (8 - state->bit_offset)) & ((1 << segment_len) - 1);
	}
      } else { // BIT_LITTLE_ENDIAN
	if (count + state->bit_offset + state->margin >= 8) {
	  segment_len = 8 - state->bit_offset - state->margin;
	  segment = (state->input[state->index] >> state->bit_offset) & ((1 << segment_len) - 1);
	  state->index++;
	  state->bit_offset = 0;
	  state->margin = 0;
	} else {
	  segment_len = count;
	  segment = (state->input[state->index] >> state->bit_offset) & ((1 << segment_len) - 1);
	  state->bit_offset += segment_len;
	}
      }
      
      // have a valid segment; time to assemble the byte
      if (state->endianness & BYTE_BIG_ENDIAN) {
	out = out << segment_len | segment;
      } else { // BYTE_LITTLE_ENDIAN
	out |= (int64_t)segment << offset;
	offset += segment_len;
      }
      count -= segment_len;
    }
  }
  out <<= final_shift;
  return (out ^ msb) - msb; // perform sign extension
}

void h_skip_bits(HInputStream* stream, size_t count) {
  size_t left;

  if (count == 0)
    return;

  if (stream->overrun)
    return;

  if (stream->index == stream->length) {
    stream->overrun = true;
    return;
  }

  // consume from a partial byte?
  left = 8 - stream->bit_offset - stream->margin;
  if (count < left) {
    stream->bit_offset += count;
    return;
  }
  if (left < 8) {
    stream->index += 1;
    stream->bit_offset = 0;
    stream->margin = 0;
    count -= left;
  }
  assert(stream->bit_offset == 0);
  assert(stream->margin == 0);

  // consume full bytes
  left = stream->length - stream->index;
  if (count / 8 <= left) {
    stream->index += count / 8;
    count = count % 8;
  } else {
    stream->index = stream->length;
    stream->overrun = true;
    return;
  }
  assert(count < 8);

  // final partial byte
  if (count > 0 && stream->index == stream->length)
    stream->overrun = true;
  else
    stream->bit_offset = count;
}

void h_seek_bits(HInputStream* stream, size_t pos) {
  size_t pos_index = pos / 8;
  size_t pos_offset = pos % 8;

  /* seek within the current byte? */
  if (pos_index == stream->index) {
    stream->bit_offset = pos_offset;
    return;
  }

  stream->margin = 0;

  /* seek past the end? */
  if ((pos_index > stream->length) ||
      (pos_index == stream->length && pos_offset > 0)) {
    stream->index = stream->length;
    stream->bit_offset = 0;
    stream->overrun = true;
    return;
  }

  stream->index = pos_index;
  stream->bit_offset = pos_offset;
  stream->margin = 0;
}
