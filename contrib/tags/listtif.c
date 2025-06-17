/*
 * listtif.c -- lists a tiff file.
 */

#include "xtiffio.h"
#include <stdlib.h>

int main(int argc, const char **argv)
{
    const char *fname = "newtif.tif";
    int flags;

    TIFF *tif = (TIFF *)0; /* TIFF-level descriptor */

    if (argc > 1)
        fname = argv[1];

    tif = XTIFFOpen(fname, "r");
    if (!tif)
        goto failure;

    /* We want the double array listed */
    flags = TIFFPRINT_MYMULTIDOUBLES;

    TIFFPrintDirectory(tif, stdout, flags);
    XTIFFClose(tif);
    return 0;

failure:
    printf("failure in listtif\n");
    if (tif)
        XTIFFClose(tif);
    return 1;
}
