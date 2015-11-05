from io import open
import os
import sys

scriptdirs = (
    'scripts',
    #'scripts/devel',  # devel scripts don't have to be documented
    'scripts/fix',
    'scripts/gui',
    'scripts/modtools')


def check_file(fname):
    doclines = []
    with open(fname, errors='ignore') as f:
        for l in f.readlines():
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
    title, underline = doclines[2], doclines[3]
    if underline != '=' * len(title):
        print('Error: title/underline mismatch:', fname, title, underline)
        return 1
    start = fname.split('/')[-2]
    if start != 'scripts' and not title.startswith(start):
        print('Error: title is missing start string: {} {} {}'.format(fname, start, title))
        return 1
    return 0


def main():
    """Check that all DFHack scripts include documentation (not 3rdparty)"""
    errors = 0
    for path in scriptdirs:
        for f in os.listdir(path):
            f = path + '/' + f
            if os.path.isfile(f) and f[-3:] in {'.rb', 'lua'}:
                errors += check_file(f)
    return errors


if __name__ == '__main__':
    sys.exit(bool(main()))
