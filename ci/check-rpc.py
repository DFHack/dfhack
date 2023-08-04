#!/usr/bin/env python3
import glob
import itertools
import sys

actual = {'': {}}
SEP = ('=' * 80)

with open(sys.argv[1]) as f:
    plugin_name = ''
    for line in f:
        line = line.rstrip()
        if line.startswith('// Plugin: '):
            plugin_name = line.split(' ')[2]
            if plugin_name not in actual:
                actual[plugin_name] = {}
        elif line.startswith('// RPC '):
            parts = line.split(' ')
            actual[plugin_name][parts[2]] = (parts[4], parts[6])

expected = {'': {}}

for p in glob.iglob('library/proto/*.proto'):
    with open(p) as f:
        for line in f:
            line = line.rstrip()
            if line.startswith('// RPC '):
                parts = line.split(' ')
                expected[''][parts[2]] = (parts[4], parts[6])

for p in itertools.chain(glob.iglob('plugins/proto/*.proto'), glob.iglob('plugins/*/proto/*.proto')):
    plugin_name = ''
    with open(p) as f:
        for line in f:
            line = line.rstrip()
            if line.startswith('// Plugin: '):
                plugin_name = line.split(' ')[2]
                if plugin_name not in expected:
                    expected[plugin_name] = {}
                break

    if plugin_name == '':
        continue

    with open(p) as f:
        for line in f:
            line = line.rstrip()
            if line.startswith('// RPC '):
                parts = line.split(' ')
                expected[plugin_name][parts[2]] = (parts[4], parts[6])

error_count = 0

for plugin_name in actual:
    methods = actual[plugin_name]

    if plugin_name not in expected:
        print(SEP)
        print('Missing documentation for plugin proto files: ' + plugin_name)
        print('Add the following lines:')
        print('// Plugin: ' + plugin_name)
        error_count += 1
        for m in methods:
            io = methods[m]
            print('// RPC ' + m + ' : ' + io[0] + ' -> ' + io[1])
            error_count += 1
    else:
        missing = []
        wrong = []
        for m in methods:
            io = methods[m]
            if m in expected[plugin_name]:
                if expected[plugin_name][m] != io:
                    wrong.append('// RPC ' + m + ' : ' + io[0] + ' -> ' + io[1])
            else:
                missing.append('// RPC ' + m + ' : ' + io[0] + ' -> ' + io[1])

        if len(missing) > 0:
            print(SEP)
            print('Incomplete documentation for ' + ('core' if plugin_name == '' else 'plugin "' + plugin_name + '"') + ' proto files. Add the following lines:')
            for m in missing:
                print(m)
                error_count += 1

        if len(wrong) > 0:
            print(SEP)
            print('Incorrect documentation for ' + ('core' if plugin_name == '' else 'plugin "' + plugin_name + '"') + ' proto files. Replace the following comments:')
            for m in wrong:
                print(m)
                error_count += 1

for plugin_name in expected:
    methods = expected[plugin_name]

    if plugin_name not in actual:
        print(SEP)
        print('Incorrect documentation for plugin proto files: ' + plugin_name)
        print('The following methods are documented, but the plugin does not provide any RPC methods:')
        for m in methods:
            io = methods[m]
            print('// RPC ' + m + ' : ' + io[0] + ' -> ' + io[1])
            error_count += 1
    else:
        missing = []
        for m in methods:
            io = methods[m]
            if m not in actual[plugin_name]:
                missing.append('// RPC ' + m + ' : ' + io[0] + ' -> ' + io[1])

        if len(missing) > 0:
            print(SEP)
            print('Incorrect documentation for ' + ('core' if plugin_name == '' else 'plugin "' + plugin_name + '"') + ' proto files. Remove the following lines:')
            for m in missing:
                print(m)
                error_count += 1

sys.exit(min(100, error_count))
