#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <signal.h>
#include <stdbool.h>
#include <time.h>
#include <linux/unistd.h>
#include <errno.h>

//Numero maximo de clientes que se pueden atender a la vez
#define MAX_CLIENTES 20

//Numero de tecnicos, responsables de reparaciones y encargados
#define NUM_TECNICOS 2
#define NUM_RESPONSABLES 2
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
//estructura que guarda la informacion del cliente
typedef struct {
	char *id;		//no seria mejor poner el id como int?
	int atendido;
	char *tipo;
	int prioridad;
	bool solicitud;
}Cliente;

//estructura que guarda la informacion del trabajador
typedef struct { 
	char *id;
	int clientesAtendidos;
	char *tipo; //Tecnico, responsable o encargado
}Trabajador;

/*Fichero de log*/
FILE *logFile;

//funcion para generar un cliente
Cliente generar_cliente(){
	static int id = 0;
	Cliente c;
	c.id = ++id;
	c.atendido = false;

	return c;
}

//funcion para atender a un cliente
void atender_cliente(){
	printf("Se esta atendiendo al cliente %d (%c)\n, ");
	sleep(1);	//se simula el tiempo de atencion
}

int main(int argc, char* argv[]) {



	return 0;
}


