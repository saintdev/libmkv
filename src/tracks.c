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
#include "config.h"
#include "libmkv.h"
#include "matroska.h"

/* TODO: Figure out what can actually fail without damaging the track. */

mk_Track *mk_createTrack(mk_Writer *w, mk_TrackConfig *tc)
{
  mk_Context *ti, *v;
  int i;
  mk_Track *track = calloc(1, sizeof(*track));
  if (track == NULL)
    return NULL;

  if ((w->tracks_arr = realloc(w->tracks_arr, (w->num_tracks + 1) * sizeof(mk_Track *))) == NULL)
    return NULL; // FIXME
  w->tracks_arr[w->num_tracks] = track;
  track->track_id = ++w->num_tracks;

  if (w->tracks == NULL)
  {
    if ((w->tracks = mk_createContext(w, w->root, 0x1654ae6b)) == NULL) // tracks
      return NULL;
  }

  if ((ti = mk_createContext(w, w->tracks, 0xae)) == NULL) // TrackEntry
    return NULL;
  if (mk_writeUInt(ti, 0xd7, track->track_id) < 0) // TrackNumber
    return NULL;
  if (tc->trackUID) {
    if (mk_writeUInt(ti, 0x73c5, tc->trackUID) < 0) /* TrackUID  */
      return NULL;
  } else {
	/*
	 * If we aren't given a UID, randomly generate one.
	 * NOTE: It would probably be better to CRC32 some unique track information
	 *		in place of something completely random.
	 */
	unsigned long track_uid;
	track_uid = random();
	
	if (mk_writeUInt(ti, 0x73c5, track_uid) < 0) /* TrackUID  */
      return NULL;
  }
  if (mk_writeUInt(ti, 0x83, tc->trackType) < 0) // TrackType
    return NULL;
  track->track_type = tc->trackType;
  if (mk_writeUInt(ti, 0x9c, tc->flagLacing) < 0) // FlagLacing
    return NULL;
  if (mk_writeStr(ti, 0x86, tc->codecID) < 0) // CodecID
    return NULL;
  if (tc->codecPrivateSize && (tc->codecPrivate != NULL))
    if (mk_writeBin(ti, 0x63a2, tc->codecPrivate, tc->codecPrivateSize) < 0) // CodecPrivate
      return NULL;
  if (tc->defaultDuration) {
    if (mk_writeUInt(ti, 0x23e383, tc->defaultDuration) < 0)
      return NULL;
    track->default_duration = tc->defaultDuration;
  }
  if (tc->language)
    if (mk_writeStr(ti, 0x22b59c, tc->language) < 0)  // Language
      return NULL;
  if (tc->flagEnabled != 1)
    if (mk_writeUInt(ti, 0xb9, tc->flagEnabled) < 0) // FlagEnabled
      return NULL;
  if (mk_writeUInt(ti, 0x88, tc->flagDefault) < 0) // FlagDefault
    return NULL;
  if (tc->flagForced)
    if (mk_writeUInt(ti, 0x55aa, tc->flagForced) < 0) // FlagForced
      return NULL;
  if (tc->minCache)
    if (mk_writeUInt(ti, 0x6de7, tc->minCache) < 0) // MinCache
      return NULL;
  /* FIXME: this won't handle NULL values, which signals that the cache is disabled. */
  if (tc->maxCache)
    if (mk_writeUInt(ti, 0x6df8, tc->maxCache) < 0) // MaxCache
      return NULL;

  switch (tc->trackType)
  {
    case MK_TRACK_VIDEO:    // Video
      if ((v = mk_createContext(w, ti, 0xe0)) == NULL)
        return NULL;
      if (tc->extra.video.pixelCrop[0] != 0 || tc->extra.video.pixelCrop[1] != 0 || tc->extra.video.pixelCrop[2] != 0 || tc->extra.video.pixelCrop[3] != 0) {
        for (i = 0; i < 4; i++) {
          if (mk_writeUInt(v, 0x54aa + (i * 0x11), tc->extra.video.pixelCrop[i]) < 0) // PixelCrop
            return NULL;
        }
      }
      if (mk_writeUInt(v, 0xb0, tc->extra.video.pixelWidth) < 0) // PixelWidth
        return NULL;
      if (mk_writeUInt(v, 0xba, tc->extra.video.pixelHeight) < 0 ) // PixelHeight
        return NULL;
      if (mk_writeUInt(v, 0x54b0, tc->extra.video.displayWidth) < 0) // DisplayWidth
        return NULL;
      if (mk_writeUInt(v, 0x54ba, tc->extra.video.displayHeight) < 0) // DisplayHeight
        return NULL;
      if (tc->extra.video.displayUnit)
        if (mk_writeUInt(v, 0x54b2, tc->extra.video.displayUnit) < 0) // DisplayUnit
          return NULL;
      break;
    case MK_TRACK_AUDIO:    // Audio
      if ((v = mk_createContext(w, ti, 0xe1)) == NULL)
        return NULL;
      if (mk_writeFloat(v, 0xb5, tc->extra.audio.samplingFreq) < 0) // SamplingFrequency
        return NULL;
      if (mk_writeUInt(v, 0x9f, tc->extra.audio.channels) < 0) // Channels
        return NULL;
      if (tc->extra.audio.bitDepth)
        if (mk_writeUInt(v, 0x6264, tc->extra.audio.bitDepth) < 0) // BitDepth
          return NULL;
      break;
    default:                // Other TODO: Implement other track types.
      return NULL;
  }

  if (mk_closeContext(v, 0) < 0)
    return NULL;
  if (mk_closeContext(ti, 0) < 0)
    return NULL;

  return track;
}

int   mk_writeTracks(mk_Writer *w, mk_Context *tracks)
{
  w->seek_data.tracks = w->root->d_cur;

  CHECK(mk_closeContext(w->tracks, 0));

  return 0;
}
