//Autor: Fábio Henrique Cabrini
//Resumo: Esse programa possibilita ligar e desligar o led onboard, além de mandar o status para o Broker MQTT possibilitando o Helix saber
//se o led está ligado ou desligado.
//Revisões:
//Rev1: 26-08-2023 Código portado para o ESP32 e para realizar a leitura de luminosidade e publicar o valor em um tópico aprorpiado do broker 
//Autor Rev1: Lucas Demetrius Augusto 
//Rev2: 28-08-2023 Ajustes para o funcionamento no FIWARE Descomplicado
//Autor Rev2: Fábio Henrique Cabrini

#include <WiFi.h>
#include <PubSubClient.h> // Importa a Biblioteca PubSubClient
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <ESP32Servo.h>

Servo myServo;
 
//defines:

//defines de id mqtt e tópicos para publicação e subscribe denominado TEF(Telemetria e Monitoramento de Equipamentos)

#define TOPICO_SUBSCRIBE    "/TEF/akona003/cmd"        //tópico MQTT de escuta
#define TOPICO_PUBLISH      "/TEF/akona003/attrs"      //tópico MQTT de envio de informações para Broker
#define TOPICO_PUBLISH_2    "/TEF/akona003/attrs/l"    //tópico MQTT de envio de informações para Broker
#define TOPICO_PUBLISH_3    "/TEF/akona003/attrs/t"
#define TOPICO_PUBLISH_4    "/TEF/akona003/attrs/h"

#define ID_MQTT  "fiware_akona3"      //id mqtt (para identificação de sessão)

const int pirPin = 35;  // Pino de entrada do sensor de presença PIR
const int ldrPin = 2;   // Pino de entrada do sensor de luz (LDR)
int servoPin = 18;
int ledPin = 4;  
const float gama = 0.7;
const float rl10 = 50;

#define DHTPIN 15           // Pino de dados do sensor DHT11
#define DHTTYPE DHT11       // Tipo do sensor DHT (DHT11)

DHT dht(DHTPIN, DHTTYPE);

// WIFI

const char* SSID = "FIAP-IBM"; // SSID / nome da rede WI-FI que deseja se conectar
const char* PASSWORD = "Challenge@23!"; // Senha da rede WI-FI que deseja se conectar

// MQTT

const char* BROKER_MQTT = "46.17.108.113"; //URL do broker MQTT que se deseja utilizar
int BROKER_PORT = 1883; // Porta do Broker MQTT
int D4 = 2;

//Variáveis e objetos globais

WiFiClient espClient; // Cria o objeto espClient
PubSubClient MQTT(espClient); // Instancia o Cliente MQTT passando o objeto espClient
char EstadoSaida = '0';  //variável que armazena o estado atual da saída

//Prototypes

void initSerial();
void initWiFi();
void initMQTT();
void reconectWiFi(); 
void mqtt_callback(char* topic, byte* payload, unsigned int length);
void VerificaConexoesWiFIEMQTT(void);
void InitOutput(void);

/* 

*  Implementações das funções

*/

void setup() {

    Serial.begin(115200); // Inicializa a comunicação serial

    //inicializações:

    InitOutput();
    initSerial();
    initWiFi();
    initMQTT();
    pinMode(pirPin, INPUT);
    pinMode(ldrPin, INPUT);
    pinMode(ledPin, OUTPUT);  

    myServo.attach(servoPin);
    myServo.write(90);
    dht.begin();
    delay(5000);
    MQTT.publish(TOPICO_PUBLISH, "s|on");

}

//Função: inicializa comunicação serial com baudrate 115200 (para fins de monitorar no terminal serial 
//        o que está acontecendo.
//Parâmetros: nenhum
//Retorno: nenhum

void initSerial() {

  Serial.begin(115200);

}

//Função: inicializa e conecta-se na rede WI-FI desejada
//Parâmetros: nenhum
//Retorno: nenhum

void initWiFi() {
    delay(10);
    Serial.println("------Conexao WI-FI------");
    Serial.print("Conectando-se na rede: ");
    Serial.println(SSID);
    Serial.println("Aguarde");
    reconectWiFi();
}

//Função: inicializa parâmetros de conexão MQTT(endereço do 
//        broker, porta e seta função de callback)
//Parâmetros: nenhum
//Retorno: nenhum

void initMQTT() {
    MQTT.setServer(BROKER_MQTT, BROKER_PORT);   //informa qual broker e porta deve ser conectado
    MQTT.setCallback(mqtt_callback);            //atribui função de callback (função chamada quando qualquer informação de um dos tópicos subescritos chega)
}

//Função: função de callback 
//        esta função é chamada toda vez que uma informação de 
//        um dos tópicos subescritos chega)
//Parâmetros: nenhum
//Retorno: nenhum

void mqtt_callback(char* topic, byte* payload, unsigned int length) 

{

    String msg;
    //obtem a string do payload recebido
    for(int i = 0; i < length; i++) {

       char c = (char)payload[i];
       msg += c;

    }

    Serial.print("- Mensagem recebida: ");
    Serial.println(msg);

    //toma ação dependendo da string recebida:
    //verifica se deve colocar nivel alto de tensão na saída D0:
    //IMPORTANTE: o Led já contido na placa é acionado com lógica invertida (ou seja,
    //enviar HIGH para o output faz o Led apagar / enviar LOW faz o Led acender)

    if (msg.equals("akona003@on|")) {

        digitalWrite(D4, HIGH);
        EstadoSaida = '1';

    }

    //verifica se deve colocar nivel alto de tensão na saída D0:

    if(msg.equals("akona003@off|")){

        digitalWrite(D4, LOW);
        EstadoSaida = '0';

    }

}

//Função: reconecta-se ao broker MQTT (caso ainda não esteja conectado ou em caso de a conexão cair)
//        em caso de sucesso na conexão ou reconexão, o subscribe dos tópicos é refeito.
//Parâmetros: nenhum
//Retorno: nenhum

void reconnectMQTT(){

    while (!MQTT.connected()){

        Serial.print("* Tentando se conectar ao Broker MQTT: ");
        Serial.println(BROKER_MQTT);

        if (MQTT.connect(ID_MQTT)){

            Serial.println("Conectado com sucesso ao broker MQTT!");
            MQTT.subscribe(TOPICO_SUBSCRIBE); 

        } 

        else{

            Serial.println("Falha ao reconectar no broker.");
            Serial.println("Havera nova tentatica de conexao em 2s");
            delay(2000);

        }

    }

}

//Função: reconecta-se ao WiFi
//Parâmetros: nenhum
//Retorno: nenhum

void reconectWiFi() {

    //se já está conectado a rede WI-FI, nada é feito. 
    //Caso contrário, são efetuadas tentativas de conexão

    if (WiFi.status() == WL_CONNECTED)

        return;

    WiFi.begin(SSID, PASSWORD); // Conecta na rede WI-FI

    while (WiFi.status() != WL_CONNECTED){

        delay(100);
        Serial.print(".");

    }

    Serial.println();
    Serial.print("Conectado com sucesso na rede ");
    Serial.print(SSID);
    Serial.println("IP obtido: ");
    Serial.println(WiFi.localIP());

}

//Função: verifica o estado das conexões WiFI e ao broker MQTT. 
//        Em caso de desconexão (qualquer uma das duas), a conexão
//        é refeita.
//Parâmetros: nenhum
//Retorno: nenhum

void VerificaConexoesWiFIEMQTT(void){

    if (!MQTT.connected()) 

        reconnectMQTT(); //se não há conexão com o Broker, a conexão é refeita

     reconectWiFi(); //se não há conexão com o WiFI, a conexão é refeita

}

//Função: envia ao Broker o estado atual do output 
//Parâmetros: nenhum
//Retorno: nenhum

void EnviaEstadoOutputMQTT(void){

    if (EstadoSaida == '1'){

      MQTT.publish(TOPICO_PUBLISH, "s|on");
      Serial.println("- Led Ligado");

    }

    if (EstadoSaida == '0'){

      MQTT.publish(TOPICO_PUBLISH, "s|off");
      Serial.println("- Led Desligado");

    }

    Serial.println("- Estado do LED onboard enviado ao broker!");
    delay(1000);

}

//Função: inicializa o output em nível lógico baixo
//Parâmetros: nenhum
//Retorno: nenhum

void InitOutput(void){

    //IMPORTANTE: o Led já contido na placa é acionado com lógica invertida (ou seja,
    //enviar HIGH para o output faz o Led apagar / enviar LOW faz o Led acender)

    pinMode(D4, OUTPUT);
    digitalWrite(D4, HIGH);
    boolean toggle = false;

    for(int i = 0; i <= 10; i++){

        toggle = !toggle;
        digitalWrite(D4,toggle);
        delay(200);           

    }

}
 
//programa principal

void loop() {   

  char msgBuffer[6];
  char luminosityBuffer[6];
  char temperatureBuffer[6];
  char humidityBuffer[6];

  //garante funcionamento das conexões WiFi e ao broker MQTT

  VerificaConexoesWiFIEMQTT();

  //envia o status de todos os outputs para o Broker no protocolo esperado

  EnviaEstadoOutputMQTT();


  //  // Leitura do sensor de presença
  //  int pirValue = digitalRead(pirPin);
  //  dtostrf(pirValue, 4, 2, msgBuffer);
  //  MQTT.publish(TOPICO_PUBLISH_2,msgBuffer);
  // Leitura do sensor de luz (LDR) e mapeamento para a faixa de 0 a 100

  int ldrValue = analogRead(ldrPin);
  int ldrMappedValue = map(ldrValue, 4095, 0, 1024, 0);
  float voltage = ldrMappedValue / 1024.*5;
  float resistance = 2000 * voltage / (1-voltage/5);
  float brightness = pow(rl10*1e3*pow(10,gama)/resistance,(1/gama));

  Serial.print("Sensor de Luz (LDR): ");
  Serial.println(brightness);

  dtostrf(brightness, 4, 2, luminosityBuffer);

  MQTT.publish(TOPICO_PUBLISH_2, luminosityBuffer);
 
  // Controle do LED com base na luminosidade e presença

  if (brightness > 50 || digitalRead(pirPin) == HIGH) {

    digitalWrite(ledPin, HIGH); // Acende o LED

  }else{

    digitalWrite(ledPin, LOW); // Apaga o LED

  }
 
 
  // Leitura da temperatura e umidade
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();

  Serial.print("Temperatura: ");
  Serial.println(temperature);
  Serial.print("Umidade: ");
  Serial.println(humidity);

  dtostrf(temperature, 4, 2, temperatureBuffer);
  MQTT.publish(TOPICO_PUBLISH_3, temperatureBuffer);
  dtostrf(humidity, 4, 2, humidityBuffer);
  MQTT.publish(TOPICO_PUBLISH_4, humidityBuffer);
 
  // Lógica de controle com base na temperatura e umidade
    if (temperature > 20 && humidity > 30){
        Serial.println("Ligando o sistema de climatização...");
        // Girar o servo motor de 0 a 180 graus

        for (int pos = 0; pos <= 180; pos += 1){
            myServo.write(pos);
            delay(3);  // Ajuste a velocidade conforme necessário
        }

        // Girar o servo motor de 180 a 0 graus
        for (int pos = 180; pos >= 0; pos -= 1) {
            myServo.write(pos);
            delay(3);  // Ajuste a velocidade conforme necessário
        }

    } else {

        // Desligar o sistema de climatização ou realizar outra ação
        Serial.println("Desligando o sistema de climatização...");

    }
 
  delay(1000);  // Leituras a cada segundo

  //keep-alive da comunicação com broker MQTT

  MQTT.loop();

}