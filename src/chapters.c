/*****************************************************************************
 * chapters.c:
 *****************************************************************************
 * Copyright (C) 2007 libmkv
 * $Id: $
 *
 * Authors: Nathan Caldwell
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

int   mk_createChapterSimple(mk_Writer *w, unsigned start, unsigned end, char *name) {
  mk_Context *ca, *cd;

  if (w->chapters == NULL)
  {
//    w->seek_data->chapters = ftell(w->fp) - w->segment_ptr;   // FIXME: This won't work any more.
    if ((w->chapters = mk_createContext(w, w->root, 0x1043a770)) == NULL) // Chapters
      return -1;
    if ((w->edition_entry = mk_createContext(w, w->chapters, 0x45b0)) == NULL) // EditionEntry
      return -1;
    CHECK(mk_writeUInt(w->edition_entry, 0xb6, 0)); // EditionFlagOrdered - Force simple chapters.
  }
  if ((ca = mk_createContext(w, w->edition_entry, 0x45db)) == NULL) // ChapterAtom
    return -1;
  CHECK(mk_writeUInt(ca, 0x91, start)); // ChapterTimeStart
  CHECK(mk_writeUInt(ca, 0x91, end)); // ChapterTimeEnd
  if (name != NULL) {
    if ((cd = mk_createContext(w, ca, 0x80)) == NULL) // ChapterDisplay
      return -1;
    CHECK(mk_writeStr(cd, 0x85, name)); // ChapterDisplay
    CHECK(mk_closeContext(cd, 0));
  }
  CHECK(mk_closeContext(ca, 0));

  return 0;
}

int   mk_writeChapters(mk_Writer *w) {
  if ((w->chapters == NULL) || (w->edition_entry == NULL))
    return -1;
  CHECK(mk_closeContext(w->edition_entry, 0));
  CHECK(mk_closeContext(w->chapters, 0));
}
