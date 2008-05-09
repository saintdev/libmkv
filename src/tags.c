/*****************************************************************************
 * tags.c:
 *****************************************************************************
 * Copyright (C) 2007 libmkv
 *
 * Authors: John A. Stebbins
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

int mk_createStringTag(mk_Writer * w, char *tag_id, char *value)
{
	mk_Context *tag, *simple;

	if (w->tags == NULL) {
		if ((w->tags = mk_createContext(w, w->root, 0x1254c367)) == NULL)	// Tags
			return -1;
	}
	if ((tag = mk_createContext(w, w->tags, 0x7373)) == NULL)	// Tag
		return -1;
	if ((simple = mk_createContext(w, tag, 0x67c8)) == NULL)	// A simple tag
		return -1;

	CHECK(mk_writeStr(simple, 0x45a3, tag_id));	/* TagName */
	CHECK(mk_writeStr(simple, 0x4487, value));	/* TagString */
	CHECK(mk_closeContext(simple, 0));
	CHECK(mk_closeContext(tag, 0));

	return 0;
}

int mk_writeTags(mk_Writer * w)
{
	if (w->tags == NULL)
		return -1;
	CHECK(mk_closeContext(w->tags, 0));

	return 0;
}
