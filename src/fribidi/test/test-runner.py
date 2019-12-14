#!/usr/bin/env python3

# Test runner that calls ../bin/fribidi --test --charset CHARSET *.input,
# captures the output and compares it to the data in the reference file given
import subprocess
import sys
import os

if len(sys.argv) != 5:
  raise Exception('Expected 4 command-line arguments: test_exe charset test.input test.reference')

script = sys.argv[0]
test_exe = sys.argv[1]
charset = sys.argv[2]
input_file = sys.argv[3]
reference_file = sys.argv[4]

if os.name == 'nt':
  libpath = os.path.join(os.path.dirname(os.path.realpath(test_exe)),
                         '..',
                         'lib')
  os.environ['PATH'] = libpath + ';' + os.environ['PATH']

try:
  output = subprocess.check_output([test_exe, '--test', '--charset', charset, input_file])
  ref_data = open(reference_file, "rb").read()
  if os.name == 'nt':
    output = output.replace(b'\r\n', b'\n')
    ref_data = ref_data.replace(b'\r\n', b'\n') 
  if output != ref_data:
    print('Output:\n', output)
    print('Reference file:\n', ref_data)
    raise Exception('fribidi --test output for charset ' + charset + ' and input file ' + input_file + ' does not match data from reference file ' + reference_file)
except:
  raise Exception('fribidi --test failed for charset ' + charset + ' and input file ' + input_file)
