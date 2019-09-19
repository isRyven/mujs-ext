#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <ctype.h>

const char *check_name(const char *name)
{
	int c = 0;
	static char buffer[256];
	memset(buffer, 0, sizeof(buffer));
	sprintf(buffer, "%s", isdigit((int)name[0]) ? "__" : "");
	int offset = strlen(buffer);
	while (name[c])
	{
	    buffer[offset + c] = isalnum(name[c]) ? name[c] : '_';
	    ++c;
	}
	return buffer;
}

int to_header(FILE* file, const char *varname, const unsigned char *buf, int size)
{
	int c = 0;
	const char *name = check_name(varname);
	fprintf(file, "unsigned char %s[] = {\n", name);
    while (c < size)
	{
	  	fprintf(file, "%s0x%02x", (c % 12) ? ", " : (",\n  ") + 2 * !c, buf[c]);
	  	++c;
	}
	fputs("\n};\n", file);
	fprintf(file, "unsigned int %s_length = %i;\n", name, size);
	return 1;
}

int main(int argc, const char **argv)
{
	FILE *f;
	unsigned char *buf;
	const char *name;
	// char cwd[1024];
	int size, readSize;
	if (argc != 4)
	{
		fprintf(stderr, "%s\n", "Error: no input files were provided");
		fprintf(stderr, "%s\n", "Usage: ./toheader varName input output.h");
		return 1;
	}
	name = argv[2];
	f = fopen(name, "rb");
	if (!f)
	{
		fprintf(stderr, "Error: could not open file for a read: %s\n%s\n", name, strerror(errno));
		return 1;
	}
	if (fseek(f, 0, SEEK_END) < 0)
	{
		fclose(f);
		fprintf(stderr, "Error: could not seek in file: %s\n%s\n", name, strerror(errno));
		return 1;
	}
	size = ftell(f);
	if (size < 0)
	{
		fclose(f);
		fprintf(stderr, "Error: could not tell in file: %s\n%s\n", name, strerror(errno));
		return 1;
	}
	if (fseek(f, 0, SEEK_SET) < 0)
	{
		fclose(f);
		fprintf(stderr, "Error: could not seek in file: %s\n%s\n", name, strerror(errno));
		return 1;
	}
	buf = malloc(size);
	if (!buf)
	{
		fclose(f);
		fprintf(stderr, "Error: could not allocate %ib chunk for file: %s\n", size, name);
		return 1;
	}
	readSize = fread(buf, 1, size, f);
	if (readSize != size)
	{
		free(buf);
		fclose(f);
		fprintf(stderr, "Error: could not read data from file: %s\n%s\n", name, strerror(errno));
		return 1;
	}
	fclose(f);
	f = fopen(argv[3], "wb+");
	if (!f)
	{
		free(buf);
		fprintf(stderr, "Error: could not open for a write: %s\n%s\n", name, strerror(errno));
		return 1;
	}
	to_header(f, argv[1], buf, size);
	fclose(f);
	fprintf(stdout, "(%s): %s -> %s\n", argv[1], argv[2], argv[3]);
	return 0;
}
