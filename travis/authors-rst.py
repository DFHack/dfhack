""" Overly-complicated script to check formatting/sorting in Authors.rst """

import re, sys

def main():
    success = [True]
    def error(line, msg, **kwargs):
        info = ''
        for k in kwargs:
            info += ' %s %s:' % (k, kwargs[k])
        print('line %i:%s %s' % (line, info, msg))
        success[0] = False
    with open('docs/Authors.rst', 'rb') as f:
        lines = list(map(lambda line: line.decode('utf8').replace('\n', ''), f.readlines()))

        if lines[1].startswith('='):
            if len(lines[0]) != len(lines[1]):
                error(2, 'Length of header does not match underline')
            if lines[1].replace('=', ''):
                error(2, 'Invalid header')

        first_div_index = list(filter(lambda pair: pair[1].startswith('==='), enumerate(lines[2:])))[0][0] + 2
        first_div = lines[first_div_index]
        div_indices = []
        for i, line in enumerate(lines[first_div_index:]):
            line_number = i + first_div_index + 1
            if '\t' in line:
                error(line_number, 'contains tabs')
            if line.startswith('==='):
                div_indices.append(i + first_div_index)
                if not re.match(r'^=+( =+)+$', line):
                    error(line_number, 'bad table divider')
                if line != lines[first_div_index]:
                    error(line_number, 'malformed table divider')
        if len(div_indices) < 3:
            error(len(lines), 'missing table divider(s)')
        for i in div_indices[3:]:
            error(i + 1, 'extra table divider')

        col_ranges = []
        i = 0
        while True:
            j = first_div.find(' ', i)
            col_ranges.append(slice(i, j if j > 0 else None))
            if j == -1:
                break
            i = j + 1

        for i, line in enumerate(lines[div_indices[1] + 1:div_indices[2]]):
            line_number = i + div_indices[1] + 2
            for c, col in enumerate(col_ranges):
                cell = line[col]
                if cell.startswith(' '):
                    error(line_number, 'text does not start in correct location', column=c+1)
                # check for text extending into next column if this isn't the last column
                if col.stop is not None and col.stop < len(line) and line[col.stop] != ' ':
                    error(line_number, 'text extends into next column', column=c+1)
            if i > 0:
                prev_line = lines[div_indices[1] + i]
                if line.lower()[col_ranges[0]] < prev_line.lower()[col_ranges[0]]:
                    error(line_number, 'not sorted: should come before line %i ("%s" before "%s")' %
                        (line_number - 1, line[col_ranges[0]].rstrip(' '), prev_line[col_ranges[0]].rstrip(' ')))

    return success[0]

if __name__ == '__main__':
    sys.exit(int(not main()))
