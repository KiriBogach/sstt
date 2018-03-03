<html>
    <head>
        <style>
        table, th, td {
        border: 1px solid black;
        border-collapse: collapse;
        }
        th, td {
        padding: 5px;
        text-align: left;
        }
        </style>
        <title>Servidor Simple para Servicios Telem&acute;ticos</title>
        <link rel="icon" type="image/ico" href="recursos/favicon.ico" sizes="192x192" />
    </head>
    <body bgcolor="lightgray">
        <h1>Esta es una pagina de prueba</h1>
        <img src="recursos/logo-um.jpg">
        <?php
        echo '<br><br>';
        function build_table($array) {
        $html = '<table>';
            foreach($array as $key=>$value) {
            $html .= '<tr>';
                $html .= '<th>' . $key . '</th>';
                $html .= '<th>' . $value . '</th>';
            $html .= '</tr>';
            }
        $html .= '</table>';
        return $html;
        }
        parse_str(implode('&', array_slice($argv, 1)), $_GET);
        echo build_table($_GET);
        ?>
        <p><h4><?php echo 'Fecha actual sacada con PHP: ' . date(DATE_RFC2822); ?></h4></p>
        <form method="POST" action="/accion_form.html">
            Correo electr&oacute;nico:<br>
            <input type="text" name="email" value=""><br>
            <input type="submit" value="Enviar">
        </form>
    </body>
</html>