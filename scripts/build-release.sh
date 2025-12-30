#!/bin/bash

cd ..
conan install . --build=missing -pr:h=profiles/profile_linux_x86 -pr:b=profiles/profile_linux_x86
cmake --preset conan-release
cmake --build --preset conan-release

