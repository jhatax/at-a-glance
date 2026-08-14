#!/usr/bin/env zsh

# Make sure the checked-in hook file has execution rights
chmod +x .githooks/pre-commit

# Redirect Git to read hooks from the checked-in folder
git config core.hooksPath .githooks
echo "Git hooks pointing to checked-in .githooks directory!"
