#!/usr/bin/python3
# Copyright (C) 2026 David Nichols
# SPDX-License-Identifier: MIT
"""Repack pinned dependency archives as deterministic offline orig components."""
import argparse
import hashlib
import json
from pathlib import Path, PurePosixPath
import subprocess
import tarfile

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('archive_directory', type=Path)
parser.add_argument('output_directory', type=Path)
args = parser.parse_args()
debian = Path(__file__).resolve().parent
specs = json.loads((debian / 'vendor-sources.json').read_text())
def field(name):
    return subprocess.check_output(['dpkg-parsechangelog', '-l' + str(debian / 'changelog'),
                                    '-S' + name], text=True).strip()
version = field('Version').rsplit('-', 1)[0]
epoch = int(field('Timestamp'))
args.output_directory.mkdir(parents=True, exist_ok=True)
for component, spec in specs.items():
    source = args.archive_directory / (spec['name'] + '-' + spec['tag'] + '.tar.gz')
    if hashlib.sha256(source.read_bytes()).hexdigest() != spec['sha256']:
        raise SystemExit(f'{component}: upstream archive checksum mismatch')
    output = args.output_directory / f'qore-zmq-module_{version}.orig-{component}.tar.xz'
    with output.open('xb') as stream, tarfile.open(source) as original:
        with tarfile.open(fileobj=stream, mode='w:xz', format=tarfile.GNU_FORMAT) as repacked:
            for item in sorted(original.getmembers(), key=lambda entry: entry.name):
                path = PurePosixPath(item.name)
                if path.is_absolute() or '..' in path.parts or path.parts[0] != spec['top']:
                    raise SystemExit(f'Unsafe archive path: {item.name}')
                relative = '/'.join(path.parts[1:])
                if not any(relative == keep or relative.startswith(keep + '/')
                           for keep in spec['retained_paths']):
                    continue
                if not (item.isfile() or item.isdir()):
                    raise SystemExit(f'Unsupported archive member: {item.name}')
                item.uid = item.gid = 0
                item.uname = item.gname = 'root'
                item.mtime = epoch
                repacked.addfile(item, original.extractfile(item) if item.isfile() else None)
    print(output)
