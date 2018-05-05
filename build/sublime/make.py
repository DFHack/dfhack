from __future__ import print_function
import argparse
import os
import re
import sys
import subprocess

class BuildError(Exception): pass

parser = argparse.ArgumentParser()
parser.add_argument('file', nargs='?', default='', help='current filename')
parser.add_argument('-a', '--all', action='store_true', help='Build all targets')
parser.add_argument('-i', '--install', action='store_true', help='Install')
parser.add_argument('-p', '--plugin', action='store_true', help='Build specified plugin')

def find_build_folder():
    # search for the one with the most recently modified Makefile
    folder = None
    mtime = 0
    for f in os.listdir('.'):
        if f.startswith('build'):
            makefile = os.path.join(f, 'Makefile')
            if os.path.isfile(makefile) and os.path.getmtime(makefile) > mtime:
                folder = f
                mtime = os.path.getmtime(makefile)
    if not folder:
        raise BuildError('No valid build folder found')
    return folder

def get_plugin_name(filename):
    filename = filename.replace('/devel', '')
    match = re.search(r'plugins/(.+?)[/\.]', filename)
    if match:
        return match.group(1)

def run_command(cmd):
    print('$ ' + ' '.join(cmd))
    sys.stdout.flush()
    if subprocess.call(cmd) != 0:
        raise BuildError('command execution failed: ' + ' '.join(cmd))

def main(args):
    os.chdir(find_build_folder())
    print('Build folder:', os.getcwd())
    cmd = ['make', '-j3']

    if args.plugin:
        plugin = get_plugin_name(args.file)
        if not plugin:
            raise BuildError('Cannot determine current plugin name from %r' % args.file)
        cmd += [plugin + '/fast']

    run_command(cmd)

    if args.install:
        run_command(['make', 'install/fast'])

if __name__ == '__main__':
    try:
        main(parser.parse_args())
    except BuildError as e:
        print('** Error: ' + str(e))
        sys.exit(1)
