#include "hash.h"

#include "pp/assert.h"
#include "pp/ignore_warning.h"
#include "str.h"
#include <limits.h>

#define HASH_NUM(name, type)                                                                                           \
	HASH_SIG(name)                                                                                                     \
	{                                                                                                                  \
		unsigned long int hash = 0xdcba0987654321;                                                                     \
		for (unsigned int i = 0; i < CHAR_BIT * sizeof(Hash); i++)                                                     \
		{                                                                                                              \
			POINTER_TO_INT_CAST(hash ^= (type)v << i);                                                                 \
		}                                                                                                              \
		return hash;                                                                                                   \
	}

HASH_NUM(char, char)
HASH_NUM(int, int)
HASH_NUM(size_t, size_t)

HASH_SIG(ptr)
{
	ASSERT(sizeof(void*) == sizeof(size_t));
	return hash_size_t(v);
}

HASH_SIG(str)
{
	// The djb2 algorithm
	Hash h = 5381;
	char c;
	char* str = ((Str*)v)->str;
	while ((c = *str++))
		h = ((h << 5) + h) ^ c;

	return h;
}
