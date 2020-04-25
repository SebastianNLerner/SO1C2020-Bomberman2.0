
#include "utils_gameboy.h"


t_paquete* armar_paquete(char** datos){ //datos = tipo_mensaje + [datos_mensaje]*

	int t_mensaje = tipo_mensaje(*datos);
	char** datos_serializar = datos + 1;

	t_paquete* paquete = malloc(sizeof(t_paquete));

	paquete -> codigo_operacion = t_mensaje;
	paquete -> buffer = malloc(sizeof(t_buffer));
	paquete -> buffer -> size = obtener_tamanio(datos_serializar) + cant_elementos(datos_serializar) * sizeof(uint32_t);
	
	//printf("[armarpaquete] tamanio de todos los datos: %d\n", paquete -> buffer -> size);
	void* stream = malloc(paquete -> buffer -> size);
	int offset = 0;

	while(*(datos_serializar) != NULL){

		int longitud_string = strlen(*datos_serializar) + 1;  // incluimos el '\0'
		
		memcpy(stream + offset, &longitud_string, sizeof(uint32_t));
		offset += sizeof(uint32_t);

		memcpy(stream + offset , *datos_serializar, longitud_string);
		offset += longitud_string;

		datos_serializar++;
	}

	paquete -> buffer -> stream = stream;
	
	return paquete;
}


void enviar_mensaje(t_paquete* paquete_enviar, int socket_cliente){
	int bytes;

	void* stream_enviar = serializar_paquete(paquete_enviar, &bytes);

	if(send(socket_cliente, stream_enviar, bytes, 0) == -1){
		printf("Error al enviar el mensaje\n");
	}

	free(paquete_enviar -> buffer -> stream);
	free(paquete_enviar -> buffer);
	free(paquete_enviar);
	free(stream_enviar);
}


void leer_mensaje(void *stream){
	int t_mensaje;
	int offset = 0;
	memcpy(&t_mensaje, stream + offset, sizeof(uint32_t));
	offset += sizeof(uint32_t);

	printf("[leer_mensaje] t_mensaje = %d\n", t_mensaje);

	int size;
	memcpy(&size, stream + offset, sizeof(uint32_t));
	offset += sizeof(uint32_t);

	printf("[leer_mensaje] size = %d\n", size);

	while(offset < size + 2 * sizeof(uint32_t) ){
		int tamanio=0;

		memcpy(&tamanio, stream + offset, sizeof(uint32_t));
		offset += sizeof(uint32_t);

		char* palabra = malloc(tamanio);
		memcpy(palabra, stream + offset, tamanio);
		offset += tamanio;

		//printf("[leer_mensaje] pointer = %p\n", stream);
		printf("[leer_mensaje] palabra: %s, tamanño = %d, offset = %d\n", palabra, tamanio, offset);
		free(palabra);
	}
	printf("fin\n");
}

/*
void* algo(char** argumentos, int* bytes){

	if(string_equals_ignore_case(*(argumentos + 1), "SUSCRIPTOR")){
		//ver que pasa en este caso
		

	} else{
		//t_paquete* paquete = armar_paquete(argumentos+2);
		//return = serializar_paquete(paquete, bytes);

	}
}*/


