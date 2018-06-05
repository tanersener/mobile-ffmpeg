tests = [
  ['BidiTest', 'BidiTest.c'],
  ['BidiCharacterTest', 'BidiCharacterTest.c'],
]
foreach t : tests
  exe = executable(t[0],
    t[1], fribidi_unicode_version_h,
    c_args: ['-DHAVE_CONFIG_H'],
    include_directories: incs,
    link_with: libfribidi)
  test(t[0], exe, args: files('@0@.txt'.format(t[0])))
endforeach
