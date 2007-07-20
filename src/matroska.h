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
  uint64_t        f_pos;
  uint64_t        f_eof;


  int64_t         duration_ptr;
  int64_t         seekhead_ptr;
  int64_t         segment_ptr;

  mk_Context      *root;
  mk_Context      *freelist;
  mk_Context      *actlist;
  mk_Context      *chapters;
  mk_Context      *edition_entry;
  mk_Context      *tracks;
  mk_Context      *cues;

  uint64_t         def_duration;
  uint64_t         timescale;

  uint8_t         wrote_header;

  uint8_t         num_tracks;
  mk_Track        **tracks_arr;

  struct {
    int64_t       segmentinfo;
    int64_t       seekhead;
    int64_t       tracks;
    int64_t       cues;
    int64_t       chapters;
    int64_t       attachments;
    int64_t       tags;
  } seek_data;

  struct {
    mk_Context    *context;
    uint64_t      block_count;
    uint64_t      count;
    uint64_t      pointer;
    int64_t       tc_scaled;
  } cluster;

  int chapter_uid;
  uint8_t vlc_compat;
};

struct mk_Track_s {
  uint8_t         track_id;

  int64_t         prev_frame_tc_scaled;
  int64_t         max_frame_tc;
  uint8_t         in_frame;
  uint64_t        default_duration;
  uint8_t         track_type;
  int64_t        prev_cue_pos;

  struct {
    mk_Context    *data;
    int64_t       timecode;
    uint8_t       keyframe;
    uint8_t       lacing;
    uint8_t       lacing_num_frames;
    uint64_t      *lacing_sizes;
  } frame;
};

/* EBML */
mk_Context *mk_createContext(mk_Writer *w, mk_Context *parent, unsigned id);
int         mk_appendContextData(mk_Context *c, const void *data, uint64_t size);
int         mk_writeID(mk_Context *c, unsigned id);
int         mk_writeSize(mk_Context *c, uint64_t size);
int         mk_writeSSize(mk_Context *c, int64_t size);
int         mk_flushContextID(mk_Context *c);
int         mk_flushContextData(mk_Context *c);
int         mk_closeContext(mk_Context *c, int64_t *off);
void        mk_destroyContexts(mk_Writer *w);
int         mk_writeStr(mk_Context *c, unsigned id, const char *str);
int         mk_writeBin(mk_Context *c, unsigned id, const void *data, unsigned size);
int         mk_writeUInt(mk_Context *c, unsigned id, uint64_t ui);
int         mk_writeSInt(mk_Context *c, unsigned id, int64_t si);
int         mk_writeFloatRaw(mk_Context *c, float f);
int         mk_writeFloat(mk_Context *c, unsigned id, float f);
unsigned    mk_ebmlSizeSize(uint64_t s);
unsigned    mk_ebmlSIntSize(int64_t si);
/* EBML */

int         mk_writeVoid(mk_Context *c, uint64_t length);
mk_Context *mk_createContext(mk_Writer *w, mk_Context *parent, unsigned id);
int         mk_writeUInt(mk_Context *c, unsigned id, uint64_t ui);
int         mk_writeFloat(mk_Context *c, unsigned id, float f);
int         mk_writeStr(mk_Context *c, unsigned id, const char *str);
int         mk_writeBin(mk_Context *c, unsigned id, const void *data, unsigned size);
int         mk_flushContextData(mk_Context *c);
int         mk_closeContext(mk_Context *c, int64_t *off);
int         mk_writeSeek(mk_Writer *w, int64_t *pointer);
int         mk_writeTracks(mk_Writer *w, mk_Context *tracks);
int         mk_writeChapters(mk_Writer *w);
int         mk_seekFile(mk_Writer *w, uint64_t pos);

#endif
