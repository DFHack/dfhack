import collections
import copy
import itertools
import os
import sys

from sphinx.errors import ExtensionError, SphinxError, SphinxWarning

from dfhack.util import DFHACK_ROOT, DOCS_ROOT, write_file_if_changed

CHANGELOG_PATHS = (
    'docs/changelog.txt',
    'scripts/changelog.txt',
    'library/xml/changelog.txt',
)
CHANGELOG_PATHS = (os.path.join(DFHACK_ROOT, p) for p in CHANGELOG_PATHS)

CHANGELOG_SECTIONS = [
    'New Tools',
    'New Plugins', # deprecated
    'New Scripts', # deprecated
    'New Tweaks', # deprecated
    'New Features',
    'New Internal Commands', # deprecated
    'Fixes',
    'Misc Improvements',
    'Documentation',
    'Removed',
    'API',
    'Internals',
    'Lua',
    'Ruby', # deprecated, but still here so old changelogs build
    'Structures',
]

REPLACEMENTS = {
    '`search`': '`search-plugin`',
}

def to_title_case(word):
    if word == word.upper():
        # Preserve acronyms
        return word
    return word[0].upper() + word[1:].lower()

def find_all_indices(string, substr):
    start = 0
    while True:
        i = string.find(substr, start)
        if i == -1:
            return
        yield i
        start = i + 1

def replace_text(string, replacements):
    for old_text, new_text in replacements.items():
        new_string = ''
        new_string_end = 0  # number of characters from string in new_string
        for i in find_all_indices(string, old_text):
            if i > 0 and string[i - 1] == '!':
                # exempt if preceded by '!'
                new_string += string[new_string_end:i - 1]
                new_string += old_text
            else:
                # copy until this occurrence
                new_string += string[new_string_end:i]
                new_string += new_text
            new_string_end = i + len(old_text)
        new_string += string[new_string_end:]
        string = new_string
    return string

class ChangelogEntry(object):
    def __init__(self, text, section, stable_version, dev_version):
        text = text.lstrip('- ')
        # normalize section to title case
        self.section = ' '.join(map(to_title_case, section.strip().split()))
        self.stable_version = stable_version
        self.dev_version = dev_version
        self.dev_only = text.startswith('@')
        text = text.lstrip('@ ')
        self.children = []

        split_index = text.find(': ')
        if split_index != -1:
            self.feature, description = text[:split_index], text[split_index+1:]
            if description.strip():
                self.children.insert(0, description.strip())
        else:
            self.feature = text
        self.feature = self.feature.replace(':\\', ':').rstrip(':')

        self.sort_key = self.feature.upper()

    def __repr__(self):
        return 'ChangelogEntry(%r, %r)' % (self.feature, self.children)

def parse_changelog():
    entries = []

    for fpath in CHANGELOG_PATHS:
        if not os.path.isfile(fpath):
            continue
        with open(fpath) as f:
            cur_stable = None
            cur_dev = None
            cur_section = None
            last_entry = None
            multiline = ''
            for line_id, line in enumerate(f.readlines()):
                line_id += 1

                if multiline:
                    multiline += line
                elif '[[[' in line:
                    multiline = line.replace('[[[', '')

                if ']]]' in multiline:
                    line = multiline.replace(']]]', '')
                    multiline = ''
                elif multiline:
                    continue

                if not line.strip() or line.startswith('==='):
                    continue

                if line.startswith('##'):
                    cur_section = line.lstrip('#').strip()
                elif line.startswith('#'):
                    cur_dev = line.lstrip('#').strip().lower()
                    if ('alpha' not in cur_dev and 'beta' not in cur_dev and
                            'rc' not in cur_dev):
                        cur_stable = cur_dev
                elif line.startswith('-'):
                    if not cur_stable or not cur_dev or not cur_section:
                        raise ValueError(
                            '%s:%i: Entry without section' % (fpath, line_id))
                    last_entry = ChangelogEntry(line.strip(), cur_section,
                                                cur_stable, cur_dev)
                    entries.append(last_entry)
                elif line.lstrip().startswith('-'):
                    if not cur_stable or not cur_dev:
                        raise ValueError(
                            '%s:%i: Sub-entry without section' % (fpath, line_id))
                    if not last_entry:
                        raise ValueError(
                            '%s:%i: Sub-entry without parent' % (fpath, line_id))
                    last_entry.children.append(line.strip('- \n'))
                else:
                    raise ValueError('%s:%i: Invalid line: %s' % (fpath, line_id, line))

    if not entries:
        raise RuntimeError('No changelog files with contents found')

    return entries

def consolidate_changelog(all_entries):
    for sections in all_entries.values():
        for section, entries in sections.items():
            entries.sort(key=lambda entry: entry.sort_key)
            new_entries = []
            for feature, group in itertools.groupby(entries,
                                                    lambda e: e.feature):
                old_entries = list(group)
                children = list(itertools.chain(*[entry.children
                                for entry in old_entries]))
                new_entry = copy.deepcopy(old_entries[0])
                new_entry.children = children
                new_entries.append(new_entry)
            entries[:] = new_entries



def print_changelog(versions, all_entries, path, replace=True, prefix=''):
    # all_entries: version -> section -> entry
    with write_file_if_changed(path) as f:
        def write(line):
            if replace:
                line = replace_text(line, REPLACEMENTS)
            f.write(prefix + line + '\n')
        for version in versions:
            sections = all_entries[version]
            if not sections:
                continue
            version = 'DFHack ' + version
            write(version)
            write('=' * len(version))
            write('')
            for section in CHANGELOG_SECTIONS:
                entries = sections[section]
                if not entries:
                    continue
                write(section)
                write('-' * len(section))
                for entry in entries:
                    if len(entry.children) == 1:
                        write('- ' + entry.feature + ': ' +
                                entry.children[0].strip('- '))
                        continue
                    elif entry.children:
                        write('- ' + entry.feature + ':')
                        for child in entry.children:
                            write('    - ' + child)
                    else:
                        write('- ' + entry.feature)
                write('')
            write('')


def generate_changelog(all=False):
    entries = parse_changelog()

    # scan for unrecognized sections
    for entry in entries:
        if entry.section not in CHANGELOG_SECTIONS:
            raise SphinxWarning('Unknown section: ' + entry.section)

    # ordered versions
    versions = ['future']
    # map versions to stable versions
    stable_version_map = {}
    # version -> section -> entry
    stable_entries = collections.defaultdict(lambda:
                        collections.defaultdict(list))
    dev_entries = collections.defaultdict(lambda:
                        collections.defaultdict(list))

    for entry in entries:
        # build list of all versions
        if entry.dev_version not in versions:
            versions.append(entry.dev_version)
        stable_version_map.setdefault(entry.dev_version, entry.stable_version)

        if not entry.dev_only:
            # add non-dev-only entries to both changelogs
            stable_entries[entry.stable_version][entry.section].append(entry)
        dev_entries[entry.dev_version][entry.section].append(entry)

    consolidate_changelog(stable_entries)
    consolidate_changelog(dev_entries)

    os.makedirs(os.path.join(DOCS_ROOT, 'changelogs'), mode=0o755, exist_ok=True)

    print_changelog(versions, stable_entries, os.path.join(DOCS_ROOT, 'changelogs/news.rst'))
    print_changelog(versions, dev_entries, os.path.join(DOCS_ROOT, 'changelogs/news-dev.rst'))

    if all:
        for version in versions:
            if version not in stable_version_map:
                print('warn: skipping ' + version)
                continue
            if stable_version_map[version] == version:
                version_entries = {version: stable_entries[version]}
            else:
                version_entries = {version: dev_entries[version]}
            print_changelog([version], version_entries,
                os.path.join(DOCS_ROOT, 'changelogs/%s-github.txt' % version),
                replace=False)
            print_changelog([version], version_entries,
                os.path.join(DOCS_ROOT, 'changelogs/%s-reddit.txt' % version),
                replace=False,
                prefix='> ')

    return entries

def cli_entrypoint():
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument('-a', '--all', action='store_true',
        help='Print changelogs for all versions to docs/changelogs')
    parser.add_argument('-c', '--check', action='store_true',
        help='Check that all entries are printed')
    args = parser.parse_args()

    entries = generate_changelog(all=args.all)

    if args.check:
        with open(os.path.join(DOCS_ROOT, 'changelogs/news.rst')) as f:
            content_stable = f.read()
        with open(os.path.join(DOCS_ROOT, 'changelogs/news-dev.rst')) as f:
            content_dev = f.read()
        for entry in entries:
            for description in entry.children:
                if not entry.dev_only and description not in content_stable:
                    print('stable missing: ' + description)
                if description not in content_dev:
                    print('dev missing: ' + description)


def sphinx_entrypoint(app, config):
    try:
        generate_changelog()
    except SphinxError:
        raise
    except Exception as e:
        raise ExtensionError(str(e), e)


def setup(app):
    app.connect('config-inited', sphinx_entrypoint)

    return {
        'version': '0.1',
        'parallel_read_safe': True,
        'parallel_write_safe': True,
    }
