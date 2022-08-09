# useful references:
#   https://www.sphinx-doc.org/en/master/extdev/appapi.html
#   https://www.sphinx-doc.org/en/master/development/tutorials/recipe.html
#   https://www.sphinx-doc.org/en/master/usage/restructuredtext/basics.html#rst-directives

import logging
import os
from typing import List

import docutils.nodes as nodes
import docutils.parsers.rst.directives as rst_directives
import sphinx
import sphinx.addnodes as addnodes
import sphinx.directives

import dfhack.util


logger = sphinx.util.logging.getLogger(__name__)

_KEYBINDS = {}
_KEYBINDS_RENDERED = set()  # commands whose keybindings have been rendered

def scan_keybinds(root, files, keybindings):
    """Add keybindings in the specified files to the
    given keybindings dict.
    """
    for file in files:
        with open(os.path.join(root, file)) as f:
            lines = [l.replace('keybinding add', '').strip() for l in f.readlines()
                     if l.startswith('keybinding add')]
        for k in lines:
            first, command = k.split(' ', 1)
            bind, context = (first.split('@') + [''])[:2]
            if ' ' not in command:
                command = command.replace('"', '')
            tool = command.split(' ')[0].replace('"', '')
            keybindings[tool] = keybindings.get(tool, []) + [
                (command, bind.split('-'), context)]

def scan_all_keybinds(root_dir):
    """Get the implemented keybinds, and return a dict of
    {tool: [(full_command, keybinding, context), ...]}.
    """
    keybindings = dict()
    for root, _, files in os.walk(root_dir):
        scan_keybinds(root, files, keybindings)
    return keybindings


def render_dfhack_keybind(command) -> List[nodes.paragraph]:
    _KEYBINDS_RENDERED.add(command)
    out = []
    if command not in _KEYBINDS:
        return out
    for keycmd, key, ctx in _KEYBINDS[command]:
        n = nodes.paragraph()
        n += nodes.strong('Keybinding:', 'Keybinding:')
        n += nodes.inline(' ', ' ')
        for k in key:
            n += nodes.inline(k, k, classes=['kbd'])
        if keycmd != command:
            n += nodes.inline(' -> ', ' -> ')
            n += nodes.literal(keycmd, keycmd, classes=['guilabel'])
        if ctx:
            n += nodes.inline(' in ', ' in ')
            n += nodes.literal(ctx, ctx)
        out.append(n)
    return out


def check_missing_keybinds():
    # FIXME: _KEYBINDS_RENDERED is empty in the parent process under parallel builds
    # consider moving to a sphinx Domain to solve this properly
    for missing_command in sorted(set(_KEYBINDS.keys()) - _KEYBINDS_RENDERED):
        logger.warning('Undocumented keybindings for command: %s', missing_command)


# pylint:disable=unused-argument,dangerous-default-value,too-many-arguments
def dfhack_keybind_role(role, rawtext, text, lineno, inliner,
                        options={}, content=[]):
    """Custom role parser for DFHack default keybinds."""
    return render_dfhack_keybind(text), []


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
            label_class('', '{}:'.format(label)),
            nodes.inline(text=' '),
            content_class('', content),
        ])

    @staticmethod
    def wrap_box(*children: List[nodes.Node]) -> nodes.Admonition:
        return nodes.topic('', *children, classes=['dfhack-tool-summary'])

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
        tag_nodes = [nodes.strong(text='Tags:'), nodes.inline(text=' ')]
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
            nodes.paragraph('', '', *tag_nodes),
        ]

    def run(self):
        out = DFHackToolDirectiveBase.run(self)
        if 'no-command' not in self.options:
            out += [self.wrap_box(*DFHackCommandDirective.render_content(self))]
        return out


class DFHackCommandDirective(DFHackToolDirectiveBase):
    def render_content(self) -> List[nodes.Node]:
        command = self.get_name_or_docname()
        return [
            self.make_labeled_paragraph('Command', command, content_class=nodes.literal),
            *render_dfhack_keybind(command),
        ]


def register(app):
    app.add_directive('dfhack-tool', DFHackToolDirective)
    app.add_directive('dfhack-command', DFHackCommandDirective)
    app.add_role('dfhack-keybind', dfhack_keybind_role)

    _KEYBINDS.update(scan_all_keybinds(os.path.join(dfhack.util.DFHACK_ROOT, 'data', 'init')))


def setup(app):
    app.connect('builder-inited', register)

    # TODO: re-enable once detection is corrected
    # app.connect('build-finished', lambda *_: check_missing_keybinds())

    return {
        'version': '0.1',
        'parallel_read_safe': True,
        'parallel_write_safe': True,
    }
