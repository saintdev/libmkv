/*****************************************************************************
 * matroska.h:
 *****************************************************************************
 * Copyright (C) 2007  libmkv
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
#ifndef _MATROSKA_H
#define _MATROSKA_H 1

typedef struct mk_Context_s mk_Context;

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
    mk_Track            *first_track;

    int64_t         def_duration;
    int64_t         timescale;
    int64_t         cluster_tc_scaled;

    char            wrote_header;
    unsigned        num_tracks;
    
    int64_t         frame_tc, prev_frame_tc_scaled, max_frame_tc;
    char            in_frame, keyframe;
};

struct mk_Track_s {
    int     track_id;
    mk_Track *next;
};

#endif
