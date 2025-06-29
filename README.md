# Documentação do Projeto: Controle de LED via MQTT

O objetivo do programa é que um cliente MQTT se conecte a um broker e se inscreva no tópico */ifpe/ads/embarcados/esp32/led*
Ao receber "1" o led da placa é ligado e ao receber "0" o led da placa é desligado.
Essa comunicação é feita usando o protocolo MQTT e o ESP-IDF.

O Broker utilizado foi o mosquitto

# No código há algumas funções Auxiliares, vale a pena comentar seu uso:

`print_user_property()`
Exibe no terminal as user properties recebidas em eventos MQTT.

`log_error_if_nonzero()`
Loga mensagens de erro em conexões TLS, útil para depuração.

Função Principal de Evento MQTT:

`mqtt5_event_handler()`
Trata os eventos:

## Evento	Ação
MQTT_EVENT_CONNECTED	Inscreve no tópico do LED
MQTT_EVENT_DATA	Verifica a mensagem recebida e liga/desliga o LED
MQTT_EVENT_ERROR	Mostra informações de erro se houver falha na conexão


# PARA CONFIGURAR O PROJETO:

rode *idf.py menuconfig* no terminal do vscode, ou então, procure a opção de SDK Configuration Editor.
No menu, vá até a opção "Example Configuration" e altere o broker url, para o IP do seu broker mqtt, seja ele local ou remoto.
Há brokers para testes públicos, como o mosquitto e o hivemq.
Ainda no menu, configure a conexão Wi-Fi, colocando o nome e a senha da rede.

Após isso, compile o projeto: idf.py build, ou pelo ESP-IDF procure a opção de build.
Em seguida, rode o comando *idf.py -p (porta onde o esp está conectado) flash monitor*. Ele irá gravar o código na placa e irá abrir o monitor device da placa, verificando a execução, permitindo visualizar mensagens de depuração e dados transmitidos.

Após os passos acima, abra um terminal PowerShell e rode o seguinte comando:
*mosquitto_pub -h test.mosquitto.org -t /ifpe/ads/embarcados/esp32/led -m 1*
Esse comando irá acender o led da placa.

Substituindo o valor por 0, deverá apagar o led.
*mosquitto_pub -h test.mosquitto.org -t /ifpe/ads/embarcados/esp32/led -m 0*
