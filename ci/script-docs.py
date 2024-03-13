#!/usr/bin/env python3
import os
from os.path import basename, dirname, exists, join, splitext
import sys

SCRIPT_PATH = sys.argv[1] if len(sys.argv) > 1 else 'scripts'
DOCS_PATH = join(SCRIPT_PATH, 'docs')
IS_GITHUB_ACTIONS = bool(os.environ.get('GITHUB_ACTIONS'))

def get_cmd(path):
    """Get the command from the name of a script."""
    dname, fname = basename(dirname(path)), splitext(basename(path))[0]
    if dname in ('devel', 'fix', 'gui', 'modtools'):
        return dname + '/' + fname
    return fname


def print_error(message, filename, line=None):
    if not isinstance(line, int):
        line = 1
    print('Error: %s:%i: %s' % (filename, line, message))
    if IS_GITHUB_ACTIONS:
        print('::error file=%s,line=%i::%s' % (filename, line, message))


def check_ls(docfile, lines):
    """Check length & existence of first sentence for "ls" builtin command."""
    # TODO
    return 0


def check_file(fname):
    errors, doc_start_line = 0, None
    docfile = join(DOCS_PATH, get_cmd(fname)+'.rst')
    if not exists(docfile):
        print_error('missing documentation file: {!r}'.format(docfile), fname)
        return 1
    with open(docfile, errors='ignore') as f:
        lines = f.readlines()
        if not lines:
            print_error('empty documentation file', docfile)
            return 1
        for i, l in enumerate(lines):
            l = l.strip()
            if l and not doc_start_line and doc_start_line != 0 and not l.startswith('..'):
                doc_start_line = i
            doc_end_line = i
            lines[i] = l

    errors += check_ls(docfile, lines)
    title, underline = lines[doc_start_line:doc_start_line+2]
    expected_underline = '=' * len(title)
    if underline != expected_underline:
        print_error('title/underline mismatch: expected {!r}, got {!r}'.format(
                expected_underline, underline),
            docfile, doc_start_line+1)
        errors += 1
    if title != get_cmd(fname):
        print_error('expected script title {!r}, got {!r}'.format(
                get_cmd(fname), title),
            docfile, doc_start_line)
        errors += 1
    return errors


def main():
    """Check that all DFHack scripts include documentation"""
    err = 0
    exclude = {'.git', 'internal', 'test'}
    for root, dirs, files in os.walk(SCRIPT_PATH, topdown=True):
        dirs[:] = [d for d in dirs if d not in exclude]
        for f in files:
            if f.split('.')[-1] in {'rb', 'lua'}:
                err += check_file(join(root, f))
    return err


if __name__ == '__main__':
    sys.exit(min(100, main()))
