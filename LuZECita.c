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
	char *id;		//no seria mejor poner el id como int? NO PORQUE EL IDENTIFICADOR ES CLIAPP_1
	int atendido; //0 si no esta atendido 1 si si 
	char *tipo;
	int prioridad;
	bool solicitud;
}Cliente;

//estructura que guarda la informacion del trabajador
typedef struct { 
	char *id;
	int clientesAtendidos;
	char *tipo; //Tecnico, responsable o encargado T R oE
	int libre; //0 si esta libre 1 si esta ocupado
}Trabajador;

/*Fichero de log*/
FILE *logFile;

//funcion para crear un tecnico
Trabajador generar_Tecnico(){
	//Tecnico 1
		static int id1=0;
		Trabajador tecnico_1;
		tecnico_1.id=++id1;
		tecnico_1.clientesAtendidos=0;
		tecnico_1.tipo=T;
		tecnico_1.libre=0;
	//Tecnico 2
		static int id2=0;
		Trabajador tecnico_2;
		tecnico_2.id=++id2;	
		tecnico_2.clientesAtendidos=0;
		tecnico_2.tipo=T;
		tecnico_2.libre=0;
}

//funcion para generar un cliente
Cliente generar_cliente(){
	static int id = 0;
	Cliente c;
	c.id = ++id;
	c.atendido = 0;//0 si no esta atendido 1 sis si

	return c;
}
/*funcion del encargado es decir atender a los clientes con problemas en la app*/
void atender_cliente_app(){
	//IMPORTANTE:::::AÑADIR LO DE LA PRIORIDAD
	//Se comprueba si el tenico esta libre
	if(tecnico_1.libre==0){
		printf("Se va a atender a un cliente con problemas en la app");
		printf("Bienvenido cliente le esta atendiendo el tecnico_1");
		tecnico1.libre=1;
		//Compruebo que el cliente que atiendo sea de app
		if(cliente.tipo==a){
			printf("El cliente se ha atendido");
			//Cambio su estado a atendido
			cliente.atendido=1;
			++tecnico_1.clientesAtendidos;
			//Si hay 5 descanso 5 seg
			if(tecnico_1.clientesAtendidos==5){
				tecnico_1.libre=1;
				sleep(5);
			}
		
		}
	}else if (tecnico_2.libre==0){
		printf("Se va a atender a un cliente con problemas en la app");
		printf("Bienvenido cliente le esta atendiendo el tecnico_2");
		tecnico2.libre=1;
		//Compruebo que el cliente que atiendo sea de app
		if(cliente.tipo==a){
			printf("El cliente se ha atendido");
			//Cambio su estado a atendido
			cliente.atendido=1;
			++tecnico_2.clientesAtendidos;
			//Si hay 5 descanso 5 seg
			if(tecnico_2.clientesAtendidos==5){
				tecnico_2.libre=1;
				sleep(5);
			}
		
		}
	}else{}

//IMPORTANTE::::::AÑADIR CONDICION DE QUE SI ESTAN LOS DOS OCUPADOS ATENDERLOS ENCARGADO
}
//funcion para atender a un cliente
void atender_cliente(){
	printf("Se esta atendiendo al cliente %d (%c)\n, ");
	sleep(1);	//se simula el tiempo de atencion
	}

int main(int argc, char* argv[]) {



	return 0;
}


