
#   ____  _____ __  __
#  / [] \|_   _|\ \/ /    appletv 2 build flag setup
# /__/\__\ |_|   \__/ 2   for xcode/clang build host

# the values here have not been extensively tested but seem to
# work for me using osx 10.11/xcode7/clang3.9 (svn build)

CC=clang
LD=clang

SDKPATH := $(shell xcrun --sdk iphoneos -show-sdk-path)
ifeq ($(strip $(wildcard $(SDKPATH)/Librar?)),)
  $(error iphone sdk path not found)
endif

TVFLAGS += -isysroot $(SDKPATH)
TVFLAGS += -arch armv7
FLAGS += -miphoneos-version-min=6.1

CFLAGS += $(TVFLAGS)
LDFLAGS += $(TVFLAGS)

# completely untested
#CXX=clang
#CXXFLAGS += $(TVFLAGS)

# see /Library/CoreServices/SystemVersion.plist on a device for appropriate
# values for -miphoneos-version-min and potentially other flags. 
# as of writing ProductVersion reads 6.1 for me.

