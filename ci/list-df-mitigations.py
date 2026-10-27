#!/usr/bin/env python3
"""List DF defect mitigations that should be re-reviewed on a new DF release.

DFHack works around a number of defects in Dwarf Fortress itself. When Bay 12
releases a new version of DF, some of those mitigations may need to be adjusted
or removed. This script generates the review checklist so the release
coordinator does not have to track the mitigations by hand.

Mitigations are marked where they live in the code with a comment containing
the ``DF-MITIGATION:`` marker, e.g.::

    // DF-MITIGATION: site_id is not assigned on reclaim until the first save

In addition, every ``fix/*`` script in the scripts repo is a mitigation by
definition, so they are listed automatically without needing a marker. Their
descriptions come from the ``:summary:`` field of their documentation.

The checklist is printed to stdout. If the GITHUB_STEP_SUMMARY environment
variable is set (i.e. when running in a GitHub Actions job), it is also
appended to the job summary so it is visible on the workflow run page.
"""

import os
import re

REPO_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SELF = relpath_self = os.path.relpath(os.path.abspath(__file__),
                                      REPO_ROOT).replace(os.sep, '/')

MARKER_RE = re.compile(r'DF-MITIGATION:?\s*(.*?)\s*(?:\*+/)?\s*$')
SUMMARY_RE = re.compile(r'^\s*:summary:\s*(.*)$')

# directories that never contain our own code
SKIP_DIRS = {'.git', 'build', 'depends', 'package'}

SOURCE_EXTS = {
    '.c', '.cc', '.cpp', '.cxx', '.h', '.hh', '.hpp',
    '.lua', '.py', '.rb', '.js', '.ts', '.sh', '.ps1',
}


def iter_source_files(root):
    for dirpath, dirnames, filenames in os.walk(root):
        dirnames[:] = [d for d in dirnames
                       if d not in SKIP_DIRS and not d.startswith('.')]
        for name in filenames:
            if os.path.splitext(name)[1].lower() in SOURCE_EXTS:
                yield os.path.join(dirpath, name)


def relpath(path):
    return os.path.relpath(path, REPO_ROOT).replace(os.sep, '/')


def collect_markers():
    entries = []
    for path in iter_source_files(REPO_ROOT):
        if relpath(path) == SELF:
            continue
        try:
            with open(path, encoding='utf-8', errors='replace') as f:
                for lineno, line in enumerate(f, 1):
                    match = MARKER_RE.search(line)
                    if match:
                        entries.append((relpath(path), lineno,
                                        match.group(1) or '(no description)'))
        except OSError:
            continue
    return sorted(entries)


def doc_summary(script_path):
    """Look up the :summary: field in the script's documentation."""
    rel = relpath(script_path)
    name = os.path.splitext(os.path.basename(rel))[0]
    for doc in (os.path.join(REPO_ROOT, 'scripts', 'docs', 'fix',
                             name + '.rst'),
                os.path.join(REPO_ROOT, 'scripts', 'docs', name + '.rst')):
        try:
            with open(doc, encoding='utf-8', errors='replace') as f:
                for line in f:
                    match = SUMMARY_RE.match(line)
                    if match:
                        return match.group(1).strip()
        except OSError:
            continue
    return None


def script_description(path):
    """Fall back to the first substantive comment line of the script."""
    try:
        with open(path, encoding='utf-8', errors='replace') as f:
            for line in f:
                line = line.strip()
                if line.startswith('--'):
                    desc = line.lstrip('-').strip()
                    if desc and not desc.startswith('@'):
                        return desc
                elif line:
                    return None
    except OSError:
        pass
    return None


def collect_fix_scripts():
    """fix/* scripts (and the top-level fix* scripts) are all DF bug
    mitigations."""
    entries = []
    scripts_dir = os.path.join(REPO_ROOT, 'scripts')
    if not os.path.isdir(scripts_dir):
        return entries
    for dirpath, _dirnames, filenames in os.walk(scripts_dir):
        if relpath(dirpath) not in ('scripts', 'scripts/fix'):
            continue
        for name in filenames:
            if not name.endswith('.lua'):
                continue
            if relpath(dirpath) == 'scripts' and not name.startswith('fix'):
                continue
            path = os.path.join(dirpath, name)
            desc = doc_summary(path) or script_description(path) \
                or '(no description)'
            entries.append((relpath(path), desc))
    return sorted(entries)


def main():
    lines = [
        '## DF defect mitigations',
        '',
        'A new DF release may have fixed some of the defects that DFHack works',
        'around. Review each entry and adjust or remove mitigations (and their',
        '`DF-MITIGATION` markers) that are no longer needed.',
        '',
        '### Code sites',
        '',
    ]
    markers = collect_markers()
    if markers:
        for path, lineno, desc in markers:
            lines.append(f'- [ ] `{path}:{lineno}`: {desc}')
    else:
        lines.append('- (none found)')

    lines += ['', '### `fix/*` scripts', '']
    fixes = collect_fix_scripts()
    if fixes:
        for path, desc in fixes:
            lines.append(f'- [ ] `{path}`: {desc}')
    else:
        lines.append('- (scripts repo not checked out)')

    lines.append('')
    output = '\n'.join(lines)
    print(output)

    summary_path = os.environ.get('GITHUB_STEP_SUMMARY')
    if summary_path:
        with open(summary_path, 'a', encoding='utf-8') as f:
            f.write(output)


if __name__ == '__main__':
    main()
