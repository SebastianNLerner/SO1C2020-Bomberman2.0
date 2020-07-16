#ifndef SUSCRIPCION_H_
#define SUSCRIPCION_H_

#include <commons/collections/list.h>
#include <pthread.h>
#include <signal.h>
#include "variables_globales.h"


typedef struct{
	int id;
	int cod_op;
	t_buffer* buffer;
}t_mensajeTeam;


int mensajeActual;

void iniciar_servidor(void);
char* leer_get_pokemon(int);
char* leer_mensaje_getPokemon(t_buffer*);
void iniciar_suscripciones(int cola0, int cola1, int cola2);
void finalizar_suscripciones(void);

#endif /* SUSCRIPCION_H_ */
