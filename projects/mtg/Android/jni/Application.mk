APP_PROJECT_PATH := $(call my-dir)/..
APP_CPPFLAGS += -frtti -fexceptions
APP_ABI := arm64-v8a
APP_PLATFORM := android-21
APP_CFLAGS += -march=armv8.1-a
APP_CPPFLAGS += -D__ARM_FEATURE_LSE=1
# Strip the absolute build path (which includes the local username) from __FILE__ and debug info,
# so it isn't embedded in libmain.so. Maps the repo root to '.' -> relative paths only.
WAGIC_ROOT := $(abspath $(APP_PROJECT_PATH)/../../..)
APP_CFLAGS   += -ffile-prefix-map=$(WAGIC_ROOT)=.
APP_CPPFLAGS += -ffile-prefix-map=$(WAGIC_ROOT)=.
#APP_ABI := x86 # mainly for emulators
APP_STL := c++_static
APP_MODULES := libpng libjpeg main SDL

#APP_OPTIM is 'release' by default
APP_OPTIM := release
