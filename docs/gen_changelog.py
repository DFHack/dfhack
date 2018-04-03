import collections
import copy
import itertools
import os
import sys

CHANGELOG_SECTIONS = [
    'New Plugins',
    'New Scripts',
    'New Tweaks',
    'New Features',
    'New Internal Commands',
    'Fixes',
    'Misc Improvements',
    'Removed',
    'Internals',
    'Structures',
    'Lua',
    'Ruby',
]

class ChangelogEntry(object):
    def __init__(self, text, section, stable_version, dev_version):
        text = text.lstrip('- ')
        # normalize section to title case
        self.section = ' '.join(word[0].upper() + word[1:].lower()
                                for word in section.strip().split())
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
    cur_stable = None
    cur_dev = None
    cur_section = None
    last_entry = None
    entries = []

    with open('docs/changelog.txt') as f:
        for line_id, line in enumerate(f.readlines()):
            line_id += 1
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
                        'changelog.txt:%i: Entry without section' % line_id)
                last_entry = ChangelogEntry(line.strip(), cur_section,
                                            cur_stable, cur_dev)
                entries.append(last_entry)
                # entries.setdefault(cur_stable, []).append(last_entry)
                # entries.setdefault(cur_dev, []).append(last_entry)
            elif line.lstrip().startswith('-'):
                if not cur_stable or not cur_dev:
                    raise ValueError(
                        'changelog.txt:%i: Sub-entry without section' % line_id)
                if not last_entry:
                    raise ValueError(
                        'changelog.txt:%i: Sub-entry without parent' % line_id)
                last_entry.children.append(line.strip('- \n'))
            else:
                raise ValueError('Invalid line: ' + line)

    return entries

def consolidate_changelog(all_entries):
    for sections in all_entries.values():
        for section, entries in sections.items():
            # sort() is stable, so reverse entries so that older entries for the
            # same feature are on top
            entries.reverse()
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



def print_changelog(versions, all_entries, path):
    # all_entries: version -> section -> entry
    with open(path, 'w') as f:
        write = lambda s: f.write(s + '\n')
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
                        write('')
                        for child in entry.children:
                            write('    - ' + child)
                        write('')
                    else:
                        write('- ' + entry.feature)
                write('')
            write('')


def generate_changelog():
    entries = parse_changelog()

    # scan for unrecognized sections
    for entry in entries:
        if entry.section not in CHANGELOG_SECTIONS:
            raise RuntimeWarning('Unknown section: ' + entry.section)

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
        if entry.dev_version not in versions:
            versions.append(entry.dev_version)
        stable_version_map.setdefault(entry.dev_version, entry.stable_version)
        if not entry.dev_only:
            stable_entries[entry.stable_version][entry.section].append(entry)
        dev_entries[entry.dev_version][entry.section].append(entry)

    consolidate_changelog(stable_entries)

    print_changelog(versions, stable_entries, 'docs/_auto/news.rst')
    print_changelog(versions, dev_entries, 'docs/_auto/news-dev.rst')

    return entries

if __name__ == '__main__':
    os.chdir(os.path.abspath(os.path.dirname(__file__)))
    os.chdir('..')
    entries = generate_changelog()
    if '--check' in sys.argv:
        with open('docs/_auto/news.rst') as f:
            content_stable = f.read()
        with open('docs/_auto/news-dev.rst') as f:
            content_dev = f.read()
        for entry in entries:
            for description in entry.children:
                if not entry.dev_only and description not in content_stable:
                    print('stable missing: ' + description)
                if description not in content_dev:
                    print('dev missing: ' + description)
