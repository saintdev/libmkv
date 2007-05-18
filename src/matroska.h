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
typedef struct mk_Seek_s mk_Seek;

struct mk_Context_s {
    mk_Context *next, **prev, *parent;
    mk_Writer  *owner;
    unsigned      id;

    void          *data;
    unsigned      d_cur, d_max;
};

struct mk_Writer_s {
    FILE            *fp;

    uint32_t        duration_ptr, seekhead_ptr;

    mk_Context      *root;
    mk_Context      *cluster;
    mk_Context      *freelist;
    mk_Context      *actlist;

    int64_t         def_duration;
    int64_t         timescale;
    int64_t         cluster_tc_scaled;

    uint8_t         wrote_header;

    mk_Seek         *seek_data;
    
    uint8_t         num_tracks;
    mk_Track        **tracks;
};

struct mk_Track_s {
    int             track_id;
    
    mk_Context      *frame;
    int64_t         frame_tc;
    int64_t         prev_frame_tc_scaled;
    int64_t         max_frame_tc;
    uint8_t         in_frame;
    uint8_t         keyframe;
    mk_TrackConfig  *config;
};

struct mk_Seek_s {
    uint32_t       segmentinfo;
    uint32_t       seekhead;
    uint32_t       tracks;
    uint32_t       cues;
    uint32_t       chapters;
    uint32_t       attachments;
    uint32_t       tags;
};

#endif
