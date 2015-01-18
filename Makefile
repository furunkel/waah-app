
# Change these if you want to cross build for Android
export ANDROID_TARGET_ARCH_ABI:=armeabi-v7a
export ANDROID_NDK_HOME:=/home/user/android-ndk-r10d/
export ANDROID_STANDALONE_TOOLCHAIN:=/home/user/android-14-toolchain

# Change these if you want to cross build for Windows
export MINGW_TOOLCHAIN:=/opt/mingw32/
export MINGW_TOOLCHAIN_PREFIX:=i686-w64-mingw32

.PHONY : test
test:
	ruby ./run_test.rb test --verbose

.PHONY : all
all:
	echo "NOOP"

.PHONY : clean
clean:
	ruby ./run_test.rb clean
