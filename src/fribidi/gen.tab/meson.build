# gen.tab

native_cc = meson.get_compiler('c')

# Don't pick up top-level config.h, as that has defines for the cross-compiler
# (if we're cross-compiling), but here we need values for the host compiler/env
# in that case. We shall pass those on the command line since it's just a few.
native_args = ['-UHAVE_CONFIG_H']

# This is available pretty much everywhere
native_args += ['-DHAVE_STRINGIZE']

native_args += ['-DDONT_HAVE_FRIBIDI_CONFIG_H']

if native_cc.has_header('stdlib.h')
  native_args += ['-DHAVE_STDLIB_H']
endif

if native_cc.has_header('string.h')
  native_args += ['-DHAVE_STRING_H']
endif

if native_cc.has_header('strings.h')
  native_args += ['-DHAVE_STRINGS_H']

  # Not entirely correct, but sufficient for us. Should move away from this
  # ancient define and just include individual headers based on individual defs.
  # stdlib.h and string.h are standard nowadays, but strings.h not (msvc)
  native_args += ['-DSTDC_HEADERS=1']
endif

gen_unicode_version = executable('gen-unicode-version',
  'gen-unicode-version.c',
  include_directories: incs,
  c_args: native_args,
  install: false)

fribidi_unicode_version_h = custom_target('fribidi-unicode-version.h',
  input: files('unidata/ReadMe.txt', 'unidata/BidiMirroring.txt'),
  output: 'fribidi-unicode-version.h',
  command: [gen_unicode_version, '@INPUT0@', '@INPUT1@', 'gen-unicode-version'],
  capture: true,
  install_dir: join_paths(get_option('includedir'), 'fribidi'),
  install: true)

COMPRESSION='2'

tabs = [
  ['bidi-type', files('unidata/UnicodeData.txt')],
  ['joining-type', files('unidata/UnicodeData.txt', 'unidata/ArabicShaping.txt')],
  ['arabic-shaping', files('unidata/UnicodeData.txt')],
  ['mirroring', files('unidata/BidiMirroring.txt')],
  ['brackets', files('unidata/BidiBrackets.txt', 'unidata/UnicodeData.txt')],
  ['brackets-type', files('unidata/BidiBrackets.txt')],
]

generated_tab_include_files = []

foreach tab : tabs
  gen_prog_name = 'gen-@0@-tab'.format(tab[0])
  gen_prog_src = 'gen-@0@-tab.c'.format(tab[0])
  gen_prog_out = '@0@.tab.i'.format(tab[0])
  gen_prog_inputs = tab[1]

  gen_exe = executable(gen_prog_name,
    fribidi_unicode_version_h,
    gen_prog_src, 'packtab.c',
    include_directories: incs,
    c_args: native_args,
    install: false)

  tab_inc_file = custom_target(gen_prog_name,
    input: gen_prog_inputs,
    output: gen_prog_out,
    command: [gen_exe, COMPRESSION, '@INPUT@', gen_prog_name],
    capture: true)

  generated_tab_include_files += [tab_inc_file]
endforeach
