#ifndef SERVER_H
#define SERVER_H

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <pthread.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <unistd.h>
#include <semaphore.h>
#include <fcntl.h>

#define PUERTO          8080
#define MAX_CONN        2
#define SOC_MAX_CONN    4096
#define ANYCHILD        -1

#define BUFFER_SIZE 2048

#define KEY_PATH        "/home/debian/TD3_2Cuat/Server/src/cfg.txt"
#define SENSOR_PATH     "/home/debian/TD3_2Cuat/Sensor/bin/sensor"
#define CFG_PATH        "/home/debian/TD3_2Cuat/Server/src/cfg.txt"
#define SEM_NAME        "BOCAAA"

#define BLACK   "\x1B[0;30m"
#define RED     "\x1B[0;31m"
#define GREEN   "\x1B[0;32m"
#define YELLOW  "\x1B[0;33m"
#define BLUE    "\x1B[0;34m"
#define PURPLE  "\x1B[0;35m"
#define CYAN    "\x1B[0;36m"
#define WHITE   "\x1B[0;37m"
#define DEFAULT "\x1B[0m"

typedef struct sv_thread
{
    pthread_t thread_id;
    uint8_t act;
    uint8_t id;
}sv_thread_t;


typedef struct connect_ctx
{
    sv_thread_t* thread;
    uint32_t soc_addr;
    float * shData;
    sem_t* semID;
}connect_ctx_t;

typedef struct bufferCirc
{
    float * data;
    uint32_t inIdx;
}dataBuffer;

typedef struct 
{
    uint32_t max_conn;
    uint32_t puerto;
    uint32_t backlog;
    sv_thread_t *threads;
}serverCfg;

void* connection_received(void* ctx);
void* cfg_check(void* ctx);

void configureSignals(void);
void leerConfiguracion(const char *nombreArchivo);
void aplicarReconfiguracion(void);
void mainServer (void);

#endif