[ $# -ne 1 ] && echo "Uso: $0 [puerto]" && exit 1

echo "Reseteando el log..."
rm webserver.log
echo "Matando procesos web_sstt..."
pkill -9 web_sstt
echo "Compilando..."
gcc -Wall web_sstt.c -o web_sstt
echo "Lanzando servidor..."
./web_sstt $1 "$PWD"

if [ $(ps aux | grep "$web_sstt $1" | wc -l) -eq 2 ]; 
then 
	echo "El servidor ha sido lanzado correctamente con el puerto [$1]."
else
	echo "Error, intente cambiar el puerto [$1]."
	pkill -9 web_sstt
fi
#curl localhost:8888 > /dev/null 2>&1