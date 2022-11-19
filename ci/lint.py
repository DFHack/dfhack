#!/usr/bin/env python3
import argparse
import fnmatch
import re
import os
import subprocess
import sys

DFHACK_ROOT = os.path.normpath(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

def load_pattern_files(paths):
    patterns = []
    for p in paths:
        with open(p) as f:
            for line in f.readlines():
                line = line.strip()
                if line and not line.startswith('#'):
                    patterns.append(line)
    return patterns

def valid_file(rel_path, check_patterns, ignore_patterns):
    return (
        any(fnmatch.fnmatch(rel_path, pattern) for pattern in check_patterns)
        and not any(fnmatch.fnmatch(rel_path, pattern) for pattern in ignore_patterns)
    )

success = True
def error(msg=None):
    global success
    success = False
    if msg:
        sys.stderr.write(msg + '\n')

def format_lines(lines, total):
    if len(lines) == total - 1:
        return 'entire file'
    if not len(lines):
        # should never happen
        return 'nowhere'
    if len(lines) == 1:
        return 'line %i' % lines[0]
    s = 'lines '
    range_start = range_end = lines[0]
    for i, line in enumerate(lines):
        if line > range_end + 1:
            if range_start == range_end:
                s += ('%i, ' % range_end)
            else:
                s += ('%i-%i, ' % (range_start, range_end))
            range_start = range_end = line
            if i == len(lines) - 1:
                s += ('%i' % line)
        else:
            range_end = line
            if i == len(lines) - 1:
                s += ('%i-%i, ' % (range_start, range_end))
    return s.rstrip(' ').rstrip(',')

class LinterError(Exception):
    def __init__(self, message, lines, total_lines):
        self.message = message
        self.lines = lines
        self.total_lines = total_lines

    def __str__(self):
        return '%s: %s' % (self.message, format_lines(self.lines, self.total_lines))

    def github_actions_workflow_command(self, filename):
        first_line = self.lines[0] if self.lines else 1
        return '::error file=%s,line=%i::%s' % (filename, first_line, self)

class Linter(object):
    ignore = False
    def check(self, lines):
        failures = []
        for i, line in enumerate(lines):
            if not self.check_line(line):
                failures.append(i + 1)
        if len(failures):
            raise LinterError(self.msg, failures, len(lines))

    def fix(self, lines):
        for i in range(len(lines)):
            lines[i] = self.fix_line(lines[i])


class NewlineLinter(Linter):
    msg = 'Contains DOS-style newlines'
    # git supports newline conversion.  Catch in CI, ignore on Windows.
    ignore = os.linesep != '\n' and not os.environ.get('CI')
    def check_line(self, line):
        return '\r' not in line
    def fix_line(self, line):
        return line.replace('\r', '')

class TrailingWhitespaceLinter(Linter):
    msg = 'Contains trailing whitespace'
    def check_line(self, line):
        line = line.replace('\r', '').replace('\n', '')
        return line == line.rstrip('\t ')
    def fix_line(self, line):
        return line.rstrip('\t ')

class TabLinter(Linter):
    msg = 'Contains tabs'
    def check_line(self, line):
        return '\t' not in line
    def fix_line(self, line):
        return line.replace('\t', '    ')

linters = [cls() for cls in Linter.__subclasses__() if not cls.ignore]

def walk_all(root_path):
    for cur, dirnames, filenames in os.walk(root_path):
        for filename in filenames:
            full_path = os.path.join(cur, filename)
            yield full_path

def walk_git_files(root_path):
    p = subprocess.Popen(['git', '-C', root_path, 'ls-files', root_path], stdout=subprocess.PIPE)
    for line in p.stdout.readlines():
        path = line.decode('utf-8').strip()
        full_path = os.path.join(root_path, path)
        yield full_path
    if p.wait() != 0:
        raise RuntimeError('git exited with %r' % p.returncode)

def main(args):
    root_path = os.path.abspath(args.path)
    if not os.path.exists(args.path):
        print('Nonexistent path: %s' % root_path)
        sys.exit(2)

    check_patterns = load_pattern_files(args.check_patterns)
    ignore_patterns = load_pattern_files(args.ignore_patterns)

    walk_iter = walk_all
    if args.git_only:
        walk_iter = walk_git_files

    for full_path in walk_iter(root_path):
        rel_path = full_path.replace(root_path, '').replace('\\', '/').lstrip('/')
        if not valid_file(rel_path, check_patterns, ignore_patterns):
            continue
        if args.verbose:
            print('Checking:', rel_path)
        lines = []
        with open(full_path, 'rb') as f:
            lines = f.read().split(b'\n')
            for i, line in enumerate(lines):
                try:
                    lines[i] = line.decode('utf-8')
                except UnicodeDecodeError:
                    msg_params = (rel_path, i + 1, 'Invalid UTF-8 (other errors will be ignored)')
                    error('%s:%i: %s' % msg_params)
                    if args.github_actions:
                        print('::error file=%s,line=%i::%s' % msg_params)
                    lines[i] = ''
        for linter in linters:
            try:
                linter.check(lines)
            except LinterError as e:
                error('%s: %s' % (rel_path, e))
                if args.github_actions:
                    print(e.github_actions_workflow_command(rel_path))
                if args.fix:
                    linter.fix(lines)
                    contents = '\n'.join(lines)
                    with open(full_path, 'wb') as f:
                        f.write(contents.encode('utf-8'))

    if success:
        print('All linters completed successfully')
        sys.exit(0)
    else:
        sys.exit(1)

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('path', nargs='?', default='.',
        help='Path to scan (default: current directory)')
    parser.add_argument('--fix', action='store_true',
        help='Attempt to modify files in-place to fix identified issues')
    parser.add_argument('--git-only', action='store_true',
        help='Only check files tracked by git')
    parser.add_argument('--github-actions', action='store_true',
        help='Enable GitHub Actions workflow command output')
    parser.add_argument('-v', '--verbose', action='store_true',
        help='Log files as they are checked')
    parser.add_argument('--check-patterns', action='append',
        default=[os.path.join(DFHACK_ROOT, 'ci', 'lint-check.txt')],
        help='File(s) containing filename patterns to check')
    parser.add_argument('--ignore-patterns', action='append',
        default=[os.path.join(DFHACK_ROOT, 'ci', 'lint-ignore.txt')],
        help='File(s) containing filename patterns to ignore')
    args = parser.parse_args()
    main(args)
