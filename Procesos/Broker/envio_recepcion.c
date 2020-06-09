#include "envio_recepcion.h"

#define cant_threads 3
pthread_t THREADS[cant_threads];
pthread_mutex_t mutex_cola = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t condicional= PTHREAD_COND_INITIALIZER;

t_queue* cola_clientes;

void* funcion_thread();

void* iniciar_servidor(void){

	int flag = 1;
	cola_clientes = queue_create();
	int socket_servidor;

	void _tratar_senal(){
		flag = 0;
		for(int i = 0; i < cant_threads; i++)
			pthread_join(THREADS[i], NULL);
	}

	signal(SIGUSR2, (void*)_tratar_senal);

    struct addrinfo hints, *servinfo, *p;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    getaddrinfo(IP_SERVER, PUERTO_SERVER, &hints, &servinfo);

    int status;

    for (p=servinfo; p != NULL; p = p->ai_next)
    {
        socket_servidor = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if(socket_servidor < 0){perror("SOCKET ERROR"); continue; }

        status = bind(socket_servidor, p->ai_addr, p->ai_addrlen);
        if(status < 0){perror("BIND ERROR"); close(socket_servidor); continue;}

        break;
    }

    status = listen(socket_servidor, SOMAXCONN);
    freeaddrinfo(servinfo);
    if(status < 0){perror("LISTEN ERROR"); raise(SIGINT);}

    SOCKET_SERVER = &socket_servidor;

    for(int i = 0; i < cant_threads; i++){
    	pthread_create(&THREADS[i], NULL, funcion_thread, NULL);
    }

    while(flag)
    	esperar_cliente(socket_servidor);

	close(socket_servidor);
    pthread_exit(0);
}


void esperar_cliente(int socket_servidor){

	struct sockaddr_in dir_cliente;

	socklen_t tam_direccion = sizeof(struct sockaddr_in);

	int socket_cliente;

	socket_cliente = accept(socket_servidor, (void*) &dir_cliente, &tam_direccion);
	if(socket_cliente < 0){perror("[envio_recepcion.c] FALLO ACCEPT"); return;}

	int* p_socket = malloc(sizeof(int));
	*p_socket = socket_cliente;

	pthread_mutex_lock(&mutex_cola);

		queue_push(cola_clientes, p_socket);
		pthread_cond_signal(&condicional);

	pthread_mutex_unlock(&mutex_cola);
}


void* funcion_thread(){
	while(1){
		pthread_mutex_lock(&mutex_cola);

			if(queue_is_empty(cola_clientes))
				pthread_cond_wait(&condicional, &mutex_cola);

			server_client(queue_pop(cola_clientes));

		pthread_mutex_unlock(&mutex_cola);
	}
}


void server_client(void* p_socket){

	int cod_op, socket = *((int*)p_socket);
	free(p_socket);

	int status = recv(socket, &cod_op, sizeof(uint32_t), 0);
	if(status < 0){ perror("[envio_recepcion.c] FALLO RECV"); cod_op = -1;}

	process_request(socket, cod_op);
}


void process_request(int cliente_fd, int cod_op){

	t_mensaje* mensaje;

	switch(cod_op){

		case NEW_POKEMON:
		case CATCH_POKEMON:
		case GET_POKEMON:

			if((mensaje = generar_nodo_mensaje(cliente_fd, cod_op, false)) == NULL)
				break;

			tratar_mensaje(cliente_fd, mensaje, cod_op);

			break;

		case APPEARED_POKEMON:
		case CAUGHT_POKEMON:
		case LOCALIZED_POKEMON:

			if((mensaje = generar_nodo_mensaje(cliente_fd, cod_op, true)) == NULL)
				break;

			tratar_mensaje(cliente_fd, mensaje, cod_op);

			break;

        case SUSCRIPTOR:

        	tratar_suscriptor(cliente_fd);

        	break;

		case -1:

			printf("NO SE RECIBIO EL MENSAJE CORRECTAMENTE\n");
			pthread_exit(NULL);

		default:

			printf("CODIGO DE OPERACION INVALIDO\n");
			break;
		}

}


void* tratar_mensaje(int socket, t_mensaje* mensaje, int cod_op){

	guardar_mensaje(mensaje, cod_op);

	informe_lista_mensajes();

	enviar_confirmacion(socket, mensaje->id);

	close(socket);

	//enviar_mensaje_suscriptores(mensaje);

	return EXIT_SUCCESS;
}


void* tratar_suscriptor(int socket){

	t_buffer* mensaje;

	if((mensaje = recibir_mensaje(socket)) == NULL){
		enviar_confirmacion(socket, false);
		return NULL;
	}
	int tiempo;
	int cod_op = obtener_cod_op(mensaje, &tiempo);

	t_suscriptor* suscriptor = nodo_suscriptor(socket);

	enviar_confirmacion(suscriptor->socket, true);

	guardar_suscriptor(suscriptor, cod_op);

	informe_lista_subs();

	//enviar_mensajes_suscriptor(suscriptor, cod_op);

	return EXIT_SUCCESS;
}


void leer_new_pokemon(int cliente_fd){

	char* pokemon;
	int size, posx, posy, cantidad;
	recv(cliente_fd, &size, sizeof(uint32_t), 0);
	recv(cliente_fd, &size, sizeof(uint32_t), 0);
	pokemon = malloc(sizeof(size));
	recv(cliente_fd, pokemon, size, 0);
	recv(cliente_fd, &posx, sizeof(uint32_t), 0);
	recv(cliente_fd, &posy, sizeof(uint32_t), 0);
	recv(cliente_fd, &cantidad, sizeof(uint32_t), 0);

	printf("pokemon = %s, posx = %d, posy = %d, cantidad = %d\n", pokemon, posx, posy, cantidad);
}


void leer_suscripcion(int cliente_fd){
	uint32_t size, cod_op, tiempo;

	recv(cliente_fd, &size, sizeof(uint32_t), 0);

	void* stream = malloc(size);
	recv(cliente_fd, stream, size, 0);

	int offset = 0;
	memcpy(&cod_op, stream + offset, sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(&tiempo, stream + offset, sizeof(uint32_t));
	offset += sizeof(uint32_t);

	printf("size = %d, cod_op = %d, tiempo = %d\n", size, cod_op, tiempo);
}


