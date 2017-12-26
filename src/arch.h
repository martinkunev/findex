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

#if !defined(_ARCH_H)
#define _ARCH_H

#include <sys/types.h>

#if (BYTE_ORDER == BIG_ENDIAN)
# define ENDIAN_BIG
#elif (BYTE_ORDER == LITTLE_ENDIAN)
# define ENDIAN_LITTLE
#else
# error "Unable to detect endianness."
#endif

//  TODO deprecated
#if defined(OS_MAC)
# include <libkern/OSByteOrder.h>

# define htobe16(x) OSSwapHostToBigInt16(x)
# define htole16(x) OSSwapHostToLittleInt16(x)
# define betoh16(x) OSSwapBigToHostInt16(x)
# define letoh16(x) OSSwapLittleToHostInt16(x)

# define htobe32(x) OSSwapHostToBigInt32(x)
# define htole32(x) OSSwapHostToLittleInt32(x)
# define betoh32(x) OSSwapBigToHostInt32(x)
# define letoh32(x) OSSwapLittleToHostInt32(x)

# define htobe64(x) OSSwapHostToBigInt64(x)
# define htole64(x) OSSwapHostToLittleInt64(x)
# define betoh64(x) OSSwapBigToHostInt64(x)
# define letoh64(x) OSSwapLittleToHostInt64(x)
#elif defined(OS_WINDOWS)
# define htobe16(x) htons(x)
# define htole16(x) (x)
# define betoh16(x) ntohs(x)
# define letoh16(x) (x)

# define htobe32(x) htonl(x)
# define htole32(x) (x)
# define betoh32(x) ntohl(x)
# define letoh32(x) (x)

# define htobe64(x) ( ( (uint64_t)(htonl( (uint32_t)(((uint64_t)x << 32) >> 32) )) << 32) | htonl( ((uint32_t)((uint64_t)x >> 32)) ) )
# define htole64(x) (x)
# define betoh64(x) ( ( (uint64_t)(htonl( (uint32_t)(((uint64_t)x << 32) >> 32) )) << 32) | htonl( ((uint32_t)((uint64_t)x >> 32)) ) )
# define letoh64(x) (x)
#elif defined(OS_LINUX) || defined(OS_ANDROID)
# include <byteswap.h>

# undef htobe16
# undef htobe32
# undef htobe64

# undef htole16
# undef htole32
# undef htole64

# undef betoh16
# undef betoh32
# undef betoh64

# undef letoh16
# undef letoh32
# undef letoh64

# if defined(ENDIAN_BIG)
#  define htobe16(x) (x)
#  define htobe32(x) (x)
#  define htobe64(x) (x)

#  define htole16(x) bswap_16(x)
#  define htole32(x) bswap_32(x)
#  define htole64(x) bswap_64(x)

#  define betoh16(x) (x)
#  define betoh32(x) (x)
#  define betoh64(x) (x)

#  define letoh16(x) bswap_16(x)
#  define letoh32(x) bswap_32(x)
#  define letoh64(x) bswap_64(x)
# else /* defined(ENDIAN_LITTLE) */

#  define htobe16(x) bswap_16(x)
#  define htobe32(x) bswap_32(x)
#  define htobe64(x) bswap_64(x)

#  define htole16(x) (x)
#  define htole32(x) (x)
#  define htole64(x) (x)

#  define betoh16(x) bswap_16(x)
#  define betoh32(x) bswap_32(x)
#  define betoh64(x) bswap_64(x)

#  define letoh16(x) (x)
#  define letoh32(x) (x)
#  define letoh64(x) (x)
# endif
#else
# include <sys/endian.h>
# define	betoh16(x)	be16toh(x)
# define	betoh32(x)	be32toh(x)
# define	betoh64(x)	be64toh(x)
# define	letoh16(x)	le16toh(x)
# define	letoh32(x)	le32toh(x)
# define	letoh64(x)	le64toh(x)
#endif

// Simple endian conversion.
// TODO is this a good idea?
#include <string.h>
static inline void endian_swap16(void *restrict to, const void *restrict from)
{
	const char *src = from;
	char *dest = to;
	dest[0] = src[1]; dest[1] = src[0];
}
static inline void endian_swap32(void *restrict to, const void *restrict from)
{
	const char *src = from;
	char *dest = to;
	dest[0] = src[3]; dest[1] = src[2];
	dest[2] = src[1]; dest[3] = src[0];
}
static inline void endian_swap64(void *restrict to, const void *restrict from)
{
	const char *src = from;
	char *dest = to;
	dest[0] = src[7]; dest[1] = src[6];
	dest[2] = src[5]; dest[3] = src[4];
	dest[4] = src[3]; dest[5] = src[2];
	dest[6] = src[1]; dest[7] = src[0];
}

#if defined(ENDIAN_BIG)
# define endian_big16(dest, src) memcpy((dest), (src), 2)
# define endian_big32(dest, src) memcpy((dest), (src), 4)
# define endian_big64(dest, src) memcpy((dest), (src), 8)
# define endian_little16(dest, src) endian_swap16((dest), (src))
# define endian_little32(dest, src) endian_swap32((dest), (src))
# define endian_little64(dest, src) endian_swap64((dest), (src))
#else /* defined(ENDIAN_LITTLE) */
# define endian_big16(dest, src) endian_swap16((dest), (src))
# define endian_big32(dest, src) endian_swap32((dest), (src))
# define endian_big64(dest, src) endian_swap64((dest), (src))
# define endian_little16(dest, src) memcpy((dest), (src), 2)
# define endian_little32(dest, src) memcpy((dest), (src), 4)
# define endian_little64(dest, src) memcpy((dest), (src), 8)
#endif

#endif /* !defined(_ARCH_H) */
