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
  mk_Track *track = calloc(1, sizeof(*track));
  if (track == NULL)
    return NULL;
  track->config = calloc(1, sizeof(mk_TrackConfig));
  if (track->config == NULL)
    return NULL;
  memcpy(track->config, tc, sizeof(mk_TrackConfig));
  track->config->name = strdup(tc->name);
  track->config->language = strdup(tc->language);
  track->config->codecID = strdup(tc->codecID);
  track->config->codecName = strdup(tc->codecName);
  
  if(tc->video)
  {
    track->config->video = calloc(1, sizeof(mk_VideoConfig));
    if (track->config->video == NULL)
      return NULL;
    memcpy(track->config->video, tc->video, sizeof(mk_VideoConfig));
  }
  else if (tc->audio)
  {
    track->config->audio = calloc(1, sizeof(mk_AudioConfig));
    if (track->config->audio == NULL)
      return NULL;
    memcpy(track->config->audio, tc->audio, sizeof(mk_AudioConfig));
  }
  if (tc->codecPrivate && (tc->codecPrivateSize > 0))
  {
    track->config->codecPrivate = calloc(1, tc->codecPrivateSize);
    if (track->config->codecPrivate == NULL)
      return NULL;
    memcpy(track->config->codecPrivate, tc->codecPrivate, tc->codecPrivateSize);
    track->config->codecPrivateSize = tc->codecPrivateSize;
  }

  w->tracks = realloc( w->tracks, (w->num_tracks + 1) * sizeof(mk_Track *));
  w->tracks[w->num_tracks] = track;
  track->track_id = w->num_tracks + 1;
  w->num_tracks++;

  return track;
}

int   mk_writeTrack(mk_Writer *w, mk_Context *tracks, mk_Track *t)
{
  mk_Context  *ti, *v;
  int i;

  if ((ti = mk_createContext(w, tracks, 0xae)) == NULL) // TrackEntry
    return -1;
  CHECK(mk_writeUInt(ti, 0xd7, t->track_id)); // TrackNumber
  if (t->config->trackUID)
    CHECK(mk_writeUInt(ti, 0x73c5, t->config->trackUID)); // TrackUID
  else
    CHECK(mk_writeUInt(ti, 0x73c5, t->track_id));
  CHECK(mk_writeUInt(ti, 0x83, t->config->trackType)); // TrackType
  CHECK(mk_writeUInt(ti, 0x9c, t->config->flagLacing)); // FlagLacing
  CHECK(mk_writeStr(ti, 0x86, t->config->codecID)); // CodecID
  if (t->config->codecPrivateSize)
    CHECK(mk_writeBin(ti, 0x63a2, t->config->codecPrivate, t->config->codecPrivateSize)); // CodecPrivate
//    if (w->def_duration)
//        CHECK(mk_writeUInt(ti, 0x23e383, w->def_duration)); // DefaultDuration
  if (t->config->defaultDuration)
    CHECK(mk_writeUInt(ti, 0x23e383, t->config->defaultDuration));
  if (t->config->language)
    CHECK(mk_writeStr(ti, 0x22b59c, t->config->language));  // Language
  CHECK(mk_writeUInt(ti, 0xb9, t->config->flagEnabled)); // FlagEnabled
  CHECK(mk_writeUInt(ti, 0xbb, t->config->flagDefault)); // FlagDefault
  if (t->config->flagForced)
    CHECK(mk_writeUInt(ti, 0x55aa, t->config->flagForced)); // FlagForced
  if (t->config->minCache)
    CHECK(mk_writeUInt(ti, 0x6de7, t->config->minCache)); // MinCache
  /* FIXME: this won't handle NULL values, which signals that the cache is disabled. */
  if (t->config->maxCache)
    CHECK(mk_writeUInt(ti, 0x6df8, t->config->maxCache)); // MaxCache
  switch (t->config->trackType)
  {
    case MK_TRACK_VIDEO:    // Video
      if ((v = mk_createContext(w, ti, 0xe0)) == NULL)
        return -1;
      if (t->config->video->pixelCrop[0] != 0 || t->config->video->pixelCrop[1] != 0 || t->config->video->pixelCrop[2] != 0 || t->config->video->pixelCrop[3] != 0) {
        for (i = 0; i < 4; i++) {
          CHECK(mk_writeUInt(v, 0x54aa + (i * 0x11), t->config->video->pixelCrop[i])); // PixelCrop
        }
      }
      CHECK(mk_writeUInt(v, 0xb0, t->config->video->pixelWidth)); // PixelWidth
      CHECK(mk_writeUInt(v, 0xba, t->config->video->pixelHeight)); // PixelHeight
      CHECK(mk_writeUInt(v, 0x54b0, t->config->video->displayWidth)); // DisplayWidth
      CHECK(mk_writeUInt(v, 0x54ba, t->config->video->displayHeight)); // DisplayHeight
      if (t->config->video->displayUnit)
        CHECK(mk_writeUInt(v, 0x54b2, t->config->video->displayUnit)); // DisplayUnit
      break;
    case MK_TRACK_AUDIO:    // Audio
      if ((v = mk_createContext(w, ti, 0xe1)) == NULL)
        return -1;
      CHECK(mk_writeFloat(v, 0xb5, t->config->audio->samplingFreq)); // SamplingFrequency
      CHECK(mk_writeUInt(v, 0x9f, t->config->audio->channels)); // Channels
      if (t->config->audio->bitDepth)
        CHECK(mk_writeUInt(v, 0x6264, t->config->audio->bitDepth)); // BitDepth
      break;
    default:                // Other
      return -1;
  }
  
  CHECK(mk_closeContext(v, 0));
  CHECK(mk_closeContext(ti, 0));

  return 0;
}

void mk_destroyTrack(mk_Track *track) {
  if (strlen(track->config->codecID) > 0)
    free(track->config->codecID);
  if (strlen(track->config->name) > 0)
    free(track->config->name);
  if (strlen(track->config->language) > 0)
    free(track->config->language);
  if (strlen(track->config->codecName) > 0)
    free(track->config->codecName);
  if (track->config->video)
    free(track->config->video);
  if (track->config->audio)
    free(track->config->audio);
  if (track->config->codecPrivate != NULL && track->config->codecPrivateSize > 0)
    free(track->config->codecPrivate);

  free(track);
}
