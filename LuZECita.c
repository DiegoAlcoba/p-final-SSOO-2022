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

//Numero maximo de clientes que se pueden atender a la vez
#define MAX_CLIENTES 20

//Numero de tecnicos, responsables de reparaciones y encargados
#define NUM_TECNICOS 2
#define NUM_RESPONSABLES 2
#define NUM_ENCARGADOS 1

/*Variables globales*/
pthread_mutex_t semaforoFichero;
pthread_mutex_t semaforoColaClientes;
pthread_mutex_t semaforoSolicitudes;

//Array de hilo-cliente
pthread_t *arrayHilosClientes;

int nClientesApp; //Contador clientes app
int nClientesRed; //Contador clientes red
int nClientes;	//Número total de clientes
int nSolicitudesDomiciliarias; //N solicitudes domiciliarias (4 para que el responsable viaje)

/*Arrays dinámicos que almacenan a los clientes (estructura)*/
struct cliente *arrayClientes;
struct cliente *arrayClientesSolicitudes; //Clientes a los que tiene que visitar el responsable

/*Estructuras*/
//estructura que guarda la informacion del cliente
struct cliente{
	char *id;		//no seria mejor poner el id como int? NO PORQUE EL IDENTIFICADOR ES CLIAPP_1
	int atendido;	//0 no atendido, 1 está siendo atendido, 2 ha sido atendido
	char *tipo;		//app o red
	int prioridad;	//Array de hilo-cliente
	int solicitud;	//0 no solicitud, 1 solicita atención domiciliaria
};

//estructura que guarda la informacion del trabajador
struct trabajador{ 
	char *id;	
	int clientesAtendidos;
	char *tipo; //Tecnico, responsable o encargado T, R o E
	int libre; //0 si esta libre 1 si esta ocupado
};

/*Fichero de log*/
FILE *logFile;

////funcion para crear un tecnico
//Trabajador generar_Tecnico(){
//	//Tecnico 1
//		static int id1=0;
//		Trabajador tecnico_1;
//		tecnico_1.id=++id1;
//		tecnico_1.clientesAtendidos=0;
//		tecnico_1.tipo=T;
//		tecnico_1.libre=0;
//	//Tecnico 2
//		static int id2=0;
//		Trabajador tecnico_2;
//		tecnico_2.id=++id2;	
//		tecnico_2.clientesAtendidos=0;
//		tecnico_2.tipo=T;
//		tecnico_2.libre=0;
//}
//
////funcion para generar un cliente
//Cliente generar_cliente(){
//	static int id = 0;
//	Cliente c;
//	c.id = ++id;
//	c.atendido = 0;//0 si no esta atendido, 1 si si lo esta
//
//	//genera un tipo de cliente aleatoriamente(para un cliente de app usaremos a, para uno de red usaremos r)
//	c.tipo = rand()%2==0 ? 'a' : 'r';
//
//	return c;
//}

/*****************************************	ESPACIO PARA ESCRIBIR LAS FUNCIONES, MANEJADORES, ETC  ***********************************************/

void crearNuevoCliente(int signum){
	pthread_mutex_lock(&semaforoColaClientes);

	if(nClientes<MAX_CLIENTES){
		struct cliente nuevoCliente;
		nClientes++;
		char numeroId[3];
		printf("Hay un nuevo cliente\n");
		sprintf(numeroId, "%d", nClientes);
		nuevoCliente.id=strcat("cliente_", numeroId);
		
		//Vemos si el cliente es de la app o red
		switch(signum){
			case SIGUSR1:
			if(signal(SIGUSR1, crearNuevoCliente)==SIG_ERR){
				perror("Error en signal");
				exit(-1);
			}
			nuevoCliente.tipo="App";	//cliente de la app
			nClientesApp++;
			break;

			case SIGUSR2:
			if(signal(SIGUSR2, crearNuevoCliente)==SIG_ERR){
				perror("Error en signal");
				exit(-1);
			}
			nuevoCliente.tipo="Red";	//cliente de red
			nClientesRed++;
			break;
		}
		nuevoCliente.atendido=0;

		arrayClientes[nClientes-1]=nuevoCliente;
		pthread_t hiloClientes;
		arrayHilosClientes[nClientes-1]=hiloClientes;
		pthread_create(&arrayHilosClientes[nClientes], NULL, accionesCliente, (void*)(nClientes-1));
		pthread_mutex_unlock(&semaforoColaClientes);
	}else{
		pthread_mutex_unlock(&semaforoColaClientes);
		printf("No se ha podido atender al cliente\n");
		return;
	}
}

//¿Esto es la función "accionesTecnico" que ejecutan los hilos de tecnicos?  
/*funcion del técnico (CUIDADO) es decir atender a los clientes con problemas en la app*/
void atenderClienteApp(){
	//IMPORTANTE:::::AÑADIR LO DE LA PRIORIDAD
	//Se comprueba si el tenico esta libre
	struct cliente cliente;
	struct trabajador trabajador;

	if(tecnico1.libre==0){
		printf("Se va a atender a un cliente con problemas en la app\n");
		printf("Bienvenido cliente, le esta atendiendo el tecnico 1\n");
		tecnico1.libre=1;
		//Compruebo que el cliente que atiendo sea de app
		if(cliente.tipo==App){
			printf("El cliente ha sido atendido\n");
			//Cambio su estado a atendido
			cliente.atendido=1;
			++tecnico1.clientesAtendidos;
			//Si hay 5 descanso 5 seg
			if(tecnico1.clientesAtendidos==5){
				tecnico1.libre=1;
				sleep(5);
			}
		}
	}else if (tecnico2.libre==0){
		printf("Se va a atender a un cliente con problemas en la app\n");
		printf("Bienvenido cliente, le esta atendiendo el tecnico 2\n");
		tecnico2.libre=1;
		//Compruebo que el cliente que atiendo sea de app
		if(cliente.tipo==App){
			printf("El cliente ha sido atendido\n");
			//Cambio su estado a atendido
			cliente.atendido=1;
			++tecnico2.clientesAtendidos;
			//Si hay 5 descanso 5 seg
			if(tecnico2.clientesAtendidos==5){
				tecnico2.libre=1;
				sleep(5);
			}
		}
	}else{
		printf("Ningun tecnico esta libre en este momento\n");
	}
}

/*************************************************************************************************************************************************/

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
	logFile = fopen("resgistroTiempos.log", "a");
	fprintf(logFile, "[%s] %s: %s\n", stnow, id, msg);
	fclose(logFile);
}

//IMPORTANTE::::::AÑADIR CONDICION DE QUE SI ESTAN LOS DOS OCUPADOS ATENDERLOS ENCARGADO

//funcion para atender a un cliente ............. Los clientes son atendidos en las funciones que ejecutan los hilos (accionesTecnico, accionesEncargado, accionesTecnicoDomiciliario)
//void atender_cliente(){
//	printf("Se esta atendiendo al cliente %d (%c)\n, c.id, c.tipo");
//	sleep(1);	//se simula el tiempo de atencion
//}

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
	arrayClientes = (struct cliente *) malloc (nClientes * sizeOf(struct cliente *)); //array del total de clientes
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

	//Lista clientes
	struct cliente *clienteApp;
		clienteApp -> id = (char *) malloc (12 * sizeof(char));
		clienteApp -> id = "cliapp_"; //Luego en cada cliente creado se le añade el número al final de la cadena de caracteres con ¿strcat? creo
		
		clienteApp -> atendido = 0;

		clienteApp -> tipo = (char *) malloc (12 * sizeof(char));
		clienteApp -> tipo = "Aplicacion";

		clienteApp -> prioridad = 0;
		clienteApp -> solicitud = 0;

	struct cliente *clienteRed ;
		clienteRed  -> id = (char *) malloc (12 * sizeof(char));
		clienteRed  -> id = "clired_"; //Luego en cada cliente creado se le añade el número al final de la cadena de caracteres con ¿strcat? creo
		
		clienteRed  -> atendido = 0;

		clienteRed  -> tipo = (char *) malloc (18 * sizeof(char));
		clienteRed  -> tipo = "Problema en red";

		clienteRed  -> prioridad = 0;
		clienteRed -> solicitud = 0;

	//Lista trabajadores
	/*Técnicos*/
	struct trabajador *tecnico_1;
	tecnico_1 -> id = (char *) malloc (10  * sizeof(char));
	tecnico_1 -> id = "tecnico_1";

	tecnico_1 -> clientesAtendidos = 0;	//No estoy seguro si hace falta inicializar a 0 esto o ya lo está
	
	tecnico_1 -> tipo = (char *) malloc (8 * sizeof(char));
	tecnico_1 -> tipo = "Tecnico";

	tecnico_1 -> libre = 0;	//No estoy seguro si hace falta inicializar a 0 esto o ya lo está

	struct trabajador *tecnico_2;
	tecnico_2 -> id = (char *) malloc (10  * sizeof(char));
	tecnico_2 -> id = "tecnico_2";

	tecnico_2 -> clientesAtendidos = 0;	//No estoy seguro si hace falta inicializar a 0 esto o ya lo está
	
	tecnico_2 -> tipo = (char *) malloc (8 * sizeof(char));
	tecnico_2 -> tipo = "Tecnico";

	tecnico_2 -> libre = 0;	//No estoy seguro si hace falta inicializar a 0 esto o ya lo está

	/*Responsables de reparaciones*/
	struct trabajador *responsable_1;
	responsable_1 -> id = (char *) malloc (14  * sizeof(char));
	responsable_1 -> id = "responsable_1";

	responsable_1 -> clientesAtendidos = 0;	//No estoy seguro si hace falta inicializar a 0 esto o ya lo está
	
	responsable_1 -> tipo = (char *) malloc (20 * sizeof(char));
	responsable_1 -> tipo = "Resp. Reparaciones";

	responsable_1 -> libre = 0;	//No estoy seguro si hace falta inicializar a 0 esto o ya lo está

	struct trabajador *responsable_2;
	responsable_2 -> id = (char *) malloc (14  * sizeof(char));
	responsable_2 -> id = "responsable_2";

	responsable_2 -> clientesAtendidos = 0;	//No estoy seguro si hace falta inicializar a 0 esto o ya lo está
	
	responsable_2 -> tipo = (char *) malloc (20 * sizeof(char));
	responsable_2 -> tipo = "Resp. Reparaciones";

	responsable_2 -> libre = 0;	//No estoy seguro si hace falta inicializar a 0 esto o ya lo está

	/*Encargado*/
	struct trabajador *encargado_;
	encargado_ -> id = (char *) malloc (10 * sizeof(char));
	encargado_ -> id = "encargado";

	encargado_ -> clientesAtendidos = 0;	//No estoy seguro si hace falta inicializar a 0 esto o ya lo está

	encargado_ -> tipo = (char *) malloc (10 * sizeof(char));
	encargado_ -> tipo = "Encargado";

	encargado_ -> libre = 0;	//No estoy seguro si hace falta inicializar a 0 esto o ya lo está

	//Fichero de log
	logFile = fopen("resgistroTiempos.log", "wt"); //Opcion "wt" = Cada vez que se ejecuta el programa, si el archivo ya existe se elimina su contenido para empezar de cero
	fclose(logFile);

	//Variables relativas a la solicitud de atención domiciliaria
	int nSolicitudesDomiciliarias = 0; //Contador para las solicitudes, cuando sea 4 se envía atención domiciliaria

	//Variables condicion

	/* CREACIÓN DE HILOS DE TECNICOS, RESPONSABLES, ENCARGADO Y ATENCION DOMICILIARIA */
	//Se pasa como argumento la estructura del trabajador que ejecuta el hilo
	pthread_create(&tecnico1, NULL, accionesTecnico, (void *) tecnico_1);
	pthread_create(&tecnico2, NULL, accionesTecnico, (void *) tecnico_2);
	pthread_create(&responsable1, NULL, accionesTecnico, (void *) responsable_1);
	pthread_create(&responsable2, NULL, accionesTecnico, (void *) responsable_2);
	pthread_create(&encargado, NULL, accionesEncargado, (void *) encargado_);
	pthread_create(&atencionDomiciliaria, NULL, accionesTecnicoDomiciliario, (void *)/*argumentos necesarios para la ejecución del hilo, ningún trabajador en particular*/);

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
