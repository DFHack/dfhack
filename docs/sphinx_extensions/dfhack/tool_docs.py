# useful references:
#   https://www.sphinx-doc.org/en/master/extdev/appapi.html
#   https://www.sphinx-doc.org/en/master/development/tutorials/recipe.html
#   https://www.sphinx-doc.org/en/master/usage/restructuredtext/basics.html#rst-directives

import docutils.nodes as nodes
# import docutils.parsers.rst.directives as rst_directives
import sphinx
import sphinx.addnodes as addnodes
import sphinx.directives

import dfhack.util

class DFHackToolDirectiveBase(sphinx.directives.ObjectDescription):
    has_content = False
    required_arguments = 0
    optional_arguments = 1

    def get_name_or_docname(self):
        if self.arguments:
            return self.arguments[0]
        else:
            return self.env.docname.split('/')[-1]

    def make_labeled_paragraph(self, label, content, label_class=nodes.strong, content_class=nodes.inline):
        return nodes.paragraph('', '', *[
            label_class('', '{}: '.format(label)),
            content_class('', content),
        ])

    def make_nodes(self):
        raise NotImplementedError

    def run(self):
        return [
            nodes.admonition('', *self.make_nodes(), classes=['dfhack-tool-summary']),
        ]


class DFHackToolDirective(DFHackToolDirectiveBase):
    option_spec = {
        'tags': dfhack.util.directive_arg_str_list,
    }

    def make_nodes(self):
        tag_nodes = [nodes.strong(text='Tags: ')]
        for tag in self.options.get('tags', []):
            tag_nodes += [
                addnodes.pending_xref(tag, nodes.inline(text=tag), **{
                    'reftype': 'ref',
                    'refdomain': 'std',
                    'reftarget': 'tag/' + tag,
                    'refexplicit': False,
                    'refwarn': True,
                }),
                nodes.inline(text=' | '),
            ]
        tag_nodes.pop()

        return [
            self.make_labeled_paragraph('Tool', self.get_name_or_docname()),
            nodes.paragraph('', '', *tag_nodes),
        ]


class DFHackCommandDirective(DFHackToolDirectiveBase):
    def make_nodes(self):
        return [
            self.make_labeled_paragraph('Command', self.get_name_or_docname(), content_class=nodes.literal),
        ]


def register(app):
    app.add_directive('dfhack-tool', DFHackToolDirective)
    app.add_directive('dfhack-command', DFHackCommandDirective)

def setup(app):
    app.connect('builder-inited', register)

    return {
        'version': '0.1',
        'parallel_read_safe': True,
        'parallel_write_safe': True,
    }
