/*
 * Copyright (c) 2008, Andrey Kiselev  <dron@ak4719.spb.edu>
 *
 * Permission to use, copy, modify, distribute, and sell this software and
 * its documentation for any purpose is hereby granted without fee, provided
 * that (i) the above copyright notices and this permission notice appear in
 * all copies of the software and related documentation, and (ii) the names of
 * Sam Leffler and Silicon Graphics may not be used in any advertising or
 * publicity relating to the software without the specific, prior written
 * permission of Sam Leffler and Silicon Graphics.
 *
 * THE SOFTWARE IS PROVIDED "AS-IS" AND WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS, IMPLIED OR OTHERWISE, INCLUDING WITHOUT LIMITATION, ANY
 * WARRANTY OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.
 *
 * IN NO EVENT SHALL SAM LEFFLER OR SILICON GRAPHICS BE LIABLE FOR
 * ANY SPECIAL, INCIDENTAL, INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY KIND,
 * OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER OR NOT ADVISED OF THE POSSIBILITY OF DAMAGE, AND ON ANY THEORY OF
 * LIABILITY, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
 * OF THIS SOFTWARE.
 */

#ifndef _TIFFTEST_
#define _TIFFTEST_

/*
 * TIFF Library
 *
 * Header file for helper testing routines.
 */

#include "tiffio.h"

int CheckShortField(TIFF *, const ttag_t, const uint16_t);
int CheckShortPairedField(TIFF *, const ttag_t, const uint16_t *);
int CheckLongField(TIFF *, const ttag_t, const uint32_t);

int write_strips(TIFF *tif, const tdata_t array, const tsize_t size);
int read_strips(TIFF *tif, const tdata_t array, const tsize_t size);
int create_image_striped(const char *name, uint32_t width, uint32_t length,
                         uint32_t rowsperstrip, uint16_t compression,
                         uint16_t spp, uint16_t bps, uint16_t photometric,
                         uint16_t sampleformat, uint16_t planarconfig,
                         const tdata_t array, const tsize_t size);
int read_image_striped(const char *name, uint32_t width, uint32_t length,
                       uint32_t rowsperstrip, uint16_t compression,
                       uint16_t spp, uint16_t bps, uint16_t photometric,
                       uint16_t sampleformat, uint16_t planarconfig,
                       const tdata_t array, const tsize_t size);

#endif /* _TIFFTEST_ */
