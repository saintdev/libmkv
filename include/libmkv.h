/*****************************************************************************
 * matroska.h:
 *****************************************************************************
 * Copyright (C) 2005 x264 project
 * $Id: $
 *
 * Authors: Mike Matsnev
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111, USA.
 *****************************************************************************/

#ifndef _LIBMKV_H
#define _LIBMKV_H 1

#ifdef __cplusplus
extern "C" {
#endif

typedef struct mk_Context_s mk_Context;
typedef struct mk_Writer_s mk_Writer;

struct mk_Context_s {
    mk_Context *next, **prev, *parent;
    mk_Writer  *owner;
    unsigned      id;

    void          *data;
    unsigned      d_cur, d_max;
};


struct mk_Writer_s {
    FILE            *fp;

    unsigned        duration_ptr;

    mk_Context          *root, *cluster, *frame;
    mk_Context          *freelist;
    mk_Context          *actlist;

    int64_t         def_duration;
    int64_t         timescale;
    int64_t         cluster_tc_scaled;
    int64_t         frame_tc, prev_frame_tc_scaled, max_frame_tc;

    char            wrote_header, in_frame, keyframe;
};

mk_Writer *mk_createWriter( const char *filename );

int  mk_writeHeader( mk_Writer *w, const char *writingApp,
                     const char *codecID,
                     const void *codecPrivate, unsigned codecPrivateSize,
                     int64_t default_frame_duration,
                     int64_t timescale,
                     unsigned width, unsigned height,
                     unsigned d_width, unsigned d_height );

int  mk_startFrame( mk_Writer *w );
int  mk_addFrameData( mk_Writer *w, const void *data, unsigned size );
int  mk_setFrameFlags( mk_Writer *w, int64_t timestamp, int keyframe );
int  mk_close( mk_Writer *w );

#ifdef __cplusplus
}
#endif

#endif
