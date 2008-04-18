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

#define TRACK_STEP 4

mk_Track *mk_createTrack(mk_Writer *w, mk_TrackConfig *tc)
{
  mk_Context *ti, *v;
  int i;
  mk_Track *track = calloc(1, sizeof(*track));
  if (track == NULL)
    return NULL;

  if (w->num_tracks + 1 > w->alloc_tracks)
  {
    if ((w->tracks_arr = realloc(w->tracks_arr, (w->alloc_tracks + TRACK_STEP) * sizeof(mk_Track *))) == NULL)
      return NULL; // FIXME
    w->alloc_tracks += TRACK_STEP;
  }
  w->tracks_arr[w->num_tracks] = track;
  track->track_id = ++w->num_tracks;

  if (w->tracks == NULL)
  {
    if ((w->tracks = mk_createContext(w, w->root, MATROSKA_ID_TRACKS)) == NULL) // tracks
      return NULL;
  }

  if ((ti = mk_createContext(w, w->tracks, MATROSKA_ID_TRACKENTRY)) == NULL) // TrackEntry
    return NULL;
  if (mk_writeUInt(ti, MATROSKA_ID_TRACKNUMBER, track->track_id) < 0) // TrackNumber
    return NULL;
  if (tc->trackUID) {
    if (mk_writeUInt(ti, MATROSKA_ID_TRACKUID, tc->trackUID) < 0) /* TrackUID  */
      return NULL;
  } else {
	/*
	 * If we aren't given a UID, randomly generate one.
	 * NOTE: It would probably be better to CRC32 some unique track information
	 *		in place of something completely random.
	 */
	unsigned long track_uid;
	track_uid = random();

	if (mk_writeUInt(ti, MATROSKA_ID_TRACKUID, track_uid) < 0) /* TrackUID  */
      return NULL;
  }
  if (mk_writeUInt(ti, MATROSKA_ID_TRACKTYPE, tc->trackType) < 0) // TrackType
    return NULL;
  track->track_type = tc->trackType;
  if (mk_writeUInt(ti, MATROSKA_ID_TRACKFLAGLACING, tc->flagLacing) < 0) // FlagLacing
    return NULL;
  if (mk_writeStr(ti, MATROSKA_ID_CODECID, tc->codecID) < 0) // CodecID
    return NULL;
  if (tc->codecPrivateSize && (tc->codecPrivate != NULL))
    if (mk_writeBin(ti, MATROSKA_ID_CODECPRIVATE, tc->codecPrivate, tc->codecPrivateSize) < 0) // CodecPrivate
      return NULL;
  if (tc->defaultDuration) {
    if (mk_writeUInt(ti, MATROSKA_ID_TRACKDEFAULTDURATION, tc->defaultDuration) < 0)
      return NULL;
    track->default_duration = tc->defaultDuration;
  }
  if (tc->language)
    if (mk_writeStr(ti, MATROSKA_ID_TRACKLANGUAGE, tc->language) < 0)  // Language
      return NULL;
  if (tc->flagEnabled != 1)
    if (mk_writeUInt(ti, MATROSKA_ID_TRACKFLAGENABLED, tc->flagEnabled) < 0) // FlagEnabled
      return NULL;
  if (mk_writeUInt(ti, MATROSKA_ID_TRACKFLAGDEFAULT, tc->flagDefault) < 0) // FlagDefault
    return NULL;
  if (tc->flagForced)
    if (mk_writeUInt(ti, MATROSKA_ID_TRACKFLAGFORCED, tc->flagForced) < 0) // FlagForced
      return NULL;
  if (tc->minCache)
    if (mk_writeUInt(ti, MATROSKA_ID_TRACKMINCACHE, tc->minCache) < 0) // MinCache
      return NULL;
  /* FIXME: this won't handle NULL values, which signals that the cache is disabled. */
  if (tc->maxCache)
    if (mk_writeUInt(ti, MATROSKA_ID_TRACKMAXCACHE, tc->maxCache) < 0) // MaxCache
      return NULL;

  switch (tc->trackType)
  {
    case MK_TRACK_VIDEO:    // Video
      if ((v = mk_createContext(w, ti, MATROSKA_ID_TRACKVIDEO)) == NULL)
        return NULL;
      if (tc->extra.video.pixelCrop[0] != 0 || tc->extra.video.pixelCrop[1] != 0 || tc->extra.video.pixelCrop[2] != 0 || tc->extra.video.pixelCrop[3] != 0) {
        for (i = 0; i < 4; i++) {
          /* Each pixel crop ID is 0x11 away from the next.
           * In order from 0x54AA to 0x54DD they are bottom, top, left, right.
           */
          if (mk_writeUInt(v, MATROSKA_ID_VIDEOPIXELCROPBOTTOM + (i * 0x11), tc->extra.video.pixelCrop[i]) < 0) // PixelCrop
            return NULL;
        }
      }
      if (mk_writeUInt(v, MATROSKA_ID_VIDEOPIXELWIDTH, tc->extra.video.pixelWidth) < 0) // PixelWidth
        return NULL;
      if (mk_writeUInt(v, MATROSKA_ID_VIDEOPIXELHEIGHT, tc->extra.video.pixelHeight) < 0 ) // PixelHeight
        return NULL;
      if (mk_writeUInt(v, MATROSKA_ID_VIDEODISPLAYWIDTH, tc->extra.video.displayWidth) < 0) // DisplayWidth
        return NULL;
      if (mk_writeUInt(v, MATROSKA_ID_VIDEODISPLAYHEIGHT, tc->extra.video.displayHeight) < 0) // DisplayHeight
        return NULL;
      if (tc->extra.video.displayUnit)
        if (mk_writeUInt(v, MATROSKA_ID_VIDEODISPLAYUNIT, tc->extra.video.displayUnit) < 0) // DisplayUnit
          return NULL;
      break;
    case MK_TRACK_AUDIO:    // Audio
      if ((v = mk_createContext(w, ti, MATROSKA_ID_TRACKAUDIO)) == NULL)
        return NULL;
      if (mk_writeFloat(v, MATROSKA_ID_AUDIOSAMPLINGFREQ, tc->extra.audio.samplingFreq) < 0) // SamplingFrequency
        return NULL;
      if (mk_writeUInt(v, MATROSKA_ID_AUDIOCHANNELS, tc->extra.audio.channels) < 0) // Channels
        return NULL;
      if (tc->extra.audio.bitDepth)
        if (mk_writeUInt(v, MATROSKA_ID_AUDIOBITDEPTH, tc->extra.audio.bitDepth) < 0) // BitDepth
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
