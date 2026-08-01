/* stb_image_write - v1.16 - public domain - http://nothings.org/stb
   writes out PNG/BMP/TGA/JPEG/HDR images to C stdio - Sean Barrett 2010-2015 */

#ifndef INCLUDE_STB_IMAGE_WRITE_H
#define INCLUDE_STB_IMAGE_WRITE_H

#include <stdlib.h>

#ifndef STBIWDEF
#ifdef STB_IMAGE_WRITE_STATIC
#define STBIWDEF  static
#else
#ifdef __cplusplus
#define STBIWDEF  extern "C"
#else
#define STBIWDEF  extern
#endif
#endif
#endif

#ifndef STB_IMAGE_WRITE_STATIC
STBIWDEF int stbi_write_png_compression_level;
STBIWDEF int stbi_write_tga_with_rle;
STBIWDEF int stbi_write_force_png_filter;
#endif

#ifndef STBI_WRITE_NO_STDIO
STBIWDEF int stbi_write_png(char const *filename, int w, int h, int comp, const void  *data, int stride_in_bytes);
STBIWDEF int stbi_write_bmp(char const *filename, int w, int h, int comp, const void  *data);
STBIWDEF void stbi_flip_vertically_on_write(int flip_boolean);
#endif

typedef void stbi_write_func(void *context, void *data, int size);
STBIWDEF int stbi_write_png_to_func(stbi_write_func *func, void *context, int w, int h, int comp, const void  *data, int stride_in_bytes);

#endif // INCLUDE_STB_IMAGE_WRITE_H

#ifdef STB_IMAGE_WRITE_IMPLEMENTATION

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifndef STBIW_MALLOC
#define STBIW_MALLOC(sz)        malloc(sz)
#define STBIW_REALLOC(p,newsz)  realloc(p,newsz)
#define STBIW_FREE(p)           free(p)
#endif

#ifndef STBIW_MEMMOVE
#define STBIW_MEMMOVE(a,b,sz) memmove(a,b,sz)
#endif

#ifndef STBIW_ASSERT
#include <assert.h>
#define STBIW_ASSERT(x) assert(x)
#endif

#define STBIW_UCHAR(x) (unsigned char) ((x) & 0xff)

#ifdef STB_IMAGE_WRITE_STATIC
static int stbi_write_png_compression_level = 8;
static int stbi_write_tga_with_rle = 1;
static int stbi_write_force_png_filter = -1;
#else
int stbi_write_png_compression_level = 8;
int stbi_write_tga_with_rle = 1;
int stbi_write_force_png_filter = -1;
#endif

static int stbi__flip_vertically_on_write = 0;

STBIWDEF void stbi_flip_vertically_on_write(int flag)
{
   stbi__flip_vertically_on_write = flag;
}

typedef struct
{
   stbi_write_func *func;
   void *context;
   unsigned char buffer[64];
   int buf_used;
} stbi__write_context;

static void stbi__start_write_callbacks(stbi__write_context *s, stbi_write_func *c, void *context)
{
   s->func    = c;
   s->context = context;
}

#ifndef STBI_WRITE_NO_STDIO
static void stbi__stdio_write(void *context, void *data, int size)
{
   fwrite(data,1,size,(FILE*) context);
}

static FILE *stbiw__fopen(char const *filename, char const *mode)
{
   FILE *f = fopen(filename, mode);
   return f;
}

static int stbi__start_write_file(stbi__write_context *s, const char *filename)
{
   FILE *f = stbiw__fopen(filename, "wb");
   stbi__start_write_callbacks(s, stbi__stdio_write, (void *) f);
   return f != NULL;
}

static void stbi__end_write_file(stbi__write_context *s)
{
   fclose((FILE *)s->context);
}
#endif

static int stbi_write_bmp_core(stbi__write_context *s, int x, int y, int comp, const void *data)
{
   int pad = (-x*3) & 3;
   int filesize = 14 + 40 + (x*3 + pad) * y;
   unsigned char header[54] = {'B','M', (unsigned char)filesize, (unsigned char)(filesize>>8), (unsigned char)(filesize>>16), (unsigned char)(filesize>>24), 0,0,0,0, 54,0,0,0, 40,0,0,0, (unsigned char)x,(unsigned char)(x>>8),(unsigned char)(x>>16),(unsigned char)(x>>24), (unsigned char)y,(unsigned char)(y>>8),(unsigned char)(y>>16),(unsigned char)(y>>24), 1,0, 24,0};
   s->func(s->context, header, 54);

   const unsigned char* pixels = (const unsigned char*)data;
   unsigned char zero[4] = {0,0,0,0};
   for (int j = y - 1; j >= 0; --j) {
       for (int i = 0; i < x; ++i) {
           int idx = (j * x + i) * comp;
           unsigned char bgr[3] = { pixels[idx+2], pixels[idx+1], pixels[idx] };
           s->func(s->context, bgr, 3);
       }
       if (pad) s->func(s->context, zero, pad);
   }
   return 1;
}

STBIWDEF int stbi_write_bmp(char const *filename, int x, int y, int comp, const void *data)
{
   stbi__write_context s = { 0 };
   if (stbi__start_write_file(&s,filename)) {
      int r = stbi_write_bmp_core(&s, x, y, comp, data);
      stbi__end_write_file(&s);
      return r;
   } else
      return 0;
}

/* Minimal PNG Writer for STB */
static unsigned int stbiw__crc32(unsigned char *buffer, int len) {
   static unsigned int crc_table[256];
   static int crc_table_computed = 0;
   if (!crc_table_computed) {
      for (unsigned int n = 0; n < 256; n++) {
         unsigned int c = n;
         for (int k = 0; k < 8; k++) {
            if (c & 1) c = 0xedb88320L ^ (c >> 1);
            else c = c >> 1;
         }
         crc_table[n] = c;
      }
      crc_table_computed = 1;
   }
   unsigned int c = 0xffffffffL;
   for (int i = 0; i < len; i++) {
      c = crc_table[(c ^ buffer[i]) & 0xff] ^ (c >> 8);
   }
   return c ^ 0xffffffffL;
}

static void stbiw__write_chunk(stbi__write_context *s, const char *tag, unsigned char *data, int len) {
   unsigned char header[8];
   header[0] = (unsigned char)(len >> 24);
   header[1] = (unsigned char)(len >> 16);
   header[2] = (unsigned char)(len >> 8);
   header[3] = (unsigned char)(len);
   header[4] = tag[0]; header[5] = tag[1]; header[6] = tag[2]; header[7] = tag[3];
   s->func(s->context, header, 8);

   unsigned int crc = stbiw__crc32((unsigned char*)tag, 4);
   if (len > 0) {
      s->func(s->context, data, len);
      static unsigned int crc_table[256];
      static int crc_table_computed = 0;
      if (!crc_table_computed) {
         for (unsigned int n = 0; n < 256; n++) {
            unsigned int c = n;
            for (int k = 0; k < 8; k++) {
               if (c & 1) c = 0xedb88320L ^ (c >> 1);
               else c = c >> 1;
            }
            crc_table[n] = c;
         }
         crc_table_computed = 1;
      }
      for (int i = 0; i < len; i++) {
         crc = crc_table[(crc ^ data[i]) & 0xff] ^ (crc >> 8);
      }
   }
   crc ^= 0xffffffffL;
   unsigned char cbuf[4];
   cbuf[0] = (unsigned char)(crc >> 24);
   cbuf[1] = (unsigned char)(crc >> 16);
   cbuf[2] = (unsigned char)(crc >> 8);
   cbuf[3] = (unsigned char)(crc);
   s->func(s->context, cbuf, 4);
}

STBIWDEF int stbi_write_png_to_func(stbi_write_func *func, void *context, int x, int y, int comp, const void  *data, int stride_in_bytes) {
   stbi__write_context s = { 0 };
   stbi__start_write_callbacks(&s, func, context);

   // PNG Header
   unsigned char sig[8] = { 137, 80, 78, 71, 13, 10, 26, 10 };
   s.func(s.context, sig, 8);

   // IHDR
   unsigned char ihdr[13];
   ihdr[0] = (unsigned char)(x >> 24); ihdr[1] = (unsigned char)(x >> 16); ihdr[2] = (unsigned char)(x >> 8); ihdr[3] = (unsigned char)(x);
   ihdr[4] = (unsigned char)(y >> 24); ihdr[5] = (unsigned char)(y >> 16); ihdr[6] = (unsigned char)(y >> 8); ihdr[7] = (unsigned char)(y);
   ihdr[8] = 8; // bit depth
   ihdr[9] = (comp == 4) ? 6 : 2; // color type RGBA (6) or RGB (2)
   ihdr[10] = 0; ihdr[11] = 0; ihdr[12] = 0;
   stbiw__write_chunk(&s, "IHDR", ihdr, 13);

   // IDAT - Uncompressed DEFLATE store blocks
   int lineLen = x * comp + 1;
   int rawLen = lineLen * y;
   int blockCount = (rawLen + 65534) / 65535;
   int idatLen = 2 + rawLen + blockCount * 5 + 4;

   unsigned char* idat = (unsigned char*)STBIW_MALLOC(idatLen);
   if (!idat) return 0;

   int p = 0;
   idat[p++] = 0x78; idat[p++] = 0x01; // zlib header

   const unsigned char* src = (const unsigned char*)data;
   int stride = (stride_in_bytes == 0) ? x * comp : stride_in_bytes;

   unsigned int s1 = 1, s2 = 0;

   for (int j = 0; j < y; ++j) {
      const unsigned char* row = src + (stbi__flip_vertically_on_write ? (y - 1 - j) : j) * stride;

      // Uncompressed block header if needed
      for (int i = -1; i < x * comp; ++i) {
         if ((p - 2) % 65540 == 0 || (i == -1 && j == 0)) {
            int currentBlock = std::min(65535, rawLen - (j * lineLen + (i + 1)));
            idat[p++] = (j == y - 1 && i >= x * comp - 1) ? 1 : 0;
            idat[p++] = (unsigned char)(currentBlock & 0xff);
            idat[p++] = (unsigned char)((currentBlock >> 8) & 0xff);
            idat[p++] = (unsigned char)(~currentBlock & 0xff);
            idat[p++] = (unsigned char)((~currentBlock >> 8) & 0xff);
         }

         unsigned char b = (i == -1) ? 0 : row[i];
         idat[p++] = b;

         s1 = (s1 + b) % 65521;
         s2 = (s2 + s1) % 65521;
      }
   }

   // Adler32
   unsigned int adler = (s2 << 16) | s1;
   idat[p++] = (unsigned char)(adler >> 24);
   idat[p++] = (unsigned char)(adler >> 16);
   idat[p++] = (unsigned char)(adler >> 8);
   idat[p++] = (unsigned char)(adler);

   stbiw__write_chunk(&s, "IDAT", idat, p);
   STBIW_FREE(idat);

   // IEND
   stbiw__write_chunk(&s, "IEND", NULL, 0);
   return 1;
}

STBIWDEF int stbi_write_png(char const *filename, int x, int y, int comp, const void  *data, int stride_in_bytes) {
   stbi__write_context s = { 0 };
   if (stbi__start_write_file(&s,filename)) {
      int r = stbi_write_png_to_func(stbi__stdio_write, s.context, x, y, comp, data, stride_in_bytes);
      stbi__end_write_file(&s);
      return r;
   } else
      return 0;
}

#endif // STB_IMAGE_WRITE_IMPLEMENTATION
