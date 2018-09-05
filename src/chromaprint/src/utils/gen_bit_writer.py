import sys
import os

nbits = int(sys.argv[1])

for block_size in range(1, nbits + 1):
    if (block_size * 8) % nbits == 0:
        break

def gen_pack_bytes(num_values):
    code = []
    nbytes = (num_values * nbits + 7) // 8
    for i in range(num_values):
        code.append('const unsigned char s{} = *src++;'.format(i))
    for i in range(nbytes):
        byte_shift = i * 8
        byte_mask = 0xff << byte_shift
        values = []
        for j in range(num_values):
            value_shift = j * nbits
            value_mask = ((1 << nbits) - 1) << value_shift
            mask = value_mask & byte_mask
            if not mask:
                continue
            if value_shift == byte_shift:
                values.append('((s{} & 0x{:02x}))'.format(j, mask >> value_shift))
            elif value_shift > byte_shift:
                values.append('((s{} & 0x{:02x}) << {})'.format(j, mask >> value_shift, value_shift - byte_shift))
            else:
                values.append('((s{} & 0x{:02x}) >> {})'.format(j, mask >> value_shift, byte_shift - value_shift))
        code.append('*dest++ = (unsigned char) {};'.format(' | '.join(values)))
    return i + 1, code

print '// Copyright (C) 2016  Lukas Lalinsky'
print '// Distributed under the MIT license, see the LICENSE file for details.'
print
print '// This file was automatically generate using {}, do not edit.'.format(os.path.basename(__file__))
print
print '#ifndef CHROMAPRINT_UTILS_PACK_INT{}_ARRAY_H_'.format(nbits)
print '#define CHROMAPRINT_UTILS_PACK_INT{}_ARRAY_H_'.format(nbits)
print
print '#include <algorithm>'
print
print 'namespace chromaprint {'
print
print 'inline size_t GetPackedInt{}ArraySize(size_t size) {{'.format(nbits)
print '\treturn (size * {} + {}) / {};'.format(block_size, block_size * 8 // nbits - 1, block_size * 8 // nbits)
print '}'
print
print 'template <typename InputIt, typename OutputIt>'
print 'inline OutputIt PackInt{}Array(const InputIt first, const InputIt last, OutputIt dest) {{'.format(nbits)
print '\tauto size = std::distance(first, last);'
print '\tauto src = first;'
first_if = True
for nbytes in range(block_size * 8 // nbits, 0, -1):
    if nbytes == block_size * 8 // nbits:
        print '\twhile (size >= {}) {{'.format(nbytes)
    else:
        if not first_if:
            print 'else',
        else:
            print '\t',
        print 'if (size == {}) {{'.format(nbytes)
        first_if = False
    packed_bits, code = gen_pack_bytes(nbytes)
    for line in code:
        print '\t\t{}'.format(line)
    if nbytes == block_size * 8 // nbits:
        print '\t\tsize -= {};'.format(nbytes)
    if nbytes == block_size * 8 // nbits or nbytes == 1:
        print '\t}'
    else:
        print '\t}',
print '\t return dest;'
print '}'
print
print '}; // namespace chromaprint'
print
print '#endif'
