# adapted from:
#   https://stackoverflow.com/a/16470058
#   https://pygments.org/docs/lexerdevelopment/

import re

from pygments.lexer import RegexLexer
from pygments.token import Comment, Generic, Text
from sphinx.highlighting import lexers

class DFHackLexer(RegexLexer):
    name = 'DFHack'
    aliases = ['dfhack']
    flags = re.IGNORECASE | re.MULTILINE

    tokens = {
        'root': [
            (r'\#.+$', Comment.Single),
            (r'^\[[a-z]+\]\# ', Generic.Prompt),
            (r'.+?', Text),
        ]
    }

def register_lexer(app):
    lexers['dfhack'] = DFHackLexer()

def setup(app):
    app.connect('builder-inited', register_lexer)

    return {
        'version': '0.1',
        'parallel_read_safe': True,
        'parallel_write_safe': True,
    }
