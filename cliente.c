#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ctype.h>

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

typedef struct {
    char matricula[20];
    ResultadoAcademico resultados_academicos;
    ResultadoPsicometrico resultados_psicometricos;
} Kardex;

// Función para mostrar el kardex
void mostrar_kardex(int sock) {
    char matricula[20];
    printf("\033[1;36m╔═══════════════════════════════════════╗\n");
    printf("║         \033[1;33m✯ KARDEX ESCOLAR ✯\033[1;36m         ║\n");
    printf("╚═══════════════════════════════════════╝\033[0m\n\n");
    printf("\033[1;33mIngrese su matrícula:\033[0m ");
    scanf(" %[^\n]", matricula);
    while (getchar() != '\n'); // Limpiar el buffer después de leer la matrícula
    
    // Enviar matrícula al servidor
    send(sock, matricula, sizeof(matricula), 0);
    
    // Recibir el estado de la respuesta
    char status;
    recv(sock, &status, sizeof(char), 0);
    
    if (status) {
        // Si el estado es exitoso, recibir el kardex
        Kardex kardex;
        recv(sock, &kardex, sizeof(Kardex), 0);
        
        printf("\n\033[1;34m┌─────────── RESULTADOS ACADÉMICOS ───────────┐\033[0m\n");
        printf("\033[1;32m│\033[0m Matemáticas:\033[1;33m %d\033[0m\n", kardex.resultados_academicos.matematicas);
        printf("\033[1;32m│\033[0m Español:\033[1;33m %d\033[0m\n", kardex.resultados_academicos.espanol);
        printf("\033[1;32m│\033[0m Inglés:\033[1;33m %d\033[0m\n", kardex.resultados_academicos.ingles);
        printf("\033[1;32m│\033[0m Promedio general:\033[1;33m %.2f\033[0m\n", kardex.resultados_academicos.promedio);
        printf("\033[1;34m└───────────────────────────────────────────────┘\033[0m\n");
        
        printf("\n\033[1;34m┌─────────── RESULTADOS PSICOMÉTRICOS ─────────┐\033[0m\n");
        printf("\033[1;32m│\033[0m Respuestas correctas:\033[1;33m %d de %d\033[0m\n", kardex.resultados_psicometricos.correctas, kardex.resultados_psicometricos.total);
        printf("\033[1;32m│\033[0m Porcentaje de aciertos:\033[1;33m %.2f%%\033[0m\n", kardex.resultados_psicometricos.porcentaje);
        printf("\033[1;32m│\033[0m Fecha:\033[1;33m %s\033[0m\n", kardex.resultados_psicometricos.fecha);
        printf("\033[1;34m└───────────────────────────────────────────────┘\033[0m\n");
    } else {
        // Si el estado indica error, recibir el mensaje de error
        char buffer[MAX_BUFFER];
        recv(sock, buffer, MAX_BUFFER, 0);
        printf("\n\033[1;31m┌─────────── ERROR ───────────┐\n");
        printf("│ %s\n", buffer);
        printf("└────────────────────────────┘\033[0m\n");
    }
    
    printf("\nPresiona Enter para continuar...");
    while (getchar() != '\n'); // Limpiar el buffer después de leer
}

// Función para limpiar la pantalla
void limpiar_pantalla() {
    system("clear");
}

// Función para mostrar el menú principal
void mostrar_menu() {
    printf("\033[1;36m╔═══════════════════════════════════════╗\n");
    printf("║      \033[1;33m✯ SISTEMA DE EVALUACIÓN ✯\033[1;36m      ║\n");
    printf("╚═══════════════════════════════════════╝\033[0m\n\n");
    printf("\033[1;34m┌─────────────── MENÚ ───────────────┐\033[0m\n\n");
    printf("\033[1;32m[1]\033[0m ► Registrarse\n");
    printf("\033[1;32m[2]\033[0m ► Iniciar Test Psicométrico\n");
    printf("\033[1;32m[3]\033[0m ► Realizar Examen Académico\n");
    printf("\033[1;32m[4]\033[0m ► Ver Kardex\n");
    printf("\033[1;31m[5]\033[0m ► Salir\n\n");
    printf("\033[1;34m└────────────────────────────────────┘\033[0m\n\n");
    printf("\033[1;33mSeleccione una opción:\033[0m ");
}

void realizar_examen_academico(int sock) {
    int num_mate, num_espanol, num_ingles;
    if (recv(sock, &num_mate, sizeof(int), 0) <= 0 ||
        recv(sock, &num_espanol, sizeof(int), 0) <= 0 ||
        recv(sock, &num_ingles, sizeof(int), 0) <= 0) {
        printf("\033[1;31mError al recibir el número de preguntas\033[0m\n");
        return;
    }
    
    if (num_mate <= 0 || num_espanol <= 0 || num_ingles <= 0) {
        printf("\033[1;31mError: No hay preguntas disponibles para el examen\033[0m\n");
        return;
    }
    
    Pregunta pregunta;
    ResultadoAcademico resultado;
    int correctas_mate = 0, correctas_espanol = 0, correctas_ingles = 0;
    
    printf("\033[1;32m=== EXAMEN DE MATEMÁTICAS ===\033[0m\n\n");
    for (int i = 0; i < num_mate; i++) {
        memset(&pregunta, 0, sizeof(Pregunta));
        if (recv(sock, &pregunta, sizeof(Pregunta), 0) <= 0) {
            printf("\033[1;31mError al recibir la pregunta\033[0m\n");
            return;
        }
        
        // Verificar si la pregunta está vacía
        if (strlen(pregunta.pregunta) == 0) {
            printf("\033[1;31mError: Pregunta vacía recibida\033[0m\n");
            return;
        }
        
        // Mostrar marco superior
        printf("\n\033[1;36m╔════════════ Pregunta %d de %d ════════════╗\033[0m\n", i + 1, num_mate);
        printf("\033[1;36m║\033[0m                                          \033[1;36m║\033[0m\n");
        printf("\033[1;36m║\033[0m \033[1;37m%s\033[0m", pregunta.pregunta);
        // Agregar espacios para alinear el marco derecho
        int espacios = 42 - strlen(pregunta.pregunta);
        for(int j = 0; j < espacios; j++) printf(" ");
        printf("\033[1;36m║\033[0m\n");
        printf("\033[1;36m║\033[0m                                          \033[1;36m║\033[0m\n");
        printf("\033[1;36m║\033[0m   \033[1;33mA)\033[0m %s", pregunta.opciones[0]);
        espacios = 39 - strlen(pregunta.opciones[0]);
        for(int j = 0; j < espacios; j++) printf(" ");
        printf("\033[1;36m║\033[0m\n");
        printf("\033[1;36m║\033[0m   \033[1;33mB)\033[0m %s", pregunta.opciones[1]);
        espacios = 39 - strlen(pregunta.opciones[1]);
        for(int j = 0; j < espacios; j++) printf(" ");
        printf("\033[1;36m║\033[0m\n");
        printf("\033[1;36m║\033[0m   \033[1;33mC)\033[0m %s", pregunta.opciones[2]);
        espacios = 39 - strlen(pregunta.opciones[2]);
        for(int j = 0; j < espacios; j++) printf(" ");
        printf("\033[1;36m║\033[0m\n");
        printf("\033[1;36m╚══════════════════════════════════════════╝\033[0m\n");
        
        char respuesta;
        do {
            printf("\033[1;32mTu respuesta (A/B/C):\033[0m ");
            scanf(" %c", &respuesta);
            while (getchar() != '\n'); // Limpiar el buffer después de leer 
            respuesta = toupper(respuesta);
            if (respuesta != 'A' && respuesta != 'B' && respuesta != 'C') {
                printf("\033[1;31mPor favor, ingresa una opción válida (A, B o C)\033[0m\n");
            }
        } while (respuesta != 'A' && respuesta != 'B' && respuesta != 'C');
        
        if (send(sock, &respuesta, 1, 0) <= 0) {
            printf("\033[1;31mError al enviar respuesta\033[0m\n");
            return;
        }
        
        char es_correcta;
        if (recv(sock, &es_correcta, 1, 0) <= 0) {
            printf("\033[1;31mError al recibir resultado\033[0m\n");
            return;
        }
        
        printf("\n");
        if (es_correcta) {
            printf("\033[1;32m╔═══════════════ RESULTADO ═══════════════╗\n");
            printf("║          ¡RESPUESTA CORRECTA!          ║\n");
            printf("║             ¡Excelente! 🌟             ║\n");
            printf("╚══════════════════════════════════════════╝\033[0m\n");
            correctas_mate++;
        } else {
            printf("\033[1;31m╔═══════════════ RESULTADO ═══════════════╗\n");
            printf("║         RESPUESTA INCORRECTA          ║\n");
            printf("║       ¡Sigue intentándolo! 📚         ║\n");
            printf("╚══════════════════════════════════════════╝\033[0m\n");
        }
    }
    
    printf("\n\033[1;32m=== EXAMEN DE ESPAÑOL ===\033[0m\n\n");
    for (int i = 0; i < num_espanol; i++) {
        memset(&pregunta, 0, sizeof(Pregunta));
        if (recv(sock, &pregunta, sizeof(Pregunta), 0) <= 0) {
            printf("\033[1;31mError al recibir la pregunta\033[0m\n");
            return;
        }
        
        // Mostrar marco superior
        printf("\n\033[1;36m╔════════════ Pregunta %d de %d ════════════╗\033[0m\n", i + 1, num_espanol);
        printf("\033[1;36m║\033[0m                                          \033[1;36m║\033[0m\n");
        printf("\033[1;36m║\033[0m \033[1;37m%s\033[0m", pregunta.pregunta);
        // Agregar espacios para alinear el marco derecho
        int espacios = 42 - strlen(pregunta.pregunta);
        for(int j = 0; j < espacios; j++) printf(" ");
        printf("\033[1;36m║\033[0m\n");
        printf("\033[1;36m║\033[0m                                          \033[1;36m║\033[0m\n");
        printf("\033[1;36m║\033[0m   \033[1;33mA)\033[0m %s", pregunta.opciones[0]);
        espacios = 39 - strlen(pregunta.opciones[0]);
        for(int j = 0; j < espacios; j++) printf(" ");
        printf("\033[1;36m║\033[0m\n");
        printf("\033[1;36m║\033[0m   \033[1;33mB)\033[0m %s", pregunta.opciones[1]);
        espacios = 39 - strlen(pregunta.opciones[1]);
        for(int j = 0; j < espacios; j++) printf(" ");
        printf("\033[1;36m║\033[0m\n");
        printf("\033[1;36m║\033[0m   \033[1;33mC)\033[0m %s", pregunta.opciones[2]);
        espacios = 39 - strlen(pregunta.opciones[2]);
        for(int j = 0; j < espacios; j++) printf(" ");
        printf("\033[1;36m║\033[0m\n");
        printf("\033[1;36m╚══════════════════════════════════════════╝\033[0m\n");
        
        char respuesta;
        do {
            printf("\033[1;32mTu respuesta (A/B/C):\033[0m ");
            scanf(" %c", &respuesta);
            while (getchar() != '\n'); // Limpiar el buffer después de leer
            respuesta = toupper(respuesta);
            if (respuesta != 'A' && respuesta != 'B' && respuesta != 'C') {
                printf("\033[1;31mPor favor, ingresa una opción válida (A, B o C)\033[0m\n");
            }
        } while (respuesta != 'A' && respuesta != 'B' && respuesta != 'C');
        
        send(sock, &respuesta, 1, 0);
        
        char es_correcta;
        recv(sock, &es_correcta, 1, 0);
        printf("\n");
        if (es_correcta) {
            printf("\033[1;32m╔═══════════════ RESULTADO ═══════════════╗\n");
            printf("║          ¡RESPUESTA CORRECTA!          ║\n");
            printf("║             ¡Excelente! 🌟             ║\n");
            printf("╚══════════════════════════════════════════╝\033[0m\n");
            correctas_espanol++;
        } else {
            printf("\033[1;31m╔═══════════════ RESULTADO ═══════════════╗\n");
            printf("║         RESPUESTA INCORRECTA          ║\n");
            printf("║       ¡Sigue intentándolo! 📚         ║\n");
            printf("╚══════════════════════════════════════════╝\033[0m\n");
        }
    }
    
    printf("\n\033[1;32m=== EXAMEN DE INGLÉS ===\033[0m\n\n");
    for (int i = 0; i < num_ingles; i++) {
        memset(&pregunta, 0, sizeof(Pregunta));
        if (recv(sock, &pregunta, sizeof(Pregunta), 0) <= 0) {
            printf("\033[1;31mError al recibir la pregunta\033[0m\n");
            return;
        }
        
        // Mostrar marco superior
        printf("\n\033[1;36m╔════════════ Pregunta %d de %d ════════════╗\033[0m\n", i + 1, num_ingles);
        printf("\033[1;36m║\033[0m                                          \033[1;36m║\033[0m\n");
        printf("\033[1;36m║\033[0m \033[1;37m%s\033[0m", pregunta.pregunta);
        // Agregar espacios para alinear el marco derecho
        int espacios = 42 - strlen(pregunta.pregunta);
        for(int j = 0; j < espacios; j++) printf(" ");
        printf("\033[1;36m║\033[0m\n");
        printf("\033[1;36m║\033[0m                                          \033[1;36m║\033[0m\n");
        printf("\033[1;36m║\033[0m   \033[1;33mA)\033[0m %s", pregunta.opciones[0]);
        espacios = 39 - strlen(pregunta.opciones[0]);
        for(int j = 0; j < espacios; j++) printf(" ");
        printf("\033[1;36m║\033[0m\n");
        printf("\033[1;36m║\033[0m   \033[1;33mB)\033[0m %s", pregunta.opciones[1]);
        espacios = 39 - strlen(pregunta.opciones[1]);
        for(int j = 0; j < espacios; j++) printf(" ");
        printf("\033[1;36m║\033[0m\n");
        printf("\033[1;36m║\033[0m   \033[1;33mC)\033[0m %s", pregunta.opciones[2]);
        espacios = 39 - strlen(pregunta.opciones[2]);
        for(int j = 0; j < espacios; j++) printf(" ");
        printf("\033[1;36m║\033[0m\n");
        printf("\033[1;36m╚══════════════════════════════════════════╝\033[0m\n");
        
        char respuesta;
        do {
            printf("\033[1;32mTu respuesta (A/B/C):\033[0m ");
            scanf(" %c", &respuesta);
            while (getchar() != '\n'); // Limpiar el buffer después de leer la matrícula
            respuesta = toupper(respuesta);
            if (respuesta != 'A' && respuesta != 'B' && respuesta != 'C') {
                printf("\033[1;31mPor favor, ingresa una opción válida (A, B o C)\033[0m\n");
            }
        } while (respuesta != 'A' && respuesta != 'B' && respuesta != 'C');
        
        send(sock, &respuesta, 1, 0);
        
        char es_correcta;
        recv(sock, &es_correcta, 1, 0);
        printf("\n");
        if (es_correcta) {
            printf("\033[1;32m╔═══════════════ RESULTADO ═══════════════╗\n");
            printf("║          ¡RESPUESTA CORRECTA!          ║\n");
            printf("║             ¡Excelente! 🌟             ║\n");
            printf("╚══════════════════════════════════════════╝\033[0m\n");
            correctas_ingles++;
        } else {
            printf("\033[1;31m╔═══════════════ RESULTADO ═══════════════╗\n");
            printf("║         RESPUESTA INCORRECTA          ║\n");
            printf("║       ¡Sigue intentándolo! 📚         ║\n");
            printf("╚══════════════════════════════════════════╝\033[0m\n");
        }
    }
    
    // Recibir resultados
    recv(sock, &resultado, sizeof(ResultadoAcademico), 0);
    
    printf("\n\033[1;36m╔═══════════════ RESULTADOS FINALES ═══════════════╗\n");
    printf("║                                                  ║\n");
    printf("║  \033[1;33mMatemáticas:\033[0m %d/%d                             ║\n", resultado.matematicas, num_mate);
    printf("║  \033[1;33mEspañol:\033[0m %d/%d                                ║\n", resultado.espanol, num_espanol);
    printf("║  \033[1;33mInglés:\033[0m %d/%d                                 ║\n", resultado.ingles, num_ingles);
    printf("║  \033[1;33mPromedio general:\033[0m %.2f                        ║\n", resultado.promedio);
    printf("║                                                  ║\n");
    
    float porcentaje_mate = (float)resultado.matematicas / num_mate * 100;
    float porcentaje_espanol = (float)resultado.espanol / num_espanol * 100;
    float porcentaje_ingles = (float)resultado.ingles / num_ingles * 100;
    
    printf("║  \033[1;34mPorcentajes por materia:\033[0m                      ║\n");
    printf("║  Matemáticas: %.1f%%                              ║\n", porcentaje_mate);
    printf("║  Español: %.1f%%                                 ║\n", porcentaje_espanol);
    printf("║  Inglés: %.1f%%                                  ║\n", porcentaje_ingles);
    printf("║                                                  ║\n");
    
    if (resultado.promedio >= 8.0) {
        printf("║  \033[1;32m¡Excelente desempeño! 🌟                         \033[1;36m║\n");
        printf("║  \033[1;32m¡Has demostrado un gran dominio! 🏆               \033[1;36m║\n");
    } else if (resultado.promedio >= 6.0) {
        printf("║  \033[1;33m¡Buen trabajo! Hay espacio para mejorar 📚        \033[1;36m║\n");
        printf("║  \033[1;33m¡Sigue practicando para alcanzar la excelencia! 💪  \033[1;36m║\n");
    } else {
        printf("║  \033[1;31m¡Ánimo! Necesitas reforzar tus conocimientos 📖    \033[1;36m║\n");
        printf("║  \033[1;31m¡Cada intento es una oportunidad de mejorar! 🎯     \033[1;36m║\n");
    }
    
    printf("║                                                  ║\n");
    printf("╚══════════════════════════════════════════════════╝\033[0m\n");
    
    printf("\nPresiona Enter para continuar...");
    while (getchar() != '\n'); // Limpiar el buffer después de leer
}

void realizar_test_psicometrico(int sock) {
    int total_preguntas;
    recv(sock, &total_preguntas, sizeof(int), 0);
    
    printf("\033[1;36m╔═══════════════════════════════════════╗\n");
    printf("║      \033[1;33m✯ TEST PSICOMÉTRICO ✯\033[1;36m      ║\n");
    printf("╚═══════════════════════════════════════╝\033[0m\n\n");
    printf("\033[1;34mTotal de preguntas: \033[1;33m%d\033[0m\n\n", total_preguntas);
    
    int correctas = 0;
    Pregunta pregunta;
    
    for (int i = 0; i < total_preguntas; i++) {
        recv(sock, &pregunta, sizeof(Pregunta), 0);
        
        printf("\n\033[1;36m╔════════════ Pregunta %d de %d ════════════╗\033[0m\n", i + 1, total_preguntas);
        printf("\033[1;36m║\033[0m                                          \033[1;36m║\033[0m\n");
        printf("\033[1;36m║\033[0m \033[1;37m%s\033[0m", pregunta.pregunta);
        int espacios = 42 - strlen(pregunta.pregunta);
        for(int j = 0; j < espacios; j++) printf(" ");
        printf("\033[1;36m║\033[0m\n");
        printf("\033[1;36m║\033[0m                                          \033[1;36m║\033[0m\n");
        printf("\033[1;36m║\033[0m   \033[1;33mA)\033[0m %s", pregunta.opciones[0]);
        espacios = 39 - strlen(pregunta.opciones[0]);
        for(int j = 0; j < espacios; j++) printf(" ");
        printf("\033[1;36m║\033[0m\n");
        printf("\033[1;36m║\033[0m   \033[1;33mB)\033[0m %s", pregunta.opciones[1]);
        espacios = 39 - strlen(pregunta.opciones[1]);
        for(int j = 0; j < espacios; j++) printf(" ");
        printf("\033[1;36m║\033[0m\n");
        printf("\033[1;36m║\033[0m   \033[1;33mC)\033[0m %s", pregunta.opciones[2]);
        espacios = 39 - strlen(pregunta.opciones[2]);
        for(int j = 0; j < espacios; j++) printf(" ");
        printf("\033[1;36m║\033[0m\n");
        printf("\033[1;36m╚══════════════════════════════════════════╝\033[0m\n");
        
        char respuesta;
        do {
            printf("\033[1;32mTu respuesta (A/B/C):\033[0m ");
            scanf(" %c", &respuesta);
            while (getchar() != '\n'); // Limpiar el buffer después de leer
            respuesta = toupper(respuesta);
            if (respuesta != 'A' && respuesta != 'B' && respuesta != 'C') {
                printf("\033[1;31mPor favor, ingresa una opción válida (A, B o C)\033[0m\n");
            }
        } while (respuesta != 'A' && respuesta != 'B' && respuesta != 'C');
        
        send(sock, &respuesta, 1, 0);
        
        char es_correcta;
        recv(sock, &es_correcta, 1, 0);
        
        printf("\n");
        if (es_correcta) {
            printf("\033[1;32m╔═══════════════ RESULTADO ═══════════════╗\n");
            printf("║          ¡RESPUESTA CORRECTA!          ║\n");
            printf("║             ¡Excelente! 🌟             ║\n");
            printf("╚══════════════════════════════════════════╝\033[0m\n");
            correctas++;
        } else {
            printf("\033[1;31m╔═══════════════ RESULTADO ═══════════════╗\n");
            printf("║         RESPUESTA INCORRECTA          ║\n");
            printf("║       ¡Sigue intentándolo! 📚         ║\n");
            printf("╚══════════════════════════════════════════╝\033[0m\n");
        }
    }
    
    float porcentaje = (float)correctas / total_preguntas * 100;
    printf("\n\033[1;36m╔═══════════════ RESULTADOS FINALES ═══════════════╗\n");
    printf("║                                                  ║\n");
    printf("║  \033[1;33mRespuestas correctas:\033[0m %d de %d                    ║\n", correctas, total_preguntas);
    printf("║  \033[1;33mPorcentaje de aciertos:\033[0m %.2f%%                   ║\n", porcentaje);
    printf("║                                                  ║\n");
    if (porcentaje >= 80) {
        printf("║  \033[1;32m¡Excelente desempeño! 🌟                         \033[1;36m║\n");
        printf("║  \033[1;32m¡Has demostrado un gran dominio! 🏆               \033[1;36m║\n");
    } else if (porcentaje >= 60) {
        printf("║  \033[1;33m¡Buen trabajo! Hay espacio para mejorar 📚        \033[1;36m║\n");
        printf("║  \033[1;33m¡Sigue practicando para alcanzar la excelencia! 💪  \033[1;36m║\n");
    } else {
        printf("║  \033[1;31mNecesitas practicar más 📖                        \033[1;36m║\n");
        printf("║  \033[1;31m¡No te rindas, cada intento es una oportunidad! 🎯  \033[1;36m║\n");
    }
    printf("║                                                  ║\n");
    printf("╚══════════════════════════════════════════════════╝\033[0m\n");

    
    printf("\nPresiona Enter para continuar...");
    while (getchar() != '\n'); // Limpiar el buffer después de leer
}

// Función para registrar usuario
Usuario registrar_usuario() {
    Usuario usuario;
    limpiar_pantalla();
    printf("\033[1;36m╔═══════════════════════════════════════╗\n");
    printf("║       \033[1;33m✯ REGISTRO DE USUARIO ✯\033[1;36m       ║\n");
    printf("╚═══════════════════════════════════════╝\033[0m\n\n");
    
    printf("\033[1;34m┌─────────── DATOS PERSONALES ───────────┐\033[0m\n\n");
    
    printf("\033[1;32m►\033[0m Nombre: ");
    scanf(" %[^\n]", usuario.nombre);
    while (getchar() != '\n'); // Limpiar el buffer después de leer
    
    printf("\033[1;32m►\033[0m Matrícula: ");
    scanf(" %[^\n]", usuario.matricula);
    while (getchar() != '\n'); // Limpiar el buffer después de leer
    
    printf("\033[1;32m►\033[0m Carrera: ");
    scanf(" %[^\n]", usuario.carrera);
    while (getchar() != '\n'); // Limpiar el buffer después de leer
    
    printf("\033[1;32m►\033[0m Edad: ");
    scanf("%d", &usuario.edad);
    while (getchar() != '\n'); // Limpiar el buffer después de leer
    
    printf("\033[1;32m►\033[0m Género (M/F): ");
    scanf(" %c", &usuario.genero);
    while (getchar() != '\n'); // Limpiar el buffer después de leer
    
    printf("\033[1;32m►\033[0m Semestre: ");
    scanf("%d", &usuario.semestre);
    while (getchar() != '\n'); // Limpiar el buffer después de leer
    
    printf("\n\033[1;34m┌─────────── SEGURIDAD ───────────┐\033[0m\n\n");
    
    printf("\033[1;32m►\033[0m Contraseña: ");
    scanf(" %[^\n]", usuario.password);
    while (getchar() != '\n'); // Limpiar el buffer después de leer
    
    printf("\n\033[1;34m└────────────────────────────────┘\033[0m\n");
    
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
        while (getchar() != '\n'); // Limpiar el buffer de entrada
        
        switch(opcion) {
            case 1: {
                Usuario usuario = registrar_usuario();
                send(sock, &opcion, sizeof(int), 0);
                send(sock, &usuario, sizeof(Usuario), 0);
                recv(sock, buffer, MAX_BUFFER, 0);
                printf("\n%s\n", buffer);
                printf("\nPresione Enter para continuar...");
                while (getchar() != '\n'); // Limpiar el buffer después de leer
                break;
            }
            case 2: {
                send(sock, &opcion, sizeof(int), 0);
                // Solicitar matrícula antes del test
                char matricula[20];
                printf("\nIngrese su matrícula: ");
                scanf(" %[^\n]", matricula);
                while (getchar() != '\n'); // Limpiar el buffer después de leer la matrícula
                send(sock, matricula, sizeof(matricula), 0);
                realizar_test_psicometrico(sock);
                break;
            }
            case 3: {
                send(sock, &opcion, sizeof(int), 0);
                // Solicitar matrícula antes del examen
                char matricula[20];
                printf("\nIngrese su matrícula: ");
                scanf(" %[^\n]", matricula);
                while (getchar() != '\n'); // Limpiar el buffer después de leer la matrícula
                send(sock, matricula, sizeof(matricula), 0);
                realizar_examen_academico(sock);
               
                break;
            }
            case 4: {
                send(sock, &opcion, sizeof(int), 0);
                mostrar_kardex(sock);
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