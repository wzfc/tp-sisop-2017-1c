#ifndef OP_MEMORIA_H_
#define OP_MEMORIA_H_

	#include <string.h>
	#include <sys/socket.h>
	#include <netinet/in.h>

	#include <qepd/qepd.h>

	/* Socket de escucha de proceso memoria */
	extern int socket_memoria;

	/* Tamanio de paginas (marcos) */
	extern int tamanio_pagina;

	/* IP de memoria */
	extern char IP_MEMORIA[16];

	/* Puerto de memoria */
	extern int PUERTO_MEMORIA;
	/* Cantidad de pags de Stack */
	extern int STACK_SIZE;

	/* Operaciones de la memoria */
	typedef enum PACKED {
		MEM_INICIALIZAR_PROGRAMA,
		MEM_ASIGNAR_PAGINAS,
		MEM_FINALIZAR_PROGRAMA,
		MEM_SOLICITAR_BYTES,
		MEM_ALMACENAR_BYTES,
		MEM_LIBERAR_PAGINA
	} op_mem;

	/* Mensaje de confirmacion */
	typedef enum PACKED {
		OK,
		ERROR
	} respuesta_op_mem;

	/* Pedido de inicializacion de programa */
	typedef struct {
		op_mem operacion;
		int PID;
		int paginas_codigo;
		int paginas_stack;
	} PACKED PedidoInicializar;

	/* Pedido de asignacion de paginas */
	typedef struct {
		op_mem operacion;
		int PID;
		int paginas;
	} PACKED PedidoAsignacion;

	typedef struct {
		op_mem operacion;
		int PID;
		int numero_pagina;
	} PACKED PedidoLiberacion;

	/* Finalizacion de programa */
	typedef struct {
		op_mem operacion;
		int PID;
	} PACKED PedidoFinalizacion;

	typedef struct {
		op_mem operacion;
		int PID;
		int numero_pagina;
		int offset;
		int size;
	} PACKED PedidoLectura;

	typedef struct {
		op_mem operacion;
		int PID;
		int numero_pagina;
		int offset;
		int size;
		void *datos;
	} PACKED PedidoEscritura;

	/**
	 * Establecer conexion con la memoria.
	 * Modifica las siguientes variable globales:
	 * 	- socket_memoria
	 * 	- tamanio_pagina
	 */
	void mem_conectar(void);

	/**
	 * Inicializar programa
	 *
	 * Parámetros:
	 * 		[Identificador del Programa]
	 * 		[Tamaño de programa]
	 * 		[Contenido de programa]
	 */
	int mem_inicializar_programa(int PID, size_t size, void *datos);

	/**
	 * Solicitar bytes de una página
	 *
	 * Parámetros:
	 * 		[Identificador del Programa]
	 * 		[#página]
	 * 		[offset]
	 * 		[tamaño]
	 */
	char *mem_leer_bytes(int PID, int pagina, off_t offset, size_t size);

	/**
	 * Almacenar bytes en una página
	 *
	 * Parámetros:
	 * 		[Identificador del Programa]
	 * 		[#página]
	 * 		[offset]
	 * 		[tamaño]
	 * 		[buffer]
	 */
	int mem_escribir_bytes(int PID, int pagina, off_t offset, size_t size, void *buffer);

	/**
	 * Asignar Páginas a Proceso
	 * Parámetros:
	 * 		[Identificador del Programa]
	 * 		[Páginas requeridas]
	 */
	int mem_asignar_paginas(int PID, int paginas);

	/**
	 * Finalizar programa
	 * Parámetros:
	 * 		[Identificador del Programa]
	 */
	int mem_finalizar_programa(int PID);

#endif /* OP_MEMORIA_H_ */
