import json, os, sys
if sys.version.startswith('2'):
    from urllib2 import urlopen, HTTPError
else:
    from urllib.request import urlopen
    from urllib.error import HTTPError
try:
    repo = os.environ.get('TRAVIS_REPO_SLUG', 'dfhack/dfhack').lower()
    pr_id = int(os.environ.get('TRAVIS_PULL_REQUEST', 'false'))
except ValueError:
    print('Not a pull request')
    sys.exit(0)
print('Pull request %s#%i' % (repo, pr_id))
if repo != 'dfhack/dfhack':
    print('Not in dfhack/dfhack')
    sys.exit(0)
res = {}
try:
    res = json.loads(urlopen('https://api.github.com/repos/%s/pulls/%i' % (repo, pr_id)).read().decode('utf-8'))
except ValueError:
    pass
except HTTPError as e:
    print('Failed to retrieve PR information from API: %s' % e)
    sys.exit(0)
if 'base' not in res or 'ref' not in res['base']:
    print('Invalid JSON returned from API')
    sys.exit(2)
if res['base']['ref'] != 'develop':
    print('Not based on develop branch')
    sys.exit(1)
else:
    print('Ok')
    sys.exit(0)
