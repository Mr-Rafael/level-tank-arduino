#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <ArduinoJson.h>


// Identificación de la red local
char* ssid = "ECOLGB";
char* password = "ecolgb123";
IPAddress ip(192,168,2,101);
IPAddress gateway(192,168,1,1);
IPAddress subnet(255,255,255,0);


StaticJsonBuffer<200> jsonBuffer;
JsonObject& generatorJson = jsonBuffer.createObject();
JsonObject& batteryJson = jsonBuffer.createObject();                                //CAMBIO 8: se creó otro jsonObject para el envío de lecturas del voltaje de batería.

 int generatorPIN = D0;                                                              //CAMBIO 1: ahora se lee el encendido del MG usando el pin digital D0 en lugar del analógico A0.
 int batteryPIN = A0;                                                                //CAMBIO 2: ahora se utiliza el pin A0 para leer el voltaje de la batería de MG.
 int analogData;
 boolean generatorRunning;
 int statusChange = false;
 float batteryVoltage;
 boolean generatorState = false; 
 String meterId = "11"; // INGRESAR METER ID                                                                          //CAMBIO 7: se agregó un timer, para que se envíen lecturas de batería cada 5 segundos.
 
void setup() {
  pinMode(batteryPIN, INPUT);
  WiFi.begin(ssid,password);   // Inicio de sesión
  WiFi.config(ip, gateway, subnet);
  Serial.begin(115200);       // Rate usada por el modulo
  Serial.println("PROBANDO");
  while (WiFi.status() != WL_CONNECTED) {  //Wait for the WiFI connection completion
 
    delay(500);
    Serial.println("conectando...");
  }
 Serial.println("ID: ");
 Serial.println(meterId);
 //t.every(5000, sendBattery);                                                //CAMBIO 8: la rutina sendBattery se repite cada 5 segundos
}
 
void loop() {
   generatorRunning = digitalRead(generatorPIN);                            //CAMBIO 3: se hace digitalRead del pin D0 (encendido del MG).se cambió el nombre de la variable a generatorRunning
   analogData = analogRead(batteryPIN);                                     //CAMBIO 4: se hace analogRead del pin A0 (voltaje de batería).
   batteryVoltage = analogData * (3.3/1023.0) * (13.3/3.3);             //CAMBIO 5: se calcula el voltaje en A0. Luego, se calcula el voltaje de batería usando el divisor de voltaje.
   Serial.println(batteryVoltage);
   Serial.println(batteryVoltage);
   generatorStateON();
   generatorStateOFF();
   if(generatorRunning){  //CMOS                                              CAMBIO 6: se quitó la comparación de voltaje que existía en el IF por una verificación lógica.
      generatorState = true;
   }
   else{
      generatorState = false;
   }
   
   Serial.print("MOTOR: ");
   Serial.println(generatorState);
   Serial.print("VOLTAGE: ");
   Serial.println(batteryVoltage);
   sendBattery();
  delay(5000);
}

void generatorStateON(){

    HTTPClient http;    //Declare object of class HTTPClient
    
    //Datos de Cliente set-Motor
    http.begin("http://192.168.2.221:4000/set-motor");      //Specify request destination
    http.addHeader("Content-Type", "application/json");  //Specify content-type header
    
    //Creando JSON GENERATORDATA
    generatorJson["is_on"] = true;
    generatorJson["meter_id"] = meterId;
    String generatorData;
    generatorJson.printTo(generatorData); 
    int httpCode = http.POST(generatorData);   //Send the request
    String payload = http.getString();                  //Get the response payload
    
    //Datos en Pantalla
    Serial.print("RESPUESTA DEL SERVIDOR: ");
    Serial.println(payload);    //Print request response payload
    http.end();  //Close connection;   
    
   


}
void generatorStateOFF(){

     HTTPClient http;    //Declare object of class HTTPClient
  
     //Datos de Cliente set-Motor
     http.begin("http://192.168.2.221:4000/set-motor");      //Specify request destination
     http.addHeader("Content-Type", "application/json");  //Specify content-type header
     
     //Creando JSON GENERATORDATA
     generatorJson["is_on"] = false;
     generatorJson["meter_id"] = meterId;
     String generatorData;
     generatorJson.printTo(generatorData); 
     int httpCode = http.POST(generatorData);   //Send the request
     String payload = http.getString();                  //Get the response payload
  
     //Datos en Pantalla
     Serial.print("RESPUESTA DEL SERVIDOR: ");
     Serial.println(payload);    //Print request response payload
     http.end();  //Close connection;    
  
  
}



void sendBattery(){                                        

    
     //CAMBIO 9: se creó una rutina que ocurre cada 2 segundos, que envía la lectura de batería.
     HTTPClient http;    //Declare object of class HTTPClient
     Serial.println("test--------------------------------------------");
     //Datos de Cliente set-Motor
     http.begin("http://192.168.2.221:4000/send-voltage-battery");      //Specify request destination
     http.addHeader("Content-Type", "application/json");  //Specify content-type header
     
     //Creando JSON BATTERYDATA
     batteryJson["battery_voltage"] = (float) batteryVoltage;
     batteryJson["is_on"] = generatorState;
     batteryJson["meter_id"] = meterId;
     String batteryData;
     batteryJson.printTo(batteryData); 
     Serial.println(batteryData);
     int httpCode = http.POST(batteryData);   //Send the request
     String payload = http.getString();                  //Get the response payload
  
     //Datos en Pantalla
     Serial.print("RESPUESTA DEL SERVIDOR: ");
     Serial.println(payload);    //Print request response payload
     http.end();  //Close connection;       
  
}
