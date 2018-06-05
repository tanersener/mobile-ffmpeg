tests = [
  ['CapRTL', 'explicit'],
  ['CapRTL', 'implicit'],
  ['CapRTL', 'isolate'],
  ['ISO8859-8', 'hebrew'],
  ['UTF-8', 'persian'],
  ['UTF-8', 'reordernsm'],
]

test_runner = find_program('test-runner.py')

foreach t : tests
  charset = t[0]
  suffix = t[1]
  test_name = '@0@_@1@'.format(charset, suffix)
  input_file = files('test_@0@.input'.format(test_name))
  reference_file = files('test_@0@.reference'.format(test_name))
  test(test_name, test_runner, args: [fribidi, charset, input_file, reference_file])
endforeach

subdir('unicode-conformance')
