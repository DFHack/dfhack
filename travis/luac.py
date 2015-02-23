import os, sys, subprocess

def main():
    root_path = os.path.abspath(sys.argv[1] if len(sys.argv) > 1 else '.')
    if not os.path.exists(root_path):
        print('Nonexistent path: %s' % root_path)
        sys.exit(2)
    err = False
    for cur, dirnames, filenames in os.walk(root_path):
        parts = cur.replace('\\', '/').split('/')
        if '.git' in parts or 'depends' in parts:
            continue
        if '.git' in cur:
            print(parts)
        for filename in filenames:
            if not filename.endswith('.lua'):
                continue
            full_path = os.path.join(cur, filename)
            try:
                subprocess.call(['luac' + os.environ.get('LUA_VERSION', ''), '-p', full_path])
            except subprocess.CalledProcessError:
                err = True
    sys.exit(int(err))

if __name__ == '__main__':
    main()
