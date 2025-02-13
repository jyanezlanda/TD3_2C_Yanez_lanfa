#include "../inc/server.h"

extern uint8_t server_working, closeSv, newCfg;
extern uint32_t fPid;
extern serverCfg svCfg;
extern connect_ctx_t *ctx;

void* connection_received(void* arg)
{
    connect_ctx_t* ctxTh = (connect_ctx_t*)arg;
    uint8_t thId = ctxTh->thread->id;
    char buffer[BUFFER_SIZE];
    int client_socket = ctxTh->soc_addr;
    float* shared_data = ctxTh->shData;
    sem_t* sem_id = ctxTh->semID;
    int active = 1;
    ssize_t bytes_sent, bytes_received;                
    char data_to_send[BUFFER_SIZE];


    printf(BLUE"[Server] - Nueva conexión atendida en socket: %d\n"DEFAULT, client_socket);

    // Protocolo inicial: enviar un mensaje de OK al cliente
    const char* init_message = "OK";
    if (send(client_socket, init_message, strlen(init_message), 0) < 0) {
        perror(RED"[Server] - Error al enviar mensaje de inicio" DEFAULT);
        close(client_socket);
        svCfg.threads[thId].act = 0;
        return NULL;
    }
    
    bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0);
    if (bytes_received <= 0) {
        printf(RED"[Server] - Cliente desconectado o error en socket: %d\n"DEFAULT, client_socket);
        close(client_socket);
        svCfg.threads[thId].act = 0;
        return NULL;
    }

    buffer[bytes_received] = '\0'; // Null-terminate del mensaje recibido
    if (strncmp(buffer, "AKN", 3) != 0)
    {
        printf(BLUE"[Server] - Comando desconocido recibido: %s\n"DEFAULT, buffer);
        printf(BLUE"[Server] - No se pudo establecer la conexión, cerrando conexión\n"DEFAULT);
        close(client_socket);
        svCfg.threads[thId].act = 0;
        return NULL;
    }

    if (send(client_socket, init_message, strlen(init_message), 0) < 0) {
        perror("[Server] - Error al enviar mensaje de inicio");
        close(client_socket);
        svCfg.threads[thId].act = 0;
        return NULL;
    }
     printf(BLUE"[Server] - Comenzando Transmision: %s\n"DEFAULT, buffer);
    while (active) {
        // Esperar mensaje del cliente
        bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0);
        if (bytes_received <= 0) {
            printf(RED"[Server] - Cliente desconectado o error en socket: %d\n"DEFAULT, client_socket);
            active = 0;
            break;
        }

        buffer[bytes_received] = '\0'; // Null-terminate del mensaje recibido
       

        if (strncmp(buffer, "KA", 2) == 0) {
            // El cliente envía un Keep-Alive (KA)
            sem_wait(sem_id); // Esperar acceso a la memoria compartida


            snprintf(data_to_send, BUFFER_SIZE,
                     "%f\n%f\n%f\n%f\n%f\n%f\n%f\n%f\n%f\n%f\n%f\n%f\n%f\n%f\n%f\n%f\n%f\n%f\n%f\n%f\n%f\n",
                     shared_data[0], shared_data[1], shared_data[2], shared_data[3],
                     shared_data[4], shared_data[5], shared_data[6], shared_data[7],
                     shared_data[8], shared_data[9], shared_data[10], shared_data[11],
                     shared_data[12], shared_data[13], shared_data[14], shared_data[15],
                     shared_data[16], shared_data[17], shared_data[18], shared_data[19], shared_data[20]);

            sem_post(sem_id); // Liberar el acceso a la memoria compartida

            
            bytes_sent = send(client_socket, data_to_send, strlen(data_to_send), 0);
            if (bytes_sent < 0) {
                perror(BLUE"[Server] - Error al enviar datos"DEFAULT);
                active = 0;
                break;
            }
        } else if (strncmp(buffer, "END", 3) == 0) {
            // El cliente solicita terminar la conexión
            printf(GREEN"[Server] - Cliente solicitó desconexión\n"DEFAULT);
            active = 0;
        } else {
            printf(BLUE"[Server] - Comando desconocido recibido: %s\n"DEFAULT, buffer);
        }
    }

    // Finalizar la conexión
    close(client_socket);
    svCfg.threads[thId].act = 0;
    printf(BLUE"[Server] - Conexión cerrada para socket: %d\n"DEFAULT, client_socket);

    pthread_exit(NULL);
}

void* cfg_check(void* arg)
{
    while(server_working == 1)
    {
        if(newCfg == 1)
        {
            aplicarReconfiguracion();
            newCfg = 0;
        }
        sleep(2);
    }
    pthread_exit(NULL);
}

void sigintHandler(int signum){
    if(closeSv == 0)
    {
        printf(BLUE "\n[Server] -Se recibió la SIGIN por primera vez, cerrando al programa productor \n"DEFAULT);
        if (kill(fPid, SIGINT) == -1) {
            printf(BLUE"[Server] - Error al enviar la señal, se cierra el servidor\n"DEFAULT);
            server_working = 0;
        }
        closeSv = 1;
    } else
    {    
        server_working = 0;
    }    
    
}

void sigchldHandler(int signum){
    int pid;
    pid = waitpid(ANYCHILD, NULL, WNOHANG);
    printf(BLUE"Termino el hijo con pid: %d \n"DEFAULT, pid);
}

void sigusr1Handler(int signum){
    if (kill(fPid, SIGUSR1) == -1) 
    {
        printf(BLUE"[Server] - Error al enviar la señal, no se reconfigura el productor\n"DEFAULT);
    }
    newCfg = 1;
}

void configureSignals(void){
    struct sigaction sigint, sigchld, sigusr1;

    sigint.sa_handler = sigintHandler;
    sigint.sa_flags = 0;  // Do not use SA_RESTART
    sigemptyset(&sigint.sa_mask);
    sigaction(SIGINT, &sigint, NULL);

    sigchld.sa_handler = sigchldHandler;
    sigchld.sa_flags = SA_NOCLDSTOP | SA_RESTART;  // Do not use SA_RESTART
    sigemptyset(&sigchld.sa_mask);
    sigaction(SIGCHLD, &sigchld, NULL);

    sigusr1.sa_handler = sigusr1Handler;
    sigusr1.sa_flags = SA_NOCLDSTOP | SA_RESTART;  // Do not use SA_RESTART
    sigemptyset(&sigusr1.sa_mask);
    sigaction(SIGUSR1, &sigusr1, NULL);

    return;
}

void leerConfiguracion(const char *nombreArchivo){
    FILE *archivo = fopen(nombreArchivo, "r");
    if (archivo == NULL) {
        perror(RED "[Server] - No se pudo abrir el archivo, se aplican configuración por defecto \n" DEFAULT);
        svCfg.puerto = 8080;
        svCfg.max_conn = 100;
        svCfg.backlog = 10;

        return;
    }

    char linea[100];
    while (fgets(linea, sizeof(linea), archivo)) {
        char clave[50];
        int valor;

        if (sscanf(linea, "%[^:]: %d", clave, &valor) == 2) {
            if (strcmp(clave, "MAX_CONN") == 0) {
                svCfg.max_conn = valor;
            } else if (strcmp(clave, "PUERTO") == 0) {
                svCfg.puerto = valor;
            } else if (strcmp(clave, "BACKLOG") == 0) {
                svCfg.backlog = valor;
            }
        }
    }

    fclose(archivo);
}

void aplicarReconfiguracion(void ) {
    uint32_t oldMaxConn, reconfigure = 1, i;

    oldMaxConn = svCfg.max_conn;

    leerConfiguracion(CFG_PATH);

    if(oldMaxConn > svCfg.max_conn){
        reconfigure = 0;
        printf(RED"La nueva cantidad de conexiones es inferior a la actual, no se cerrará ninguna conexión activa.\n"DEFAULT);
    }   

    printf(BLUE"Configuración cargada:\n"DEFAULT);
    printf(BLUE"MAX_CONN: %d\n"DEFAULT, svCfg.max_conn);
    printf(BLUE"PUERTO: %d\n"DEFAULT, svCfg.puerto);
    printf(BLUE"BACKLOG: %d\n"DEFAULT, svCfg.backlog);
    printf(GREEN"DISCLAIMER: SOLO SE APLICARÁ EL CAMBIO EN MAX CONNECTION, PARA EL RESTO REINICIE EL SERVIDOR\n"DEFAULT);

    printf("\n---------------------------------------\n");
    printf(BLUE"[Server]-$ Actualizando la máxima cantidad de conexiones ... \n"DEFAULT);

    if(reconfigure == 1)
    {
        if(svCfg.max_conn != oldMaxConn)
        {
            svCfg.threads = realloc(svCfg.threads, sizeof(sv_thread_t)*svCfg.max_conn);
            for(i = oldMaxConn; i < svCfg.max_conn; i++ ){
                svCfg.threads[i].act = 0;                                // Initialize the clients off
            }
            ctx = realloc(ctx, sizeof(connect_ctx_t)*svCfg.max_conn);
        }
    }    
    printf(GREEN"Maximas conexiones actualizadas a %d\n"DEFAULT, svCfg.max_conn);
    printf("---------------------------------------\n\n");
    
}