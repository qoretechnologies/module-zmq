#!/usr/bin/env python3
# Copyright (C) 2026 Qore Technologies, s.r.o.
# SPDX-License-Identifier: MIT
"""Verify that the optional loader allowance cannot hide sandbox errors."""
import contextlib
import importlib.util
import io
from pathlib import Path
import subprocess
import tempfile
import unittest
from unittest import mock

SPEC = importlib.util.spec_from_file_location(
    'sandbox_runner', Path(__file__).with_name('run-sandbox-errors.py'))
RUNNER = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(RUNNER)
WARNING = (
    "warning: binary module '/usr/lib64/qore-modules/3.0.0/QUnit.qmod' for feature 'QUnit' "
    "failed to load; loading source module '/usr/share/qore-modules/3.0.0/QUnit.qm' instead: "
    "AOT-MODULE-STALE: AOT module '/usr/lib64/qore-modules/3.0.0/QUnit.qmod' was compiled "
    "when optional module 'xml >= 1.3' was not available, but it is available now; "
    "rebuild the binary module\n"
)


class SandboxRunnerTest(unittest.TestCase):
    def run_fixture(self, stderr='', allow=False, status=0):
        with tempfile.TemporaryDirectory() as tmp:
            binary = Path(tmp) / 'zmq-api-2.0.qmod'
            binary.touch()
            argv = ['--module', str(binary)]
            if allow:
                argv.append('--allow-qunit-xml-fallback')
            result = subprocess.CompletedProcess([], status, 'sandbox cases passed\n', stderr)
            output, errors = io.StringIO(), io.StringIO()
            with mock.patch.object(RUNNER.subprocess, 'run', return_value=result) as run:
                with contextlib.redirect_stdout(output), contextlib.redirect_stderr(errors):
                    RUNNER.main(argv)
                self.assertEqual(run.call_args.args[0][:5],
                                 ['qore', '-b', '--enable-debug', '-l', str(binary)])
            self.assertEqual(output.getvalue(), result.stdout)
            self.assertEqual(errors.getvalue(), stderr)

    def test_empty_stderr_passes_in_both_modes(self):
        for allow in (False, True):
            with self.subTest(allow=allow):
                self.run_fixture(allow=allow)

    def test_exact_warning_requires_opt_in_and_remains_visible(self):
        with self.assertRaisesRegex(RuntimeError, 'unexpected stderr'):
            self.run_fixture(WARNING)
        self.run_fixture(WARNING, allow=True)

    def test_failed_qore_status_is_never_accepted(self):
        for stderr in ('', WARNING):
            with self.subTest(stderr=stderr), self.assertRaises(subprocess.CalledProcessError):
                self.run_fixture(stderr, allow=True, status=1)

    def test_other_diagnostics_are_rejected(self):
        variants = [
            'NETWORK-ACCESS-DENIED: abandoned exception\n',
            WARNING + 'NETWORK-ACCESS-DENIED: abandoned exception\n',
            'unexpected prefix\n' + WARNING,
            WARNING + WARNING,
            WARNING.replace('QUnit', 'HttpServer'),
            WARNING.replace('xml >= 1.3', 'msgpack'),
            WARNING.replace('AOT-MODULE-STALE', 'AOT-MODULE-ERROR'),
            WARNING.replace('/usr/share/qore-modules/3.0.0', '/usr/share/qore-modules/3.1.0'),
            WARNING.replace('/usr/lib64', '/tmp'),
            WARNING.replace('not available, but it is available now',
                            'available, but it is not available now'),
            WARNING.replace('rebuild the binary module', 'invalid binary module'),
            WARNING.rstrip('\n'),
        ]
        for stderr in variants:
            with self.subTest(stderr=stderr), self.assertRaisesRegex(RuntimeError, 'unexpected stderr'):
                self.run_fixture(stderr, allow=True)


if __name__ == '__main__':
    unittest.main()
