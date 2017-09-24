//#include "utf.h"
#include "debug.h"
#include "wrappers.h"
#include <unistd.h>

int
from_utf16le_to_utf16be(int infile, int outfile)
{
  int bom;
  utf16_glyph_t buf;
  ssize_t bytes_read;
  size_t bytes_to_write;
  int ret = 0;

  bom = UTF16BE;
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
  reverse_bytes(&bom, 2);
#endif
  write_to_bigendian(outfile, &bom, 2);

  while ((bytes_read = read_to_bigendian(infile, &(buf.upper_bytes), 2)) > 0) {
    bytes_to_write = 2;
    reverse_bytes(&(buf.upper_bytes), 2);
    if(is_lower_surrogate_pair(buf)) {
      if((bytes_read = read_to_bigendian(infile, &(buf.lower_bytes), 2)) < 0) {
        break;
      }
      reverse_bytes(&(buf.lower_bytes), 2);
      bytes_to_write += 2;
    }
    write_to_bigendian(outfile, &buf, bytes_to_write);
  }
  ret = bytes_read;
  return ret;
}

int
from_utf16le_to_utf8(int infile, int outfile)
{
  /* TODO */
  int ret = 0;
  int bom;
  utf8_glyph_t utf8_buf;
  ssize_t bytes_read;
  //size_t bytes_to_write;
  //size_t remaining_bytes;
  size_t size_of_glyph;
  code_point_t code_point;
  utf16_glyph_t utf16_buf;

  bom = UTF8;
  #if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
  reverse_bytes(&bom, 3);
  #endif
  //write_to_bigendian(outfile, &bom, 3);

  while ((bytes_read = read_to_bigendian(infile, &(utf16_buf.upper_bytes), 2)) > 0) {
    //bytes_to_write = 2;
    //reverse_bytes(&(utf16_buf.upper_bytes), 2);
    if(is_upper_surrogate_pair(utf16_buf)) {
      debug("32 bit");
      if((bytes_read = read_to_bigendian(infile, &(utf16_buf.lower_bytes), 2)) < 0) {
        break;
      }

      //reverse_bytes(&(utf16_buf.lower_bytes), 2); //maybe?
      //bytes_to_write += 2;
    }
    else{
      uint16_t temp = utf16_buf.upper_bytes;
      utf16_buf.upper_bytes = utf16_buf.lower_bytes;
      utf16_buf.lower_bytes = temp;
    }
    if(utf16_buf.upper_bytes != 0xFEFF){
      debug("upper: %X lower: %X", utf16_buf.upper_bytes, utf16_buf.lower_bytes);
      code_point = utf16_glyph_to_code_point(&utf16_buf);
      debug("%X", code_point);
      utf8_buf = code_point_to_utf8_glyph(code_point, &size_of_glyph);
      //debug("%X, %X, %X, %X", utf8_buf.bytes[0], utf8_buf.bytes[1], utf8_buf.bytes[2], utf8_buf.bytes[3])
      write_to_bigendian(outfile, &utf8_buf, size_of_glyph);
      utf16_buf.upper_bytes = 0;
      utf16_buf.lower_bytes = 0;
    }

  }
  ret = bytes_read;
  return ret;
}

utf16_glyph_t
code_point_to_utf16le_glyph(code_point_t code_point, size_t *size_of_glyph)
{
  utf16_glyph_t ret;

  memeset(&ret, 0, sizeof ret);
  if(is_code_point_surrogate(code_point)) {
    code_point -= 0x10000;
    ret.upper_bytes = (code_point >> 10) + 0xD800;
    ret.lower_bytes = (code_point & 0x3FF) + 0xDC00;
  #if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    reverse_bytes(&ret.upper_bytes, 2);
    reverse_bytes(&ret.lower_bytes, 2);
  #endif
    *size_of_glyph = 4;
  }
  else {
    ret.upper_bytes |= code_point;
  #if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    reverse_bytes(&ret.upper_bytes, 2);
  #endif
    *size_of_glyph = 2;
  }
  return ret;
}
