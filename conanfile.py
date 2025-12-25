from conan import ConanFile
from conan.tools.files import copy
from conan.tools.cmake import CMake, CMakeToolchain
import os


class VideoPlayerRecipe(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeDeps"

    def requirements(self):
        self.requires("ffmpeg/8.0.1")
        self.requires("cli11/2.6.0")
        self.requires("spdlog/1.16.0")

    def configure(self):
        self.options["ffmpeg"].with_pulse = False

    def generate(self):
        tc = CMakeToolchain(self, generator="Ninja")
        tc.variables["CMAKE_EXPORT_COMPILE_COMMANDS"] = True
        tc.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def layout(self):
        self.folders.generators = os.path.join(
            "build", str(self.settings.build_type), "generators"
        )
        self.folders.build = os.path.join("build", str(self.settings.build_type))
