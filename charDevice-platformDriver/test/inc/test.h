#ifndef TEST_H
#define TEST_H
#include <stdio.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>

#define TEMP_ADD 0x48
#define CONST_CONV_TEMP 0.0625
#define CHAR_BIT 8	// Cantidad de bits por byte

struct ModuleData
{
	int16_t accel_outx;
	int16_t accel_outy;
	int16_t accel_outz;
	int16_t temp;
	int16_t gyro_outx;
	int16_t gyro_outy;
	int16_t gyro_outz;
};

struct ModuleData_Float
{
	float accel_outx;
	float accel_outy;
	float accel_outz;
	float temp;
	float gyro_outx;
	float gyro_outy;
	float gyro_outz;
};

#endif