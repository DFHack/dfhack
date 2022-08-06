# useful references:
#   https://www.sphinx-doc.org/en/master/extdev/appapi.html
#   https://www.sphinx-doc.org/en/master/development/tutorials/recipe.html
#   https://www.sphinx-doc.org/en/master/usage/restructuredtext/basics.html#rst-directives

import docutils.nodes as nodes
# import docutils.parsers.rst.directives as rst_directives
import sphinx
import sphinx.directives

import dfhack.util

class DFHackToolDirective(sphinx.directives.ObjectDescription):
    has_content = False
    required_arguments = 0
    option_spec = {
        'tags': dfhack.util.directive_arg_str_list,
    }

    def run(self):
        if self.arguments:
            tool_name = self.arguments[0]
        else:
            tool_name = self.env.docname.split('/')[-1]

        tag_nodes = [nodes.strong(text='Tags: ')]
        for tag in self.options.get('tags', []):
            tag_nodes += [
                nodes.literal(tag, tag),
                nodes.inline(text=' | '),
            ]
        tag_nodes.pop()

        return [
            nodes.section('', nodes.title(text=tool_name), ids=[tool_name]),
            nodes.paragraph('', '', *tag_nodes),
        ]


def register(app):
    app.add_directive('dfhack-tool', DFHackToolDirective)

def setup(app):
    app.connect('builder-inited', register)

    return {
        'version': '0.1',
        'parallel_read_safe': True,
        'parallel_write_safe': True,
    }
