/*
 * Filement Index
 * Copyright (C) 2018  Martin Kunev <martinkunev@gmail.com>
 *
 * This file is part of Filement Index.
 *
 * Filement Index is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation version 3 of the License.
 *
 * Filement Index is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Filement Index.  If not, see <http://www.gnu.org/licenses/>.
 */

#define MAGIC_SIZE 16

enum
{
	CONTENT_DIRECTORY = 0x1,
	CONTENT_LINK = 0x2,
	CONTENT_SPECIAL = 0x4,
	CONTENT_EXECUTABLE = 0x8,
	CONTENT_TEXT = 0x10,
	CONTENT_ARCHIVE = 0x20,
	CONTENT_DOCUMENT = 0x40,
	CONTENT_IMAGE = 0x80,
	CONTENT_AUDIO = 0x100,
	CONTENT_VIDEO = 0x200,
	CONTENT_DATABASE = 0x400,
};

struct filetype
{
	uint16_t content;
	const struct bytes *mime_type;
};

extern const struct filetype typeinfo[];

enum type {TYPE_UNKNOWN, TYPE_TAR, TYPE_ZIP, TYPE_RAR, TYPE_7ZIP, TYPE_GZIP, TYPE_BZIP2, TYPE_XZ, TYPE_ELF, TYPE_MACHO, TYPE_DJVU, TYPE_PDF, TYPE_MSWORD, TYPE_PNG, TYPE_JPEG, TYPE_GIF, TYPE_BMP, TYPE_MPEGAUDIO, TYPE_MPEGVIDEO, TYPE_OGG, TYPE_MATROSKA, TYPE_WAVE, TYPE_AVI, TYPE_ASF, TYPE_QUICKTIME, TYPE_MPEG4, TYPE_M4AUDIO, TYPE_M4VIDEO, TYPE_3GPP, TYPE_TEXT, TYPE_TEXT_SCRIPT, TYPE_TEXT_XML};

enum type content(const unsigned char *magic, size_t size);
