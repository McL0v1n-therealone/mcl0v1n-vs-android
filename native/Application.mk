# Mcl0v1n vs android - NDK Application config
# Galaxy S III (GT-I9300) is an Exynos 4412, Cortex-A9 => armeabi-v7a.
APP_ABI       := armeabi-v7a
APP_PLATFORM  := android-19       # Android 4.4.4 / KitKat
APP_STL       := system
APP_OPTIM     := release
# r10e GCC 4.8 toolchain matches the KitKat ABI the best.
NDK_TOOLCHAIN_VERSION := 4.8
