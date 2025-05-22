#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <ctype.h>

#define PORT 8080
#define MAX_BUFFER 1024
#define MAX_PREGUNTAS 10

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

// Estructura para preguntas
typedef struct {
    char pregunta[256];
    char opciones[3][64];
    char respuesta;
} Pregunta;

typedef struct {
    int matematicas;
    int espanol;
    int ingles;
    float promedio;
} ResultadoAcademico;

typedef struct {
    int correctas;
    int total;
    float porcentaje;
    char fecha[20];
} ResultadoPsicometrico;



// Estructura para el kardex completo
typedef struct {
    char matricula[20];
    ResultadoAcademico resultados_academicos;
    ResultadoPsicometrico resultados_psicometricos;
} Kardex;

// Prototipos de funciones
void guardar_kardex(const char* matricula, ResultadoAcademico* resultado_academico, ResultadoPsicometrico* resultado_psicometrico);

// Función para cargar preguntas desde archivo
int cargar_preguntas(const char* archivo, Pregunta* preguntas) {
    FILE* f = fopen(archivo, "rb");
    if (!f) {
        printf("Error al abrir el archivo: %s\n", archivo);
        return 0;
    }
    
    int count = 0;
    char linea[512];
    char *resultado;
    
    while (count < MAX_PREGUNTAS) {
        // Leer pregunta
        resultado = fgets(linea, sizeof(linea), f);
        if (!resultado) break;
        
        // Saltar líneas vacías
        if (strlen(linea) <= 1) continue;
        
        // Eliminar el salto de línea final si existe
        size_t len = strlen(linea);
        if (len > 0 && linea[len-1] == '\n') {
            linea[len-1] = '\0';
        }
        
        // Verificar si la pregunta está vacía
        if (strlen(linea) == 0) continue;
        
        // Copiar pregunta
        strncpy(preguntas[count].pregunta, linea, sizeof(preguntas[count].pregunta) - 1);
        preguntas[count].pregunta[sizeof(preguntas[count].pregunta) - 1] = '\0';
        
        // Leer opciones
        int opciones_validas = 0;
        for (int i = 0; i < 3; i++) {
            resultado = fgets(linea, sizeof(linea), f);
            if (!resultado) break;
            
            // Verificar formato de opción
            if (strlen(linea) > 3 && (linea[0] == 'A' || linea[0] == 'B' || linea[0] == 'C') && linea[1] == ')' && linea[2] == ' ') {
                // Eliminar el salto de línea final
                len = strlen(linea);
                if (len > 0 && linea[len-1] == '\n') {
                    linea[len-1] = '\0';
                }
                
                // Verificar si la opción está vacía
                if (strlen(linea + 3) == 0) break;
                
                // Copiar opción sin el prefijo "X) "
                strncpy(preguntas[count].opciones[i], linea + 3, sizeof(preguntas[count].opciones[i]) - 1);
                preguntas[count].opciones[i][sizeof(preguntas[count].opciones[i]) - 1] = '\0';
                opciones_validas++;
            }
        }
        
        // Verificar que se leyeron todas las opciones y que ninguna está vacía
        if (opciones_validas != 3) {
            printf("Error: Opciones incompletas o inválidas en la pregunta %d\n", count + 1);
            continue;
        }
        
        // Leer respuesta
        resultado = fgets(linea, sizeof(linea), f);
        if (!resultado) break;
        
        if (strncmp(linea, "RESPUESTA:", 10) == 0) {
            preguntas[count].respuesta = toupper(linea[10]);
            if (preguntas[count].respuesta >= 'A' && preguntas[count].respuesta <= 'C') {
                count++;
            } else {
                printf("Error: Respuesta inválida en la pregunta %d\n", count + 1);
            }
        }
    }
    
    fclose(f);
    if (count == 0) {
        printf("\033[1;31m┌─────────── ERROR DE CARGA ───────────┐\n");
        printf("│ No se pudieron cargar preguntas del   │\n");
        printf("│ archivo: %-28s │\n", archivo);
        printf("└─────────────────────────────────────┘\033[0m\n");
    } else {
        printf("\033[1;32m┌─────────── CARGA EXITOSA ───────────┐\n");
        printf("│ Se cargaron \033[1;33m%d\033[1;32m preguntas del        │\n", count);
        printf("│ archivo: %-28s │\n", archivo);
        printf("└─────────────────────────────────────┘\033[0m\n");
    }
    return count;
}

// Función para manejar la conexión con el cliente
void enviar_examen_academico(int sock, const char* matricula) {
    Pregunta preguntas_mate[MAX_PREGUNTAS];
    Pregunta preguntas_espanol[MAX_PREGUNTAS];
    Pregunta preguntas_ingles[MAX_PREGUNTAS];
    
    int num_mate = cargar_preguntas("preguntas_mate.txt", preguntas_mate);
    int num_espanol = cargar_preguntas("preguntas_espanol.txt", preguntas_espanol);
    int num_ingles = cargar_preguntas("preguntas_ingles.txt", preguntas_ingles);
    
    ResultadoAcademico resultado = {0, 0, 0, 0.0};
    
    // Enviar número total de preguntas por materia
    send(sock, &num_mate, sizeof(int), 0);
    send(sock, &num_espanol, sizeof(int), 0);
    send(sock, &num_ingles, sizeof(int), 0);
    
    // Enviar y procesar preguntas de matemáticas
    for (int i = 0; i < num_mate; i++) {
        send(sock, &preguntas_mate[i], sizeof(Pregunta), 0);
        char respuesta_usuario;
        if (recv(sock, &respuesta_usuario, 1, 0) <= 0) {
            printf("\033[1;31m┌─────────── ERROR DE COMUNICACIÓN ───────────┐\n");
            printf("│ Error al recibir respuesta de matemáticas    │\n");
            printf("└──────────────────────────────────────────────┘\033[0m\n");
            return;
        }
        char es_correcta = (respuesta_usuario == preguntas_mate[i].respuesta);
        if (es_correcta) resultado.matematicas++;
        if (send(sock, &es_correcta, 1, 0) <= 0) {
            printf("\033[1;31m┌─────────── ERROR DE COMUNICACIÓN ───────────┐\n");
            printf("│ Error al enviar resultado de matemáticas     │\n");
            printf("└──────────────────────────────────────────────┘\033[0m\n");
            return;
        }
    }
    
    // Enviar y procesar preguntas de español
    for (int i = 0; i < num_espanol; i++) {
        send(sock, &preguntas_espanol[i], sizeof(Pregunta), 0);
        char respuesta_usuario;
        if (recv(sock, &respuesta_usuario, 1, 0) <= 0) {
            printf("\033[1;31m┌─────────── ERROR DE COMUNICACIÓN ───────────┐\n");
            printf("│ Error al recibir respuesta de español        │\n");
            printf("└──────────────────────────────────────────────┘\033[0m\n");
            return;
        }
        char es_correcta = (respuesta_usuario == preguntas_espanol[i].respuesta);
        if (es_correcta) resultado.espanol++;
        if (send(sock, &es_correcta, 1, 0) <= 0) {
            printf("\033[1;31m┌─────────── ERROR DE COMUNICACIÓN ───────────┐\n");
            printf("│ Error al enviar resultado de español         │\n");
            printf("└──────────────────────────────────────────────┘\033[0m\n");
            return;
        }
    }
    
    // Enviar y procesar preguntas de inglés
    for (int i = 0; i < num_ingles; i++) {
        send(sock, &preguntas_ingles[i], sizeof(Pregunta), 0);
        char respuesta_usuario;
        if (recv(sock, &respuesta_usuario, 1, 0) <= 0) {
            printf("\033[1;31m┌─────────── ERROR DE COMUNICACIÓN ───────────┐\n");
            printf("│ Error al recibir respuesta de inglés         │\n");
            printf("└──────────────────────────────────────────────┘\033[0m\n");
            return;
        }
        char es_correcta = (respuesta_usuario == preguntas_ingles[i].respuesta);
        if (es_correcta) resultado.ingles++;
        if (send(sock, &es_correcta, 1, 0) <= 0) {
            printf("\033[1;31m┌─────────── ERROR DE COMUNICACIÓN ───────────┐\n");
            printf("│ Error al enviar resultado de inglés          │\n");
            printf("└──────────────────────────────────────────────┘\033[0m\n");
            return;
        }
    }
    
    // Calcular promedio
    resultado.promedio = (float)(resultado.matematicas + resultado.espanol + resultado.ingles) / 3.0;
    
    // Guardar resultado en el kardex
    ResultadoPsicometrico resultado_psico = {0}; // Resultado vacío para el kardex
    guardar_kardex(matricula, &resultado, &resultado_psico);
    
    // Enviar resultados al cliente
    if (send(sock, &resultado, sizeof(ResultadoAcademico), 0) <= 0) {
        printf("\033[1;31m┌─────────── ERROR DE COMUNICACIÓN ───────────┐\n");
        printf("│ Error al enviar resultados finales          │\n");
        printf("└──────────────────────────────────────────────┘\033[0m\n");
        return;
    }
}

void guardar_kardex(const char* matricula, ResultadoAcademico* resultado_academico, ResultadoPsicometrico* resultado_psicometrico) {
    FILE* f = fopen("kardex.txt", "a");
    if (f != NULL) {
        fprintf(f, "%s,%d,%d,%d,%.2f,%d,%d,%.2f,%s\n",
                matricula,
                resultado_academico->matematicas,
                resultado_academico->espanol,
                resultado_academico->ingles,
                resultado_academico->promedio,
                resultado_psicometrico->correctas,
                resultado_psicometrico->total,
                resultado_psicometrico->porcentaje,
                resultado_psicometrico->fecha);
        fclose(f);
    }
}

void enviar_kardex(int sock, const char* matricula) {
    FILE* f = fopen("kardex.txt", "r");
    if (f == NULL) {
        char* mensaje = "No hay registros disponibles";
        send(sock, mensaje, strlen(mensaje), 0);
        return;
    }

    char linea[512];
    char mat_temp[20];
    Kardex kardex;
    memset(&kardex, 0, sizeof(Kardex));
    int encontrado = 0;

    while (fgets(linea, sizeof(linea), f)) {
        if (sscanf(linea, "%[^,],%d,%d,%d,%f,%d,%d,%f,%s",
               mat_temp,
               &kardex.resultados_academicos.matematicas,
               &kardex.resultados_academicos.espanol,
               &kardex.resultados_academicos.ingles,
               &kardex.resultados_academicos.promedio,
               &kardex.resultados_psicometricos.correctas,
               &kardex.resultados_psicometricos.total,
               &kardex.resultados_psicometricos.porcentaje,
               kardex.resultados_psicometricos.fecha) == 9) {

            if (strcmp(mat_temp, matricula) == 0) {
                encontrado = 1;
                strncpy(kardex.matricula, matricula, sizeof(kardex.matricula) - 1);
                kardex.matricula[sizeof(kardex.matricula) - 1] = '\0';
                
                // Enviar un indicador de éxito
                char status = 1;
                send(sock, &status, sizeof(char), 0);
                // Enviar el kardex
                send(sock, &kardex, sizeof(Kardex), 0);
                break;
            }
        }
    }

    if (!encontrado) {
        // Enviar un indicador de error
        char status = 0;
        send(sock, &status, sizeof(char), 0);
        char* mensaje = "No se encontraron registros para esta matrícula";
        send(sock, mensaje, strlen(mensaje), 0);
    }

    fclose(f);
}

void enviar_test_psicometrico(int sock, const char* matricula) {
    Pregunta preguntas_visual[MAX_PREGUNTAS];
    Pregunta preguntas_razon[MAX_PREGUNTAS];
    int num_visual = cargar_preguntas("preguntas_visual.txt", preguntas_visual);
    int num_razon = cargar_preguntas("preguntas_razonamiento.txt", preguntas_razon);
    
    // Enviar número total de preguntas
    int total = num_visual + num_razon;
    send(sock, &total, sizeof(int), 0);
    
    int correctas = 0;
    
    // Enviar preguntas visuales
    for (int i = 0; i < num_visual; i++) {
        send(sock, &preguntas_visual[i], sizeof(Pregunta), 0);
        char respuesta_usuario;
        recv(sock, &respuesta_usuario, 1, 0);
        
        // Enviar si es correcta
        char es_correcta = (respuesta_usuario == preguntas_visual[i].respuesta);
        if (es_correcta) correctas++;
        send(sock, &es_correcta, 1, 0);
    }
    
    // Enviar preguntas de razonamiento
    for (int i = 0; i < num_razon; i++) {
        send(sock, &preguntas_razon[i], sizeof(Pregunta), 0);
        char respuesta_usuario;
        recv(sock, &respuesta_usuario, 1, 0);
        
        // Enviar si es correcta
        char es_correcta = (respuesta_usuario == preguntas_razon[i].respuesta);
        if (es_correcta) correctas++;
        send(sock, &es_correcta, 1, 0);
    }
    
    // Guardar resultados
    ResultadoPsicometrico resultado = {
        correctas,
        total,
        (float)correctas / total * 100,
        "" // La fecha se establecerá en el cliente
    };
    
    // Guardar en el kardex
    ResultadoAcademico resultado_academico = {0}; // Resultado académico vacío
    guardar_kardex(matricula, &resultado_academico, &resultado);
    
    // Enviar resultado completo al cliente
    send(sock, &resultado, sizeof(ResultadoPsicometrico), 0);
}

// Variables globales para el seguimiento de clientes
static int clientes_activos = 0;

void mostrar_banner_servidor() {
    printf("\033[1;34m╔═══════════════════════════════════════════════════════════════════╗\n");
    printf("║          ✦ SERVIDOR DE EVALUACIÓN ACADÉMICA INTERACTIVO ✦         ║\n");
    printf("║                         Puerto: %d                              ║\n", PORT);
    printf("╚═══════════════════════════════════════════════════════════════════╝\033[0m\n");
    printf("\033[1;36m┌─────────────────── INFORMACIÓN DEL SISTEMA ────────────────────┐\n");
    printf("│  • Estado: \033[1;32mActivo\033[1;36m                                              │\n");
    printf("│  • Versión: 1.0                                                │\n");
    printf("│  • Modo: Producción                                            │\n");
    printf("└────────────────────────────────────────────────────────────────┘\033[0m\n");
}

void mostrar_estado_conexion(const char* evento, const char* ip, int socket, int num_clientes) {
    time_t tiempo = time(NULL);
    struct tm* tm_info = localtime(&tiempo);
    char tiempo_str[20];
    strftime(tiempo_str, 20, "%H:%M:%S", tm_info);

    printf("\033[1;36m╔══════════════════════ EVENTO DEL SISTEMA ══════════════════════╗\033[0m\n");
    printf("\033[1;36m║\033[0m \033[1;33m⏰ Hora: %s\033[0m\n", tiempo_str);
    printf("\033[1;36m║\033[0m \033[1;35m🔔 Evento: %s\033[0m\n", evento);
    printf("\033[1;36m║\033[0m \033[1;32m🌐 IP Cliente: %s\033[0m\n", ip);
    printf("\033[1;36m║\033[0m \033[1;34m🔌 Socket: %d\033[0m\n", socket);
    printf("\033[1;36m║\033[0m \033[1;33m👥 Clientes Activos: %d\033[0m\n", num_clientes);
    printf("\033[1;36m╚══════════════════════════════════════════════════════════════════╝\033[0m\n");

    // Mostrar barra de estado
    printf("\033[1;37m[Estado del Servidor]\033[0m ");
    for(int i = 0; i < num_clientes; i++) {
        printf("\033[1;32m■\033[0m");
    }
    for(int i = num_clientes; i < 10; i++) {
        printf("\033[1;31m□\033[0m");
    }
    printf(" (%d/10)\n\n", num_clientes);
}

void *manejar_cliente(void *socket_desc) {
    int sock = *(int*)socket_desc;
    char buffer[MAX_BUFFER] = {0};
    int opcion;
    char matricula[20] = {0};
    
    // Obtener información del cliente
    struct sockaddr_in addr;
    socklen_t addr_size = sizeof(struct sockaddr_in);
    getpeername(sock, (struct sockaddr *)&addr, &addr_size);
    char *ip_cliente = inet_ntoa(addr.sin_addr);
    
    // Incrementar contador de clientes y mostrar conexión
    clientes_activos++;
    mostrar_estado_conexion("NUEVA CONEXIÓN", ip_cliente, sock, clientes_activos);
    
    while(1) {
        // Recibir opción del menú
        if (recv(sock, &opcion, sizeof(int), 0) <= 0) {
            break; // Si el cliente se desconecta
        }
        
        switch(opcion) {
            case 1: {
                Usuario usuario;
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
                break;
            }
            case 2: {
                // Recibir matrícula
                recv(sock, matricula, sizeof(matricula), 0);
                enviar_test_psicometrico(sock, matricula);
                break;
            }
            case 3: {
                // Recibir matrícula
                recv(sock, matricula, sizeof(matricula), 0);
                enviar_examen_academico(sock, matricula);
                
                break;
            }
            case 4: {
                // Recibir matrícula
                recv(sock, matricula, sizeof(matricula), 0);
                enviar_kardex(sock, matricula);
                break;
            }
            case 5: {
                // Mostrar mensaje de desconexión
                clientes_activos--;
                mostrar_estado_conexion("CLIENTE DESCONECTADO", ip_cliente, sock, clientes_activos);
                free(socket_desc);
                close(sock);
                return 0;
            }
        }
    }
    
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
    
    mostrar_banner_servidor();
    printf("\033[1;32m[✓] Servidor iniciado correctamente\033[0m\n");
    printf("\033[1;33m[*] Esperando conexiones...\033[0m\n\n");
    
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