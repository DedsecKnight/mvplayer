#!/bin/bash

cd ..
conan install . --build=missing -s build_type=Debug -pr:h=profiles/profile_linux_gcc -pr:b=profiles/profile_linux_gcc
cmake --preset conan-debug
cmake --build --preset conan-debug

