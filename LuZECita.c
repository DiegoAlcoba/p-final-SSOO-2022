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
#include <stdint.h>
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
char *nombrePrograma = "    LuZECita";
char *iniciaPrograma = "COMIENZA LA ATENCIÓN DE LOS CLIENTES";
char *entrada = "Hora de entrada al sistema.";
char *appDificil = "El cliente se ha ido porque encuentra la aplicación difícil.";
char *muchaEspera = "El cliente se ha ido porque se ha cansado de esperar.";
char *noInternet = "El cliente se ha ido porque ha perdido la conexión a internet.";
char *esperaAtencion = "El cliente espera por la atención domiciliaria";
char *atencionFinaliza = "La atención domiciliaria ha finalizado";
char *clienteAtendido = "El cliente ha sido atendido correctamente y se va.";
char *clienteEmpiezaAtendido= "El cliente comienza a ser atendido.";
char *clienteFinalizaAtencion= "El cliente finaliza la atencion.";
char *todoEnRegla= "Se ha finalizado la atencion ya que el cliente tenia todo en regla";
char *malIdentificados= "Se ha finalizado la atencion ya que el cliente estaba mal identificado";
char *confusionCompañia= "Se ha finalizado la atencion ya que el cliente se ha confundido de compañia";
char *signalFinalizacion = "    SIGINT";
char *finPrograma = "SE PROCEDE A LA FINALIZACION DEL PROGRAMA";
char *atencionDomiciliariaText= "Se comienza con la accion domiciliaria del cliente";
char *atencionDomiciliariaAtendido= " Se ha atendido al cliente en el domicilio";
char *atencionDomiciliariaFin= "Se finaliza con la accion domiciliaria del cliente";

/*Variables globales*/
//Declaración de los hilos de trabajadores
pthread_t tecnico_1, tecnico_2, responsable_1, responsable_2, encargado_, atencionDomiciliaria_;

//Semáforos mutex
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
void crearNuevoCliente(int signum) { //Solo recibe como argumento la señal, la estructura del cliente es una variable global
	struct cliente nuevoCliente;
	bool espacioDisponible=false;		//creamos una variable booleana inicializada a false

	pthread_mutex_lock(&semaforoColaClientes);		//bloqueamos el acceso para que otros hilos no puedan entrar en la seccion critica
	if(nClientes<MAX_CLIENTES){		//miramos que el contador de clientes no sea mayor que el maximo de clientes
    	espacioDisponible=true;
	}
	pthread_mutex_unlock(&semaforoColaClientes);		//desbloqueamos el acceso para que otros hilos puedan entrar en la seccion critica

	if(espacioDisponible==true){		
		pthread_mutex_lock(&semaforoColaClientes);
		nClientes++;	//aumentamos el contador de clientes totales
		pthread_mutex_unlock(&semaforoColaClientes);

		char numeroId[2];		//creamos numeroId
		printf("Hay un nuevo cliente\n");
		
		//Vemos si el cliente es de la app o red
		switch(signum){		//dependiendo de la señal que recibamos sera un tipo de cliente u otro
			case SIGUSR1:		//cliente de app
				pthread_mutex_lock(&semaforoColaClientes);
				if(signal(SIGUSR1, crearNuevoCliente)==SIG_ERR){		//si la señal SIGUSR1 nos da SIG_ERR se ha producido un error
					perror("Error en signal");
					exit(-1);
				}
			
				nClientesApp++;				//aumentamos el contador de clientes de app

				sprintf(numeroId, "%d", nClientesApp);		

				nuevoCliente.id=strcat("cliapp_", numeroId);		//concatenamos el numero de id a el nombre de cliente
				nuevoCliente.atendido=0;		//todavia no ha sido atendido
				nuevoCliente.tipo="App";	//cliente de la app
				nuevoCliente.prioridad=calculaAleatorios(1, 10);	//damos un numero de prioridad aleatorio al cliente
				pthread_mutex_unlock(&semaforoColaClientes);

				printf("El cliente es %s\n", nuevoCliente.id);
				
			break;

			case SIGUSR2:		//cliente de red
				pthread_mutex_lock(&semaforoColaClientes);
				if(signal(SIGUSR2, crearNuevoCliente)==SIG_ERR){
					perror("Error en signal");
					exit(-1);
				}
				nClientesRed++;		//aumentamos el contador de clientes de red
				
				sprintf(numeroId, "%d", nClientesRed);
				
				nuevoCliente.id=strcat("clired_", numeroId);
				nuevoCliente.atendido=0;
				nuevoCliente.tipo="Red";	//cliente de red
				nuevoCliente.solicitud=0;		//todavia no ha hecho solicitud domiciliaria
				nuevoCliente.prioridad=calculaAleatorios(1, 10);
				pthread_mutex_unlock(&semaforoColaClientes);

				printf("El cliente es %s\n", nuevoCliente.id);

			break;

		}
		pthread_mutex_lock(&semaforoColaClientes);
		arrayClientes[nClientes-1]=nuevoCliente; //asigna la estructura nuevoCliente al ultimo elemento de arrayClientes
		pthread_t hiloClientes; // declaramos hiloClientes para almacenar el identificador de un hilo específico
		arrayHilosClientes[nClientes-1]=hiloClientes; //asigna la estructura hiloClientes al ultimo elemento de arrayHilosClientes
		pthread_create(&arrayHilosClientes[nClientes], NULL, accionesCliente, (void*)(nClientes-1)); //pthread_create() esta creando un nuevo hilo y asignandole la funcion accionesCliente() como funcion de entrada, se almacena en el elemento nClientes de arrayHilosClientes. La funcion accionesCliente() recibe como argumento el indice del elemento del arreglo de clientes correspondiente al hilo
		pthread_mutex_unlock(&semaforoColaClientes);
		
		printf("Se ha creado un nuevo cliente con ID %s y tipo %s\n", nuevoCliente.id, nuevoCliente.tipo);

	}else{
		printf("No hay espacio disponible para atender a más clientes en este momento\n");
	}
}

void *accionesCliente (void* nuevoCliente) {
	//int aCliente = *(int *)nuevoCliente ??
	int aCliente = *(intptr_t *)nuevoCliente; //<-- Este creo que no falla y sería solo trabajar con aCliente
	//(int *) nuevoCliente;
	
	/*Guardar en log la hora de entrada al sistema y tipo de cliente*/
	pthread_mutex_lock(&semaforoFichero);
	writeLogMessage(arrayClientes[aCliente].id, entrada); //Escribe el id del cliente y el char "entrada" (variable global)
	writeLogMessage(arrayClientes[aCliente].id, arrayClientes[aCliente].tipo); //Escribe el id y tipo del cliente
	pthread_mutex_unlock(&semaforoFichero);

	do {
		/*Comprobar si el cliente está siendo atendido*/
		pthread_mutex_lock(&semaforoColaClientes);
		if (arrayClientes[aCliente].atendido == 0) {
			printf("El cliente aún no ha sido atendido...\n");
		}
		pthread_mutex_unlock(&semaforoColaClientes);

		int comportamiento = calculaAleatorios(1, 100);
		
		//Un 10% encuentra la app dificil y se va
		if (comportamiento <= 10) {
			clienteFuera(arrayClientes[aCliente].id, appDificil); //Se escribe en el log

			//Libera el espacio en el array de clientes
			pthread_mutex_lock(&semaforoColaClientes);
			borrarCliente(aCliente); //Función que borra al cliente del arrayClientes
			printf("El cliente ha decidido irse. Razón registrada en el fichero de logs.\n");
			pthread_mutex_unlock(&semaforoColaClientes);
				
			//Finaliza el hilo
			pthread_exit(NULL); //Se finaliza el hilo cliente
		}
		//Un 20% se cansa de esperar
		else if (comportamiento > 10 && comportamiento <= 30) {
			sleep(8);

			clienteFuera(arrayClientes[aCliente].id, muchaEspera);

			//Libera el espacio en el array de clientes
			pthread_mutex_lock(&semaforoColaClientes);
			borrarCliente(aCliente); //Función que borra al cliente del arrayClientes
			printf("El cliente ha decidido irse. Razón registrada en el fichero de logs.\n");
			pthread_mutex_unlock(&semaforoColaClientes);
				
			//Finaliza el hilo
			pthread_exit(NULL); //Se finaliza el hilo cliente
		}
		//Un 5% del restante pierde la conexión a internet
		else if (comportamiento > 30 && comportamiento <= 35) { 
			clienteFuera(arrayClientes[aCliente].id, noInternet);

			//Libera el espacio en el array de clientes
			pthread_mutex_lock(&semaforoColaClientes);
			borrarCliente(aCliente); //Función que borra al cliente del arrayClientes
			printf("El cliente ha decidido irse. Razón registrada en el fichero de logs.\n");
			pthread_mutex_unlock(&semaforoColaClientes);
				
			//Finaliza el hilo
			pthread_exit(NULL); //Se finaliza el hilo cliente
		}
		//Si se queda duerme 2 segundos y vuelve a comprobar si se va o se queda
		else {
			sleep(2);
		}

	} while (arrayClientes[aCliente].atendido == 0);

	/*Cliente siendo atendido, comprueba cada 2 segundos que la atención haya terminado*/
	while (arrayClientes[aCliente].atendido == 1){

		pthread_mutex_lock(&semaforoColaClientes);
		printf("Cliente siendo atendido. Esperando finalización...\n");
		pthread_mutex_unlock(&semaforoColaClientes);

		sleep(2);
	}

	/*Cliente de tipo red, que ya ha sido atendido y que ha solicitado atención domiciliaria*/
	if (arrayClientes[aCliente].tipo == "Red" && arrayClientes[aCliente].atendido == 2 && arrayClientes[aCliente].solicitud == 2) {
		 
		do {
			//Comprueba el número de solicitudes pendientes
			pthread_mutex_lock(&semaforoSolicitudes);
			if (nSolicitudesDomiciliarias < 4) {
				printf("Actualmente hay %d solicitudes para atencion domiciliaria.\n", nSolicitudesDomiciliarias);
				pthread_mutex_unlock(&semaforoSolicitudes);
			} 
			else {
				sleep(3); //Si hay 4 o más solicitudes duerme 3 segundos y vuelve al inicio
			}

			//Si es menor de 4 se incrementa
			pthread_mutex_lock(&semaforoSolicitudes);
			nSolicitudesDomiciliarias++; 
			pthread_mutex_unlock(&semaforoSolicitudes);
  	    	 
  	    	//Se escribe en el log que el cliente espera para ser atendido
  	    	pthread_mutex_lock(&semaforoFichero);
  	    	writeLogMessage(arrayClientes[aCliente].id, esperaAtencion);
  	    	pthread_mutex_unlock(&semaforoFichero);
  	    	
			//Se cambia el valor de solicitud a 1 (cliente esperando atención domiciliaria)
			pthread_mutex_lock(&semaforoColaClientes);
			arrayClientes[aCliente].solicitud == 1; 
			pthread_mutex_unlock(&semaforoColaClientes);

			//Si hay 4 solicitudes (tras incrementar)
			if (nSolicitudesDomiciliarias == 4) {
				//El cuarto en solicitad envía la señal al técnico de que ya puede salir de viaje
				pthread_mutex_lock(&semaforoSolicitudes); //<--- LOCK
				printf("Hay 4 solicitudes de atencion domiciliaria. Avisando al técnico para que comience la atención...\n");
				pthread_cond_signal(&condicionSaleViaje);

				//Se bloquea hasta que el técnico ponga solicitud a 0, es decir, el cliente ya ha recibido la atención domiciliaria
				//Cuando recibe la señal de que ha terminado el viaje, semaforoSolicitudes se desbloquea, no hace falta mutex_unlock
				pthread_cond_wait(&condicionTerminaViaje, &semaforoSolicitudes); //<--- UNLOCK <--- ESTÁ ESPERANDO A QUE EL TECNICODOMICILIARIO ENVÍE pthread_cond_signal(&condicionTerminaViaje) cuando ha terminado de atenderlo y vuelve a poner la solicitud a 0
  	    		 
				//Se escribe en el log que la atencion ha finalizado
  	   			pthread_mutex_lock(&semaforoFichero);
  	   			writeLogMessage(arrayClientes[aCliente].id, atencionFinaliza);
  	   			pthread_mutex_unlock(&semaforoFichero);
				
				/*La atención domiciliaria ha finalizado, el cliente sale del sistema*/
				//Libera el espacio en el array de clientes
				pthread_mutex_lock(&semaforoColaClientes);
				borrarCliente(aCliente); //Función que borra al cliente del arrayClientes
				pthread_mutex_unlock(&semaforoColaClientes);
					
				//Finaliza el hilo
				pthread_exit(NULL); //Se finaliza el hilo cliente
			}

		} while (nSolicitudesDomiciliarias < 4);
  	}
	
	/*Cliente atendido satisfactoriamente*/
	//Libera el espacio en el array de clientes
	pthread_mutex_lock(&semaforoColaClientes);
	borrarCliente(aCliente); //Función que borra al cliente del arrayClientes
	pthread_mutex_unlock(&semaforoColaClientes);

	//Escribe en el log que el cliente ha sido atendido y se va
  	pthread_mutex_lock(&semaforoFichero);
  	writeLogMessage(arrayClientes[aCliente].id, clienteAtendido);
  	pthread_mutex_unlock(&semaforoFichero);

	//Fin del hilo cliente
	pthread_exit(NULL);
}

void *accionesTecnico(void *arg){
	int tiempoAtencion=0;
	int flagAtendido=0;
	int tiempo=0;
	bool ate= true;
	struct cliente cliente;
	struct trabajador tecnico1;
	struct trabajador tecnico2;

	while(ate){
		while(nClientesApp==0){
			sleep(1);
		}
	
		for(int i =0; i<nClientes; i++ ){
			if(arrayClientes[i].tipo="App"){
				if(tecnico1.clientesAtendidos<5 && tecnico1.libre==0){
				
					pthread_mutex_lock(&semaforoColaClientes);
					tiempoAtencion=mayorPrioridad();
					pthread_mutex_unlock(&semaforoColaClientes);

					pthread_mutex_lock(&semaforoColaClientes);
					printf("Comienza el tecnico a atender al cliente %s: ", arrayClientes[tiempoAtencion].id);
					tecnico1.libre=1;
					arrayClientes[tiempoAtencion].atendido=1;

					pthread_mutex_unlock(&semaforoColaClientes);
					flagAtendido=calculaAleatorios(1, 100);
				

					if(flagAtendido<=80){
						tiempo=calculaAleatorios(1,4);

						pthread_mutex_lock(&semaforoFichero);
						writeLogMessage(clienteEmpiezaAtendido, arrayClientes[tiempoAtencion].id);
						pthread_mutex_unlock(&semaforoFichero);

						sleep(tiempo);

						pthread_mutex_lock(&semaforoFichero);
						writeLogMessage(clienteFinalizaAtencion, arrayClientes[tiempoAtencion].id);
						pthread_mutex_unlock(&semaforoFichero);

						pthread_mutex_lock(&semaforoFichero);
						writeLogMessage(todoEnRegla, arrayClientes[tiempoAtencion].id);
						pthread_mutex_unlock(&semaforoFichero);

					}else if(flagAtendido>80&&flagAtendido<=90){
						tiempo=calculaAleatorios(2,6);

						pthread_mutex_lock(&semaforoFichero);
						writeLogMessage(clienteEmpiezaAtendido, arrayClientes[tiempoAtencion].id);
						pthread_mutex_unlock(&semaforoFichero);

						sleep(tiempo);

						pthread_mutex_lock(&semaforoFichero);
						writeLogMessage(clienteFinalizaAtencion, arrayClientes[tiempoAtencion].id);
						pthread_mutex_unlock(&semaforoFichero);

						pthread_mutex_lock(&semaforoFichero);
						writeLogMessage(malIdentificados, arrayClientes[tiempoAtencion].id);
						pthread_mutex_unlock(&semaforoFichero);


					}else{
						tiempo=calculaAleatorios(1,2);

						pthread_mutex_lock(&semaforoFichero);
						writeLogMessage(clienteEmpiezaAtendido, arrayClientes[tiempoAtencion].id);
						pthread_mutex_unlock(&semaforoFichero);

						sleep(tiempo);

						pthread_mutex_lock(&semaforoFichero);
						writeLogMessage(clienteFinalizaAtencion, arrayClientes[tiempoAtencion].id);
						pthread_mutex_unlock(&semaforoFichero);

						pthread_mutex_lock(&semaforoFichero);
						writeLogMessage(confusionCompañia, arrayClientes[tiempoAtencion].id);
						pthread_mutex_unlock(&semaforoFichero);
					}
					pthread_mutex_lock(&semaforoColaClientes);
					arrayClientes[tiempoAtencion].atendido=2;
					tecnico1.libre=0;
					pthread_mutex_unlock(&semaforoColaClientes);

					if(tecnico1.clientesAtendidos==5){
						sleep(5);
						tecnico1.clientesAtendidos=0;
					}
				}else if(tecnico2.clientesAtendidos<5 && tecnico2.libre==0){
				
					pthread_mutex_lock(&semaforoColaClientes);
					tiempoAtencion=mayorPrioridad();
					pthread_mutex_unlock(&semaforoColaClientes);

					pthread_mutex_lock(&semaforoColaClientes);
					printf("Comienza el tecnico a atender al cliente %s: ", arrayClientes[tiempoAtencion].id);
					tecnico2.libre=1;
					arrayClientes[tiempoAtencion].atendido=1;

					pthread_mutex_unlock(&semaforoColaClientes);
					flagAtendido=calculaAleatorios(1, 100);
					

					if(flagAtendido<=80){
						tiempo=calculaAleatorios(1,4);

						pthread_mutex_lock(&semaforoFichero);
						writeLogMessage(clienteEmpiezaAtendido, arrayClientes[tiempoAtencion].id);
						pthread_mutex_unlock(&semaforoFichero);

						sleep(tiempo);

						pthread_mutex_lock(&semaforoFichero);
						writeLogMessage(clienteFinalizaAtencion, arrayClientes[tiempoAtencion].id);
						pthread_mutex_unlock(&semaforoFichero);

						pthread_mutex_lock(&semaforoFichero);
						writeLogMessage(todoEnRegla, arrayClientes[tiempoAtencion].id);
						pthread_mutex_unlock(&semaforoFichero);

					}else if(flagAtendido>80&&flagAtendido<=90){
						tiempo=calculaAleatorios(2,6);

						pthread_mutex_lock(&semaforoFichero);
						writeLogMessage(clienteEmpiezaAtendido, arrayClientes[tiempoAtencion].id);
						pthread_mutex_unlock(&semaforoFichero);

						sleep(tiempo);

						pthread_mutex_lock(&semaforoFichero);
						writeLogMessage(clienteFinalizaAtencion, arrayClientes[tiempoAtencion].id);
						pthread_mutex_unlock(&semaforoFichero);

						pthread_mutex_lock(&semaforoFichero);
						writeLogMessage(malIdentificados, arrayClientes[tiempoAtencion].id);
						pthread_mutex_unlock(&semaforoFichero);


					}else{
						tiempo=calculaAleatorios(1,2);

						pthread_mutex_lock(&semaforoFichero);
						writeLogMessage(clienteEmpiezaAtendido, arrayClientes[tiempoAtencion].id);
						pthread_mutex_unlock(&semaforoFichero);

						sleep(tiempo);

						pthread_mutex_lock(&semaforoFichero);
						writeLogMessage(clienteFinalizaAtencion, arrayClientes[tiempoAtencion].id);
						pthread_mutex_unlock(&semaforoFichero);

						pthread_mutex_lock(&semaforoFichero);
						writeLogMessage(confusionCompañia, arrayClientes[tiempoAtencion].id);
						pthread_mutex_unlock(&semaforoFichero);
					}
					pthread_mutex_lock(&semaforoColaClientes);
					arrayClientes[tiempoAtencion].atendido=2;
					tecnico2.libre=0;
					pthread_mutex_unlock(&semaforoColaClientes);

					if(tecnico2.clientesAtendidos==5){
						sleep(5);
						tecnico2.clientesAtendidos=0;
					}
				}
			}
		}
	}
}

/*COMPROBAR SI ESTA BIEN O USAR LA OTRA(mas abajo)*/
// void *accionesEncargado(void *arg){
// 	int i;
// 	bool atendiendo=true; //bandera que indica si está atendiendo a un cliente o no
	
// 	while(atendiendo){
// 		//espera a que haya clientes disponibles
// 		while(nClientes==0){
// 			sleep(3);
// 		}
// 		//busca al cliente para atender primero de red y si no hubiera de tipo app
// 		int clienteAtender=-1;
// 		pthread_mutex_lock(&semaforoColaClientes);
// 		for(i=0; i<nClientes; i++){
// 			if(arrayClientes[i].atendido==0){ //si el cliente no ha sido atendido
// 				if(strcmp(arrayClientes[i].tipo, "Red")==0){ //si es de tipo red
// 					clienteAtender=i;
// 					break;
// 				}
// 				else if(strcmp(arrayClientes[i].tipo, "App")==0 && clienteAtender==-1){ //si es de tipo app y no se ha encontrado aún un cliente de red
// 					clienteAtender=i;
// 				}	
// 			}
// 		}
// 		if(clienteAtender==-1){ //si no se ha encontrado ningún cliente de tipo red o app
// 			//atiende a la prioridad y sino al que más tiempo lleve esperando
// 			int tiempoEspera=0;
// 			for(i=0; i<nClientes; i++){
// 				if(arrayClientes[i].atendido==0){ //si el cliente no ha sido atendido
// 					if(arrayClientes[i].prioridad>tiempoEspera){ //si tiene mayor prioridad
// 						tiempoEspera=arrayClientes[i].prioridad;
// 						clienteAtender=i;
// 					}
// 					else if(arrayClientes[i].prioridad==tiempoEspera){ //si tiene la misma prioridad
// 						if(arrayClientes[i].id>arrayClientes[clienteAtender].id){ //si lleva más tiempo esperando
// 							clienteAtender=i;
// 						}
// 					}
// 				}
// 			}
// 		}
// 		if(clienteAtender!=-1){ //si se ha encontrado un cliente para atender
// 			arrayClientes[clienteAtender].atendido=1; //marca el cliente como siendo atendido
// 			pthread_mutex_unlock(&semaforoColaClientes);

// 			//calculamos el tipo de atención y en función de esto el tiempo de atención (el 80%, 10%, 10%)
// 			int tiempoAtencion;
// 			if(arrayClientes[clienteAtender].solicitud==1){ //si el cliente ha solicitado atención domiciliaria
// 				tiempoAtencion=(80 * rand())/RAND_MAX + 1;
// 			}
// 			else{ //si el cliente no ha solicitado atención domiciliaria
//   				if(strcmp(arrayClientes[clienteAtender].tipo, "App")==0){ //si es de tipo app
//     			tiempoAtencion=(10 * rand())/RAND_MAX + 1;
//   				}
//   				else{ //si es de tipo red
//     				tiempoAtencion=(20 * rand())/RAND_MAX + 1;
//   				}
// 			}
// 			//guardar en el log que comienza la atención
// 			pthread_mutex_lock(&semaforoFichero);
// 			fprintf(logFile, "El encargado comienza a atender al cliente %s\n", arrayClientes[clienteAtender].id);
// 			pthread_mutex_unlock(&semaforoFichero);

// 			//dormimos el tiempo de atención
// 			sleep(tiempoAtencion);

// 			//guardar en el log que finaliza la atención
// 			pthread_mutex_lock(&semaforoFichero);
// 			fprintf(logFile, "El encargado ha finalizado la atención al cliente %s\n", arrayClientes[clienteAtender].id);
// 			pthread_mutex_unlock(&semaforoFichero);

// 			//guardar en el log el motivo del fin de la atención
// 			if(arrayClientes[clienteAtender].solicitud == 1){
// 				pthread_mutex_lock(&semaforoFichero);
// 				fprintf(logFile, "%s\n", atencionFinaliza);
// 				pthread_mutex_unlock(&semaforoFichero);
// 			}
// 			else if(arrayClientes[clienteAtender].prioridad == 0){
// 				pthread_mutex_lock(&semaforoFichero);
// 				fprintf(logFile, "%s\n", muchaEspera);
// 				pthread_mutex_unlock(&semaforoFichero);
// 			}
// 		}
// 	}
// }

void *accionesEncargado(void *arg){
	struct trabajador tecnico1;
	struct trabajador tecnico2;
	struct trabajador responsable1;
	struct trabajador responsable2;
	
	int prio=0;
	int prio2=0;
	bool atendiendo=true; //bandera que indica si está atendiendo a un cliente o no
	

	while(atendiendo){
		if(nClientesRed==0){		//no hay clientes de red
			if(nClientesApp==0){		//no hay clientes de red ni de app
				sleep(3);		//espera 3 segundos
			}
			else{
				while(tecnico1.libre==1){		//indicamos que el tecnico 1 esta ocupado
					while(tecnico2.libre==1){		//indicamos que el tecnico 2 esta ocupado
						pthread_mutex_lock(&semaforoColaClientes);
						prio=mayorPrioridad();		//utilizando la funcion auxiliar "mayorPrioridad" calculas el cliente con mayor prioridad, o en el caso de teener todos la misma, el que mas tiempo ha esperado
						arrayClientes[prio].atendido=1;		//ponemos el atendido a 1, que indica que esta siendo atendido
						pthread_mutex_unlock(&semaforoColaClientes);

						int tempAten=calculaAleatorios(1, 100);		//usamos la funcion "calculaAleatorios" conseguir un numero aleatorio del 1 al 100

						if(tempAten<81){		//si el numero es del 1 al 80
							int tiempo=calculaAleatorios(1,4);		//calculamos aleatorio del 1 al 4 incluidos

							pthread_mutex_lock(&semaforoFichero);		//bloqueamos el acceso a fichero
							writeLogMessage(clienteEmpiezaAtendido, arrayClientes[prio].id);		//escribimos el mensaje de que empieza a atender en el log
							pthread_mutex_unlock(&semaforoFichero);		//desbloqueamos el acceso a fichero

							sleep(tiempo);		//dormimos el tiempo que nos dio el aleatorio

							pthread_mutex_lock(&semaforoFichero);
							writeLogMessage(clienteFinalizaAtencion, arrayClientes[prio].id);	//escribimos el mensaje de que el cliente ha acabado
							pthread_mutex_unlock(&semaforoFichero);

							pthread_mutex_lock(&semaforoFichero);
							writeLogMessage(todoEnRegla, arrayClientes[prio].id);		//rescribimos el mensaje de que el cliente tiene todo en regla
							pthread_mutex_unlock(&semaforoFichero);

							pthread_mutex_lock(&semaforoColaClientes);
							arrayClientes[prio].atendido=2;		//cambiamos el estado a ya atendido
							pthread_mutex_unlock(&semaforoColaClientes);

						}else if(tempAten>80&&tempAten<91){		//si el numero esta entre el 81 y 90 incluidos
							int tiempo=calculaAleatorios(2,6);		//usamos la funcion "calculaAleatorios" conseguir un numero aleatorio del 2 al 6 incluidos

							pthread_mutex_lock(&semaforoFichero);
							writeLogMessage(clienteEmpiezaAtendido, arrayClientes[prio].id);
							pthread_mutex_unlock(&semaforoFichero);

							sleep(tiempo);

							pthread_mutex_lock(&semaforoFichero);
							writeLogMessage(clienteFinalizaAtencion, arrayClientes[prio].id);
							pthread_mutex_unlock(&semaforoFichero);

							pthread_mutex_lock(&semaforoFichero);
							writeLogMessage(malIdentificados, arrayClientes[prio].id);
							pthread_mutex_unlock(&semaforoFichero);

							pthread_mutex_lock(&semaforoColaClientes);
							arrayClientes[prio].atendido=2;
							pthread_mutex_unlock(&semaforoColaClientes);

						}else{		//si el numero esta entre el 91 y 100
							int tiempo=calculaAleatorios(1,2);		//usamos la funcion "calculaAleatorios" conseguir un numero aleatorio entre 1 y 2

							pthread_mutex_lock(&semaforoFichero);
							writeLogMessage(clienteEmpiezaAtendido, arrayClientes[prio].id);
							pthread_mutex_unlock(&semaforoFichero);

							sleep(tiempo);

							pthread_mutex_lock(&semaforoFichero);
							writeLogMessage(clienteFinalizaAtencion, arrayClientes[prio].id);
							pthread_mutex_unlock(&semaforoFichero);

							pthread_mutex_lock(&semaforoFichero);
							writeLogMessage(confusionCompañia, arrayClientes[prio].id);
							pthread_mutex_unlock(&semaforoFichero);

							pthread_mutex_lock(&semaforoColaClientes);
							arrayClientes[prio].atendido=2;
							pthread_mutex_unlock(&semaforoColaClientes);
						}
					}
				}
			}
		}
		else{
			if(nClientesApp==0){		//si hay clientes de red pero no de app
				while(responsable1.libre==1){		//indicamos que el responsable 1 esta ocupado
					while(responsable2.libre==1){		//indicamos que el responsable 2 esta ocupado
						pthread_mutex_lock(&semaforoColaClientes);
						prio2=mayorPrioridadRed();
						arrayClientes[prio2].atendido=1;
						pthread_mutex_unlock(&semaforoColaClientes);

						int tempAten=calculaAleatorios(1, 100);

						if(tempAten<81){
							int tiempo=calculaAleatorios(1,4);

							pthread_mutex_lock(&semaforoFichero);
							writeLogMessage(clienteEmpiezaAtendido, arrayClientes[prio2].id);
							pthread_mutex_unlock(&semaforoFichero);

							sleep(tiempo);

							pthread_mutex_lock(&semaforoFichero);
							writeLogMessage(clienteFinalizaAtencion, arrayClientes[prio2].id);
							pthread_mutex_unlock(&semaforoFichero);

							pthread_mutex_lock(&semaforoFichero);
							writeLogMessage(todoEnRegla, arrayClientes[prio2].id);
							pthread_mutex_unlock(&semaforoFichero);

							pthread_mutex_lock(&semaforoColaClientes);
							arrayClientes[prio2].atendido=2;
							pthread_mutex_unlock(&semaforoColaClientes);

						}else if(tempAten>80&&tempAten<91){
							int tiempo=calculaAleatorios(2,6);

							pthread_mutex_lock(&semaforoFichero);
							writeLogMessage(clienteEmpiezaAtendido, arrayClientes[prio2].id);
							pthread_mutex_unlock(&semaforoFichero);

							sleep(tiempo);

							pthread_mutex_lock(&semaforoFichero);
							writeLogMessage(clienteFinalizaAtencion, arrayClientes[prio2].id);
							pthread_mutex_unlock(&semaforoFichero);

							pthread_mutex_lock(&semaforoFichero);
							writeLogMessage(malIdentificados, arrayClientes[prio2].id);
							pthread_mutex_unlock(&semaforoFichero);

							pthread_mutex_lock(&semaforoColaClientes);
							arrayClientes[prio2].atendido=2;
							pthread_mutex_unlock(&semaforoColaClientes);

						}else{
							int tiempo=calculaAleatorios(1,2);

							pthread_mutex_lock(&semaforoFichero);
							writeLogMessage(clienteEmpiezaAtendido, arrayClientes[prio2].id);
							pthread_mutex_unlock(&semaforoFichero);

							sleep(tiempo);

							pthread_mutex_lock(&semaforoFichero);
							writeLogMessage(clienteFinalizaAtencion, arrayClientes[prio2].id);
							pthread_mutex_unlock(&semaforoFichero);

							pthread_mutex_lock(&semaforoFichero);
							writeLogMessage(confusionCompañia, arrayClientes[prio2].id);
							pthread_mutex_unlock(&semaforoFichero);

							pthread_mutex_lock(&semaforoColaClientes);
							arrayClientes[prio2].atendido=2;
							pthread_mutex_unlock(&semaforoColaClientes);
						}
					}
				}
			}
		}
	}		
}

void accionesTecnicoDomiciliario(){
	for(int i=0; i<nSolicitudesDomiciliarias; i++ ){
		//hacer el apso 1

		pthread_mutex_lock(&semaforoFichero);
		writeLogMessage(atencionDomiciliariaText, arrayClientesSolicitudes[tiempoAtencion].id);
		pthread_mutex_unlock(&semaforoFichero);

		for(int i=0; i<nSolicitudesDomiciliarias; i++){
			sleep(1);
		}

		pthread_mutex_lock(&semaforoFichero);
		writeLogMessage(atencionDomiciliariaAtendido, arrayClientesSolicitudes[tiempoAtencion].id);
		pthread_mutex_unlock(&semaforoFichero);
		nSolicitudesDomiciliarias
	}


}
//:::::::::::::::::::::::: NO DEFINITIVA, HA SIDO A VOLEO ::::::::::::::::::::::::::::::::::::::::::::::
/*Función que finaliza el programa al recibir la señal SIGINT*/
void finalizarPrograma (int signal) {

	//Antes de que finalice se debe terminar de atender a todos los clientes en cola
	/*
		Hilos tienen que terminar de forma correcta, se libera memoria, etc.
	*/

	printf("\nSe ha recibido la señal SIGINT. Procediendo a finalizar el programa...\n");

	/*Se escribe en el log que se va a finalizar el programa*/
	pthread_mutex_lock(&semaforoFichero);
	writeLogMessage(signalFinalizacion, finPrograma);
	pthread_mutex_unlock(&semaforoFichero);

	pthread_mutex_lock(&semaforoColaClientes);
	pthread_mutex_lock(&semaforoSolicitudes);

	//Se ponen las solicitudes domiciliarias a 0
	nSolicitudesDomiciliarias = 0;

	//A cada cliente se le cambia la flag de solicitud a 0 y espera a que termine de ser atendido para finalizar los hilos de los trabajadores
	for (int i = 0; i < nClientes; i++) {
		arrayClientes[i].solicitud = 0;
		while (arrayClientes[i].atendido == 1) {
			sleep(2);
		}
	}

	pthread_mutex_unlock(&semaforoColaClientes);
	pthread_mutex_unlock(&semaforoSolicitudes);

	//pthread_mutex_destroy(); <-- No estoy seguro de si hace falta

	/* LIBERACIÓN DE MEMORIA */
	free(arrayHilosClientes);
	free(arrayClientes);
	free(arrayClientesSolicitudes);
	//Puede que si sea necesario liberar memoria de los char* cuando se inicializan las estructuras en el main, no estoy seguro, depende de si da fallo en ejecución

	/*Se matan los hilos de los trabajadores*/
	pthread_cancel(tecnico_1);
	pthread_cancel(tecnico_2);
	pthread_cancel(responsable_1);
	pthread_cancel(responsable_2);
	pthread_cancel(encargado_);
	pthread_cancel(atencionDomiciliaria_);

	printf("\nAdiós!\n");

	exit(0);
}

/****************************************** FUNCIONES AUXILIARES ******************************************/
int mayorPrioridad(){
	int mayorPrioridad=-999;
	int pos=0;
	for(int i=0; i<nClientes; i++){
		if(arrayClientes[i].tipo="App"){
			//Comprobamos solo la prioridad porque al ser un array por orden de llegada el primero va a ser el que devuelva, es decir el que mas tiempo lleve esperando
			if(mayorPrioridad<arrayClientes[i].prioridad){
				mayorPrioridad=arrayClientes[i].prioridad;
				pos=i;
			}
		}

	}
	return pos;
}

int mayorPrioridadRed(){
	int mayorPrioridad=-999;
	int posRed=0;
	for(int i=0; i<nClientes; i++){
		if(arrayClientes[i].tipo="Red"){
			//Comprobamos solo la prioridad porque al ser un array por orden de llegada el primero va a ser el que devuelva, es decir el que mas tiempo lleve esperando
			if(mayorPrioridad<arrayClientes[i].prioridad){
				mayorPrioridad=arrayClientes[i].prioridad;
				posRed=i;
			}
		}

	}
	return posRed;
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

/*Función que escribe en el log cuando un cliente se va antes de ser atendido*/
//Cambiar nombre a logClienteFuera para que se entienda mejor en otras funciones
void clienteFuera(char *id, char *razon) {
	pthread_mutex_lock(&semaforoFichero);
	writeLogMessage(id, razon); //Escribe el id del cliente y el char "appDificil" 
	pthread_mutex_unlock(&semaforoFichero);
}

/*Función que elimina un cliente del array de clientes*/
void borrarCliente (int posicionCliente) {

	for (int i = posicionCliente; i < (nClientes - 1); i++) {
		arrayClientes[i] = arrayClientes[i + 1]; //El cliente en cuestión se iguala al de la siguiente posición (inicializado todo a 0)
	}

	struct cliente structVacia; //Se declara un cliente vacío
	arrayClientes[nClientes - 1] = structVacia; //El cliente pasa a estar vació (todo a 0)

	nClientes--;
}

/*Función que calcula números aleatorios*/
int calculaAleatorios(int min, int max) {
	srand(time(NULL));
	
	return rand() % (max - min + 1) + min;
}


int main(int argc, char* argv[]) {
	
	/* INICIALIZACIÓN DE RECURSOS */
	arrayHilosClientes = (pthread_t *) malloc (nClientes * sizeof(struct cliente *)); //Array dinámico de hilos de clientes
	arrayClientes = (struct cliente *) malloc (nClientes * sizeof(struct cliente *)); //array del total de clientes
	arrayClientesSolicitudes = (struct cliente *) malloc (4 * sizeof(struct cliente *)); // array de clientes con solicitudes (4 para que salga el responsable)
	
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

	//Contador de solicitudes domiciliarias
	int nSolicitudesDomiciliarias = 0;

	//Lista de tipos de clientes
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

	//Lista de trabajadores
	/*Técnicos*/
	struct trabajador *tecnico1;
	tecnico1 -> id = (char *) malloc (10  * sizeof(char));
	tecnico1 -> id = "tecnico1";

	tecnico1 -> clientesAtendidos = 0;	//No estoy seguro si hace falta inicializar a 0 esto o ya lo está
	
	tecnico1 -> tipo = (char *) malloc (8 * sizeof(char));
	tecnico1 -> tipo = "Tecnico";

	tecnico1 -> libre = 0;	//No estoy seguro si hace falta inicializar a 0 esto o ya lo está

	struct trabajador *tecnico2;
	tecnico2 -> id = (char *) malloc (10  * sizeof(char));
	tecnico2 -> id = "tecnico2";

	tecnico2 -> clientesAtendidos = 0;	//No estoy seguro si hace falta inicializar a 0 esto o ya lo está
	
	tecnico2 -> tipo = (char *) malloc (8 * sizeof(char));
	tecnico2 -> tipo = "Tecnico";

	tecnico2 -> libre = 0;	//No estoy seguro si hace falta inicializar a 0 esto o ya lo está

	/*Responsables de reparaciones*/
	struct trabajador *responsable1;
	responsable1 -> id = (char *) malloc (14  * sizeof(char));
	responsable1 -> id = "responsable1";

	responsable1 -> clientesAtendidos = 0;	//No estoy seguro si hace falta inicializar a 0 esto o ya lo está
	
	responsable1 -> tipo = (char *) malloc (20 * sizeof(char));
	responsable1 -> tipo = "Resp. Reparaciones";

	responsable1 -> libre = 0;	//No estoy seguro si hace falta inicializar a 0 esto o ya lo está

	struct trabajador *responsable2;
	responsable2 -> id = (char *) malloc (14  * sizeof(char));
	responsable2 -> id = "responsable2";

	responsable2 -> clientesAtendidos = 0;	//No estoy seguro si hace falta inicializar a 0 esto o ya lo está
	
	responsable2 -> tipo = (char *) malloc (20 * sizeof(char));
	responsable2 -> tipo = "Resp. Reparaciones";

	responsable2 -> libre = 0;	//No estoy seguro si hace falta inicializar a 0 esto o ya lo está

	/*Encargado*/
	struct trabajador *encargado;
	encargado -> id = (char *) malloc (10 * sizeof(char));
	encargado -> id = "encargado";

	encargado -> clientesAtendidos = 0;	//No estoy seguro si hace falta inicializar a 0 esto o ya lo está

	encargado -> tipo = (char *) malloc (10 * sizeof(char));
	encargado -> tipo = "Encargado";

	encargado -> libre = 0;	//No estoy seguro si hace falta inicializar a 0 esto o ya lo está

	//Fichero de log
	logFile = fopen("registroTiempos.log", "wt"); //Opcion "wt" = Cada vez que se ejecuta el programa, si el archivo ya existe se elimina su contenido para empezar de cero
	fclose(logFile);

	/*Explicación por pantalla del funcionamiento del programa*/
	printf("****************************** FUNCIONAMIENTO DEL PROGRAMA ******************************\n\n");
	printf("Introduzca 'kill -10 %d' en otra terminal para añadir un cliende de la app.\n", getpid());
	printf("Introduzca 'kill -12 %d' en otra terminal para añadir un cliende de reparación de red.\n", getpid());
	printf("Introduzca 'kill -2 %d' en otra terminal para finalizar el programa.\n", getpid());

	/*Escribe al principio del archivo de log que se inicia la atención a los clientes*/
	writeLogMessage(nombrePrograma, iniciaPrograma); 

	/* CREACIÓN DE HILOS DE TECNICOS, RESPONSABLES, ENCARGADO Y ATENCION DOMICILIARIA */
	//Se pasa como argumento la estructura del trabajador que ejecuta el hilo
	pthread_create(&tecnico_1, NULL, accionesTecnico, (void *) tecnico1);
	pthread_create(&tecnico_2, NULL, accionesTecnico, (void *) tecnico2);
	pthread_create(&responsable_1, NULL, accionesTecnico, (void *) responsable1);
	pthread_create(&responsable_2, NULL, accionesTecnico, (void *) responsable2);
	pthread_create(&encargado_, NULL, accionesEncargado, (void *) encargado);
	pthread_create(&atencionDomiciliaria_, NULL, accionesTecnicoDomiciliario, NULL);/*Este hilo no lo ejecuta ningún trabajador en particular*/

	/* ESPERAR POR SEÑALES INFINITAMENTE */
	while(1) {
		pause();
	}

	return 0;
}
