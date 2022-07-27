import os

DFHACK_ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
DOCS_ROOT = os.path.join(DFHACK_ROOT, 'docs')

if not os.path.isdir(DOCS_ROOT):
    raise ReferenceError('docs root not found: %s' % DOCS_ROOT)

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
