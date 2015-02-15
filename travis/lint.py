import re, os, sys

valid_extensions = ['c', 'cpp', 'h', 'hpp', 'mm', 'lua', 'rb', 'proto',
                    'init', 'init-example']
path_blacklist = [
    'library/include/df/',
    'plugins/stonesense/allegro',
    'plugins/isoworld/allegro',
    'plugins/isoworld/agui',
]

def valid_file(filename):
    return len(filter(lambda ext: filename.endswith('.' + ext), valid_extensions)) and \
        not len(filter(lambda path: path.replace('\\', '/') in filename.replace('\\', '/'), path_blacklist))

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

    def display_lines(self, lines, total):
        if len(lines) == total - 1:
            return 'entire file'
        if not len(lines):
            # should never happen
            return 'nowhere'
        s = 'lines ' if len(lines) != 1 else 'line '
        range_start = range_end = lines[0]
        for i, line in enumerate(lines):
            if line > range_start + 1 or i == len(lines) - 1:
                if i == len(lines) - 1:
                    range_end = line
                if range_start == range_end:
                    s += ('%i, ' % range_end)
                else:
                    s += ('%i-%i, ' % (range_start, range_end))
                range_start = range_end = line
            else:
                range_end = line
        return s.rstrip(' ').rstrip(',')

class NewlineLinter(Linter):
    msg = 'Contains DOS-style newlines'
    def check_line(self, line):
        return '\r' not in line

class TrailingWhitespaceLinter(Linter):
    msg = 'Contains trailing whitespace'
    def check_line(self, line):
        line = line.replace('\r', '')
        return not line.endswith(' ') and not line.endswith('\t')

linters = [NewlineLinter(), TrailingWhitespaceLinter()]

root_path = os.path.abspath(sys.argv[1] if len(sys.argv) > 1 else '.')
path_blacklist.append(root_path + '/.git')
path_blacklist.append(root_path + '/build')

for cur, dirnames, filenames in os.walk(root_path):
    for filename in filenames:
        full_path = os.path.join(cur, filename)
        rel_path = full_path.replace(root_path, '.')
        if not valid_file(full_path):
            continue
        with open(full_path, 'rb') as f:
            lines = f.read().split('\n')
            for linter in linters:
                try:
                    linter.check(lines)
                except LinterError as e:
                    error('%s: %s' % (rel_path, e))

if success:
    print('All linters completed successfully')
    sys.exit(0)
else:
    sys.exit(1)
