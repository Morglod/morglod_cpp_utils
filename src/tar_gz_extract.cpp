#include "./tar_gz_extract.hpp"

#include "./log.hpp"
#include "./read_file.hpp"
#include "./tar_extract.hpp"
#include "./profiler.hpp"

#include <libdeflate.h>

#include <errno.h>
#include <sys/stat.h>
#ifdef _WIN32
#  include <sys/utime.h>
#else
#  include <sys/time.h>
#  include <unistd.h>
#  include <utime.h>
#endif

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

#define MIN(a, b)		((a) <= (b) ? (a) : (b))
#define MAX(a, b)		((a) >= (b) ? (a) : (b))

#if defined(__GNUC__) && !defined(__clang__) && !defined(__INTEL_COMPILER)
#  define GCC_PREREQ(major, minor)		\
	(__GNUC__ > (major) ||			\
	 (__GNUC__ == (major) && __GNUC_MINOR__ >= (minor)))
#else
#  define GCC_PREREQ(major, minor)	0
#endif

#if defined(__BYTE_ORDER__) /* gcc v4.6+ and clang */
#  define CPU_IS_LITTLE_ENDIAN()  (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
#elif defined(_MSC_VER)
#  define CPU_IS_LITTLE_ENDIAN()  true
#else
static forceinline bool CPU_IS_LITTLE_ENDIAN(void)
{
	union {
		u32 w;
		u8 b;
	} u;

	u.w = 1;
	return u.b;
}
#endif

/* bswap16(v) - swap the bytes of a 16-bit integer */
static inline uint16_t bswap16(uint16_t v)
{
#if GCC_PREREQ(4, 8) || __has_builtin(__builtin_bswap16)
	return __builtin_bswap16(v);
#elif defined(_MSC_VER)
	return _byteswap_ushort(v);
#else
	return (v << 8) | (v >> 8);
#endif
}

/* bswap32(v) - swap the bytes of a 32-bit integer */
static inline uint32_t bswap32(uint32_t v)
{
#if GCC_PREREQ(4, 3) || __has_builtin(__builtin_bswap32)
	return __builtin_bswap32(v);
#elif defined(_MSC_VER)
	return _byteswap_ulong(v);
#else
	return ((v & 0x000000FF) << 24) |
	       ((v & 0x0000FF00) << 8) |
	       ((v & 0x00FF0000) >> 8) |
	       ((v & 0xFF000000) >> 24);
#endif
}

/* bswap64(v) - swap the bytes of a 64-bit integer */
static inline uint64_t bswap64(uint64_t v)
{
#if GCC_PREREQ(4, 3) || __has_builtin(__builtin_bswap64)
	return __builtin_bswap64(v);
#elif defined(_MSC_VER)
	return _byteswap_uint64(v);
#else
	return ((v & 0x00000000000000FF) << 56) |
	       ((v & 0x000000000000FF00) << 40) |
	       ((v & 0x0000000000FF0000) << 24) |
	       ((v & 0x00000000FF000000) << 8) |
	       ((v & 0x000000FF00000000) >> 8) |
	       ((v & 0x0000FF0000000000) >> 24) |
	       ((v & 0x00FF000000000000) >> 40) |
	       ((v & 0xFF00000000000000) >> 56);
#endif
}

#define le16_bswap(v) (CPU_IS_LITTLE_ENDIAN() ? (v) : bswap16(v))
#define le32_bswap(v) (CPU_IS_LITTLE_ENDIAN() ? (v) : bswap32(v))
#define le64_bswap(v) (CPU_IS_LITTLE_ENDIAN() ? (v) : bswap64(v))
#define be16_bswap(v) (CPU_IS_LITTLE_ENDIAN() ? bswap16(v) : (v))
#define be32_bswap(v) (CPU_IS_LITTLE_ENDIAN() ? bswap32(v) : (v))
#define be64_bswap(v) (CPU_IS_LITTLE_ENDIAN() ? bswap64(v) : (v))

#define GZIP_MIN_HEADER_SIZE	10
#define GZIP_FOOTER_SIZE	8
#define GZIP_MIN_OVERHEAD	(GZIP_MIN_HEADER_SIZE + GZIP_FOOTER_SIZE)
#define GZIP_ID1		0x1F
#define GZIP_ID2		0x8B

/*
 * UNALIGNED_ACCESS_IS_FAST() - 1 if unaligned memory accesses can be performed
 * efficiently on the target platform, otherwise 0.
 */
#if (defined(__GNUC__) || defined(__clang__)) && \
	(defined(ARCH_X86_64) || defined(ARCH_X86_32) || \
	 defined(__ARM_FEATURE_UNALIGNED) || defined(__powerpc64__) || \
	 /*
	  * For all compilation purposes, WebAssembly behaves like any other CPU
	  * instruction set. Even though WebAssembly engine might be running on
	  * top of different actual CPU architectures, the WebAssembly spec
	  * itself permits unaligned access and it will be fast on most of those
	  * platforms, and simulated at the engine level on others, so it's
	  * worth treating it as a CPU architecture with fast unaligned access.
	  */ defined(__wasm__))
#  define UNALIGNED_ACCESS_IS_FAST	1
#elif defined(_MSC_VER)
#  define UNALIGNED_ACCESS_IS_FAST	1
#else
#  define UNALIGNED_ACCESS_IS_FAST	0
#endif

inline uint32_t load_u32_unaligned(const uint8_t* p) {
	return *reinterpret_cast<const uint32_t*>(p);
}

inline uint32_t
get_unaligned_le32(const uint8_t *p)
{
	if (UNALIGNED_ACCESS_IS_FAST)
		return le32_bswap(load_u32_unaligned(p));
	else
		return ((uint32_t)p[3] << 24) | ((uint32_t)p[2] << 16) |
			((uint32_t)p[1] << 8) | p[0];
}

// ---------------------------------------
// ---------------------------------------
// ---------------------------------------
// ---------------------------------------
// ---------------------------------------
// ---------------------------------------
// ---------------------------------------
// ---------------------------------------
// ---------------------------------------

int do_decompress(
	struct libdeflate_decompressor *decompressor,
	const uint8_t* compressed_data, size_t compressed_size,
	char** out_uncompressed_data, size_t* out_uncompressed_size
)
{
    _PROFILER_AUTO();

	void *uncompressed_data = NULL;
	size_t uncompressed_size;
	size_t max_uncompressed_size;
	size_t actual_in_nbytes;
	size_t actual_out_nbytes;
	enum libdeflate_result result;
	int ret = 0;

	if (compressed_size < GZIP_MIN_OVERHEAD ||
	    compressed_data[0] != GZIP_ID1 ||
	    compressed_data[1] != GZIP_ID2)
	{
		KS_CRIT_LOG_ERROR("not gzip format");
		return -1;
	}

	/*
	 * Use the ISIZE field as a hint for the decompressed data size.  It may
	 * need to be increased later, however, because the file may contain
	 * multiple gzip members and the particular ISIZE we happen to use may
	 * not be the largest; or the real size may be >= 4 GiB, causing ISIZE
	 * to overflow.  In any case, make sure to allocate at least one byte.
	 */
	uncompressed_size = get_unaligned_le32(&compressed_data[compressed_size - 4]);
	if (uncompressed_size == 0)
		uncompressed_size = 1;

	/*
	 * DEFLATE cannot expand data more than 1032x, so there's no need to
	 * ever allocate a buffer more than 1032 times larger than the
	 * compressed data.  This is a fail-safe, albeit not a very good one, if
	 * ISIZE becomes corrupted on a small file.  (The 1032x number comes
	 * from each 2 bits generating a 258-byte match.  This is a hard upper
	 * bound; the real upper bound is slightly smaller due to overhead.)
	 */
	if (compressed_size <= SIZE_MAX / 1032)
		max_uncompressed_size = compressed_size * 1032;
	else
		max_uncompressed_size = SIZE_MAX;

	do {
		if (uncompressed_data == NULL) {
			uncompressed_size = MIN(uncompressed_size, max_uncompressed_size);
			uncompressed_data = malloc(uncompressed_size);
			if (uncompressed_data == NULL) {
				KS_CRIT_LOG_ERROR("file is probably too large to be processed by this program");
				ret = -1;
				goto out;
			}
		}

		result = libdeflate_gzip_decompress_ex(
			decompressor,
			compressed_data,
			compressed_size,
			uncompressed_data,
			uncompressed_size,
			&actual_in_nbytes,
			&actual_out_nbytes
		);

		if (result == LIBDEFLATE_INSUFFICIENT_SPACE) {
			if (uncompressed_size >= max_uncompressed_size) {
				KS_CRIT_LOG_ERROR("Bug in libdeflate_gzip_decompress_ex(): data expanded too much!");
				ret = -1;
				goto out;
			}
			if (uncompressed_size * 2 <= uncompressed_size) {
				KS_CRIT_LOG_ERROR("file corrupt or too large to be  processed by this program");
				ret = -1;
				goto out;
			}
			uncompressed_size *= 2;
			free(uncompressed_data);
			uncompressed_data = NULL;
			continue;
		}

		if (result != LIBDEFLATE_SUCCESS) {
			KS_CRIT_LOG_ERROR("file corrupt or not in gzip format");
			ret = -1;
			goto out;
		}

		if (actual_in_nbytes == 0 ||
		    actual_in_nbytes > compressed_size ||
		    actual_out_nbytes > uncompressed_size) {
			KS_CRIT_LOG_ERROR("Bug in libdeflate_gzip_decompress_ex(): impossible actual_nbytes value!");
			ret = -1;
			goto out;
		}

		// ret = full_write(out, uncompressed_data, actual_out_nbytes);
		// if (ret != 0)
		// 	goto out;

		compressed_data += actual_in_nbytes;
		compressed_size -= actual_in_nbytes;
	} while (compressed_size != 0);

out:
	*out_uncompressed_data = (char*)uncompressed_data;
	*out_uncompressed_size = uncompressed_size;

	return ret;
}

static struct libdeflate_decompressor *_decompressor = nullptr;

const char* tar_gz_extract_map_file(const char* fname, size_t& length);

void tar_gz_extract(
    const char *filename,
    char filepath_buf[1024], int dst_path_len, int substr_archive_entry_path
) {
    _PROFILER_AUTO();

	if (!_decompressor) _decompressor = libdeflate_alloc_decompressor();

	uint8_t* compressed_data;
	// size_t compressed_size;
	uint32_t compressed_size;
	read_file_all(filename, (char**)&compressed_data, &compressed_size);
	// const uint8_t* compressed_data = (const uint8_t*)tar_gz_extract_map_file(filename, compressed_size);

	char* uncompressed_data;
	size_t uncompressed_size;
	do_decompress(_decompressor, compressed_data, compressed_size, &uncompressed_data, &uncompressed_size);

	// munmap((void*)compressed_data, compressed_size);
	free(compressed_data);

	tar_extract_nodeflate_from_memory(uncompressed_data, uncompressed_size, filepath_buf, dst_path_len, substr_archive_entry_path);

	free(uncompressed_data);
}

const char* tar_gz_extract_map_file(const char* fname, size_t& length)
{
    _PROFILER_AUTO();

    int fd = open(fname, O_RDONLY);
    if (fd == -1)
        KS_CRIT_LOG_ERROR("open");

    // obtain file size
    struct stat sb;
    if (fstat(fd, &sb) == -1)
        KS_CRIT_LOG_ERROR("fstat");

    length = sb.st_size;

    const char* addr = static_cast<const char*>(mmap(NULL, length, PROT_READ, MAP_PRIVATE, fd, 0u));
    if (addr == MAP_FAILED)
        KS_CRIT_LOG_ERROR("mmap");

    // TODO close fd at some point in time, call munmap(...)
    return addr;
}
