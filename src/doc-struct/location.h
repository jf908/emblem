#pragma once

#include "data/str.h"
#include "ext/ext-env.h"
#include <stddef.h>

typedef struct
{
	size_t first_line;
	size_t first_column;
	size_t last_line;
	size_t last_column;
	Str* src_file;
} Location;

Location* dup_loc(Location* todup);
void set_ext_location_globals(ExtensionState* s);
