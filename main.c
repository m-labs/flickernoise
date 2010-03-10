#include <stdio.h>
#include <stdlib.h>

#include <dopelib.h>
#include <vscreen.h>

int main(int argc, char *argv[])
{
	dope_init();

	dope_eventloop(0);

	return 0;
}
