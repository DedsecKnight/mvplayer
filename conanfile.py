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
        self.requires("opencv/4.12.0")
        self.requires("sdl/3.2.20")
        self.requires("gtest/1.17.0")

    def configure(self):
        self.options["ffmpeg"].with_pulse = False
        self.options["ffmpeg"].avdevice = False
        self.options["ffmpeg"].swresample = False
        self.options["ffmpeg"].postproc = False
        self.options["ffmpeg"].with_avfoundation = False
        self.options["ffmpeg"].with_coreimage = False
        self.options["ffmpeg"].with_libalsa = False
        self.options["ffmpeg"].with_xcb = False
        self.options["ffmpeg"].with_xlib = False

        self.options["sdl"].shared = True

        self.options["opencv"].ml = False
        self.options["opencv"].dnn = False
        self.options["opencv"].stitching = False
        self.options["opencv"].dnn = False
        self.options["opencv"].calib3d = False
        self.options["opencv"].highgui = False
        self.options["opencv"].videoio = False
        self.options["opencv"].objdetect = False
        self.options["opencv"].features2d = False
        self.options["opencv"].with_ffmpeg = False
        self.options["opencv"].with_quirc = False

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
