import contextlib
import io
import os

DFHACK_ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
DOCS_ROOT = os.path.join(DFHACK_ROOT, 'docs')

if not os.path.isdir(DOCS_ROOT):
    raise ReferenceError('docs root not found: %s' % DOCS_ROOT)


@contextlib.contextmanager
def write_file_if_changed(path):
    with io.StringIO() as buffer:
        yield buffer
        new_contents = buffer.getvalue()

    try:
        with open(path, 'r') as infile:
            old_contents = infile.read()
    except IOError:
        old_contents = None

    if old_contents != new_contents:
        with open(path, 'w') as outfile:
            outfile.write(new_contents)


# directive argument helpers (supplementing docutils.parsers.rst.directives)
def directive_arg_str_list(argument):
    """
    Converts a space- or comma-separated list of values into a Python list
    of strings.
    (Directive option conversion function.)
    """
    if ',' in argument:
        entries = argument.split(',')
    else:
        entries = argument.split()
    return [entry.strip() for entry in entries]
