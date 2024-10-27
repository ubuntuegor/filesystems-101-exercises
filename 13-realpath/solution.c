#include "solution.h"

#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <limits.h>
#include <unistd.h>
#include <fs_malloc.h>
#include <fs_string.h>

static struct {
	char* path;
	size_t capacity;
} state;

static void reset_path() {
	state.path[0] = '\0';
}

static void append_dir() {
	size_t path_len = strlen(state.path);
	while (path_len + 2 > state.capacity) {
		state.capacity *= 2;
		state.path = fs_xrealloc(state.path, state.capacity);
	}
	strcpy(state.path + path_len, "/");
}

static void append_child(const char* child) {
	size_t path_len = strlen(state.path);
	size_t child_len = strlen(child);
	while (path_len + child_len + 1 > state.capacity) {
		state.capacity *= 2;
		state.path = fs_xrealloc(state.path, state.capacity);
	}
	strcpy(state.path + path_len, child);
}

static void go_back() {
	char* last_dir = strrchr(state.path, '/');
	*(last_dir) = '\0';
}

static char is_directory(const char *path) {
    struct stat path_stat;

	char* last_dir = strrchr(path, '/');
	size_t parent_size = last_dir - path;
	char* parent = fs_xmalloc(parent_size + 1);
	parent[0] = '\0';
	strncat(parent, path, parent_size);

    if (stat(path, &path_stat) == -1) {
        report_error(parent, last_dir + 1, errno);
		fs_xfree(parent);
        return -1;
    }

	fs_xfree(parent);
    return S_ISDIR(path_stat.st_mode);
}

static char is_link(const char* child) {
	struct stat path_stat;

	char* path = fs_xasprintf("%s/%s", state.path, child);

    if (lstat(path, &path_stat) < 0) {
        report_error(state.path, child, errno);
		fs_xfree(path);
        return -1;
    }

	fs_xfree(path);

    return S_ISLNK(path_stat.st_mode);
}

static char* read_link(const char* child) {
	static char buf[PATH_MAX];

	char* path = fs_xasprintf("%s/%s", state.path, child);

	ssize_t size = readlink(path, buf, PATH_MAX);
    if (size < 0) {
        report_error(state.path, child, errno);
		fs_xfree(path);
        return NULL;
    }

	fs_xfree(path);

	char* result = fs_xmalloc(size + 1);
	memcpy(result, buf, size);
	result[size] = '\0';

    return result;
}

static char walkpath(const char* path) {
	const char* path_end = path + strlen(path);

	if (path[0] == '/') {
		reset_path();
		path++;
	}

	while (path < path_end) {
		const char* next_path = strchrnul(path, '/');
		size_t name_len = next_path - path;
		char* child = fs_xmalloc(name_len + 1);
		child[0] = '\0';
		strncat(child, path, name_len);

		if (name_len == 0 || strncmp(path, ".", name_len) == 0) {
			path = next_path + 1;
			fs_xfree(child);
			continue;
		} else if (strncmp(path, "..", name_len) == 0) {
			go_back();
			path = next_path + 1;
			fs_xfree(child);
			continue;
		}

		char is_link_result = is_link(child);

		if (is_link_result < 0) {
			fs_xfree(child);
			return -1;
		}

		if (is_link_result) {
			char* link = read_link(child);
			if (link == NULL) {
				fs_xfree(child);
				return -1;
			}

			if (walkpath(link) < 0) {
				fs_xfree(child);
				fs_xfree(link);
				return -1;
			}
			fs_xfree(link);
		} else {
			append_dir();
			append_child(child);
		}

		fs_xfree(child);
		path = next_path + 1;
	}

	return 0;
}

void abspath(const char *path)
{
	state.capacity = 4;
	state.path = fs_xmalloc(state.capacity);
	reset_path();
	if (walkpath(path) < 0) {
		fs_xfree(state.path);
		return;
	}

	char is_directory_result = is_directory(path);
	if (is_directory_result < 0) {
		fs_xfree(state.path);
		return;
	}

	if (is_directory_result) {
		append_dir();
	}

	report_path(state.path);

	fs_xfree(state.path);
}
