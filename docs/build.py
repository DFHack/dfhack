#!/usr/bin/env python3

# for help, run: python3 build.py --help

import argparse
import os
import subprocess
import sys

class SphinxOutputFormat:
    def __init__(self, name, pre_args):
        self.name = str(name)
        self.pre_args = tuple(pre_args)

    @property
    def args(self):
        output_dir = os.path.join('docs', self.name)
        artifacts_dir = os.path.join('build', 'docs', self.name)  # for artifacts not part of the final documentation
        os.makedirs(artifacts_dir, mode=0o755, exist_ok=True)
        return [
            *self.pre_args,
            '.',  # source dir
            output_dir,
            '-d', artifacts_dir,
            '-w', os.path.join(artifacts_dir, 'sphinx-warnings.txt'),
        ]

OUTPUT_FORMATS = {
    'html': SphinxOutputFormat('html', pre_args=['-b', 'html']),
    'text': SphinxOutputFormat('text', pre_args=['-b', 'text']),
    'pdf': SphinxOutputFormat('pdf', pre_args=['-M', 'latexpdf']),
    'xml': SphinxOutputFormat('xml', pre_args=['-b', 'xml']),
    'pseudoxml': SphinxOutputFormat('pseudoxml', pre_args=['-b', 'pseudoxml']),
}

def _parse_known_args(parser, source_args):
    # pass along any arguments after '--'
    ignored_args = []
    if '--' in source_args:
        source_args, ignored_args = source_args[:source_args.index('--')], source_args[source_args.index('--')+1:]
    args, forward_args = parser.parse_known_args(source_args)
    forward_args += ignored_args
    return args, forward_args

def parse_args(source_args):
    def output_format(s):
        if s in OUTPUT_FORMATS:
            return s
        raise ValueError

    parser = argparse.ArgumentParser(usage='%(prog)s [{} ...] [options] [--] [sphinx_options]'.format('|'.join(OUTPUT_FORMATS.keys())), description='''
    DFHack wrapper around sphinx-build.

    Any unrecognized options are passed directly to sphinx-build, as well as any
    options following a '--' argument, if specified.
    ''')
    parser.add_argument('format', nargs='*', type=output_format, action='append',
        help='Documentation format(s) to build - choose from {}'.format(', '.join(OUTPUT_FORMATS.keys())))
    parser.add_argument('-E', '--clean', action='store_true',
        help='Re-read all input files')
    parser.add_argument('--sphinx', type=str, default=os.environ.get('SPHINX', 'sphinx-build'),
        help='Sphinx executable to run [environment variable: SPHINX; default: "sphinx-build"]')
    parser.add_argument('-j', '--jobs', type=str, default=os.environ.get('JOBS', 'auto'),
        help='Number of Sphinx threads to run [environment variable: JOBS; default: "auto"]')
    parser.add_argument('-q', '--quiet', action='store_true',
        help='Disable most output on stdout (also passed to sphinx-build)')
    parser.add_argument('--debug', action='store_true',
        help='Log commands that are run, etc.')
    parser.add_argument('--offline', action='store_true',
        help='Disable network connections')
    args, forward_args = _parse_known_args(parser, source_args)

    # work around weirdness with list args
    args.format = args.format[0]
    if not args.format:
        args.format = ['html']

    return args, forward_args

if __name__ == '__main__':
    os.chdir(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
    if not os.path.isfile('conf.py'):
        print('Could not find conf.py', file=sys.stderr)
        exit(1)

    args, forward_args = parse_args(sys.argv[1:])

    sphinx_env = os.environ.copy()
    if args.offline:
        sphinx_env['DFHACK_DOCS_BUILD_OFFLINE'] = '1'

    for format_name in args.format:
        command = [args.sphinx] + OUTPUT_FORMATS[format_name].args + ['-j', args.jobs]
        if args.clean:
            command += ['-E']
        if args.quiet:
            command += ['-q']
        command += forward_args

        if args.debug:
            print('Building:', format_name)
            print('Running:', command)
        subprocess.run(command, check=True, env=sphinx_env)

        if not args.quiet:
            print('')
