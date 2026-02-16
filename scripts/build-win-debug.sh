#!/bin/bash

cd ..
conan install . --build=missing -s build_type=Debug -pr:h=profiles/profile_win2022 -pr:b=profiles/profile_win2022
cmake --preset conan-debug
cmake --build --preset conan-debug

