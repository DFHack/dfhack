import argparse
import os
import subprocess
import sys

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
                subprocess.check_output(cmd + [full_path])
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
    args = parser.parse_args()
    main(args)
