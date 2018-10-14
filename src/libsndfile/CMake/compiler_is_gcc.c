int main (void)
{
#if __GNUC__
	#if __clang__
		This is clang
#	endif
#else
	This is not GCC.
#endif
	return 0 ;
}
