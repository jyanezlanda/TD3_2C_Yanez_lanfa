#include "../inc/test.h"

int  main (void)
{
	int fd, i=0, cant = 1;
	uint8_t dato_a_leer[14 * cant];
	struct ModuleData module_data_raw;
	struct ModuleData_Float md_float;
	int len=0;

	printf("Mi Pid es: %d\n", getpid());
	//printf("[Prueba]-$ Estoy por abrir el driver\n");
	
	fd = open("/dev/td3_jyl", O_RDWR);
	if (fd < 0)
	{
		printf("[Prueba]-$ No se pudo abrir el driver pa\n");
		return -1;
	}
	//printf("[Prueba]-$ Open Operation Finished\n");

	//sleep(1);
	//printf("[Prueba]-$ Starting Read Operation\n");
	len = read(fd, dato_a_leer, cant);

	//printf("[Prueba]-$ Cantidad de datos a leer: %d.\n", len);
	if(len < 0)
	{
		printf("[Prueba]-$ Error al leer.\n");
	 	close (fd);
	 	return 0;
	}

	module_data_raw.accel_outx = 0;
	module_data_raw.accel_outy = 0;
	module_data_raw.accel_outz = 0;
	module_data_raw.temp 	   = 0;
	module_data_raw.gyro_outx  = 0;
	module_data_raw.gyro_outy  = 0;
	module_data_raw.gyro_outz  = 0;

	for(i = 0; i < 14; i++)
		printf("[Prueba]-$ dato_a_leer[%d] = 0x%X \n", i, dato_a_leer[i]);
	for(int i = 0; i< cant; i++)
	{
		module_data_raw.accel_outx += (((dato_a_leer[14*i + 0] << 8) | dato_a_leer[14*i + 1]))/cant;
		module_data_raw.accel_outy += (((dato_a_leer[14*i + 2] << 8) | dato_a_leer[14*i + 3]))/cant;
		module_data_raw.accel_outz += (((dato_a_leer[14*i + 4] << 8) | dato_a_leer[14*i + 5]))/cant;
		module_data_raw.temp 	   += (((dato_a_leer[14*i + 6] << 8) | dato_a_leer[14*i + 7]))/cant;
		module_data_raw.gyro_outx  += (((dato_a_leer[14*i + 8] << 8) | dato_a_leer[14*i + 9]))/cant;
		module_data_raw.gyro_outy  += (((dato_a_leer[14*i + 10] << 8) | dato_a_leer[14*i + 11]))/cant;
		module_data_raw.gyro_outz  += (((dato_a_leer[14*i + 12] << 8) | dato_a_leer[14*i + 13]))/cant;
	}
	/*printf("raw_accel_outx=%d\nraw_accel_outy=%d\nraw_accel_outz=%d\nraw_temp=%d\nraw_gyro_outx=%d\nraw_gyro_outy=%d\nraw_gyro_outz=%d\n",
	module_data_raw.accel_outx,module_data_raw.accel_outy,module_data_raw.accel_outz,module_data_raw.temp,module_data_raw.gyro_outx,module_data_raw.gyro_outy,module_data_raw.gyro_outz);
*/
	md_float.accel_outx = module_data_raw.accel_outx / 16384.0;
	md_float.accel_outy = module_data_raw.accel_outy / 16384.0;
	md_float.accel_outz = module_data_raw.accel_outz / 16384.0;
	md_float.gyro_outx  = module_data_raw.gyro_outx / 32.8;
	md_float.gyro_outy  = module_data_raw.gyro_outy / 32.8;
	md_float.gyro_outz  = module_data_raw.gyro_outz / 32.8;
	md_float.temp 		= module_data_raw.temp/340.0 + 36.53;
	printf("accel_outx=%f\naccel_outy=%f\naccel_outz=%f\ntemp=%f\ngyro_outx=%f\ngyro_outy=%f\ngyro_outz=%f\n",
	md_float.accel_outx,md_float.accel_outy,md_float.accel_outz,md_float.temp,md_float.gyro_outx,md_float.gyro_outy,md_float.gyro_outz);
	
	close(fd);
	printf("[tester]-$ Close Operation Finished\n");

	return 0;
}
