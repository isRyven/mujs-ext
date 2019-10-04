#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <mujs/mujs.h>

double get_time()
{
    return (double)clock() / CLOCKS_PER_SEC;
}

#define SAFE_PAD 8
#define MAX_LENGHT 256
#define RAND_CHAR (65 + (rand() % 60))

const char *getRandomName(int maxlen) 
{	
	static char buf[MAX_LENGHT];
	int len = SAFE_PAD + ((rand()) % maxlen % (MAX_LENGHT - SAFE_PAD));
	for (int i = 0; i < len; i++)
		buf[i] = RAND_CHAR;
	buf[len] = 0;
	return buf;
}

void benchmark_entry_search(int numEntries, int numRandAccess)
{
	double start, end;
	js_State *J = js_newstate(NULL, NULL, 0);
	js_newobject(J);
	srand(numEntries);
	char **entries = malloc(numEntries * sizeof(char*));
	// insert
	start = get_time();
	for (int i = 0; i < numEntries; i++)
	{
		entries[i] = malloc(MAX_LENGHT);
		const char *name = getRandomName(32);
		strcpy(entries[i], name);
		// set key
		js_pushnumber(J, i);
		js_setproperty(J, -2, name);
	}
	end = get_time();
	printf("insert: %f\n", end - start);
	
	// work
	start = get_time();
	for (int i = 0; i < numRandAccess; i++)
	{
		int entryIndex = rand() % numEntries;
		const char *entryName = entries[entryIndex];
		js_getproperty(J, -1, entryName);
		int value = js_tointeger(J, -1);
		if (value != entryIndex)
			js_error(J, "invalid value");
		js_pop(J, 1);
	}
	end = get_time();
	printf("get: %f\n", end - start);

	// release strings
	for (int i = 0; i < numEntries; i++)
		free(entries[i]);
	free(entries);
}

void benchmark_keys_listing(int numEntries)
{
	double start, end;
	js_State *J = js_newstate(NULL, NULL, 0);
	js_newobject(J);
	srand(numEntries);
	char **entries = malloc(numEntries * sizeof(char*));
	if (js_try(J))
	{
		printf("%s\n", js_tostring(J, -1));
		return;
	}
	// insert
	start = get_time();
	for (int i = 0; i < numEntries; i++)
	{
		entries[i] = malloc(MAX_LENGHT);
		const char *name = getRandomName(32);
		strcpy(entries[i], name);
		// set key
		js_pushnumber(J, i);
		js_setproperty(J, -2, name);
	}
	end = get_time();
	printf("insert: %f\n", end - start);
	
	// work
	start = get_time();
	js_getglobal(J, "Object");
	js_getproperty(J, -1, "keys");
	js_pushundefined(J);
	js_copy(J, -4);
	js_call(J, 1);
	if (!js_isarray(J, -1))
		js_error(J, "expected array");
	if (js_getlength(J, -1) != numEntries)
		js_error(J, "invalid length size");

	for (int i = 0; i < numEntries; i++)
	{
		js_getindex(J, -1, i);
		js_pop(J, 1);
	}
	end = get_time();
	printf("keys: %f\n", end - start);

	// release strings
	for (int i = 0; i < numEntries; i++)
		free(entries[i]);
	free(entries);
	js_endtry(J);
	js_freestate(J);
}

int main(int arg, const char **argv) 
{	
	printf("<entry search>\n");
	benchmark_entry_search(100000, 100000);
	printf("<key iteration>\n");
	benchmark_keys_listing(10000);
	return 0;
}
