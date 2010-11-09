#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "vector.h"

static int grow(struct vector *vec, int gently)
{
	size_t new_alloc;
	void *new_data;

	new_alloc = vec->alloc * 3 / 2;
	if (!new_alloc)
		new_alloc = 8;
	new_data = realloc(vec->data, new_alloc * vec->size);
	if (!new_data) {
		if (gently)
			return ENOMEM;
		perror("vector.c:grow()");
		exit(1);
	}
	vec->data = new_data;
	vec->alloc = new_alloc;
	return 0;
}

int vector_push(struct vector *vec, const void *data, int gently)
{
	int rc;

	if (vec->count == vec->alloc && (rc = grow(vec, gently)))
		return rc;
	if (data)
		memmove(vec->data + vec->count * vec->size, data, vec->size);
	else
		memset(vec->data + vec->count * vec->size, 0, vec->size);
	vec->count++;
	return 0;
}
