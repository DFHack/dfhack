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
def error(msg):
    global success
    success = False
    sys.stderr.write(msg + '\n')

class LinterError(Exception): pass

class Linter(object):
    def check(self, lines):
        failures = []
        for i, line in enumerate(lines):
            if not self.check_line(line):
                failures.append(i + 1)
        if len(failures):
            raise LinterError('%s: %s' % (self.msg, self.display_lines(failures, len(lines))))

    def fix(self, lines):
        for i in range(len(lines)):
            lines[i] = self.fix_line(lines[i])

    def display_lines(self, lines, total):
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

class NewlineLinter(Linter):
    msg = 'Contains DOS-style newlines'
    def __init__(self):
        # git supports newline conversion.  Catch in CI, ignore on Windows.
        self.ignore = sys.platform == 'win32' and not os.environ.get('TRAVIS')
    def check_line(self, line):
        return self.ignore or '\r' not in line
    def fix_line(self, line):
        return line.replace('\r', '')

class TrailingWhitespaceLinter(Linter):
    msg = 'Contains trailing whitespace'
    def check_line(self, line):
        line = line.replace('\r', '')
        return not line.strip() or (not line.endswith(' ') and not line.endswith('\t'))
    def fix_line(self, line):
        return line.rstrip('\t ')

class TabLinter(Linter):
    msg = 'Contains tabs'
    def check_line(self, line):
        return '\t' not in line
    def fix_line(self, line):
        return line.replace('\t', '    ')

linters = [cls() for cls in Linter.__subclasses__()]

def main():
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
                        error('%s:%i: Invalid UTF-8 (other errors will be ignored)' % (rel_path, i + 1))
                        lines[i] = ''
            for linter in linters:
                try:
                    linter.check(lines)
                except LinterError as e:
                    error('%s: %s' % (rel_path, e))
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
