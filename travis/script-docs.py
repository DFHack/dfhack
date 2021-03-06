#!/usr/bin/env python3
import os
from os.path import basename, dirname, join, splitext
import sys

SCRIPT_PATH = sys.argv[1] if len(sys.argv) > 1 else 'scripts'
IS_GITHUB_ACTIONS = bool(os.environ.get('GITHUB_ACTIONS'))

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
        print_error('missing leading comment (requred for `ls`)', fname)
        return 1
    return 0


def print_error(message, filename, line=None):
    if not isinstance(line, int):
        line = 1
    print('Error: %s:%i: %s' % (filename, line, message))
    if IS_GITHUB_ACTIONS:
        print('::error file=%s,line=%i::%s' % (filename, line, message))


def check_file(fname):
    errors, doclines = 0, []
    tok1, tok2 = ('=begin', '=end') if fname.endswith('.rb') else \
        ('[====[', ']====]')
    doc_start_line = None
    with open(fname, errors='ignore') as f:
        lines = f.readlines()
        if not lines:
            print_error('empty file', fname)
            return 1
        errors += check_ls(fname, lines[0])
        for i, l in enumerate(lines):
            if doclines or l.strip().endswith(tok1):
                if not doclines:
                    doc_start_line = i + 1
                doclines.append(l.rstrip())
            if l.startswith(tok2):
                break
        else:
            if doclines:
                print_error('docs start but do not end', fname, doc_start_line)
            else:
                print_error('no documentation found', fname)
            return 1

    if not doclines:
        print_error('missing or malformed documentation', fname)
        return 1

    title, underline = [d for d in doclines
                        if d and '=begin' not in d and '[====[' not in d][:2]
    title_line = doc_start_line + doclines.index(title)
    expected_underline = '=' * len(title)
    if underline != expected_underline:
        print_error('title/underline mismatch: expected {!r}, got {!r}'.format(
                expected_underline, underline),
            fname, title_line + 1)
        errors += 1
    if title != expected_cmd(fname):
        print_error('expected script title {!r}, got {!r}'.format(
                expected_cmd(fname), title),
            fname, title_line)
        errors += 1
    return errors


def main():
    """Check that all DFHack scripts include documentation"""
    err = 0
    exclude = set(['internal', 'test'])
    for root, dirs, files in os.walk(SCRIPT_PATH, topdown=True):
        dirs[:] = [d for d in dirs if d not in exclude]
        for f in files:
            if f[-3:] in {'.rb', 'lua'}:
                err += check_file(join(root, f))
    return err


if __name__ == '__main__':
    sys.exit(min(100, main()))
