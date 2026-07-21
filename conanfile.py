from conan import ConanFile

class Caelispect(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = "VirtualBuildEnv", "CMakeConfigDeps"

    options = {
        "macos_ne": [True, False]
    }
    default_options = {
        "macos_ne": False
    }

    def requirements(self):
        os = str(self.settings.os)

        has_ne = os == "iOS" or (os == "Macos" and self.options.macos_ne)
        has_service = os == "Windows" or os == "Linux" or (os == "Macos" and not has_ne)

        if has_service:
            if os == "Windows":
                self.requires("awg-windows/0.1.9")
                self.requires("win-split-tunnel/1.2.5.0")
                self.requires("wintun/0.14.1")
            else:
                self.requires("awg-go/0.2.18")


        if has_ne:
            self.requires("awg-apple/2.0.2")

        if os == "Android":
            self.requires("awg-android/2.0.1")

        self.requires("openssl/3.6.2")
