#!/bin/bash

cd ..
conan install . --build=missing -pr:h=profiles/profile_win2022 -pr:b=profilesprofile_win2022/
cmake --preset conan-release
cmake --build --preset conan-release

