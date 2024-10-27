#include "solution.h"

#include <fuse.h>
#include <errno.h>
#include <string.h>
#include <fs_malloc.h>
#include <fs_string.h>

#define MY_FILENAME "hello"

static int hello_getattr(const char *path, struct stat *stbuf,
						 struct fuse_file_info *fi)
{
	(void)fi;
	int res = 0;

	memset(stbuf, 0, sizeof(struct stat));
	if (strcmp(path, "/") == 0)
	{
		stbuf->st_mode = S_IFDIR | 0775;
		stbuf->st_nlink = 2;
	}
	else if (strcmp(path + 1, MY_FILENAME) == 0)
	{
		stbuf->st_mode = S_IFREG | 0400;
		stbuf->st_nlink = 1;
		stbuf->st_size = 69;
	}
	else
		res = -ENOENT;

	return res;
}

static int hello_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
						 off_t offset, struct fuse_file_info *fi,
						 enum fuse_readdir_flags flags)
{
	(void)offset;
	(void)fi;
	(void)flags;

	if (strcmp(path, "/") != 0)
		return -ENOENT;

	filler(buf, ".", NULL, 0, 0);
	filler(buf, "..", NULL, 0, 0);
	filler(buf, MY_FILENAME, NULL, 0, 0);

	return 0;
}

static int hello_open(const char *path, struct fuse_file_info *fi)
{
	if (strcmp(path + 1, MY_FILENAME) != 0)
		return -ENOENT;

	if ((fi->flags & O_ACCMODE) != O_RDONLY)
		return -EROFS;

	return 0;
}

static int hello_read(const char *path, char *buf, size_t size, off_t offset,
					  struct fuse_file_info *fi)
{
	(void)fi;
	if (strcmp(path + 1, MY_FILENAME) != 0)
		return -ENOENT;

	struct fuse_context *context = fuse_get_context();
	const char *contents = fs_xasprintf("hello, %d\n", context->pid);

	off_t len = strlen(contents);
	if (offset < len)
	{
		if (offset + (off_t) size > len)
			size = len - offset;
		memcpy(buf, contents + offset, size);
	}
	else
		size = 0;

	fs_xfree((void*) contents);

	return size;
}

static int hello_mknod(const char *, mode_t, dev_t) {
	return -EROFS;
}

static int hello_mkdir(const char *, mode_t) {
	return -EROFS;
}

static int hello_link(const char *, const char *) {
	return -EROFS;
}

static int hello_symlink(const char *, const char *) {
	return -EROFS;
}

static int hello_rename(const char *, const char *, unsigned int flags) {
	(void) flags;
	return -EROFS;
}

static const struct fuse_operations hellofs_ops = {
	.getattr = hello_getattr,
	.readdir = hello_readdir,
	.open = hello_open,
	.read = hello_read,
	.mknod = hello_mknod,
	.mkdir = hello_mkdir,
	.link = hello_link,
	.symlink = hello_symlink,
	.rename = hello_rename,
};

int helloworld(const char *mntp)
{
	char *argv[] = {"exercise", "-f", (char *)mntp, NULL};
	return fuse_main(3, argv, &hellofs_ops, NULL);
}
