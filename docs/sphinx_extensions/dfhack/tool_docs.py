# useful references:
#   https://www.sphinx-doc.org/en/master/extdev/appapi.html
#   https://www.sphinx-doc.org/en/master/development/tutorials/recipe.html
#   https://www.sphinx-doc.org/en/master/usage/restructuredtext/basics.html#rst-directives

from typing import List

import docutils.nodes as nodes
import docutils.parsers.rst.directives as rst_directives
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

    @staticmethod
    def make_labeled_paragraph(label, content, label_class=nodes.strong, content_class=nodes.inline) -> nodes.paragraph:
        return nodes.paragraph('', '', *[
            label_class('', '{}: '.format(label)),
            content_class('', content),
        ])

    @staticmethod
    def wrap_box(*children: List[nodes.Node]) -> nodes.Admonition:
        return nodes.admonition('', *children, classes=['dfhack-tool-summary'])

    def render_content(self) -> List[nodes.Node]:
        raise NotImplementedError

    def run(self):
        return [self.wrap_box(*self.render_content())]


class DFHackToolDirective(DFHackToolDirectiveBase):
    option_spec = {
        'tags': dfhack.util.directive_arg_str_list,
        'no-command': rst_directives.flag,
    }

    def render_content(self) -> List[nodes.Node]:
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

    def run(self):
        out = DFHackToolDirectiveBase.run(self)
        if 'no-command' not in self.options:
            out += [self.wrap_box(*DFHackCommandDirective.render_content(self))]
        return out


class DFHackCommandDirective(DFHackToolDirectiveBase):
    def render_content(self) -> List[nodes.Node]:
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
