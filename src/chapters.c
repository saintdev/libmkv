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
#include "config.h"
#include "libmkv.h"
#include "matroska.h"

int   mk_createChapterSimple(mk_Writer *w, uint64_t start, uint64_t end, char *name)
{
  mk_Context *ca, *cd;
  unsigned long	chapter_uid;

  /*
  * Generate a random UID for this Chapter.
  * NOTE: This probably should be a CRC32 of some unique chapter information.
  *		In place of being completely random.
  */
  chapter_uid = random();
  
  if (w->chapters == NULL)
  {
	unsigned long edition_uid;
	edition_uid = random();
	
    if ((w->chapters = mk_createContext(w, w->root, 0x1043a770)) == NULL) // Chapters
      return -1;
    if ((w->edition_entry = mk_createContext(w, w->chapters, 0x45b9)) == NULL) // EditionEntry
      return -1;
	CHECK(mk_writeUInt(w->edition_entry, 0x45bc, edition_uid));	/* EditionUID - See note above about Chapter UID. */
    CHECK(mk_writeUInt(w->edition_entry, 0x45db, 1)); // EditionFlagDefault - Force this to be the default.
    CHECK(mk_writeUInt(w->edition_entry, 0x45dd, 0)); // EditionFlagOrdered - Force simple chapters.
  }
  if ((ca = mk_createContext(w, w->edition_entry, 0xb6)) == NULL) // ChapterAtom
    return -1;
  CHECK(mk_writeUInt(ca, 0x73c4, chapter_uid));	/* ChapterUID */
  CHECK(mk_writeUInt(ca, 0x91, start)); // ChapterTimeStart
  if (end != start)                     // Just create a StartTime if chapter length would be 0.
    CHECK(mk_writeUInt(ca, 0x92, end)); // ChapterTimeEnd
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

  return 0;
}
