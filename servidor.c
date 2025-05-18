#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PORT 8080
#define MAX_BUFFER 1024

// Estructura para almacenar datos del usuario
typedef struct {
    char nombre[50];
    char matricula[20];
    char carrera[50];
    int edad;
    char genero;
    int semestre;
    char password[50];
} Usuario;

// Función para manejar la conexión con el cliente
void *manejar_cliente(void *socket_desc) {
    int sock = *(int*)socket_desc;
    char buffer[MAX_BUFFER] = {0};
    Usuario usuario;
    
    // Recibir datos del usuario
    recv(sock, &usuario, sizeof(Usuario), 0);
    
    // Guardar en archivo
    FILE *f = fopen("registros.txt", "a");
    if (f != NULL) {
        fprintf(f, "%s,%s,%s,%d,%c,%d,%s\n", 
                usuario.nombre, 
                usuario.matricula,
                usuario.carrera,
                usuario.edad,
                usuario.genero,
                usuario.semestre,
                usuario.password);
        fclose(f);
    }
    
    // Enviar confirmación
    char *mensaje = "Registro exitoso";
    send(sock, mensaje, strlen(mensaje), 0);
    
    free(socket_desc);
    close(sock);
    return 0;
}

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    
    // Crear socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Error al crear socket");
        exit(EXIT_FAILURE);
    }
    
    // Configurar socket
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    
    // Vincular socket
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Error en bind");
        exit(EXIT_FAILURE);
    }
    
    // Escuchar conexiones
    if (listen(server_fd, 3) < 0) {
        perror("Error en listen");
        exit(EXIT_FAILURE);
    }
    
    printf("\033[1;34mServidor iniciado. Esperando conexiones...\033[0m\n");
    
    while(1) {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            perror("Error en accept");
            exit(EXIT_FAILURE);
        }
        
        int *new_sock = malloc(sizeof(int));
        *new_sock = new_socket;
        
        pthread_t thread_id;
        if(pthread_create(&thread_id, NULL, manejar_cliente, (void*)new_sock) < 0) {
            perror("Error al crear thread");
            return 1;
        }
        pthread_detach(thread_id);
    }
    
    return 0;
}