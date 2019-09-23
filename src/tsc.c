#include <string.h>
#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>	
#include <dirent.h>
#include <inttypes.h>
#include <sys/time.h>
#include <time.h>

#include "mujs.h"
#include "nozip.h"

#if defined(_WIN32)
#define OS_PLATFORM "win32"
#elif defined(__APPLE__)
#define OS_PLATFORM "darwin"
#else
#define OS_PLATFORM "linux"
#endif

extern unsigned char __pak_data_img[];
extern unsigned int __pak_data_img_length;

#define MANIFEST_FILE "manifest.json"

static int64_t msfromtime(const struct timespec *tv)
{
	return (int64_t)tv->tv_sec * 1000 + (tv->tv_nsec / 1000000);
}

static void timefromms(struct timeval *tv, uint64_t v)
{
	tv->tv_sec = v / 1000;
	tv->tv_usec = (v % 1000) * 1000;
}

#define geterrstr(n) (n == 0 ? "" : strerror(n))

char* extract_file(nozip_t *zip, const char *name, size_t *size) 
{
	char *output;
	nozip_entry_t *entry = nozip_locate(zip, name);
	if (!entry)
		return NULL; // TODO: error message?
	output = malloc(entry->uncompressed_size);
	if (!output)
		return NULL;
	if (!output)
		return NULL;
	if (nozip_uncompress(zip, entry, output, entry->uncompressed_size) != entry->uncompressed_size)
		return NULL;
	*size = entry->uncompressed_size;
	return output;
}

typedef enum {
	SCRIPT_INVALID,
	SCRIPT_REGULAR,
	SCRIPT_BINARY
} script_type_t;

int extract_script(nozip_t *zip, const char *filename, char **output, size_t *output_size) 
{
	char path[256], *ext;
	// try loading jsbin, if js script is not found
	if (!(*output = extract_file(zip, filename, output_size))) {
		sprintf(path, "%s", filename);
		ext = strrchr(path, '.');
		if (!ext) {
			return SCRIPT_INVALID;
		}
		*ext = 0;
		strcat(path, ".jsbin");
		if ((*output = extract_file(zip, path, output_size))) {
			return SCRIPT_BINARY;
		}
	}
	if (!output)
		return SCRIPT_INVALID;
	return SCRIPT_REGULAR;
}

int parse_json(js_State *J, const char *source) 
{
	js_getglobal(J, "JSON");
	js_getproperty(J, -1, "parse");
	js_rot2(J);
	js_pushconst(J, source);
	if (js_pcall(J, 1)) {
		fprintf(stderr, "%s\n", js_trystring(J, -1, "unrecognized error"));
		return 1;
	}
	return 0;
}

int print_host_version(js_State *J) 
{
	const char *ver;
	if (!js_isobject(J, -1))
		return -1;
	js_getproperty(J, -1, "version");
	ver = js_trystring(J, -1, "n/a");
	printf("%s\n", ver);
	js_pop(J, 2);
	return 0;
}

int print_host_about(js_State *J) 
{
	const char *ver, *name, *desc;
	if (!js_isobject(J, -1))
		return -1;
	js_getproperty(J, -1, "version");
	ver = js_trystring(J, -1, "n/a");
	js_getproperty(J, -2, "name");
	name = js_trystring(J, -1, "n/a");
	js_getproperty(J, -3, "description");
	desc = js_trystring(J, -1, "n/a");
	printf("%s %s\n", name, ver);
	printf("%s\n", desc);
	js_pop(J, 4);
	return 0;
}

/* Built-ins */

static void jsB_print(js_State *J)
{
	int i, top = js_gettop(J);
	for (i = 1; i < top; ++i) {
		const char *s = js_tostring(J, i);
		if (i > 1) putchar(' ');
		fputs(s, stdout);
	}
	js_pushundefined(J);
}

static void jsB_repr(js_State *J)
{
	js_tryrepr(J, 1, "cannot represent value");
}

static void jsB_load(js_State *J)
{
	js_loadfile(J, js_trystring(J, 1, ""));
}

static void jsB_loadInternal(js_State *J)
{
	char *output;
	size_t output_size;
	int result = 0;
	nozip_t *zip;
	const char *filename = js_tostring(J, 1);
	if (!filename[0])
		js_error(J, "loadInternal: expected scriptname");
	js_getregistry(J, "InternalFiles");
	zip = js_touserdata(J, -1, "InternalFiles");
	js_pop(J, -1);
	result = extract_script(zip, filename, &output, &output_size);
	if (result == SCRIPT_REGULAR) {
		if (js_isobject(J, 2)) {
			js_copy(J, 2);
			js_loadstringE(J, filename, output);
		} else {
			js_loadstring(J, filename, output);
		}
	} else if (result == SCRIPT_BINARY) {
		if (js_isobject(J, 2)) {
			js_copy(J, 2);
			js_loadbinE(J, output, output_size);
		} else {
			js_loadbin(J, output, output_size);
		}
	} else {
		js_error(J, "loadInternal: could not extract script '%s': %s", filename, geterrstr(errno));
	}
}

static void jsB_compile(js_State *J)
{
	const char *source = js_tostring(J, 1);
	const char *filename = js_isdefined(J, 2) ? js_tostring(J, 2) : "[string]";
	if (js_isobject(J, 3)) {
		js_copy(J, 3);
		js_loadstringE(J, filename, source);
	} else
		js_loadstring(J, filename, source);
}

static void jsB_writeFile(js_State *J)
{
	FILE *f;
	const char *filename = js_tostring(J, 1);
	const char *data = js_tostring(J, 2);
	size_t length = js_getstrlen(J, 2); 
	size_t written;
	f = fopen(filename, "wb+");
	if (!f) {
		js_error(J, "writeFile: cannot open file '%s': %s", filename, geterrstr(errno));
	}
	written = fwrite(data, 1, length, f);
	if (written != length) {
		js_error(J, "writeFile: cannot write complete data in file '%s': %s", filename, geterrstr(errno));
	}
	fclose(f);
	js_pushundefined(J);
}

static void jsB_readFile(js_State *J)
{
	const char *filename = js_tostring(J, 1);
	FILE *f;
	char *s;
	int n, t;

	f = fopen(filename, "rb");
	if (!f) {
		js_error(J, "readFile: cannot open file '%s': %s", filename, geterrstr(errno));
	}

	if (fseek(f, 0, SEEK_END) < 0) {
		fclose(f);
		js_error(J, "readFile: cannot seek in file '%s': %s", filename, geterrstr(errno));
	}

	n = ftell(f);
	if (n < 0) {
		fclose(f);
		js_error(J, "readFile: cannot tell in file '%s': %s", filename, geterrstr(errno));
	}

	if (fseek(f, 0, SEEK_SET) < 0) {
		fclose(f);
		js_error(J, "readFile: cannot seek in file '%s': %s", filename, geterrstr(errno));
	}

	s = malloc(n);
	if (!s) {
		fclose(f);
		js_error(J, "readFile: out of memory");
	}

	t = fread(s, 1, n, f);
	if (t != n) {
		free(s);
		fclose(f);
		js_error(J, "readFile: cannot read file '%s': %s", filename, geterrstr(errno));
	}
	
	js_pushlstring(J, s, n);
	free(s);
	fclose(f);
}

static void jsB_readInternalFile(js_State *J)
{
	nozip_t *zip;
	char *output;
	size_t output_size;
	const char *filename = js_tostring(J, 1);
	if (!filename[0])
		js_error(J, "readInternalFile: expected filename");
	js_getregistry(J, "InternalFiles");
	zip = js_touserdata(J, -1, "InternalFiles");
	js_pop(J, -1);
	if (!(output = extract_file(zip, filename, &output_size)))
		js_error(J, "readInternalFile: cannot open file '%s': %s", filename, geterrstr(errno));
	js_pushlstring(J, output, output_size);
	js_free(J, output);
}

static void jsB_existsInternal(js_State *J)
{
	nozip_t *zip;
	char path[256], *ext;
	const char *filename = js_tostring(J, 1);
	if (!filename[0])
		js_error(J, "existsInternal: expected filename");
	js_getregistry(J, "InternalFiles");
	zip = js_touserdata(J, -1, "InternalFiles");
	js_pop(J, -1);
	// naive lookup
	nozip_entry_t *entry = nozip_locate(zip, filename);
	if (entry) {
		js_pushboolean(J, 1);
		return;
	}
	// script lookup
	sprintf(path, "%s", filename);
	ext = strrchr(path, '.');
	if (!ext) {
		js_pushboolean(J, 0);
		return;
	}
	if (!strcmp(ext, ".js")) {
		*ext = 0;
		strcat(path, ".jsbin");
		nozip_entry_t *entry = nozip_locate(zip, path);
		if (entry) {
			js_pushboolean(J, 1);
			return;
		}
	}
	js_pushboolean(J, 0);
	return;
}

static void jsB_exit(js_State *J)
{
	exit(js_checkinteger(J, 1, 1));
}

static void jsB_getenv(js_State *J)
{
	const char *value;
	const char *name = js_tostring(J, 1);
	if (!name[0]) {
		js_pushundefined(J);
		return;
	}
	value = getenv(name);
	if (!value)
		js_pushundefined(J);
	else
		js_pushstring(J, value);
}

static void jsB_getcwd(js_State *J)
{
	char cwd[1024];
	if (getcwd(cwd, sizeof(cwd)) == NULL) 
		js_error(J, "getcwd: %s", geterrstr(errno));
	js_pushstring(J, cwd);
}

static void jsB_realpath(js_State *J)
{
	char buf[1024];
	char *result = realpath(js_tostring(J, 1), buf);
	if (!result)
		js_error(J, "realpath: %s", geterrstr(errno));
	js_pushstring(J, result);
}

static void jsB_mkdir(js_State *J)
{
	int status;
	const char *path = js_tostring(J, 1);
	int mode = js_checkinteger(J, 2, 0777);
	if (!path[0])
		js_error(J, "mkdir: expected valid path");
	status = mkdir(js_tostring(J, 1), mode);
	if (status) {
		js_error(J, "mkdir: %s", geterrstr(errno));
	}
	js_pushnumber(J, (double)status);
}

static void jsB_remove(js_State *J)
{
	int status;
	const char *path = js_tostring(J, 1);
	if (!path[0])
		js_error(J, "remove: expected valid path");
	status = remove(path);
	if (status)
		js_error(J, "remove: %s", geterrstr(errno));
	js_pushnumber(J, (double)status);
}

static void jsB_readdir(js_State *J)
{
	int len = 0;
	struct dirent *dent;
	DIR *dd;
	const char *path = js_tostring(J, 1);
	if (!path[0])
		js_error(J, "readdir: expected valid path");
	js_newarray(J);
	dd = opendir(path);
	if (!dd)
		js_error(J, "readdir: %s", geterrstr(errno));
	while (1) {
		errno = 0;
		dent = readdir(dd);
		if (!dent) {
			if (errno != 0) {
				js_error(J, "readdir: %s", geterrstr(errno));
			}
			break;
		}
		js_pushstring(J, dent->d_name);
		js_setindex(J, -2, len++);
	}
	closedir(dd);
}

static void jsB_stat(js_State *J)
{
	int status;
	struct stat st;
	const char *path = js_tostring(J, 1);
	if (!path[0])
		js_error(J, "stat: expected valid path");
	status = stat(path, &st);
	if (status)
		js_error(J, "stat: %s", geterrstr(errno));
	js_newobject(J);
	js_pushboolean(J, (st.st_mode & S_IFMT) == S_IFREG);
	js_setproperty(J, -2, "isFile");
	js_pushboolean(J, (st.st_mode & S_IFMT) == S_IFDIR);
	js_setproperty(J, -2, "isDirectory");
	js_pushnumber(J, (double)st.st_size);
	js_setproperty(J, -2, "size");
	js_pushnumber(J, (double)msfromtime(&st.st_atim));
	js_setproperty(J, -2, "atime");
	js_pushnumber(J, (double)msfromtime(&st.st_mtim));
	js_setproperty(J, -2, "mtime");
	js_pushnumber(J, (double)msfromtime(&st.st_ctim));
	js_setproperty(J, -2, "ctime");
}

static void jsB_utimes(js_State *J)
{
	int status;
	int64_t atime;
	int64_t mtime;
	const char *path = js_tostring(J, 1);
	struct timeval timevals[2];
	if (!path[0])
		js_error(J, "utimes: expected valid path");
	atime = (int64_t)js_tonumber(J, 2);
	mtime = (int64_t)js_tonumber(J, 3);
	timefromms(timevals, atime);
	timefromms(timevals + 1, mtime);
	status = utimes(path, timevals);
	if (status)
		js_error(J, "utimes: %s", geterrstr(errno));
	js_pushundefined(J);
}	

static void jsB_exists(js_State *J)
{
	int status;
	struct stat st;
	const char *path = js_tostring(J, 1);
	int checkdir = js_isdefined(J, 2) ? js_toboolean(J, 2) : 0;
	if (!path[0])
		js_error(J, "exists: expected valid path");
	status = stat(path, &st);
	if (status) {
		js_pushboolean(J, 0);
	} else if (checkdir) {
		js_pushboolean(J, (st.st_mode & S_IFMT) == S_IFDIR);
	} else {
		js_pushboolean(J, 1);
	}
}

int main(int argc, char *argv[]) {
	int i, status = 0;
	const char *bootstrap_name;
	char *bootstrap_src, *manifest_src = NULL;
	size_t bootstrap_size, manifest_size;
	js_State *J = js_newstate(NULL, NULL, 0);
	typedef int (*host_action_t)(js_State *J);
	host_action_t host_action;
	size_t zsize = nozip_size_mem(__pak_data_img, __pak_data_img_length);
	nozip_t *zip = malloc(zsize); 
	if (!zip || !nozip_read_mem(zip, __pak_data_img, __pak_data_img_length)) {
		js_freestate(J);
		fprintf(stderr, "cannot allocate memory or read zip file.\n");
		return 1;
	}

#define GET_FILE_OR_EXIT(name, out, outsize) \
	if (!(out = extract_file(zip, name, outsize))) { \
		free(zip); \
		fprintf(stderr, "cannot extract file '%s': %s", name, geterrstr(errno)); \
		js_freestate(J); \
		return 1; \
	}

	GET_FILE_OR_EXIT(MANIFEST_FILE, manifest_src, &manifest_size);
	parse_json(J, manifest_src);

	// print meta
	for (i = 1; i < argc; ++i) {
		if (!strcmp(argv[i], "--host-version")) {
			host_action = print_host_version;
		} else if (!strcmp(argv[i], "--host-about")) {
			host_action = print_host_about;
		} else {
			continue;
		}
		status = host_action(J);
		goto end;
	}

	js_getproperty(J, -1, "bootstrap");
	if (js_isundefined(J, -1)) {
		fprintf(stderr, "cannot find bootstrap script.\n");
		status = 1;
		goto end;
	}
	bootstrap_name = js_tostring(J, -1);

	js_newobject(J);

	js_newcfunction(J, jsB_print, "__print", 0);
	js_setproperty(J, -2, "__print");

	js_newcfunction(J, jsB_repr, "__repr", 1);
	js_setproperty(J, -2, "__repr");
	
	js_newcfunction(J, jsB_load, "__load", 1);
	js_setproperty(J, -2, "__load");

	js_newcfunction(J, jsB_loadInternal, "__loadInternal", 2);
	js_setproperty(J, -2, "__loadInternal");

	js_newcfunction(J, jsB_compile, "__compile", 3);
	js_setproperty(J, -2, "__compile");
	
	js_newcfunction(J, jsB_readFile, "__readFile", 1);
	js_setproperty(J, -2, "__readFile");

	js_newcfunction(J, jsB_readInternalFile, "__readInternalFile", 1);
	js_setproperty(J, -2, "__readInternalFile");

	js_newcfunction(J, jsB_existsInternal, "__existsInternal", 1);
	js_setproperty(J, -2, "__existsInternal");

	js_newcfunction(J, jsB_writeFile, "__writeFile", 2);
	js_setproperty(J, -2, "__writeFile");

	js_newcfunction(J, jsB_exit, "__exit", 1);
	js_setproperty(J, -2, "__exit");	

	js_newcfunction(J, jsB_getenv, "__getenv", 1);
	js_setproperty(J, -2, "__getenv");

	js_newcfunction(J, jsB_getcwd, "__getcwd", 0);
	js_setproperty(J, -2, "__getcwd");

	js_newcfunction(J, jsB_realpath, "__realpath", 1);
	js_setproperty(J, -2, "__realpath");

	js_newcfunction(J, jsB_mkdir, "__mkdir", 2);
	js_setproperty(J, -2, "__mkdir");

	js_newcfunction(J, jsB_remove, "__remove", 1);
	js_setproperty(J, -2, "__remove");

	js_newcfunction(J, jsB_readdir, "__readdir", 1);
	js_setproperty(J, -2, "__readdir");

	js_newcfunction(J, jsB_stat, "__stat", 1);
	js_setproperty(J, -2, "__stat");

	js_newcfunction(J, jsB_utimes, "__utimes", 1);
	js_setproperty(J, -2, "__utimes");

	js_newcfunction(J, jsB_exists, "__exists", 1);
	js_setproperty(J, -2, "__exists");

	js_pushstring(J, bootstrap_name);
	js_setproperty(J, -2, "__filename");

	js_pushstring(J, OS_PLATFORM);
	js_setproperty(J, -2, "__platform");

	js_newarray(J);
	for (i = 0; i < argc; ++i) {
		js_pushstring(J, argv[i]);
		js_setindex(J, -2, i);
	}
	js_setproperty(J, -2, "__scriptArgs");

	js_newobject(J);
	js_newuserdata(J, "InternalFiles", zip, NULL);
	js_setregistry(J, "InternalFiles");

	if (js_try(J)) {
		status = 1;
		fprintf(stderr, "%s\n", js_tostring(J, -1));		
	} else {
		int result = extract_script(zip, bootstrap_name, &bootstrap_src, &bootstrap_size);
		if (result == SCRIPT_REGULAR) {
			js_loadstringE(J, bootstrap_name, bootstrap_src);
		} else if (result == SCRIPT_BINARY) {
			js_loadbinE(J, bootstrap_src, bootstrap_size);
		} else {
			js_error(J, "cannot extract boostrap script '%s': %s", bootstrap_name, geterrstr(errno));
		}
		js_pushundefined(J);
		if (js_pcall(J, 0)) {
			status = 1;
			fprintf(stderr, "%s\n", js_tostring(J, -1));
		}
	}

	free(bootstrap_src);
end:
	js_freestate(J);
	free(manifest_src);
	free(zip);

	return status ? 1 : 0;
}
