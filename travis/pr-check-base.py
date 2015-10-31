import os, sys
repo = os.environ.get('TRAVIS_REPO_SLUG', 'dfhack/dfhack').lower()
branch = os.environ.get('TRAVIS_BRANCH', 'master')
try:
    pr_id = int(os.environ.get('TRAVIS_PULL_REQUEST', 'false'))
except ValueError:
    print('Not a pull request')
    sys.exit(0)
print('Pull request %s#%i' % (repo, pr_id))
if repo != 'dfhack/dfhack':
    print('Not in dfhack/dfhack')
    sys.exit(0)
if branch != 'develop':
    print('Not based on develop branch')
    sys.exit(1)
else:
    print('Ok')
    sys.exit(0)
