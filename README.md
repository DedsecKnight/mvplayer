# MVPlayer 

A mini video-player implemented from scratch using LibAV and SDL. Also included a simplified actor framework system.  

## Getting Started

### Dependencies

- C++ compiler with C++23 support (GCC 15+, Clang 20+)
- CMake 4.0 or higher
- Conan 2.0 

### Installation

1. Clone the repository:
```bash
git clone https://github.com/DedsecKnight/mvplayer.git
cd mvplayer
```

2. Install all dependencies through Conan:
```bash
conan install . --build=missing -pr:h=profiles/profile_linux_gcc -pr:b=profiles/profile_linux_gcc
```

3. Configure project with CMake:
```bash
cmake --preset conan-release
```

4. Build project with CMake:
```bash
cmake --build --preset conan-release
```

### Running the Program

If your machine is using Wayland, set the `EGL_DISPLAY` environment variable before running:
```bash
export EGL_DISPLAY=wayland
```

```bash
./build/Release/apps/video-player/video_player -i /path/to/video_file.mp4
```


## Project Structure

```
mvplayer/
├── src/              # Source files
├── profiles/         # Profiles for building with Conan 
├── scripts/          # Utility scripts, mainly for automating build 
├── include/          # Header files
├── tests/            # Unit tests
├── apps/             # Example applications
├── CMakeLists.txt    # CMake configuration
└── README.md         # This file
```

## Building with CMake

### Debug Build
```bash
conan install . --build=missing -s build_type=Debug -pr:h=profiles/profile_linux_gcc -pr:b=profiles/profile_linux_gcc
cmake --preset conan-debug
cmake --build --preset conan-debug
```

### Release Build
```bash
conan install . --build=missing -pr:h=profiles/profile_linux_gcc -pr:b=profiles/profile_linux_gcc
cmake --preset conan-release
cmake --build --preset conan-release
```

## Running Tests

```bash
./build/Release/tests/test
```
