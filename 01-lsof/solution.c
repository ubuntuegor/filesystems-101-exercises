#include "solution.h"

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <ctype.h>
#include <sys/stat.h>
#include <fcntl.h>

void print_symlink(const char* path) {
	static char buf[PATH_MAX + 1];

	ssize_t written_size = readlink(path, buf, sizeof(buf));
	if (written_size < 0) {
		report_error(path, errno);
		return;
	}
	buf[written_size] = '\0';

	report_file(buf);
}

void lsof(void)
{
	char path_buf[1024];

	DIR* proc_dir = opendir("/proc");

	if (proc_dir == NULL) {
		report_error("/proc", errno);
		exit(EXIT_FAILURE);
	}

	struct dirent* proc_entry;
	while ((proc_entry = readdir(proc_dir)) ) {
		const char* dir_name = proc_entry->d_name;

		if (!isdigit(dir_name[0])) {
			continue;
		}

		snprintf(path_buf, sizeof(path_buf), "/proc/%s/cwd", dir_name);
		print_symlink(path_buf);

		snprintf(path_buf, sizeof(path_buf), "/proc/%s/root", dir_name);
		print_symlink(path_buf);

		snprintf(path_buf, sizeof(path_buf), "/proc/%s/exe", dir_name);
		print_symlink(path_buf);

		snprintf(path_buf, sizeof(path_buf), "/proc/%s/fd", dir_name);
		DIR* fd_dir = opendir(path_buf);

		if (fd_dir == NULL) {
			report_error(path_buf, errno);
			continue;
		}

		struct dirent* fd_entry;
		while ((fd_entry = readdir(fd_dir)) ) {
			if (!isdigit(fd_entry->d_name[0])) {
				continue;
			}

			snprintf(path_buf, sizeof(path_buf), "/proc/%s/fd/%s", dir_name, fd_entry->d_name);
			print_symlink(path_buf);
		}

		closedir(fd_dir);
	}

	closedir(proc_dir);
}
