#include "envio_recepcion.h"

static void* recibir_mensaje(int socket_cliente);
static void* generar_nodo_mensaje(int socket, int cod_op, bool EsCorrelativo);

static int enviar_confirmacion(int socket, int mensaje);
static int obtener_cod_op(t_buffer* buffer, int* tiempo);

static void cargar_envios_mensajes(int cod_op, int id_suscriptor);
static void cargar_envios_suscriptores(int cod_op, int id_mensaje);
static void cargar_envio(int cod_op, int id_mensaje, int id_suscriptor);

void eliminar_suscriptor_tiempo(int tiempo, int id_sub, int cod_op);
static void eliminar_sub_tiempo(void* _datos);



int tratar_mensaje(int socket, int cod_op, bool esCorrelativo){

	t_mensaje* mensaje = generar_nodo_mensaje(socket, cod_op, esCorrelativo);
	if (mensaje == NULL) return EXIT_FAILURE;

	guardar_mensaje(mensaje, cod_op);

	char* log_mensaje = string_from_format("Llego un mensaje a la cola %s", cod_opToString(cod_op));
	logear_mensaje(log_mensaje);
	free(log_mensaje);

	enviar_confirmacion(socket, mensaje->id);

	cargar_envios_suscriptores(cod_op, mensaje->id);

	close(socket);

	return EXIT_SUCCESS;
}


int tratar_suscriptor(int socket){

	t_buffer* mensaje = recibir_mensaje(socket);
	if (mensaje == NULL) { enviar_confirmacion(socket, -1); return EXIT_FAILURE; }

	int tiempo, cod_op = obtener_cod_op(mensaje, &tiempo);

	t_suscriptor* suscriptor = crear_nodo_suscriptor(cod_op, socket);

	//printf("cod_op = %d, socket = %d, suscriptor = %p\n", suscriptor->cod_op, suscriptor->socket, suscriptor);

	guardar_suscriptor(suscriptor, cod_op);
	if(tiempo != -1){ eliminar_suscriptor_tiempo(tiempo, suscriptor->id, cod_op); }

	char* log_mensaje = string_from_format("Un proceso se suscribio a la cola %s", cod_opToString(cod_op));
	logear_mensaje(log_mensaje);
	free(log_mensaje);

	enviar_confirmacion(suscriptor->socket, suscriptor->id);

	cargar_envios_mensajes(cod_op, suscriptor->id);

	return EXIT_SUCCESS;
}


int tratar_reconexion(int socket){

	int id_suscriptor, cola_suscrito, s;

	s = recv(socket, &id_suscriptor, sizeof(uint32_t), 0);
	if (s <= 0) perror("[ENVIO_RECEPCION_EXTEND.C] RECV ERROR");

	s = recv(socket, &cola_suscrito, sizeof(uint32_t), 0);
	if (s <= 0) perror("[ENVIO_RECEPCION_EXTEND.C] RECV ERROR");

	s = reconectar_suscriptor(id_suscriptor, cola_suscrito);
	if (s == EXIT_FAILURE) { printf("No se encontro al suscriptor"); enviar_confirmacion(socket, false); return EXIT_FAILURE; }

	enviar_confirmacion(socket, true);//analizar que devuelve la confirmacion un bool o el id del suscriptor

	return EXIT_SUCCESS;
}


static void* generar_nodo_mensaje(int socket, int cod_op, bool EsCorrelativo){

	int s, id_correlativo;

	if (EsCorrelativo) {

		s = recv(socket, &id_correlativo, sizeof(uint32_t), 0);
		if (s < 0) { perror("[ENVIO_RECEPCION_EXTEND.C] RECV ERROR"); return NULL; }

	} else id_correlativo = -1;

	t_buffer* mensaje = recibir_mensaje(socket);
	if (mensaje == NULL) return NULL;

	void* n_mensaje = crear_nodo_mensaje(cod_op, id_correlativo, mensaje);

	//printf("Cod_op = %d, Id_correlativo = %d, Mensaje_size = %d\n", n_mensaje->cod_op, n_mensaje->id_correlativo, n_mensaje->mensaje->size);

	return n_mensaje;
}


static int enviar_confirmacion(int socket, int mensaje){

	int s;
	void* mensaje_enviar = malloc(sizeof(uint32_t));

	memcpy(mensaje_enviar, &mensaje, sizeof(uint32_t));

	s = send(socket, mensaje_enviar, sizeof(uint32_t), MSG_NOSIGNAL);
	if (s < 0) { perror("[ENVIO_RECEPCION_EXTEND.C]SEND ERROR"); free(mensaje_enviar); return EXIT_FAILURE; }

	free(mensaje_enviar);

	return EXIT_SUCCESS;
}



static void* recibir_mensaje(int cliente_fd){

	int s;
	t_buffer* buffer = malloc(sizeof(t_buffer));

	s = recv(cliente_fd, &(buffer->size), sizeof(uint32_t), 0);
	if (s <= 0) { free(buffer); perror("[ENVIO_RECEPCION_EXTEND.C] RECV ERROR"); return NULL; }

	buffer->stream = malloc(buffer->size); //*puntero -> posicion_tu_bloque_memoria;

	s = recv(cliente_fd, buffer->stream, buffer->size, 0);
	if (s <= 0) { free(buffer); free(buffer->stream); perror("[ENVIO_RECEPCION_EXTEND.C] RECV ERROR"); return NULL; }

	return buffer;
}


static int obtener_cod_op(t_buffer* buffer, int* tiempo){

	int cod_op, offset = 0;

	memcpy(&cod_op, buffer->stream, sizeof(uint32_t));
	offset += sizeof(uint32_t);

	memcpy(tiempo, buffer->stream + offset, sizeof(uint32_t));
	offset += sizeof(uint32_t);

	free(buffer->stream);
	free(buffer);

	return cod_op;
}



//====================================================================



static void cargar_envios_mensajes(int cod_op, int id_suscriptor){

	t_list* lista_mensajes = obtener_lista_ids_mensajes(cod_op);

	int id_mensaje;

	for (int i = 0; i < list_size(lista_mensajes); i++){

		id_mensaje = *((int*)list_get(lista_mensajes, i));

		cargar_envio(cod_op, id_mensaje, id_suscriptor);
	}

	list_destroy_and_destroy_elements(lista_mensajes, free);
}


static void cargar_envios_suscriptores(int cod_op, int id_mensaje){

	t_list* lista_subs = obtener_lista_ids_suscriptores(cod_op);

	int id_suscriptor;

	for (int i = 0; i < list_size(lista_subs); i++){

		id_suscriptor = *((int*)list_get(lista_subs, i));

		cargar_envio(cod_op, id_mensaje, id_suscriptor);
	}

	list_destroy_and_destroy_elements(lista_subs, free);
}


static void cargar_envio(int cod_op, int id_mensaje, int id_suscriptor){

	t_envio* envio = crear_nodo_envio(cod_op, id_mensaje, id_suscriptor);

	pthread_mutex_lock(&mutex_cola_envios);

	queue_push(cola_envios, envio);

	pthread_cond_signal(&cond_cola_envios);

	pthread_mutex_unlock(&mutex_cola_envios);
}



//===================================================================================



static void eliminar_sub_tiempo(void* _datos){

	t_datos* datos = _datos;

	sleep(datos->tiempo);

	eliminar_suscriptor_id(datos->cod_op, datos->id_suscriptor);

	free(datos);
}


void eliminar_suscriptor_tiempo(int tiempo, int id_sub, int cod_op){

	t_datos* datos = crear_nodo_datos(cod_op, id_sub, tiempo);

	pthread_t tid;

	pthread_create(&tid, NULL, (void*)eliminar_sub_tiempo, datos);
	pthread_detach(tid);
}


