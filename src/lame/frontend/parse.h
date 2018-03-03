#ifndef PARSE_H_INCLUDED
#define PARSE_H_INCLUDED

#if defined(__cplusplus)
extern "C" {
#endif

int     usage(FILE * const fp, const char *ProgramName);
int     short_help(const lame_global_flags * gfp, FILE * const fp, const char *ProgramName);
int     long_help(const lame_global_flags * gfp, FILE * const fp, const char *ProgramName,
                  int lessmode);
int     display_bitrates(FILE * const fp);

int     parse_args(lame_global_flags * gfp, int argc, char **argv, char *const inPath,
                   char *const outPath, char **nogap_inPath, int *max_nogap);

void    parse_close();

int     generateOutPath(char const* inPath, char const* outDir, char const* suffix, char* outPath);

#if defined(__cplusplus)
}
#endif

#endif
/* end of parse.h */
