#!/bin/bash

echo "Please enter the release tag, this will be used by the CI to create the release (ex: 2.17.10): "
read -p "-> " input_string
echo ""

git pull;

echo ""

read -p "Do you want to create a new release on Git? (y/N): " answerOne

if [[ "$answerOne" =~ ^[Yy]$ ]]; then
  echo "Creating tag: $input_string"
  git tag -a "$input_string" -m "v$input_string";
  echo "Pushing tags..."
  git push origin --tags;
  echo "GitHub Actions is building the project:"
  echo ""
  echo "https://github.com/sblantipodi/glow_worm_luciferin/actions"
else
  echo "Skipping main release..."
fi