/*****************************************************************************
 * matroska.c:
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
#include "libmkv.h"
#include "matroska.h"
#include "config.h"

mk_Context *mk_createContext(mk_Writer *w, mk_Context *parent, unsigned id) {
  mk_Context  *c;

  if (w->freelist) {
    c = w->freelist;
    w->freelist = w->freelist->next;
  } else {
    c = malloc(sizeof(*c));
    memset(c, 0, sizeof(*c));
  }

  if (c == NULL)
    return NULL;

  c->parent = parent;
  c->owner = w;
  c->id = id;

  if (c->owner->actlist)
    c->owner->actlist->prev = &c->next;
  c->next = c->owner->actlist;
  c->prev = &c->owner->actlist;

  return c;
}

static int    mk_appendContextData(mk_Context *c, const void *data, unsigned size) {
  unsigned  ns = c->d_cur + size;

  if (ns > c->d_max) {
    void      *dp;
    unsigned  dn = c->d_max ? c->d_max << 1 : 16;
    while (ns > dn)
      dn <<= 1;

    dp = realloc(c->data, dn);
    if (dp == NULL)
      return -1;

    c->data = dp;
    c->d_max = dn;
  }

  memcpy((char*)c->data + c->d_cur, data, size);

  c->d_cur = ns;

  return 0;
}

static int    mk_writeID(mk_Context *c, unsigned id) {
  unsigned char   c_id[4] = { id >> 24, id >> 16, id >> 8, id };

  if (c_id[0])
    return mk_appendContextData(c, c_id, 4);
  if (c_id[1])
    return mk_appendContextData(c, c_id+1, 3);
  if (c_id[2])
    return mk_appendContextData(c, c_id+2, 2);
  return mk_appendContextData(c, c_id+3, 1);
}

static int    mk_writeSize(mk_Context *c, uint64_t size) {
  unsigned char   c_size[8] = { 0x01, size >> 48, size >> 40, size >> 32, size >> 24, size >> 16, size >> 8, size };

  if (size < 0x7f) {
    c_size[7] |= 0x80;
    return mk_appendContextData(c, c_size+7, 1);
  }
  if (size < 0x3fff) {
    c_size[6] |= 0x40;
    return mk_appendContextData(c, c_size+6, 2);
  }
  if (size < 0x1fffff) {
    c_size[5] |= 0x20;
    return mk_appendContextData(c, c_size+5, 3);
  }
  if (size < 0x0fffffff) {
    c_size[4] |= 0x10;
    return mk_appendContextData(c, c_size+4, 4);
  }
  if (size < 0x07ffffffff) {
    c_size[3] |= 0x08;
    return mk_appendContextData(c, c_size+3, 5);
  }
  if (size < 0x03ffffffffff) {
    c_size[2] |= 0x04;
    return mk_appendContextData(c, c_size+2, 6);
  }
  if (size < 0x01ffffffffffff) {
    c_size[1] |= 0x02;
    return mk_appendContextData(c, c_size+1, 7);
  }
  return mk_appendContextData(c, c_size, 9);
}

static int    mk_flushContextID(mk_Context *c) {
  unsigned char size_undf[8] = {0x01, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

  if (c->id == 0)
    return 0;

  CHECK(mk_writeID(c->parent, c->id));
  CHECK(mk_appendContextData(c->parent, &size_undf, 8));

  c->id = 0;

  return 0;
}

int    mk_flushContextData(mk_Context *c) {
  if (c->d_cur == 0)
    return 0;

  if (c->parent)
    CHECK(mk_appendContextData(c->parent, c->data, c->d_cur));
  else
    if (fwrite(c->data, c->d_cur, 1, c->owner->fp) != 1)
      return -1;

  c->d_cur = 0;

  return 0;
}

int    mk_closeContext(mk_Context *c, off_t *off) {
  if (c->id) {
    CHECK(mk_writeID(c->parent, c->id));
    CHECK(mk_writeSize(c->parent, c->d_cur));
  }

  if (c->parent && off != NULL)
    *off += c->parent->d_cur;

  CHECK(mk_flushContextData(c));

  if (c->next)
    c->next->prev = c->prev;
  *(c->prev) = c->next;
  c->next = c->owner->freelist;
  c->owner->freelist = c;

  return 0;
}

static void   mk_destroyContexts(mk_Writer *w) {
  mk_Context  *cur, *next;

  for (cur = w->freelist; cur; cur = next) {
    next = cur->next;
    free(cur->data);
    free(cur);
  }

  for (cur = w->actlist; cur; cur = next) {
    next = cur->next;
    free(cur->data);
    free(cur);
  }

  w->freelist = w->actlist = w->root = NULL;
}

int    mk_writeStr(mk_Context *c, unsigned id, const char *str) {
  size_t  len = strlen(str);

  CHECK(mk_writeID(c, id));
  CHECK(mk_writeSize(c, len));
  CHECK(mk_appendContextData(c, str, len));
  return 0;
}

int    mk_writeBin(mk_Context *c, unsigned id, const void *data, unsigned size) {
  CHECK(mk_writeID(c, id));
  CHECK(mk_writeSize(c, size));
  CHECK(mk_appendContextData(c, data, size));
  return 0;
}

int    mk_writeUInt(mk_Context *c, unsigned id, uint64_t ui) {
  unsigned char   c_ui[8] = { ui >> 56, ui >> 48, ui >> 40, ui >> 32, ui >> 24, ui >> 16, ui >> 8, ui };
  unsigned    i = 0;

  CHECK(mk_writeID(c, id));
  while (i < 7 && c_ui[i] == 0)
    ++i;
  CHECK(mk_writeSize(c, 8 - i));
  CHECK(mk_appendContextData(c, c_ui+i, 8 - i));
  return 0;
}

static int    mk_writeSInt(mk_Context *c, unsigned id, int64_t si) {
  unsigned char   c_si[8] = { si >> 56, si >> 48, si >> 40, si >> 32, si >> 24, si >> 16, si >> 8, si };
  unsigned    i = 0;

  CHECK(mk_writeID(c, id));
  if (si < 0)
    while (i < 7 && c_si[i] == 0xff && c_si[i+1] & 0x80)
      ++i;
  else
    while (i < 7 && c_si[i] == 0 && !(c_si[i+1] & 0x80))
      ++i;
  CHECK(mk_writeSize(c, 8 - i));
  CHECK(mk_appendContextData(c, c_si+i, 8 - i));
  return 0;
}

static int    mk_writeFloatRaw(mk_Context *c, float f) {
  union {
    float f;
    unsigned u;
  } u;
  unsigned char c_f[4];

  u.f = f;
  c_f[0] = u.u >> 24;
  c_f[1] = u.u >> 16;
  c_f[2] = u.u >> 8;
  c_f[3] = u.u;

  return mk_appendContextData(c, c_f, 4);
}

int    mk_writeFloat(mk_Context *c, unsigned id, float f) {
  CHECK(mk_writeID(c, id));
  CHECK(mk_writeSize(c, 4));
  CHECK(mk_writeFloatRaw(c, f));
  return 0;
}

static int    mk_writeVoid(mk_Context *c, unsigned length) {
  char *c_void = calloc(length, sizeof(char));

  CHECK(mk_writeID(c, 0xec));
  CHECK(mk_writeSize(c, length));
  CHECK(mk_appendContextData(c, c_void, length));
  free(c_void);
  return 0;
}

static unsigned   mk_ebmlSizeSize(uint64_t s) {
  if (s < 0x7f)
    return 1;
  if (s < 0x3fff)
    return 2;
  if (s < 0x1fffff)
    return 3;
  if (s < 0x0fffffff)
    return 4;
  if (s < 0x07ffffffff)
    return 5;
  if (s < 0x03ffffffffff)
    return 6;
  if (s < 0x01ffffffffffff)
    return 7;
  return 8
}

static unsigned   mk_ebmlSIntSize(int64_t si) {
  unsigned char   c_si[8] = { si >> 56, si >> 48, si >> 40, si >> 32, si >> 24, si >> 16, si >> 8, si };
  unsigned    i = 0;

  if (si < 0)
    while (i < 7 && c_si[i] == 0xff && c_si[i+1] & 0x80)
      ++i;
  else
    while (i < 7 && c_si[i] == 0 && !(c_si[i+1] & 0x80))
      ++i;

  return 8 - i;
}

mk_Writer *mk_createWriter(const char *filename, int64_t timescale, uint8_t vlc_compat) {
  mk_Writer *w = malloc(sizeof(*w));
  if (w == NULL)
    return NULL;

  memset(w, 0, sizeof(*w));

  w->root = mk_createContext(w, NULL, 0);
  if (w->root == NULL) {
    free(w);
    return NULL;
  }

  if ((w->cues = mk_createContext(w, w->root, 0x1c53bb6b)) == NULL) // Cues
  {
    mk_destroyContexts(w);
    free(w);
    return NULL;
  }

  w->fp = fopen(filename, "wb");
  if (w->fp == NULL) {
    mk_destroyContexts(w);
    free(w);
    return NULL;
  }

  w->timescale = timescale;
  w->vlc_compat = vlc_compat;

  return w;
}

int   mk_writeHeader(mk_Writer *w, const char *writingApp) {
  mk_Context  *c;

  if (w->wrote_header)
    return -1;

  if ((c = mk_createContext(w, w->root, 0x1a45dfa3)) == NULL) // EBML
    return -1;
  CHECK(mk_writeUInt(c, 0x4286, 1)); // EBMLVersion
  CHECK(mk_writeUInt(c, 0x42f7, 1)); // EBMLReadVersion
  CHECK(mk_writeUInt(c, 0x42f2, 4)); // EBMLMaxIDLength
  CHECK(mk_writeUInt(c, 0x42f3, 8)); // EBMLMaxSizeLength
  CHECK(mk_writeStr(c, 0x4282, "matroska")); // DocType
  CHECK(mk_writeUInt(c, 0x4287, 1)); // DocTypeVersion
  CHECK(mk_writeUInt(c, 0x4285, 1)); // DocTypeReadversion
  CHECK(mk_closeContext(c, 0));

  if ((c = mk_createContext(w, w->root, 0x18538067)) == NULL) // Segment
    return -1;
  CHECK(mk_flushContextID(c));
  w->segment_ptr = c->d_cur;
  CHECK(mk_closeContext(c, &w->segment_ptr));

  if (w->vlc_compat)
  {
    CHECK(mk_writeVoid(w->root, 256));  // 256 bytes should be enough room for our Seek entries.
  } else
  {
    w->seek_data.seekhead = 0x80000000;
    CHECK(mk_writeSeek(w, &w->seekhead_ptr));
    w->seek_data.seekhead = 0;
  }

  if ((c = mk_createContext(w, w->root, 0x1549a966)) == NULL) // SegmentInfo
    return -1;
  w->seek_data.segmentinfo = w->root->d_cur - w->segment_ptr;
  CHECK(mk_writeStr(c, 0x4d80, PACKAGE_STRING)); // MuxingApp
  CHECK(mk_writeStr(c, 0x5741, writingApp)); // WritingApp
  CHECK(mk_writeUInt(c, 0x2ad7b1, w->timescale)); // TimecodeScale
  CHECK(mk_writeFloat(c, 0x4489, 0)); // Duration
  w->duration_ptr = c->d_cur - 4;
  CHECK(mk_closeContext(c, &w->duration_ptr));

  w->seek_data.tracks = w->root->d_cur - w->segment_ptr;
  
  if (w->tracks) {
    CHECK(mk_closeContext(w->tracks, 0));
  }
  
  CHECK(mk_flushContextData(w->root));

  w->wrote_header = 1;
  w->def_duration = w->tracks_arr[0]->default_duration;
  return 0;
}

static int mk_closeCluster(mk_Writer *w) {
  if (w->cluster.context == NULL)
    return 0;
  w->cluster.count++;
  CHECK(mk_closeContext(w->cluster.context, 0));
  w->cluster.context = NULL;
  CHECK(mk_flushContextData(w->root));
  return 0;
}

int   mk_flushFrame(mk_Writer *w, mk_Track *track) {
  mk_Context *c;
  int64_t   delta, ref = 0;
  unsigned  fsize, bgsize;
  unsigned char c_delta_flags[3];
  int i;

  if (!track->in_frame)
    return 0;

  delta = track->frame_tc/w->timescale - w->cluster.tc_scaled;
  if (delta > 32767ll || delta < -32768ll)
    CHECK(mk_closeCluster(w));

  if (w->cluster.context == NULL) {
    w->cluster.tc_scaled = track->frame_tc / w->timescale;
    w->cluster.context = mk_createContext(w, w->root, 0x1f43b675); // Cluster
    if (w->cluster.context == NULL)
      return -1;

  w->cluster.pointer = ftell(w->fp) - w->segment_ptr;

    CHECK(mk_writeUInt(w->cluster.context, 0xe7, w->cluster.tc_scaled)); // Cluster Timecode

    delta = 0;
    w->cluster.block_count = 0;

    if (w->cluster.count % 5 == 0) {
      for(i = 0; i < w->num_tracks; i++)
        w->tracks_arr[i]->cue_flag = 1;
    }
  }

  fsize = track->frame ? track->frame->d_cur : 0;
  bgsize = fsize + 4 + mk_ebmlSizeSize(fsize + 4) + 1;
  if (!track->keyframe) {
    ref = track->prev_frame_tc_scaled - w->cluster.tc_scaled - delta;
    bgsize += 1 + 1 + mk_ebmlSIntSize(ref);
  }

  CHECK(mk_writeID(w->cluster.context, 0xa0)); // BlockGroup
  CHECK(mk_writeSize(w->cluster.context, bgsize));
  CHECK(mk_writeID(w->cluster.context, 0xa1)); // Block
  CHECK(mk_writeSize(w->cluster.context, fsize + 4));
  CHECK(mk_writeSize(w->cluster.context, track->track_id)); // track number

  w->cluster.block_count++;

  c_delta_flags[0] = delta >> 8;
  c_delta_flags[1] = delta;
  c_delta_flags[2] = 0;
  CHECK(mk_appendContextData(w->cluster.context, c_delta_flags, 3));
  if (track->frame) {
    CHECK(mk_appendContextData(w->cluster.context, track->frame->data, track->frame->d_cur));
    track->frame->d_cur = 0;
  }
  if (!track->keyframe)
    CHECK(mk_writeSInt(w->cluster.context, 0xfb, ref)); // ReferenceBlock

  if (track->cue_flag && track->keyframe) {
    if (w->cue_point.timecode != track->frame_tc) {
      if (w->cue_point.context != NULL) {
        CHECK(mk_closeContext(w->cue_point.context, 0));
        w->cue_point.context = NULL;
      }
    }
    if (w->cue_point.context == NULL) {
      if ((w->cue_point.context = mk_createContext(w, w->cues, 0xbb)) == NULL)  // CuePoint
        return -1;
      CHECK(mk_writeUInt(w->cue_point.context, 0xb3, track->frame_tc)); // CueTime
      w->cue_point.timecode = track->frame_tc;
    }

    if ((c = mk_createContext(w, w->cue_point.context, 0xb7)) == NULL)  // CueTrackPositions
      return -1;
    CHECK(mk_writeUInt(c, 0xf7, track->track_id)); // CueTrack
    CHECK(mk_writeUInt(c, 0xf1, w->cluster.pointer));  // CueClusterPosition
    CHECK(mk_writeUInt(c, 0x5378, w->cluster.block_count));  // CueBlockNumber
    CHECK(mk_closeContext(c, 0));
  }

  track->in_frame = 0;
  track->prev_frame_tc_scaled = w->cluster.tc_scaled + delta;

  if (w->cluster.context->d_cur > CLSIZE)
    CHECK(mk_closeCluster(w));

  return 0;
}

int   mk_startFrame(mk_Writer *w, mk_Track *track) {
  if (mk_flushFrame(w, track) < 0)
    return -1;

  track->in_frame = 1;
  track->keyframe = 0;

  return 0;
}

int   mk_setFrameFlags(mk_Writer *w, mk_Track *track, int64_t timestamp, int keyframe) {
  if (!track->in_frame)
    return -1;

  track->frame_tc = timestamp;
  track->keyframe = keyframe != 0;

  if (track->max_frame_tc < timestamp)
    track->max_frame_tc = timestamp;

  return 0;
}

int   mk_addFrameData(mk_Writer *w, mk_Track *track, const void *data, unsigned size) {
  if (!track->in_frame)
    return -1;

  if (track->frame == NULL)
    if ((track->frame = mk_createContext(w, NULL, 0)) == NULL)
      return -1;

  return mk_appendContextData(track->frame, data, size);
}

/* The offset of the SeekHead is returned in *pointer. */
int mk_writeSeek(mk_Writer *w, off_t *pointer) {
  mk_Context  *c, *s;
  off_t   seekhead_ptr;

  if ((c = mk_createContext(w, w->root, 0x114d9b74)) == NULL) // SeekHead
    return -1;
  if (pointer != NULL)
    seekhead_ptr = ftell(w->fp);
  if (w->seek_data.seekhead) {
    if ((s = mk_createContext(w, c, 0x4dbb)) == NULL) // Seek
      return -1;
    CHECK(mk_writeUInt(s, 0x53ab, 0x114d9b74)); // SeekID
    CHECK(mk_writeUInt(s, 0x53ac, w->seek_data.seekhead)); // SeekPosition
    CHECK(mk_closeContext(s, 0));
  }
  if (w->seek_data.segmentinfo) {
    if ((s = mk_createContext(w, c, 0x4dbb)) == NULL) // Seek
      return -1;
    CHECK(mk_writeUInt(s, 0x53ab, 0x1549a966)); // SeekID
    CHECK(mk_writeUInt(s, 0x53ac, w->seek_data.segmentinfo)); // SeekPosition
    CHECK(mk_closeContext(s, 0));
  }
  if (w->seek_data.tracks) {
    if ((s = mk_createContext(w, c, 0x4dbb)) == NULL) // Seek
      return -1;
    CHECK(mk_writeUInt(s, 0x53ab, 0x1654ae6b)); // SeekID
    CHECK(mk_writeUInt(s, 0x53ac, w->seek_data.tracks)); // SeekPosition
    CHECK(mk_closeContext(s, 0));
  }
  if (w->seek_data.cues) {
    if ((s = mk_createContext(w, c, 0x4dbb)) == NULL) // Seek
      return -1;
    CHECK(mk_writeUInt(s, 0x53ab, 0x1c53bb6b)); // SeekID
    CHECK(mk_writeUInt(s, 0x53ac, w->seek_data.cues)); // SeekPosition
    CHECK(mk_closeContext(s, 0));
  }
  if (w->seek_data.attachments) {
    if ((s = mk_createContext(w, c, 0x4dbb)) == NULL) // Seek
      return -1;
    CHECK(mk_writeUInt(s, 0x53ab, 0x1941a469)); // SeekID
    CHECK(mk_writeUInt(s, 0x53ac, w->seek_data.attachments)); // SeekPosition
    CHECK(mk_closeContext(s, 0));
  }
  if (w->seek_data.chapters) {
    if ((s = mk_createContext(w, c, 0x4dbb)) == NULL) // Seek
      return -1;
    CHECK(mk_writeUInt(s, 0x53ab, 0x1043a770)); // SeekID
    CHECK(mk_writeUInt(s, 0x53ac, w->seek_data.chapters)); // SeekPosition
    CHECK(mk_closeContext(s, 0));
  }
  if (w->seek_data.tags) {
    if ((s = mk_createContext(w, c, 0x4dbb)) == NULL) // Seek
      return -1;
    CHECK(mk_writeUInt(s, 0x53ab, 0x1254c367)); // SeekID
    CHECK(mk_writeUInt(s, 0x53ac, w->seek_data.tags)); // SeekPosition
    CHECK(mk_closeContext(s, 0));
  }
  CHECK(mk_closeContext(c, 0));

  if (pointer != NULL)
    *pointer = seekhead_ptr;

  return 0;
}

int   mk_close(mk_Writer *w) {
  int   i, ret = 0;
  mk_Track *tk;

  for (i = w->num_tracks - 1; i >= 0; i--)
  {
    tk = w->tracks_arr[i];
    if (mk_flushFrame(w, tk) < 0)
      ret = -1;
  }
  
  if (mk_closeCluster(w) < 0)
    ret = -1;

  if (w->chapters != NULL)
  {
    w->seek_data.chapters = ftell(w->fp) - w->segment_ptr;
    mk_writeChapters(w);
    if (mk_flushContextData(w->root) < 0)
      ret = -1;
  }

  w->seek_data.cues = ftell(w->fp) - w->segment_ptr;
  if (w->cue_point.context != NULL)
    if (mk_closeContext(w->cue_point.context, 0) < 0)
      ret = -1;
  if (mk_closeContext(w->cues, 0) < 0)
    ret = -1;
  if (mk_flushContextData(w->root) < 0)
    ret = -1;

  if (w->wrote_header) {
    if (w->vlc_compat)
      fseek(w->fp, w->segment_ptr, SEEK_SET);

    if (mk_writeSeek(w, &w->seek_data.seekhead) < 0)
      ret = -1;
    w->seek_data.seekhead -= w->segment_ptr;

    if (w->vlc_compat)
    {
      if (mk_flushContextData(w->root) < 0)
        ret = -1;
      if (mk_writeVoid(w->root, (256 - (ftell(w->fp) - w->segment_ptr))) < 0)
        ret = -1;
    }

    if (mk_flushContextData(w->root) < 0)
      ret = -1;

    if (!w->vlc_compat)
    {
      int i = w->seek_data.segmentinfo;
      w->seek_data.segmentinfo = 0;
      w->seek_data.tracks = 0;
      w->seek_data.cues = 0;
      w->seek_data.chapters = 0;
      w->seek_data.attachments = 0;
      w->seek_data.tags = 0;
      fseek(w->fp, w->segment_ptr, SEEK_SET);
      if (mk_writeSeek(w, NULL) < 0 ||
          mk_flushContextData(w->root) < 0)
        ret = -1;
      if (((i + w->segment_ptr) - ftell(w->fp) - 2) > 1)
        if (mk_writeVoid(w->root, (i + w->segment_ptr) - ftell(w->fp) - 2) < 0 ||
            mk_flushContextData(w->root) < 0)
          ret = -1;
    }

    fseek(w->fp, w->duration_ptr, SEEK_SET);
    if (mk_writeFloatRaw(w->root, (float)((double)(w->tracks_arr[0]->max_frame_tc+w->def_duration) / w->timescale)) < 0 ||
        mk_flushContextData(w->root) < 0)
      ret = -1;
  }

  mk_destroyContexts(w);
  fclose(w->fp);
  free(w->tracks_arr);
  free(w);

  return ret;
}
