/*
 * FM9.c
 *
 *  Created on: 1 sep. 2018
 *      Author: Solitario Windows 95
 */
#include "FM9.h"

int main(void) {
	system("clear");
	puts("PROCESO FM9\n");

	log_consola = init_log(PATH_LOG, "Consola FM9", false, LOG_LEVEL_INFO);
	log_fm9 = init_log(PATH_LOG, "Proceso FM9", true, LOG_LEVEL_INFO);
	log_info(log_fm9, "Inicio del proceso");

	config = load_config();

	pthread_create(&thread_servidor, NULL, (void *) server, NULL);

	pthread_create(&thread_consola, NULL, (void *) consola, NULL);

	pthread_join(thread_consola, NULL);

	pthread_join(thread_servidor, NULL);

	exit(EXIT_SUCCESS);
}

config_t load_config() {
	t_config *config = config_create(PATH_CONFIG);

	config_t miConfig;
	miConfig.PUERTO = config_get_int_value(config, "PUERTO");
	miConfig.MODO = strdup(config_get_string_value(config, "MODO"));
	miConfig.TAMANIO = config_get_int_value(config, "TAMANIO");
	miConfig.MAX_LINEA = config_get_int_value(config, "MAX_LINEA");
	miConfig.TAM_PAGINA = config_get_int_value(config, "TAM_PAGINA");

	log_info(log_fm9, "---- Configuracion ----");
	log_info(log_fm9, "PUERTO = %d", miConfig.PUERTO);
	log_info(log_fm9, "MODO = %s", miConfig.MODO);
	log_info(log_fm9, "TAMANIO = %d", miConfig.TAMANIO);
	log_info(log_fm9, "MAX_LINEA = %d", miConfig.MAX_LINEA);
	log_info(log_fm9, "TAM_PAGINA = %d", miConfig.TAM_PAGINA);
	log_info(log_fm9, "-----------------------");

	config_destroy(config);
	return miConfig;
}

void server() {
	fd_set master; // conjunto maestro de descriptores de fichero
	fd_set read_fds; // conjunto temporal de descriptores de fichero para select()
	struct sockaddr_in remoteaddr; // dirección del cliente
	uint32_t fdmax; // número máximo de descriptores de fichero
	uint32_t newfd; // descriptor de socket de nueva conexión aceptada
	uint32_t command; // comando del cliente
	uint32_t nbytes;
	uint32_t addrlen;
	FD_ZERO(&master); // borra los conjuntos maestro y temporal
	FD_ZERO(&read_fds);

	// obtener socket a la escucha
	uint32_t servidor = build_server(config.PUERTO, log_consola);

	// añadir listener al conjunto maestro
	FD_SET(servidor, &master);
	// seguir la pista del descriptor de fichero mayor
	fdmax = servidor; // por ahora es éste

	// bucle principal
	while (true) {
		read_fds = master; // cópialo
		if (select(fdmax + 1, &read_fds, NULL, NULL, NULL) == -1) {
			log_error(log_consola, "select");
			exit(EXIT_FAILURE);
		}
		// explorar conexiones existentes en busca de datos que leer
		for (uint32_t i = 0; i <= fdmax; i++)
			if (FD_ISSET(i, &read_fds)) { // ¡¡tenemos datos!!
				if (i == servidor) {
					// gestionar nuevas conexiones
					addrlen = sizeof(remoteaddr);
					if ((newfd = accept(servidor, (struct sockaddr *) &remoteaddr, &addrlen)) == -1)
						log_error(log_consola, "accept");
					else {
						FD_SET(newfd, &master); // añadir al conjunto maestro
						if (newfd > fdmax) // actualizar el máximo
							fdmax = newfd;
						log_info(log_consola, "Nueva conexion desde %s en el socket %d", inet_ntoa(remoteaddr.sin_addr), newfd);
					}
				}
				else
					// gestionar datos de un cliente
					if ((nbytes = receive_int(i, &command)) <= 0) {
						// error o conexión cerrada por el cliente
						if (nbytes == 0)
							// conexión cerrada
							log_info(log_consola, "Socket %d colgado", i);
						else
							log_error(log_consola, "recv (comando)");

						close(i); // ¡Hasta luego!
						FD_CLR(i, &master); // eliminar del conjunto maestro
					}
					else
						// tenemos datos de algún cliente
						command_handler(command);
			} // if (FD_ISSET(i, &read_fds))
	} // while (true)
}

void command_handler(uint32_t command) {
	switch (command) {
	case NUEVA_CONEXION_CPU:
		log_info(log_consola, "Nueva conexion de CPU");
		break;
	case NUEVA_CONEXION_DIEGO:
		log_info(log_consola, "Nueva conexion desde El Diego");
		break;
	default:
		log_warning(log_consola, "%d: Comando recibido incorrecto", command);
	}
}

void consola() {
	char *linea;
	char *token;
	console_t *consola;

	while (true) {
		linea = readline("FM9> ");

		if (strlen(linea) > 0) {
			add_history(linea);
			log_info(log_consola, "Linea leida: %s", linea);
			consola = malloc(sizeof(console_t));

			if (consola != NULL) {
				consola->comando = strdup(strtok(linea, " "));
				consola->cant_params = 0;

				while (consola->cant_params < MAX_PARAMS && (token = strtok(NULL, " ")) != NULL)
					consola->param[consola->cant_params++] = strdup(token);

				if (streq(consola->comando, "clear"))
					system("clear");

				else if (streq(consola->comando, "dump"))
					if (consola->cant_params < 1)
						print_c((void *) log_info, "%s: falta el parametro ID del DTB\n", consola->comando);
					else {
						// TODO: comando ejecutar
					}

				else
					print_c((void *) log_info, "%s: Comando incorrecto\n", consola->comando);

				free(consola->comando);
				for (uint32_t i = 0; i < consola->cant_params; i++)
					free(consola->param[i]);
				free(consola);
			}
		}
		free(linea);
	}
}

void print_c(void (*log_function)(t_log *, const char *), char *message_template, ...) {
	va_list arguments;
	va_start(arguments, message_template);
	char *message = string_from_vformat(message_template, arguments);
	va_end(arguments);
	log_function(log_consola, message);
	printf("%s", message);
	free(message);
}
