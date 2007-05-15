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

#ifdef HAVE_STDINT_H
#include <stdint.h>
#else
#include <inttypes.h>
#endif

/* Video codecs */
#define MK_VCODEC_MPEG1    "V_MPEG1"
#define MK_VCODEC_MPEG2    "V_MPEG2"
#define MK_VCODEC_THEORA   "V_THEORA"
#define MK_VCODEC_SNOW     "V_SNOW"
#define MK_VCODEC_MP4ASP   "V_MPEG4/ISO/ASP"
#define MK_VCODEC_MP4AVC   "V_MPEG4/ISO/AVC"

/* Audio codecs */
#define MK_ACODEC_AC3      "A_AC3"
#define MK_ACODEC_MP3      "A_MPEG/L3"
#define MK_ACODEC_MP2      "A_MPEG/L2"
#define MK_ACODEC_MP1      "A_MPEG/L1"
#define MK_ACODEC_DTS      "A_DTS"
#define MK_ACODEC_PCMINTLE "A_PCM/INT/LIT"
#define MK_ACODEC_PCMFLTLE "A_PCM/FLOAT/IEEE"
#define MK_ACODEC_TTA1     "A_TTA1"
#define MK_ACODEC_WAVPACK  "A_WAVPACK4"
#define MK_ACODEC_VORBIS   "A_VORBIS"
#define MK_ACODEC_FLAC     "A_FLAC"
#define MK_ACODEC_AAC      "A_AAC"

/* Subtitles */
#define MK_SUBTITLE_ASCII  "S_TEXT/ASCII"
#define MK_SUBTITLE_UTF8   "S_TEXT/UTF8"
#define MK_SUBTITLE_SSA    "S_TEXT/SSA"
#define MK_SUBTITLE_ASS    "S_TEXT/ASS"
#define MK_SUBTITLE_USF    "S_TEXT/USF"
#define MK_SUBTITLE_VOBSUB "S_VOBSUB"

#define MK_TRACK_VIDEO     0x01
#define MK_TRACK_AUDIO     0x02
#define MK_TRACK_COMPLEX   0x03
#define MK_TRACK_LOGO      0x10
#define MK_TRACK_SUBTITLE  0x11
#define MK_TRACK_BUTTONS   0x12
#define MK_TRACK_CONTROL   0x20

#ifdef __cplusplus
extern "C" {
#endif

typedef struct mk_Writer_s mk_Writer;
typedef struct mk_Track_s mk_Track;
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
    mk_Track       *track;          // Handle for this track.
};

struct mk_VideoConfig_s {
    char    flagInterlaced;
    unsigned width;                 // Pixel width
    unsigned height;                // Pixel height
    unsigned d_width;               // Display width
    unsigned d_height;              // Display height
};

struct mk_AudioConfig_s {
    float   samplingFreq;           // Sampling Frequency in Hz
    unsigned    channels;           // Number of channels for this track
    unsigned    bitDepth;           // Bits per sample (PCM)
};

mk_Writer *mk_createWriter(const char *filename,
//                           int64_t default_frame_duration,
                           int64_t timescale);
mk_Track *mk_createTrack(mk_Writer *w, mk_TrackConfig *tc);
int   mk_writeHeader(mk_Writer *w, const char *writingApp);

int  mk_startFrame( mk_Writer *w, mk_Track *track );
int  mk_addFrameData(mk_Writer *w, mk_Track *track, const void *data, unsigned size);
int  mk_setFrameFlags(mk_Writer *w, mk_Track *track, int64_t timestamp, int keyframe);
int  mk_close( mk_Writer *w );

#ifdef __cplusplus
}
#endif

#endif
