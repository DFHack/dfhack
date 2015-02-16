import json, os, sys
if sys.version.startswith('2'):
    from urllib2 import urlopen, HTTPError
else:
    from urllib.request import urlopen
    from urllib.error import HTTPError
try:
    pr_id = int(os.environ.get('TRAVIS_PULL_REQUEST', 'false'))
except ValueError:
    print('Not a pull request')
    sys.exit(0)
print('Pull request %i' % pr_id)
res = {}
try:
    res = json.loads(urlopen('https://api.github.com/repos/dfhack/dfhack/pulls/%i' % pr_id).read().decode('utf-8'))
except ValueError:
    pass
except HTTPError:
    print('Failed to retrieve PR information from API')
    sys.exit(2)
if 'base' not in res or 'ref' not in res['base']:
    print('Invalid JSON returned from API')
    sys.exit(2)
if res['base']['ref'] != 'develop':
    print('Not based on develop branch')
    sys.exit(1)
else:
    print('Ok')
    sys.exit(0)
