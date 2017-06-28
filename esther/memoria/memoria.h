/*
 * memoria.h
 *
 *  Created on: 27/6/2017
 *      Author: utnso
 */

#ifndef MEMORIA_H_
#define MEMORIA_H_

	#include <stdio.h>
	#include <stddef.h>
	#include <stdlib.h>
	#include "qepd/qepd.h"

	typedef u_int32_t t_puntero;

	typedef struct estructuraAdministrativa_cache {
		int pid;
		int pag;
		short int lru;
		int contenido_pagina; //Numero de byte a partir del cual empieza la pag en cuestion. Se llamo asi para respetar el enunciado
	} PACKED estructura_administrativa_cache;

	typedef struct estructuraAdministrativa {
		int frame;
		int pid;
		int pag;
	} PACKED estructura_administrativa;

	typedef struct Victima {
		int indice;
		int lru;
	} Victima;

	extern estructura_administrativa_cache *tabla_administrativa_cache;
	extern estructura_administrativa *tabla_administrativa;

	extern char *memoria_cache;
	extern char *memoria;

	extern int MARCO_SIZE;
	extern int MARCOS;
	extern short int RETARDO;

	extern int socket_kernel;

	/*-----------PROTOTIPOS DE FUNCIONES----------*/

	int			asignar_frames_contiguos			(int,int,int,size_t,void*);
	int			asignar_paginas_a_proceso			(int,int);
	int			ultima_pagina_proceso 				(int);
	void		atender_kernel						();
	void 		atender_CPU							(int);
	void		cerrar_conexion						(int,char*);
	void		configurar_retardo					();
	void		crear_memoria						(void);
	void		dump								();
	void 		enviar_excepcion					(int,int);
	void		establecer_configuracion			();
	void*		atender_cliente						(void *);
	void		finalizarPrograma					(int,unsigned short);
	void		flush								();
	void		imprimir_opciones_memoria			();
	void		inicializar_tabla					();
	void		inicializar_tabla_cache				();
	void*		interaccion_memoria					(void*);
	char*		ir_a_frame							(int);
	char*		ir_a_frame_cache					(int);
	void		limpiar_pantalla					();
	void		size								();
	int 		max_LRU								();
	int 		posicion_a_reemplazar_cache			();
	char*		remover_salto_linea					(char*);
	int 		traducir_a_frame					(int,int);

	int 		memoria_inicializar_programa		(int,int);
	char* 		memoria_solicitar_bytes				(int,int,int,int);
	int 		memoria_almacenar_bytes				(int,int,int,int,void*);
	int 		memoria_asignar_paginas				(int,int);
	int 		memoria_liberar_pagina				(int,int);
	int 		memoria_finalizar_programa			(int);
	int 		memoria_handshake					(int);

	int 		cache_buscar_pagina					(int,int,int);
	int 		cache_almacenar_pagina				(int,int,int);
	char*		cache_solicitar_bytes				(int,int,int,int);
	int 		cache_almacenar_bytes				(int,int,int,int,int,void*);

#endif /* MEMORIA_H_ */