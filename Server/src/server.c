#include "server.h"
//#include "../../Sensor/inc/sensor.h"

uint8_t server_working, closeSv, newCfg;
uint32_t fPid;

serverCfg   svCfg;
connect_ctx_t *ctx;

int main (void){

    configureSignals();

    fPid = fork();

    switch (fPid)
    {
    case -1:
        perror("Fork");
        exit(1);
        break;
    
    case 0: //Proceso hijO
        execlp(SENSOR_PATH, SENSOR_PATH, NULL);
        break;

    default: //Proceso padre
        mainServer();
        break;
    }
    
    return 0;
}

void mainServer (void)
{
    unsigned int s, conn_s, i = 0, j=0,  shmId;

    sem_t* semId;

    float * sharedData;

    key_t msgKey;


    struct sockaddr_in server, conn_addr;

    socklen_t sin_size;

    pthread_t threadCfg;

    newCfg = 0;
    server_working = 1;

    leerConfiguracion(CFG_PATH);

    svCfg.threads = malloc(svCfg.max_conn * sizeof(sv_thread_t));
    ctx = malloc(svCfg.max_conn * sizeof(connect_ctx_t));

    printf(BLUE"·-----------------------------------------------------------------------·\n"DEFAULT);
    printf(BLUE"|                                                                       |\n"DEFAULT);
    printf(BLUE"|                            TD3 - Server                               |\n"DEFAULT);
    printf(BLUE"|                  Autor T.P.: Yañez Landa joaquin                      |\n"DEFAULT);
    printf(BLUE"|                             UTN - FRBA                                |\n"DEFAULT);
    printf(BLUE"|                             2ºC - 2024                                |\n"DEFAULT);
    printf(BLUE"|                            Max Conn:  %d                               |\n"DEFAULT, svCfg.max_conn);
    printf(BLUE"|                            Puerto:  %d                              |\n"DEFAULT, svCfg.puerto);
    printf(BLUE"|                             Backlog:  %d                              |\n"DEFAULT, svCfg.backlog);
    printf(BLUE"|                                                                       |\n"DEFAULT);
    printf(BLUE"·-----------------------------------------------------------------------·\n"DEFAULT);

    closeSv = 0;
    msgKey = ftok(KEY_PATH, 'B' + 'O' + 'C' + 'A');

    if (msgKey == -1)
    {
        perror(RED "fTok :(" DEFAULT);
        exit(1);
    }

    shmId = shmget(msgKey, 4096 , 0664 | IPC_CREAT); //0664 user-group-others: rw- rw- r--
    if (shmId == -1)
    {
        perror(RED "[SERVER]- ERROR on ShmGet"DEFAULT);
        exit(1);
    }

    semId = sem_open(SEM_NAME, O_CREAT, 0666, 1);
    sharedData = shmat(shmId, (void*) 0, SHM_RDONLY);
    if (sharedData == (void *) -1)
    {
        perror("shMat");
        exit(1);
    }

    for (i = 0; i < svCfg.max_conn; i++)
    {
        svCfg.threads[i].act = 0;
        svCfg.threads[i].id = i;
    }

    server.sin_family = AF_INET;
    server.sin_port = htons(svCfg.puerto);
    server.sin_addr.s_addr = INADDR_ANY;
    memset(&(server.sin_zero), '\0', 8);

    if(pthread_create( &threadCfg, NULL, cfg_check, NULL ) != 0)
    {
        perror("Error creando el thread de configuración");
    }

    if( (s = socket(AF_INET, SOCK_STREAM, 0 )) < 0)
    {
        perror(RED "socket" DEFAULT);
        exit(1);
    }

    // Permitir reutilización del puerto
    int opt = 1;
    if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt(SO_REUSEADDR) failed");
        close(s);
        exit(1);
    }

    if( bind(s, (struct sockaddr *) &server, sizeof(struct sockaddr)) )
    {
        perror("bind");
        exit(1);
    }
    
    if (listen(s, svCfg.backlog) == -1)
    {
        perror("listen");
        exit(1);
    }

    sin_size = sizeof(struct sockaddr_in);

    printf(BLUE"[Server] - El pid del proceso principal es: %d \n", getpid());
    printf(BLUE"[Server] - El pid del proceso hijo es: %d \n", fPid);
        

    while(server_working == 1)
    {
        if ( (conn_s = accept(s, (struct sockaddr *)&conn_addr, &sin_size)) != -1)
        {  
            j = -1;
            for ( i = 0; i < svCfg.max_conn; i++)
            {
                if( svCfg.threads[i].act == 0 )
                {
                    j = i;
                    i = svCfg.max_conn;
                }
            }
            
            if( j != -1 )
            {
                printf(GREEN"[Server] - Conexión recibida %s: %d \n" DEFAULT, inet_ntoa(conn_addr.sin_addr), ntohs(conn_addr.sin_port));

                svCfg.threads[j].act = 1;

                ctx[j].soc_addr = conn_s;
                ctx[j].thread = &svCfg.threads[j];
                ctx[j].shData = sharedData;
                ctx[j].semID = semId;
                
                if(pthread_create( &(svCfg.threads[j].thread_id), NULL, connection_received, (void*) &(ctx[j]) ) != 0)
                {
                    perror("Error creando el thread");
                }
            }
            else{
                printf(BLUE "\nActualmente todos nuestros operadore/s se encuentran ocupados, cerrando la conexión\n" DEFAULT);

                const char* init_message = "Actualmente todos nuestros operadore/s se encuentran ocupados, cerrando la conexión\n";
                if (send(conn_s, init_message, strlen(init_message), 0) < 0) {
                    perror(RED"[Server] - Error al enviar mensaje de inicio" DEFAULT);
                   
                }
                close(conn_s);
            }   
        }      
    }
    printf(BLUE "\nEl servidor se esta cerrando\n" DEFAULT);
    for ( i = 0; i < svCfg.max_conn; i++)
    {
        if(svCfg.threads[i].act == 1)
        {
            close(ctx[i].soc_addr);
            pthread_cancel(svCfg.threads[i].thread_id);
            
        }
    }

    close (s);
    
    sem_close(semId);
    
    if (sem_unlink(SEM_NAME) == -1) {
    perror("sem_unlink");
    }
    
    if (shmdt(sharedData) == -1) {
        perror("shmdt");
    }

    if (shmctl(shmId, IPC_RMID, NULL) == -1) {
        perror("shmctl");
    }

    free(svCfg.threads);
    free(ctx);
}