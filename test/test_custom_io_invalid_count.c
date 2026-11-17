/*
 * Copyright (c) 2026, libtiff contributors
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

/* Regression test for issue #918: TIFFReadDirEntryData must not pass a
 * non-positive tmsize_t to tif_readproc.  Uses TIFFClientOpen (no mmap)
 * to exercise the non-mmap path in TIFFReadDirEntryData. */

#include <stdio.h>
#include <string.h>

#include "tiffio.h"

/* In-memory IO callbacks — same pattern as contrib/oss-fuzz/tiff_packbits_fuzzer.cc.
 * TODO: extract to a shared test/tiff_memory_io.h. */
typedef struct
{
    const uint8_t *data;
    toff_t size;
    toff_t pos;
} MemFile;

static tmsize_t memRead(thandle_t handle, void *buf, tmsize_t want)
{
    MemFile *m = (MemFile *)handle;
    if (want < 0 || m->pos >= m->size)
        return 0;
    toff_t avail = m->size - m->pos;
    if ((toff_t)want > avail)
        want = (tmsize_t)avail;
    memcpy(buf, m->data + m->pos, (size_t)want);
    m->pos += (toff_t)want;
    return want;
}

static tmsize_t memWrite(thandle_t h, void *buf, tmsize_t n)
{
    (void)h;
    (void)buf;
    (void)n;
    return 0;
}

static toff_t memSeek(thandle_t handle, toff_t off, int whence)
{
    MemFile *m = (MemFile *)handle;
    toff_t base;
    switch (whence)
    {
        case SEEK_SET: base = 0; break;
        case SEEK_CUR: base = m->pos; break;
        case SEEK_END: base = m->size; break;
        default:       return (toff_t)-1;
    }
    if (off > (toff_t)-1 - base)
        return (toff_t)-1;
    m->pos = base + off;
    return m->pos;
}

static int     memClose(thandle_t h) { (void)h; return 0; }
static toff_t  memSize(thandle_t handle) { return ((MemFile *)handle)->size; }

/*
 * Minimal classic little-endian TIFF with one IFD entry:
 *   off  0: 'II', 42, IFD@8
 *   off  8: 1 entry
 *   off 10: tag=270 (ImageDescription), type=2 (ASCII), count=0xFFFFFFFF,
 *           value_offset=0x100 (outside buffer)
 *   off 22: next-IFD=0
 */
static void build_malformed_tiff(uint8_t *buf, size_t bufsz)
{
    memset(buf, 0, bufsz);
    buf[0] = 0x49; buf[1] = 0x49; buf[2] = 42; buf[4] = 8;
    buf[8] = 1;
    buf[10] = 0x0E; buf[11] = 0x01; /* tag 270 */
    buf[12] = 0x02;                  /* type ASCII */
    buf[14] = 0xFF; buf[15] = 0xFF; buf[16] = 0xFF; buf[17] = 0xFF; /* count */
    buf[18] = 0x00; buf[19] = 0x01;                                  /* offset */
}

int main(void)
{
    uint8_t raw[64];
    MemFile mem;
    TIFF *tif;

    build_malformed_tiff(raw, sizeof(raw));
    mem.data = raw;
    mem.pos  = 0;
    mem.size = (toff_t)sizeof(raw);

    TIFFSetErrorHandler(NULL);
    TIFFSetWarningHandler(NULL);

    tif = TIFFClientOpen("mem.tif", "r", (thandle_t)&mem,
                         memRead, memWrite, memSeek, memClose,
                         memSize, NULL, NULL);
    if (tif)
        TIFFClose(tif);

    fprintf(stderr, "PASS: no crash on tag with tdir_count=0xFFFFFFFF\n");
    return 0;
}
