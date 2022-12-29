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
#define NUM_ENCARGADOS 1.0

/*Variables globales*/
pthread_mutex_t semaforoFichero;
pthread_mutex_t semaforoColaClientes;
pthread_mutex_t semaforoSolicitudes;

pthread_t *arrayHilosClientes; //Array de hilo-cliente

int nClientesApp; //Contador clientes app
int nClientesRed; //Contador clientes red
int nClientes;	//Número total de clientes
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

<<<<<<< HEAD
/*Función que calcula números aleatorios*/
int calculaAleatorios(int min, int max) {
	srand(time(NULL));
	
	return rand() % (max - min + 1) + min;
}

/*Función que escribe los mensajes en log*/
void writeLogMessage(char *id, char *msg) {
	//Se calcula la hora actual
	time_t now = time(0);
	struct tm *tlocal = localtime(&now);
	char stnow[25];
	strftime(stnow, 25, "%d/%m/%y  %H:%M:%S", tlocal);

	//Escribimos en el log
	logFile = fopen(logFileName, "a");
	fprintf(logFile, "[%s] %s: %s\n", stnow, id, msg);
	fclose(logFile);
}

=======
//IMPORTANTE::::::AÑADIR CONDICION DE QUE SI ESTAN LOS DOS OCUPADOS ATENDERLOS ENCARGADO
}
>>>>>>> 689c80b61b4a59839bc4bdd19f6f4fc482f6ed80
//funcion para atender a un cliente
void atender_cliente(){
	printf("Se esta atendiendo al cliente %d (%c)\n, ");
	sleep(1);	//se simula el tiempo de atencion
	}

/*Función que finaliza el programa al recibir la señal*/
void finalizarPrograma (int signal) {

	//Antes de que finalice se debe terminar de atender a todos los clientes en cola

	printf("\nAdiós!\n");

	exit(0);
}

int main(int argc, char* argv[]) {
	
	/* DECLARACIONES DE RECURSOS */
	pthread_t tecnico1, tecnico2, responsable1, responsable2, encargado, atencionDomiciliaria;

	arrayHilosClientes = (pthread_t *) malloc (nClientes * sizeOf(struct cliente *)); //Array dinámico de hilos de clientes
	arrayClientes = (struct cliente *) malloc (nCliente * sizeOf(struct cliente *)); //array del total de clientes
	arrayClientesSolicitudes = (struct cliente *) malloc (4 * sizeOf(struct cliente *)); // array de clientes con solicitudes (4 para que salga el responsable)
	
	/*Mostramos por pantalla el PID del programa para enviar las señales*/
	printf("\nPID: %s\n", getpid());

	/* SEÑALES */
	//Cliente App
	if (signal(SIGUSR1, nuevoCliente) == SIG_ERR) {
		perror("Error en la señal");

		exit(-1);
	}

	//Cliente Red
	if (signal(SIGUSR2, nuevoCliente) == SIG_ERR) {
		perror("Error en la señal");

		exit(-1);
	}
	
	//Finalización del programa 
	if (signal(SIGINT, finalizarPrograma) == SIG_ERR) {
		perror("Error en la señal");

		exit(-1);
	}

	/* INICIALIZACIÓN DE RECURSOS */
	//Inicialización de los semáforos
	if (pthread_mutex_init(&semaforoFichero, NULL) != 0) exit(-1);
	if (pthread_mutex_init(&semaforoColaClientes, NULL) != 0) exit(-1);
	if (pthread_mutex_init(&semaforoSolicitudes, NULL) != 0) exit(-1);

	//Contadores de clientes
	int nClientes = 0;
	int nClientesApp = 0;
	int nClientesRed = 0;
	int nSolicitudesDomiciliarias = 0; //Imagino que este también hace falta, anque puede que con el array sea suficiente

	//Listas clientes y trabajadores

	//Fichero de log
	logFile = fopen("resgistroTiempos.log", "wt"); //Comprobar si wt es la opcion correcta
	fclose(logFile);

	//Variables relativas a la solicitud de atención domiciliaria

	//Variables condicion

	/* CREACIÓN DE HILOS DE TECNICOS, RESPONSABLES, ENCARGADO Y ATENCION DOMICILIARIA */

	/* ESPERAR POR SEÑALES INFINITAMENTE */
	while(1) {
		pause();
	}

	/* LIBERACIÓN DE MEMORIA */
	free(arrayHilosClientes);
	free(arrayClientes);
	free(arrayClientesSolicitudes);

	return 0;
}
