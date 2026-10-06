#!/usr/bin/env python3
# Copyright (C) 2026 Qore Technologies, s.r.o.
# SPDX-License-Identifier: MIT
"""Require caught sandbox errors without an abandoned exception-sink diagnostic."""
import argparse
from pathlib import Path
import subprocess
import sys


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--module', type=Path, required=True)
    args = parser.parse_args()
    module = args.module.resolve(strict=True)
    if module.suffix != '.qmod':
        parser.error('--module must name a binary Qore module')
    result = subprocess.run(['qore', '-b', '--enable-debug', '-l', str(module),
                             str(Path(__file__).with_name('zmq-sandbox-errors.qtest'))],
                            stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    sys.stdout.write(result.stdout)
    sys.stderr.write(result.stderr)
    result.check_returncode()
    if result.stderr:
        raise RuntimeError('Caught sandbox errors produced unexpected stderr')


if __name__ == '__main__':
    main()
