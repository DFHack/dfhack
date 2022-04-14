#!/usr/bin/env python3

import argparse
import hashlib
import hmac
import json
import logging
import os
import uuid

import requests

logging.basicConfig(level=logging.DEBUG)


def get_required_env(name):
    value = os.environ.get(name)
    if not value:
        raise ValueError(f'Expected environment variable {name!r} to be non-empty')
    return value


def make_sig(body, secret, algorithm):
    return hmac.new(secret.encode(), body.encode(), algorithm).hexdigest()


webhook_url = get_required_env('DFHACK_BUILDMASTER_WEBHOOK_URL')
secret = get_required_env('DFHACK_BUILDMASTER_WEBHOOK_SECRET')
api_token = get_required_env('GITHUB_TOKEN')
repo = get_required_env('GITHUB_REPO')


parser = argparse.ArgumentParser()
target_group = parser.add_mutually_exclusive_group(required=True)
target_group.add_argument('--pull-request', type=int, help='Pull request to rebuild')
args = parser.parse_args()


response = requests.get(
    f'https://api.github.com/repos/{repo}/pulls/{args.pull_request}',
    headers={
        'Authorization': f'Bearer {api_token}',
    },
)
response.raise_for_status()
pull_request_data = response.json()


body = json.dumps({
    'action': 'synchronize',
    'number': args.pull_request,
    'pull_request': pull_request_data,
})

response = requests.post(
    'https://lubar-webhook-proxy.appspot.com/github/buildmaster',
    headers={
        'Content-Type': 'application/json',
        'User-Agent': 'GitHub-Hookshot/' + requests.utils.default_user_agent(),
        'X-GitHub-Delivery': 'dfhack-rebuild-' + str(uuid.uuid4()),
        'X-GitHub-Event': 'pull_request',
        'X-Hub-Signature': 'sha1=' + make_sig(body, secret, hashlib.sha1),
        'X-Hub-Signature-256': 'sha256=' + make_sig(body, secret, hashlib.sha256),
    },
    data=body,
    timeout=15,
)

print(response)
print(str(response.text))
response.raise_for_status()
