#!/bin/sh
git log --pretty="commit %h (parents: %p): %s" -1
