/*
 * broker.h
 *
 *  Created on: 26 abr. 2020
 *      Author: utnso
 */

#ifndef TEAM_H_
#define TEAM_H_

typedef struct{
	int posx;
	int posy;
}t_posicion;

typedef struct{
	pthread_mutex_t *queueMutex;
	pthread_cond_t *queueCond;
}t_semaforo;


typedef struct{
	t_posicion* posicion;
	t_semaforo* semaforo;
	char* mensaje; //temporal
	char* algoritmo_de_planificacion;
	char** objetivo;
	char** pokemones;
}t_entrenador;

void planificacion();

void leer_archivo_configuracion();


#endif /* BROKER_H_ */
