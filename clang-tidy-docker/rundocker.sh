#!/bin/bash

env > env_file
docker run --env-file env_file \
          -u $(id -u $USER) \
          -i \
          --rm \
          --mount type=bind,src=${PWD}/../,dst=/build \
          clang-tidy-image
