#ifndef SENSOR_H
#define SENSOR_H

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>
#include <netinet/in.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <unistd.h>
#include <fcntl.h>
#include <semaphore.h>


#define KEY_PATH        "/home/debian/TD3_2Cuat/Server/src/cfg.txt"
#define SEM_NAME        "BOCAAA"
#define CFG_PATH		"/home/debian/TD3_2Cuat/Sensor/src/cfg.txt"

#define BLACK   "\x1B[0;30m"
#define RED     "\x1B[0;31m"
#define GREEN   "\x1B[0;32m"
#define YELLOW  "\x1B[0;33m"
#define BLUE    "\x1B[0;34m"
#define PURPLE  "\x1B[0;35m"
#define CYAN    "\x1B[0;36m"
#define WHITE   "\x1B[0;37m"
#define DEFAULT "\x1B[0m"

#define DATABLOCK_LEN   14

typedef struct bufferCirc
{
    float * data;
    uint32_t inIdx;
}dataBuffer;

typedef struct {
	int16_t accel_outx;
	int16_t accel_outy;
	int16_t accel_outz;
	int16_t temp;
	int16_t gyro_outx;
	int16_t gyro_outy;
	int16_t gyro_outz;
}MPU6050_data_raw;

typedef struct {
	float accelX;
	float accelY;
	float accelZ;
	float gyroX;
	float gyroY;
	float gyroZ;
	float temp;
}MPU60250_var;

typedef struct {
	float r_accel_outx;
	float r_accel_outy;
	float r_accel_outz;
	float r_gyro_outx;
	float r_gyro_outy;
	float r_gyro_outz;
	float r_temp;

	float SMA_accel_outx;
	float SMA_accel_outy;
	float SMA_accel_outz;
	float SMA_gyro_outx;
	float SMA_gyro_outy;
	float SMA_gyro_outz;
	float SMA_temp;

	float EWMA_accel_outx;
	float EWMA_accel_outy;
	float EWMA_accel_outz;
	float EWMA_gyro_outx;
	float EWMA_gyro_outy;
	float EWMA_gyro_outz;
	float EWMA_temp;
}MPU6050_data_procces;

typedef struct {
	uint32_t orderSMA;
	float alphaEWMA;
}configProd;

void cargarSharedMem(dataBuffer * shMem, sem_t* semId, const configProd* config);
void proccesDataMPU(uint8_t *buffer_raw, MPU6050_data_procces *data_out, ssize_t cant );
void configureSignals(void);
int  leerConfiguracion(const char *cfgPath, configProd* config);
int  recargarConfiguracion(const char *cfgPath, configProd* config);
void SMAFilter(MPU6050_data_procces *data, uint32_t order);
void EWMAFilter(MPU6050_data_procces *data, float alpha);

#endif