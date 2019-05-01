#define MAJORVER 2              /* \ version 1.3 */
#define MINORVER 1              /* /             */

#define CRYPTMAGIC  ((unsigned char)0xB5)

#define NAMELEN 13

typedef struct {
    char signature[4];
    char majorver, minorver;
    unsigned short numfiles;
    unsigned long diroffset;
} JLIB_HEADER;

typedef struct {
    char filename[NAMELEN];
    unsigned long filesize;
    unsigned long fileusize;
    unsigned long fileoffset;
} JLIB_DIRECTORY;

/*
#define CRYPTMAGIC 0xB5B5B5B5L
void crypt(void *buffer, unsigned int size)
{

register unsigned int dsize = (size>>2);
register unsigned long *dbuffer = buffer;
size -= dsize<<2;
buffer = &dbuffer[dsize];
while (dsize>0)
  dbuffer[--dsize] ^= (unsigned long)CRYPTMAGIC;
while (size>0)
  ((unsigned char *)buffer)[--size] ^= (unsigned char)(CRYPTMAGIC&0xFF);
}
*/

// Full crypting
static void jcrypt(unsigned long seek, void *buffer, unsigned int size)
{
register unsigned char *dbuffer = (unsigned char *)buffer;
register unsigned long seed = seek*CRYPTMAGIC;
register unsigned int dsize = size;
while (dsize>0)
  {
  seed+=CRYPTMAGIC;
  *dbuffer++ ^= (unsigned char)(seed & 0xFF);
  --dsize;
  }
}

// Crypt for packing
static void jpcrypt(unsigned long magic, void *buffer, unsigned int size)
{
register unsigned char dmagic = (unsigned char)(magic & 0xFF);
register unsigned char *dbuffer = (unsigned char *)buffer;
register unsigned int dsize = size;
while (dsize>0)
  {
  *dbuffer++ ^= dmagic;
  --dsize;
  }
}


// number of unpacked bytes, return original number of packed bytes
long jtest( void *buffer, long len);

// return number of bytes written in dst.
long jpack(void *dst, void *src, long len);
long jpackf(FILE *dst, FILE *src, long len);
long junpack(void *dst, long dstlen, void *src, long len);
long junpackf(void *dst, long dstlen, FILE *src, long len);


