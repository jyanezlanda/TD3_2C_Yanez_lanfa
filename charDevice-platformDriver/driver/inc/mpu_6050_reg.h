#ifndef __MPU6050_H
#define __MPU6050_H
#include <linux/types.h>

typedef struct{
	uint8_t ACCELX_H;
	uint8_t ACCELX_L;
	uint8_t ACCELY_H;
	uint8_t ACCELY_L;
	uint8_t ACCELZ_H;
	uint8_t ACCELZ_L;
	uint8_t TEMP_H;
	uint8_t TEMP_L;
	uint8_t GYROX_H;
	uint8_t GYROX_L;
	uint8_t GYROY_H;
	uint8_t GYROY_L;
	uint8_t GYROZ_H;
	uint8_t GYROZ_L;
}dataframe;

#define USER_CTRL     0x6A
#define PWR_MGMT_1    0x6B
#define WHO_AM_I      0x75
#define CONFIG        0x1A
#define GYRO_CONFIG   0x1B
#define ACCEL_CONFIG  0x1C
#define FIFO_EN       0x23
#define INT_ENABLE    0x38
#define INT_STATUS    0x3A
#define ACCEL_XOUT_H  0x3B
#define ACCEL_XOUT_L  0x3C
#define ACCEL_YOUT_H  0x3D
#define ACCEL_YOUT_L  0x3E
#define ACCEL_ZOUT_H  0x3F
#define ACCEL_ZOUT_L  0x40
#define TEMP_OUT_H    0x41
#define TEMP_OUT_L    0x42
#define GYRO_XOUT_H   0x43
#define GYRO_XOUT_L   0x44
#define GYRO_YOUT_H   0x45
#define GYRO_YOUT_L   0x46
#define GYRO_ZOUT_H   0x47
#define GYRO_ZOUT_L   0x48

#define DATAFRAME_LEN 14
#define FIFO_LEN	  1024	

uint32_t init_mpu6050(void);
uint32_t leerByteSensor(uint32_t reg2Write);
uint32_t escribirByteSensor(uint32_t reg2Write, uint32_t data);
uint32_t writeSensor(uint8_t * dataTx, uint32_t lenTx);
uint32_t readSensor ( uint32_t registro, uint8_t * dataRx, ssize_t lenRx );

#endif  /*__MPU6050_H*/