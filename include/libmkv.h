/*****************************************************************************
 * matroska.h:
 *****************************************************************************
 * Copyright (C) 2005 x264 project
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

#ifndef _LIBMKV_H
#define _LIBMKV_H 1

/* Video codecs */
#define MKV_VCODEC_MPEG1    "V_MPEG1"
#define MKV_VCODEC_MPEG2    "V_MPEG2"
#define MKV_VCODEC_THEORA   "V_THEORA"
#define MKV_VCODEC_SNOW     "V_SNOW"
#define MKV_VCODEC_MP4ASP   "V_MPEG4/ISO/ASP"
#define MKV_VCODEC_MP4AVC   "V_MPEG4/ISO/AVC"

/* Audio codecs */
#define MKV_ACODEC_AC3      "A_AC3"
#define MKV_ACODEC_MP3      "A_MPEG/L3"
#define MKV_ACODEC_MP2      "A_MPEG/L2"
#define MKV_ACODEC_MP1      "A_MPEG/L1"
#define MKV_ACODEC_DTS      "A_DTS"
#define MKV_ACODEC_PCMINTLE "A_PCM/INT/LIT"
#define MKV_ACODEC_PCMFLTLE "A_PCM/FLOAT/IEEE"
#define MKV_ACODEC_TTA1     "A_TTA1"
#define MKV_ACODEC_WAVPACK  "A_WAVPACK4"
#define MKV_ACODEC_VORBIS   "A_VORBIS"
#define MKV_ACODEC_FLAC     "A_FLAC"
#define MKV_ACODEC_AAC      "A_AAC"

/* Subtitles */
#define MKV_SUBTITLE_ASCII  "S_TEXT/ASCII"
#define MKV_SUBTITLE_UTF8   "S_TEXT/UTF8"
#define MKV_SUBTITLE_SSA    "S_TEXT/SSA"
#define MKV_SUBTITLE_ASS    "S_TEXT/ASS"
#define MKV_SUBTITLE_USF    "S_TEXT/USF"
#define MKV_SUBTITLE_VOBSUB "S_VOBSUB"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct mk_Context_s mk_Context;
typedef struct mk_Writer_s mk_Writer;
typedef struct mk_TrackConfig_s mk_TrackConfig;
typedef struct mk_VideoConfig_s mk_VideoConfig;
typedef struct mk_AudioConfig_s mk_AudioConfig;

struct mk_TrackConfig_s {
    unsigned    trackUID;           // Optional: Unique identifier for the track.
    unsigned    trackType;          // Required: 1 = Video, 2 = Audio.
    char        flagEnabled;        // Optional: Set 1 if the track is used (Default: enabled)
    char        flagDefault;
    char        flagForced;         // Optional: Set 1 if the track MUST be shown during playback (Default: disabled)
    char        flagLacing;         // Required: Set 1 if the track may contain blocks using lacing.
    unsigned    minCache;           // Optional: See Matroska spec. (Default: cache disabled)
    unsigned    maxCache;
    int64_t     defaultDuration;    // Optional: Number of nanoseconds per frame.
    char        *name;
    char        *language;
    char        *codecID;           // Required: See codecs above.
    void        *codecPrivate;
    unsigned    codecPrivateSize;
    char        *codecName;
    mk_VideoConfig *video;
    mk_AudioConfig *audio;
};

struct mk_VideoConfig_s {
    char    flagInterlaced;
    unsigned width;
    unsigned height;
    unsigned d_width;
    unsigned d_height;
};

struct mk_AudioConfig_s {
    float   samplingFreq;
    unsigned    channels;
    unsigned    bitDepth;
};

mk_Writer *mk_createWriter( const char *filename );

int   mk_writeHeader(mk_Writer *w, const char *writingApp,
                     int64_t default_frame_duration,
                     int64_t timescale );

int  mk_addTrack(mk_Writer *w, mk_TrackConfig *tc);

int  mk_startFrame( mk_Writer *w, int trackID );
int  mk_addFrameData( mk_Writer *w, const void *data, unsigned size );
int  mk_setFrameFlags( mk_Writer *w, int64_t timestamp, int keyframe );
int  mk_close( mk_Writer *w );

#ifdef __cplusplus
}
#endif

#endif
