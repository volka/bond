from conan import ConanFile
from conan.tools.cmake import CMake, CMakeToolchain, cmake_layout
from conan.tools.files import copy
from conan.tools.scm import Version
from conan.tools.build import can_run
import os
import shutil

class BondCppConan(ConanFile):
    name = "bond-cpp"
    version = "0.13.2"
    license = "MIT"
    author = "Microsoft"
    url = "https://github.com/microsoft/bond"
    description = "Bond is an open source, cross-platform framework for working with schematized data."
    topics = ("serialization", "protocol buffers", "schema")
    settings = "os", "compiler", "build_type", "arch"
    options = {
        "shared": [True, False],
        "fPIC": [True, False],
        "bond_enable_grpc": [True, False]
    }
    default_options = {
        "shared": False,
        "fPIC": True,
        "bond_enable_grpc": False
    }
    exports_sources = (
        "CMakeLists.txt",
        "inc/*",
        "src/*",
        "test/*",
        "../LICENSE",
        "../cmake/*",
        "../compiler/*",
        "../idl/*"
    )
    package_type = "library"

    def config_options(self):
        if self.settings.os == "Windows":
            del self.options.fPIC

    def layout(self):
        cmake_layout(self)
        # Set source folder to the project root relative to conanfile.py (which is in cpp/)
        self.folders.source = ".."
        # Adjust build folder to be lowercase build_type, consistent with some conventions
        self.folders.build = os.path.join("build", str(self.settings.build_type).lower())
        self.folders.generators = os.path.join(self.folders.build, "conan")

    def requirements(self):
        self.requires("boost/1.81.0")
        self.requires("rapidjson/1.1.0")
        if self.options.bond_enable_grpc:
            self.requires("grpc/1.50.1")

    def tool_requires(self):
        # This is a placeholder. A robust solution would be to use a conan package for stack
        # or ensure it's available on the system. For now, we'll assume stack is in PATH.
        # Example: self.tool_requires("stack_installer/latest@user/channel")
        pass # Assuming stack is in PATH for now

    def source(self):
        # Source method is not strictly needed here because:
        # 1. `exports_sources` makes the source code available in the build context.
        # 2. `self.folders.source = ".."` correctly points to the project root relative to conanfile.py.
        # If we needed to copy the LICENSE to self.recipe_folder for some reason, it could be done here.
        # e.g., shutil.copy(os.path.join(self.source_folder, "LICENSE"), os.path.join(self.recipe_folder, "LICENSE"))
        # However, package() can now directly access it from self.source_folder.
        pass

    def generate(self):
        tc = CMakeToolchain(self)
        tc.variables["BOND_ENABLE_GRPC"] = self.options.bond_enable_grpc
        if self.settings.os != "Windows" and self.options.fPIC:
            tc.variables["CMAKE_POSITION_INDEPENDENT_CODE"] = True

        # Define the path where gbc executable will be located after _build_gbc()
        self._gbc_executable_path = os.path.join(self.build_folder, "gbc_install", "bin", "gbc")
        if self.settings.os == "Windows":
            self._gbc_executable_path += ".exe"

        tc.variables["BOND_COMPILER_EXECUTABLE"] = self._gbc_executable_path
        # BOND_ROOT_DIR should point to the root of the Bond project, which is self.source_folder here.
        tc.variables["BOND_ROOT_DIR"] = self.source_folder
        tc.generate()

    def _build_gbc(self):
        self.output.info("Building Bond compiler (gbc) with Stack...")
        compiler_dir = os.path.join(self.source_folder, "compiler") # Should be ../compiler from cpp/

        gbc_install_bin_dir = os.path.join(self.build_folder, "gbc_install", "bin")
        os.makedirs(gbc_install_bin_dir, exist_ok=True)

        stack_yaml_path = os.path.join(compiler_dir, "stack.yaml")
        if not os.path.exists(stack_yaml_path):
            self.output.error(f"stack.yaml not found at {stack_yaml_path}")
            # Try to list contents of compiler_dir for diagnostics
            if os.path.exists(compiler_dir):
                self.output.warn(f"Contents of {compiler_dir}: {os.listdir(compiler_dir)}")
            else:
                self.output.warn(f"Compiler directory {compiler_dir} does not exist.")
            raise Exception(f"stack.yaml not found for gbc build at {stack_yaml_path}. Check source folder configuration and exports_sources.")

        stack_args = [
            "stack",
            "--stack-yaml", stack_yaml_path,
            "install",
            "--local-bin-path", gbc_install_bin_dir
        ]
        try:
            self.run(" ".join(stack_args), cwd=compiler_dir)
            self.output.info(f"gbc potentially installed to: {gbc_install_bin_dir}")
            if not os.path.exists(self._gbc_executable_path):
                self.output.error(f"gbc executable not found at {self._gbc_executable_path} after stack install.")
                if os.path.exists(gbc_install_bin_dir):
                    self.output.error(f"Contents of {gbc_install_bin_dir}: {os.listdir(gbc_install_bin_dir)}")
                else:
                    self.output.error(f"Directory {gbc_install_bin_dir} does not exist.")
                raise Exception("gbc build failed or executable not found.")
        except Exception as e:
            self.output.error(f"Failed to build gbc: {e}")
            raise

    def build(self):
        self._build_gbc()
        cmake = CMake(self)
        # Configure CMake for the C++ library, pointing to cpp/CMakeLists.txt
        cmake.configure(build_script_folder=self.recipe_folder)
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()
        # Copy LICENSE file from the source_folder (project root) to the package's licenses folder
        copy(self, "LICENSE", src=self.source_folder, dst=os.path.join(self.package_folder, "licenses"))

    def package_info(self):
        self.cpp_info.libs = ["bond", "bond_apply"]
        # self.cpp_info.includedirs needs to be relative to the package_folder
        # By default, cmake.install() with CMAKE_INSTALL_PREFIX set to self.package_folder
        # and `DESTINATION include` in CMakeLists.txt for headers should handle this.
        # If headers are in <package_folder>/include, then cpp_info.includedirs = ["include"] is correct.
        self.cpp_info.includedirs = ["include"]
        self.cpp_info.libdirs = ["lib"] # Same for libs, assuming they are in <package_folder>/lib

        if self.options.bond_enable_grpc:
            self.cpp_info.defines.append("BOND_ENABLE_GRPC=1")

    def test(self):
        if can_run(self):
            # The CMake object needs to be initialized for the build context
            cmake = CMake(self)
            try:
                # self.build_folder is where the build happened (e.g., cpp/build/release)
                # CMake.test() runs ctest from the build directory.
                self.output.info(f"Running tests from build folder: {self.build_folder}")
                # Ensure that the working directory for tests is appropriate if tests rely on relative paths.
                # CMake.test() by default runs from self.build_folder/self.settings.build_type (if not multi-config)
                # or just self.build_folder. Given our layout, self.build_folder is already specific.
                cmake.test()
            except Exception as e:
                self.output.error(f"Tests failed during `cmake.test()`: {e}")
                # It's often good to see the test output directly if possible.
                # CTest logs are usually in Testing/Temporary under the build directory.
                # Example: test_log_dir = os.path.join(self.build_folder, "Testing", "Temporary")
                # if os.path.exists(test_log_dir):
                #    self.output.info(f"CTest logs might be in: {test_log_dir}")
                raise
        else:
            self.output.info("Skipping tests: can_run() is false (likely cross-compiling without emulator).")
