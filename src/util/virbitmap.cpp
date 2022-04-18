/*
 * virbitmap.c: Simple bitmap operations
 *
 * Copyright (C) 2010-2013 Red Hat, Inc.
 * Copyright (C) 2010 Novell, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include <config.h>

#include <sys/types.h>
#include <vector>
#include <stdexcept>
#include <string>
#include <sstream>
#include <iterator>

#include "virbitmap.h"
#include "viralloc.h"
#include "virbuffer.h"
#include "virstring.h"
#include "virerror.h"

using namespace std;

#define VIR_FROM_THIS VIR_FROM_NONE

struct _virBitmap {
    vector<bool> map;
};


/**
 * virBitmapNew:
 * @size: number of bits
 *
 * Allocate a bitmap capable of containing @size bits.
 *
 * Returns a pointer to the allocated bitmap.
 */
virBitmap *
virBitmapNew(size_t size)
{
    return new virBitmap(vector<bool>(size));
}


/**
 * virBitmapFree:
 * @bitmap: previously allocated bitmap
 *
 * Free @bitmap previously allocated by virBitmapNew.
 */
void
virBitmapFree(virBitmap *bitmap)
{
    delete bitmap;
}


/**
 * virBitmapSetBit:
 * @bitmap: Pointer to bitmap
 * @b: bit position to set
 *
 * Set bit position @b in @bitmap
 *
 * Returns 0 on if bit is successfully set, -1 on error.
 */
int
virBitmapSetBit(virBitmap *bitmap,
                size_t b)
{
    try {
        bitmap->map.at(b) = true;
    } catch (const out_of_range &oor) {
        return -1;
    }
    return 0;
}


/**
 * virBitmapExpand:
 * @map: Pointer to bitmap
 * @b: bit position to include in bitmap
 *
 * Resizes the bitmap so that bit @b will fit into it. This shall be called only
 * if @b would not fit into the map.
 */
static void
virBitmapExpand(virBitmap *map,
                size_t b)
{
    try {
        map->map.resize(b + 1);
    } catch (const bad_alloc &ba) {
        abort();
    }
}


/**
 * virBitmapSetBitExpand:
 * @bitmap: Pointer to bitmap
 * @b: bit position to set
 *
 * Set bit position @b in @bitmap. Expands the bitmap as necessary so that @b is
 * included in the map.
 */
void
virBitmapSetBitExpand(virBitmap *bitmap,
                      size_t b)
{
    if (bitmap->map.size() <= b)
        virBitmapExpand(bitmap, b);
    ignore_value(virBitmapSetBit(bitmap, b));
}


/**
 * virBitmapClearBit:
 * @bitmap: Pointer to bitmap
 * @b: bit position to clear
 *
 * Clear bit position @b in @bitmap
 *
 * Returns 0 on if bit is successfully clear, -1 on error.
 */
int
virBitmapClearBit(virBitmap *bitmap,
                  size_t b)
{
    try {
        bitmap->map.at(b) = false;
    } catch (const out_of_range &oor) {
        return -1;
    }
    return 0;
}


/**
 * virBitmapClearBitExpand:
 * @bitmap: Pointer to bitmap
 * @b: bit position to set
 *
 * Clear bit position @b in @bitmap. Expands the bitmap as necessary so that
 * @b is included in the map.
 */
void
virBitmapClearBitExpand(virBitmap *bitmap,
                        size_t b)
{
    if (bitmap->map.size() <= b)
        virBitmapExpand(bitmap, b);
    ignore_value(virBitmapClearBit(bitmap, b));
}


/* Helper function. caller must ensure b < bitmap->nbits */
static bool
virBitmapIsSet(virBitmap *bitmap, size_t b)
{
    return bitmap->map.at(b) == true;
}


/**
 * virBitmapIsBitSet:
 * @bitmap: Pointer to bitmap (May be NULL)
 * @b: bit position to get
 *
 * Get setting of bit position @b in @bitmap.
 *
 * If @b is in the range of @bitmap, returns the value of the bit.
 * Otherwise false is returned.
 */
bool
virBitmapIsBitSet(virBitmap *bitmap,
                  size_t b)
{
    if (!bitmap || bitmap->map.size() <= b)
        return false;

    return virBitmapIsSet(bitmap, b);
}


/**
 * virBitmapGetBit:
 * @bitmap: Pointer to bitmap
 * @b: bit position to get
 * @result: bool pointer to receive bit setting
 *
 * Get setting of bit position @b in @bitmap and store in @result
 *
 * On success, @result will contain the setting of @b and 0 is
 * returned.  On failure, -1 is returned and @result is unchanged.
 */
int
virBitmapGetBit(virBitmap *bitmap,
                size_t b,
                bool *result)
{
    if (bitmap->map.size() <= b)
        return -1;

    *result = virBitmapIsSet(bitmap, b);
    return 0;
}


/**
 * virBitmapToString:
 * @bitmap: Pointer to bitmap
 *
 * Convert @bitmap to a number where the bit with highest position/index in
 * @bitmap represents the most significant bit and return the number in form
 * of a hexadecimal string.
 *
 * Returns pointer to the string or NULL on error.
 */
char *
virBitmapToString(virBitmap *bitmap)
{
    string ret;
    const int buflen = sizeof(unsigned long) * CHAR_BIT / 4;
    size_t len = bitmap->map.size() / buflen;
    size_t i;

    for (i = 0; i <= len; i++) {
        char buf[buflen + 1] = { };
        unsigned long val = 0;
        size_t j;

        for (j = 0; j < buflen; j++) {
            val |= bitmap->map[i * buflen + j] << j;
        }

        snprintf(buf, buflen + 1, "%0*lx", buflen / 4, val);
        ret = buf + ret;
    }

    return g_strdup(ret.c_str());
}


/**
 * virBitmapFormat:
 * @bitmap: the bitmap
 *
 * This function is the counterpart of virBitmapParse. This function creates
 * a human-readable string representing the bits in bitmap.
 *
 * See virBitmapParse for the format of @str.
 *
 * If bitmap is NULL or it has no bits set, an empty string is returned.
 *
 * Returns the string on success or NULL otherwise. Caller should call
 * VIR_FREE to free the string.
 */
char *
virBitmapFormat(virBitmap *bitmap)
{
    ostringstream buf;
    bool first = true;
    int start, cur, prev;

    if (!bitmap || (cur = virBitmapNextSetBit(bitmap, -1)) < 0) {
        return g_strdup("");
    }

    start = prev = cur;
    while (prev >= 0) {
        cur = virBitmapNextSetBit(bitmap, prev);

        if (cur == prev + 1) {
            prev = cur;
            continue;
        }

        /* cur < 0 or cur > prev + 1 */

        if (!first)
            buf << ",";
        else
            first = false;

        if (prev == start)
            buf << start;
        else
            buf << start << '-' << prev;

        start = prev = cur;
    }

    return g_strdup(buf.str().c_str());
}


/**
 * virBitmapParseInternal:
 * @str: points to a string representing a human-readable bitmap
 * @bitmap: a bitmap populated from @str
 * @limited: Don't use self-expanding APIs, report error if bit exceeds bitmap size
 *
 * This function is the counterpart of virBitmapFormat. This function creates
 * a bitmap, in which bits are set according to the content of @str.
 *
 * @str is a comma separated string of fields N, which means a number of bit
 * to set, and ^N, which means to unset the bit, and N-M for ranges of bits
 * to set.
 *
 * Returns 0 on success, or -1 in case of error.
 */
static int
virBitmapParseInternal(const char *str,
                       virBitmap *bitmap,
                       bool limited)
{
    bool neg = false;
    const char *cur = str;
    char *tmp;
    size_t i;
    int start, last;

    if (!str)
        goto error;

    virSkipSpaces(&cur);

    if (*cur == '\0')
        goto error;

    while (*cur != 0) {
        /*
         * 3 constructs are allowed:
         *     - N   : a single CPU number
         *     - N-M : a range of CPU numbers with N < M
         *     - ^N  : remove a single CPU number from the current set
         */
        if (*cur == '^') {
            cur++;
            neg = true;
        }

        if (!g_ascii_isdigit(*cur))
            goto error;

        if (virStrToLong_i(cur, &tmp, 10, &start) < 0)
            goto error;
        if (start < 0)
            goto error;

        cur = tmp;

        virSkipSpaces(&cur);

        if (*cur == ',' || *cur == 0) {
            if (neg) {
                if (limited) {
                    if (virBitmapClearBit(bitmap, start) < 0)
                        goto error;
                } else {
                    virBitmapClearBitExpand(bitmap, start);
                }
            } else {
                if (limited) {
                    if (virBitmapSetBit(bitmap, start) < 0)
                        goto error;
                } else {
                    virBitmapSetBitExpand(bitmap, start);
                }
            }
        } else if (*cur == '-') {
            if (neg)
                goto error;

            cur++;
            virSkipSpaces(&cur);

            if (virStrToLong_i(cur, &tmp, 10, &last) < 0)
                goto error;
            if (last < start)
                goto error;

            cur = tmp;

            for (i = start; i <= last; i++) {
                if (limited) {
                    if (virBitmapSetBit(bitmap, i) < 0)
                        goto error;
                } else {
                    virBitmapSetBitExpand(bitmap, i);
                }
            }

            virSkipSpaces(&cur);
        }

        if (*cur == ',') {
            cur++;
            virSkipSpaces(&cur);
            neg = false;
        } else if (*cur == 0) {
            break;
        } else {
            goto error;
        }
    }

    return 0;

 error:
    virReportError(VIR_ERR_INVALID_ARG,
                   _("Failed to parse bitmap '%1$s'"), str);
    return -1;
}


/**
 * virBitmapParse:
 * @str: points to a string representing a human-readable bitmap
 * @bitmap: a bitmap created from @str
 * @bitmapSize: the upper limit of num of bits in created bitmap
 *
 * This function is the counterpart of virBitmapFormat. This function creates
 * a bitmap, in which bits are set according to the content of @str.
 *
 * @str is a comma separated string of fields N, which means a number of bit
 * to set, and ^N, which means to unset the bit, and N-M for ranges of bits
 * to set.
 *
 * Returns 0 on success, or -1 in case of error.
 */
int
virBitmapParse(const char *str,
               virBitmap **bitmap,
               size_t bitmapSize)
{
    g_autoptr(virBitmap) tmp = virBitmapNew(bitmapSize);

    if (virBitmapParseInternal(str, tmp, true) < 0)
        return -1;

    *bitmap = tmp;
    tmp = NULL;

    return 0;
}


/**
 * virBitmapParseUnlimited:
 * @str: points to a string representing a human-readable bitmap
 *
 * This function is the counterpart of virBitmapFormat. This function creates
 * a bitmap, in which bits are set according to the content of @str.
 *
 * The bitmap is expanded to accommodate all the bits.
 *
 * @str is a comma separated string of fields N, which means a number of bit
 * to set, and ^N, which means to unset the bit, and N-M for ranges of bits
 * to set.
 *
 * Returns @bitmap on success, or NULL in case of error
 */
virBitmap *
virBitmapParseUnlimited(const char *str)
{
    virBitmap *tmp = virBitmapNew(0);

    if (virBitmapParseInternal(str, tmp, false) < 0) {
        virBitmapFree(tmp);
        return NULL;
    }

    return tmp;
}


/**
 * virBitmapNewCopy:
 * @src: the source bitmap.
 *
 * Returns a copy of bitmap @src.
 */
virBitmap *
virBitmapNewCopy(virBitmap *src)
{
    return new virBitmap(vector<bool>(src->map));
}


/**
 * virBitmapNewData:
 * @data: the data
 * @len: length of @data in bytes
 *
 * Allocate a bitmap from a chunk of data containing bits
 * information
 *
 * Returns a pointer to the allocated bitmap or NULL if
 * memory cannot be allocated.
 */
virBitmap *
virBitmapNewData(const void *data,
                 int len)
{
    virBitmap *bitmap;

    bitmap = virBitmapNew(len);

    bitmap->map.assign(len, data);

    return bitmap;
}


/**
 * virBitmapToData:
 * @data: the data
 * @len: len of @data in byte
 *
 * Convert a bitmap to a chunk of data containing bits information.
 * Data consists of sequential bytes, with lower bytes containing
 * lower bits. This function allocates @data.
 *
 * Returns 0 on success, -1 otherwise.
 */
int
virBitmapToData(virBitmap *bitmap,
                unsigned char **data,
                int *dataLen)
{
    ssize_t len;

    if ((len = virBitmapLastSetBit(bitmap)) < 0)
        len = 1;
    else
        len = (len + CHAR_BIT) / CHAR_BIT;

    *data = g_new0(unsigned char, len);
    *dataLen = len;

    virBitmapToDataBuf(bitmap, *data, *dataLen);

    return 0;
}


/**
 * virBitmapToDataBuf:
 * @bytes: pointer to memory to fill
 * @len: len of @bytes in byte
 *
 * Convert a bitmap to a chunk of data containing bits information.
 * Data consists of sequential bytes, with lower bytes containing
 * lower bits.
 */
void
virBitmapToDataBuf(virBitmap *bitmap,
                   unsigned char *bytes,
                   size_t len)
{
    size_t nbytes = MIN(len, bitmap->map.size() / CHAR_BIT);
    size_t i;

    memset(bytes, 0, len);

    for (i = 0; i < nbytes; i++) {
        size_t j;

        for (j = 0; j < CHAR_BIT; j++)
            bytes[i] |= bitmap->map[i * CHAR_BIT + j] << j;
    }
}


/**
 * virBitmapEqual:
 * @b1: bitmap 1
 * @b2: bitmap 2
 *
 * Compares two bitmaps, whose lengths can be different from each other.
 *
 * Returns true if two bitmaps have exactly the same set of bits set,
 * otherwise false.
 */
bool
virBitmapEqual(virBitmap *b1,
               virBitmap *b2)
{
    if (!b1 && !b2)
        return true;

    if (!b1 || !b2)
        return false;

    return b1->map == b2->map;
}


/**
 * virBitmapSize:
 * @bitmap: virBitmap to inspect
 *
 * Returns number of bits @bitmap can store.
 */
size_t
virBitmapSize(virBitmap *bitmap)
{
    return bitmap->map.size();
}


/**
 * virBitmapSetAll:
 * @bitmap: the bitmap
 *
 * set all bits in @bitmap.
 */
void virBitmapSetAll(virBitmap *bitmap)
{
    auto s = bitmap->map.size();

    bitmap->map.clear();
    bitmap->map.resize(s);
    bitmap->map.flip();
}


/**
 * virBitmapClearAll:
 * @bitmap: the bitmap
 *
 * clear all bits in @bitmap.
 */
void
virBitmapClearAll(virBitmap *bitmap)
{
    auto s = bitmap->map.size();

    bitmap->map.clear();
    bitmap->map.resize(s);
}


/**
 * virBitmapIsAllSet:
 * @bitmap: the bitmap to check
 *
 * check if all bits in @bitmap are set.
 */
bool
virBitmapIsAllSet(virBitmap *bitmap)
{
    auto it = bitmap->map.begin();

    for (; it != bitmap->map.end(); it++) {
        if (*it == false)
            return false;
    }

    return true;
}


/**
 * virBitmapIsAllClear:
 * @bitmap: the bitmap to check
 *
 * check if all bits in @bitmap are clear
 */
bool
virBitmapIsAllClear(virBitmap *bitmap)
{
    auto it = bitmap->map.begin();

    for (; it != bitmap->map.end(); it++) {
        if (*it == true)
            return false;
    }

    return true;
}


/**
 * virBitmapNextSetBit:
 * @bitmap: the bitmap
 * @pos: the position after which to search for a set bit
 *
 * Search for the first set bit after position @pos in bitmap @bitmap.
 * @pos can be -1 to search for the first set bit. Position starts
 * at 0.
 *
 * Returns the position of the found bit, or -1 if no bit found.
 */
ssize_t
virBitmapNextSetBit(virBitmap *bitmap,
                    ssize_t pos)
{
    size_t i;

    if (!bitmap)
        return -1;

    if (pos < 0)
        pos = -1;

    for (i = pos + 1; i< bitmap->map.size(); i++) {
        if (bitmap->map[i] == true)
            return i;
    }

    return -1;
}


/**
 * virBitmapLastSetBit:
 * @bitmap: the bitmap
 *
 * Search for the last set bit in bitmap @bitmap.
 *
 * Returns the position of the found bit, or -1 if no bit is set.
 */
ssize_t
virBitmapLastSetBit(virBitmap *bitmap)
{
    ssize_t i;

    if (!bitmap)
        return -1;

    for (i = bitmap->map.size() - 1; i >= 0; i--) {
        if (bitmap->map[i] == true)
            return i;
    }

    return -1;
}


/**
 * virBitmapNextClearBit:
 * @bitmap: the bitmap
 * @pos: the position after which to search for a clear bit
 *
 * Search for the first clear bit after position @pos in bitmap @bitmap.
 * @pos can be -1 to search for the first set bit. Position starts
 * at 0.
 *
 * Returns the position of the found bit, or -1 if no bit found.
 */
ssize_t
virBitmapNextClearBit(virBitmap *bitmap,
                      ssize_t pos)
{
    size_t i;

    if (!bitmap)
        return -1;

    if (pos < 0)
        pos = -1;

    for (i = pos + 1; i< bitmap->map.size(); i++) {
        if (bitmap->map[i] == false)
            return i;
    }

    return -1;
}


/**
 * virBitmapCountBits:
 * @bitmap: bitmap to inspect
 *
 * Return the number of bits currently set in @bitmap.
 */
size_t
virBitmapCountBits(virBitmap *bitmap)
{
    size_t i;
    size_t count = 0;

    for (i = 0; i< bitmap->map.size(); i++) {
        if (bitmap->map[i] == true)
            count++;
    }

    return count;
}


/**
 * virBitmapNewString:
 * @string: the string to be converted to a bitmap
 *
 * Allocate a bitmap and populate it from @string representing a number in
 * hexadecimal format. Note that the most significant bit of the number
 * represented by @string will correspond to the highest index/position in the
 * bitmap. The size of the returned bitmap corresponds to 4 * the length of
 * @string.
 *
 * Returns a pointer to the allocated bitmap or NULL and reports an error if
 * @string can't be converted.
 */
virBitmap *
virBitmapNewString(const char *string)
{
    virBitmap *bitmap;
    size_t i = 0;
    size_t len = strlen(string);

    if (strspn(string, "0123456789abcdefABCDEF") != len) {
        virReportError(VIR_ERR_INVALID_ARG,
                       _("Invalid hexadecimal string '%1$s'"), string);
        return NULL;
    }

    bitmap = virBitmapNew(len * 4);

    for (i = 0; i < len; i++) {
        unsigned long nibble = g_ascii_xdigit_value(string[len - i - 1]);

        if (nibble & 0x1)
            ignore_value(virBitmapSetBit(bitmap, i * 4 + 0));
        if (nibble & 0x2)
            ignore_value(virBitmapSetBit(bitmap, i * 4 + 1));
        if (nibble & 0x4)
            ignore_value(virBitmapSetBit(bitmap, i * 4 + 2));
        if (nibble & 0x8)
            ignore_value(virBitmapSetBit(bitmap, i * 4 + 3));
    }

    return bitmap;
}


/**
 * virBitmapDataFormat:
 * @data: the data
 * @len: length of @data in bytes
 *
 * Convert a chunk of data containing bits information to a human
 * readable string, e.g.: 0-1,4
 *
 * Returns: a string representation of the data, or NULL on error
 */
char *
virBitmapDataFormat(const void *data,
                    int len)
{
    g_autoptr(virBitmap) map = NULL;

    if (!(map = virBitmapNewData(data, len)))
        return NULL;

    return virBitmapFormat(map);
}


/**
 * virBitmapOverlaps:
 * @b1: virBitmap to inspect
 * @b2: virBitmap to inspect
 *
 * Returns true if at least one bit with the same index is set both in @b1 and
 * @b2.
 */
bool
virBitmapOverlaps(virBitmap *b1,
                  virBitmap *b2)
{
    size_t i;

    if (b1->map.size() > b2->map.size()) {
        virBitmap *tmp = b1;
        b1 = b2;
        b2 = tmp;
    }

    for (i = 0; i < b1->map.size(); i++) {
        if (b1->map[i] & b2->map[i])
            return true;
    }

    return false;
}


/**
 * virBitmapIntersect:
 * @a: bitmap, modified to contain result
 * @b: bitmap
 *
 * Performs intersection of two bitmaps: a = intersect(a, b)
 */
void
virBitmapIntersect(virBitmap *a,
                   virBitmap *b)
{
    size_t i;
    size_t max = MIN(a->map.size(), b->map.size());

    for (i = 0; i < max; i++) {
        a->map[i] = a->map[i] && b->map[i];
    }
}


/**
 * virBitmapUnion:
 * @a: bitmap, modified to contain result
 * @b: other bitmap
 *
 * Performs union of two bitmaps: a = union(a, b)
 */
void
virBitmapUnion(virBitmap *a,
               const virBitmap *b)
{
    size_t i;

    if (a->map.size() < b->map.size())
        virBitmapExpand(a, b->map.size() - 1);

    for (i = 0; i < b->map.size(); i++)
        a->map[i] = a->map[i] | b->map[i];
}


/**
 * virBitmapSubtract:
 * @a: minuend/result
 * @b: subtrahend
 *
 * Performs subtraction of two bitmaps: a = a - b
 */
void
virBitmapSubtract(virBitmap *a,
                  virBitmap *b)
{
    size_t i;
    size_t max = MIN(a->map.size(), b->map.size());

    for (i = 0; i < max; i++) {
        a->map[i] = a->map[i] && !b->map[i];
    }
}


/**
 * virBitmapShrink:
 * @map: Pointer to bitmap
 * @b: Size to reduce the bitmap to
 *
 * Reduces the bitmap to size @b.  Nothing will change if the size is already
 * smaller than or equal to @b.
 */
void
virBitmapShrink(virBitmap *map,
                size_t b)
{
    map->map.resize(b);
}
