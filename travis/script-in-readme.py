import os, sys

scriptdir = 'scripts'

def is_script(fname):
    if not os.path.isfile(fname):
        return False
    return fname.endswith('.lua') or fname.endswith('.rb')

def main():
    files = []
    for item in os.listdir(scriptdir):
        path = os.path.join(scriptdir, item)
        if is_script(path):
            files.append(item)
        elif os.path.isdir(path) and item not in ('devel', '3rdparty'):
            files.extend([item + '/' + f for f in os.listdir(path)
                          if is_script(os.path.join(path, f))])
    with open('docs/Scripts.rst') as f:
        text = f.read()
    error = 0
    for f, _ in [os.path.splitext(p) for p in files]:
        heading = '\n' + f + '\n' + '=' * len(f) + '\n'
        if heading not in text:
            print('WARNING:  {:28} not documented in docs/Scripts'.format(f))
            error = 1
    sys.exit(error)

if __name__ == '__main__':
    main()
