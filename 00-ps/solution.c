#include "solution.h"
#include <dirent.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>

void ps(void)
{
	char path_buf[256];
	char exe_buf[PATH_MAX + 1];

	DIR* proc_ptr = opendir("/proc");
	if (proc_ptr == NULL) {
		report_error("/proc", errno);
		exit(EXIT_FAILURE);
	}

	char* argv_read = malloc(4 * 1024 * 1024);
	char* envp_read = malloc(4 * 1024 * 1024);
	char** argv_buf = malloc(4 * 1024 * 1024);
	char** envp_buf = malloc(4 * 1024 * 1024);

	struct dirent* pid_ptr;
	while ((pid_ptr = readdir(proc_ptr))) {
		if (pid_ptr->d_name[0] < '0' || pid_ptr->d_name[0] > '9') {
			continue;
		}

		pid_t pid = atol(pid_ptr->d_name);

		snprintf(path_buf, sizeof(path_buf), "/proc/%d", pid);
		int pid_dir = open(path_buf, O_RDONLY | O_DIRECTORY);
		if (pid_dir < 0) {
			report_error(path_buf, errno);
			continue;
		}

		int exe_len = readlinkat(pid_dir, "exe", exe_buf, sizeof(exe_buf));
		if (exe_len < 0) {
			snprintf(path_buf, sizeof(path_buf), "/proc/%d/exe", pid);
			report_error(path_buf, errno);
			goto close;
		}

		exe_buf[exe_len] = '\0';

		int argv_fd = openat(pid_dir, "cmdline", O_RDONLY);
		if (argv_fd < 0) {
			snprintf(path_buf, sizeof(path_buf), "/proc/%d/cmdline", pid);
			report_error(path_buf, errno);
			goto close;
		}

		int bytes_read_total = 0;
		int bytes_read;

		while ((bytes_read = read(argv_fd, argv_read + bytes_read_total, 4 * 1024 * 1024 - bytes_read_total))) {
			if (bytes_read < 0) {
				goto close_argv;
			}
			bytes_read_total += bytes_read;
		}

		argv_read[bytes_read_total] = '\0';

		if (close(argv_fd) < 0) {
close_argv:
			snprintf(path_buf, sizeof(path_buf), "/proc/%d/cmdline", pid);
			report_error(path_buf, errno);
			goto close;
		}

		int found = 0;
		char* ptr = argv_read;
		while (1) {
			if (ptr > argv_read + bytes_read_total) {
				argv_buf[found] = NULL;
				break;
			}
			size_t next = strlen(ptr);
			if (next == 0) {
				argv_buf[found] = NULL;
				break;
			}
			argv_buf[found++] = ptr;
			ptr += next + 1;
		}


		int envp_fd = openat(pid_dir, "environ", O_RDONLY);
		if (envp_fd < 0) {
			snprintf(path_buf, sizeof(path_buf), "/proc/%d/environ", pid);
			report_error(path_buf, errno);
			goto close;
		}

		bytes_read_total = 0;

		while ((bytes_read = read(envp_fd, envp_read + bytes_read_total, 4 * 1024 * 1024 - bytes_read_total))) {
			if (bytes_read < 0) {
				goto close_envp;
			}
			bytes_read_total += bytes_read;
		}

		envp_read[bytes_read_total] = '\0';

		if (close(envp_fd) < 0) {
close_envp:
			snprintf(path_buf, sizeof(path_buf), "/proc/%d/environ", pid);
			report_error(path_buf, errno);
			goto close;
		}

		found = 0;
		ptr = envp_read;
		while (1) {
			if (ptr > envp_read + bytes_read_total) {
				envp_buf[found] = NULL;
				break;
			}
			size_t next = strlen(ptr);
			if (next == 0) {
				envp_buf[found] = NULL;
				break;
			}
			envp_buf[found++] = ptr;
			ptr += next + 1;
		}


		report_process(pid, exe_buf, argv_buf, envp_buf);

close:
		if (close(pid_dir) < 0) {
			snprintf(path_buf, sizeof(path_buf), "/proc/%d", pid);
			report_error(path_buf, errno);
			continue;
		}
		errno = 0;
	}

	free(argv_read);
	free(argv_buf);
	free(envp_read);
	free(envp_buf);

	if (errno != 0) {
		report_error("/proc", errno);
		exit(EXIT_FAILURE);
	}

	if (closedir(proc_ptr) < 0) {
		report_error("/proc", errno);
		exit(EXIT_FAILURE);
	}
}
