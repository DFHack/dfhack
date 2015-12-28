from __future__ import print_function
from io import open
import os
from os.path import basename, dirname, join, splitext
import sys


def expected_cmd(path):
    """Get the command from the name of a script."""
    dname, fname = basename(dirname(path)), splitext(basename(path))[0]
    if dname in ('devel', 'fix', 'gui', 'modtools'):
        return dname + '/' + fname
    return fname


def check_file(fname):
    errors, doclines = 0, []
    with open(fname, errors='ignore') as f:
        for l in f.readlines():
            if not l.strip():
                continue
            if doclines or l.strip().endswith('=begin'):
                doclines.append(l.rstrip())
            if l.startswith('=end'):
                break
        else:
            if doclines:
                print('Error: docs start but not end: ' + fname)
            else:
                print('Error: no documentation in: ' + fname)
            return 1
    title, underline = doclines[1], doclines[2]
    if underline != '=' * len(title):
        print('Error: title/underline mismatch:', fname, title, underline)
        errors += 1
    if title != expected_cmd(fname):
        print('Warning: expected script title {}, got {}'.format(
              expected_cmd(fname), title))
        errors += 1
    return errors


def main():
    """Check that all DFHack scripts include documentation (not 3rdparty)"""
    err = 0
    for root, _, files in os.walk('scripts'):
        for f in files:
            # TODO: remove 3rdparty exemptions from checks
            # Requires reading their CMakeLists to only apply to used scripts
            if f[-3:] in {'.rb', 'lua'} and '3rdparty' not in root:
                err += check_file(join(root, f))
    return err


if __name__ == '__main__':
    sys.exit(bool(main()))
