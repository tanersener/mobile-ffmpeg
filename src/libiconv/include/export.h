
#if @HAVE_VISIBILITY@ && BUILDING_LIBICONV
#define LIBICONV_DLL_EXPORTED __attribute__((__visibility__("default")))
#elif defined _MSC_VER && BUILDING_LIBICONV
#define LIBICONV_DLL_EXPORTED __declspec(dllexport)
#else
#define LIBICONV_DLL_EXPORTED
#endif
