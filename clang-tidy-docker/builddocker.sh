#!/bin/bash

#export CR_PAT=<TOKEN>
echo $CR_PAT | docker login ghcr.io -u DrLk --password-stdin
docker build --no-cache --pull --tag clang-tidy-image .
docker image tag clang-tidy-image ghcr.io/drlk/clang-tidy-image:v13
docker image push ghcr.io/drlk/clang-tidy-image:v13
