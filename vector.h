#ifndef CGIT_VECTOR_H
#define CGIT_VECTOR_H

#include <stdlib.h>

struct vector {
	size_t size;
	size_t count;
	size_t alloc;
	void *data;
};

#define VECTOR_INIT(type) {sizeof(type), 0, 0, NULL}

int vector_push(struct vector *vec, const void *data, int gently);

#endif /* CGIT_VECTOR_H */
