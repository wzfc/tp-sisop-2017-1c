#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "commons/log.h"
#include "commons/config.h"

#define MOSTRAR_LOGS_EN_PANTALLA true

#define RUTA_CONFIG "config.cfg"
#define RUTA_LOG "consola.log"

t_log* logger;
t_config* config;

#define PORT "8081"   // port we're listening on

#define MAX_NUM_CLIENTES 100

enum CodigoDeOperacion {
    MENSAJE, CONSOLA, MEMORIA, FILESYSTEM, CPU
};

#define ID_CLIENTE(x) ID_CLIENTES[x-1]
static const char *ID_CLIENTES[] = {
	"Consola", "Memoria", "File System", "CPU"
};

// Para definirlo se puede usar tanto "headerDeLosRipeados" como "struct headerDeLosRipeados"
typedef struct headerDeLosRipeados {
    unsigned short bytesDePayload;
    char codigoDeOperacion; // 0 (mensaje). Handshake: 1 (consola), 2 (memoria), 3 (filesystem), 4 (cpu), 5 (kernel)
}__attribute__((packed, aligned(1))) headerDeLosRipeados;

// Para definirlo se puede usar tanto "miCliente" como "struct miCliente"
typedef struct miCliente {
    short socketCliente;
    char identificador;
} miCliente;

void limpiarClientes(miCliente *);
void analizarCodigosDeOperacion(int , char , miCliente *);
int analizarHeader(int , char* , miCliente *);
void leerMensaje(int , short , miCliente *);
void cerrarConexion(int , char* );
int posicionSocket(int , miCliente *);
void agregarCliente(char , int , miCliente *);
void borrarCliente(int , miCliente *);
void serializarHeader(headerDeLosRipeados *, char *);
void deserializarHeader(headerDeLosRipeados *, char *);
// void handshake(int , char );

void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(void) {
	miCliente _misClientes[MAX_NUM_CLIENTES];
	miCliente *misClientes = _misClientes;
	//miCliente* misClientes = malloc(sizeof(miCliente)*MAX_NUM_CLIENTES);
    limpiarClientes(misClientes);

    fd_set master;
    fd_set read_fds;
    int fdmax;

    int listener;
    int newfd;
    struct sockaddr_storage remoteaddr;
    socklen_t addrlen;

    int buffersize = sizeof(headerDeLosRipeados);
    char* buffer = malloc(buffersize);

    int bytesRecibidos;

    int i;

    FD_ZERO(&master);
    FD_ZERO(&read_fds);

    struct addrinfo hints; // Le da una idea al getaddrinfo() el tipo de info que debe retornar
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET; // IPv4
    hints.ai_socktype = SOCK_STREAM; // TCP
    hints.ai_flags = AI_PASSIVE;

    int rv;
	struct addrinfo *ai;
    if ((rv = getaddrinfo(NULL, PORT, &hints, &ai)) != 0) {
        fprintf(stderr, "selectserver: %s\n", gai_strerror(rv));
        exit(1);
    }

    struct addrinfo *p; // Puntero para recorrer la lista
    for (p = ai; p != NULL; p = p->ai_next) {
        listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (listener < 0) {
            continue;
        }

        int activado = 1;
        setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &activado, sizeof(activado));

        if (bind(listener, p->ai_addr, p->ai_addrlen) < 0) {
            close(listener);
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "Fallo el bind\n");
        exit(2);
    }

    freeaddrinfo(ai);

    if (listen(listener, 10) == -1) {
        perror("listen");
        exit(3);
    }
    printf("Estoy escuchando\n");
    FD_SET(listener, &master);

    fdmax = listener;

    for(;;) {
        read_fds = master;
        if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1) {
            perror("select");
            exit(4);
        }

        for(i = 0; i <= fdmax; i++) {
        	if (FD_ISSET(i, &read_fds) == 0) {
        		continue;
        	}
			if (i == listener) {
				addrlen = sizeof remoteaddr;
				newfd = accept(listener, (struct sockaddr *) &remoteaddr,
						&addrlen);

				if (newfd == -1) {
					perror("accept");
				}
				else {
					FD_SET(newfd, &master);
					if (newfd > fdmax) {
						fdmax = newfd;
					}
				    char remoteIP[INET_ADDRSTRLEN]; // string que contiene la direccion IP del cliente
					inet_ntop(AF_INET,	get_in_addr((struct sockaddr*) &remoteaddr), remoteIP, INET_ADDRSTRLEN);
					printf("Nueva conexión desde %s en el socket %d\n", remoteIP, newfd);
				}
			}
			else {
				if ((bytesRecibidos = recv(i, buffer, buffersize + 1, 0))
						<= 0) {
					if (bytesRecibidos == 0) {
						printf("selectserver: socket %d hung up\n", i);
					}
					else {
						perror("recv");
					}
					borrarCliente(i, misClientes);
					cerrarConexion(i, "time-out");
					FD_CLR(i, &master);
				}
				else {
					// llegó info, vamos a ver el header
					int estado = analizarHeader(i, buffer, misClientes);

					if (estado < 0) { //ocurrió algún problema con ese socket (puede ser un cliente o no)
						send(i, "Error, desconectado", 20, 0);
						borrarCliente(i, misClientes);
						cerrarConexion(i, "operación incorrecta");
						FD_CLR(i, &master);
						continue; //salteamos esta iteración
					}
					char *respuesta = "Header recibido";
					send(i, respuesta, strlen(respuesta) + 1, 0);
				}
			}
        }
    }
    free(buffer);
    //free(misClientes);
    return 0;
}

void agregarCliente(char identificador, int socketCliente, miCliente *clientes) {
	int i;
	for (i=0; i<MAX_NUM_CLIENTES; i++) {
		if (clientes[i].socketCliente == -1) { //woohoo, encontramos un espacio libre
			clientes[i].socketCliente = socketCliente;
			clientes[i].identificador = identificador;
			break;
		}
	}
}

void borrarCliente(int socketCliente, miCliente *clientes) {
	int posicion = posicionSocket(socketCliente,clientes);
	if (posicion >= 0) {
		clientes[posicion].socketCliente = -1;
		clientes[posicion].identificador = 255;
	}
}

/**
 * Analiza el contenido del header, y respecto a ello realiza distintas acciones
 * devuelve -1 si el socket causa problemas
 */
int analizarHeader(int socketCliente, char* buffer, miCliente *clientes) {
    headerDeLosRipeados header;
    deserializarHeader(&header, buffer);
    int indice = posicionSocket(socketCliente,clientes);

    if (header.codigoDeOperacion >= CONSOLA && header.codigoDeOperacion <= CPU) {
    	// No estaba antes en el array de clientes
    	if (indice < 0) {
    		printf("El nuevo cliente fue identificado como: %s\n", ID_CLIENTE(header.codigoDeOperacion));
    		agregarCliente(header.codigoDeOperacion,socketCliente,clientes);
    	}
    	else { //No se puede enviar un handshake 2 veces (el cliente ya estaba en el array de clientes)
    		return -1; // Otro cacho de código se va a encargar de borrarlo
    	}
    }

    else if (header.codigoDeOperacion == MENSAJE) {
        if (header.bytesDePayload <= 0) {
            printf("El cliente %i intento mandar un mensaje sin contenido\n", socketCliente);
            return -1;
        }
        else {
            leerMensaje(socketCliente, header.bytesDePayload, clientes);
        }
    }

    else {
    	if (indice >= 0) { //Si se encontró el cliente en la estructura de clientes (osea ya hizo handshake)
    		analizarCodigosDeOperacion(socketCliente, header.codigoDeOperacion, clientes); // TODO
    	}
    	else { //Header no reconocido, chau cliente intruso
    		return -1;
    	}
    }
    return 0;
}

int enviarMensajeATodos(int socketCliente, char* mensaje, miCliente *clientes) {
	int cantidad = 0;
	int i;
	for (i=0;i<MAX_NUM_CLIENTES;i++) { //enviamos mensaje a los clientes registrados
		if (clientes[i].identificador >= MEMORIA && clientes[i].identificador <= CPU) { //solo le mandamos a MEMORIA,FILESYSTEM y CPU
			send(clientes[i].socketCliente,mensaje,strlen(mensaje),0);
			cantidad++;
		}
	}
	return cantidad;
}

void leerMensaje(int socketCliente, short bytesDePayload, miCliente *clientes) {
    char* mensaje = malloc(bytesDePayload+1);
    recv(socketCliente,mensaje,bytesDePayload,0);
    mensaje[bytesDePayload]='\0';
    printf("Mensaje recibido: %s\n",mensaje);
    int cantidad = enviarMensajeATodos(socketCliente,mensaje, clientes);
    printf("Mensaje retransmitido a %i clientes\n",cantidad);
    free(mensaje);
}

void limpiarClientes(miCliente *clientes) {
    int i;
    for(i = 0; i < MAX_NUM_CLIENTES; i++) {
        clientes[i].socketCliente = -1;
        clientes[i].identificador = 255;
    }
}

int posicionSocket(int socketCliente, miCliente *clientes) {
    int i;
    for(i = 0; i < MAX_NUM_CLIENTES; i++) {
        if(clientes[i].socketCliente == socketCliente) {
            return i;
        }
    }
    return -1;
}

void analizarCodigosDeOperacion(int socketCliente, char codigoDeOperacion, miCliente *clientes) {
    char codigoDelCliente = clientes[posicionSocket(socketCliente, clientes)].identificador;
    switch(codigoDelCliente) {
        case CONSOLA:
            // TODO
            break;
        case MEMORIA:
            // TODO
            break;
        case FILESYSTEM:
            // TODO
            break;
        case CPU:
            // TODO
            break;
        default:
            printf("TODO, cod. de operación recibido: %d\n",codigoDeOperacion);
            // TODO
    }
}

void cerrarConexion(int socketCliente, char* motivo){
	close(socketCliente);
	printf("El socket %i ha sido desconectado por %s\n", socketCliente, motivo);
}

/*
// Por ahora no lo usamos
void handshake(int socket, char operacion) {
	printf("Conectando a servidor 0%%\n");
	headerDeLosRipeados handy;
	handy.bytesDePayload = 0;
	handy.codigoDeOperacion = operacion;

	int buffersize = sizeof(headerDeLosRipeados);
	char *buffer = malloc(buffersize);
	serializarHeader(&handy, buffer);
	send(socket, (void*) buffer, buffersize, 0);

	char respuesta[1024];
	int bytesRecibidos = recv(socket, (void *) &respuesta, sizeof(respuesta), 0);

	if (bytesRecibidos > 0) {
		printf("Conectado a servidor 100 porciento\n");
		printf("Mensaje del servidor: \"%s\"\n", respuesta);
	}
	else {
		printf("Ripeaste\n");
		exit(0);
	}
	free(buffer);
}
*/

void serializarHeader(headerDeLosRipeados *header, char *buffer) {
	short *pBytesDePayload = (short*) buffer;
	*pBytesDePayload = header->bytesDePayload;
	char *pCodigoDeOperacion = (char*)(pBytesDePayload + 1);
	*pCodigoDeOperacion = header->codigoDeOperacion;
}

void deserializarHeader(headerDeLosRipeados *header, char *buffer) {
	short *pBytesDePayload = (short*) buffer;
	header->bytesDePayload = *pBytesDePayload;
	char *pCodigoDeOperacion = (char*)(pBytesDePayload + 1);
	header->codigoDeOperacion = *pCodigoDeOperacion;
}
