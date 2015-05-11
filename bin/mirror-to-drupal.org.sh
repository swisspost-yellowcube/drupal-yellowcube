#!/usr/bin/env bash

# Pushes the current branch to the drupal branch.
#
# The ssh key is decrypted in the .travis.yml file and written to `~/.ssh/id_rsa`

CURRENT_BRANCH=`git rev-parse --abbrev-ref HEAD`

git remote add drupal adrip@git.drupal.org:project/yellowcube.git
git push drupal $CURRENT_BRANCH --tags
