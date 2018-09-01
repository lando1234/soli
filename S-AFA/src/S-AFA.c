/*
 * S-AFA.c
 *
 *  Created on: 1 sep. 2018
 *      Author: Solitario Windows 95
 */
#include "S-AFA.h"

int main(void) {
	system("clear");
	puts("PROCESO S-AFA\n");

	log_consola = init_log(PATH_LOG, "Consola S-AFA", false, LOG_LEVEL_INFO);
	log_safa = init_log(PATH_LOG, "Proceso S-AFA", true, LOG_LEVEL_INFO);
	log_info(log_safa, "Inicio del proceso");

	config = load_config();

	pthread_create(&thread_servidor, NULL, (void *) consola, NULL);

	pthread_create(&thread_consola, NULL, (void *) consola, NULL);

	pthread_join(thread_consola, NULL);

	pthread_join(thread_servidor, NULL);

	exit(EXIT_SUCCESS);
}

config_t load_config() {
	t_config *config = config_create(PATH_CONFIG);

	config_t miConfig;
	miConfig.PUERTO = config_get_int_value(config, "PUERTO");
	miConfig.ALGORITMO = strdup(config_get_string_value(config, "ALGORITMO"));
	miConfig.QUANTUM = config_get_int_value(config, "QUANTUM");
	miConfig.MULTIPROGRAMACION = config_get_int_value(config, "MULTIPROGRAMACION");
	miConfig.RETARDO_PLANIF = config_get_int_value(config, "RETARDO_PLANIF");

	log_info(log_safa, "---- Configuracion ----");
	log_info(log_safa, "PUERTO = %d", miConfig.PUERTO);
	log_info(log_safa, "ALGORITMO = %s", miConfig.ALGORITMO);
	log_info(log_safa, "QUANTUM = %d", miConfig.QUANTUM);
	log_info(log_safa, "MULTIPROGRAMACION = %d", miConfig.MULTIPROGRAMACION);
	log_info(log_safa, "RETARDO_PLANIF = %d milisegundos", miConfig.RETARDO_PLANIF);
	log_info(log_safa, "-----------------------");

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
	uint32_t servidor = build_server(config.PUERTO, log_safa);

	// añadir listener al conjunto maestro
	FD_SET(servidor, &master);
	// seguir la pista del descriptor de fichero mayor
	fdmax = servidor; // por ahora es éste

	// bucle principal
	while (true) {
		read_fds = master; // cópialo
		if (select(fdmax + 1, &read_fds, NULL, NULL, NULL) == -1) {
			log_error(log_safa, "select");
			exit(EXIT_FAILURE);
		}
		// explorar conexiones existentes en busca de datos que leer
		for (uint32_t i = 0; i <= fdmax; i++)
			if (FD_ISSET(i, &read_fds)) { // ¡¡tenemos datos!!
				if (i == servidor) {
					// gestionar nuevas conexiones
					addrlen = sizeof(remoteaddr);
					if ((newfd = accept(servidor, (struct sockaddr *) &remoteaddr, &addrlen)) == -1)
						log_error(log_safa, "accept");
					else {
						FD_SET(newfd, &master); // añadir al conjunto maestro
						if (newfd > fdmax) // actualizar el máximo
							fdmax = newfd;
						log_info(log_safa, "Nueva conexion desde %s en el socket %d", inet_ntoa(remoteaddr.sin_addr), newfd);
					}
				}
				else
					// gestionar datos de un cliente
					if ((nbytes = receive_int(i, &command)) <= 0) {
						// error o conexión cerrada por el cliente
						if (nbytes == 0)
							// conexión cerrada
							log_info(log_safa, "Socket %d colgado", i);
						else
							log_error(log_safa, "recv (comando)");

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

	default:
		log_warning(log_safa, "%d: Comando recibido incorrecto", command);
	}
}

void consola() {
	char *linea;

	while (true) {
		linea = readline("S-AFA> ");

		if (strlen(linea) > 0) {
			add_history(linea);

			print_c((void *) log_info, "Comando incorrecto\n");
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