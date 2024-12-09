/*
 * Copyright (c) 1992-1997 Sam Leffler
 * Copyright (c) 1992-1997 Silicon Graphics, Inc.
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

#include "tif_config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tiffio.h"

#ifndef EXIT_SUCCESS
#define EXIT_SUCCESS 0
#endif
#ifndef EXIT_FAILURE
#define EXIT_FAILURE 1
#endif

#ifndef HAVE_GETOPT
extern int getopt(int argc, char * const argv[], const char *optstring);
#endif

#define	CopyField(tag, v) \
    if (TIFFGetField(in, tag, &v)) TIFFSetField(out, tag, v)
#define	CopyField2(tag, v1, v2) \
    if (TIFFGetField(in, tag, &v1, &v2)) TIFFSetField(out, tag, v1, v2)
#define	CopyField3(tag, v1, v2, v3) \
    if (TIFFGetField(in, tag, &v1, &v2, &v3)) TIFFSetField(out, tag, v1, v2, v3)

static	int tiffcp(TIFF*, TIFF*);
static	int cpStrips(TIFF*, TIFF*);
static	int cpTiles(TIFF*, TIFF*);

/* This is based on tiffsplit.c - only the main func differs */

int
main(int argc, char* argv[])
{
	TIFF *in, *out;

	if (argc < 3) {
                fprintf(stderr, "%s\n\n", TIFFGetVersion());
		fprintf(stderr, "usage: tiffjoin input1.tif ...inputN.tif output.tif\n");
		return (EXIT_FAILURE);
	}

	in = TIFFOpen(argv[1], "r");
	if (in == NULL) {
		return (EXIT_FAILURE);
	}
	out = TIFFOpen(argv[argc-1], TIFFIsBigEndian(in)?"wb":"wl");
	(void) TIFFClose(in);
	for (int i = 1; i < argc-1; ++i){
		in = TIFFOpen(argv[i], "r");
		
		do {
			printf("copying %s:%d to %s...\n", argv[i], TIFFCurrentDirectory(in), argv[argc-1]);
			if (!tiffcp(in, out))
				return (EXIT_FAILURE);
			TIFFWriteDirectory(out);
		} while (TIFFReadDirectory(in));
		(void) TIFFClose(in);
	}
	TIFFClose(out);
	return (EXIT_SUCCESS);
}

static int
tiffcp(TIFF* in, TIFF* out)
{
	uint16 bitspersample, samplesperpixel, compression, shortv, *shortav;
	uint32 w, l;
	float floatv;
	char *stringv;
	uint32 longv;

	CopyField(TIFFTAG_SUBFILETYPE, longv);
	CopyField(TIFFTAG_TILEWIDTH, w);
	CopyField(TIFFTAG_TILELENGTH, l);
	CopyField(TIFFTAG_IMAGEWIDTH, w);
	CopyField(TIFFTAG_IMAGELENGTH, l);
	CopyField(TIFFTAG_BITSPERSAMPLE, bitspersample);
	CopyField(TIFFTAG_SAMPLESPERPIXEL, samplesperpixel);
	CopyField(TIFFTAG_COMPRESSION, compression);
	if (compression == COMPRESSION_JPEG) {
		uint32 count = 0;
		void *table = NULL;
		if (TIFFGetField(in, TIFFTAG_JPEGTABLES, &count, &table)
		    && count > 0 && table) {
		    TIFFSetField(out, TIFFTAG_JPEGTABLES, count, table);
		}
	}
        CopyField(TIFFTAG_PHOTOMETRIC, shortv);
	CopyField(TIFFTAG_PREDICTOR, shortv);
	CopyField(TIFFTAG_THRESHHOLDING, shortv);
	CopyField(TIFFTAG_FILLORDER, shortv);
	CopyField(TIFFTAG_ORIENTATION, shortv);
	CopyField(TIFFTAG_MINSAMPLEVALUE, shortv);
	CopyField(TIFFTAG_MAXSAMPLEVALUE, shortv);
	CopyField(TIFFTAG_XRESOLUTION, floatv);
	CopyField(TIFFTAG_YRESOLUTION, floatv);
	CopyField(TIFFTAG_GROUP3OPTIONS, longv);
	CopyField(TIFFTAG_GROUP4OPTIONS, longv);
	CopyField(TIFFTAG_RESOLUTIONUNIT, shortv);
	CopyField(TIFFTAG_PLANARCONFIG, shortv);
	CopyField(TIFFTAG_ROWSPERSTRIP, longv);
	CopyField(TIFFTAG_XPOSITION, floatv);
	CopyField(TIFFTAG_YPOSITION, floatv);
	CopyField(TIFFTAG_IMAGEDEPTH, longv);
	CopyField(TIFFTAG_TILEDEPTH, longv);
	CopyField(TIFFTAG_SAMPLEFORMAT, shortv);
	CopyField2(TIFFTAG_EXTRASAMPLES, shortv, shortav);
	{ uint16 *red, *green, *blue;
	  CopyField3(TIFFTAG_COLORMAP, red, green, blue);
	}
	{ uint16 shortv2;
	  CopyField2(TIFFTAG_PAGENUMBER, shortv, shortv2);
	}
	CopyField(TIFFTAG_ARTIST, stringv);
	CopyField(TIFFTAG_IMAGEDESCRIPTION, stringv);
	CopyField(TIFFTAG_MAKE, stringv);
	CopyField(TIFFTAG_MODEL, stringv);
	CopyField(TIFFTAG_SOFTWARE, stringv);
	CopyField(TIFFTAG_DATETIME, stringv);
	CopyField(TIFFTAG_HOSTCOMPUTER, stringv);
	CopyField(TIFFTAG_PAGENAME, stringv);
	CopyField(TIFFTAG_DOCUMENTNAME, stringv);
	CopyField(TIFFTAG_BADFAXLINES, longv);
	CopyField(TIFFTAG_CLEANFAXDATA, longv);
	CopyField(TIFFTAG_CONSECUTIVEBADFAXLINES, longv);
	CopyField(TIFFTAG_FAXRECVPARAMS, longv);
	CopyField(TIFFTAG_FAXRECVTIME, longv);
	CopyField(TIFFTAG_FAXSUBADDRESS, stringv);
	CopyField(TIFFTAG_FAXDCS, stringv);
	if (TIFFIsTiled(in))
		return (cpTiles(in, out));
	else
		return (cpStrips(in, out));
}

static int
cpStrips(TIFF* in, TIFF* out)
{
	tmsize_t bufsize  = TIFFStripSize(in);
	unsigned char *buf = (unsigned char *)_TIFFmalloc(bufsize);

	if (buf) {
		tstrip_t s, ns = TIFFNumberOfStrips(in);
		uint64 *bytecounts;

		if (!TIFFGetField(in, TIFFTAG_STRIPBYTECOUNTS, &bytecounts)) {
			fprintf(stderr, "tiffjoin: strip byte counts are missing\n");
                        _TIFFfree(buf);
			return (0);
		}
		for (s = 0; s < ns; s++) {
			if (bytecounts[s] > (uint64)bufsize) {
				buf = (unsigned char *)_TIFFrealloc(buf, (tmsize_t)bytecounts[s]);
				if (!buf)
					return (0);
				bufsize = (tmsize_t)bytecounts[s];
			}
			if (TIFFReadRawStrip(in, s, buf, (tmsize_t)bytecounts[s]) < 0 ||
			    TIFFWriteRawStrip(out, s, buf, (tmsize_t)bytecounts[s]) < 0) {
				_TIFFfree(buf);
				return (0);
			}
		}
		_TIFFfree(buf);
		return (1);
	}
	return (0);
}

static int
cpTiles(TIFF* in, TIFF* out)
{
	tmsize_t bufsize = TIFFTileSize(in);
	unsigned char *buf = (unsigned char *)_TIFFmalloc(bufsize);

	if (buf) {
		ttile_t t, nt = TIFFNumberOfTiles(in);
		uint64 *bytecounts;

		if (!TIFFGetField(in, TIFFTAG_TILEBYTECOUNTS, &bytecounts)) {
			fprintf(stderr, "tiffjoin: tile byte counts are missing\n");
                        _TIFFfree(buf);
			return (0);
		}
		for (t = 0; t < nt; t++) {
			if (bytecounts[t] > (uint64) bufsize) {
				buf = (unsigned char *)_TIFFrealloc(buf, (tmsize_t)bytecounts[t]);
				if (!buf)
					return (0);
				bufsize = (tmsize_t)bytecounts[t];
			}
			if (TIFFReadRawTile(in, t, buf, (tmsize_t)bytecounts[t]) < 0 ||
			    TIFFWriteRawTile(out, t, buf, (tmsize_t)bytecounts[t]) < 0) {
				_TIFFfree(buf);
				return (0);
			}
		}
		_TIFFfree(buf);
		return (1);
	}
	return (0);
}

/* vim: set ts=8 sts=8 sw=8 noet: */
/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 8
 * fill-column: 78
 * End:
 */