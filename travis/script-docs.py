from __future__ import print_function
from io import open
import os
from os.path import basename, dirname, join, splitext
import sys

SCRIPT_PATH = sys.argv[1] if len(sys.argv) > 1 else 'scripts'

def expected_cmd(path):
    """Get the command from the name of a script."""
    dname, fname = basename(dirname(path)), splitext(basename(path))[0]
    if dname in ('devel', 'fix', 'gui', 'modtools'):
        return dname + '/' + fname
    return fname


def check_ls(fname, line):
    """Check length & existence of leading comment for "ls" builtin command."""
    line = line.strip()
    comment = '--' if fname.endswith('.lua') else '#'
    if '[====[' in line or not line.startswith(comment):
        print('Error: no leading comment in ' + fname)
        return 1
    return 0


def check_file(fname):
    errors, doclines = 0, []
    tok1, tok2 = ('=begin', '=end') if fname.endswith('.rb') else \
        ('[====[', ']====]')
    with open(fname, errors='ignore') as f:
        lines = f.readlines()
        errors += check_ls(fname, lines[0])
        for l in lines:
            if doclines or l.strip().endswith(tok1):
                doclines.append(l.rstrip())
            if l.startswith(tok2):
                break
        else:
            if doclines:
                print('Error: docs start but not end: ' + fname)
            else:
                print('Error: no documentation in: ' + fname)
            return 1

    if not doclines:
        print('Error: missing or malformed documentation in: ' + fname)
        return 1

    title, underline = [d for d in doclines
                        if d and '=begin' not in d and '[====[' not in d][:2]
    if underline != '=' * len(title):
        print('Error: title/underline mismatch:', fname, title, underline)
        errors += 1
    if title != expected_cmd(fname):
        print('Warning: expected script title {}, got {}'.format(
              expected_cmd(fname), title))
        errors += 1
    return errors


def main():
    """Check that all DFHack scripts include documentation"""
    err = 0
    for root, _, files in os.walk(SCRIPT_PATH):
        for f in files:
            if f[-3:] in {'.rb', 'lua'}:
                err += check_file(join(root, f))
    return err


if __name__ == '__main__':
    sys.exit(min(100, main()))
