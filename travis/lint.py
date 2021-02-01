#!/usr/bin/env python3
import re, os, sys

valid_extensions = ['c', 'cpp', 'h', 'hpp', 'mm', 'lua', 'rb', 'proto',
                    'init', 'init-example', 'rst']
path_blacklist = [
    '^library/include/df/',
    '^plugins/stonesense/allegro',
    '^plugins/isoworld/allegro',
    '^plugins/isoworld/agui',
    '^depends/',
    '^.git/',
    '^build',
    '.pb.h',
]

def valid_file(filename):
    return len(list(filter(lambda ext: filename.endswith('.' + ext), valid_extensions))) and \
        not len(list(filter(lambda path: path.replace('\\', '/') in filename.replace('\\', '/'), path_blacklist)))

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
    ignore = os.linesep != '\n' and not os.environ.get('TRAVIS')
    def check_line(self, line):
        return '\r' not in line
    def fix_line(self, line):
        return line.replace('\r', '')

class TrailingWhitespaceLinter(Linter):
    msg = 'Contains trailing whitespace'
    def check_line(self, line):
        line = line.replace('\r', '').replace('\n', '')
        return not line.strip() or line == line.rstrip('\t ')
    def fix_line(self, line):
        return line.rstrip('\t ')

class TabLinter(Linter):
    msg = 'Contains tabs'
    def check_line(self, line):
        return '\t' not in line
    def fix_line(self, line):
        return line.replace('\t', '    ')

linters = [cls() for cls in Linter.__subclasses__() if not cls.ignore]

def main():
    is_github_actions = bool(os.environ.get('GITHUB_ACTIONS'))
    root_path = os.path.abspath(sys.argv[1] if len(sys.argv) > 1 else '.')
    if not os.path.exists(root_path):
        print('Nonexistent path: %s' % root_path)
        sys.exit(2)
    fix = (len(sys.argv) > 2 and sys.argv[2] == '--fix')
    global path_blacklist
    path_blacklist = list(map(lambda s: os.path.join(root_path, s.replace('^', '')) if s.startswith('^') else s, path_blacklist))

    for cur, dirnames, filenames in os.walk(root_path):
        for filename in filenames:
            full_path = os.path.join(cur, filename)
            rel_path = full_path.replace(root_path, '.')
            if not valid_file(full_path):
                continue
            lines = []
            with open(full_path, 'rb') as f:
                lines = f.read().split(b'\n')
                for i, line in enumerate(lines):
                    try:
                        lines[i] = line.decode('utf-8')
                    except UnicodeDecodeError:
                        msg_params = (rel_path, i + 1, 'Invalid UTF-8 (other errors will be ignored)')
                        error('%s:%i: %s' % msg_params)
                        if is_github_actions:
                            print('::error file=%s,line=%i::%s' % msg_params)
                        lines[i] = ''
            for linter in linters:
                try:
                    linter.check(lines)
                except LinterError as e:
                    error('%s: %s' % (rel_path, e))
                    if is_github_actions:
                        print(e.github_actions_workflow_command(rel_path))
                    if fix:
                        linter.fix(lines)
                        contents = '\n'.join(lines)
                        with open(full_path, 'wb') as f:
                            f.write(contents)

    if success:
        print('All linters completed successfully')
        sys.exit(0)
    else:
        sys.exit(1)

if __name__ == '__main__':
    main()
