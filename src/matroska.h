/*****************************************************************************
 * matroska.h:
 *****************************************************************************
 * Copyright (C) 2005 x264 project
 * $Id: $
 *
 * Authors: Mike Matsnev
 *          Nathan Caldwell
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

#define _LARGEFILE_SOURCE
#define _FILE_OFFSET_BITS 64

#define CLSIZE    1048576
#define CHECK(x)  do { if ((x) < 0) return -1; } while (0)

typedef struct mk_Context_s mk_Context;
typedef struct mk_Seek_s mk_Seek;
typedef struct mk_Chapter_s mk_Chapter;

struct mk_Context_s {
    mk_Context *next, **prev, *parent;
    mk_Writer  *owner;
    unsigned      id;

    void          *data;
    unsigned      d_cur, d_max;
};

struct mk_Writer_s {
  FILE            *fp;

  uint32_t        duration_ptr;
  uint32_t        seekhead_ptr;
  uint32_t        segment_ptr;

  mk_Context      *root;
  mk_Context      *cluster;
  mk_Context      *freelist;
  mk_Context      *actlist;

  int64_t         def_duration;
  int64_t         timescale;
  int64_t         cluster_tc_scaled;

  uint8_t         wrote_header;

  mk_Seek         *seek_data;

  mk_Chapter      *chapters;

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

struct mk_Chapter_s {
  uint64_t    chapterUID;
  uint64_t    segmentUID;
  unsigned    timeStart;
  unsigned    timeEnd;
  char        flagHidden;
  char        *chapterDisplay;
  
  mk_Chapter  *next, *prev, **tail;
};

mk_Context *mk_createContext(mk_Writer *w, mk_Context *parent, unsigned id);
int  mk_writeUInt(mk_Context *c, unsigned id, uint64_t ui);
int  mk_writeFloat(mk_Context *c, unsigned id, float f);
int  mk_writeStr(mk_Context *c, unsigned id, const char *str);
int  mk_writeBin(mk_Context *c, unsigned id, const void *data, unsigned size);
int  mk_flushContextData(mk_Context *c);
int  mk_closeContext(mk_Context *c, unsigned *off);

int  mk_writeTrack(mk_Writer *w, mk_Context *tracks, mk_Track *t);
void mk_destroyTrack(mk_Track *track);
int  mk_writeChapters(mk_Writer *w);
void mk_destroyChapters(mk_Writer *w);

#endif
