#include "sensor.h"


uint8_t producerOn;
uint8_t newCfg;
extern MPU60250_var* SMAVector;

int main (void)
{
    uint32_t shmId;
    sem_t *semId;
    key_t msgKey;
    dataBuffer shDat;
    configProd cfgSensor;

    printf(YELLOW "[Sensor] - Hola soy el programa sensor, voy a cargar la SHMEM \n" DEFAULT);
    printf(YELLOW "[Sensor] - Mi pid es: %d \n" DEFAULT, getpid());
    
    configureSignals();

    leerConfiguracion(CFG_PATH, &cfgSensor);

    producerOn = 1;

    msgKey = ftok(KEY_PATH, 'B' + 'O' + 'C' + 'A');

    if( (semId = sem_open(SEM_NAME, O_CREAT, 0666, 1)) == SEM_FAILED){
        perror(RED "Error al abrir el semáforo" DEFAULT);
        exit(1);
    }
    shmId = shmget(msgKey, 4096 , 0664 ); //0664 user-group-others: rw- rw- r--
    

    shDat.data = shmat(shmId, (void*) 0, 0);
    shDat.inIdx  = 0;

    while( producerOn == 1)
    {
        cargarSharedMem(&shDat, semId, &cfgSensor);

        if(newCfg == 1)
        {
            printf(YELLOW"[Sensor] - Se aplicará la nueva configuración\n"DEFAULT);
            recargarConfiguracion(CFG_PATH, &cfgSensor);
            newCfg = 0;
        }

        //sleep(1);
        usleep(500000);
    }

    printf(YELLOW"[Sensor] - Terminando programa productor\n"DEFAULT);

    if (shmdt(shDat.data) == -1) {
        perror("shmdt");
    }

    sem_close (semId);
    free(SMAVector);
    return 0;
}
