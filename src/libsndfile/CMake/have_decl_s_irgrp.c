#include <sys/stat.h>
int main (void)
{
	/* This will fail to compile if S_IRGRP doesn't exist. */
	return S_IRGRP ;
}
