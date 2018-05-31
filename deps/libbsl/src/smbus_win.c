#pragma once

#include <Windows.h>
#include <WinIoCtl.h>
#include <string.h>

#include "hddl-i2c.h"
#include "hddl_bsl_priv.h"

#define FILE_DEVICE_HDDLI2C             0x0116
#define IOCTL_HDDLI2C_REGISTER_READ              CTL_CODE(FILE_DEVICE_HDDLI2C, 0x700, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_HDDLI2C_REGISTER_WRITE             CTL_CODE(FILE_DEVICE_HDDLI2C, 0x701, METHOD_BUFFERED, FILE_ANY_ACCESS)


#define SPBHDDL_I2C_NAME L"HDDLSMBUS"

#define SPBHDDL_I2C_SYMBOLIC_NAME L"\\DosDevices\\" SPBHDDL_I2C_NAME
#define SPBHDDL_I2C_USERMODE_PATH L"\\\\.\\" SPBHDDL_I2C_NAME
#define SPBHDDL_I2C_USERMODE_PATH_SIZE sizeof(SPBHDDL_I2C_USERMODE_PATH)


int i2c_fetch_address_by_scan(int start_addr,
    int end_addr,
    int* dev_addr)
{
    int i;
    int find_index = 0;
    int res = -1;
    DWORD dwOutput;
    UCHAR outBuff = 0;
    UCHAR data[3] = { 0x1f, 0x00, 0xff }; // Address, Register, data

    HANDLE hDevice = CreateFileW(SPBHDDL_I2C_USERMODE_PATH,
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_OVERLAPPED,
        NULL);
    if (hDevice == INVALID_HANDLE_VALUE)
    {
        return -1;
    }
    BOOL ioresult;
    for (i = start_addr; i <= end_addr; i++)
    {
        /* Set slave address */
        data[0] = i;
        ioresult = DeviceIoControl(hDevice, IOCTL_HDDLI2C_REGISTER_READ, data, sizeof(data), &outBuff, sizeof(outBuff), &dwOutput, NULL);

        if (ioresult)
        {
            if (find_index < I2C_MAX_I2C_ADDR_NUM)
            {
                dev_addr[find_index] = i;
                find_index++;
            }
            if (find_index >= I2C_MAX_I2C_ADDR_NUM)
                break;
        }
        else
        {
            DWORD error_code = GetLastError();
            if (error_code == 55)
                continue;
            else
            {
                break;
            }
        }
    }
    CloseHandle(hDevice);
    return find_index;
}

int i2c_write_byte(int i2c_addr, int reg, int value)
{
    DWORD dwOutput;
    UCHAR data[3] = { 0x1f, 0x01, 0xff }; // Address, Register, data

    data[0] = (UCHAR)(i2c_addr);
    data[1] = (UCHAR)(reg);
    data[2] = (UCHAR)(value & 0xFF);

    HANDLE hDevice = CreateFileW(SPBHDDL_I2C_USERMODE_PATH,
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_OVERLAPPED,
        NULL);
    if (hDevice == INVALID_HANDLE_VALUE)
    {
        return -1;
    }
    BOOL ioresult;
    ioresult = DeviceIoControl(hDevice, IOCTL_HDDLI2C_REGISTER_WRITE, data, sizeof(data), NULL, 0, &dwOutput, NULL);
    CloseHandle(hDevice);
    if (ioresult)
        return 0;
    else
        return -1;
}


int i2c_read_byte(int i2c_addr, int reg, int* value)
{
    DWORD dwOutput;
    UCHAR outBuff = 0;
    UCHAR data[3] = { 0x1f, 0x01, 0xff }; // Address, Register, data

    data[0] = (UCHAR)(i2c_addr);
    data[1] = (UCHAR)(reg);

    HANDLE hDevice = CreateFileW(SPBHDDL_I2C_USERMODE_PATH,
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_OVERLAPPED,
        NULL);
    if (hDevice == INVALID_HANDLE_VALUE)
    {
        return -1;
    }
    BOOL ioresult;
    ioresult = DeviceIoControl(hDevice, IOCTL_HDDLI2C_REGISTER_READ, data, sizeof(data), &outBuff, sizeof(outBuff), &dwOutput, NULL);
    CloseHandle(hDevice);
    if (ioresult)
    {
        *value = outBuff;
        return 0;
    }
    else
        return -1;
}

