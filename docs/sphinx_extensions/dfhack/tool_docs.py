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
    required_arguments = 1
    option_spec = {
        'tags': dfhack.util.directive_arg_str_list,
    }

    def run(self):
        tool_name = self.get_signatures()[0]

        tag_nodes = [nodes.strong(text='Tags: ')]
        for tag in self.options['tags']:
            tag_nodes += [
                nodes.literal(tag, tag),
                nodes.inline(text=' | '),
            ]
        tag_nodes.pop()

        return [
            nodes.title(text=tool_name),
            nodes.paragraph('', '', *tag_nodes),
        ]

    # simpler but always renders as "inline code" (and less customizable)
    # def handle_signature(self, sig, signode):
    #     signode += addnodes.desc_name(text=sig)
    #     return sig


def register(app):
    app.add_directive('dfhack-tool', DFHackToolDirective)

def setup(app):
    app.connect('builder-inited', register)

    return {
        'version': '0.1',
        'parallel_read_safe': True,
        'parallel_write_safe': True,
    }
