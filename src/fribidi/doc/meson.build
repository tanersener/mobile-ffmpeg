c2man = find_program('c2man', required: false)

if not c2man.found()
  error('c2man is required to build documentation. Or disable with -Ddocs=false')
endif

# Extract list of man pages (one man page per symbol) from lib/libfribidi.def
python3 = import('python3').find_python()

result = run_command(python3,
  '-c', 'import sys; print(open(sys.argv[1], "r").read())',
  files('../lib/fribidi.def'))

if result.returncode() != 0
  error('Could not extract list of symbols from fribidi.def.')
endif

gen_man_pages = []

syms = result.stdout().strip().split('\n')
foreach sym : syms
  # for consistency with autotools build and because we don't pass -v to c2man
  if sym != 'fribidi_unicode_version' and sym != 'fribidi_version_info'
    gen_man_pages += ['@0@.3'.format(sym)]
  endif
endforeach

c2man_incs = []
c2man_incs += ['-I' + join_paths(meson.source_root(), 'lib')]
c2man_incs += ['-I' + join_paths(meson.build_root(), 'lib')]
c2man_incs += ['-I' + join_paths(meson.source_root(), 'gen.tab')]
c2man_incs += ['-I' + join_paths(meson.build_root(), 'gen.tab')]

custom_target('man pages',
  command: [c2man, '-T', 'n', '-M', 'Programmer\'s Manual', c2man_incs,
            '-D__FRIBIDI_DOC', '-DDONT_HAVE_FRIBIDI_CONFIG_H',
            '-o@0@'.format(meson.current_build_dir()),
           fribidi_headers],
  depends: [fribidi_unicode_version_h],
  output: gen_man_pages,
  install_dir: join_paths(get_option('prefix'), get_option('mandir'), 'man3'),
  install: true)
