#!/bin/bash

cd ..
conan install . --build=missing -pr:h=profiles/profile_linux_gcc -pr:b=profiles/profile_linux_gcc
cmake --preset conan-release
cmake --build --preset conan-release

