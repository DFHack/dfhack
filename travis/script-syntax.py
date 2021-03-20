#!/usr/bin/env python3
import argparse
import os
import subprocess
import sys


def print_stderr(stderr, args):
    if not args.github_actions:
        sys.stderr.write(stderr + '\n')
        return

    for line in stderr.split('\n'):
        print(line)
        parts = list(map(str.strip, line.split(':')))
        # e.g. luac prints "luac:" in front of messages, so find the first part
        # containing the actual filename
        for i in range(len(parts) - 1):
            if parts[i].endswith('.' + args.ext) and parts[i + 1].isdigit():
                print('::error file=%s,line=%s::%s' % (parts[i], parts[i + 1], ':'.join(parts[i + 2:])))
                break


def main(args):
    root_path = os.path.abspath(args.path)
    cmd = args.cmd.split(' ')
    if not os.path.exists(root_path):
        print('Nonexistent path: %s' % root_path)
        sys.exit(2)
    err = False
    for cur, dirnames, filenames in os.walk(root_path):
        parts = cur.replace('\\', '/').split('/')
        if '.git' in parts or 'depends' in parts:
            continue
        for filename in filenames:
            if not filename.endswith('.' + args.ext):
                continue
            full_path = os.path.join(cur, filename)
            try:
                p = subprocess.Popen(cmd + [full_path], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
                _, stderr = p.communicate()
                stderr = stderr.decode('utf-8', errors='ignore')
                if stderr:
                    print_stderr(stderr, args)
                if p.returncode != 0:
                    err = True
            except subprocess.CalledProcessError:
                err = True
            except IOError:
                if not err:
                    print('Warning: cannot check %s script syntax' % args.ext)
                err = True
    sys.exit(int(err))


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--path', default='.', help='Root directory')
    parser.add_argument('--ext', help='Script extension', required=True)
    parser.add_argument('--cmd', help='Command', required=True)
    parser.add_argument('--github-actions', action='store_true',
        help='Enable GitHub Actions workflow command output')
    args = parser.parse_args()
    main(args)
