import os

DFHACK_ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
DOCS_ROOT = os.path.join(DFHACK_ROOT, 'docs')

if not os.path.isdir(DOCS_ROOT):
    raise ReferenceError('docs root not found: %s' % DOCS_ROOT)
