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
  mk_Chapter *chapter = calloc(1, sizeof(*chapter));
  if (chapter == NULL)
    return -1;

  chapter->timeStart = start;
  chapter->timeEnd = end;
  chapter->chapterDisplay = strdup(name);
  chapter->next = NULL;
  if (w->chapters == NULL) {
    chapter->prev = NULL;
    chapter->tail = malloc(sizeof(mk_Chapter *));
    w->chapters = chapter;
  }
  else {
    chapter->prev = *(w->chapters->tail);
    chapter->tail = w->chapters->tail;
    (*chapter->tail)->next = chapter;
  }
  *chapter->tail = chapter;


  return 0;
}

int   mk_writeChapters(mk_Writer *w) {
  mk_Chapter *chapter;
  mk_Context *c, *e;
  
  if (w->chapters == NULL)
    return -1;
  w->seek_data->chapters = ftell(w->fp) - w->segment_ptr;
  chapter = w->chapters;
  if ((c = mk_createContext(w, w->root, 0x1043a770)) == NULL) // Chapters
    return -1;
  if ((e = mk_createContext(w, c, 0x45b0)) == NULL) // EditionEntry
    return -1;
  CHECK(mk_writeUInt(e, 0xb6, 0)); // EditionFlagOrdered - Force simple chapters for now.
  while (chapter != NULL) {
    mk_writeChapter(w, e, chapter); // FIXME: check return value!
    chapter = chapter->next;
  }
  CHECK(mk_closeContext(e, 0));
  CHECK(mk_closeContext(c, 0));
  mk_flushContextData(w->root);         // FIXME: check return value!
}

int   mk_writeChapter(mk_Writer *w, mk_Context *c, mk_Chapter *chapter) {
  mk_Context *ca, *cd;
  
  if ((ca = mk_createContext(w, c, 0x45db)) == NULL) // ChapterAtom
    return -1;
  CHECK(mk_writeUInt(ca, 0x91, chapter->timeStart)); // ChapterTimeStart
  CHECK(mk_writeUInt(ca, 0x92, chapter->timeEnd)); // ChapterTimeEnd
  if (chapter->chapterDisplay != NULL) {
    if ((cd = mk_createContext(w, ca, 0x80)) == NULL) // ChapterDisplay
      return -1;
    CHECK(mk_writeStr(cd, 0x85, chapter->chapterDisplay)); // ChapString
    CHECK(mk_closeContext(cd, 0));
  }
  CHECK(mk_closeContext(ca, 0));
}

void  mk_destroyChapter(mk_Chapter *chapter) {
  if (strlen(chapter->chapterDisplay) > 0)
    free(chapter->chapterDisplay);
  if (chapter->next == NULL)
    *chapter->tail = chapter->prev;
  else
    chapter->next->prev = chapter->prev;
  if (chapter->prev != NULL)
    chapter->prev->next = chapter->next;
  free(chapter);
}

void mk_destroyChapters(mk_Writer *w) {
  mk_Chapter *chapter = *w->chapters->tail;

  while (chapter != NULL) {
    mk_destroyChapter(chapter);
    chapter = chapter->prev;
  }
  w->chapters = NULL;
}
