
#if @HAVE_VISIBILITY@ && BUILDING_LIBCHARSET
#define LIBCHARSET_DLL_EXPORTED __attribute__((__visibility__("default")))
#elif defined _MSC_VER && BUILDING_LIBCHARSET
#define LIBCHARSET_DLL_EXPORTED __declspec(dllexport)
#else
#define LIBCHARSET_DLL_EXPORTED
#endif
