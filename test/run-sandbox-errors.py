#!/usr/bin/env python3
# Copyright (C) 2026 Qore Technologies, s.r.o.
# SPDX-License-Identifier: MIT
"""Require caught sandbox errors without an abandoned exception-sink diagnostic."""
import argparse
from pathlib import Path
import re
import subprocess
import sys


# Installing the optional XML module invalidates QUnit's precompiled branch.
# Keep the exact loader diagnostic visible; no sandbox diagnostic is permitted.
QUNIT_XML_FALLBACK = re.compile(
    r"warning: binary module '(/usr/lib64/qore-modules/([0-9]+\.[0-9]+\.[0-9]+)/QUnit\.qmod)' "
    r"for feature 'QUnit' failed to load; loading source module "
    r"'/usr/share/qore-modules/\2/QUnit\.qm' instead: AOT-MODULE-STALE: AOT module '\1' "
    r"was compiled when optional module 'xml >= 1\.3' was not available, "
    r"but it is available now; rebuild the binary module\n"
)


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--module', type=Path, required=True)
    parser.add_argument('--allow-qunit-xml-fallback', action='store_true',
                        help='accept only QUnit\'s exact optional-XML AOT fallback warning')
    args = parser.parse_args(argv)
    module = args.module.resolve(strict=True)
    if module.suffix != '.qmod':
        parser.error('--module must name a binary Qore module')
    result = subprocess.run(['qore', '-b', '--enable-debug', '-l', str(module),
                             str(Path(__file__).with_name('zmq-sandbox-errors.qtest'))],
                            stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    sys.stdout.write(result.stdout)
    sys.stderr.write(result.stderr)
    result.check_returncode()
    if result.stderr and not (args.allow_qunit_xml_fallback
                              and QUNIT_XML_FALLBACK.fullmatch(result.stderr)):
        raise RuntimeError('Caught sandbox errors produced unexpected stderr')


if __name__ == '__main__':
    main()
