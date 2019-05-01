#define HAVE_LIBEXPAT 1
#define __va_copy(ap1, ap2) ((ap1) = (ap2))

// This controls the capabilities of our Character Encoding Transformations.
// Undefne for minimal.
// Define to zero for the common UTF-8, ASCII and related sets.
// Define to one for everything we know.
#if 1
/* 0 for most-used, 1 for all character sets */
#define CET_WANTED 1

/* 1 to enable as many formats as possible */
#define MAXIMAL_ENABLED 0

/* 1 to enable the CSV formats support */
#define CSVFMTS_ENABLED 1

/* 1 to enable all the filters. */
#define FILTERS_ENABLED 1

/* 1 to enable Palm PDB support */
#define  PDBFMTS_ENABLED 0

/* 1 to enable shapefile support */
#define SHAPELIB_ENABLED 0

/* 1 to inhibit our use of zlib. */
#undef ZLIB_INHIBITED
#endif


#define __WIN32__ 1
#define WIN32_LEAN_AND_MEAN 
#define DNO_USB