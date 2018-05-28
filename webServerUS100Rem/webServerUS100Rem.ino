#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>
#include <math.h>
#include "ESP8266HTTPClient.h"
#include <SoftwareSerial.h>
#include <EEPROM.h>

const float pi = 3.1416;
//Definir los pines del SensorUltrasonico
#define RX  4
#define TX  5

float DIAMETER = 146; /* 56.5*/;
float LENGTH = 180.9; /* 86.5*/
int MAXVOLUME = 800;

struct datapoint {
   float distance;
   float volume;
};

/*-------------------------- Struct de la EEPROM. Se usa para guardar las dimensiones del tanque, y que sobrevivan apagones. -----------------------------------------------*/
struct EEprom_package
{
    float l;
    float d;
    bool s;
} EEprom_Data;


// Variables Globales
  float sonnarValue = 0;
  float sonnarHeight = 0;
  float temperatureValue = 0;

char*  meterId = "9"; // INGRESAR METER ID

struct datapoint data;

ESP8266WebServer server;
StaticJsonBuffer<200> jsonBuffer;
JsonObject& root = jsonBuffer.createObject();
JsonObject& levelJson = jsonBuffer.createObject();

// Identificación de la red local

char* ssid = "ECOLGB";
char* password = "ecolgb123";
IPAddress ip(192,168,2,102);
IPAddress gateway(192,168,1,1);
IPAddress subnet(255,255,255,0);
//-------------------------------------

// Comunicacion serial con el sensor.
SoftwareSerial sensorSerial(RX,TX);

void setup()
{
  // Inicio de sesión
  WiFi.begin(ssid,password);   
  WiFi.config(ip, gateway, subnet);

  //Comunicacion serial con PC y con Sensor
  Serial.begin(9600);
  sensorSerial.begin(9600);

  
  while(WiFi.status()!= WL_CONNECTED)
  {
    Serial.print(".");
    delay(500);
  }

  EEPROMLoad();
  
  server.on("/",[](){server.send(200,"text/plain","INDEX");});    // raìz
  server.on("/gas-level",gaslevel);                               // gas-level ruta
  server.on("/configData", HTTP_POST, [](){                       // configData Seteo de Valores desde POST
    Serial.println("Cambiando Dimensiones del Tanque");
    StaticJsonBuffer<200> configBuffer;
    JsonObject& configJson= configBuffer.parseObject(server.arg("plain"));
    String l = configJson["l"];
    String d = configJson["d"];
    String v = configJson["vol"];
    String meterId2 = configJson["meterId"];
    String volumenTotal = configJson["vol"];
    Serial.println("-------------UPDATING TANK -------------");
    DIAMETER = d.toFloat();                                //Las medidas del smart site se envían en pulgadas, aquí se convierten a cm.
    LENGTH = l.toFloat();
    MAXVOLUME = v.toInt();
    server.send ( 200, "text/json", "{success:true}" );
    EEPROMSave();
    EEPROMLoad();
  });  
  server.begin();   // Iniciar el servidor
  delay(10000);
}

void loop() 
{
  server.handleClient();  //Escuchar
  data = leerSonnar();
  sendGasLevel(data.volume, data.distance);
  delay(2000);
}

void gaslevel()       // GET para gas-level 
{
  
  root["sonnar"] = sonnarValue;         //Sonnar value volume in gals
  root["temperature"] = temperatureValue;      //Temperature value
  root["meterId"] = meterId;      //Temperature value\
  
  String gasLevelJson;     
  root.printTo(gasLevelJson); // Formato
  
  server.send(200,"text/json",gasLevelJson); // send
   
}

void sendGasLevel(float sonnarValue, float sonnarHeight){
  if(WiFi.status()== WL_CONNECTED){
      Serial.println("Sending gas level..");
     HTTPClient http;    //Declare object of class HTTPClient
     //Datos de Cliente set-Motor
     http.begin("http://192.168.2.221:4000/send-gas-level");      //Specify request destination
     http.addHeader("Content-Type", "application/json");  //Specify content-type header
     
     //Creando JSON GENERATORDATA
     levelJson["level"] = sonnarValue;
     levelJson["distance"] = sonnarHeight;
     String levelData;
     levelJson.printTo(levelData); 
     int httpCode = http.POST(levelData);   //Send the request
     String payload = http.getString();         //Get the response payload
  
     /*
     Datos en Pantalla
     Serial.print("RESPUESTA DEL SERVIDOR: ");
     Serial.println(payload);    /Print request response payload */
     http.end();  //Close connection;    
  }
}
  
struct datapoint leerSonnar(){
  Serial.println("____________________________________________________________");
  struct datapoint res;
  float valorH;
  float height = DIAMETER;
  float len = LENGTH;
  float radius = height/2.0;
  float volume;

  valorH = measureDistance();
  volume = calcVol(valorH , radius, len);          // Se calcula el volumen de combustible, ya convertido a galones.
  //Serial.println("Volumen calculado, en galones:");
  //Serial.println(volume);
  /*volume = volume * 14.39;   */                              //simulacion de laboratorio
  res.distance = valorH;
  Serial.println("Distancia al Combustible (cm): ");
  Serial.println(valorH);
  Serial.println("Valor Calculado (gal): ");
  Serial.println(volume);
  res.volume = volume;
  return res;  
}

float measureDistance() {
  unsigned int mSByte;
  unsigned int lSByte;
  float distance;
  int temp;

  sensorSerial.flush();
  sensorSerial.write(0x55);                     //Solicitud de medicion de distancia
  delay(500);

  if (sensorSerial.available() >= 2) {
    mSByte = sensorSerial.read();
    lSByte = sensorSerial.read();
    distance = mSByte * 256 + lSByte;
  } else {
    Serial.println("Sensor no respondio solicitud de distancia");
    distance = 0;
  }

  sensorSerial.flush();
  sensorSerial.write(0x50);                    //Solicitud de medicion de temperatura

  delay(500);

  if (sensorSerial.available() >= 1) {
                                                      Serial.println("Recibido temp");
    temp = sensorSerial.read();
    temp -= 45;
  } else {
    Serial.println("Sensor no respondio solicitud de temperatura");
    temp = 0;
  }

  Serial.print(temp, DEC);
  Serial.println(" °C");
  
  return (distance/10.0);
}
 
float calcVol(float h, float r, float l) {
  Serial.print("Diametro actual: ");
  Serial.println(DIAMETER);
  Serial.print("Longitud actual: ");
  Serial.println(LENGTH);
  float vol;
  if (h <= (2*r) && h >= 0) {
    vol = (pi*r*r - r*r*acos((r-h)/r) + (r-h)*sqrt(2*r*h-h*h))*l* 0.000264172;
  } else {
    vol = 0.01;
  }
  return vol;
}

void EEPROMSave() {
    EEprom_Data.l = LENGTH;
    EEprom_Data.d = DIAMETER;
    Serial.println("Guardado en Memoria:");
    Serial.println(EEprom_Data.l);
    Serial.println(EEprom_Data.d);
    EEPROM.begin(512);
    EEPROM.put(0, EEprom_Data);
    EEPROM.end();
    EEPROM.begin(512);
    EEPROM.put(0, EEprom_Data);
    EEPROM.end();
}

void EEPROMLoad() {
  EEPROM.begin(512);
  EEPROM.get(0,EEprom_Data);
  EEPROM.commit();
  EEPROM.end();
  DIAMETER=EEprom_Data.d;
  LENGTH=EEprom_Data.l;
}

