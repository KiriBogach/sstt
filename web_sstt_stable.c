#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>

#define VERSION				24
#define BUFSIZE				8096
#define ERROR				42
#define LOG					44
#define NOFICHERO			0
#define NOEXTENSION			""
#define OK					200
#define BAD_REQUEST			400
#define PROHIBIDO			403
#define NOENCONTRADO		404
#define TOO_MANY_REQUESTS	429
#define REQUEST_BUFF_SIZE	8192 // 8KiB
#define DATE_BUFF_SIZE		128  // 128 caracteres
#define COOKIE_BUFF_SIZE	128  // 128 caracteres
#define EXTENSIONS_ENABLED	0 	 // 0: Admite las extensiones en 'extensions'; 1: Permite todo tipo de extension
#define PHP_ENABLED 		1	 // 0: No se ejecuturá php sobre los archivos '.php'; 1: Se ejecutará php
#define PERSISTENT_ENABLED 	1	 // 0: No se ejecuturá php sobre los archivos '.php'; 1: Se ejecutará php
#define PERSISTENT_TIME  	1	 // Tiempo que dura la conexión persistente sin leer nada.
#define COOKIES_ENABLED 	1	 // 0: No se ejecuturá la lógica de cookies; 1: Se ejecutará la lógica de cookies
#define MAX_COOKIE_REQUEST 	10
#define COOKIE_TIMEOUT	 	1	 // 10 minutos como indica en enunciado	
#define DEFAULT_HTML_FILE	"index.html"
#define MY_EMAIL			"kyryl.bogachy%40um.es"

/* Variables globales para la gestión de código en PHP */
int IS_PHP = 0;
char* PHP_PATH = "";
char* PHP_QUERY = "";
/* Variables globales para la gestión de cookies */
int cookie_value = -1;

struct {
	char *ext;
	char *filetype;
} extensions [] = {
	{"gif", "image/gif" },
	{"jpg", "image/jpg" },
	{"jpeg", "image/jpeg"},
	{"png", "image/png" },
	{"ico", "image/ico" },
	{"zip", "image/zip" },
	{"gz",  "image/gz"  },
	{"tar", "image/tar" },
	{"htm", "text/html" },
	{"html", "text/html" },
	{"mp3", "audio/mpeg" },
	{"js", "text/html" },
	{"php", "text/html" },
	{"pde", "text/html" },
	{"ttf", "text/html" },
	{0, 0}
};

void debug(int log_message_type, char *message, char *additional_info, int socket_fd) {
	int fd ;
	char logbuffer[BUFSIZE * 2];

	switch (log_message_type) {
	case ERROR: (void)sprintf(logbuffer, "ERROR: %s:%s Errno = %d exiting pid = %d", message, additional_info, errno, getpid());
		break;
	case PROHIBIDO:
		// Enviar como respuesta 403 Forbidden
		(void)sprintf(logbuffer, "FORBIDDEN: %s : %s", message, additional_info);
		break;
	case NOENCONTRADO:
		// Enviar como respuesta 404 Not Found
		(void)sprintf(logbuffer, "NOT FOUND: %s : %s", message, additional_info);
		break;
	case LOG: (void)sprintf(logbuffer, "INFO-> socket: %d | PID: %d | %s | %s", socket_fd, getpid(), message, additional_info); break;
	}

	if ((fd = open("webserver.log", O_CREAT | O_WRONLY | O_APPEND, 0644)) >= 0) {
		(void)write(fd, logbuffer, strlen(logbuffer));
		(void)write(fd, "\n", 1);
		(void)close(fd);
	}
	if (log_message_type == ERROR || log_message_type == NOENCONTRADO || log_message_type == PROHIBIDO) exit(3);
}

char* date_as_string(int incremento_minutos) {
	char buffer[DATE_BUFF_SIZE];
	time_t tiempo = time(&tiempo);     // Segundos desde el 1 de Enero del 1970
	tiempo += incremento_minutos * 60; // Minutos * 60 segundos
	struct tm *tm = gmtime(&tiempo);   // Estructura tm que contiene los datos parseados de la fecha
	strftime(buffer, DATE_BUFF_SIZE, "Date: %a, %d %b %Y %H:%M:%S GMT\r\n", tm);
	return strdup(buffer);
}


int parse_post(char *post) {
	char* peticion = strstr(post, "email=") + 6; // +6 por quedarnos al final de 'email='
	int index = 0;
	while (peticion[index] && MY_EMAIL[index] && peticion[index] == MY_EMAIL[index]) {
		index++;
	}
	int fd_form;
	if (index == strlen(MY_EMAIL) && !peticion[index]) {
		fd_form = open("accion_form_ok.html", O_RDONLY);
	} else {
		fd_form = open("accion_form_ko.html", O_RDONLY);
	}
	return fd_form;
}

void parse_get(char *stream, char **path, char **query) {
	char* start_get = strchr(stream, ' ') + 2; // Inicio del path (sin '/')
	char* query_get = strchr(stream, '?');     // Posible query ('?')
	char* end_get   = strchr(start_get, ' ');  // Fin del path

	/* TODO: mirar si existe en cada directorio un index.html, por ejemplo
		si entro en /otrapagina/ que se abra el index.html de ahí */

	int path_size  = (end_get - start_get) + 1; // 1 para '\0'
	int query_size = (end_get - query_get) + 1; // 1 para '\0'

	//
	//	Como se trata el caso excepcional de la URL que no apunta a ningún fichero
	//	html
	//

	/* Si el path es '/' devolvemos index.html */
	if (end_get - start_get == 0) {
		*path = strdup(DEFAULT_HTML_FILE);
		return;
	} else if (query_get && query_get - start_get == 0) {
		/* Si path es '\' y tenemos un query */
		*path = strdup(DEFAULT_HTML_FILE);
		*query = malloc(sizeof(char) * query_size);
		strncpy(*query, start_get, end_get - query_get);
		(*query)[query_size - 1] = '\0';
		return;
	}

	if (!query_get) { // No existe query
		/* Rellenamos el puntero path */
		*path = malloc(sizeof(char) * path_size);
		strncpy(*path, start_get, end_get - start_get);
		(*path)[path_size - 1] = '\0';
	} else { // Existe un query
		/* Rellenamos el puntero path */
		path_size = (query_get - start_get) + 1; // 1 para '\0'
		*path = malloc(sizeof(char) * path_size);
		strncpy(*path, start_get, query_get - start_get);
		(*path)[path_size - 1] = '\0';

		/* Rellenamos el puntero query */
		*query = malloc(sizeof(char) * query_size);
		strncpy(*query, query_get, end_get - query_get);
		(*query)[query_size - 1] = '\0';
	}

}

/* Comprbamos el formato de 'Cabecera: Datos \r\n' */
int peticion_mal_formada(char* buffer) {
	/* La primera línea la comprobamos dinámicamente en 'process_web_request'
		por lo tanto, nos la saltamos */
	char* fin_primera_linea = strstr(buffer, "\r\n");
	char* linea = strtok(fin_primera_linea, "\r\n");
	while (linea) {
		//printf("<<<%s\n", linea);
		/* Se comprueba 'Cabecera: Datos \r\n' teniendo en cuenta el posible POST */
		if (!strchr(linea, ':') && !strstr(linea, "email"))  {
			/* !strstr(linea, "email") para el caso de la última línea del post */
			return 1;
		}
		linea = strtok(NULL, "\r\n");
	}
	return 0;
}

/* Devuelve la extensión corresponiente según la tabla 'extensions' */
char* analyze_extension(char* extension) {
	//
	//	Evaluar el tipo de fichero que se está solicitando, y actuar en
	//	consecuencia devolviendolo si se soporta u devolviendo el error correspondiente en otro caso
	//
	extension = strrchr(extension, '.'); // Cogemos a partir del último punto
	if (!extension) return NULL;
	extension++; // Le quitamos el punto
	int i = 0;
	while (extensions[i].ext != 0) { // Iteramos hasta el fin de las extensiones dadas
		if (strcmp(extensions[i].ext, extension) == 0) {
			if (strcmp("php", extension) == 0) IS_PHP = 1;
			return strdup(extensions[i].filetype);
		}
		i++;
	}
	if (EXTENSIONS_ENABLED) return strdup("ALLOWED");
	return NULL;
}

char* remove_from_string(char* string, char* to_remove) {
	char buffer[REQUEST_BUFF_SIZE];
	int index = 0;
	for (int i = 0; i < strlen(string); i++) {
		if (!strchr(to_remove, string[i]))
			buffer[index++] = string[i];
	}
	return strdup(buffer);
}

int get_fd_size(int fd) {
	/* Usamos la estructura stat para conocer el tamaño del fichero */
	struct stat file_stat;
	fstat(fd, &file_stat);
	return file_stat.st_size;
}

int forbidden_paths(char *path) {
	//
	//	Como se trata el caso de acceso ilegal a directorios superiores de la
	//	jerarquia de directorios
	//	del sistema
	//
	return
	    (
	        path[0]	== '/' // Para el caso de una ruta absoluta como //home/alumno/fichero
	        ||
	        strstr(path, "../") != NULL
	        ||
	        strstr(path, "./")  != NULL
	    );
}

char* make_cookie() {
	char cookie[COOKIE_BUFF_SIZE];
	char* timeout = date_as_string(COOKIE_TIMEOUT);
	if (cookie_value < 0) { // cookie_value incial (-1)
		sprintf(cookie, "Set-Cookie: cookie_counter=1; Path=/; expires=%s", timeout);
	} else if (cookie_value < MAX_COOKIE_REQUEST) {
		sprintf(cookie, "Set-Cookie: cookie_counter=%d; Path=/; expires=%s", ++cookie_value, timeout);
	} else {
		free(timeout);
		return NULL;
	}
	free(timeout);
	return strdup(cookie);
}

int php(int fd, int fd_fichero) {
	if (fork() == 0) { 		// Hijo
		close(fd_fichero);  // El hijo cierra no necesita este fd
		dup2(fd, 1);		// Salida estandar -> fd del socket
		if (!PHP_QUERY) {
			/* Si no hay QUERY para el PHP */
			execl("/usr/bin/php", "php", PHP_PATH, NULL);
		} else {
			/* Hay un query que le pasamos al PHP */
			PHP_QUERY++; 			// nos saltamos '?'
			int num_argumentos = 1; // Argumentos totales (separados por '&')
			char* aux = PHP_QUERY;
			while (*aux) if (*aux++ == '&') num_argumentos++;
			int total_argumentos = 3 + num_argumentos; // 3 argumentos fijos
			char *argumentos[total_argumentos];
			argumentos[0] = "php"; 					   // Fijo 1
			argumentos[1] = PHP_PATH;				   // Fijo 2
			argumentos[total_argumentos - 1] = NULL;   // Fijo 3
			int argumento_variable = 2; 			   // Empieza por '2'
			char* linea = strtok(PHP_QUERY, "&");
			while (linea) {
				argumentos[argumento_variable++] = linea;
				linea = strtok(NULL, "&");
			}
			execv("/usr/bin/php", argumentos);
		}
		/* Esto no debería ejecutarse si PHP está instalado */
		perror("PHP no instalado");
		exit(EXIT_FAILURE);

	} else { // Padre
		/* Esperamos al hijo, resetamos variables globales
		 * e indicamos que hemos ejecutado PHP */
		wait(NULL);
		IS_PHP = 0;
		PHP_PATH = "";
		PHP_QUERY = "";
		return 1;
	}
	return 0; // No hay que ejecutar PHP
}


void enviar_respuesta(int fd, int tipo_respuesta, int fd_fichero, char* extension) {
	char respuesta[REQUEST_BUFF_SIZE];
	int indice;

	char* cookie = make_cookie();
	if (COOKIES_ENABLED && cookie == NULL) {
		tipo_respuesta = TOO_MANY_REQUESTS;
	}

	switch (tipo_respuesta) {
	case OK:
		indice = sprintf(respuesta, "%s", "HTTP/1.1 200 OK\r\n");
		if (PHP_ENABLED && IS_PHP) {
			indice += sprintf(respuesta + indice, "\r\n");
			write(fd, respuesta, indice); // Se escribe todo lo incluido en 'respuesta'
			php(fd, fd_fichero);
			free(cookie);
			close(fd);
			exit(EXIT_SUCCESS);
		}
		break;
	case BAD_REQUEST:
		indice = sprintf(respuesta, "%s", "HTTP/1.1 400 Bad Request\r\n");
		fd_fichero = open("400.html", O_RDONLY);
		extension = "text/html";
		break;
	case PROHIBIDO:
		indice = sprintf(respuesta, "%s", "HTTP/1.1 403 Forbidden\r\n");
		fd_fichero = open("403.html", O_RDONLY);
		extension = "text/html";
		break;
	case NOENCONTRADO:
		indice = sprintf(respuesta, "%s", "HTTP/1.1 404 Not Found\r\n");
		fd_fichero = open("404.html", O_RDONLY);
		extension = "text/html";
		break;
	case TOO_MANY_REQUESTS:
		indice = sprintf(respuesta, "%s", "HTTP/1.1 429 Too Many Requests\r\n");
		fd_fichero = open("429.html", O_RDONLY);
		extension = "text/html";
		break;
	}

	char* hora = date_as_string(0);
	indice += sprintf(respuesta + indice, "%s", hora);
	free(hora);
	indice += sprintf(respuesta + indice, "Content-Length: %d\r\n", get_fd_size(fd_fichero));
	indice += sprintf(respuesta + indice, "Content-Type: %s\r\n", extension);
	if (COOKIES_ENABLED && cookie != NULL) indice += sprintf(respuesta + indice, "%s", cookie);
	indice += sprintf(respuesta + indice, "\r\n");

	// TODO: mal formada

	free(cookie);

	//
	//	En caso de que el fichero sea soportado, exista, etc. se envia el fichero con la cabecera
	//	correspondiente, y el envio del fichero se hace en blockes de un máximo de  8kB
	//

	debug(LOG, "Enviamos un mensaje tipo", respuesta, fd);

	write(fd, respuesta, indice); // Se escribe todo lo incluido en 'respuesta'

	/* Leemos el fichero y lo escribimos en nuestro socket */
	int bytes_leidos;
	while ((bytes_leidos = read(fd_fichero, &respuesta, REQUEST_BUFF_SIZE)) > 0) {
		write(fd, respuesta, bytes_leidos);
	}
	if (tipo_respuesta != OK) close(fd_fichero);
}

// https://linux.die.net/man/3/fd_set
int fd_has_something_to_read(int fd) {
	/* 15 segundos de timeout si el fd ya no tiene más que leer */
	struct timeval tv;
	fd_set rfds;
	tv.tv_sec = PERSISTENT_TIME;
	tv.tv_usec = 0;
	FD_ZERO(&rfds);
	FD_SET(fd, &rfds);
	if (select(fd + 1, &rfds, NULL, NULL, &tv) < 0) {
		perror("select()");
		exit(EXIT_FAILURE);
	}
	/* Se comprueba si hay cosas por leer aún */
	return (FD_ISSET(fd, &rfds));
}

void process_web_request(int fd) {
	while (PERSISTENT_ENABLED && fd_has_something_to_read(fd)) {
		debug(LOG, "Request", "Ha llegado una peticion", fd);

		//
		// Definir buffer y variables necesarias para leer las peticiones
		//

		char buffer[REQUEST_BUFF_SIZE];		// El buffer para la lectura del socket
		char* peticion;						// La lectura del buffer sin '\r' ni '\n'
		char* path;							// La ruta del GET
		char* query;						// El posible query, e.g. ?nombre=Kiri&pass=123
		char* extension;					// La extensión del fichero del path

		//
		// Leer la petición HTTP
		//

		int bytes_leidos = read(fd, buffer, REQUEST_BUFF_SIZE);

		//
		// Comprobación de errores de lectura
		//

		if (bytes_leidos < 0) { // Si es -1
			perror("read");
			close(fd);
			exit(EXIT_FAILURE);
		}

		debug(LOG, "Mensaje", buffer, fd);
		//
		// Si la lectura tiene datos válidos terminar el buffer con un \0
		//

		buffer[bytes_leidos] = '\0';

		/* Gestión de cookies */
		if (COOKIES_ENABLED) {
			char* cookie_loc = "Cookie: cookie_counter="; // Para buscar la cookie en el request
			char* cookie_string = strstr(buffer, cookie_loc);
			cookie_value = (cookie_string) ? atoi(cookie_string + strlen(cookie_loc)) : -1;
		}

		//
		// Se eliminan los caracteres de retorno de carro y nueva linea
		//

		int bien_formado = 0;
		if ((buffer[0] == 'G' && buffer[1] == 'E' && buffer[2] == 'T' && buffer[3] == ' ' && buffer[4] == '/')
		        || (buffer[0] == 'P' && buffer[1] == 'O' && buffer[2] == 'S' && buffer[3] == 'T' && buffer[4] == ' ' && buffer[5] == '/'))
			bien_formado = 1;

		peticion = remove_from_string(buffer, "\r\n");
		char* primera_linea_end = strstr(peticion, "HTTP/1.1");
		if (primera_linea_end[-1] != ' ' || primera_linea_end[-2] == ' ') bien_formado = 0;
		if (!bien_formado || !primera_linea_end || peticion_mal_formada(buffer)) {
			enviar_respuesta(fd, BAD_REQUEST, NOFICHERO, NOEXTENSION);
			free(peticion); continue;
		}

		char primera_linea[primera_linea_end - peticion + 1];
		strncpy(primera_linea, peticion, primera_linea_end - peticion);
		primera_linea[primera_linea_end - peticion] = '\0';
		int fd_fichero;

		char* is_post = strstr(primera_linea, "POST");
		char* is_get  = strstr(primera_linea, "GET");

		if (!((is_post != NULL) ^ (is_get != NULL))) { // XOR
			/* Posibilidad de existencia de 'POST' o 'GET' en la primera línea */
			enviar_respuesta(fd, BAD_REQUEST, NOFICHERO, NOEXTENSION);
			free(peticion); continue;
		}

		if (is_post) {
			/* Tratamos el caso de un POST */
			fd_fichero = parse_post(peticion);
			if (fd_fichero < 0) {
				enviar_respuesta(fd, PROHIBIDO, NOFICHERO, NOEXTENSION);
			} else {
				enviar_respuesta(fd, OK, fd_fichero, "text/html");
			}
		} else if (is_get) {
			/* Tratamos el caso de un GET */
			parse_get(primera_linea, &path, &query);
			PHP_PATH = path;
			PHP_QUERY = query;
			extension = analyze_extension(path);
			fd_fichero = open(path, O_RDONLY);
			int tipo_respuesta;

			/* Ruta no válida o extensión no válida */
			if (forbidden_paths(path) || !extension) {
				tipo_respuesta = PROHIBIDO;
			} else if (fd_fichero < 0) {
				tipo_respuesta = NOENCONTRADO; // No se ha encontrado/abierto el fd
			} else {
				tipo_respuesta = OK; // Si ninguno de los cacos anteriores se da, entonces OK
			}
			enviar_respuesta(fd, tipo_respuesta, fd_fichero, extension);
		}
		//free(path); free(query); free(extension)
		free(peticion); close(fd_fichero);
	}
	close(fd); exit(EXIT_FAILURE);
}

int main(int argc, char **argv)
{

	int port, pid, listenfd, socketfd;
	socklen_t length;
	static struct sockaddr_in cli_addr;		// static = Inicializado con ceros
	static struct sockaddr_in serv_addr;	// static = Inicializado con ceros

	//  Argumentos que se esperan:
	//
	//	argv[1]
	//	En el primer argumento del programa se espera el puerto en el que el servidor escuchara
	//
	//  argv[2]
	//  En el segundo argumento del programa se espera el directorio en el que se encuentran los ficheros del servidor
	//
	//  Verficiar que los argumentos que se pasan al iniciar el programa son los esperados
	//

	//
	//  Verficiar que el directorio escogido es apto. Que no es un directorio del sistema y que se tienen
	//  permisos para ser usado
	//

	if (chdir(argv[2]) == -1) {
		(void)printf("ERROR: No se puede cambiar de directorio %s\n", argv[2]);
		exit(4);
	}
	// Hacemos que el proceso sea un demonio sin hijos zombies
	if (fork() != 0)
		return 0; // El proceso padre devuelve un OK al shell

	(void)signal(SIGCHLD, SIG_IGN); // Ignoramos a los hijos
	(void)signal(SIGHUP, SIG_IGN); // Ignoramos cuelgues

	debug(LOG, "web server starting...", argv[1] , getpid());

	/* setup the network socket */
	if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		debug(ERROR, "system call", "socket", 0);

	port = atoi(argv[1]);

	if (port < 0 || port > 60000)
		debug(ERROR, "Puerto invalido, prueba un puerto de 1 a 60000", argv[1], 0);

	/* Se crea una estructura para la información IP y puerto donde escucha el servidor */
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY); /* Escucha en cualquier IP disponible */
	serv_addr.sin_port = htons(port); /* ... en el puerto port especificado como parámetro */

	if (bind(listenfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
		debug(ERROR, "system call", "bind", 0);

	if (listen(listenfd, 64) < 0)
		debug(ERROR, "system call", "listen", 0);

	while (1) {
		length = sizeof(cli_addr);
		if ((socketfd = accept(listenfd, (struct sockaddr *)&cli_addr, &length)) < 0)
			debug(ERROR, "system call", "accept", 0);
		if ((pid = fork()) < 0) {
			debug(ERROR, "system call", "fork", 0);
		} else {
			if (pid == 0) { 	// Proceso hijo
				(void)close(listenfd);
				process_web_request(socketfd); // El hijo termina tras llamar a esta función
			} else { 			// Proceso padre
				(void)close(socketfd);
			}
		}
	}
}
