/*
 * Filement Index
 * Copyright (C) 2017  Martin Kunev <martinkunev@gmail.com>
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

#include <inttypes.h>
#include <stdlib.h>
#include <unistd.h>

#include "arch.h"
#include "base.h"
#include "magic.h"

#define STRING(s) (s), sizeof(s) - 1

// WARNING: Files < 16B are never considered binary

// This library uses big endian as internal format.

#define sign4(b3, b2, b1, b0)					(((b3) << 24) | ((b2) << 16) | ((b1) << 8) | (b0))
#define sign8(b7, b6, b5, b4, b3, b2, b1, b0)	(((uint64_t)sign4((b7), (b6), (b5), (b4)) << 32) | (uint64_t)sign4((b3), (b2), (b1), (b0)))

// TODO: remove these
#define SIGNATURE_8(s) (((uint64_t)(s)[0] << 56) | ((uint64_t)(s)[1] << 48) | ((uint64_t)(s)[2] << 40) | ((uint64_t)(s)[3] << 32) | ((uint64_t)(s)[4] << 24) | ((uint64_t)(s)[5] << 16) | ((uint64_t)(s)[6] << 8) | (uint64_t)(s)[7])
#define SIGNATURE_4(s) (((s)[0] << 24) | ((s)[1] << 16) | ((s)[2] << 8) | (s)[3])

// Adding new formats can introduce the following problems:
// - Valid text files could be recognized as being the new format.
// - Files with size < 16B will not be recognized.

// Text files are expected to be encoded in UTF-8 without BOM.

/*
TODO: what we need to know for a file
- mime type
- archive format
- could it be tar (or compressed tar)
- is it text file
*/


/* TODO file size
JPEG
http://en.wikipedia.org/wiki/JPEG#Syntax_and_structure
File starts with Start Of Image marker. Immediately after it there is another marker.
bytes 0-1:		Start Of Image			ffd8
byte 2:									ff
*/
#define JPEG_MASK_0_3			0xffffff00
#define JPEG_MAGIC_0_3			0xffd8ff00


/*
PNG
http://en.wikipedia.org/wiki/Portable_Network_Graphics#File_header
File starts with Signature (8B) and at least one chunk which (>= 12B).
bytes 0-7:		Signature				89504e470d0a1a0a
*/
#define PNG_MAGIC_0_7			0x89504e470d0a1a0a


/* TODO: valid text
GIF
http://en.wikipedia.org/wiki/Gif#File_format
http://tronche.com/computer-graphics/gif/gif89a.html#screen-descriptor
Header Block (6B) is always followed by Logical Screen Descriptor (7B). There is at least one segment (>= 2B). File always ends in terminator (1B).
There are 2 versions of the format - 87 and 89.
bytes 0-5:		header					"GIF87a" || "GIF89a"
*/
#define GIF_MASK_0_7			0xffffffffffff0000
#define GIF_87_MAGIC_0_7		sign8('G', 'I', 'F', '8', '7', 'a', 0, 0)
#define GIF_89_MAGIC_0_7		sign8('G', 'I', 'F', '8', '9', 'a', 0, 0)


/* TODO: valid text
BMP
http://en.wikipedia.org/wiki/BMP_file_format#File_structure
Bitmap header (14B) is followed by DIB header (>= 12B).
bytes 0-1:		header field			"BM"
bytes 2-5:		file size LE
*/
#define BMP_MAGIC_0_1			(('B' << 8) | 'M')


/*
QuickTime File Format
https://developer.apple.com/library/mac/#documentation/QuickTime/qtff/QTFFChap1/qtff1.html
File starts with File Type Compatibility Atom. It is at least 16B long.
bytes 0-3		atom size with headers
bytes 4-7:		atom type				"ftyp"
bytes 8-11:		major brand
bytes 12-15:	minor version
Assume that bytes 0-2 are 0 since most real QTFF files have File Type Compatibility Atom with size less than 255.

Major brand can be used to find the exact MIME type.
http://ftyps.com/#2
http://wiki.multimedia.cx/index.php?title=QuickTime_container#wide
*/
// TODO: some QTFF files begin with skip or wide atom
#define QTFF_MASK_0_7					0xffffff00ffffffff
#define QTFF_MAGIC_0_7					sign8(0, 0, 0, 0, 'f', 't', 'y', 'p')

#define QTFF_QUICKTIME					sign4('q', 't', ' ', ' ')

#define QTFF_MPEG4_0					sign4('a', 'v', 'c', '1')
#define QTFF_MPEG4_1					sign4('i', 's', 'o', '2')
#define QTFF_MPEG4_2					sign4('i', 's', 'o', 'm')
#define QTFF_MPEG4_3					sign4('m', 'p', '4', '1')
#define QTFF_MPEG4_4					sign4('m', 'p', '4', '2')

#define QTFF_M4AUDIO					sign4('M', '4', 'A', ' ')

#define QTFF_M4VIDEO					sign4('M', '4', 'V', ' ')

#define QTFF_3GPP_0						sign4('3', 'g', 'p', '4')
#define QTFF_3GPP_1						sign4('3', 'g', 'p', '5')
#define QTFF_3GPP_2						sign4('3', 'g', 'p', '6')


/* TODO file size
Matroska
http://matroska.org/technical/specs/index.html
http://matroska.org/files/matroska.pdf
http://matroska.org/news/webm-matroska.html
Files use the markup language EBML. WebM is Matroska that uses specific audio and video codecs.
bytes 0-3:								1a45dfa3
*/
// TODO: this assumes that each EBML file is Matroska
#define MATROSKA_0_3					0x1a45dfa3


/* TODO file size
MPEG Audio
http://en.wikipedia.org/wiki/MP3
File starts with either MPEG Audio Frame Header or ID3v2 metadata.
Some files start with lots of 0 bytes (probably padding to allow later addition of ID3v2 tag).

MPEG Audio Frame Header:
http://mpgedit.org/mpgedit/mpeg_format/mpeghdr.htm
bits 0-11:		frame sync; version		fff
bit 12:			version 1 / 2
bits 13-14:		layer description		1 (layer III) || 2 (layer II)
bit 15:			protection

ID3v2 metadata:
http://id3.org/id3v2.4.0-structure
bytes 0-2:								"ID3"
*/
// TODO: files starting with 16 zero bytes are assumed to be audio/mpeg
#define MPEGAUDIO_MASK_0_1				0xfff6
#define MP2_MAGIC_0_1					0xfff4
#define MP3_MAGIC_0_1					0xfff2
#define ID3v2_MASK_0_3					0xffffff00
#define ID3v2_MAGIC_0_3					sign4('I', 'D', '3', 0)


/* TODO file size
MPEG video
http://dvd.sourceforge.net/dvdinfo/mpeghdrs.html
http://aeroquartet.com/movierepair/mpeg
Files start with 3B prefix and 1B Stream ID. Stream ID is usually Pack Header.
bytes 0-3:		Pack Header				000001ba
*/
#define MPEGVIDEO_0_3					0x000001ba


/*
OGG					application/ogg
http://en.wikipedia.org/wiki/Ogg#File_format
bytes 0-3:		capture pattern			"OggS"
byte 4:			version					0
Determine whether the file is audio or video by the first bitstream.
TODO: distinguish vorbis / theora
*/
#define OGG_MAGIC_0_3					sign4('O', 'g', 'g', 'S')
#define OGG_MAGIC_4						0


/* TODO: this is usually wma or wmv
ASF					application/vnd.ms-asf
http://www.digitalpreservation.gov/formats/fdd/fdd000067.shtml
bytes 0-15:		magic					3026b2758e66cf11a6d900aa0062ce6c
*/
#define ASF_MAGIC_0_7			0x3026b2758e66cf11
#define ASF_MAGIC_8_15			0xa6d900aa0062ce6c


/* TODO file size; valid text
Resource Interchange File Format
http://en.wikipedia.org/wiki/Resource_Interchange_File_Format
File starts with RIFF chunk.
bytes 0-3		chunk identifier		"RIFF"
bytes 4-7		chunk size LE
bytes 8-11		file content			"WAVE" || "AVI "
*/
#define RIFF_MAGIC_0_3					sign4('R', 'I', 'F', 'F')
#define RIFF_WAVE						sign4('W', 'A', 'V', 'E')
#define RIFF_AVI						sign4('A', 'V', 'I', ' ')


/* TODO valid text
DjVu				image/vnd.djvu
http://www.fileformat.info/info/mimetype/image/vnd.djvu/index.htm
http://djvu.sourceforge.net/specs/djvu3changes.txt
http://www.martinreddy.net/gfx/2d/IFF.txt
bytes 0-3:								"AT&T"
bytes 4-7:			IFF85 ID			"FORM"
bytes 8-11:			chunk size
bytes 12-15:							"DJVU" | "DJVM" | "PM44" | "BM44"
*/
#define DJVU_MAGIC_0_7					SIGNATURE_8("AT&TFORM")

/*
Microsoft Office OLE Compound File
http://www.forensicswiki.org/wiki/OLE_Compound_File
All MS Office documents up to 2003 use this format.
bytes 0-7:								d0cf11e0a1b11ae1
*/
// TODO: there is no good way to distinguish office documents
#define OLECF_MAGIC_0_7					0xd0cf11e0a1b11ae1


/* TODO file size
ZIP					application/zip
http://www.pkware.com/documents/casestudies/APPNOTE.TXT
File starts with a 4B signature.
bytes 0-3:		signature				04034b50 || 06054b50 || 08074b50
*/
#define ZIP_MAGIC_FILE_0_3				0x504b0304
#define ZIP_MAGIC_EMPTY_0_3				0x504b0506
#define ZIP_MAGIC_SPLIT_0_3				0x504b0708

/* TODO valid text
RAR					application/vnd.rar
http://en.wikipedia.org/wiki/RAR
Minimum size is 20B
bytes 0-3:		signature				"Rar!"
*/
#define RAR_MAGIC_0_3					sign4('R', 'a', 'r', '!')

/* TODO file size; valid text
PDF					application/pdf
http://www.digitalpreservation.gov/formats/fdd/fdd000030.shtml
bytes 0-3:		magic					"%PDF"
*/
#define PDF_MAGIC_0_3					SIGNATURE_4("%PDF")


/* TODO file size
GZip				application/gzip
http://www.gzip.org/zlib/rfc-gzip.html
File starts with 2 identification bytes.
bytes 0-1								1f8b
*/
#define GZIP_MAGIC_0_1					0x1f8b


/* TODO file size; valid text
BZip 2				application/x-bzip2
http://en.wikipedia.org/wiki/Bzip2
bytes 0-1:								"BZ"
byte 2			version					'h'
byte 3			block size				'1' ... '9'
Check whether bytes 0-2 match and the first 4 bits of byte 3 match (all ASCII digits start with the same 4 bits).
*/
#define BZIP2_MASK_0_3					0xfffffff0
#define BZIP2_MAGIC_0_3					sign4('B', 'Z', 'h', '0' & 0xf0)


/* TODO file size
Xz					application/x-xz
https://tukaani.org/xz/xz-file-format.txt
bytes 0-5		header magic bytes		fd377a585a00
*/
#define XZ_MASK_0_7						0xffffffffffff0000llu
#define XZ_MAGIC_0_7					sign8(0xfd, 0x37, 0x7a, 0x58, 0x5a, 0, 0, 0)


/* TODO file size
7-Zip				application/x-7z-compressed
http://cpansearch.perl.org/src/BJOERN/Compress-Deflate7-1.0/7zip/DOC/7zFormat.txt
bytes 0-1		signature				"7z"
bytes 2-5		signature				bcaf271c
*/
#define ZIP7_MASK_0_7					0xffffffffffff0000llu
#define ZIP7_MAGIC_0_7					sign8('7', 'z', 0xbc, 0xaf, 0x27, 0x1c, 0, 0)


/* TODO better detection
tar					application/x-tar
http://www.gnu.org/software/tar/manual/html_node/Standard.html
http://www.fileformat.info/format/tar/corion.htm
Sequence of 0 or more ASCII characters followed by 1 or more NULs. The first 100 bytes are checked.
*/


/* TODO: this doesn't work because the header is at the end of the file
Apple Disk Image	application/x-apple-diskimage
http://en.wikipedia.org/wiki/Apple_Disk_Image#Data_format
http://newosxbook.com/DMG.html
bytes 0-3:		magic					"GMI2"
*/
//#define MAGIC_DMG_0_3			SIGNATURE_4("GMI2")
//uint32_t MAGIC_DMG = STRING("application/x-apple-diskimage");


// TODO: recognize different msoffice formats
// TODO: different text files:
//  <?php <?xml #! <! <html %!

static const unsigned char utf8[] = {
//	_0    _1    _2    _3    _4    _5    _6    _7    _8    _9    _a    _b    _c    _d    _e    _f
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,    0,    0, 0xff, 0xff,    0, 0xff, 0xff, // 0_
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, // 1_
	   0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0, // 2_
	   0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0, // 3_
	   0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0, // 4_
	   0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0, // 5_
	   0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0, // 6_
	   0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 0xff, // 7_
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, // 8_
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, // 9_
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, // a_
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, // b_
	   1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1, // c_
	   1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1, // d_
	   2,    2,    2,    2,    2,    2,    2,    2,    2,    2,    2,    2,    2,    2,    2,    2, // e_
	   3,    3,    3,    3,    3,    3,    3,    3,    4,    4,    4,    4,    5,    5, 0xff, 0xff, // f_
};
#define UTF8_INVALID 0xff

const struct filetype typeinfo[] =
{
	// TODO ar
	// TODO: better tar detection
	[TYPE_TAR] = {CONTENT_ARCHIVE | CONTENT_DIRECTORY, bytes("application/x-tar")},
	[TYPE_ZIP] = {CONTENT_ARCHIVE | CONTENT_DIRECTORY, bytes("application/zip")},
	[TYPE_RAR] = {CONTENT_ARCHIVE, bytes("application/vnd.rar")},
	[TYPE_7ZIP] = {CONTENT_ARCHIVE, bytes("application/x-7z-compressed")},

	// WARNING: Files compressed with gzip, bzip2 or xz are treated as tar archives.
	[TYPE_GZIP] = {CONTENT_ARCHIVE | CONTENT_DIRECTORY, bytes("application/gzip")},
	[TYPE_BZIP2] = {CONTENT_ARCHIVE, bytes("application/x-bzip2")},
	[TYPE_XZ] = {CONTENT_ARCHIVE, bytes("application/x-xz")},

	// TODO openoffice
	[TYPE_DJVU] = {CONTENT_DOCUMENT, bytes("image/vnd.djvu")},
	[TYPE_PDF] = {CONTENT_DOCUMENT, bytes("application/pdf")},
	[TYPE_MSWORD] = {CONTENT_DOCUMENT, bytes("application/msword")}, // TODO this could be any msoffice document

	[TYPE_PNG] = {CONTENT_IMAGE, bytes("image/png")},
	[TYPE_JPEG] = {CONTENT_IMAGE, bytes("image/jpeg")},
	[TYPE_GIF] = {CONTENT_IMAGE | CONTENT_VIDEO, bytes("image/gif")}, // TODO not all gifs are animated
	[TYPE_BMP] = {CONTENT_IMAGE, bytes("image/x-bmp")},

	[TYPE_MPEGAUDIO] = {CONTENT_AUDIO, bytes("audio/mpeg")},

	[TYPE_MPEGVIDEO] = {CONTENT_AUDIO | CONTENT_VIDEO, bytes("video/mpeg")},

	[TYPE_OGG] = {CONTENT_AUDIO, bytes("application/ogg")}, // TODO could be vorbis/theora
	[TYPE_MATROSKA] = {CONTENT_AUDIO | CONTENT_VIDEO, bytes("video/x-matroska")}, // TODO could be regular/webm
	[TYPE_WAVE] = {CONTENT_AUDIO, bytes("audio/wave")},
	[TYPE_AVI] = {CONTENT_AUDIO | CONTENT_VIDEO, bytes("video/avi")},
	[TYPE_ASF] = {CONTENT_AUDIO | CONTENT_VIDEO, bytes("application/vnd.ms-asf")}, // TODO could be wma/wmv
	[TYPE_QUICKTIME] = {CONTENT_AUDIO | CONTENT_VIDEO, bytes("video/quicktime")},
	[TYPE_MPEG4] = {CONTENT_AUDIO | CONTENT_VIDEO, bytes("video/mp4")},
	[TYPE_M4AUDIO] = {CONTENT_AUDIO, bytes("audio/x-m4a")},
	[TYPE_M4VIDEO] = {CONTENT_AUDIO | CONTENT_VIDEO, bytes("video/x-m4v")},
	[TYPE_3GPP] = {CONTENT_AUDIO | CONTENT_VIDEO, bytes("video/3gpp")},

	[TYPE_TEXT] = {CONTENT_TEXT, bytes("text/plain")},
	[TYPE_TEXT_SCRIPT] = {CONTENT_TEXT | CONTENT_EXECUTABLE, bytes("text/plain")}, // TODO differentiate sh, csh, python, perl, ruby, etc.
	[TYPE_TEXT_XML] = {CONTENT_TEXT, bytes("text/xml")},

	[TYPE_UNKNOWN] = {0, bytes("application/octet-stream")},
};

// Treat files that start with ASCII characters and continue with NULs as tar archives.
static enum type content_unknown(const unsigned char *restrict magic, size_t size)
{
	const unsigned char *start = magic, *end;
	unsigned type;

	// TODO ? support encodings other than UTF-8

	for(end = start + size; start < end; ++start)
	{
		type = utf8[*start];
		if (type == UTF8_INVALID)
		{
			if (size >= 100)
			{
				while (!*start)
					if (++start >= (magic + 100))
						return TYPE_TAR;
			}
			return TYPE_UNKNOWN;
		}

		// There should be type number of bytes, starting with 10 bits after the first byte.
		while (type--)
			if ((++start == end) || ((*start & 0xc0) != 0x80))
				return TYPE_UNKNOWN;
	}

	if (!memcmp(magic, STRING("#!")))
		return TYPE_TEXT_SCRIPT;
	else if (!memcmp(magic, STRING("<?xml ")))
		return TYPE_TEXT_XML;
	else
		return TYPE_TEXT;
}

// WARNING: When size is less than 16, this function considers that this is the size of the whole file.
enum type content(const unsigned char *magic, size_t size)
{
	if (size < MAGIC_SIZE) return content_unknown(magic, size); // most likely a text file

	uint64_t bytes_0_7, bytes_8_15;
	endian_big64(&bytes_0_7, magic);
	endian_big64(&bytes_8_15, magic + 8);

	uint32_t bytes_0_3 = bytes_0_7 >> 32;
	uint16_t bytes_0_1 = bytes_0_7 >> 48;

	// image

	if ((bytes_0_3 & JPEG_MASK_0_3) == JPEG_MAGIC_0_3)
		return TYPE_JPEG;

	if (bytes_0_7 == PNG_MAGIC_0_7)
		return TYPE_PNG;

	switch (bytes_0_7 & GIF_MASK_0_7) // GIF
	{
	case GIF_87_MAGIC_0_7:
	case GIF_89_MAGIC_0_7:
		return TYPE_GIF;
	}

	// audio/video

	if ((bytes_0_7 & QTFF_MASK_0_7) == QTFF_MAGIC_0_7) // quick time container
	{
		switch (bytes_8_15 >> 32)
		{
		case QTFF_QUICKTIME:
			return TYPE_QUICKTIME;
		case QTFF_MPEG4_0:
		case QTFF_MPEG4_1:
		case QTFF_MPEG4_2:
		case QTFF_MPEG4_3:
		case QTFF_MPEG4_4:
			return TYPE_MPEG4;
		case QTFF_M4AUDIO:
			return TYPE_M4AUDIO;
		case QTFF_M4VIDEO:
			return TYPE_M4VIDEO;
		case QTFF_3GPP_0:
		case QTFF_3GPP_1:
		case QTFF_3GPP_2:
			return TYPE_3GPP;
		}
	}

	if (bytes_0_3 == MATROSKA_0_3)
		return TYPE_MATROSKA;

	if (((bytes_0_1 & MPEGAUDIO_MASK_0_1) == MP3_MAGIC_0_1) || ((bytes_0_1 & MPEGAUDIO_MASK_0_1) == MP2_MAGIC_0_1) || ((bytes_0_3 & ID3v2_MASK_0_3) == ID3v2_MAGIC_0_3) || (!bytes_0_7 && !bytes_8_15))
		return TYPE_MPEGAUDIO;

	if (bytes_0_3 == MPEGVIDEO_0_3)
		return TYPE_MPEGVIDEO;

	if ((bytes_0_3 == OGG_MAGIC_0_3) && (magic[4] == OGG_MAGIC_4))
		return TYPE_OGG;

	if (bytes_0_3 == RIFF_MAGIC_0_3)
	{
		switch (bytes_8_15 >> 32)
		{
		case RIFF_WAVE:
			return TYPE_WAVE;
		case RIFF_AVI:
			return TYPE_AVI;
		}
	}

	if ((bytes_0_7 == ASF_MAGIC_0_7) && (bytes_8_15 == ASF_MAGIC_8_15))
		return TYPE_ASF;

	// archive

	switch (bytes_0_3)
	{
	case ZIP_MAGIC_FILE_0_3:
	case ZIP_MAGIC_SPLIT_0_3:
	case ZIP_MAGIC_EMPTY_0_3:
		return TYPE_ZIP;
	case RAR_MAGIC_0_3:
		return TYPE_RAR;
	}

	if (bytes_0_1 == GZIP_MAGIC_0_1)
		return TYPE_GZIP;

	if ((bytes_0_3 & BZIP2_MASK_0_3) == BZIP2_MAGIC_0_3)
		return TYPE_BZIP2;

	if ((bytes_0_7 & XZ_MASK_0_7) == XZ_MAGIC_0_7)
		return TYPE_XZ;

	if ((bytes_0_7 & ZIP7_MASK_0_7) == ZIP7_MAGIC_0_7)
		return TYPE_7ZIP;

	// other formats

	if (bytes_0_3 == PDF_MAGIC_0_3)
		return TYPE_PDF;

	if (bytes_0_7 == OLECF_MAGIC_0_7)
		return TYPE_MSWORD;

	if (bytes_0_7 == DJVU_MAGIC_0_7)
		return TYPE_DJVU;

	if ((bytes_0_1 == BMP_MAGIC_0_1) && ((bytes_0_7 >> 16) & 0xffffffff >= 26))
		return TYPE_BMP;

	return content_unknown(magic, size);
}
