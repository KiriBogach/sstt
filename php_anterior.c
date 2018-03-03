void enviar_respuesta(int fd, int tipo_respuesta, int fd_fichero, char* extension) {
	char respuesta[REQUEST_BUFF_SIZE];
	int indice;

	char* cookie = make_cookie();
	if (COOKIES_ENABLED && cookie == NULL) {
		tipo_respuesta = PROHIBIDO;
	}

	switch (tipo_respuesta) {
	case OK:
		indice = sprintf(respuesta, "%s", "HTTP/1.1 200 OK\r\n");
		char* hora = date_as_string(0);
		indice += sprintf(respuesta + indice, "%s", hora);
		free(hora);
		if (PHP_ENABLED && IS_PHP) {
			if (fork() == 0) { // Hijo
				close(fd_fichero);
				int fd_prueba = open("auxiliar.php", O_RDWR | O_CREAT | O_TRUNC, 0600);
				dup2(fd_prueba, 1);
				execl("/usr/bin/php", "php", PHP_PATH, (char *)0);
			} else {
				wait(NULL);
				fd_fichero = open("auxiliar.php", O_RDONLY);
			}
		}
		indice += sprintf(respuesta + indice, "Content-Length: %d\r\n", get_fd_size(fd_fichero));
		indice += sprintf(respuesta + indice, "Content-Type: %s\r\n", extension);
		if (COOKIES_ENABLED) indice += sprintf(respuesta + indice, "%s", cookie);
		indice += sprintf(respuesta + indice, "\r\n");
		break;
	case PROHIBIDO:
		indice = sprintf(respuesta, "%s", "HTTP/1.1 403 Forbidden\r\n");
		break;
	case NOENCONTRADO:
		indice = sprintf(respuesta, "%s", "HTTP/1.1 404 Not Found\r\n");
		break;
	}

	free(cookie);

	write(fd, respuesta, indice); // Se escribe todo lo incluido en 'respuesta'
	if (tipo_respuesta == OK) {
		/* Leemos el fichero y lo escribimos en nuestro socket */
		char buffer_fichero[REQUEST_BUFF_SIZE];
		int bytes_leidos;
		while ((bytes_leidos = read(fd_fichero, &buffer_fichero, REQUEST_BUFF_SIZE)) > 0) {
			write(fd, buffer_fichero, bytes_leidos);
		}
		if (PHP_ENABLED && IS_PHP) {
			remove("auxiliar.php");
			IS_PHP = 0;
			PHP_PATH = "";
		}
	}
}