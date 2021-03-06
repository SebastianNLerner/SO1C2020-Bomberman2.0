#include "gameboy.h"


int main(int argc,char* argv[]){

	inicializar_archivos();

	obtener_direcciones_envio(argv[1]);

	int bytes_enviar;
	void* mensaje_enviar = armar_mensaje_enviar(argv+1, &bytes_enviar);

	int conexion = crear_conexion(IP_SERVER, PUERTO_SERVER);

	void cerrar_socket(){
		liberar_conexion(conexion);
		raise(SIGTERM);
	}

	signal(SIGINT, (void*)cerrar_socket);

	enviar_mensaje(mensaje_enviar, bytes_enviar, conexion);

	generar_log_suscripcion(argv+1);

	esperando_respuestas(conexion, argv[1]);

	terminar_programa(conexion, LOGGER, CONFIG);

	return 0;
}

