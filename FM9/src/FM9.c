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
	
	setear_modo();


	pthread_create(&thread_consola, NULL, (void *) consola, NULL);

	pthread_join(thread_consola, NULL);

	pthread_join(thread_servidor, NULL);

	exit(EXIT_SUCCESS);
	
	//free();


}

config_t load_config() {
	t_config *config = config_create(PATH_CONFIG);

	config_t miConfig;
	miConfig.PUERTO = config_get_int_value(config, "PUERTO");
	miConfig.MODO = strdup(config_get_string_value(config, "MODO"));
	miConfig.TAMANIO = config_get_int_value(config, "TAMANIO");
	miConfig.MAX_LINEA = config_get_int_value(config, "MAX_LINEA");
	miConfig.TAM_PAGINA = config_get_int_value(config, "TAM_PAGINA");
	miConfig.TRANSFER_SIZE = config_get_int_value(config, "TRANSFER_SIZE");

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
						//log_info(log_consola, "Nueva conexion desde %s en el socket %d", inet_ntoa(remoteaddr.sin_addr), newfd);
					}
				}
				else
					// gestionar datos de un cliente
					if ((nbytes = receive_int(i, &command)) <= 0) {
						// error o conexión cerrada por el cliente
						if (nbytes == 0)
							// conexión cerrada
							if (i == diego)
								log_info(log_consola, "Se desconecto El Diego");
							else
								log_info(log_consola, "CPU desconectada");
						else
							log_error(log_consola, "recv (comando)");

						close(i); // ¡Hasta luego!
						FD_CLR(i, &master); // eliminar del conjunto maestro
					}
					else
						// tenemos datos de algún cliente
						command_handler(command, i);
			} // Si, ahí está bien, pero si por ejemplo querés que ese valor contra el que comparas sea dinámico en base a un valor que recibis en la función, tenés que hacer referencia a esa variable y poner la función condicional en la función que hace el list_findif (FD_ISSET(i, &read_fds))
	} // while (true)
}

void command_handler(uint32_t command, uint32_t socket) {
	switch (command) {
	case NUEVA_CONEXION_DIEGO:
		log_info(log_consola, "Nueva conexion desde El Diego");
		diego = socket;
		break;
	case NUEVA_CONEXION_CPU:
		log_info(log_consola, "Nueva conexion de CPU");

		break;
	case CARGAR_PROCESO:
		log_info(log_consola, "Cargando proceso en memoria");
		guardar_proceso(socket);
		break;
	case ABRIR_LINEA:
		log_info(log_consola, ".");
		abrir_linea(socket);
		break;
	case MODIFICAR_LINEA:
		modificar_linea(socket);
		log_info(log_consola, ".");

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

				if (str_eq(consola->comando, "clear"))
					system("clear");

				else if (str_eq(consola->comando, "dump"))
					if (consola->cant_params < 1)
						print_c(log_consola, "%s: falta el parametro ID del DTB\n", consola->comando);
					else {
						// TODO: comando dump
					}

				else
					print_c(log_consola, "%s: Comando incorrecto\n", consola->comando);

				free(consola->comando);
				for (uint32_t i = 0; i < consola->cant_params; i++)
					free(consola->param[i]);
				free(consola);
			}
		}
		free(linea);
	}
}

void setear_segmentacion_simple(){
	log_info(log_fm9, "Segmentación Simple seteada");
	inicializar_memoria_segmentacion_simple();
}

void setear_paginacion_invertida(){
	log_info(log_fm9, "Tablas de Paginación Invertida seteada");
	inicializar_memoria_paginacion_invertida();
}

void setear_segmentacion_paginada(){
	log_info(log_fm9, "Segmentación Paginada seteada");
	inicializar_memoria_segmentacion_paginada();
}





void setear_modo(){
	if(strcmp("SEG", config.MODO)== 0){
		setear_segmentacion_simple();
	}
	else if(strcmp("TPI", config.MODO)== 0){
		setear_paginacion_invertida();
	}
	else if(strcmp("SPI", config.MODO)== 0){
		setear_segmentacion_paginada();
	}
	else {
		log_error(log_fm9, "Modo de Gestión de Memoria desconocido");
	}
}

int obtener_cantidad_lineas(int longitud_paquete){


	return (longitud_paquete + config.MAX_LINEA - 1) / config.MAX_LINEA;

}

void guardar_proceso(int socket_diego){

	int pid = recibir_int(socket_diego);
	int cantidad_lineas = recibir_int(socket_diego);
	char* buffer_recepcion = malloc(cantidad_lineas * config.MAX_LINEA);
	buffer_recepcion= recibir_char(socket_diego, cantidad_lineas * config.MAX_LINEA);


	if(strcmp("SEG", config.MODO)== 0){
		guardar_proceso_segmentacion_simple(pid ,cantidad_lineas,buffer_recepcion);
		}
		else if(strcmp("TPI", config.MODO)== 0){
		//TODO guardar_proceso_paginas_invertidas(pid ,longitud_paquete,buffer_recepcion);
		}
		else if(strcmp("SPI", config.MODO)== 0){
		//TODO guardar_proceso_segmentacion_paginada(pid ,longitud_paquete, buffer_recepcion);
		}
		else {
			log_error(log_fm9, "Modo de Gestión de Memoria desconocido");
		}

}

void abrir_linea(int socket_cpu){


	int pid = recibir_int(socket_cpu);
	int numero_linea = recibir_int(socket_cpu);


	if(strcmp("SEG", config.MODO)== 0){
			abrir_linea_segmentacion_simple(socket_cpu, pid, numero_linea);
			}
			else if(strcmp("TPI", config.MODO)== 0){
				int resto=0;
				int pagina = numero_linea / CANTIDADLINEASxPAG ;
				resto = numero_linea % CANTIDADLINEASxPAG;
				if (resto >0){
					pagina=pagina+1;
				}				
				int offset = numero_linea * config.MAX_LINEA;
				int tamanio = config.MAX_LINEA;//yoha
				char* buffer = malloc(tamanio); //TODO VALGRIND
				if(solicitarLinea(pid, pagina, offset, tamanio, buffer)){
					send(socket_cpu, config.MAX_LINEA, sizeof(int), MSG_WAITALL);
					send(socket_cpu, buffer, config.MAX_LINEA, MSG_WAITALL);
				}else{
					//send(socket_cpu,FALLO_DE_MEMORIA);
				}
				free(buffer);
			
			//TODO devolver_proceso_paginas_invertidas(socket_diego, pid);
			
			}
			else if(strcmp("SPI", config.MODO)== 0){
			//TODO  devolver_proceso_segmentacion_paginada(socket_diego, pid);
			}
			else {
			//TODO  log_error(log_fm9, "Modo de Gestión de Memoria desconocido");
			}


}

void modificar_linea(int socket_cpu){


	int pid = recibir_int(socket_cpu);
	int numero_linea = recibir_int(socket_cpu);
	int longitud_linea = recibir_int(socket_cpu);
	char* nueva_linea = calloc(1, longitud_linea);
	nueva_linea = recibir_char(socket_cpu, longitud_linea);
	char* linea_tratada = malloc(config.MAX_LINEA);

	memcpy(linea_tratada, nueva_linea, config.MAX_LINEA);

	free(nueva_linea);

	if(strcmp("SEG", config.MODO)== 0){
			modificar_linea_segmentacion_simple(socket_cpu, pid,numero_linea, linea_tratada);
			}
			else if(strcmp("TPI", config.MODO)== 0){
			//TODO devolver_proceso_paginas_invertidas(socket_diego, pid);
			}
			else if(strcmp("SPI", config.MODO)== 0){
			//TODO  devolver_proceso_segmentacion_paginada(socket_diego, pid);
			}
			else {
			//TODO  log_error(log_fm9, "Modo de Gestión de Memoria desconocido");
			}

free(linea_tratada);
}


int recibir_int(int socket){
int buffer;

int resultado_recv = recv(socket, &buffer, sizeof(int), MSG_WAITALL);

if(resultado_recv == -1){

	log_error(log_fm9, "Error en la repcecepción de mensaje");
	return -1;
}else{
return buffer;
}

}

char* recibir_char(int socket, int longitud_paquete){
char* buffer_recepcion = malloc(longitud_paquete);

int resultado = recv(socket, buffer_recepcion, longitud_paquete, MSG_WAITALL);

if(resultado == -1){

	log_error(log_fm9, "Error en la repcecepción de mensaje");
	return "Error";}

	else{

		return buffer_recepcion;
	}


}

//---------------------------------------------------------------------------------------------------------
//SEGMENTACION SIMPLE

void inicializar_memoria_segmentacion_simple(){
	//tabla de segmentos

	tabla_de_segmentos = list_create();
	tabla_administrativa_segmentacion = list_create();

	puntero_memoria_segmentada = malloc(config.TAMANIO);
	bitarray_memoria = bitarray_create(b_m_s,config.TAMANIO / config.MAX_LINEA);

	/* VER PORQUE LE GUSTA ROMPER
	for(int i = 0; i < config.TAMANIO / config.MAX_LINEA; i++){
	bitarray_clean_bit(bitarray_memoria,  i);
	}

*/

	if(puntero_memoria_segmentada == NULL){

		log_error(log_fm9, "No se pudo inicializar la memoria");
	}else{

		log_info(log_fm9, "Inicialización de memoria exitosa");
	}

}

void guardar_proceso_segmentacion_simple(int pid ,int cantidad_lineas, char* buffer_recepcion){


segmento_offset_t* segmento_nuevo = malloc(sizeof(segmento_offset_t));
segmento_tabla_t* entrada_tabla = malloc(sizeof(segmento_tabla_t));

int resultado;
//entrada_administrativa_segmentacion_t* entrada_administrativa = malloc(sizeof(entrada_administrativa_segmentacion_t));




//entrada_administrativa->pid = pid;




	if(list_is_empty(tabla_de_segmentos)){



							//entrada_administrativa->id = pid;

							entrada_tabla->id = pid;
							entrada_tabla->base = 0 ;
							entrada_tabla->limite = cantidad_lineas;


							reservar_bitarray(bitarray_memoria, entrada_tabla->base, entrada_tabla->limite);

							//list_add(tabla_administrativa_segmentacion, entrada_administrativa);
							list_add(tabla_de_segmentos, entrada_tabla);

							memcpy(puntero_memoria_segmentada, buffer_recepcion, cantidad_lineas * config.MAX_LINEA);
	}else{

				if(entra_en_memoria(cantidad_lineas) == 1){


					segmento_nuevo = buscar_segmento_vacio(cantidad_lineas);


					entrada_tabla->id = pid;


					entrada_tabla->base = segmento_nuevo->segmento;

					entrada_tabla->limite = segmento_nuevo->offset;

					memcpy(puntero_memoria_segmentada + entrada_tabla->base * config.MAX_LINEA, buffer_recepcion, entrada_tabla->limite * config.MAX_LINEA);



					reservar_bitarray(bitarray_memoria, entrada_tabla->base, entrada_tabla->limite);


					list_add(tabla_de_segmentos, entrada_tabla);
					//list_add(tabla_administrativa_segmentacion, entrada_administrativa);


					}else{


					}log_error(log_fm9, "Archivo no entra en memoria");
					//TODO 10002 ESPACIO INSUFICIENTE


free(segmento_nuevo);
free(entrada_tabla);

}

}

int entra_en_memoria(int cantidad_lineas){

	int base=0;
	int otra_base = 0;

	while(base < config.TAMANIO && cantidad_lineas < (otra_base - base)){

		base = otra_base;

		while (bitarray_test_bit(bitarray_memoria, base) && base < config.TAMANIO){


		base++;
		}


		otra_base = base;

		while(!(bitarray_test_bit(bitarray_memoria, otra_base)) && otra_base < config.TAMANIO){

		otra_base++;


		}

		}
	if(cantidad_lineas > otra_base - base){

		return 0;
	}else{
		return 1;
	}


}

void abrir_linea_segmentacion_simple(int socket_cpu,int pid,int numero_linea){
	segmento_tabla_t* segmento_linea;

	int resultado;

	bool es_pid(segmento_tabla_t* entrada){
			segmento_tabla_t* segmento;
					return segmento->id == pid;

		}

	segmento_linea = list_find(tabla_de_segmentos, (void*) es_pid);


	char* buffer_envio = malloc(config.MAX_LINEA);

	if(segmento_linea != NULL){

	memcpy(buffer_envio,puntero_memoria_segmentada + segmento_linea->base + (numero_linea * config.MAX_LINEA), config.MAX_LINEA );

	resultado = OK ;

	send(socket_cpu, &resultado, sizeof(int), MSG_WAITALL);
	send(socket_cpu, config.MAX_LINEA, sizeof(int), MSG_WAITALL);
	send(socket_cpu, buffer_envio, config.MAX_LINEA, MSG_WAITALL);

	}else{

   send(socket_cpu, &resultado, sizeof(int), MSG_WAITALL);
   log_error(log_fm9, "No se encuentra la linea en memoria");
   //TODO FALLO DE SEGMENTO

	}
	free(buffer_envio);
}

void modificar_linea_segmentacion_simple(int socket_cpu,int pid,int numero_linea, char* linea_nueva){
	segmento_tabla_t* segmento_linea;
	int resultado;

		bool es_pid(segmento_tabla_t* entrada){
				segmento_tabla_t* segmento;
						return segmento->id == pid;

			}

		segmento_linea = list_find(tabla_de_segmentos, (void*) es_pid);

		if(segmento_linea != NULL){


		memcpy(puntero_memoria_segmentada + segmento_linea->base + (numero_linea * config.MAX_LINEA), linea_nueva ,config.MAX_LINEA );
		resultado = OK ;
		}else{

			log_error(log_fm9, "El segmento no se encuentra en memoria");
			resultado = ERROR ;
		}
	free(linea_nueva);






   //TODO VER TAMAÑO DE TRANSF
   send(socket_cpu, &resultado, config.MAX_LINEA, MSG_WAITALL);

}


segmento_offset_t* obtener_segmento_linea(int pid, int numnero_linea){
	segmento_tabla_t* segmento;
	t_list* lista_busqueda_segmento;

	bool es_id(segmento_tabla_t* entrada_segmento){


							return entrada_segmento->id == id;
						}


	segmento = list_find(tabla_de_segmentos, (void*) es_id);



	return segmento;
}

//--
void liberar_segmento(int pid, int base, int limite){
	int id_segmento;

	entrada_administrativa_segmentacion_t* entrada_admin;


	liberar_bitarray(bitarray_memoria, base, limite);


	bool id_pid(entrada_administrativa_segmentacion_t* entrada){

				return entrada_admin->pid == pid;
			}



	entrada_admin = list_find(tabla_administrativa_segmentacion,(void*)id_pid);
	id_segmento = entrada_admin->id;
	free(entrada_admin);

	bool es_pid(segmento_tabla_t* entrada_segmento){


						return entrada_segmento->id == id_segmento;
					}



	list_remove_by_condition(tabla_de_segmentos,(void*) id_pid);
	list_remove_by_condition(tabla_administrativa_segmentacion, (void*)es_pid);

	liberar_bitarray(bitarray_memoria, base, limite);
}







segmento_offset_t* buscar_segmento_vacio(int cantidad_lineas){

segmento_offset_t* segmento;



int base=0;
int otra_base = 0;

while(base < config.TAMANIO && cantidad_lineas < (otra_base - base)){

	base = otra_base;

	while (bitarray_test_bit(bitarray_memoria, base) && base < config.TAMANIO){


	base++;
	}


	otra_base = base;

	while(!(bitarray_test_bit(bitarray_memoria, otra_base)) && otra_base < config.TAMANIO){

	otra_base++;


	}

	}

segmento->offset = cantidad_lineas;
segmento->segmento = base;

return segmento;
}








//-------------------------------------------------------------------------------------------------------------------------
//PAGINACION INVERTIDA



void inicializar_memoria_paginacion_invertida(){
	//Logear que inicializamos
	MARCO_SIZE = config.TAM_PAGINA;
	MARCOS = config.TAMANIO / MARCO_SIZE;
	CANTIDADLINEASxPAG = MARCO_SIZE / config.MAX_LINEA;
	
	int frames = config.TAMANIO / config.TAM_PAGINA;
	crearMemoriaPrincipalPaginacion(frames, config.TAM_PAGINA);
	
	crearEstructurasAdministrativas(frames);
	
	

}

void crearMemoriaPrincipalPaginacion(int frames,int tamanio_pagina){

	 int resto=0;

    //Calculo cuanto Marcos me va a llevar la estructura administrativa en si.
    ESTRUCTURA_ADM_SIZE= MARCOS*sizeof(t_tablaPaginaInvertida)/MARCO_SIZE;
    resto = MARCOS*sizeof(t_tablaPaginaInvertida)%MARCO_SIZE;

    // si el resto es cero necesito una cantidad justa de marcos, sino voy a necesitar uno mas.
    if (resto >0){
        ESTRUCTURA_ADM_CANT_FRAMES=ESTRUCTURA_ADM_SIZE+1;
    }else{
            ESTRUCTURA_ADM_CANT_FRAMES = ESTRUCTURA_ADM_SIZE;
        }


    //int cant;
    //ceil(ESTRUCTURA_ADM_SIZE);//ESTRUCTURA_ADM_CANT_FRAMES
    int Cant_Total = MARCOS*MARCO_SIZE;
    memoria = malloc(sizeof(t_memoria_principal)+ESTRUCTURA_ADM_SIZE);
    memoria->memoria=malloc(Cant_Total);
    memoria->estructura_administrativa= memoria->memoria; // hito en la humanida'
    memoria->frames= memoria->memoria+(ESTRUCTURA_ADM_CANT_FRAMES*MARCO_SIZE);


}

int crearEstructurasAdministrativas() {

    int numeroDeFrame;
    for (numeroDeFrame = 0; numeroDeFrame < ESTRUCTURA_ADM_CANT_FRAMES;
            numeroDeFrame++) {
        memoria->estructura_administrativa[numeroDeFrame].frame = numeroDeFrame;
        memoria->estructura_administrativa[numeroDeFrame].nroPag = FRAME_ADM;
        memoria->estructura_administrativa[numeroDeFrame].pid = FRAME_ADM;
    }

    //inicializar frame
    for (numeroDeFrame=0; numeroDeFrame < MARCOS; numeroDeFrame++) {
        memoria->estructura_administrativa[numeroDeFrame].frame = numeroDeFrame;
        memoria->estructura_administrativa[numeroDeFrame].nroPag = PAGINALIBRE;
        memoria->estructura_administrativa[numeroDeFrame].pid = PAGINALIBRE;
    }

    return 0;
}


//Leer 
int solicitarLinea(int unPid, int pagina, int offset, int tamanio, char *buffer) {

		int frame;
		frame = buscarFrame(unPid, pagina);

		if (frame > 0) {
			int espacioEnMemoria = calcularPosicion(frame);
			memcpy(buffer, memoria->frames + espacioEnMemoria + offset, tamanio);
		} else {
			log_info(log_fm9, "El pid PID %i no tiene la pagina asignada PG %i  ", unPid, pagina);
			return 0;
		}

	return 1;
}


int hash(int unPid, int pagina) {
	printf("pid %i, pagina %i\n",unPid,pagina);
	int frame;   //TODO funcion mas pro loco
	frame = (((unPid * MARCO_SIZE)+(pagina*MARCOS))/MARCO_SIZE);
	if (frame > MARCOS){
		frame = frame - MARCOS;
	}
	printf("framede hash %i\n",frame);
    return frame;
}

int buscarFrame(int unPid, int pagina) {
	int frame = hash(unPid, pagina);
	int frameAux = frame;
	int noEncontre = 1;
	while (noEncontre) {
		if (memoria->estructura_administrativa[frame].pid == unPid && memoria->estructura_administrativa[frame].nroPag == pagina) {
			return frame;
		} else {
			frame++;
			if (frame > MARCOS) {
				frame = 0;
			}
			if (frame == frameAux) {
				noEncontre = 0;
			}
		}
	}
	return -1;
}


//ESCRIBIR
int almacenarLinea(int unPid, int pagina, int offset, int tamanio, char * buffer) {

	printf("almacenar Linea \n");
    int frame = buscarFrame(unPid,pagina);
    int espacioEnMemoria = calcularPosicion(frame);
    printf("BUSCO FRAME: %i\n",frame);

	if (frame > 0) {

		//Escribo en memoria
		memcpy(memoria->frames + espacioEnMemoria + offset, buffer, tamanio);
		return 1;

	} else {
//		log_info(memLog, "El pid PID %i no tiene la pagina asignada PG %i  ", unPid, pagina);
		printf("El pid PID %i no tiene la pagina asignada PG %i  ", unPid, pagina);
		return 0;

	}
}


int calcularPosicion(int frame){

	int posicionFrame = (frame + ESTRUCTURA_ADM_CANT_FRAMES) *MARCO_SIZE;
	return posicionFrame;

}


//Pedido de Apertura de Archivo por parte de DMA 
void crearPid(int unPid, int paginas) {
//devuelvo el handshake cuando este asigno las paginas
	asignarPaginasIniciales(unPid, paginas);
	
}

int asignarPaginasIniciales(int unPid, int paginas) {
	int ok = 1;
	int encontre_frame = 0;
	int cont_frame = 0;

	//seguro hay q hacer hs de nuevo?
	//TODO AGREGAR ESTRUCTURAS PID CANT PAGINAS
	int pag,frame;

	for (pag = 0; pag < paginas; pag++){
		    // me ofrece un frame
			frame = hash(unPid,pag);
			encontre_frame=0;
			cont_frame=0;
			// me fijo si ese frame esta disponible

			if(memoria->estructura_administrativa[frame].pid== PAGINALIBRE && memoria->estructura_administrativa[frame].nroPag== PAGINALIBRE){
				memoria->estructura_administrativa[frame].pid= unPid;
			    memoria->estructura_administrativa[frame].nroPag=pag;
			    //TODO se debera incluir la pagina a la lista de paginas del proceso
			   
			    printf("PID: %d | PG: %d | FRAME %d \n",unPid,pag,frame);
			   	encontre_frame = 1;
			}else{
				// ciclo hasta encontrar un frame libre
				while(encontre_frame < 1) { //ciclo infinite
					if (cont_frame > MARCOS){
						//TODO Devuelvo que no tengo espacio.
						ok=-1;
						//TODO corto el flujo de procesamiento
						encontre_frame = 1; //pq no tenia este ajaj
					}
				if (memoria->estructura_administrativa[frame + cont_frame].pid == PAGINALIBRE && memoria->estructura_administrativa[frame + cont_frame].nroPag == PAGINALIBRE) {
					memoria->estructura_administrativa[frame + cont_frame].pid = unPid;
					memoria->estructura_administrativa[frame + cont_frame].nroPag = pag;
					encontre_frame = 1;//TODO ACA
					
					printf("PID: %d | PG: %d | FRAME %d \n", unPid, pag, frame+cont_frame);
				} else {
						cont_frame++;
					}
				}
			}
	}
//TODO BERU DEFINIR INTERFAZ DE MENSAJE
		/*	if (ok) {
					enviarHS(socket, INI_PROG_EXITOSO);
				} else {
					enviarHS(socket, INI_PROG_FALLIDO);
				}*/

	return 1;
}

//Elimino todas las paginas de un pid
int eliminarPid(int pid) {
	int frameNro,procesoNro;
	for (frameNro = 0; frameNro < MARCOS; frameNro++) {
		//verificar que el frame este vacio
		if (memoria->estructura_administrativa[frameNro].pid == pid) {//[i]
			memoria->estructura_administrativa[frameNro].pid = PAGINALIBRE;
			memoria->estructura_administrativa[frameNro].nroPag = PAGINALIBRE;
			memoria->estructura_administrativa[frameNro].frame = PAGINALIBRE; //YOHANNA TODO PROBAR
			//TODO ELIMINAR ESTRUCTURA DE PROCESOS ACTIVOS DEFE
		}
	}

	return 1;
}


//Cuando cpu envia a eliminar una pagina del heap.
void eliminarPagina(int pid, int nroPag){

	int i;
	int encontro =0;


		for (i = 0; i < MARCOS; i++) {
			//verificar que el frame este vacio
			if (memoria->estructura_administrativa[i].pid == pid && memoria->estructura_administrativa[i].nroPag== nroPag) {

				memoria->estructura_administrativa[i].pid = PAGINALIBRE;
				memoria->estructura_administrativa[i].nroPag = PAGINALIBRE;
				memoria->estructura_administrativa[i].frame = PAGINALIBRE; 
				encontro = 1;
			}
		}

		/*
		if (encontro != 1 ){
				enviarHS(socket,ELIMINAR_PAGINA_MEM_FALLIDO);//structHilo->fd
			//	return -1; // TODO asignar define
		}else{
		    	enviarHS(socket,ELIMINAR_PAGINA_MEM_EXITO);
		}*/
}


/*
void inicializar_ta---------------------------------------------------------------------------
//SEbla_de_paginas(int numero_lineas_memoria){
	segmento_tabla_t* entrada_vacia = malloc(sizeof(segmento_tabla_t));

	entrada_vacia->base=0;
	entrada_vacia->limite=0;



	for(int i=0; i < numero_lineas_memoria; i++){
		int j= 0;

		entrada_vacia->id = j;
		list_add(tabla_de_segmentos, entrada_vacia);
		j++;

	}

free(entrada_vacia);

}
*/



//-------------------------------------------------------------------------------------------------------------------
//SEGMENTACION PAGINADA

void inicializar_memoria_segmentacion_paginada(){



}

//-------------------------------------------------------------------------------------------------------------------
//BITARRAY

void reservar_bitarray(t_bitarray* bitarray_memoria_segmentada,int base,int limite){

	for(int i= 0; i <= limite; i++){

		bitarray_set_bit(bitarray_memoria_segmentada, i);

	}


}

void liberar_bitarray(t_bitarray* bitarray_memoria_segmentada,int base,int limite){

	for(int i= 0; i <= limite; i++){

		bitarray_clean_bit(bitarray_memoria_segmentada, i);

	}

}




