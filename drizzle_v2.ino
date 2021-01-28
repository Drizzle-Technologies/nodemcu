/*
  O grupo 33 da disciplina de Introdução à Engenharia Elétrica (ou Drizzle Technologies se preferir),
  adaptou os cógidos disponíveis como exemplo da biblioteca da ESP8266, e também,o código presente no link abaixo, para o próprio projeto.
  https://buger.dread.cz/simple-esp8266-https-client-without-verification-of-certificate-fingerprint.html
*/

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>
#include <Arduino_JSON.h> //decodificar a JSON file

ESP8266WiFiMulti WiFiMulti;

char* ssid = "Megalink_Kissia";
char* password = "24031987";
//"PERSONAL", "@Opers6@#", "NORTLINK_KISSIA", "25031964" , "MARIA_DELIA", "theo1623"

//----VARIÁVEIS DO GET-----//
String capacidadeMaxima, payload;
//ainda deixei como vetor mesmo tendo uma váriavel só por desencargo de consciência, mas talvez poderíamos testar pra ser só um float mesmo
int capacidadeMaximaVetor[1] = {-1}; //incializando com esse valor pra não dar problema com occupancy
//variável para verificar que passou pelo loop apenas na primeira vez
int inicio = 0;

//-----VARIÁVEIS DO POST-----//
int id = 2;

//fazer o POST da quantidade atual da variável
String postSensor;

// Set timer to 1 hour (3600000)
unsigned long timerDelayPost = 3600000;

int feito = 0;

//-----VARIÁVEIS DO SENSOR-----//
int lumVal1, lumVal2;
unsigned int occupancy = 0; //guarda o número de pessoas em cada estabelecimento
unsigned int change_b = 0, change_a = 0; //verifica se houve mudança na variável de ocupação do estabelecimento
// as variaveis são unsigned long porque como o tempo tá em ms, o número fica grande demais para ser armazenado nem int
unsigned long lastTime1 = millis();
unsigned long lastTime2 = millis(); //variável auxiliar para controle de tempo passado
// Set timer to 300 mili seconds (300)
unsigned long timerDelaySensor = 300;

void setup() {

  Serial.begin(115200);
  // Serial.setDebugOutput(true);

  Serial.println();
  Serial.println();
  Serial.println();

  for (uint8_t t = 4; t > 0; t--) {
    Serial.printf("[SETUP] WAIT %d...\n", t);
    Serial.flush();
    delay(1000);
  }

  WiFi.mode(WIFI_STA);
  WiFiMulti.addAP(ssid, password);

  //incializando os pinos 
  pinMode(A0, INPUT); //ldr como entrada
  pinMode(D0, OUTPUT); //sensor saida
  pinMode(D1, OUTPUT); //sensor entrada
  pinMode(D6, OUTPUT); //led entrada 
  pinMode(D8, OUTPUT); //led saida
}

void loop() {
  // wait for WiFi connection
  if ((WiFiMulti.run() == WL_CONNECTED)) {

    std::unique_ptr<BearSSL::WiFiClientSecure>client(new BearSSL::WiFiClientSecure);

    client->setInsecure();

    HTTPClient https;

    if(!inicio){
      //----- INÍCIO DA FUNÇÃO GET -----//
      Serial.print("[HTTPS] begin...\n");
      if (https.begin(*client, "https://inteng33.herokuapp.com/get_max_people/2")) {  // HTTPS with path or Domain name with URL path 
  
        Serial.print("[HTTPS] GET...\n");
        // start connection and send HTTP header
        int httpCode = https.GET();
  
        // httpCode will be negative on error
        if (httpCode > 0) {
          // HTTP header has been send and Server response header has been handled
          Serial.printf("[HTTPS] GET... code: %d\n", httpCode);
  
          // file found at server
          if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
            payload = https.getString();
            Serial.println(payload);
          }
        } else {
          Serial.printf("[HTTPS] GET... failed, error: %s\n", https.errorToString(httpCode).c_str());
        }
  
        https.end();
        //----- TRATAMENTO DA STRING RECEBIDA E JSON FILE -----//
        //fazer um teste pra saber como que tá vindo a JSON file
        capacidadeMaxima = payload; //fazendo o get com a função definida abaixo
        Serial.println(capacidadeMaxima);
        JSONVar myObject = JSON.parse(capacidadeMaxima); //nesse caso tá puxando uma string, podemos testar mudar pra apenas uma variavel
  
        // JSON.typeof(jsonVar) usado para saber o tipo da variável
        if (JSON.typeof(myObject) == "undefined") {
          Serial.println("Parsing input failed!");
          return;
        }
    
        Serial.print("JSON object = ");
        Serial.println(myObject);
  
        //a partir daqui ele tá decodificando a JSON file 
        // myObject.keys() usado para pegar um vetor com todas as chaves dos objetos
        JSONVar keys = myObject.keys();
        JSONVar value = myObject[keys[0]];
        Serial.print(keys[0]);
        Serial.print(" = ");
        Serial.println(value);
        capacidadeMaximaVetor[0] = value; //só tá guardando o valor da variavel de capacidade máxima
        inicio = 1;
        Serial.println("Fim do GET");
        //----- FIM DA FUNÇÃO GET -----// 
      }
      else {
        Serial.printf("[HTTPS] Unable to connect\n");
      }
    }
    change_b = occupancy;
    sensor();
    change_a = occupancy
    //-----INICIO DO POST-----//
    if((change(change_b,change_a) && !feito) || (((millis() - lastTime2) < timerDelayPost) && !feito)){  //rodando a cada mudança de variável ou passada 1h
      Serial.print("[HTTPS] begin...\n");
      // configure traged server and url
      https.begin(*client, "https://inteng33.herokuapp.com/add_occupancy"); //HTTP
      https.addHeader("Content-Type", "application/json"); //text/plain
  
      Serial.print("[HTTPS] POST...\n");
      // start connection and send HTTP header and body
      //"{\"id\":\"2\",\"occupancy\":\"30\"}"
      postSensor = "{\"id\":\"";
      postSensor += id;
      postSensor += "\",\"occupancy\":\"";
      postSensor += occupancy;
      postSensor += "\"}";
      int httpCode = https.POST(postSensor);
      
      // httpCode will be negative on error
      if (httpCode > 0) {
        // HTTP header has been send and Server response header has been handled
        Serial.printf("[HTTPS] POST... code: %d\n", httpCode);
     
        // file found at server
        if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
          String payload = https.getString();
          Serial.println("received payload:\n<<");
          Serial.println(payload);
          Serial.println(">>");
        }
       } else {
        Serial.printf("[HTTPS] POST... failed, error: %s\n", https.errorToString(httpCode).c_str());
      }
      https.end();
      feito = 1;
    } //FIM DO POST
    if((millis() - lastTime2) > timerDelayPost){
      lastTime2 = millis();
      feito = 0;
    }
  } //fim do loop da conexão de wifi 
}


void sensor(){
  if((millis() - lastTime1) < timerDelaySensor){ //executa o loop a cada 300ms
    //lendo os valores dos sensores, acho que talvez deveriamos diminuir o tempo do loop 
    
    //confere entrada
    digitalWrite(D0, 0);
    digitalWrite(D1, 1);
    lumVal1 = (analogRead(A0));
    delay(30);
    //confere saida
    digitalWrite(D0, 1);
    digitalWrite(D1, 0);
    lumVal2 = (analogRead(A0));
    delay(30);
    digitalWrite(D0, 0);
    digitalWrite(D1, 0);
    
    //----- IDENTIFICAÇÃO DA PASSAGEM DE PESSOAS -----//
    //conferir isso aqui, acho que talvez não precise
     if (occupancy == capacidadeMaximaVetor[0]) {
      Serial.println("ESTABELECIMENTO CHEIO");
    }
        if(lumVal1 < 200 && lumVal2 > 500){
      Serial.println(lumVal1);
      Serial.println("Entrou");
      occupancy++;
      Serial.println("Numero de Pessoas: ");
      Serial.print(occupancy);
      digitalWrite(D6, 1);
      delay(1000);
    } 
    else if(lumVal2 < 200 && lumVal1 > 500){
      Serial.println(lumVal1);
      Serial.println("Saiu");
      occupancy--;
      Serial.println("Numero de Pessoas: ");
      Serial.print(occupancy);
      digitalWrite(D8, 1);
      delay(1000);
    } else{
      digitalWrite(D8, 0);
      digitalWrite(D6, 0);
    }
  } //fim do loop da contagem de pessoas
  if((millis() - lastTime1) > timerDelaySensor*2){
    lastTime1 = millis();
  }
}

int change(unsigned int before, unsigned int after){
    if((before - after >= 5) || (before - after <= -5)){
        return 1;
    }
    else return 0;
}
