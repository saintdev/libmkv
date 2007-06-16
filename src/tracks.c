/*****************************************************************************
 * tracks.c:
 *****************************************************************************
 * Copyright (C) 2007 libmkv
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

mk_Track *mk_createTrack(mk_Writer *w, mk_TrackConfig *tc)
{
  mk_Context *ti, *v;
  int i;
  mk_Track *track = calloc(1, sizeof(*track));
  if (track == NULL)
    return NULL;

  w->tracks_arr = realloc(w->tracks_arr, (w->num_tracks + 1) * sizeof(mk_Track *)); // FIXME: Check return!
  w->tracks_arr[w->num_tracks] = track;
  track->track_id = ++w->num_tracks;

  if (w->tracks == NULL)
  {
    if ((w->tracks = mk_createContext(w, w->root, 0x1654ae6b)) == NULL) // tracks
      return -1;
  }

  if ((ti = mk_createContext(w, w->tracks, 0xae)) == NULL) // TrackEntry
    return -1;
  CHECK(mk_writeUInt(ti, 0xd7, track->track_id)); // TrackNumber
  if (tc->trackUID)
    CHECK(mk_writeUInt(ti, 0x73c5, tc->trackUID)); // TrackUID
  else
    CHECK(mk_writeUInt(ti, 0x73c5, track->track_id));
  CHECK(mk_writeUInt(ti, 0x83, tc->trackType)); // TrackType
  CHECK(mk_writeUInt(ti, 0x9c, tc->flagLacing)); // FlagLacing
  CHECK(mk_writeStr(ti, 0x86, tc->codecID)); // CodecID
  if (tc->codecPrivateSize && (tc->codecPrivate != NULL))
    CHECK(mk_writeBin(ti, 0x63a2, tc->codecPrivate, tc->codecPrivateSize)); // CodecPrivate
  if (tc->defaultDuration) {
    CHECK(mk_writeUInt(ti, 0x23e383, tc->defaultDuration));
    track->default_duration = tc->defaultDuration;
  }
  if (tc->language)
    CHECK(mk_writeStr(ti, 0x22b59c, tc->language));  // Language
  CHECK(mk_writeUInt(ti, 0xb9, tc->flagEnabled)); // FlagEnabled
  CHECK(mk_writeUInt(ti, 0xbb, tc->flagDefault)); // FlagDefault
  if (tc->flagForced)
    CHECK(mk_writeUInt(ti, 0x55aa, tc->flagForced)); // FlagForced
  if (tc->minCache)
    CHECK(mk_writeUInt(ti, 0x6de7, tc->minCache)); // MinCache
  /* FIXME: this won't handle NULL values, which signals that the cache is disabled. */
  if (tc->maxCache)
    CHECK(mk_writeUInt(ti, 0x6df8, tc->maxCache)); // MaxCache

  switch (tc->trackType)
  {
    case MK_TRACK_VIDEO:    // Video
      if ((v = mk_createContext(w, ti, 0xe0)) == NULL)
        return -1;
      if (tc->video.pixelCrop[0] != 0 || tc->video.pixelCrop[1] != 0 || tc->video.pixelCrop[2] != 0 || tc->video.pixelCrop[3] != 0) {
        for (i = 0; i < 4; i++) {
          CHECK(mk_writeUInt(v, 0x54aa + (i * 0x11), tc->video.pixelCrop[i])); // PixelCrop
        }
      }
      CHECK(mk_writeUInt(v, 0xb0, tc->video.pixelWidth)); // PixelWidth
      CHECK(mk_writeUInt(v, 0xba, tc->video.pixelHeight)); // PixelHeight
      CHECK(mk_writeUInt(v, 0x54b0, tc->video.displayWidth)); // DisplayWidth
      CHECK(mk_writeUInt(v, 0x54ba, tc->video.displayHeight)); // DisplayHeight
      if (tc->video.displayUnit)
        CHECK(mk_writeUInt(v, 0x54b2, tc->video.displayUnit)); // DisplayUnit
      break;
    case MK_TRACK_AUDIO:    // Audio
      if ((v = mk_createContext(w, ti, 0xe1)) == NULL)
        return -1;
      CHECK(mk_writeFloat(v, 0xb5, tc->audio.samplingFreq)); // SamplingFrequency
      CHECK(mk_writeUInt(v, 0x9f, tc->audio.channels)); // Channels
      if (tc->audio.bitDepth)
        CHECK(mk_writeUInt(v, 0x6264, tc->audio.bitDepth)); // BitDepth
      break;
    default:                // Other
      return -1;
  }

  CHECK(mk_closeContext(v, 0));
  CHECK(mk_closeContext(ti, 0));

  return track;
}

int   mk_writeTracks(mk_Writer *w, mk_Context *tracks)
{
  w->seek_data.tracks = w->root->d_cur;

  CHECK(mk_closeContext(w->tracks, 0));

  return 0;
}
