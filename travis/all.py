import argparse, os, sys, time

parser = argparse.ArgumentParser()
parser.add_argument('-n', '--dry-run', action='store_true', help='Display commands without running them')
args = parser.parse_args()

red = '\x1b[31m\x1b[1m'
green = '\x1b[32m\x1b[1m'
reset = '\x1b(B\x1b[m'
if os.environ.get('TRAVIS', '') == 'true':
    print('This script cannot be used in a travis build')
    sys.exit(1)
os.chdir(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
commands = []
with open('.travis.yml') as f:
    lines = list(f.readlines())
    script_found = False
    for line in lines:
        if line.startswith('script:'):
            script_found = True
        elif script_found:
            if line.startswith('- '):
                if line.startswith('- python '):
                    commands.append(line[2:].rstrip('\r\n'))
            else:
                break
ret = 0
for cmd in commands:
    print('$ %s' % cmd)
    if args.dry_run:
        continue
    start = time.time()
    code = os.system(cmd)
    end = time.time()
    if code != 0:
        ret = 1
    print('\n%sThe command "%s" exited with %i.%s [%.3f secs]' %
        (green if code == 0 else red, cmd, code, reset, end - start))

print('\nDone. Your build exited with %i.' % ret)
sys.exit(ret)
