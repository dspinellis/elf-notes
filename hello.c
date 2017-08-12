#include <stdio.h>

extern int dgsh_force_include;

int
main(int argc, char *argv[])
{
	dgsh_force_include = 0;
	printf("hello, world\n");
	return 0;
}
