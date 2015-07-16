/**
 * @author David W. Stockton
 */

#ifndef UT_BITMAP_H
#define UT_BITMAP_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * The Bitmap structure holds an extensible
 * bitmap of essentially any number of bits.
 */
typedef struct Bitmap Bitmap;

/*
 * Functions to creat, initialize, reset and
 * free Bitmpaps. All Bitmaps are the user's
 * responsibility to free. The Init function
 * can be called on Bitmaps on the stack.
 * The Reset function clears all the internal
 * content.
 */
Bitmap *bitmapNew();
void bitmapInit( Bitmap* entry );
void bitmapReset( Bitmap* entry );
void bitmapFree( Bitmap* entry );

/*
 * Functions to do the bit manipulations and
 * testing for Bitmaps.
 */
int bitIsSet( Bitmap* bitmap, int  bit );
int setBit( Bitmap* bitmap, int  bit );
int clearBit( Bitmap *bitmap, int bit );

/*
 * Utility functions analogous to strcmp(), strcpy(),
 * and strdup() for Bitmaps.
 */
int bitmapCmp( const Bitmap* b1, const Bitmap* b2 );
Bitmap *bitmapCopy( Bitmap* dest, const Bitmap* src );
Bitmap *bitmapDup( const Bitmap* src );

/*
 * Equivalent to printf, fprintf, and snprintf except
 * they take a format string and a bitmap argument.
 * The formats recognized are like those for printf
 * except only %x (for hex), %o (for octal) and %b
 * (for binary) are recognized. These can be modified
 * with '#', '0', '-' and width specifications like
 * normal printf. Text that precedes or follows the
 * format specifier is copied to the output without
 * change (be careful about '%'s in this text, use
 * "%%" to include a literal '%').
 */
int snprintfBitmap( char *buf, size_t size, const char *fmt, const Bitmap *bitmap );
int fprintfBitmap( FILE *fp, const char *fmt, const Bitmap *bitmap );
int printfBitmap( const char *fmt, const Bitmap *bitmap );

#ifdef __cplusplus
}
#endif

#endif
