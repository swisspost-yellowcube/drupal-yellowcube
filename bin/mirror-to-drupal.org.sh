#!/usr/bin/env bash

# Pushes the current branch to the drupal branch.
#
# The ssh key is decrypted in the .travis.yml file and written to `~/.ssh/id_rsa`

CURRENT_BRANCH=`git rev-parse --abbrev-ref HEAD`
DRUPAL_HOST="git.drupal.org"

# Add drupal.org to known keys
if [ -z `ssh-keygen -F $DRUPAL_HOST` ]; then
  ssh-keyscan -H $DRUPAL_HOST >> ~/.ssh/known_hosts
fi

git remote add drupal adrip@git.drupal.org:project/yellowcube.git
git push drupal $CURRENT_BRANCH --tags
