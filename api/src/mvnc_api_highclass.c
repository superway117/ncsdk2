/*
* Copyright 2018 Intel Corporation.
* The source code, information and material ("Material") contained herein is
* owned by Intel Corporation or its suppliers or licensors, and title to such
* Material remains with Intel Corporation or its suppliers or licensors.
* The Material contains proprietary information of Intel or its suppliers and
* licensors. The Material is protected by worldwide copyright laws and treaty
* provisions.
* No part of the Material may be used, copied, reproduced, modified, published,
* uploaded, posted, transmitted, distributed or disclosed in any way without
* Intel's prior express written permission. No license under any patent,
* copyright or other intellectual property rights in the Material is granted to
* or conferred upon you, either expressly, by implication, inducement, estoppel
* or otherwise.
* Any license under such intellectual property rights must be express and
* approved by Intel in writing.
*/

#define _GNU_SOURCE
#if (defined(_WIN32) || defined(_WIN64) )
#include "gettime.h"
#else
#include <unistd.h>
#include <XLinkConsole.h>
#endif
#include <XLink.h>
#include "ncCommPrivate.h"
#include "ncPrivateTypes.h"
#include "ncHighClass.h"

#include <stdlib.h>
#include <string.h>

#define MVLOG_UNIT_NAME ncAPIHighClass
#include "mvLog.h"

static void initShell(struct _devicePrivate_t *d, int portNum)
{
#if (defined(_WIN32) || defined(_WIN64) )
#else
    initXlinkShell(d->usb_link, portNum);
#endif
}

ncStatus_t setDeviceOptionClass2(struct _devicePrivate_t *d,
                                 int option,
                                 const void *data, unsigned int dataLength)
{
    deviceCommand_t config;
    config.optionClass = NC_OPTION_CLASS2;
    if (d->dev_attr.max_device_opt_class < NC_OPTION_CLASS2) {
        return NC_UNAUTHORIZED;
    }
    if ((option == NC_RW_DEVICE_BACKOFF_TIME_NORMAL ||
         option == NC_RW_DEVICE_BACKOFF_TIME_HIGH ||
         option == NC_RW_DEVICE_BACKOFF_TIME_CRITICAL ||
         option == NC_RW_DEVICE_TEMPERATURE_DEBUG) &&
        dataLength < sizeof(int)) {
        mvLog(MVLOG_ERROR, "The dataLength is smaller that required %d",
              sizeof(int));
        return NC_INVALID_DATA_LENGTH;
    }
    if ((option == NC_RW_DEVICE_TEMP_LIM_LOWER ||
         option == NC_RW_DEVICE_TEMP_LIM_HIGHER) &&
        dataLength < sizeof(float)) {
        mvLog(MVLOG_ERROR, "The dataLength is smaller that required %d",
              sizeof(float));
        return NC_INVALID_DATA_LENGTH;
    }
    switch (option) {
    case NC_RW_DEVICE_TEMP_LIM_LOWER:
        config.type.c2 = CLASS2_SET_TEMP_LIM_LOWER;
        config.data = *(float *) data;
        break;
    case NC_RW_DEVICE_TEMP_LIM_HIGHER:
        config.type.c2 = CLASS2_SET_TEMP_LIM_HIGHER;
        config.data = *(float *) data;
        break;
    case NC_RW_DEVICE_BACKOFF_TIME_NORMAL:
        config.type.c2 = CLASS2_SET_BACKOFF_TIME_NORMAL;
        config.data = *(int *) data;
        break;
    case NC_RW_DEVICE_BACKOFF_TIME_HIGH:
        config.type.c2 = CLASS2_SET_BACKOFF_TIME_HIGH;
        config.data = *(int *) data;
        break;
    case NC_RW_DEVICE_BACKOFF_TIME_CRITICAL:
        config.type.c2 = CLASS2_SET_BACKOFF_TIME_CRITICAL;
        config.data = *(int *) data;
        break;
    case NC_RW_DEVICE_TEMPERATURE_DEBUG:
        config.type.c2 = CLASS2_SET_TEMPERATURE_DEBUG;
        config.data = *(int *) data;
        break;
    default:
        return NC_INVALID_PARAMETERS;
    }
    pthread_mutex_lock(&d->dev_stream_m);
    if (XLinkWriteData(d->device_mon_stream_id, &config, sizeof(config)) !=
        X_LINK_SUCCESS) {
        pthread_mutex_unlock(&d->dev_stream_m);
        return NC_ERROR;
    }
    pthread_mutex_unlock(&d->dev_stream_m);
    return NC_OK;
}

ncStatus_t setDeviceOptionClass3(struct _devicePrivate_t * d,
                                 int option, const void *data,
                                 unsigned int dataLength)
{

    if (option == NC_RW_DEVICE_SHELL_ENABLE && dataLength < sizeof(int)) {
        mvLog(MVLOG_ERROR, "The dataLength is smaller that required %d",
              sizeof(int));
        return NC_INVALID_DATA_LENGTH;
    }
    if ((option == CLASS3_SET_LOG_LEVEL_GLOBAL ||
         option == NC_RW_DEVICE_MVTENSOR_LOG_LEVEL ||
         option == NC_RW_DEVICE_XLINK_LOG_LEVEL) &&
        dataLength < sizeof(uint32_t)) {
        mvLog(MVLOG_ERROR, "The dataLength is smaller that required %d",
              sizeof(uint32_t));
        return NC_INVALID_DATA_LENGTH;
    }
    deviceCommand_t config;
    config.optionClass = NC_OPTION_CLASS3;
    if (d->dev_attr.max_device_opt_class < NC_OPTION_CLASS3) {
        return NC_UNAUTHORIZED;
    }
    switch (option) {
    case NC_RW_DEVICE_SHELL_ENABLE:
        initShell(d, *(int *) data);
        config.type.c3 = CLASS3_START_SHELL;
        config.data = 0;
        break;
    case NC_RW_DEVICE_LOG_LEVEL:
        config.type.c3 = CLASS3_SET_LOG_LEVEL_GLOBAL;
        config.data = *(uint32_t *) data;
        break;
    case NC_RW_DEVICE_MVTENSOR_LOG_LEVEL:
        config.type.c3 = CLASS3_SET_LOG_LEVEL_FATHOM;
        config.data = *(uint32_t *) data;
        break;
    case NC_RW_DEVICE_XLINK_LOG_LEVEL:
        config.type.c3 = CLASS3_SET_LOG_LEVEL_XLINK;
        config.data = *(uint32_t *) data;
        break;
    default:
        return NC_INVALID_PARAMETERS;
        break;
    }
    sleep(1);
    printf("0-----------------------\n");
    XLinkWriteData(d->device_mon_stream_id, &config, sizeof(config));
    return NC_OK;
}

static ncStatus_t getOptimisationList(struct _devicePrivate_t *d)
{
    int i;
    char *p;

    if (d->optimisation_list)
        return NC_OK;

    d->optimisation_list = calloc(NC_OPTIMISATION_LIST_BUFFER_SIZE, 1);
    if (!d->optimisation_list)
        return NC_OUT_OF_MEMORY;

    deviceCommand_t config;
    config.type.c2 = CLASS2_OPT_LIST;
    config.optionClass = NC_OPTION_CLASS2;
    pthread_mutex_lock(&d->dev_stream_m);
    if (XLinkWriteData(d->device_mon_stream_id, (const uint8_t *) &config,
                       sizeof(config)) != 0) {
        pthread_mutex_unlock(&d->dev_stream_m);
        return NC_ERROR;
    }
    streamPacketDesc_t *packet;
    if (XLinkReadData(d->device_mon_stream_id, &packet) != 0) {
        pthread_mutex_unlock(&d->dev_stream_m);
        return NC_ERROR;
    }
    pthread_mutex_unlock(&d->dev_stream_m);
    memcpy(d->optimisation_list, packet->data, packet->length);
    XLinkReleaseData(d->device_mon_stream_id);

    for (i = 0; i < NC_MAX_OPTIMISATIONS; i++) {
        p = strchr(d->optimisation_list + i * NC_OPTIMISATION_NAME_LEN, '~');
        if (p)
            *p = 0;
    }
    return NC_OK;
}
ncStatus_t getDeviceOptionClass2(struct _devicePrivate_t * d,
                                 int option,
                                 void *data, unsigned int *dataLength)
{
    switch (option) {
    case NC_RW_DEVICE_TEMP_LIM_LOWER:
    case NC_RW_DEVICE_TEMP_LIM_HIGHER:
    case NC_RW_DEVICE_BACKOFF_TIME_NORMAL:
    case NC_RW_DEVICE_BACKOFF_TIME_HIGH:
    case NC_RW_DEVICE_BACKOFF_TIME_CRITICAL:
    case NC_RW_DEVICE_TEMPERATURE_DEBUG:{
            /* check dataLength before sending command */
            if ((option == NC_RW_DEVICE_TEMP_LIM_LOWER) ||
                (option == NC_RW_DEVICE_TEMP_LIM_HIGHER)) {
                if (*dataLength < sizeof(float)) {
                    *dataLength = sizeof(float);
                    mvLog(MVLOG_ERROR,
                          "The dataLength is smaller that required %d",
                          *dataLength);
                    return NC_INVALID_DATA_LENGTH;
                }
            } else if ((option == NC_RW_DEVICE_BACKOFF_TIME_NORMAL) ||
                       (option == NC_RW_DEVICE_BACKOFF_TIME_HIGH) ||
                       (option == NC_RW_DEVICE_BACKOFF_TIME_CRITICAL)) {
                if (*dataLength < sizeof(uint32_t)) {
                    *dataLength = sizeof(uint32_t);
                    mvLog(MVLOG_ERROR,
                          "The dataLength is smaller that required %d",
                          *dataLength);
                    return NC_INVALID_DATA_LENGTH;
                }
            } else {
                if (*dataLength < sizeof(int)) {
                    *dataLength = sizeof(int);
                    mvLog(MVLOG_ERROR,
                          "The dataLength is smaller that required %d",
                          *dataLength);
                    return NC_INVALID_DATA_LENGTH;
                }
            }

            deviceCommand_t config;
            /* Formula to translate class1 index from mvncapi to internal FW api:
             * CLASS1_GET_* = (x - 2100) * 2
             * CLASS1_SET_* = (x - 2100) * 2 + 1
             */
            config.optionClass = NC_OPTION_CLASS2;
            config.type.c2 = (option - NC_RW_DEVICE_TEMP_LIM_LOWER) * 2;

            pthread_mutex_lock(&d->dev_stream_m);
            if (XLinkWriteData
                (d->device_mon_stream_id, (const uint8_t *) &config,
                 sizeof(config)) != X_LINK_SUCCESS) {
                pthread_mutex_unlock(&d->dev_stream_m);
                return NC_ERROR;
            }
            streamPacketDesc_t *packet;

            if (XLinkReadData(d->device_mon_stream_id, &packet) != 0) {
                pthread_mutex_unlock(&d->dev_stream_m);
                return NC_ERROR;
            }
            pthread_mutex_unlock(&d->dev_stream_m);

            /* Validate size based on class */
            if ((option == NC_RW_DEVICE_TEMP_LIM_LOWER) ||
                (option == NC_RW_DEVICE_TEMP_LIM_HIGHER)) {
                if (packet->length != sizeof(float)) {
                    XLinkReleaseData(d->device_mon_stream_id);
                    return NC_ERROR;
                }
                *(float *) data = *(float *) packet->data;
                *dataLength = sizeof(float);
            } else if ((option == NC_RW_DEVICE_BACKOFF_TIME_NORMAL) ||
                       (option == NC_RW_DEVICE_BACKOFF_TIME_HIGH) ||
                       (option == NC_RW_DEVICE_BACKOFF_TIME_CRITICAL)) {
                if (packet->length != sizeof(uint32_t)) {
                    XLinkReleaseData(d->device_mon_stream_id);
                    return NC_ERROR;
                }
                *(uint32_t *) data = *(uint32_t *) packet->data;
                *dataLength = sizeof(uint32_t);
            } else {
                if (packet->length != sizeof(int)) {
                    XLinkReleaseData(d->device_mon_stream_id);
                    return NC_ERROR;
                }
                *(int *) data = *(int *) packet->data;
                *dataLength = sizeof(int);
            }
            XLinkReleaseData(d->device_mon_stream_id);
            break;
        }
    case NC_RO_DEVICE_OPTIMISATION_LIST:
        if (*dataLength < NC_OPTIMISATION_LIST_BUFFER_SIZE) {
            mvLog(MVLOG_ERROR,
                  "data length of output buffer (%d) is smaller that required (%d)!\n",
                  *dataLength, NC_OPTIMISATION_LIST_BUFFER_SIZE);
            *dataLength = NC_OPTIMISATION_LIST_BUFFER_SIZE;
            return NC_INVALID_DATA_LENGTH;
        }
        int rc = getOptimisationList(d);
        if (rc) {
            return rc;
        }
        memcpy((char *) data, d->optimisation_list,
               NC_OPTIMISATION_LIST_BUFFER_SIZE);
        *dataLength = NC_OPTIMISATION_LIST_BUFFER_SIZE;
        break;
    default:
        return NC_INVALID_PARAMETERS;
    }
    return NC_OK;
}

ncStatus_t getDeviceOptionClass3(struct _devicePrivate_t * d,
                                 int option,
                                 void *data, unsigned int *dataLength)
{
    return NC_UNSUPPORTED_FEATURE;
}

ncStatus_t getGraphOptionClass2(struct _graphPrivate_t * g,
                                int option,
                                void *data, unsigned int *dataLength)
{
    switch (option) {
    case NC_RW_GRAPH_BATCH_SIZE:
        if (*dataLength < sizeof(int)) {
            mvLog(MVLOG_ERROR, "The dataLength is smaller that required %d",
                  *dataLength);
            return NC_INVALID_DATA_LENGTH;
        }
        *(int *) data = g->batch_size;
        *dataLength = sizeof(int);
        return NC_OK;
    case NC_RW_GRAPH_RUNTIME_CONFIG:
        return NC_UNSUPPORTED_FEATURE;
    default:
        mvLog(MVLOG_ERROR, "There is no such option in class 2");
        return NC_INVALID_PARAMETERS;
    }
    return NC_OK;
}

ncStatus_t getGraphOptionClass3(struct _graphPrivate_t * g,
                                int option,
                                void *data, unsigned int *dataLength)
{
    return NC_UNSUPPORTED_FEATURE;
}

ncStatus_t setGraphOptionClass2(struct _graphPrivate_t * g,
                                int option,
                                const void *data, unsigned int dataLength)
{
    switch (option) {
    case NC_RW_GRAPH_BATCH_SIZE:
    case NC_RW_GRAPH_RUNTIME_CONFIG:
        return NC_UNSUPPORTED_FEATURE;
    default:
        mvLog(MVLOG_ERROR, "There is no such option in class 2");
        return NC_INVALID_PARAMETERS;
    }
    return NC_OK;
}

ncStatus_t setGraphOptionClass3(struct _graphPrivate_t * g,
                                int option,
                                const void *data, unsigned int dataLength)
{
    return NC_UNSUPPORTED_FEATURE;
}
