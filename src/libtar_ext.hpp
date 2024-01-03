#pragma once

#include <cstring>
#include <filesystem>
#include <zlib.h>
#include <libtar.h>
#include <fcntl.h>
#include <utime.h>
#include <libgen.h>

#define BIT_ISSET(bitmask, bit) ((bitmask) & (bit))

int
tar_extract_regfile2(TAR *t, char *realname, char* block_size_buf)
{
    _PROFILER_AUTO();

	mode_t mode;
	size_t size;
	uid_t uid;
	gid_t gid;
	int fdout;
	ssize_t i, k;
	char *filename;

#ifdef tarlib_ext_DEBUG
	printf("==> tar_extract_regfile(t=0x%lx, realname=\"%s\")\n", t,
	       realname);
#endif

	if (!TH_ISREG(t))
	{
		errno = EINVAL;
		return -1;
	}

	filename = (realname ? realname : th_get_pathname(t));
	mode = th_get_mode(t);
	size = th_get_size(t);
	uid = th_get_uid(t);
	gid = th_get_gid(t);

	_PROFILER_START("tar_extract_regfile hierdir", hierdir);
	if (mkdirhier(dirname(filename)) == -1) {
	_PROFILER_END("tar_extract_regfile hierdir", hierdir);
		return -1;
	}
	_PROFILER_END("tar_extract_regfile hierdir", hierdir);

#ifdef tarlib_ext_DEBUG
	printf("  ==> extracting: %s (mode %04o, uid %d, gid %d, %d bytes)\n",
	       filename, mode, uid, gid, size);
#endif
	_PROFILER_START("tar_extract_regfile open", fileopen);
	fdout = open(filename, O_WRONLY | O_CREAT | O_TRUNC
#ifdef O_BINARY
		     | O_BINARY
#endif
		    , 0666);
	_PROFILER_END("tar_extract_regfile open", fileopen);
	if (fdout == -1)
	{
#ifdef tarlib_ext_DEBUG
		perror("open()");
#endif
		return -1;
	}

#if 0
	/* change the owner.  (will only work if run as root) */
	if (fchown(fdout, uid, gid) == -1 && errno != EPERM)
	{
#ifdef tarlib_ext_DEBUG
		perror("fchown()");
#endif
		return -1;
	}

	/* make sure the mode isn't inheritted from a file we're overwriting */
	if (fchmod(fdout, mode & 07777) == -1)
	{
#ifdef tarlib_ext_DEBUG
		perror("fchmod()");
#endif
		return -1;
	}
#endif

	/* extract the file */
	_PROFILER_START("tar_extract_regfile readloop", readloop);
	for (i = size; i > 0; i -= T_BLOCKSIZE)
	{
	    _PROFILER_AUTO(read_loop_iter);

		k = tar_block_read(t, block_size_buf);
		if (k != T_BLOCKSIZE)
		{
			if (k != -1)
				errno = EINVAL;
			close(fdout);
			return -1;
		}

		/* write block to output file */
		if (write(fdout, block_size_buf, ((i > T_BLOCKSIZE) ? T_BLOCKSIZE : i)) == -1)
		{
			close(fdout);
			return -1;
		}
	}
	_PROFILER_END("tar_extract_regfile readloop", readloop);

	_PROFILER_START("tar_extract_regfile close", fileclose);
	
	/* close output file */
	if (close(fdout) == -1) {
		_PROFILER_END("tar_extract_regfile close", fileclose);
		return -1;
	}


	_PROFILER_END("tar_extract_regfile close", fileclose);

#ifdef tarlib_ext_DEBUG
	printf("### done extracting %s\n", filename);
#endif

	return 0;
}

static int
tar_set_file_perms(TAR *t, char *realname) {
	mode_t mode;
	uid_t uid;
	gid_t gid;
	struct utimbuf ut;
	char *filename;

	filename = (realname ? realname : th_get_pathname(t));
	mode = th_get_mode(t);
	uid = th_get_uid(t);
	gid = th_get_gid(t);
	ut.modtime = ut.actime = th_get_mtime(t);

	/* change owner/group */
	if (geteuid() == 0)
#ifdef HAVE_LCHOWN
		if (lchown(filename, uid, gid) == -1)
		{
# ifdef tarlib_ext_DEBUG
			fprintf(stderr, "lchown(\"%s\", %d, %d): %s\n",
				filename, uid, gid, strerror(errno));
# endif
#else /* ! HAVE_LCHOWN */
		if (!TH_ISSYM(t) && chown(filename, uid, gid) == -1)
		{
# ifdef tarlib_ext_DEBUG
			fprintf(stderr, "chown(\"%s\", %d, %d): %s\n",
				filename, uid, gid, strerror(errno));
# endif
#endif /* HAVE_LCHOWN */
			return -1;
		}

	/* change access/modification time */
	if (!TH_ISSYM(t) && utime(filename, &ut) == -1)
	{
#ifdef tarlib_ext_DEBUG
		perror("utime()");
#endif
		return -1;
	}

	/* change permissions */
	if (!TH_ISSYM(t) && chmod(filename, mode) == -1)
	{
#ifdef tarlib_ext_DEBUG
		perror("chmod()");
#endif
		return -1;
	}

	return 0;
}

int
th_read_internal(TAR *t)
{
	int i;
	int num_zero_blocks = 0;

#ifdef tarlib_ext_DEBUG
	printf("==> th_read_internal(TAR=\"%s\")\n", t->pathname);
#endif

	while ((i = tar_block_read(t, &(t->th_buf))) == T_BLOCKSIZE)
	{
		/* two all-zero blocks mark EOF */
		if (t->th_buf.name[0] == '\0')
		{
			num_zero_blocks++;
			if (!BIT_ISSET(t->options, TAR_IGNORE_EOT)
			    && num_zero_blocks >= 2)
				return 0;	/* EOF */
			else
				continue;
		}

		/* verify magic and version */
		if (BIT_ISSET(t->options, TAR_CHECK_MAGIC)
		    && strncmp(t->th_buf.magic, TMAGIC, TMAGLEN - 1) != 0)
		{
#ifdef tarlib_ext_DEBUG
			puts("!!! unknown magic value in tar header");
#endif
			return -2;
		}

		if (BIT_ISSET(t->options, TAR_CHECK_VERSION)
		    && strncmp(t->th_buf.version, TVERSION, TVERSLEN) != 0)
		{
#ifdef tarlib_ext_DEBUG
			puts("!!! unknown version value in tar header");
#endif
			return -2;
		}

		/* check chksum */
		if (!BIT_ISSET(t->options, TAR_IGNORE_CRC)
		    && !th_crc_ok(t))
		{
#ifdef tarlib_ext_DEBUG
			puts("!!! tar header checksum error");
#endif
			return -2;
		}

		break;
	}

#ifdef tarlib_ext_DEBUG
	printf("<== th_read_internal(): returning %d\n", i);
#endif
	return i;
}

int th_read_ext(TAR *t)
{
	int i;
	size_t sz, j, blocks;
	char *ptr;

#ifdef tarlib_ext_DEBUG
	printf("==> th_read(t=0x%lx)\n", t);
#endif

	if (t->th_buf.gnu_longname != NULL)
		free(t->th_buf.gnu_longname);
	if (t->th_buf.gnu_longlink != NULL)
		free(t->th_buf.gnu_longlink);
	memset(&(t->th_buf), 0, sizeof(struct tar_header));

	i = th_read_internal(t);
	if (i == 0)
		return 1;
	else if (i != T_BLOCKSIZE)
	{
		if (i != -1) {
            printf("th_read_internal failed\n");
			errno = EINVAL;
        }
		return -1;
	}

	/* check for GNU long link extention */
	if (TH_ISLONGLINK(t))
	{
		sz = th_get_size(t);
		blocks = (sz / T_BLOCKSIZE) + (sz % T_BLOCKSIZE ? 1 : 0);
		if (blocks > ((size_t)-1 / T_BLOCKSIZE))
		{
            printf("TH_ISLONGLINK > argument list too big\n");
			errno = E2BIG;
			return -1;
		}
#ifdef tarlib_ext_DEBUG
		printf("    th_read(): GNU long linkname detected "
		       "(%ld bytes, %d blocks)\n", sz, blocks);
#endif
		t->th_buf.gnu_longlink = (char *)malloc(blocks * T_BLOCKSIZE);
		if (t->th_buf.gnu_longlink == NULL)
			return -1;

		for (j = 0, ptr = t->th_buf.gnu_longlink; j < blocks;
		     j++, ptr += T_BLOCKSIZE)
		{
#ifdef tarlib_ext_DEBUG
			printf("    th_read(): reading long linkname "
			       "(%d blocks left, ptr == %ld)\n", blocks-j, ptr);
#endif
			i = tar_block_read(t, ptr);
			if (i != T_BLOCKSIZE)
			{
				if (i != -1) {
                    printf("TH_ISLONGLINK > tar_block_read <=> T_BLOCKSIZE\n");
					errno = EINVAL;
                }
				return -1;
			}
#ifdef tarlib_ext_DEBUG
			printf("    th_read(): read block == \"%s\"\n", ptr);
#endif
		}
#ifdef tarlib_ext_DEBUG
		printf("    th_read(): t->th_buf.gnu_longlink == \"%s\"\n",
		       t->th_buf.gnu_longlink);
#endif

		i = th_read_internal(t);
		if (i != T_BLOCKSIZE)
		{
			if (i != -1) {
                printf("th_read_internal != T_BLOCKSIZE\n");
				errno = EINVAL;
            }
			return -1;
		}
	}

	/* check for GNU long name extention */
	if (TH_ISLONGNAME(t))
	{
		sz = th_get_size(t);
		blocks = (sz / T_BLOCKSIZE) + (sz % T_BLOCKSIZE ? 1 : 0);
		if (blocks > ((size_t)-1 / T_BLOCKSIZE))
		{
            printf("TH_ISLONGNAME > toobig\n");
			errno = E2BIG;
			return -1;
		}
#ifdef tarlib_ext_DEBUG
		printf("    th_read(): GNU long filename detected "
		       "(%ld bytes, %d blocks)\n", sz, blocks);
#endif
		t->th_buf.gnu_longname = (char *)malloc(blocks * T_BLOCKSIZE);
		if (t->th_buf.gnu_longname == NULL)
			return -1;

		for (j = 0, ptr = t->th_buf.gnu_longname; j < blocks;
		     j++, ptr += T_BLOCKSIZE)
		{
#ifdef tarlib_ext_DEBUG
			printf("    th_read(): reading long filename "
			       "(%d blocks left, ptr == %ld)\n", blocks-j, ptr);
#endif
			i = tar_block_read(t, ptr);
			if (i != T_BLOCKSIZE)
			{
				if (i != -1) {
                    printf("TH_ISLONGNAME > tar_block_read <=> T_BLOCKSIZE\n");
					errno = EINVAL;
                }
				return -1;
			}
#ifdef tarlib_ext_DEBUG
			printf("    th_read(): read block == \"%s\"\n", ptr);
#endif
		}
#ifdef tarlib_ext_DEBUG
		printf("    th_read(): t->th_buf.gnu_longname == \"%s\"\n",
		       t->th_buf.gnu_longname);
#endif

		i = th_read_internal(t);
		if (i != T_BLOCKSIZE)
		{
			if (i != -1) {
                printf("TH_ISLONGNAME > th_read_internal != T_BLOCKSIZE\n");
				errno = EINVAL;
            }
			return -1;
		}
	}

	return 0;
}

int
tar_extract_file2(TAR *t, char *realname, char* block_size_buf)
{
    _PROFILER_AUTO();

	int i;
	char *lnp;
	int pathname_len;
	int realname_len;

	_PROFILER_START("tar_extract_file2 > TAR_NOOVERWRITE", TAR_NOOVERWRITE);
	if (t->options & TAR_NOOVERWRITE)
	{
		struct stat s;

		if (lstat(realname, &s) == 0 || errno != ENOENT)
		{
			errno = EEXIST;
			return -1;
		}
	}
	_PROFILER_END("tar_extract_file2 > TAR_NOOVERWRITE", TAR_NOOVERWRITE);

	_PROFILER_START("tar_extract_file2 > ifs", ifs);
	if (TH_ISREG(t)) {
		i = tar_extract_regfile2(t, realname, block_size_buf);
	}
	else if (TH_ISDIR(t))
	{
    	_PROFILER_START("tar_extract_file2 > tar_extract_dir", tar_extract_dir);
		i = tar_extract_dir(t, realname);
		if (i == 1)
			i = 0;
    	_PROFILER_END("tar_extract_file2 > tar_extract_dir", tar_extract_dir);
	}
	else if (TH_ISLNK(t))
		i = tar_extract_hardlink(t, realname);
	else if (TH_ISSYM(t))
		i = tar_extract_symlink(t, realname);
	else if (TH_ISCHR(t))
		i = tar_extract_chardev(t, realname);
	else if (TH_ISBLK(t))
		i = tar_extract_blockdev(t, realname);
	else if (TH_ISFIFO(t))
		i = tar_extract_fifo(t, realname);
	else {
		printf("libtar unknown file type\n");
		return -1;
	}
	_PROFILER_END("tar_extract_file2 > ifs", ifs);

	if (i != 0)
		return i;

    // _PROFILER_START("tar_extract_file2 > tar_set_file_perms", tar_set_file_perms);
	// i = tar_set_file_perms(t, realname);
	// if (i != 0)
	// 	return i;
    // _PROFILER_END("tar_extract_file2 > tar_set_file_perms", tar_set_file_perms);

    _PROFILER_START("tar_extract_file2 > calloc", calloc);
	pathname_len = strlen(th_get_pathname(t)) + 1;
	realname_len = strlen(realname) + 1;
	lnp = (char *)calloc(1, pathname_len + realname_len);
	if (lnp == NULL)
		return -1;
	strcpy(&lnp[0], th_get_pathname(t));
	strcpy(&lnp[pathname_len], realname);
#ifdef tarlib_ext_DEBUG
	printf("tar_extract_file(): calling libtar_hash_add(): key=\"%s\", "
	       "value=\"%s\"\n", th_get_pathname(t), realname);
#endif
	if (libtar_hash_add(t->h, lnp) != 0)
		return -1;

    _PROFILER_END("tar_extract_file2 > calloc", calloc);

	return 0;
}