/* 
 * Tux Racer 
 * Copyright (C) 1999-2001 Jasmin F. Patry
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include "tuxracer.h"
#include "pic.h"

/*
 *  Memory allocation and other macros :
 */

//#define StrAlloc(n) ((char *) malloc((unsigned)(n))) //---- kong ---- already defined in pic.h

#define PicAlloc() ((Pic*)malloc((unsigned)(sizeof(Pic))))
#define PicFree(p) ((void)free((p)->filename), (void)free((char*)(p)))

Pic *PicOpen(const char *filename, short width, short height)
{
    Pic     *ppmFile;
    FILE    *fptr;

    ppmFile = (Pic *) NULL;

    if (width <= 0) {
        fprintf(stderr, "ERROR: PicOpen() - `%d' is an invalid scanline width.\n", width);
    } else if (height <= 0) {
        fprintf(stderr, "ERROR: PicOpen() - `%d' is an invalid number of scanlines.\n", height);
    } else if (! (fptr = fopen(filename, "w"))) {
        fprintf(stderr, "ERROR: PicOpen() - couldn't open file `%s' for writing.\n", filename);
    } else if (! (ppmFile = PicAlloc())) {
        fprintf(stderr, "ERROR: PicOpen() - cannot allocate `Pic' structure.\n");
    } else {
        /* setup the structure and write the header to the file */
        ppmFile->width    = width;
        ppmFile->height   = height;
        ppmFile->scanline = 0;
        ppmFile->fptr     = fptr;
        ppmFile->filename = StrAlloc(strlen(filename) + 1);
        (void)strcpy(ppmFile->filename, filename);

        /* P6 is a Raw PPM signiture and must be the first 2 bytes */
        fprintf(ppmFile->fptr, "P6\n# A Raw PPM file\n# width\n%d\n# height\n%d\n# max component value\n255\n",
            ppmFile->width,
            ppmFile->height
        );
    }
    return(ppmFile);
}

bool_t PicWriteLine(Pic *ppmFile, Pic_Pixel *pixels)
{
    int i;

    /* Don't do anything if the file is already complete: */
    if (ppmFile->scanline == ppmFile->height) {
        fprintf(stderr, "WARNING: PicWriteLine() - `%s' is complete, scanline ignored.\n", ppmFile->filename);
        return( False );
    }
    for (i = 0; i < ppmFile->width; ++i) {
        putc((char)pixels[i].r, ppmFile->fptr);
        putc((char)pixels[i].g, ppmFile->fptr);
        putc((char)pixels[i].b, ppmFile->fptr);
    }

    /* Don't forget to increment the current scanline: */
    ++ ppmFile->scanline;
    return( True );
}

void PicClose(Pic *ppmFile)
{
    if (ppmFile->scanline < ppmFile->height) {
        fprintf(stderr, "WARNING: PicClose() - only %d of %d scanlines written to `%s'.\n",
            ppmFile->scanline,
            ppmFile->height,
            ppmFile->filename
        );
    }
    fclose(ppmFile->fptr);
    PicFree(ppmFile);
}

/* EOF */