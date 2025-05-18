#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 8080
#define MAX_BUFFER 1024
#define SERVER_IP "127.0.0.1"

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

// Función para limpiar la pantalla
void limpiar_pantalla() {
    system("clear");
}

// Función para mostrar el menú principal
void mostrar_menu() {
    printf("\033[1;34m╔══════════════════════════╗\n");
    printf("║    SISTEMA DE EXAMEN    ║\n");
    printf("╚══════════════════════════╝\033[0m\n\n");
    printf("1. Registrarse\n");
    printf("2. Iniciar Test Psicométrico\n");
    printf("3. Realizar Examen Académico\n");
    printf("4. Ver Cardex\n");
    printf("5. Salir\n\n");
    printf("Seleccione una opción: ");
}

// Función para registrar usuario
Usuario registrar_usuario() {
    Usuario usuario;
    limpiar_pantalla();
    printf("\033[1;32m=== REGISTRO DE USUARIO ===\033[0m\n\n");
    
    printf("Nombre: ");
    scanf(" %[^\n]", usuario.nombre);
    
    printf("Matrícula: ");
    scanf(" %[^\n]", usuario.matricula);
    
    printf("Carrera: ");
    scanf(" %[^\n]", usuario.carrera);
    
    printf("Edad: ");
    scanf("%d", &usuario.edad);
    
    printf("Género (M/F): ");
    scanf(" %c", &usuario.genero);
    
    printf("Semestre: ");
    scanf("%d", &usuario.semestre);
    
    printf("Contraseña: ");
    scanf(" %[^\n]", usuario.password);
    
    return usuario;
}

int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[MAX_BUFFER] = {0};
    int opcion;
    
    // Crear socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\nError al crear el socket\n");
        return -1;
    }
    
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    
    // Convertir dirección IP
    if(inet_pton(AF_INET, SERVER_IP, &serv_addr.sin_addr) <= 0) {
        printf("\nDirección inválida o no soportada\n");
        return -1;
    }
    
    // Conectar al servidor
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\nError de conexión\n");
        return -1;
    }
    
    while(1) {
        limpiar_pantalla();
        mostrar_menu();
        scanf("%d", &opcion);
        
        switch(opcion) {
            case 1: {
                Usuario usuario = registrar_usuario();
                send(sock, &usuario, sizeof(Usuario), 0);
                recv(sock, buffer, MAX_BUFFER, 0);
                printf("\n%s\n", buffer);
                printf("\nPresione Enter para continuar...");
                getchar();
                getchar();
                break;
            }
            case 5:
                close(sock);
                return 0;
            default:
                printf("\nOpción no válida\n");
                sleep(1);
        }
    }
    
    return 0;
}