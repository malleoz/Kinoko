#!/usr/bin/env python3
# Decodes the "UNINIT:" lines that debug builds of Kinoko print when a Checked<T> member is read
# before it was ever assigned. Each report's caller addresses are resolved to function/file/line
# with addr2line, and identical reports are grouped together.
#
# Usage:
#   ./kinokoD test -s testCases.bin > log.txt
#   python tools/decode_uninit.py log.txt [--exe out/kinokoD.exe] [--frames 4]
#
# The addresses are only meaningful for the exact binary that produced the log, so don't rebuild
# kinokoD between running it and decoding.

import argparse
import collections
import os
import re
import shutil
import struct
import subprocess
import sys

UNINIT_RE = re.compile(r'UNINIT: type="(?P<type>.*)" addr=\S+ stamp=(?P<stamp>[0-9a-f]+) '
                       r'frames=(?P<frames>\S*)')


def read_image_info(exe_path: str) -> tuple[int, int | None]:
    """Returns (preferred image base, PE link timestamp). ELF images use base 0 and no stamp."""
    with open(exe_path, 'rb') as f:
        data = f.read(4096)

    if data[:4] == b'\x7fELF':
        return 0, None

    if data[:2] != b'MZ':
        sys.exit(f'{exe_path} is not a PE or ELF executable')

    pe_offset = struct.unpack_from('<I', data, 0x3c)[0]
    timestamp = struct.unpack_from('<I', data, pe_offset + 8)[0]
    opt_offset = pe_offset + 24
    magic = struct.unpack_from('<H', data, opt_offset)[0]
    if magic == 0x20b:  # PE32+
        image_base = struct.unpack_from('<Q', data, opt_offset + 24)[0]
    else:
        image_base = struct.unpack_from('<I', data, opt_offset + 28)[0]
    return image_base, timestamp


def resolve(addr2line: str, exe_path: str, addresses: list[int]) -> dict[int, str]:
    if not addresses:
        return {}

    # Return addresses point after the call instruction, so look up the byte before them.
    args = [addr2line, '-f', '-C', '-e', exe_path] + [hex(a - 1) for a in addresses]
    out = subprocess.run(args, capture_output=True, text=True, check=True).stdout.splitlines()

    resolved = {}
    for i, addr in enumerate(addresses):
        func = out[2 * i]
        loc = out[2 * i + 1]
        loc = re.sub(r' \(discriminator \d+\)', '', loc)
        loc = re.sub(r'^.*[\\/](source|include)[\\/]', r'\1/', loc).replace('\\', '/')
        func = re.sub(r'\(.*\)( const)?$', '', func).replace('Kinoko::', '')
        resolved[addr] = f'{func} @ {loc}'
    return resolved


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('log', help='Kinoko output containing UNINIT: lines ("-" for stdin)')
    parser.add_argument('--exe', default=os.path.join('out', 'kinokoD.exe' if os.name == 'nt'
                                                      else 'kinokoD'))
    parser.add_argument('--frames', type=int, default=4,
                        help='caller frames to show per report, excluding Checked internals')
    parser.add_argument('--addr2line', default=shutil.which('addr2line') or 'addr2line')
    args = parser.parse_args()

    if args.log == '-':
        log = sys.stdin.read()
    else:
        raw = open(args.log, 'rb').read()
        # PowerShell's ">" redirection writes UTF-16
        encoding = 'utf-16' if raw[:2] in (b'\xff\xfe', b'\xfe\xff') else 'utf-8'
        log = raw.decode(encoding, errors='replace')
    reports = [m for m in UNINIT_RE.finditer(log)]
    if not reports:
        print('No UNINIT reports found.')
        return

    image_base, timestamp = read_image_info(args.exe)
    stamps = {int(m['stamp'], 16) for m in reports}
    if timestamp is not None and stamps != {timestamp}:
        print(f'WARNING: log was produced by a different build of {args.exe} '
              f'(log stamp(s) {", ".join(f"{s:08x}" for s in stamps)}, '
              f'binary stamp {timestamp:08x}). Results will be wrong.\n', file=sys.stderr)

    parsed = []
    for m in reports:
        offsets = [int(f, 16) for f in m['frames'].replace('+', '').split(',') if f]
        parsed.append((m['type'], [image_base + o for o in offsets]))

    unique = sorted({a for _, frames in parsed for a in frames})
    names = resolve(args.addr2line, args.exe, unique)

    groups = collections.Counter()
    for type_name, frames in parsed:
        callers = [names[a] for a in frames if 'include/Checked.hh' not in names[a]]
        groups[(type_name, tuple(callers[:args.frames]))] += 1

    print(f'{len(reports)} report(s), {len(groups)} distinct site(s)\n')
    for (type_name, callers), count in groups.most_common():
        print(f'{count}x  Checked<{type_name}>')
        for c in callers:
            print(f'      {c}')
        print()


if __name__ == '__main__':
    main()
