#define BLYNK_TEMPLATE_ID ""
#define BLYNK_TEMPLATE_NAME "ESP32 - PINZA MÚSCULO"
#define BLYNK_AUTH_TOKEN ""

// Librerías y códigos a importar:
#include <ESP32Servo.h>        //Servo
#include <Wire.h>              // Para el protocolo I2C de la pantalla
#include <Adafruit_GFX.h>      // Pantalla
#include <Adafruit_SSD1306.h>  // Pantalla
#include <WiFi.h>              // Wifi
#include <WiFiClient.h>        // Wifi
#include <BlynkSimpleEsp32.h>  //Blynk

// Imágenes
#include "Brazo.h"


// Puertos
#define pinServo 18
#define musculoECG 34
// SCL 22
//SDA 21
#define LED 2

// Propiedades pantalla
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define BLYNK_PRINT Serial  // Para DEBUG, quitar cuando no haga falta

// Obj para las librerias
Servo servo;
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
BlynkTimer timer;


// Establecemos las variables a usar de forma global
char ssid[] = "";  // cambiar por SSID
char pass[] = "";
unsigned long mili;
int encendido;

// Filtro de media movil
float musculo = 0;
float MUS_filtrado = 0;
float alpha = 0.5;  // Valor de filtrado

unsigned long t_lectura_ant;
int n_lectura = 50;

// Funciones personalizadas
void textoPantalla(String texto, bool scroll) {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0, 10);
  display.println(texto);
  display.display();
  if (scroll) {
    display.startscrollright(0x00, 0x0F);
  } else {
    display.stopscroll();
  }

  // Mostramos el texto tambien en la web y app
  Blynk.virtualWrite(V5, texto);
}

void myTimerEvent()  // Sacado de documentation
{
  Blynk.virtualWrite(V2, millis() / 1000);
}


BLYNK_WRITE(V0) { // Función ejecutada cuando el pin virtual V0 cambia (botón de encendido)
  encendido = param.asInt();
  display.clearDisplay();
  display.drawBitmap(0, 0, image_data_Brazo, 128, 64, 1);
  display.display();
  digitalWrite(LED, LOW);
  Serial.println("Cambio en estado del sistema.");
}


void setup() {
  Serial.begin(115200);  // Para poder usar consola.

  // Iniciamos la pantalla
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("Error al conectar la pantalla.\n Recuerde pines:\n\t SCL: 22 \n\t SDA: 21"));
    pinMode(LED, OUTPUT);
    while(true){ // DEBUG Físico sin Serial
      digitalWrite(LED, HIGH);
      delay(200); // Como ha habido un error podemos usar un delay, no se procede con el resto de Blynk
      digitalWrite(LED, LOW);
      delay(200);
    }
  }

  textoPantalla("Iniciando", false);


  pinMode(pinServo, OUTPUT);
  servo.attach(pinServo);
  servo.write(45);  // Ponemos la pinza en abierto como estado por defecto.

  pinMode(musculoECG, INPUT);

  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
  timer.setInterval(1000L, myTimerEvent);

  Blynk.virtualWrite(V4, 0); // Ponemos a 0 el LED Virtual del Dashboard web de Blynk.
  encendido = 1;

  display.clearDisplay();
  display.display();

  Blynk.virtualWrite(V4, 255);  // Para mostrar verde en web

  pinMode(LED, OUTPUT);  // Uso de LED interno para debug físico
}

void loop() {

  Blynk.run();


  timer.run();  // Para poner el Uptime en la web.

  //musculo = random(1, 4095);  // Quitar para uso en producción
  // Si está encendido medimos y hacemos media movil, aunque no movamos la pinza, para evitar escalones
  if (millis() - t_lectura_ant > n_lectura && encendido == 1) {
    t_lectura_ant = millis();
    // Obtenemos los datos del sensor del músculo
    musculo = analogRead(musculoECG);
    MUS_filtrado = (alpha * musculo) + ((1.0 - alpha) * MUS_filtrado);

    Serial.println(String(musculo) + "," + String(MUS_filtrado, 1));  // Para Excel (no va si el resto de Serial.println estan activados)
    digitalWrite(LED, LOW);
  }

  //Lógica pinza
  if ((millis() - mili) >= 500 && encendido == 1) {  // Ejecutamos cada 0.5s
    mili = millis();
    digitalWrite(LED, HIGH);
    Blynk.run();
    Blynk.virtualWrite(V7, MUS_filtrado);
    Blynk.virtualWrite(V8, musculo);  // Enviamos valores cada 0.5s para evitar usar muchas llamadas.
    // Lógica pinza
    if (MUS_filtrado >= 2047) {  // Si detectamos fuersza cerramos.
      servo.write(90);
      Serial.println("Pinza cerrada, detectado valor superior a 2047 en sensor");
      textoPantalla("Cerrada\nhaciendo\nfuerza.", false); // \n para que salga en la siguiente línea con Unicode


    } else {
      // Si no detectamos, abrimos.
      servo.write(45);
      Serial.println("Pinza abierta.");
      textoPantalla("Abierta.", false);
    }
  }
}
