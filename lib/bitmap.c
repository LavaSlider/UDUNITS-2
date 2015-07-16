/*
 * Copyright 2015 David W. Stockton
 *
 * Utility extension for implementing extensible bitmaps.
 *
 * This file is part of the UDUNITS-2 package.  See the file COPYRIGHT
 * in the top-level source-directory of the package for copying and
 * redistribution conditions.
 */

/*LINTLIBRARY*/

#ifndef	_XOPEN_SOURCE
#   define _XOPEN_SOURCE 500
#endif

#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>	/* For printf */
#if __DARWIN_C_LEVEL < 200112L && !defined(_C99_SOURCE) && !defined(__cplusplus)
__BEGIN_DECLS
int	 snprintf(char * __restrict, size_t, const char * __restrict, ...) __printflike(3, 4);
__END_DECLS
#endif /* __DARWIN_C_LEVEL < 200112L && !defined(_C99_SOURCE) && !defined(__cplusplus) */

#include <string.h>
// Add a declaration to suppress a compiler warning when
// compiling on Mountain Lion because the declaration is
// ifdef'd out in string.h
#if __DARWIN_C_LEVEL < 200112L
char *strdup(const char *);
#endif /* __DARWIN_C_LEVEL < 200112L */
#include "udunits2.h"
#include "bitmap.h"

/******************************************************************************
 * Generic Bitmap Functions:
 ******************************************************************************/
typedef unsigned int chunkType;
struct Bitmap {
    int		chunkCount;
    chunkType	*chunks;
};

int
snprintfBitmap(
    char		*buf,
    size_t		size,
    const char		*fmt,
    const Bitmap	*bitmap)
{
    int	 pad = 0;
    int	 altForm = 0;
    int	 leftAlign = 0;
    char fieldWidthString[32] = "";
    int	 fieldWidth = 0;
    char precisionString[32] = "";
    int	 precision = 0;
    int	 format = 'x';	// Legal are: xX: Hex, bB: Binary, oO: octal
    			// Should I do unsigned and decmial?
    size_t loc = 0;
    size_t maxLoc = size - 1;
    size_t wouldBeLength = 0;

    // Figure out the format options
    if (fmt) {
	while (*fmt) {
	    if (*fmt == '%') {
		if (*(fmt+1) != '%')
		    break;
		++fmt;
	    }
	    if (loc < maxLoc) buf[loc++] = *fmt;
	    ++wouldBeLength;
	    ++fmt;
	}
	if (*fmt == '%') {			++fmt; }
	if (*fmt == '#') { altForm = 1;		++fmt; }
	if (*fmt == '-') { leftAlign = 1;	++fmt; }
	if (*fmt == '+') {			++fmt; }
	if (*fmt == '0') { pad = 1;		++fmt; }
	int i = 0;
	while (isdigit(*fmt)) {
	    if (i < 31) fieldWidthString[i++] = *fmt;
	    fieldWidth = 10 * fieldWidth + (*fmt - '0');
	    ++fmt;
	}
	fieldWidthString[i] = '\0';
	if (*fmt == '.') {
	    ++fmt;
	    i = 0;
	    while (isdigit(*fmt)) {
		if (i < 31) precisionString[i++] = *fmt;
		precision = 10 * precision + (*fmt - '0');
		++fmt;
	    }
	    precisionString[i] = '\0';
	}
	if (isalpha(*fmt)) { format = *fmt; ++fmt; }
    }


    if (bitmap) {
	int bitSpan = 0;
	int chunkSize = sizeof(*(bitmap->chunks)) << 3;
	if (bitmap->chunks) {
	    for (int i = bitmap->chunkCount - 1; i >= 0; --i) {
		if (bitmap->chunks[i] != 0) {
		    for (int bit = chunkSize - 1; bit >= 0; --bit) {
			if ((bitmap->chunks[i] & (0x01 << bit)) != 0) {
			    bitSpan = bit + 1;
			    break;
			}
		    }
		    bitSpan += chunkSize * i;
		    break;
		}
	    }
	}
	int minWidth = 0;
	int extraWidth = 0;
	char *extraChars = "";
	if (bitSpan > 0) {
	    switch (format) {
		default:	    format = 'x'; /* fall through */
		case 'X': case 'x': minWidth = 1 + ((bitSpan-1) / 4);	break;
		case 'O': case 'o': minWidth = 1 + ((bitSpan-1) / 3);	break;
		case 'B': case 'b': minWidth = bitSpan;			break;
	    }
	    if (altForm) {
		switch (format) {
		    case 'X': extraWidth = 2; extraChars = "0X"; break;
		    case 'x': extraWidth = 2; extraChars = "0x"; break;
		    case 'O':
		    case 'o': extraWidth = 1; extraChars =  "0"; break;
		    case 'B': extraWidth = 2; extraChars = "0B"; break;
		    case 'b': extraWidth = 2; extraChars = "0b"; break;
		}
	    }
	}
	else {
	    minWidth = 1;
	}
	if (fieldWidth > (minWidth+extraWidth) && !pad && !leftAlign) {
	    for (int j = fieldWidth - (minWidth+extraWidth); j > 0; --j) {
		if (loc < maxLoc) buf[loc++] = ' ';
		++wouldBeLength;
	    }
	}
	if (extraChars && *extraChars) {
	    for (int j = 0; extraChars[j]; ++j) {
		if (loc < maxLoc) buf[loc++] = extraChars[j];
		++wouldBeLength;
	    }
	}
	if (fieldWidth > (minWidth+extraWidth) && pad) {
	    for (int j = fieldWidth - (minWidth+extraWidth); j > 0; --j) {
		if (loc < maxLoc) buf[loc++] = '0';
		++wouldBeLength;
	    }
	}
	if (bitSpan > 0) {
	    int doZeros = 0;
	    int	cCnt;
	    int	oVal = 0;
	    int	oBits = (3 - (bitSpan % 3)) % 3;
	    for (int i = bitmap->chunkCount - 1; i >= 0; --i) {
		if (doZeros || bitmap->chunks[i] != 0) {
		    if (format == 'x' || format == 'X') { // Hex
			if (!doZeros) {
			    cCnt = snprintf(buf+loc, size-loc, "%x", bitmap->chunks[i]);
			} else {
			    cCnt = snprintf(buf+loc, size-loc, "%0*x", (int) (2 *
				sizeof(*bitmap->chunks)), bitmap->chunks[i]);
			}
			loc += (cCnt > size-loc) ? size-loc : cCnt;
			wouldBeLength += cCnt;
		    }
		    else if (format == 'b' || format == 'B') { // Binary
			for (int bit = chunkSize - 1; bit >= 0; --bit) {
			    if (((bitmap->chunks[i] & (0x01 << bit)) != 0)) {
				if (loc < maxLoc) buf[loc++] = '1';
				++wouldBeLength;
				doZeros = 1;
			    }
			    else if (doZeros) {
				if (loc < maxLoc) buf[loc++] = '0';
				++wouldBeLength;
			    }
			}
		    }
		    else { // Octal, format == 'o' || format == 'O'
			for (int bit = chunkSize - 1; bit >= 0; --bit) {
			    if (((bitmap->chunks[i] & (0x01 << bit)) != 0)) {
				oVal = (oVal << 1) + 1;
				++oBits;
			    }
			    else if (oVal || doZeros) {
			        oVal <<= 1;
			        ++oBits;
			    }
			    if (oBits == 3 || (bitSpan < 3 && i == 0)) {
			        cCnt = snprintf(buf+loc, size-loc, "%o", oVal);
			        loc += (cCnt > size-loc) ? size-loc : cCnt;
			        wouldBeLength += cCnt;
				doZeros = 1;
				oBits = 0;
				oVal = 0;
			    }
			}
		    }
		    doZeros = 1;
		}
	    }
	}
	else {
	    if (loc < maxLoc) buf[loc++] = '0';
	    ++wouldBeLength;
	}
	if (fieldWidth > (minWidth+extraWidth) && !pad && leftAlign) {
	    for (int j = fieldWidth - (minWidth+extraWidth); j > 0; --j) {
		if (loc < maxLoc) buf[loc++] = ' ';
		++wouldBeLength;
	    }
	}
    }
    // Passed a NULL bitmap pointer - do like null string pointer.
    // (alternatively a NULL pointer could be like an empty bitmap)
    else {
	char	formatString[69];
	int	i = 0;
	formatString[i++] = '%';
	if (altForm)	formatString[i++] = '#';
	if (leftAlign)	formatString[i++] = '-';
	if (pad)	formatString[i++] = '0';
	for (int j = 0; fieldWidthString[j]; ++j) {
	    formatString[i++] = fieldWidthString[j];
	}
	if (precision > 0) {
	    formatString[i++] = '.';
	    for (int j = 0; precisionString[j]; ++j) {
		formatString[i++] = precisionString[j];
	    }
	}
	formatString[i++] = 's';
	formatString[i] = '\0';
	i = snprintf(buf+loc, size-loc, formatString, "(null)");
	loc += (i > size-loc) ? size-loc : i;
	wouldBeLength += i;
    }
    // Output any literals from the end of the format string
    if (fmt) {
	while (*fmt) {
	    // '%'s are still special even if we don't do anything with them
	    if (*fmt == '%') {
		++fmt;	// Skip the '%'
		if (*fmt == '%') {
		    if (loc < maxLoc) buf[loc++] = *fmt;
		    ++wouldBeLength;
		    ++fmt;
		}
	    }
	    else {
		if (loc < maxLoc) buf[loc++] = *fmt;
		++wouldBeLength;
		++fmt;
	    }
	}
    }
    if (loc > maxLoc) loc = maxLoc;
    buf[loc] = '\0';
    return (int) wouldBeLength;
}

/*
 *
 */
int
fprintfBitmap(
    FILE		*fp,
    const char		*fmt,
    const Bitmap	*bitmap)
{
    // I should calculate the length here:
    //	strlen(fmt) + #bits in the bitmap + 3
    //	    (will be wrong if a field with is specified)
    char buf[4096];
    int  len = snprintfBitmap(buf, sizeof(buf), fmt, bitmap);
    if (len > sizeof(buf)) {
	// I could malloc a buffer, sntprinfBitmap() to it then free it
	fprintf (fp, "overflow printf bitmap!!\n");
	ut_handle_error_message(
	    "Couldn't print bitmap because too small of an internal buffer");
        return -1;
    }
    return fprintf(fp, "%s", buf);
}
int
printfBitmap(
     const char		*fmt,
     const Bitmap	*bitmap)
{
    return fprintfBitmap(stdout, fmt, bitmap);
}

/*
 * Initialize a newly allocated bitmap that contains randum junk
 */
void
bitmapInit(
    Bitmap*	entry)
{
    if (entry != NULL) {
	entry->chunkCount = -1;
	entry->chunks = NULL;
    }
}

/*
 * Clear and free the contents of an existing bitmap
 */
void
bitmapReset(
    Bitmap*	entry)
{
    if (entry != NULL) {
	if (entry->chunks) free((void *) entry->chunks);
	entry->chunks = NULL;
	entry->chunkCount = -1;
    }
}

/*
 * Allocate and initialize a new bitmap on the heap
 */
Bitmap *
bitmapNew() {
    Bitmap*	entry = malloc(sizeof(Bitmap));

    if (entry == NULL) {
	ut_handle_error_message(strerror(errno));
	ut_handle_error_message(
	    "Couldn't allocate %lu-byte bitmap",
	    sizeof(Bitmap) );
    }
    else {
	bitmapInit(entry);
    }

    return entry;
}

/*
 * Compare two bitmaps and return -1, 0, or 1 if b1 is <, == or > b2
 */
int
bitmapCmp(
    const Bitmap*	b1,
    const Bitmap*	b2)
{
    int i, j;
    // Both NULL or point to the same place
    if (b1 == b2)
	return 0;	// Same
    // Both empty
    if (b1 == NULL && b2 != NULL && b2->chunkCount <= 0)
	return 0;
    if (b1 != NULL && b1->chunkCount <= 0 && b2 == NULL)
	return 0;
    if (b1 != NULL && b2 != NULL && b1->chunkCount <= 0 && b2->chunkCount <= 0)
	return 0;
    // b1 NULL and b2 not (or they would be equal)
    if (b1 == NULL) {
	if (b2->chunkCount > 0 && b2->chunks != NULL) {
	    for (i = 0; i < b2->chunkCount; ++i) {
		if (b2->chunks[i] != 0)
		    return -1;	// b1 empty, b2 not, so b1 < b2
	    }
	}
	return 0;
    }
    // b1 not NULL and b2 NULL (or they would be equal)
    if (b2 == NULL) {
	if (b1->chunkCount > 0 && b1->chunks != NULL) {
	    for (i = 0; i < b1->chunkCount; ++i) {
		if (b1->chunks[i] != 0)
		    return 1;	// b2 empty, b1 not, so b1 > b2
	    }
	}
	return 0;
    }
    // Neither b1 nor b2 NULL
    if (b1->chunkCount > b2->chunkCount) {
	i = b1->chunkCount - 1;
	while (i >= b2->chunkCount) {
	    if (b1->chunks[i] != 0)
		return 1;
	    --i;
	}
    }
    else if (b1->chunkCount < b2->chunkCount) {
	i = b2->chunkCount - 1;
	while (i >= b1->chunkCount) {
	    if (b2->chunks[i--] != 0)
		return -1;
	}
    }
    else {
	i = b1->chunkCount - 1;
    }
    while (i >= 0) {
	if (b1->chunks[i] > b2->chunks[i]) {
	    return 1;
	}
	else if (b1->chunks[i] < b2->chunks[i]) {
	    return -1;
	}
	--i;
    }
    return 0;
}

/*
 * Copy bitmap src to bitmap dest
 */
Bitmap *
bitmapCopy(
    Bitmap*		dest,
    const Bitmap*	src)
{
    if (dest) {
	bitmapReset(dest);
	if (src && src->chunkCount > 0 && src->chunks) {
	    int n = src->chunkCount - 1;
	    while (n >= 0) {
		if (src->chunks[n])
		    break;
		--n;
	    }
	    dest->chunkCount = n + 1;
	    dest->chunks = malloc((n+1) * sizeof(*(dest->chunks)));
	    if (dest->chunks == NULL) {
		ut_handle_error_message(strerror(errno));
		ut_handle_error_message(
		    "Couldn't allocate %lu-byte bitmap bit array",
		    (n+1) * sizeof(*(dest->chunks)));
		dest->chunkCount = -1;
	    }
	    else {
		for (int i = 0; i <= n; ++i)
		    dest->chunks[i] = src->chunks[i];
	    }
	}
    }
    return dest;
}

/*
 * Duplicate a bitmap onto the heap
 */
Bitmap *
bitmapDup(
    const Bitmap*	src)
{
    Bitmap*		newBitmap = NULL;
    if (src) {
	newBitmap = bitmapNew();
	bitmapCopy(newBitmap, src);
    }
    return newBitmap;
}

/*
 * Free all memory associated with the bitmap
 */
void
bitmapFree(
    Bitmap*	entry)
{
    bitmapReset(entry);
    free(entry);
}

/*
 * Test if the specified bit is set in the bitmap
 */
int
bitIsSet(
    Bitmap*	bitmap,
    int		i)
{
    int	isSet = 0;
    if (i >= 0 && bitmap && bitmap->chunks) {
	int chunkSize = sizeof(*(bitmap->chunks)) << 3;
	int chunk = i / chunkSize;
	if (chunk < bitmap->chunkCount) {
	    int bit = i % chunkSize;
	    if ((bitmap->chunks[chunk] & (0x01 << bit)) != 0)
		isSet = 1;
	}
    }
    return isSet;
}

/*
 * Set the specified bit in the bitmap, return whether it
 * was previously set or not.
 */
int
setBit(
    Bitmap*	bitmap,
    int		n)
{
    int	wasSet = 0;
    if (n >= 0 && bitmap) {
	size_t chunkSize = sizeof(*(bitmap->chunks)) << 3;
	int chunk = n / chunkSize;
	// Allocate space if needed
	if (chunk >= bitmap->chunkCount || bitmap->chunks == NULL) {
	    bitmap->chunks = realloc(bitmap->chunks, (chunk+1) * sizeof(*(bitmap->chunks)));
	    if (bitmap->chunks) {
		int i = bitmap->chunkCount > 0 ? bitmap->chunkCount : 0;
		while (i <= chunk)
		    bitmap->chunks[i++] = 0;
		bitmap->chunkCount = chunk + 1;
	    }
	    else {
		ut_handle_error_message(strerror(errno));
		ut_handle_error_message(
		    "Couldn't re-allocate %lu-byte bitmap bit array",
		    (chunk+1) * sizeof(*(bitmap->chunks)));
		bitmap->chunkCount = -1;
	    }
	}
	if (chunk < bitmap->chunkCount) {
	    int bit = n % chunkSize;
	    if((bitmap->chunks[chunk] & ((chunkType) (0x01 << bit))) != 0)
		wasSet = 1;
	    bitmap->chunks[chunk] |= (chunkType) (0x01 << bit);
	}
    }
    else {
	// Error reporting?
    }
    return wasSet;
}

/*
 * Clear the specified bit in the bitmap, return whether it
 * was previously set or not.
 */
int
clearBit(
    Bitmap *bitmap,
    int			  i)
{
    int	wasSet = 0;
    if (i >= 0 && bitmap && bitmap->chunks) {
	size_t chunkSize = sizeof(*(bitmap->chunks)) << 3;
	int chunk = i / chunkSize;
	if (chunk < bitmap->chunkCount) {
	    int bit = i % chunkSize;
	    if ((bitmap->chunks[chunk] & ((chunkType) (0x01 << bit))) != 0)
		wasSet = 1;
	    bitmap->chunks[chunk] &= ~((chunkType) (0x01 << bit));
	    /* Should we do any cleanup... if all the chunks are empty
	       we could free them or truncate trailing empty chunks. */
	}
    }
    return wasSet;
}
