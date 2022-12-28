#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#define NUM_TECNICOS 2
#define NUM_RESPONSABLE_REPARACIONES 2
#define NUM_ENCARGADO 1

void* hilo_creado(void *arg){
	printf("El hilo se ha creado correctamente\n");
}


int main(int argc, char* argv[]) {
	pthread_t tecnico, responsable_reparaciones, encargado, clientes_app, clientes_red;
	int resultado_tecnico= pthread_create(&tecnico, NULL, hilo_creado, NULL);
	if(resultado_tecnico!=0){
		//ha ocurrido un erro al crear el hilo
		perror("Error al creal el hilo tecnico\n");
		return 1;
	}
	int resultado_responsable_reparaciones= pthread_create(&responsable_reparaciones, NULL, hilo_creado, NULL);
	if(resultado_responsable_reparaciones!=0){
		//ha ocurrido un erro al crear el hilo
		perror("Error al creal el hilo tecnico\n");
		return 1;
	}
	int resultado_encargado= pthread_create(&encargado, NULL, hilo_creado, NULL);
	if(resultado_encargado!=0){
		//ha ocurrido un erro al crear el hilo
		perror("Error al creal el hilo tecnico\n");
		return 1;
	}
	return 0;
}


