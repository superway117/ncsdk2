# Copyright 2018 Intel Corporation.
# The source code, information and material ("Material") contained herein is
# owned by Intel Corporation or its suppliers or licensors, and title to such
# Material remains with Intel Corporation or its suppliers or licensors.
# The Material contains proprietary information of Intel or its suppliers and
# licensors. The Material is protected by worldwide copyright laws and treaty
# provisions.
# No part of the Material may be used, copied, reproduced, modified, published,
# uploaded, posted, transmitted, distributed or disclosed in any way without
# Intel's prior express written permission. No license under any patent,
# copyright or other intellectual property rights in the Material is granted to
# or conferred upon you, either expressly, by implication, inducement, estoppel
# or otherwise.
# Any license under such intellectual property rights must be express and
# approved by Intel in writing.

from enum import Enum
from ctypes import *

MAX_OPTIMISATIONS = 40
OPTIMISATION_NAME_LEN = 50

# Redefine Option enum to add high class values
class Option(Enum):
    CLASS0 = 0
    CLASS1 = 1
    CLASS2 = 2
    CLASS3 = 3


class GraphOptionClass2(Enum):
    RW_BATCH_SIZE = 1201                   # configure batch size. 1 by default
    RW_RUNTIME_CONFIG = 1202               # blob config file with resource hints
    NOT_IMPLEMENTED = 1203


class GraphOptionClass3(Enum):
    NOT_IMPLEMENTED = 1300


class DeviceOptionClass2(Enum):
    RW_TEMP_LIM_LOWER = 2200               # Temperature for short sleep, float, not for general use
    RW_TEMP_LIM_HIGHER = 2201              # Temperature for long sleep, float, not for general use
    RW_BACKOFF_TIME_NORMAL = 2202          # Normal sleep in ms, int, not for general use
    RW_BACKOFF_TIME_HIGH = 2203            # Short sleep in ms, int, not for general use
    RW_BACKOFF_TIME_CRITICAL = 2204        # Long sleep in ms, int, not for general use
    RW_TEMPERATURE_DEBUG = 2205            # Stop on critical temperature, int, not for general use
    RO_OPTIMISATION_LIST = 2206            # Return optimisations list, char *, not for general use



class DeviceOptionClass3(Enum):
    RW_SHELL_ENABLE = 2300             # Activate RTEMS shell.
    RW_LOG_LEVEL = 2301                # Set/Get log level for the NC infrastructure on the device
    RW_MVTENSOR_LOG_LEVEL = 2302       # Set/Get log level of MV_Tensor
    RW_XLINK_LOG_LEVEL = 2303          # Set/Get XLink log level
    NOT_IMPLEMENTED = 2304


def device_set_option_hc(self, f, Status, option, value):
    """
    Set an optional feature of the device.
    :param option: a DeviceOption enumeration
    :param value: value for the option
    """

    if (option == DeviceOptionClass2.RW_TEMP_LIM_LOWER or option == DeviceOptionClass2.RW_TEMP_LIM_HIGHER):
        data = c_float(value)
    elif (option == DeviceOptionClass2.RW_BACKOFF_TIME_NORMAL or option == DeviceOptionClass2.RW_BACKOFF_TIME_HIGH or
          option == DeviceOptionClass2.RW_BACKOFF_TIME_CRITICAL or option == DeviceOptionClass2.RW_TEMPERATURE_DEBUG):
        data = c_int(value)
    elif (option == DeviceOptionClass3.RW_SHELL_ENABLE or option == DeviceOptionClass3.RW_LOG_LEVEL or
          option == DeviceOptionClass3.RW_MVTENSOR_LOG_LEVEL or option == DeviceOptionClass3.RW_XLINK_LOG_LEVEL):
        data = c_int(value)
    else:
        raise Exception(Status.INVALID_PARAMETERS)

    # Write device option
    return data


def device_get_option_hc(self, f, Status, option):
    """
    Get optional information from the device.
    :param option: a DeviceOption enumeration
    :return option: value for the option
    """

    # Determine option data type
    if (option == DeviceOptionClass2.RW_TEMP_LIM_LOWER or option == DeviceOptionClass2.RW_TEMP_LIM_HIGHER):
        optdata = c_float()
        def get_optval(raw_optdata): return raw_optdata.value
    elif (option == DeviceOptionClass2.RW_BACKOFF_TIME_NORMAL or option == DeviceOptionClass2.RW_BACKOFF_TIME_HIGH or
          option == DeviceOptionClass2.RW_BACKOFF_TIME_CRITICAL or option == DeviceOptionClass2.RW_TEMPERATURE_DEBUG):
        optdata = c_int()
        def get_optval(raw_optdata): return raw_optdata.value
    elif (option == DeviceOptionClass3.RW_SHELL_ENABLE or option == DeviceOptionClass3.RW_LOG_LEVEL or
          option == DeviceOptionClass3.RW_MVTENSOR_LOG_LEVEL or option == DeviceOptionClass3.RW_XLINK_LOG_LEVEL):
        optdata = c_int()
        def get_optval(raw_optdata): return raw_optdata.value
    elif option == DeviceOptionClass2.RO_OPTIMISATION_LIST:
            # list of strings
            optdata = create_string_buffer(
                MAX_OPTIMISATIONS * OPTIMISATION_NAME_LEN)

            def get_optval(raw_optdata):
                return raw_optdata.raw.decode().replace('\x00', ' ').split()
    else:
        raise Exception(Status.INVALID_PARAMETERS)

    # Read device option
    optsize = c_uint(sizeof(optdata))
    status = f.ncDeviceGetOption(self.handle, option.value, byref(optdata), byref(optsize))
    if status != Status.OK.value:
        raise Exception(Status(status))

    # Handle int or float
    if (option == DeviceOptionClass2.RW_TEMP_LIM_LOWER or option == DeviceOptionClass2.RW_TEMP_LIM_HIGHER or
        option == DeviceOptionClass2.RW_BACKOFF_TIME_NORMAL or option == DeviceOptionClass2.RW_BACKOFF_TIME_HIGH or
        option == DeviceOptionClass2.RW_BACKOFF_TIME_CRITICAL or option == DeviceOptionClass2.RW_TEMPERATURE_DEBUG or
        option == DeviceOptionClass3.RW_SHELL_ENABLE or option == DeviceOptionClass3.RW_LOG_LEVEL or
        option == DeviceOptionClass3.RW_MVTENSOR_LOG_LEVEL or option == DeviceOptionClass3.RW_XLINK_LOG_LEVEL or
        option == DeviceOptionClass2.RO_OPTIMISATION_LIST):
            return get_optval(optdata)

    # Create array buffer
    v = create_string_buffer(optsize.value)
    memmove(v, optdata, optsize.value)

    # Default: return raw data
    return int.from_bytes(v.raw, byteorder='little')


def graph_get_option_hc(self, f, Status, option):
    """
    Get optional information from the graph.
    :param option: a GraphOption enumeration
    :return option: value for the option
    """

    # Determine option data type
    if (option == GraphOptionClass2.RW_BATCH_SIZE):
        optdata = c_int()
    elif (option == GraphOptionClass2.RW_RUNTIME_CONFIG):
        optdata = POINTER(c_byte)()
    else:
        raise Exception(Status.INVALID_PARAMETERS)

    # Read device option
    optsize = c_uint(sizeof(optdata))
    status = f.ncGraphGetOption(self.handle, option.value, byref(optdata), byref(optsize))
    if status != Status.OK.value:
        raise Exception(Status(status))

    # Handle int or float
    if (option == GraphOptionClass2.RW_BATCH_SIZE):
        return optdata.value

    # Create array buffer
    v = create_string_buffer(optsize.value)
    memmove(v, optdata, optsize.value)

    # Handle string
    """TODO: how to handle RW_RUNTIME_CONFIG?"""
    if (option == GraphOptionClass2.RW_RUNTIME_CONFIG):
        return v.raw[0:v.raw.find(0)].decode()

    # Default: return raw data
    return int.from_bytes(v.raw, byteorder='little')
