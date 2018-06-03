# ------------------------------[ App ]--------------------------------------------#
# Default application name is the same as the folder it is in.
# This can be overridden here if something different is required
#APPNAME ?= $(notdir $(shell pwd))

# Verbose or not, common out for more verbosity
ECHO ?=@

# ------------------------------[ Build overrides ]--------------------------------#
# overrides that need to be set before including generic.mk
#ShaveSymObject   = $(DirAppObjDir)/$(APPNAME).sym.o

MV_SOC_PLATFORM ?= myriad2
MV_SOC_REV      ?= ma2480

ComponentList_LOS    := PipePrint FatalExtension

# Ensure that the we are using the correct rtems libs etc
MV_SOC_OS = rtems
RTEMS_BUILD_NAME = b-prebuilt

# Use this makefile variable to specify the USB device class protos to be included into the build
# Available USB device protos: protohid protomsc protondfu protovideo protovsc2
MV_USB_PROTOS = protovsc2
# Uncomment the following line to use the debug build of USB libraries
#RTEMS_USB_LIB_BUILD_TYPE = debug

# ------------------------------[ Tools ]------------------------------------------#
# Hardcode tool version here if needed, otherwise defaults to Latest
#MV_TOOLS_VERSION = Latest

# Set MV_COMMON_BASE relative to mdk directory location (but allow user to override in environment)
MV_COMMON_BASE  ?= ../../../../../common

# Include the generic Makefile
include $(MV_COMMON_BASE)/generic.mk

# -------------------------------- [ Build Options ] ------------------------------ #
TEST_TAGS := "MA2480"
TEST_TYPE := "MANUAL"
TEST_HW_PLATFORM := "MV0235_MA2480"

