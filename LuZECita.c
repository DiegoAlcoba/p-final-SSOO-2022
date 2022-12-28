#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <stdbool.h>
#include <time.h>
#include <linux/unistd.h>
#include <errno.h>

#define NTECNICOS 2.0
#define NRESPONSABLES 2.0
#define NENCARGADOS 1.0

/*Variables globales*/
pthread_mutex_t semaforoFichero;
pthread_mutex_t semaforoColaClientes;
pthread_mutex_t semaforoSolicitudes;

pthread_t *arrayHilosClientes; //Array de hilo-cliente

int nClientesApp; //Contador clientes app
int nClientesRed; //Contador clientes red
int nSolicitudesDomiciliarias; //N solicitudes domiciliarias (4 para que el responsable viaje)

/*Arrays dinámicos que almacenan a los clientes (estructura)*/
struct cliente *arrayClientes;
struct cliente *arrayClientesSolicitudes; //Clientes a los que tiene que visitar el responsable

/*Estructuras*/
struct cliente {
	char *id;
	int atendido;
	char *tipo;
	int prioridad;
	int solicitud; //Quizás mejor un booleano? Solicitud sí/no
};

struct trabajador { 
	char *id;
	int clientesAtendidos;
	char *tipo; //Tecnico, responsable o encargado
};

/*Fichero de log*/
FILE *logFile;

int main(int argc, char* argv[]) {



	return 0;
}


