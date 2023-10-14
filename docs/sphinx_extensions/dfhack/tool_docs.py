# useful references:
#   https://www.sphinx-doc.org/en/master/extdev/appapi.html
#   https://www.sphinx-doc.org/en/master/development/tutorials/recipe.html
#   https://www.sphinx-doc.org/en/master/usage/restructuredtext/basics.html#rst-directives

from collections import defaultdict
import logging
import os
import re
from typing import Dict, Iterable, List, Optional, Tuple, Type

import docutils.nodes as nodes
from docutils.nodes import Node
import docutils.parsers.rst.directives as rst_directives
import sphinx
import sphinx.addnodes as addnodes
import sphinx.directives
from sphinx.domains import Domain, Index, IndexEntry
from sphinx.util.docutils import SphinxDirective
from sphinx.util.nodes import process_index_entry

import dfhack.util


logger = sphinx.util.logging.getLogger(__name__)


def get_label_class(builder: sphinx.builders.Builder) -> Type[nodes.Inline]:
    if builder.format == 'text':
        return nodes.inline
    else:
        return nodes.strong

def make_labeled_paragraph(label: Optional[str]=None, content: Optional[str]=None,
            label_class=nodes.strong, content_class=nodes.inline) -> nodes.paragraph:
    p = nodes.paragraph('', '')
    if label is not None:
        p += [
            label_class('', '{}:'.format(label)),
            nodes.inline('', ' '),
        ]
    if content is not None:
        p += content_class('', content)
    return p

def make_summary(builder: sphinx.builders.Builder, summary: str) -> nodes.paragraph:
    para = nodes.paragraph('', '')
    if builder.format == 'text':
        # It might be clearer to block indent instead of just indenting the
        # first line, but this is clearer than nothing.
        para += nodes.inline(text='  ')
    para += nodes.inline(text=summary)
    return para

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


def render_dfhack_keybind(command, builder: sphinx.builders.Builder) -> List[nodes.paragraph]:
    _KEYBINDS_RENDERED.add(command)
    out = []
    if command not in _KEYBINDS:
        return out
    for keycmd, key, ctx in _KEYBINDS[command]:
        n = make_labeled_paragraph('Keybinding', label_class=get_label_class(builder))
        for k in key:
            if builder.format == 'text':
                k = '[{}]'.format(k)
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


_anchor_pattern = re.compile(r'^\d+')

def to_anchor(name: str) -> str:
    name = name.lower()
    name = name.replace('/', '-')
    name = re.sub(_anchor_pattern, '', name)
    return name

class DFHackToolDirectiveBase(sphinx.directives.ObjectDescription):
    has_content = False
    required_arguments = 0
    optional_arguments = 1

    def get_tool_name_from_docname(self):
        parts = self.env.docname.split('/')
        if 'tools' in parts:
            return '/'.join(parts[parts.index('tools') + 1:])
        else:
            return parts[-1]

    def get_name_or_docname(self):
        if self.arguments:
            return self.arguments[0]
        return self.get_tool_name_from_docname()

    def add_index_entries(self, name) -> None:
        docname = self.env.docname
        anchor = to_anchor(self.get_tool_name_from_docname())
        tags = self.env.domaindata['tag-repo']['doctags'][docname]
        indexdata = (name, self.options.get('summary', ''), '', docname, anchor, 0)
        if 'unavailable' not in tags:
            self.env.domaindata['all']['objects'].append(indexdata)
        for tag in tags:
            self.env.domaindata[tag]['objects'].append(indexdata)

    @staticmethod
    def wrap_box(*children: List[nodes.Node]) -> nodes.Admonition:
        return nodes.topic('', *children, classes=['dfhack-tool-summary'])

    def make_labeled_paragraph(self, *args, **kwargs):
        # convenience wrapper to set label_class to the desired builder-specific node type
        kwargs.setdefault('label_class', get_label_class(self.env.app.builder))
        return make_labeled_paragraph(*args, **kwargs)

    def render_content(self) -> List[nodes.Node]:
        raise NotImplementedError

    def run(self):
        return [self.wrap_box(*self.render_content())]


class DFHackToolDirective(DFHackToolDirectiveBase):
    option_spec = {
        'tags': dfhack.util.directive_arg_str_list,
        'no-command': rst_directives.flag,
        'summary': rst_directives.unchanged,
    }

    def render_content(self) -> List[nodes.Node]:
        tag_paragraph = self.make_labeled_paragraph('Tags')
        tags = self.options.get('tags', [])
        self.env.domaindata['tag-repo']['doctags'][self.env.docname] = tags
        for tag in tags:
            tag_paragraph += [
                addnodes.pending_xref(tag, nodes.inline(text=tag), **{
                    'reftype': 'ref',
                    'refdomain': 'std',
                    'reftarget': tag + '-tag-index',
                    'refexplicit': True,
                    'refwarn': True,
                }),
                nodes.inline(text=' | '),
            ]
        tag_paragraph.pop()

        ret_nodes = [tag_paragraph]
        if 'no-command' in self.options:
            self.add_index_entries(self.get_name_or_docname() + ' (plugin)')
            ret_nodes += [make_summary(self.env.app.builder, self.options.get('summary', ''))]
        return ret_nodes

    def run(self):
        out = DFHackToolDirectiveBase.run(self)
        if 'no-command' not in self.options:
            out += [self.wrap_box(*DFHackCommandDirective.render_content(self))]
        return out


class DFHackCommandDirective(DFHackToolDirectiveBase):
    option_spec = {
        'summary': rst_directives.unchanged_required,
    }

    def render_content(self) -> List[nodes.Node]:
        command = self.get_name_or_docname()
        self.add_index_entries(command)
        return [
            self.make_labeled_paragraph('Command', command, content_class=nodes.literal),
            make_summary(self.env.app.builder, self.options.get('summary', '')),
            *render_dfhack_keybind(command, builder=self.env.app.builder),
        ]


class TagRepoDomain(Domain):
    name = 'tag-repo'
    label = 'Holds tag associations per document'
    initial_data = {'doctags': {}}

    def merge_domaindata(self, docnames: List[str], otherdata: Dict) -> None:
        self.data['doctags'].update(otherdata['doctags'])


def get_tags():
    groups = {}
    group_re = re.compile(r'"([^"]+)"')
    tag_re = re.compile(r'- `([^ ]+) <[^>]+>`: (.*)')
    with open(os.path.join(dfhack.util.DOCS_ROOT, 'Tags.rst')) as f:
        lines = f.readlines()
        for line in lines:
            line = line.strip()
            m = re.match(group_re, line)
            if m:
                group = m.group(1)
                groups[group] = []
                continue
            m = re.match(tag_re, line)
            if m:
                tag = m.group(1)
                desc = m.group(2)
                groups[group].append((tag, desc))
    return groups


def tag_domain_get_objects(self):
    for obj in self.data['objects']:
        yield(obj)

def tag_domain_merge_domaindata(self, docnames: List[str], otherdata: Dict) -> None:
    seen = set()
    objs = self.data['objects']
    for obj in objs:
        seen.add(obj[0])
    for obj in otherdata['objects']:
        if obj[0] not in seen:
            objs.append(obj)
    objs.sort()

def tag_index_generate(self, docnames: Optional[Iterable[str]] = None) -> Tuple[List[Tuple[str, List[IndexEntry]]], bool]:
    content = defaultdict(list)
    for name, desc, _, docname, anchor, _ in self.domain.data['objects']:
        first_letter = name[0].lower()
        extra, descr = desc, ''
        if self.domain.env.app.builder.format == 'html':
            extra, descr = '', desc
        content[first_letter].append(
            IndexEntry(name, 0, docname, anchor, extra, '', descr))
    return (sorted(content.items()), False)

def register_index(app, tag, title):
    domain_class = type(tag+'Domain', (Domain, ), {
            'name': tag,
            'label': 'Container domain for tag: ' + tag,
            'initial_data': {'objects': []},
            'merge_domaindata': tag_domain_merge_domaindata,
            'get_objects': tag_domain_get_objects,
        })
    index_class = type(tag+'Index', (Index, ), {
            'name': 'tag-index',
            'localname': title,
            'shortname': tag,
            'generate': tag_index_generate,
        })
    app.add_domain(domain_class)
    app.add_index_to_domain(tag, index_class)

def init_tag_indices(app):
    os.makedirs(os.path.join(dfhack.util.DOCS_ROOT, 'tags'), mode=0o755, exist_ok=True)
    tag_groups = get_tags()
    for tag_group in tag_groups:
        group_file_path = os.path.join(dfhack.util.DOCS_ROOT, 'tags', 'by{group}.rst'.format(group=tag_group))
        with dfhack.util.write_file_if_changed(group_file_path) as topidx:
            for tag_tuple in tag_groups[tag_group]:
                tag, desc = tag_tuple[0], tag_tuple[1]
                topidx.write(('- `{name} <{name}-tag-index>`\n').format(name=tag))
                topidx.write(('    {desc}\n').format(desc=desc))
                register_index(app, tag, desc)


def update_index_titles(app):
    for domain in app.env.domains.values():
        for index in domain.indices:
            if index.shortname == 'all':
                continue
            if app.builder.format == 'html':
                index.localname = '"%s" tag index<h4>%s</h4>' % (index.shortname, index.localname)
            else:
                index.localname = '"%s" tag index - %s' % (index.shortname, index.localname)

def register(app):
    app.add_directive('dfhack-tool', DFHackToolDirective)
    app.add_directive('dfhack-command', DFHackCommandDirective)
    update_index_titles(app)
    _KEYBINDS.update(scan_all_keybinds(os.path.join(dfhack.util.DFHACK_ROOT, 'data', 'init')))

def setup(app):
    app.connect('builder-inited', register)

    app.add_domain(TagRepoDomain)
    register_index(app, 'all', 'Index of DFHack tools')
    init_tag_indices(app)

    # TODO: re-enable once detection is corrected
    # app.connect('build-finished', lambda *_: check_missing_keybinds())

    return {
        'version': '0.1',
        'parallel_read_safe': True,
        'parallel_write_safe': True,
    }
