#include "cgit.h"

void cgit_print_blob(struct cacheitem *item, const char *hex, char *path)
{

	unsigned char sha1[20];
	enum object_type type;
	unsigned char *buf;
	unsigned long size;

	if (get_sha1_hex(hex, sha1)){
		cgit_print_error(fmt("Bad hex value: %s", hex));
	        return;
	}

	type = sha1_object_info(sha1, &size);
	if (type == OBJ_BAD) {
		cgit_print_error(fmt("Bad object name: %s", hex));
		return;
	}

	buf = read_sha1_file(sha1, &type, &size);
	if (!buf) {
		cgit_print_error(fmt("Error reading object %s", hex));
		return;
	}

	buf[size] = '\0';
	cgit_print_snapshot_start("text/plain", path, item);
	write(htmlfd, buf, size);
}
