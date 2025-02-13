#include "../inc/sensor.h"

extern uint8_t producerOn;
extern uint8_t newCfg;

MPU60250_var* SMAVector;

void cargarSharedMem(dataBuffer * shMem, sem_t * semId, const configProd* config){
    uint8_t buffer[DATABLOCK_LEN * cant];
    MPU6050_data_procces dataProcces;
    int i2c_fd = open("/dev/td3_jyl", O_RDWR);
    if (i2c_fd < 0) {
        perror(RED "Error al abrir /dev/td3_jyl" DEFAULT);
        exit(1);
    }

    //printf(YELLOW"[Productor] - Archivo abierto crack\n"DEFAULT);

    ssize_t bytes_read = read(i2c_fd, &buffer, 1);
    if (bytes_read == -1) {
        perror(RED "Error al leer de /dev/td3_jyl" DEFAULT);
        exit(1);
    }
    
    close(i2c_fd);

    proccesDataMPU(buffer, &dataProcces, cant);

    SMAFilter(&dataProcces, config->orderSMA);
    EWMAFilter(&dataProcces, config->alphaEWMA);

    if (sem_wait(semId) == EINTR) {
        perror(RED "Error al tomar el semáforo" DEFAULT);
        exit(-1);
    }

    memcpy(shMem->data, &dataProcces, sizeof(MPU6050_data_procces)  );

    if (sem_post(semId) == EINVAL) {
        perror(RED "Error al dejar el semáforo" DEFAULT);
        exit(-1);
    }
    /*
    printf(YELLOW "[Sensor] - AccelX Raw %f\n" DEFAULT, dataProcces.r_accel_outx );
    printf(YELLOW "[Sensor] - AccelY Raw: %f\n" DEFAULT, dataProcces.r_accel_outy );
    printf(YELLOW "[Sensor] - AccelZ Raw: %f\n" DEFAULT, dataProcces.r_accel_outz );
    printf(YELLOW "[Sensor] - Temp Raw: %f\n" DEFAULT, dataProcces.r_temp );
    printf(YELLOW "[Sensor] - Temp SMA: %f\n" DEFAULT, dataProcces.SMA_temp );
    printf(YELLOW "[Sensor] - AccelX SMA: %f\n" DEFAULT, dataProcces.SMA_accel_outx );
    printf(YELLOW "[Sensor] - AccelY SMA: %f\n" DEFAULT, dataProcces.SMA_accel_outy );
    printf(YELLOW "[Sensor] - AccelZ SMA: %f\n" DEFAULT, dataProcces.SMA_accel_outz );
    printf(YELLOW "[Sensor] - AccelX EWMA: %f\n" DEFAULT, dataProcces.EWMA_accel_outx );
    printf(YELLOW "[Sensor] - AccelY EWMA: %f\n" DEFAULT, dataProcces.EWMA_accel_outy );
    printf(YELLOW "[Sensor] - AccelZ EWMA: %f\n" DEFAULT, dataProcces.EWMA_accel_outz );*/

    return;
}

void proccesDataMPU(uint8_t *buffer_raw, MPU6050_data_procces *data_out, ssize_t cant )
{
    int i = 0;
    MPU6050_data_raw dataRaw;

    dataRaw.accel_outx = 0;
    dataRaw.accel_outy = 0;
    dataRaw.accel_outz = 0;
    dataRaw.temp 	   = 0;
    dataRaw.gyro_outx  = 0;
    dataRaw.gyro_outy  = 0;
    dataRaw.gyro_outz  = 0;
    
    for(i = 0; i < cant; i++) {
        dataRaw.accel_outx  += ((buffer_raw[i * DATABLOCK_LEN + 0] << 8) | buffer_raw[i * DATABLOCK_LEN + 1]);
        dataRaw.accel_outy  += ((buffer_raw[i * DATABLOCK_LEN + 2] << 8) | buffer_raw[i * DATABLOCK_LEN + 3]);
        dataRaw.accel_outz  += ((buffer_raw[i * DATABLOCK_LEN + 4] << 8) | buffer_raw[i * DATABLOCK_LEN + 5]);
        dataRaw.temp        += ((buffer_raw[i * DATABLOCK_LEN + 6] << 8) | buffer_raw[i * DATABLOCK_LEN + 7]);
        dataRaw.gyro_outx   += ((buffer_raw[i * DATABLOCK_LEN + 8] << 8) | buffer_raw[i * DATABLOCK_LEN + 9]);
        dataRaw.gyro_outy   += ((buffer_raw[i * DATABLOCK_LEN + 10] << 8) | buffer_raw[i * DATABLOCK_LEN + 11]);
        dataRaw.gyro_outz   += ((buffer_raw[i * DATABLOCK_LEN + 12] << 8) | buffer_raw[i * DATABLOCK_LEN + 13]);
    }   

    dataRaw.accel_outx /= cant;
    dataRaw.accel_outy /= cant;
    dataRaw.accel_outz /= cant;
    dataRaw.temp        /= cant;
    dataRaw.gyro_outx   /= cant;
    dataRaw.gyro_outy   /= cant;
    dataRaw.gyro_outz   /= cant;

    /*printf("[Productor] - Ax=%d\n", dataRaw.accel_outx);
    printf("[Productor] - Ay=%d\n", dataRaw.accel_outy);
    printf("[Productor] - Az=%d\n", dataRaw.accel_outz);
    printf("[Productor] - Gx=%d\n", dataRaw.gyro_outx);
    printf("[Productor] - Gy=%d\n", dataRaw.gyro_outy);
    printf("[Productor] - Gz=%d\n", dataRaw.gyro_outz);
    printf("[Productor] - Te=%d\n", dataRaw.temp);*/

	data_out->r_accel_outx = dataRaw.accel_outx/16384.0;
	data_out->r_accel_outy = dataRaw.accel_outy/16384.0;
	data_out->r_accel_outz = dataRaw.accel_outz/16384.0;
	data_out->r_gyro_outx  = dataRaw.gyro_outx/32.8;
	data_out->r_gyro_outy  = dataRaw.gyro_outy/32.8;
	data_out->r_gyro_outz  = dataRaw.gyro_outz/32.8;
	data_out->r_temp  	   = dataRaw.temp/340 + 30.53;

    return;
}

void SMAFilter(MPU6050_data_procces *data, uint32_t order)
{
    int i;

    data->SMA_accel_outx = 0;
    data->SMA_accel_outy = 0;
    data->SMA_accel_outz = 0;
    data->SMA_gyro_outx = 0;
    data->SMA_gyro_outy = 0;
    data->SMA_gyro_outz = 0;
    data->SMA_temp = 0;

    for(i = order-1; i  > 0; i--)
    {
        SMAVector[i].accelX = SMAVector[i-1].accelX;
        SMAVector[i].accelY = SMAVector[i-1].accelY;
        SMAVector[i].accelZ = SMAVector[i-1].accelZ;
        SMAVector[i].gyroX  = SMAVector[i-1].gyroX;
        SMAVector[i].gyroY  = SMAVector[i-1].gyroY;
        SMAVector[i].gyroZ  = SMAVector[i-1].gyroZ;
        SMAVector[i].temp   = SMAVector[i-1].temp;
    }

    SMAVector[0].accelX = data->r_accel_outx;
    SMAVector[0].accelY = data->r_accel_outy;
    SMAVector[0].accelZ = data->r_accel_outz;
    SMAVector[0].gyroX  = data->r_gyro_outx;
    SMAVector[0].gyroY  = data->r_gyro_outy;
    SMAVector[0].gyroZ  = data->r_gyro_outz;
    SMAVector[0].temp   = data->r_temp;

    for(i = 0; i < order; i++)
    {
        data->SMA_accel_outx += SMAVector[i].accelX;
        data->SMA_accel_outy += SMAVector[i].accelY;
        data->SMA_accel_outz += SMAVector[i].accelZ;
        data->SMA_gyro_outx += SMAVector[i].gyroX;
        data->SMA_gyro_outy += SMAVector[i].gyroY;
        data->SMA_gyro_outz += SMAVector[i].gyroZ;
        data->SMA_temp += SMAVector[i].temp;
    }

    data->SMA_accel_outx  /= order;
    data->SMA_accel_outy  /= order;
    data->SMA_accel_outz  /= order;
    data->SMA_gyro_outx  /= order;
    data->SMA_gyro_outy  /= order;
    data->SMA_gyro_outz  /= order;
    data->SMA_temp  /= order;
    
    return;
}

void EWMAFilter(MPU6050_data_procces *data, float alpha)
{
    static float accel_outx_prev = 0;
    static float accel_outy_prev = 0;
    static float accel_outz_prev = 0;
    static float gyro_outx_prev = 0;
    static float gyro_outy_prev = 0;
    static float gyro_outz_prev = 0;
    static float temp_prev = 0;

    data->EWMA_accel_outx = data->r_accel_outx * alpha + (1 - alpha) * accel_outx_prev;
    data->EWMA_accel_outy = data->r_accel_outy * alpha + (1 - alpha) * accel_outy_prev;
    data->EWMA_accel_outz = data->r_accel_outz * alpha + (1 - alpha) * accel_outz_prev;
    data->EWMA_gyro_outx  = data->r_gyro_outx * alpha + (1 - alpha) * gyro_outx_prev;
    data->EWMA_gyro_outy  = data->r_gyro_outy * alpha + (1 - alpha) * gyro_outy_prev;
    data->EWMA_gyro_outz  = data->r_gyro_outz * alpha + (1 - alpha) * gyro_outz_prev;
    data->EWMA_temp       = data->r_temp * alpha + (1 - alpha) * temp_prev;

    accel_outx_prev = data->EWMA_accel_outx;
    accel_outy_prev = data->EWMA_accel_outy;
    accel_outz_prev = data->EWMA_accel_outz;
    gyro_outx_prev  = data->EWMA_gyro_outx;
    gyro_outy_prev  = data->EWMA_gyro_outy;
    gyro_outz_prev  = data->EWMA_gyro_outz;
    temp_prev       = data->EWMA_temp;
    return;
}

void sigintHandler(int signum)
{
    producerOn = 0;

}

void sigusr1Handler(int signum){
    printf(YELLOW"[Sensor] - Llego la sigusr1 \n"DEFAULT);
    newCfg = 1;
}

int leerConfiguracion(const char *cfgPath, configProd* config){
    int i = 0;

    FILE *fp = fopen(cfgPath, "r");
    if (!fp) {
        printf(RED"[Sensor] - Error al abrir el archivo se aplican valores por defecto"DEFAULT);
        config->orderSMA = 5;
        config->alphaEWMA = 0.75;
        return -1;
    }

    char linea[100];
    int ordenSMA_set = 0;
    int alphaEWMA_set = 0;

    while (fgets(linea, sizeof(linea), fp)) {
        // Remover caracteres de nueva línea o espacios al final
        linea[strcspn(linea, "\r\n")] = 0;

        // Verificar si la línea contiene "ORDEN_SMA" o "ALPHA_EWMA"
        if (strncmp(linea, "ORDEN_SMA:", 10) == 0) {
            char *valor = linea + 10;
            while (*valor == ' ') valor++; // Saltar espacios
            int orden = atoi(valor);
            if (orden > 0) { // Validar que sea un número positivo
                config->orderSMA = (uint32_t)orden;
                ordenSMA_set = 1;
            } else {
                printf(RED"[Sensor] - Error: ORDEN_SMA debe ser un número positivo.\n"DEFAULT);
            }
        } else if (strncmp(linea, "ALPHA_EWMA:", 11) == 0) {
            char *valor = linea + 11;
            while (*valor == ' ') valor++; // Saltar espacios
            float alpha = atof(valor);
            if (alpha >= 0.0f && alpha <= 1.0f) { // Validar que esté en el rango [0, 1]
                config->alphaEWMA = alpha;
                alphaEWMA_set = 1;
            } else {
                printf(RED"[Sensor] - Error: ALPHA_EWMA debe estar en el rango [0, 1].\n"DEFAULT);
            }
        } else {
            printf(RED"[Sensor] - Advertencia: Línea no reconocida: '%s'\n"DEFAULT, linea);
        }
    }

    fclose(fp);

    // Verificar que se hayan establecido ambos valores
    if (!ordenSMA_set || !alphaEWMA_set) {
        printf(RED"[Sensor] - Error: El archivo de configuración está incompleto o mal formado se aplican configuraciones por defecto.\n"DEFAULT);
        
        config->orderSMA = 5;
        config->alphaEWMA = 0.75;

        return -1;
    }

    SMAVector = malloc( sizeof(MPU60250_var) * config->orderSMA);

    for (i = 0; i < config->orderSMA; i++)
    {
        SMAVector[i].accelX = 0;
        SMAVector[i].accelY = 0;
        SMAVector[i].accelZ = 0;
        SMAVector[i].gyroX = 0;
        SMAVector[i].gyroY = 0;
        SMAVector[i].gyroZ = 0;
        SMAVector[i].temp = 0;
    }

    printf(YELLOW"[Sensor] - Configuración de los filtros: \n"DEFAULT);
    printf(YELLOW"[Sensor] - Orden del filtro SMA: %d\n"DEFAULT, config->orderSMA);
    printf(YELLOW"[Sensor] - Alpha del filtro EWMA: %f \n"DEFAULT, config->alphaEWMA);
    return 0; // Éxito
}

int  recargarConfiguracion(const char *cfgPath, configProd* config){
    MPU60250_var SMAVect_aux[config->orderSMA];

    FILE *fp = fopen(cfgPath, "r");
    if (!fp) {
        printf(RED"[Sensor] - Error al abrir el archivo no se aplican nuevos valores\n"DEFAULT);
        return -1;
    }

    char linea[100];
    int ordenSMA_set = 0, ordenSMA_old;
    int alphaEWMA_set = 0;

    ordenSMA_old = config->orderSMA;

    while (fgets(linea, sizeof(linea), fp)) {
        // Remover caracteres de nueva línea o espacios al final
        linea[strcspn(linea, "\r\n")] = 0;

        // Verificar si la línea contiene "ORDEN_SMA" o "ALPHA_EWMA"
        if (strncmp(linea, "ORDEN_SMA:", 10) == 0) {
            char *valor = linea + 10;
            while (*valor == ' ') valor++; // Saltar espacios
            int orden = atoi(valor);
            if (orden > 0) { // Validar que sea un número positivo
                config->orderSMA = (uint32_t)orden;
                ordenSMA_set = 1;
            } else {
                printf(RED"[Sensor] - Error: ORDEN_SMA debe ser un número positivo.\n"DEFAULT);
            }
        } else if (strncmp(linea, "ALPHA_EWMA:", 11) == 0) {
            char *valor = linea + 11;
            while (*valor == ' ') valor++; // Saltar espacios
            float alpha = atof(valor);
            if (alpha >= 0.0f && alpha <= 1.0f) { // Validar que esté en el rango [0, 1]
                config->alphaEWMA = alpha;
                alphaEWMA_set = 1;
            } else {
                printf(RED"[Sensor] - Error: ALPHA_EWMA debe estar en el rango [0, 1].\n"DEFAULT);
            }
        } else {
            printf(RED"[Sensor] - Advertencia: Línea no reconocida: '%s'\n"DEFAULT, linea);
        }
    }

    fclose(fp);

    // Verificar que se hayan establecido ambos valores
    if (!ordenSMA_set || !alphaEWMA_set) {
        printf(RED"[Sensor] - Error: El archivo de configuración está incompleto o mal formado, no se aplican configuraciones nuevas.\n"DEFAULT);
        return -1;
    }

    if ( ordenSMA_old < config->orderSMA)
    {
        SMAVector = realloc(SMAVector, sizeof(MPU60250_var)*config->orderSMA);
    }

    if (ordenSMA_old > config->orderSMA)
    {
        memcpy(SMAVect_aux, SMAVector,  sizeof(MPU60250_var)*config->orderSMA);
        
        free(SMAVector);
        SMAVector = malloc(sizeof(MPU60250_var)*config->orderSMA);

        memcpy(SMAVector, SMAVect_aux,  sizeof(MPU60250_var)*config->orderSMA);
    }

    printf(YELLOW"[Sensor] - Nueva configuracionzrz de los filtros: \n"DEFAULT);
    printf(YELLOW"[Sensor] - Orden del filtro SMA: %d\n"DEFAULT, config->orderSMA);
    printf(YELLOW"[Sensor] - Alpha del filtro EWMA: %f \n"DEFAULT, config->alphaEWMA);

    return 0;
}

void configureSignals(void){
    struct sigaction sigint, sigusr1;

    sigint.sa_handler = sigintHandler;
    sigint.sa_flags = 0;  // Do not use SA_RESTART
    sigemptyset(&sigint.sa_mask);
    sigaction(SIGINT, &sigint, NULL);

    sigusr1.sa_handler = sigusr1Handler;
    sigusr1.sa_flags = SA_NOCLDSTOP | SA_RESTART;  // Do not use SA_RESTART
    sigemptyset(&sigusr1.sa_mask);
    sigaction(SIGUSR1, &sigusr1, NULL);

    return;
}