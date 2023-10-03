#!/usr/bin/env python3
import argparse
import enum
import json
import os
import re
import shutil
import subprocess
import sys

parser = argparse.ArgumentParser()
parser.add_argument('df_folder', help='DF base folder')
parser.add_argument('--headless', action='store_true',
    help='Run without opening DF window (requires non-Windows)')
parser.add_argument('--keep-status', action='store_true',
    help='Do not delete final status file')
parser.add_argument('--no-quit', action='store_true',
    help='Do not quit DF when done')
parser.add_argument('--test-dir', '--test-folder',
    help='Base test folder (default: df_folder/test)')
parser.add_argument('-t', '--test', dest='tests', nargs='+',
    help='Test(s) to run (Lua patterns accepted)')
args = parser.parse_args()

if (not sys.stdin.isatty() or not sys.stdout.isatty() or not sys.stderr.isatty()) and not args.headless:
    print('WARN: no TTY detected, enabling headless mode')
    args.headless = True

if args.test_dir is not None:
    args.test_dir = os.path.normpath(os.path.join(os.getcwd(), args.test_dir))
    if not os.path.isdir(args.test_dir):
        print('ERROR: invalid test folder: %r' % args.test_dir)

MAX_TRIES = 5

dfhack = 'Dwarf Fortress.exe' if sys.platform == 'win32' else './dfhack'
test_status_file = 'test_status.json'

class TestStatus(enum.Enum):
    PENDING = 'pending'
    PASSED = 'passed'
    FAILED = 'failed'

def get_test_status():
    if os.path.isfile(test_status_file):
        with open(test_status_file) as f:
            return {k: TestStatus(v) for k, v in json.load(f).items()}

def change_setting(content, setting, value):
    return '[' + setting + ':' + value + ']\n' + re.sub(
        r'\[' + setting + r':.+?\]', '(overridden)', content, flags=re.IGNORECASE)

os.chdir(args.df_folder)
if os.path.exists(test_status_file):
    os.remove(test_status_file)

print('Backing up init.txt to init.txt.orig')
default_init_txt_path = 'data/init/init_default.txt'
prefs_path = 'prefs'
init_txt_path = 'prefs/init.txt'
if not os.path.exists(init_txt_path):
    os.makedirs(prefs_path, exist_ok=True)
    shutil.copyfile(default_init_txt_path, init_txt_path)

shutil.copyfile(init_txt_path, init_txt_path + '.orig')
with open(init_txt_path) as f:
    init_contents = f.read()
init_contents = change_setting(init_contents, 'SOUND', 'NO')
init_contents = change_setting(init_contents, 'WINDOWED', 'YES')
init_contents = change_setting(init_contents, 'WINDOWEDX', '1200')
init_contents = change_setting(init_contents, 'WINDOWEDY', '800')
#if args.headless:
#    init_contents = change_setting(init_contents, 'PRINT_MODE', 'TEXT')

init_path = 'dfhack-config/init'
if not os.path.isdir('hack/init'):
    # we're on an old branch that still reads init files from the root dir
    init_path = '.'
os.makedirs(init_path, exist_ok=True)
test_init_file = os.path.join(init_path, 'dfhackzzz_test.init')  # Core sorts these alphabetically
with open(test_init_file, 'w') as f:
    f.write('''
    devel/dump-rpc dfhack-rpc.txt
    test --resume -- lua scr.breakdown_level=df.interface_breakdown_types.%s
    ''' % ('NONE' if args.no_quit else 'QUIT'))

test_config_file = 'test_config.json'
with open(test_config_file, 'w') as f:
    json.dump({
        'test_dir': args.test_dir,
        'tests': args.tests,
    }, f)

try:
    with open(init_txt_path, 'w') as f:
        f.write(init_contents)

    tries = 0
    while True:
        status = get_test_status()
        if status is not None:
            if all(s != TestStatus.PENDING for s in status.values()):
                print('Done!')
                sys.exit(int(any(s != TestStatus.PASSED for s in status.values())))
        elif tries > 0:
            print('ERROR: Could not read status file')
            sys.exit(2)

        tries += 1
        print('Starting DF: #%i' % (tries))
        if tries > MAX_TRIES:
            print('ERROR: Too many tries - aborting')
            sys.exit(1)

        if args.headless:
            os.environ['DFHACK_HEADLESS'] = '1'
            os.environ['DFHACK_DISABLE_CONSOLE'] = '1'

        process = subprocess.Popen([dfhack],
            stdin=subprocess.PIPE if args.headless else sys.stdin,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE)
        out, err = process.communicate()
        if err:
            print('WARN: DF produced stderr: ' + repr(err[:5000]))
            with open('df-raw-stderr.log', 'wb') as f:
                f.write(err)
        if process.returncode != 0:
            print('ERROR: DF exited with ' + repr(process.returncode))
            print('DF stdout: ' + repr(out[:5000]))
finally:
    print('\nRestoring original init.txt')
    shutil.copyfile(init_txt_path + '.orig', init_txt_path)
    if os.path.isfile(test_init_file):
        os.remove(test_init_file)
    if not args.keep_status and os.path.isfile(test_status_file):
        os.remove(test_status_file)
    print('Cleanup done')
