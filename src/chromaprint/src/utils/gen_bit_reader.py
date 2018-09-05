import sys
import os

nbits = int(sys.argv[1])

for block_size in range(1, nbits + 1):
    if (block_size * 8) % nbits == 0:
        break

def gen_unpack_bytes(nbytes):
    code = []
    for i in range(nbytes):
        code.append('const unsigned char s{} = *src++;'.format(i))
    for i in range(nbytes * 8 // nbits):
        shift = i * nbits
        mask = ((1 << nbits) - 1) << shift
        block_masks = [mask & (255 << (j * 8)) for j in range(nbytes)]

        parts = []
        for j in range(nbytes):
            part_shift = j * 8
            part_mask = mask & (255 << part_shift)
            if part_mask == 0:
                continue
            if shift == part_shift:
                parts.append('(s{} & 0x{:02x})'.format(j, part_mask >> part_shift))
            elif shift > part_shift:
                parts.append('((s{} & 0x{:02x}) >> {})'.format(j, part_mask >> part_shift, shift - part_shift))
            else:
                parts.append('((s{} & 0x{:02x}) << {})'.format(j, part_mask >> part_shift, part_shift - shift))
        code.append('*dest++ = {};'.format(' | '.join(parts)))
    return i + 1, code

print '// Copyright (C) 2016  Lukas Lalinsky'
print '// Distributed under the MIT license, see the LICENSE file for details.'
print
print '// This file was automatically generate using {}, do not edit.'.format(os.path.basename(__file__))
print
print '#ifndef CHROMAPRINT_UTILS_UNPACK_INT{}_ARRAY_H_'.format(nbits)
print '#define CHROMAPRINT_UTILS_UNPACK_INT{}_ARRAY_H_'.format(nbits)
print
print '#include <algorithm>'
print
print 'namespace chromaprint {'
print
print 'inline size_t GetUnpackedInt{}ArraySize(size_t size) {{'.format(nbits)
print '\treturn size * {} / {};'.format(block_size * 8 // nbits, block_size)
print '}'
print
print 'template <typename InputIt, typename OutputIt>'
print 'inline OutputIt UnpackInt{}Array(const InputIt first, const InputIt last, OutputIt dest) {{'.format(nbits)
print '\tauto size = std::distance(first, last);'
print '\tauto src = first;'
first_if = True
for nbytes in range(block_size, 0, -1):
    if nbytes == block_size:
        print '\twhile (size >= {}) {{'.format(nbytes)
    else:
        if not first_if:
            print 'else',
        else:
            print '\t',
        print 'if (size == {}) {{'.format(nbytes)
        first_if = False
    unpacked_bits, code = gen_unpack_bytes(nbytes)
    for line in code:
        print '\t\t{}'.format(line)
    if nbytes == block_size:
        print '\t\tsize -= {};'.format(nbytes)
    if nbytes == block_size or nbytes == 1:
        print '\t}'
    else:
        print '\t}',
print '\treturn dest;'
print '}'
print
print '}; // namespace chromaprint'
print
print '#endif'
