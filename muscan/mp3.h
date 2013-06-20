/* Tools - Tools for Museek (muscan)
 *
 * Copyright (C) 2003-2004 Hyriand <hyriand@thegraveyard.org>
 * Copyright 2008 little blue poney <lbponey@users.sourceforge.net>
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __MP3SCAN_H__
#define __MP3SCAN_H__

#include <stdio.h>
#include <string.h>

typedef struct
{
	char valid;
	char vbr;
	
	int mpeg_version;
	char layer;
	char protection_bit;
	int bitrate;
	int samplerate;
	char padding_bit;
	char private_bit;
	char mode;
	char mode_extension;
	char copyright;
	char original;
	char emphasis;
	
	double framelength;
	double samplesperframe;
	long length;
} mp3info;

typedef unsigned int uint32;

char mp3_scan(const char *filename, mp3info *info);

#endif /* __MP3SCAN_HH__ */
