echo "Reseteando el log..."
rm webserver.log
echo "Matando procesos web_sstt..."
pkill -9 web_sstt
echo "Borrando el ejecutable web_sstt..."
rm web_sstt