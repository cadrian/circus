#!/usr/bin/env bash

cd $(dirname $(readlink -f $0))

tar cfz travis_key.tar.gz travis_key travis_key.pub travis_key.sh
travis encrypt-file travis_key.tar.gz -r cadrian/circus
git add travis_key.tar.gz.enc
