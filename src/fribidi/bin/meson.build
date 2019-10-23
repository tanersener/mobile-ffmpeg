fribidi = executable('fribidi',
  'fribidi-main.c', 'getopt.c', 'getopt1.c', fribidi_unicode_version_h,
  c_args: ['-DHAVE_CONFIG_H'] + visibility_args,
  include_directories: incs,
  link_with: libfribidi,
  install: true)

executable('fribidi-benchmark',
  'fribidi-benchmark.c', 'getopt.c', 'getopt1.c', fribidi_unicode_version_h,
  c_args: ['-DHAVE_CONFIG_H'] + visibility_args,
  include_directories: incs,
  link_with: libfribidi,
  install: false)

executable('fribidi-bidi-types',
  'fribidi-bidi-types.c', fribidi_unicode_version_h,
  c_args: ['-DHAVE_CONFIG_H'] + visibility_args,
  include_directories: incs,
  link_with: libfribidi,
  install: false)

executable('fribidi-caprtl2utf8',
  'fribidi-caprtl2utf8.c', fribidi_unicode_version_h,
  c_args: ['-DHAVE_CONFIG_H'] + visibility_args,
  include_directories: incs,
  link_with: libfribidi,
  install: false)
