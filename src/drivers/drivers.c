/**
 * @file drivers.c
 * @brief Handles the execution of output drivers both from the core and in extensions
 * @author Edward Jones
 * @date 2021-09-17
 */
#include "drivers.h"

#include "data/cmp.h"
#include "data/list.h"
#include "ext/lua-ast-io.h"
#include "logs/logs.h"
#include "pp/lambda.h"
#include "pp/unused.h"
#include "write-out.h"
#include <lauxlib.h>
#include <lua.h>
#include <stdio.h>
#include <string.h>

#define OUTPUT_DRIVER_RIDX "emblem_output_driver"
#define STYLESHEET_HEADER                                                                                              \
	"/*\n"                                                                                                             \
	" * This file was generated by `em` on %s.\n"                                                                      \
	" * Any changes will be overwritten next time typesetting is run.\n"                                               \
	" */\n"
#define TIME_STR_MAX_SUPPORTED_SIZE 26

static int init_core_output_driver(OutputDriver* driver, InternalDriver* idriver, Args* args);
static int init_extension_output_driver(OutputDriver* driver, Args* args, ExtensionEnv* ext);
static int run_extension_driver(OutputDriver* driver, Doc* doc, ExtensionEnv* ext, Str* time_str);
static void compute_output_stem(Str* output_stem, Args* args);
static void strip_ext(char* fname);
static void get_time_str(Str* time_str);
static int output_stylesheet(Str* stem, Str* time_str, List* css_snippets);

InternalDriver internal_drivers[] = {
	{ "pdf", 0, false, NULL },
};

int get_output_driver(OutputDriver* driver, Args* args, ExtensionEnv* ext)
{
	log_info("Loading driver '%s'", args->driver);

	for (size_t i = 0; i < sizeof(internal_drivers) / sizeof(*internal_drivers); i++)
		if (streq(internal_drivers[i].name, args->driver) && internal_drivers[i].run)
			return init_core_output_driver(driver, &internal_drivers[i], args);

	return init_extension_output_driver(driver, args, ext);
}

static int init_core_output_driver(OutputDriver* driver, InternalDriver* idriver, Args* args)
{
	driver->support				= idriver->support;
	driver->use_stdout			= streq(args->output_stem, "-");
	driver->output_stem			= malloc(sizeof(Str));
	driver->run					= idriver->run;
	driver->requires_stylesheet = idriver->requires_stylesheet;
	compute_output_stem(driver->output_stem, args);

	return 0;
}

static int init_extension_output_driver(OutputDriver* driver, Args* args, ExtensionEnv* ext)
{
	driver->use_stdout	= streq(args->output_stem, "-");
	driver->output_stem = malloc(sizeof(Str));
	driver->run			= run_extension_driver;
	compute_output_stem(driver->output_stem, args);

	ExtensionState* s = ext->state;
	lua_getglobal(s, "require");
	lua_pushstring(s, "std.out.drivers");
	if (lua_pcall(s, 1, 1, 0))
	{
		log_err("Failed to require driver file: %s", lua_tostring(s, -1));
		return 1;
	}
	lua_getfield(s, -1, "set_output_driver");
	lua_pushstring(s, args->driver);
	if (lua_pcall(s, 1, 0, 0))
	{
		log_err("Failed to set extension output driver '%s': %s", args->driver, lua_tostring(s, -1));
		return 1;
	}

	lua_getfield(s, -1, "get_output_driver");
	if (lua_pcall(s, 0, 1, 0))
	{
		log_err("Failed to get extension output driver '%s': %s", args->driver, lua_tostring(s, -1));
		return 1;
	}
	lua_getfield(s, -1, "support");
	if (!lua_isinteger(s, -1))
	{
		log_err("Support field is not an integer, instead got a %s", luaL_typename(s, -1));
		return 1;
	}
	driver->support = lua_tointeger(s, -1);
	lua_pop(s, 1);

	lua_getfield(s, -1, "requires_stylesheet");
	driver->requires_stylesheet = lua_toboolean(s, -1);
	lua_pop(s, 1);

	lua_setfield(s, LUA_REGISTRYINDEX, OUTPUT_DRIVER_RIDX);
	lua_pop(s, 1);
	return 0;
}

int run_output_driver(OutputDriver* driver, Doc* doc, ExtensionEnv* ext)
{
	Str time_str;
	get_time_str(&time_str);

	if (driver->requires_stylesheet)
	{
		int rc = output_stylesheet(driver->output_stem, &time_str, doc->styler->snippets);
		if (rc)
			return rc;
	}
	int rc = driver->run(driver, doc, ext, &time_str);

	dest_str(&time_str);
	return rc;
}

static int run_extension_driver(OutputDriver* driver, Doc* doc, ExtensionEnv* e, Str* time_str)
{
	ExtensionState* s = e->state;
	lua_getfield(s, LUA_REGISTRYINDEX, OUTPUT_DRIVER_RIDX);
	lua_getfield(s, -1, "output");
	lua_rotate(s, -2, 1);
	pack_tree(s, doc->root);
	lua_pushboolean(s, driver->use_stdout);
	lua_pushlstring(s, driver->output_stem->str, driver->output_stem->len);
	lua_pushlstring(s, time_str->str, time_str->len);
	if (lua_pcall(s, 5, 0, 0))
	{
		log_err("Extension output driver failed: %s", lua_tostring(s, -1));
		return 1;
	}
	return 0;
}

void dest_output_driver(OutputDriver* driver) { dest_free_str(driver->output_stem); }

static void compute_output_stem(Str* output_stem, Args* args)
{
	if (streq(args->output_stem, "-"))
		make_strv(output_stem, "emdoc");
	else if (!streq(args->output_stem, ""))
		make_strv(output_stem, args->output_stem);
	else if (streq(args->input_file, "-"))
		make_strv(output_stem, "emdoc");
	else
	{
		// Remove extension from the input and use that as the default
		char* inoext = strdup(args->input_file);
		strip_ext(inoext);
		make_strr(output_stem, inoext);
	}
}

static void strip_ext(char* fname)
{
	char* end = fname + strlen(fname);
	while (end > fname && *end != '.' && *end != '/' && *end != '\\')
		end--;
	if (end > fname && *end == '.' && *(end - 1) != '/' && *(end - 1) != '\\')
		*end = '\0';
}

static int output_stylesheet(Str* stem, Str* time_str, List* css_snippets)
{
	log_info("Preparing stylesheet for output");

	// Add the header to the css
	size_t header_len	 = 1 + snprintf(NULL, 0, STYLESHEET_HEADER, time_str->str);
	char* header_content = malloc(header_len);
	snprintf(header_content, header_len, STYLESHEET_HEADER, time_str->str);
	Str* header_str = malloc(sizeof(Str));
	make_strr(header_str, header_content);
	prepend_list(css_snippets, header_str);

	Str fmt;
	make_strv(&fmt, "%s.css");
	int rc = write_output(&fmt, stem, false, css_snippets);
	dest_str(&fmt);
	return rc;
}

static void get_time_str(Str* time_str)
{
	time_t curr_time;
	time(&curr_time);
	char* time_buf		 = malloc(TIME_STR_MAX_SUPPORTED_SIZE * sizeof(char));
	struct tm* time_info = localtime(&curr_time);
	asctime_r(time_info, time_buf);
	time_buf[strlen(time_buf) - 1] = '\0'; // Strip trailing newline

	make_strr(time_str, time_buf);
}
