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

int mk_seekFile(mk_Writer *w, uint64_t pos) {
  if (fseek(w->fp, pos, SEEK_SET))
    return -1;

  w->f_pos = pos;

  if (pos > w->f_eof)
    w->f_eof = pos;

  return 0;
}

char  *mk_laceXiph(uint64_t *sizes, uint8_t num_frames, uint64_t *output_size) {
  unsigned i, j;
  uint64_t offset = 0;
  uint64_t alloc_size = num_frames * 6;  // Complete guess. We'll realloc if we need more space, though.
  char *laced = calloc(alloc_size, sizeof(char));
  if (laced == NULL)
    return NULL;
  
  laced[offset++] = num_frames;
  for (i = 0; i < num_frames; i++)
  {
    for (j = sizes[i]; j >= 255 ; j -= 255)
    {
      laced[offset++] = 255;
      if (offset + 1 >= alloc_size) {
        int avg_sz = offset / (i - 1);  // Compute approximate average bytes/frame
        alloc_size += avg_sz * (num_frames - i);  // Add our average + number of frames left to size
        if ((laced = realloc(laced, alloc_size)) == NULL)
          return NULL;
      }
    }
    laced[offset++] = j;
  }

  if (output_size != NULL)
    *output_size = offset;

  return laced;
}

mk_Writer *mk_createWriter(const char *filename, int64_t timescale, uint8_t vlc_compat) {
  mk_Writer *w = calloc(1, sizeof(*w));
  if (w == NULL)
    return NULL;

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

  if (vlc_compat) {
    if ((w->cluster.seekhead = mk_createContext(w, w->root, 0x114d9b74)) == NULL) // SeekHead
    {
      mk_destroyContexts(w);
      free(w);
      return NULL;
    }
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

  if (w->vlc_compat) {
    CHECK(mk_writeVoid(w->root, 0x100));  // 256 bytes should be enough room for our Seek entries.
    CHECK(mk_writeVoid(w->root, 0x800)); // 2048 bytes for Chapters.
  }
  else {
    w->seek_data.seekhead = 0x80000000;
    CHECK(mk_writeSeekHead(w, &w->seekhead_ptr));
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
  
  if (w->tracks)
    CHECK(mk_closeContext(w->tracks, 0));

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
  mk_Context *c, *tp;
  int64_t   delta, ref = 0;
  unsigned  fsize, bgsize;
  uint8_t   flags, c_delta[2];
  int i;
  char *laced = NULL;
  uint64_t  length = 0;

  if (!track->in_frame)
    return 0;

  delta = track->frame.timecode/w->timescale - w->cluster.tc_scaled;
  if (delta > 2000ll || delta < -2000ll)
    CHECK(mk_closeCluster(w));

  if (w->cluster.context == NULL) {
    w->cluster.tc_scaled = track->frame.timecode / w->timescale;
    w->cluster.context = mk_createContext(w, w->root, 0x1f43b675); // Cluster
    if (w->cluster.context == NULL)
      return -1;

    w->cluster.pointer = w->f_pos - w->segment_ptr;

    if (w->vlc_compat)
      CHECK(mk_writeSeek(w, w->cluster.seekhead, 0x1f43b675, w->cluster.pointer));

    CHECK(mk_writeUInt(w->cluster.context, 0xe7, w->cluster.tc_scaled)); // Cluster Timecode

    delta = 0;
    w->cluster.block_count = 0;
  }

  /* Calculate the encoded lacing sizes. */
  switch (track->frame.lacing) {
	  case MK_LACING_XIPH:
		  laced = mk_laceXiph(track->frame.lacing_sizes, track->frame.lacing_num_frames, &length);
		  break;
	  case MK_LACING_EBML:
	  {
		  uint64_t u_size = 0;
		  length += mk_ebmlSizeSize(track->frame.lacing_sizes[0]) + 1;	// Add one for the frame count.
		  for (i = 1; i < track->frame.lacing_num_frames; i++)
		  {
			  u_size = llabs(track->frame.lacing_sizes[i] - track->frame.lacing_sizes[i-1]);
			  length += mk_ebmlSizeSize((u_size) << 1);		// Shift by one so we get the right size for a signed number.
		  }
		  break;
	  }
	  case MK_LACING_FIXED:
	  {
		  laced = calloc(1, sizeof(char));
		  laced[0] = track->frame.lacing_num_frames;
		  ++length;
		  break;
	  }
  }
  
  fsize = track->frame.data ? track->frame.data->d_cur : 0;
  bgsize = fsize + 4 + mk_ebmlSizeSize(fsize + 4 + length) + 1 + length;
  if (!track->frame.keyframe) {
    ref = track->prev_frame_tc_scaled - w->cluster.tc_scaled - delta;
    bgsize += 1 + 1 + mk_ebmlSIntSize(ref);
  }

  CHECK(mk_writeID(w->cluster.context, 0xa0)); // BlockGroup
  CHECK(mk_writeSize(w->cluster.context, bgsize));
  CHECK(mk_writeID(w->cluster.context, 0xa1)); // Block
  CHECK(mk_writeSize(w->cluster.context, fsize + 4 + length));	// Block size
  CHECK(mk_writeSize(w->cluster.context, track->track_id)); // track number

  w->cluster.block_count++;

  c_delta[0] = delta >> 8;
  c_delta[1] = delta;
  CHECK(mk_appendContextData(w->cluster.context, c_delta, 2));	// Timecode relative to Cluster.

//   flags = ( track->frame.keyframe << 8 ) | track->frame.lacing;
  flags = track->frame.lacing << 1;	// Flags: Bit 5-6 describe what type of lacing to use.
  CHECK(mk_appendContextData(w->cluster.context, &flags, 1));
  if (track->frame.lacing) {
    if (track->frame.lacing == MK_LACING_EBML) {
      CHECK(mk_appendContextData(w->cluster.context, &track->frame.lacing_num_frames, 1));	// Number of frames in lace-1
      CHECK(mk_writeSize(w->cluster.context, track->frame.lacing_sizes[0]));	// Size of 1st frame.
      for (i = 1; i < track->frame.lacing_num_frames; i++)
      {
        CHECK(mk_writeSSize(w->cluster.context, track->frame.lacing_sizes[i] - track->frame.lacing_sizes[i-1]));	// Size difference between previous size and this size.
      }
    } else if (length > 0 && laced != NULL) {
      CHECK(mk_appendContextData(w->cluster.context, laced, length));
      free(laced);
      laced = NULL;
    }
  }

  if (track->frame.data) {
    CHECK(mk_appendContextData(w->cluster.context, track->frame.data->data, track->frame.data->d_cur));
    track->frame.data->d_cur = 0;
  }
  if (!track->frame.keyframe)
    CHECK(mk_writeSInt(w->cluster.context, 0xfb, ref)); // ReferenceBlock

  if (track->frame.keyframe && (track->track_type & MK_TRACK_VIDEO) && ((track->prev_cue_pos + 3*CLSIZE) <= w->f_pos || track->frame.timecode == 0)) {
    if ((c = mk_createContext(w, w->cues, 0xbb)) == NULL)  // CuePoint
      return -1;
    CHECK(mk_writeUInt(c, 0xb3, track->frame.timecode)); // CueTime

    if ((tp = mk_createContext(w, c, 0xb7)) == NULL)  // CueTrackPositions
      return -1;
    CHECK(mk_writeUInt(tp, 0xf7, track->track_id)); // CueTrack
    CHECK(mk_writeUInt(tp, 0xf1, w->cluster.pointer));  // CueClusterPosition
//     CHECK(mk_writeUInt(c, 0x5378, w->cluster.block_count));  // CueBlockNumber
    CHECK(mk_closeContext(tp, 0));
    CHECK(mk_closeContext(c, 0));
    track->prev_cue_pos = w->f_pos;
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
  track->frame.keyframe = 0;
  track->frame.lacing = MK_LACING_NONE;
  track->frame.lacing_num_frames = 0;
  track->frame.lacing_sizes = NULL;

  return 0;
}

int   mk_setFrameFlags(mk_Writer *w, mk_Track *track, int64_t timestamp, unsigned keyframe) {
  if (!track->in_frame)
    return -1;

  track->frame.timecode = timestamp;
  track->frame.keyframe = keyframe != 0;

  if (track->max_frame_tc < timestamp)
    track->max_frame_tc = timestamp;

  return 0;
}

int   mk_setFrameLacing(mk_Writer *w, mk_Track *track, uint8_t lacing, uint8_t num_frames, uint64_t sizes[]) {
  if (!track->in_frame)
    return -1;
  track->frame.lacing_sizes = calloc(num_frames, sizeof(uint64_t));

  track->frame.lacing = lacing;
  track->frame.lacing_num_frames = num_frames;
  memcpy(track->frame.lacing_sizes, sizes, num_frames * sizeof(uint64_t));

  return 0;
}

int   mk_addFrameData(mk_Writer *w, mk_Track *track, const void *data, unsigned size) {
  if (!track->in_frame)
    return -1;

  if (track->frame.data == NULL)
    if ((track->frame.data = mk_createContext(w, NULL, 0)) == NULL)
      return -1;

  return mk_appendContextData(track->frame.data, data, size);
}

int   mk_writeSeek(mk_Writer *w, mk_Context *c, unsigned seek_id, uint64_t seek_pos) {
  mk_Context  *s;

  if ((s = mk_createContext(w, c, 0x4dbb)) == NULL) // Seek
    return -1;
  CHECK(mk_writeUInt(s, 0x53ab, seek_id));  // SeekID
  CHECK(mk_writeUInt(s, 0x53ac, seek_pos)); // SeekPosition
  CHECK(mk_closeContext(s, 0));

  return 0;
}

/* The offset of the SeekHead is returned in *pointer. */
int mk_writeSeekHead(mk_Writer *w, int64_t *pointer) {
  mk_Context  *c;
  int64_t   seekhead_ptr;

  if ((c = mk_createContext(w, w->root, 0x114d9b74)) == NULL) // SeekHead
    return -1;
  if (pointer != NULL)
    seekhead_ptr = w->f_pos;
  if (w->seek_data.seekhead)
    CHECK(mk_writeSeek(w, c, 0x114d9b74, w->seek_data.seekhead));
  if (w->seek_data.segmentinfo)
    CHECK(mk_writeSeek(w, c, 0x1549a966, w->seek_data.segmentinfo));
  if (w->seek_data.tracks)
    CHECK(mk_writeSeek(w, c, 0x1654ae6b, w->seek_data.tracks));
  if (w->seek_data.cues)
    CHECK(mk_writeSeek(w, c, 0x1c53bb6b, w->seek_data.cues));
  if (w->seek_data.attachments)
    CHECK(mk_writeSeek(w, c, 0x1941a469, w->seek_data.attachments));
  if (w->seek_data.chapters)
    CHECK(mk_writeSeek(w, c, 0x1043a770, w->seek_data.chapters));
  if (w->seek_data.tags)
    CHECK(mk_writeSeek(w, c, 0x1254c367, w->seek_data.tags));
  CHECK(mk_closeContext(c, 0));

  if (pointer != NULL)
    *pointer = seekhead_ptr;

  return 0;
}

int   mk_close(mk_Writer *w) {
  int   i, ret = 0;
  mk_Track *tk;
  int64_t max_frame_tc = w->tracks_arr[0]->max_frame_tc;
  uint64_t segment_size = 0;
  unsigned char c_size[8];

  for (i = w->num_tracks - 1; i >= 0; i--)
  {
    tk = w->tracks_arr[i];
    w->tracks_arr[i] = NULL;
    --w->num_tracks;
    if (mk_flushFrame(w, tk) < 0)
      ret = -1;
    free(tk);
    tk = NULL;
  }

  if (mk_closeCluster(w) < 0)
    ret = -1;

  w->seek_data.cues = w->f_pos - w->segment_ptr;
  if (mk_closeContext(w->cues, 0) < 0)
    ret = -1;
  if (mk_flushContextData(w->root) < 0)
    ret = -1;

  if (w->vlc_compat && w->cluster.seekhead) {
    w->seek_data.seekhead = w->f_pos - w->segment_ptr;
    if (mk_closeContext(w->cluster.seekhead, 0) < 0)
      ret = -1;
    if (mk_flushContextData(w->root) < 0)
      ret = -1;
  }
  
  if (w->chapters != NULL)
  {
    if (w->vlc_compat) {
      if (mk_flushContextData(w->root) < 0)
        ret = -1;
      if (mk_seekFile(w, w->segment_ptr + 0x100 + 3) < 0)
        ret = -1;
    }
    w->seek_data.chapters = w->f_pos - w->segment_ptr;
    mk_writeChapters(w);
    if (mk_flushContextData(w->root) < 0)
      ret = -1;
    if (w->vlc_compat) {
      if (mk_writeVoid(w->root, (0x800 - (w->f_pos - w->segment_ptr - 0x100 - 3))) < 0)
        ret = -1;
      if (mk_flushContextData(w->root) < 0)
        ret = -1;
    }
  }

  if (w->wrote_header) {
    if (w->vlc_compat) {
      if (mk_seekFile(w, w->segment_ptr) < 0)
        ret = -1;
    }

    if (mk_writeSeekHead(w, &w->seek_data.seekhead) < 0)
      ret = -1;
    w->seek_data.seekhead -= w->segment_ptr;

    if (w->vlc_compat)
    {
      if (mk_flushContextData(w->root) < 0)
        ret = -1;
      if (mk_writeVoid(w->root, (0x100 - (w->f_pos - w->segment_ptr))) < 0)
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
      if (mk_seekFile(w, w->segment_ptr) < 0)
        ret = -1;
      if (mk_writeSeekHead(w, NULL) < 0 ||
          mk_flushContextData(w->root) < 0)
        ret = -1;
      if (((i + w->segment_ptr) - w->f_pos - 2) > 1)
        if (mk_writeVoid(w->root, (i + w->segment_ptr) - w->f_pos - 2) < 0 ||
            mk_flushContextData(w->root) < 0)
          ret = -1;
    }

    if (mk_seekFile(w, w->duration_ptr) < 0)
      ret = -1;
    if (mk_writeFloatRaw(w->root, (float)((double)(max_frame_tc+w->def_duration) / w->timescale)) < 0 ||
        mk_flushContextData(w->root) < 0)
      ret = -1;
    if (mk_seekFile(w, w->segment_ptr - 8) < 0)
      ret = -1;
    segment_size = w->f_eof - w->segment_ptr;
    for (i = 7; i > 0; --i)
      c_size[i] = segment_size >> (8 * (7-i));
    c_size[i] = 0x01;
    if (mk_appendContextData(w->root, &c_size, 8) < 0 ||
        mk_flushContextData(w->root) < 0)
      ret = -1;
  }

  if (mk_closeContext(w->root, 0) < 0)
    ret = -1;
  mk_destroyContexts(w);
  fclose(w->fp);
  free(w->tracks_arr);
  free(w);

  return ret;
}
