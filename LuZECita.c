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

/*Mensajes de log*/
//Mensajes al principio del log de que comienza la atención a los clientes
char *entrada = "Hora de entrada al sistema.";
char *appDificil = "El cliente se ha ido porque encuentra la aplicación difícil.";
char *muchaEspera = "El cliente se ha ido porque se ha cansado de esperar.";
char *noInternet = "El cliente se ha ido porque ha perdido la conexión a internet.";
char *esperaAtencion = "El cliente espera por la atención domiciliaria";
char *atencionFinaliza = "La atención domiciliaria ha finalizado";
char *clienteAtendido = "El cliente ha sido atendido correctamente y se va.";
char *clienteEmpiezaAtendido= "El cliente comienza a ser atendido.";

/*Variables globales*/
pthread_mutex_t semaforoFichero;
pthread_mutex_t semaforoColaClientes;
pthread_mutex_t semaforoSolicitudes;

//Variables condicion
pthread_cond_t condicionSaleViaje; //Condición por la que el técnico sale de viaje
pthread_cond_t condicionTerminaViaje; //Condición por la que el técnico avisa de que ha terminado de atender al cliente de forma domiciliaria

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
	int solicitud;	//0 = no solicitud, 1 = esperando la atención domiciliaria, 2 = ha solicitado atención domiciliaria
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


/****************************************** HILOS Y FUNCIONES PRINCIPALES ******************************************/

/*Crea un nuevo cliente cuando recibe una de las dos señales definidas para ello*/
void crearNuevoCliente(int signum, struct cliente *nuevoCliente){
	bool espacioDisponible=false;

	pthread_mutex_lock(&semaforoColaClientes);
	if(nClientes<MAX_CLIENTES){
    	espacioDisponible=true;
	}
	pthread_mutex_unlock(&semaforoColaClientes);

	if(espacioDisponible==true){
		pthread_mutex_lock(&semaforoColaClientes);
		nClientes++;	//aumentamos el contador de clientes totales

		char numeroId[2];		//creamos numeroId
		printf("Hay un nuevo cliente\n");
		
		//Vemos si el cliente es de la app o red
		switch(signum){
			case SIGUSR1:
				if(signal(SIGUSR1, crearNuevoCliente)==SIG_ERR){
					perror("Error en signal");
					exit(-1);
				}
			
				nClientesApp++;				//aumentamos el contador de clientes de app

				snprintf(numeroId, "%d", nClientesApp);
				nuevoCliente->id=strdup(strcat("cliapp_", numeroId));
				
				nuevoCliente->atendido=0;	//0 indica que el cliente todavia no ha sido atendido
				nuevoCliente->tipo=strdup("App");	//cliente de la app
				nuevoCliente->solicitud=0;	//ponemos la solicitud del cliente en 0
				nuevoCliente->prioridad=calculaAleatorios(1, 10);	//damos un numero de prioridad aleatorio al cliente
				printf("El cliente es %s\n", nuevoCliente->id);
				
			break;

			case SIGUSR2:
				if(signal(SIGUSR2, crearNuevoCliente)==SIG_ERR){
					perror("Error en signal");
					exit(-1);
				}
				nClientesRed++;		//aumentamos el contador de clientes de red
				
				snprintf(numeroId, "%d", nClientesRed);
				nuevoCliente->id=strdup(strcat("clired_", numeroId));

				nuevoCliente->atendido=0;
				nuevoCliente->tipo=strdup("Red");	//cliente de red
				nuevoCliente->prioridad=calculaAleatorios(1, 10);
				printf("El cliente es %s\n", nuevoCliente->id);

			break;

			free(nuevoCliente->id);
			free(nuevoCliente->tipo);
		}
		pthread_t hiloClientes;		//hiloClientes es una variable de tipo pthread_t que se ha declarado para almacenar el identificador de un hilo específico
		pthread_create(&hiloClientes, NULL, accionesCliente, (void*)nuevoCliente);		//pthread_create() esta creando un nuevo hilo y asignandole la funcion accionesCliente() como funcion de entrada. El hilo se almacena en el elemento nClientes de arrayHilosClientes. La funcion accionesCliente() recibe como argumento el indice del elemento del arreglo de clientes correspondiente al hilo
		pthread_detach(hiloClientes);
		pthread_mutex_unlock(&semaforoColaClientes);		//libera el hilo del mutex y permite a otros otros hilos poder entrar en la cola de cliente y asi en la seccion critica
		printf("Se ha creado un nuevo cliente con ID %s y tipo %s\n", nuevoCliente->id, nuevoCliente->tipo);

	}else{
		printf("No hay espacio disponible para atender a más clientes en este momento\n");
	}
}

void *accionesCliente (void* nuevoCliente) {
	//Esto creo que no funcione, probar otra manera de castear
	//int aCliente = *(int *)nuevoCliente ??
	//int aCliente = *(intptr_t *)nuevoCliente <-- Este creo que no falla
	(int *) nuevoCliente;

	/*	
	Hay que cambiar los nuevoCliente.tipo, etc. por arrayClientes[(nuevoCliente)].atendido, etc.
	*/

	/*Guardar en log la hora de entrada al sistema y tipo de cliente*/
	pthread_mutex_lock(&semaforoFichero);
	writeLogMessage(arrayClientes[(nuevoCliente)].id, entrada); //Escribe el id del cliente y el char "entrada" (variable global)
	writeLogMessage(arrayClientes[(nuevoCliente)]id, arrayClientes[(nuevoCliente)].tipo); //Escribe el id y tipo del cliente
	pthread_mutex_unlock(&semaforoFichero);

	/*Comprobar si el cliente está atendido - Comprobar que no lo está*/
	pthread_mutex_lock(&semaforoColaClientes);

	do {
		int comportamiento = calculaAleatorios(1, 100);

		if (comportamiento <= 10) { //Un 10% encuentra la app dificil y se va
			clienteFuera(arrayClientes[(nuevoCliente)].id, appDificil); //Se escribe en el log

			//Liberar espacio en la cola de clientes
			liberaEspacioClientes(arrayClientes[(nuevoCliente)].tipo); //Funcíón que resta un cliente de nClientes y nClientes(tipo) cuando este se va

			//Libera un hueco en el array de clientes
			borrarCliente((int )nuevoCliente); //Función que borra al cliente del arrayClientes
				
			//Libera el mutex y finaliza el hilo
			pthread_mutex_unlock(&semaforoColaClientes); //Se libera el mutex antes de finalizar el hilo
			pthread_exit(NULL); //Se finaliza el hilo cliente
		}
		else if (comportamiento > 10 && comportamiento <= 30) { //Un 20% se cansa de esperar
			sleep(8);

			clienteFuera(arrayClientes[(nuevoCliente)].id, muchaEspera);

			//Liberar espacio en la cola de clientes
			liberaEspacioClientes(arrayClientes[(nuevoCliente)].tipo); //Funcíón que resta un cliente de nClientes y nClientes(tipo) cuando este se va

			//Libera un hueco en el array de clientes
			borrarCliente((int )nuevoCliente); //Función que borra al cliente del arrayClientes

			//Libera el mutex y finaliza el hilo
			pthread_mutex_unlock(&semaforoColaClientes);
			pthread_exit(NULL);
		}
		else if (comportamiento > 30 && comportamiento <= 35) { //Un 5% del restante pierde la conexión a internet
			clienteFuera(arrayClientes[(nuevoCliente)].id, noInternet);

			//Liberar espacio en la cola de clientes (Antes o después dek unlock & exit??)
			liberaEspacioClientes(arrayClientes[(nuevoCliente)].tipo);

			//Libera un hueco en el array de clientes
			borrarCliente((int )nuevoCliente); //Función que borra al cliente del arrayClientes

			//Libera el mutex y finaliza el hilo
			pthread_mutex_unlock(&semaforoColaClientes);
			pthread_exit(NULL);
		}
		else {//Si se queda duerme 2 segundos y vuelve a comprobar si se va o se queda
			sleep(2);
		}

	} while (arrayClientes[(nuevoCliente)].atendido == 0);

	/*Cliente siendo atendido, comprueba cada 2 segundos que la atención haya terminado*/
	while (arrayClientes[(nuevoCliente)].atendido == 1){
		sleep(2);
	}

	/**************************************************************************************************/
	//int atendido;	//0 no atendido, 1 está siendo atendido, 2 ha sido atendido
	//int solicitud;	//0 = no solicitud, 1 = esperando la atención domiciliaria, 2 = ha solicitado atención domiciliaria
	//pthread_cond_t condicionSaleViaje; //Condición por la que el técnico sale de viaje
	//pthread_cond_t condicionTerminaViaje; //Condición por la que el técnico avisa de que ha terminado de atender a los 4 de atención domiciliaria

	/*Cliente de tipo red, que ya ha sido atendido y que ha solicitado atención domiciliaria*/
	if (arrayClientes[(nuevoCliente)].tipo == "Red" && arrayClientes[(nuevoCliente)].atendido == 2 && arrayClientes[(nuevoCliente)].solicitud == 2) {
		 
  	    pthread_mutex_lock(&semaforoSolicitudes);
  	     
		//Comprueba el número de solicitudes pendientes
		do {
			//Si es menor de 4 se incrementa
			nSolicitudesDomiciliarias++; 
  	    	 
  	    	//Se escribe en el log que el cliente quiere atención
  	    	pthread_mutex_lock(&semaforoFichero);
  	    	writeLogMessage(arrayClientes[(nuevoCliente)].id, esperaAtencion);
  	    	pthread_mutex_unlock(&semaforoFichero);
  	    	
			//Se cambia el valor de solicitud a 1 (cliente esperando atención domiciliaria)
			pthread_mutex_lock(&semaforoColaClientes);
			arrayClientes[(nuevoCliente)].solicitud == 1; 
			pthread_mutex_unlock(&semaforoColaClientes);

			//Si hay 4 solicitudes (tras incrementar)
			if (nSolicitudesDomiciliarias == 4) {

				//El cuarto en solicitad envía la señal al técnico de que ya puede salir de viaje
				pthread_cond_signal(&condicionSaleViaje);

				//Se bloquea hasta que el técnico ponga solicitud a 0, es decir, el cliente ya ha recibido la atención domiciliaria
				//Cuando recibe la señal de que ha terminado el viaje semaforoSolicitudes se desbloque, no hace falta mutex_unlock
				pthread_cond_wait(&condicionTerminaViaje, &semaforoSolicitudes);
  	    		 
				//Se escribe en el log que la atencion ha finalizado
  	   			pthread_mutex_lock(&semaforoFichero);
  	   			writeLogMessage(arrayClientes[(nuevoCliente)].id, atencionFinaliza);
  	   			pthread_mutex_unlock(&semaforoFichero);
			}

		} while (nSolicitudesDomiciliarias < 4);

		//:::::::::::::::::::::Falta el duerme 3 segundos y vuelve a 5.a (el do)
  	}
	
	/*Cliente atendido satisfactoriamente*/
	//Liberar espacio en la cola de clientes (Antes o después dek unlock & exit??)
	liberaEspacioClientes(arrayClientes[(nuevoCliente)].tipo);
	
	//Libera un hueco en el array de clientes
	//No estoy seguro de si es esto, arrayClientes o nuevoCliente simplemente
	borrarCliente((int )nuevoCliente); //Función que borra al cliente del arrayClientes
	
	//Libera el mutex y finaliza el hilo
	pthread_mutex_unlock(&semaforoColaClientes);

	//Escribe en el log que el cliente ha sido atendido y se va
  	pthread_mutex_lock(&semaforoFichero);
  	writeLogMessage(arrayClientes[(nuevoCliente)].id, clienteAtendido);
  	pthread_mutex_unlock(&semaforoFichero);

	//Fin del hilo cliente
	pthread_exit(NULL);
}

void *accionesTecnico(void *arg){
	int tiempoAtencion;
	if(nClientesApp==0){
		sleep(5);
		accionesTecnico();
	}
	//dudoso
	if(tecnico1.clientesAtendidos==5){
		tecnico1.libre=1;
		tecnico1.clientesAtendidos=0;
		sleep(5);
	}
	if(tecnico1.libre==0){
		for(i=0; i<nClientes; i++){
			if(arrayClientes[i].atendido==0&&arrayClientes[i].tipo=="App"){
					//prioridades
					//mayor tiempo esperando
					
					arrayClientes[i].atendido=1;
					writeLogMessage(arrayClientes[(i)].id, clienteEmpiezaAtendido);
					
					if(arrayClientes[clienteAtender].solicitud==1){ //si el cliente ha solicitado atención domiciliaria
						tiempoAtencion=(80 * rand())/RAND_MAX + 1;
					}
					if(arrayClientes[clienteAtender].solicitud==1){ //si el cliente ha solicitado atención domiciliaria
						tiempoAtencion=(10 * rand())/RAND_MAX + 1;
					}
					if(arrayClientes[clienteAtender].solicitud==1){ //si el cliente ha solicitado atención domiciliaria
						tiempoAtencion=(10 * rand())/RAND_MAX + 1;
					}
						

			}
		}
	}else if(tecnico2.libre==0){

	}else{
		accionesEncargado();
	}

	
}

void *accionesEncargado(void *arg){
	int i;
	bool atendiendo=true; //bandera que indica si está atendiendo a un cliente o no
	
	while(atendiendo){
		//espera a que haya clientes disponibles
		while(nClientes==0){
			sleep(3);
		}
		//busca al cliente para atender primero de red y si no hubiera de tipo app
		int clienteAtender=-1;
		pthread_mutex_lock(&semaforoColaClientes);
		for(i=0; i<nClientes; i++){
			if(arrayClientes[i].atendido==0){ //si el cliente no ha sido atendido
				if(strcmp(arrayClientes[i].tipo, "Red")==0){ //si es de tipo red
					clienteAtender=i;
					break;
				}
				else if(strcmp(arrayClientes[i].tipo, "App")==0 && clienteAtender==-1){ //si es de tipo app y no se ha encontrado aún un cliente de red
					clienteAtender=i;
				}	
			}
		}
		if(clienteAtender==-1){ //si no se ha encontrado ningún cliente de tipo red o app
			//atiende a la prioridad y sino al que más tiempo lleve esperando
			int tiempoEspera=0;
			for(i=0; i<nClientes; i++){
				if(arrayClientes[i].atendido==0){ //si el cliente no ha sido atendido
					if(arrayClientes[i].prioridad>tiempoEspera){ //si tiene mayor prioridad
						tiempoEspera=arrayClientes[i].prioridad;
						clienteAtender=i;
					}
					else if(arrayClientes[i].prioridad==tiempoEspera){ //si tiene la misma prioridad
						if(arrayClientes[i].id>arrayClientes[clienteAtender].id){ //si lleva más tiempo esperando
							clienteAtender=i;
						}
					}
				}
			}
		}
		if(clienteAtender!=-1){ //si se ha encontrado un cliente para atender
			arrayClientes[clienteAtender].atendido=1; //marca el cliente como siendo atendido
			pthread_mutex_unlock(&semaforoColaClientes);

			//calculamos el tipo de atención y en función de esto el tiempo de atención (el 80%, 10%, 10%)
			int tiempoAtencion;
			if(arrayClientes[clienteAtender].solicitud==1){ //si el cliente ha solicitado atención domiciliaria
				tiempoAtencion=(80 * rand())/RAND_MAX + 1;
			}
			else{ //si el cliente no ha solicitado atención domiciliaria
  				if(strcmp(arrayClientes[clienteAtender].tipo, "App")==0){ //si es de tipo app
    			tiempoAtencion=(10 * rand())/RAND_MAX + 1;
  				}
  				else{ //si es de tipo red
    				tiempoAtencion=(20 * rand())/RAND_MAX + 1;
  				}
			}
			//guardar en el log que comienza la atención
			pthread_mutex_lock(&semaforoFichero);
			fprintf(logFile, "El encargado comienza a atender al cliente %s\n", arrayClientes[clienteAtender].id);
			pthread_mutex_unlock(&semaforoFichero);

			//dormimos el tiempo de atención
			sleep(tiempoAtencion);

			//guardar en el log que finaliza la atención
			pthread_mutex_lock(&semaforoFichero);
			fprintf(logFile, "El encargado ha finalizado la atención al cliente %s\n", arrayClientes[clienteAtender].id);
			pthread_mutex_unlock(&semaforoFichero);

			//guardar en el log el motivo del fin de la atención
			if(arrayClientes[clienteAtender].solicitud == 1){
				pthread_mutex_lock(&semaforoFichero);
				fprintf(logFile, "%s\n", atencionFinaliza);
				pthread_mutex_unlock(&semaforoFichero);
			}
			else if(arrayClientes[clienteAtender].prioridad == 0){
				pthread_mutex_lock(&semaforoFichero);
				fprintf(logFile, "%s\n", muchaEspera);
				pthread_mutex_unlock(&semaforoFichero);
			}
		}
	}
}

void accionesTecnicoDomiciliario(){

}

/*Función que finaliza el programa al recibir la señal SIGINT*/
void finalizarPrograma (int signal) {

	//Antes de que finalice se debe terminar de atender a todos los clientes en cola

	printf("\nAdiós!\n");

	exit(0);
}


/******************************************************** SIGO SIN SABER QUÉ ES ESTO ********************************************************/
/********************************************************************************************************************************************/
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


/****************************************** FUNCIONES AUXILIARES ******************************************/

/*Función que escribe en el log cuando un cliente se va antes de ser atendido*/
void clienteFuera(char *id, char *razon) {
	pthread_mutex_lock(&semaforoFichero);
	writeLogMessage(id, razon); //Escribe el id del cliente y el char "appDificil" 
	pthread_mutex_unlock(&semaforoFichero);
}

/*Función que decrementa el número de clientes total y del tipo*/
void liberaEspacioClientes(char *tipoCliente) {
	if (tipoCliente == "App") {
		nClientesApp--;	
	} else {
		nClientesRed--;
	}
	nClientes--;
}

/*Función que elimina un cliente del array de clientes*/
void borrarCliente (int posicionCliente) {

	for (int i = posicionCliente; i < (nClientes - 1); i++) {
		arrayClientes[i] = arrayClientes[i + 1]; //El cliente en cuestión se iguala al de la siguiente posición (inicializado todo a 0)
	}

	struct cliente structVacia; //Se declara un cliente vacío
	arrayClientes[nClientes - 1] = structVacia; //El cliente pasa a estar vació (todo a 0)
}

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
	logFile = fopen("registroTiempos.log", "a");
	fprintf(logFile, "[%s] %s: %s\n", stnow, id, msg);
	fclose(logFile);
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
	if (signal(SIGUSR1, crearNuevoCliente) == SIG_ERR) {
		perror("Error en la señal");

		exit(-1);
	}

	//Cliente Red
	if (signal(SIGUSR2, crearNuevoCliente) == SIG_ERR) {
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

	/* Inicialización de variables condicion*/
	if (pthread_cond_init(&condicionSaleViaje, NULL) != 0) exit(-1);
	if (pthread_cond_init(&condicionTerminaViaje, NULL) != 0) exit(-1);

	//Contadores de clientes
	int nClientes = 0;
	int nClientesApp = 0;
	int nClientesRed = 0;

	//Variables relativas a la solicitud de atención domiciliaria
	int nSolicitudesDomiciliarias = 0; //Contador para las solicitudes, cuando sean 4 el técnico sale de viaje

	//Lista clientes
	struct cliente *clienteApp;
		clienteApp -> id = (char *) malloc (12 * sizeof(char));
		clienteApp -> id = "cliapp_"; //Luego en cada cliente creado se le añade el número al final de la cadena de caracteres con ¿strcat? creo
		
		clienteApp -> atendido = 0;

		clienteApp -> tipo = (char *) malloc (12 * sizeof(char));
		clienteApp -> tipo = "App";

		clienteApp -> prioridad = 0;
		clienteApp -> solicitud = 0;

	struct cliente *clienteRed ;
		clienteRed  -> id = (char *) malloc (12 * sizeof(char));
		clienteRed  -> id = "clired_"; //Luego en cada cliente creado se le añade el número al final de la cadena de caracteres con ¿strcat? creo
		
		clienteRed  -> atendido = 0;

		clienteRed  -> tipo = (char *) malloc (18 * sizeof(char));
		clienteRed  -> tipo = "Red";

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
	logFile = fopen("registroTiempos.log", "wt"); //Opcion "wt" = Cada vez que se ejecuta el programa, si el archivo ya existe se elimina su contenido para empezar de cero
	fclose(logFile);

	//writeLogMessage con un mensaje de que se inicia el programa

	//::::::::::::::::::::::::::::Añadir un mutex a la creación de hilos? No sé si hace falta

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

	//pthread_join();
	//pthread_mutex_destroy();

	/* LIBERACIÓN DE MEMORIA */
	free(arrayHilosClientes);
	free(arrayClientes);
	free(arrayClientesSolicitudes);

	return 0;
}
