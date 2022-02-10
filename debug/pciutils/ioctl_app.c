/*
 * Copyright (C) 2016 Spreadtrum Communications Inc.
 *
 * Authors	: jinglong.chen
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>

typedef struct {
	unsigned short t;
	unsigned short l;
	unsigned char v[0];
} tlv_t;

int hexdump(char *name, char *buf, int len)
{
	int i, count;
	unsigned int *p;
	count = len / 32;
	count += 1;
	printf("hexdump %s hex(len=%d):\n", name, len);
	for (i = 0; i < count; i++) {
		p = (unsigned int *)(buf + i * 32);
		printf
		    ("mem[0x%04x] 0x%08x,0x%08x,0x%08x,0x%08x,0x%08x,0x%08x,0x%08x,0x%08x,\n",
		     i * 32, p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7]);
	}
	return 0;
}

unsigned char input[4096] = { 0 };

int main(int argc, char *argv[])
{
	int ret, i, fd = 0;
	unsigned char *p;
	tlv_t *tlv;

	p = input;
	for (i = 1; i < argc; i++) {
		p += sprintf(p, " %s", argv[i]);
	}
	printf("input:%s\n", input);

	fd = open("/dev/kchar", O_RDWR);
	if (0 == fd) {
		printf("open err\n");
		return -1;
	}
	ret = ioctl(fd, 1, input);
	if (ret < 0) {
		printf("ioctl ret %d err\n", ret);
		close(fd);
		return -1;
	}
	tlv = input;
	switch (tlv->t) {
	case 0:
		printf("replay(%d,%d):%s\n", tlv->t, tlv->l, tlv->v);
		break;
	case 1:
		hexdump("replay", tlv->v, tlv->l);
		break;
	default:
		break;
	}
	close(fd);
	return 0;
}
