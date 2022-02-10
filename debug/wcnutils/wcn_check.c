#include <stdio.h>
#include <sched.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>

#include <linux/kernel.h>
#include <linux/magic.h>

#include <sys/stat.h>
#include <sys/mount.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <sys/vfs.h>
#include <sys/mman.h>

/* #define DEBUG */
#ifdef DEBUG
#define DEBUGSEE fprintf
#else
#define DEBUGSEE(...)
#endif

/* option flags */
#define LOOKAT_F_GET (1 << 0)
#define LOOKAT_F_GET_MUL (1 << 1)
#define LOOKAT_F_SET (1 << 2)
#define LOOKAT_F_INNER (1 << 3)

static int debugfs_found = 0;
char debugfs_mountpoint[4096 + 1] = "/sys/kernel/debug";
static int debugfs_premounted;

#define SIZE_BUF   24
static const char *debugfs_known_mountpoints[] = {
    "/sys/kernel/debug/", "/debug/", "/d/", 0,
};

struct wcn_reg_ctl {
	unsigned int addr;
	unsigned int len;
	/* 1: rw_extended; 0:rw_direct */
	unsigned int rw_extended;
	/* make sure sdio critical buf >512,
	 * but the frame size is larger than 2048
	 * bytes is not permission in kernel to android
	 */
	unsigned int value[256];
};

/* verify that a mountpoint is actually a debugfs instance */
static int debugfs_valid_mountpoint(const char *debugfs) {
  struct statfs st_fs;

  if (statfs(debugfs, &st_fs) < 0)
    return -2;
  else if (st_fs.f_type != (long)DEBUGFS_MAGIC)
    return -2;

  return 0;
}

/* find the path to the mounted debugfs */
static const char *debugfs_find_mountpoint(void) {
  const char **ptr;
  char type[100];
  FILE *fp;

  if (debugfs_found) return (const char *)debugfs_mountpoint;

  ptr = debugfs_known_mountpoints;
  while (*ptr) {
    if (debugfs_valid_mountpoint(*ptr) == 0) {
      debugfs_found = 1;
      strcpy(debugfs_mountpoint, *ptr);
      return debugfs_mountpoint;
    }
    ptr++;
  }

  /* give up and parse /proc/mounts */
  fp = fopen("/proc/mounts", "r");
  if (fp == NULL) return NULL;

  while (fscanf(fp, "%*s %4096s %99s %*s %*d %*d\n", debugfs_mountpoint,
                type) == 2) {
    if (strcmp(type, "debugfs") == 0) break;
  }
  fclose(fp);

  if (strcmp(type, "debugfs") != 0) return NULL;

  debugfs_found = 1;

  return debugfs_mountpoint;
}

/* mount the debugfs somewhere if it's not mounted */
static char *debugfs_mount(const char *mountpoint) {
  struct statfs st_fs;
  /* see if it's already mounted */
  if (debugfs_find_mountpoint()) {
    debugfs_premounted = 1;
    goto out;
  }
  /* if not mounted and no argument */
  if (mountpoint == NULL) {
    /* see if environment variable set */
    /* if no environment variable, use default */
    mountpoint = "/sys/kernel/debug/";
  }

  if (statfs("/debug_12345", &st_fs) < 0) {
    if (mkdir("/debug_12345/", 0644) < 0) return NULL;
  }
  if (mount(NULL, mountpoint, "debugfs", 0, NULL) < 0) return NULL;

  /* save the mountpoint */
  debugfs_found = 1;
  strncpy(debugfs_mountpoint, mountpoint, sizeof(debugfs_mountpoint));
out:

  return debugfs_mountpoint;
}

static int find_create_debugfs(char **p) {
  char *_p = 0;
  char *debugfs_dir = debugfs_mount("/debug_12345/");
  if (!debugfs_dir) return -1;
  _p = debugfs_dir;
  while (*_p) _p++;
  if (*(_p - 1) != '/') {
    *(_p) = '/';
    *(_p + 1) = 0;
  }

  DEBUGSEE("debugfs_dir = %s\n", debugfs_dir);
  *p = debugfs_dir;

  return 0;
}
int sub_main(int flags, unsigned int addr, unsigned int value, unsigned int nword) {
    int fd;
    char *dir = 0;
    char strPath1[256] = {0};
    ssize_t cnt = 0;
	unsigned int i;
    struct wcn_reg_ctl reg_ctl;

    if (find_create_debugfs(&dir)) return 0;

    sprintf(strPath1, "%s%s", dir, "wcn/regctl");

    if ((fd = open(strPath1, O_RDWR | O_SYNC)) < 0) {
        perror("open wcn/regctl");
        exit(EXIT_FAILURE);
        return -1;
    }

    printf("open wcn/regctl OK !!!");

	if (flags & LOOKAT_F_INNER)
		reg_ctl.rw_extended =0;
	else
        reg_ctl.rw_extended =1;
    reg_ctl.addr = addr;
    reg_ctl.len = nword;
    reg_ctl.value[0] = value;
    if (reg_ctl.len == 0)
		reg_ctl.len =1;
// write register
    if ((flags & LOOKAT_F_SET)) {
		cnt = write(fd, &reg_ctl, sizeof(reg_ctl));
		printf("write ret cnt = %zd\n", cnt);
		if (cnt < 0) {
			perror("write wcn/regctl\n");
			close(fd);
			return -1;
		}
		printf("write reg ok!\n");
		close(fd);
		return 0;
    }
// read register
    cnt = read(fd, &reg_ctl, sizeof(reg_ctl));
    printf("read ret cnt = %zd\n", cnt);
    fprintf(stdout,
            "  ADDRESS  |   VALUE\n"
            "-----------+-----------\n");
    for (i = 0; i < reg_ctl.len; i++) {
		printf("0x%x | 0x%x\n",reg_ctl.addr+i*4, reg_ctl.value[i]);
    }

    close(fd);
    return 0;
}

void usage1(void) {
  fprintf(stderr,
          "\n"
          "Usage: wcn_check [-l nword] [-s value] [-h] [-a inner] addr_in_hex\n"

          "  -l nword     print values of nword continous regs start from \n"
          "               addr_in_hex in a formated way\n"
          "  -s value     set a reg at addr_in_hex to value\n"
          "  -a           read and write SDIO inner register\n"
          "  -h           print this message\n\n"
          " NOTE:\n"
          " * read extended register:   wcn_check addr\n"
          " * read extended register:   wcn_check -l len addr \n"
          " * write extended register:  wcn_check -s value addr \n"
          " * read direct register:   wcn_check addr\n"
          " * read direct register:   wcn_check -a -l len addr \n"
          " * write direct register:  wcn_check -a -s value addr \n");
}

int main(int argc, char **argv) {
  int opt;
  int flags = 0; /* option indicator */
  unsigned int paddr;
  unsigned int value = 0, nword = 0;
  char *op = "as:l:h";
  void (*usage)(void);

  usage = usage1;
  while ((opt = getopt(argc, argv, op)) != -1) {
    switch (opt) {
      case 'a':
        flags |= LOOKAT_F_INNER;
        //value = strtoul(optarg, NULL, 16);
        fprintf(stdout, "[I] WR SDIO Inner register\n");
        break;
      case 's':
        flags |= LOOKAT_F_SET;
        value = strtoul(optarg, NULL, 16);
        fprintf(stdout, "[I] value = 0x%x\n", value);
        break;
      case 'l':
        flags |= LOOKAT_F_GET_MUL;
        nword = strtoul(optarg, NULL, 10);
        fprintf(stdout, "[I] nword = 0x%d\n", nword);
        if (nword <= 0) nword = 1;
        break;
      case 'h':
        usage();
		break;
        //exit(0);
      default:
          usage();
		  break;
          //exit(EXIT_FAILURE);
    }
  }

  if (optind == 1) flags |= LOOKAT_F_GET;

  if (optind >= argc) {
    fprintf(stderr, "Expected argument after options\n");
    exit(EXIT_FAILURE);
  }

  paddr = strtoul(argv[optind], NULL, 16);
  DEBUGSEE(stdout, "[I] paddr = 0x%lx\n", paddr);

    if ((flags & LOOKAT_F_INNER)) {
		goto  action;
    }

  if (paddr & 0x3) {
    fprintf(stderr, "address should be 4-byte aligned\n");
    exit(EXIT_FAILURE);
  }

action:
    fprintf(stderr, "flags=%d, paddr=0x%x, value= 0x%x, nword =%d\n", flags,paddr ,value, nword);
    sub_main(flags, paddr, value, nword);

  return 0;
}
