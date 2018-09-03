/*
 * FM9.h
 *
 *  Created on: 1 sep. 2018
 *      Author: Solitario Windows 95
 */

#ifndef SRC_FM9_H_
#define SRC_FM9_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <stdarg.h>
#include "funciones/funciones.h"
#include "commons/config.h"
#include "servidor/servidor.h"
#include "commons/string.h"

// constantes
char *PATH_LOG = "/home/utnso/solitario/tp-2018-2c-Solitario-Windows-95/Logs/logFM9.txt";
char *PATH_CONFIG = "/home/utnso/solitario/tp-2018-2c-Solitario-Windows-95/FM9/config.txt";

#define MAX_PARAMS 1

#define NUEVA_CONEXION_CPU 1
#define NUEVA_CONEXION_DIEGO 2

// estructuras
typedef struct {
	uint32_t PUERTO;
	char *MODO;
	uint32_t TAMANIO;
	uint32_t MAX_LINEA;
	uint32_t TAM_PAGINA;
} config_t;

typedef struct {
	char *comando;
	char *param[MAX_PARAMS];
	uint32_t cant_params;
} console_t;

// variables
t_log *log_consola;
t_log *log_fm9;
config_t config;
pthread_t thread_servidor;
pthread_t thread_consola;

// funciones
config_t load_config();
void server();
void command_handler(uint32_t command);
void consola();
void print_c(void (*log_function)(t_log *, const char *), char *message_template, ...);

#endif /* SRC_FM9_H_ */
