



/*

El programa se encarga de detectar un objeto, medir su componente RGB para luego clasificarlo por color.
Utiliza driver para motor de CC, servomotor para clasificación, sensor RGB y LED de alta potencia.
Programado en ESP32.

Autor: Martín Cioffi
Fecha: 23-11-2023
Versión: 1.00
Repositorio: https://github.com/martuan/cintaTransportadoraRGB




*/



#include <BluetoothSerial.h>
#include "Adafruit_APDS9960.h"
#include <ESP32Servo.h>
#include "ColoresPatrones.h"
#include <Adafruit_NeoPixel.h>

#define VELOCIDAD_INICIAL 150//200
#define VELOCIDAD_FINAL 250//255
#define NUMPIXELS 16
#define PIN 19//TX0

//**************************************
//*********** BLUETOOTH  *****************
//**************************************

BluetoothSerial SerialBT;

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif



Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

Servo myservo;  // create servo object to control a servo

coloresPatrones ROJO_PATRON;
coloresPatrones VERDE_PATRON;
coloresPatrones AZUL_PATRON;

//create the APDS9960 object
Adafruit_APDS9960 apds;

int numDePantalla = 1;
int cambioDeEstado = 0;
int flagComenzarEnsayo = 1;


// Motor A
int motor1Pin1 = 26;//27; 
int motor1Pin2 = 27;//26; 
int motor1PinENABLE = 25;//14; 

// Setting PWM properties
const int freq = 30000;
const int pwmChannel = 3;
const int resolution = 8;
//int dutyCycle = 200;
int dutyCycle = 200;
int i = VELOCIDAD_INICIAL;
int demoraDelay = 50;
bool sentido = 0;
int rebote = 0;

char flagGiroUnidireccional = 1;
int valorADC = 0;
int pinPote = 35;
int valorMap = 0;
int proximidad = 0;
int PROXIMIDAD_UMBRAL = 7;//6;
char flagSentidoMotor = 'B';
String colorDetectado = {};
int UMBRAL_ROJO = 80;
int UMBRAL_VERDE = 30;
int UMBRAL_AZUL = 30;
const int pwmChannelServo = 1;
int contadorRojo = 0;
int contadorVerde = 0;
int contadorAzul = 0;
int toleranciaColor = 25;
int RGB_r = 0;
int RGB_g = 0;
int RGB_b = 0;
int flagSimulacionColor = 0;
int demoraEntrePasos = 25;

//------------------------------------------------------------------------------------------

void setup() {
  
	// sets the pins as outputs:
	pinMode(motor1Pin1, OUTPUT);
	pinMode(motor1Pin2, OUTPUT);
	pinMode(motor1PinENABLE, OUTPUT);
	//pinMode(pinPote, INPUT);

	// configure LED PWM functionalitites
	ledcSetup(pwmChannel, freq, resolution);
	ledcAttachPin(motor1PinENABLE, pwmChannel);

	int channel = myservo.attach(14);  // attaches the servo on pin 13 to the servo object
	
	Serial.begin(115200);
	SerialBT.begin("cintaRGB-MNC"); //Bluetooth device name

	// testing
	Serial.println("Testing DC Motor...");

	Serial.print("Channel PWM Servo = ");
	Serial.println(channel);

	// Inicializamos nuestra cinta led RGB
	pixels.begin(); 

	for(int i=0;i<NUMPIXELS;i++){

		pixels.setPixelColor(i, 200, 100, 200); //

		pixels.show();   // Mostramos y actualizamos el color del pixel de nuestra cinta led RGB

		delay(10); // Pausa por un periodo de tiempo (en milisegundos).

  	}

	if(!apds.begin()){
		Serial.println("failed to initialize device! Please check your wiring.");
	}
	else Serial.println("Device initialized!");

	//enable proximity mode
	apds.enableProximity(true);

	//enable color sensign mode
	apds.enableColor(true);

	apds.setADCGain(APDS9960_AGAIN_64X);

	digitalWrite(motor1Pin1, HIGH);
	digitalWrite(motor1Pin2, LOW);
	ledcWrite(pwmChannel, 255);

}

//------------------------------------------------------------------------------------------

void loop(void) {


	cambioDeParametros();//chequea cambio de parámetros por bluetooth o por serie
/*
	if(flagSentidoMotor == 'F'){
		digitalWrite(motor1Pin1, LOW);
		digitalWrite(motor1Pin2, HIGH);
		//Serial.println("Sentido: F");


	}else if(flagSentidoMotor == 'B'){
		digitalWrite(motor1Pin1, HIGH);
		digitalWrite(motor1Pin2, LOW);
		//Serial.println("Sentido: B");

	}else if(flagSentidoMotor == 'S'){
		digitalWrite(motor1Pin1, HIGH);
		digitalWrite(motor1Pin2, HIGH);
		//Serial.println("Sentido: S");

	}
*/
	ledcWrite(pwmChannel, 255);

	proximidad = apds.readProximity();
	delay(50);
/*
	if(flagSimulacionColor == 1){

		detectarColor();

	}
*/		
	//Serial.println(proximidad);
	if(proximidad >= PROXIMIDAD_UMBRAL){
		Serial.println("*******Objeto detectado*********");
		Serial.println("Stop");
		
		
		//frenar motor y leer RGB	
		digitalWrite(motor1Pin1, HIGH);
		digitalWrite(motor1Pin2, HIGH);

		ledcWrite(pwmChannel, 255);

		//detectarColor();
		detectarColorAvanzado(demoraEntrePasos);
		//microAvanceMotorCC();



		if(colorDetectado == "ROJO"){
			
			myservo.write(135);//coloca el servo
			contadorRojo++;


		}else if(colorDetectado == "VERDE"){
			
			myservo.write(90);//coloca el servo
			contadorVerde++;

		}else if(colorDetectado == "AZUL"){
			
			myservo.write(45);//coloca el servo
			contadorAzul++;

		}else if(colorDetectado == "INDEFINIDO"){
			
			//myservo.write(15);//coloca el servo

		}

		Serial.print("              ");
		Serial.print("Contadores -->");
		Serial.print(" Rojo = ");
		Serial.print(contadorRojo);
		Serial.print(" Verde = ");
		Serial.print(contadorVerde);
		Serial.print(" Azul = ");
		Serial.println(contadorAzul);

		SerialBT.println("Contadores -->");
		SerialBT.print(" Rojo = ");
		SerialBT.print(contadorRojo);
		SerialBT.print(" Verde = ");
		SerialBT.print(contadorVerde);
		SerialBT.print(" Azul = ");
		SerialBT.println(contadorAzul);

		delay(500);

		//avanza la cinta
		digitalWrite(motor1Pin1, HIGH);
		digitalWrite(motor1Pin2, LOW);
		ledcWrite(pwmChannel, 255);

		delay(1000);
		//myservo.write(0);//retira el servo

		//delay(200);


	} 
}

void cambiarSentidoMotor(void){

	sentido = !sentido;//invierte el estado anterior

	if(sentido == 0){

		Serial.println("Horario");
		digitalWrite(motor1Pin1, LOW);
		digitalWrite(motor1Pin2, HIGH);

	}else{

		Serial.println("Antihorario");	
		digitalWrite(motor1Pin1, HIGH);
		digitalWrite(motor1Pin2, LOW);

	}

}


//puede cambiar parámetros a través del puerto serie o por bluetooth
//Se debe enviar un caracter de identificación del parámetro a cambiar y
//luego el valor.
//Por ejemplo: cambiar el tiempo entre lecturas de temperatura
//enviar T100  siendo T: tiempoEntreLecturas; 100: 100 ms
//los parámetros que se pueden modificar son:
//  distanciaConfigurada--> D;
//  distanciaTolerancia--> t;
//  tempFiebre--> F;
//  tempMin--> m;
//  tempMax--> M;
//  tempOffset--> O;
//  tiempoEntreLecturas--> T;
//  cantLecturas--> C;
//  emisividad--> E;
//  Wifi--> W;  [Ejemplo: Wmyssid mypassword](El espacio se usa como delimitador)
//  debug--> d  [1 para activarlo; 0 para desactivarlo]
//  cantSensoresIR-->S
//  consultarLecturas-->P
//  escannearDispositivosI2C-->s  [1 para activarlo; 0 para desactivarlo]
//  cambiarDireccionI2C-->A       [A90 91]
//  analizarLecturasCantidad-->U
//  intercambiarSensores-->I;
void cambioDeParametros(void){

  char charParamID = ' ';
  String valorParam = "";
  int inChar = 0;
  String inString = "";
    
  
  //**** Chequeo por Serie o Bluetooth ***************
  while (Serial.available() > 0 || SerialBT.available() > 0) {

    if(Serial.available() > 0){
      inChar = Serial.read();
    }else if(SerialBT.available() > 0){
      inChar = SerialBT.read();
    }
    

    if(inChar != '\n'){
      Serial.print((char)inChar);

      inString += (char)inChar;//encola los caracteres recibidos

    }else{//si llegó el caracter de terminación
      
      Serial.print("Input string: ");
      Serial.println(inString);
      Serial.print("string length: ");
      Serial.println(inString.length());


      //obtiene el identificador
      charParamID = inString.charAt(0);
      
      Serial.println(charParamID);
      
      //obtiene el valor
      for(int i = 1; i < inString.length(); i++){
        valorParam += inString.charAt(i);
      }

      Serial.println(valorParam);

      //evalua el identificador y los parámetros enviados
      switchCaseParametros(charParamID, valorParam);
      
      //borra el contenido y lo prepara para recibir uno nuevo
      inString = "";
    
    }
  }

}




void switchCaseParametros(char charParamID, String valorParam){

  int inChar = 0;
  int index = 0;
  int valorParamLength = 0;
  int endIndex = 0;
  int modoDebug = 0;
  int consultarLecturas = 0;
  int correccionActivada = 0;
  uint8_t numSensor = 0;
  uint16_t direccion = 0;
  int scanActivado = 0;
  byte oldAddress = 0;
  byte newAddress = 0;
  int analizarLecturasCantidad = 0;
  int intercambioSensores = 0;
  int color = 0;
  String nombreSensor = "";
  int velocidad = 0;
  String colorSimulado = {};
  int r, g, b;

  
  //valorParam = 
  valorParam.replace(0x0A,'\0');//Se filtra el caracter LF
  valorParam.replace(0x0D,'\0');//Se filtra el caracter CR

  switch(charParamID){
    case 'F':
			Serial.println("Forward");
			SerialBT.println("Forward");

		accionarMotor(charParamID, valorParam.toInt());
      
    break;
	case '8':
			Serial.println("Forward");
			SerialBT.println("Forward");

		accionarMotor(charParamID, valorParam.toInt());
      
    break;
    case 'B':
//	case '2':
			
		Serial.print("Back: ");
		SerialBT.println("Back");


		accionarMotor(charParamID, valorParam.toInt());	
    break;
	case '2':
//	case '2':
			
		Serial.print("Back: ");
		SerialBT.println("Back");


		accionarMotor(charParamID, valorParam.toInt());	
    break;


	case 'S':
		/*
		if(cuentaLeftRight < 5){//aumenta o disminuye solo si esta dentro del rango permitido
			cuentaLeftRight++;
		}
		*/
		Serial.println("Detener");
	

		accionarMotor(charParamID, 0);
    break;
	case '5':
		/*
		if(cuentaLeftRight < 5){//aumenta o disminuye solo si esta dentro del rango permitido
			cuentaLeftRight++;
		}
		*/
		SerialBT.println("Detener");
	

		accionarMotor(charParamID, 0);
    break;
	case 'U':
		/*
		if(cuentaLeftRight < 5){//aumenta o disminuye solo si esta dentro del rango permitido
			cuentaLeftRight++;
		}
		*/
		
		
		PROXIMIDAD_UMBRAL = valorParam.toInt();
		Serial.println("Umbral = ");
		Serial.println(PROXIMIDAD_UMBRAL);

		accionarMotor(charParamID, 0);
    break;
	
	case 'D':
		
		if(valorParam.toInt() >= 10 && valorParam.toInt() <= 100){
			demoraEntrePasos = valorParam.toInt();
		}else{
			Serial.println("Valor inaceptable");
		}
		
		

	break;


	case 'C'://autocalibracion

		color = valorParam.toInt();
		autocalibracion(color, demoraEntrePasos);

	break;
	case 'T'://tolerancia

		toleranciaColor = valorParam.toInt();

	break;

	case 'R'://tolerancia

		flagSimulacionColor = 1; 
		colorSimulado = valorParam;
		simulacionRGB(colorSimulado);

	break;
	case 'N'://tolerancia

		r = valorParam.substring(0,4).toInt();
		g = valorParam.substring(4,8).toInt();
		b = valorParam.substring(8,12).toInt();
		
		Serial.printf("setearNeopixel r g b = %i, %i, %i", r, g, b);

		setearColorNeopixel(r, g, b);
		

	break;
    default:
      Serial.println("Parámetro incorrecto");
    break;

  }  
}
//duty: entre 0 y 255
void accionarMotor(char param, int duty){

	switch (param)
	{
	case 'F':
		//apagar LEDs de señalizadores que no corresponden y encender el indicado
		Serial.println("Horario");	

		digitalWrite(motor1Pin1, HIGH);
		digitalWrite(motor1Pin2, LOW);
		flagSentidoMotor = 'F';
		ledcWrite(pwmChannel, duty);   
		//delay(demoraDelay);

	break;
	case '8':
		//apagar LEDs de señalizadores que no corresponden y encender el indicado
		Serial.println("Horario");	
		
		digitalWrite(motor1Pin1, HIGH);
		digitalWrite(motor1Pin2, LOW);
		flagSentidoMotor = 'F';
		ledcWrite(pwmChannel, duty);   
		//delay(demoraDelay);

	break;
	case 'B':
		//apagar LEDs de señalizadores que no corresponden y encender el indicado
		Serial.println("Antihorario");	
		
		digitalWrite(motor1Pin1, LOW);
		digitalWrite(motor1Pin2, HIGH);
		flagSentidoMotor = 'B';
		ledcWrite(pwmChannel, duty);   

	break;
	case '2':
		//apagar LEDs de señalizadores que no corresponden y encender el indicado
		Serial.println("Antihorario");	
		
		digitalWrite(motor1Pin1, LOW);
		digitalWrite(motor1Pin2, HIGH);
		flagSentidoMotor = 'B';
		ledcWrite(pwmChannel, duty);   

	break;
	case 'S':
		//apagar LEDs de señalizadores que no corresponden y encender el indicado
		Serial.println("Stop");	
		digitalWrite(motor1Pin1, HIGH);
		digitalWrite(motor1Pin2, HIGH);
		flagSentidoMotor = 'S';

	break;
	case '5':
		//apagar LEDs de señalizadores que no corresponden y encender el indicado
		Serial.println("Stop");	
		digitalWrite(motor1Pin1, HIGH);
		digitalWrite(motor1Pin2, HIGH);
		flagSentidoMotor = 'S';

	break;
		
	default:
		break;
	}
}

void detectarColor(void){

  //create some variables to store the color data in
  uint16_t r, g, b, c;
  float tolerencia = (float)toleranciaColor;

  //wait for color data to be ready
  while(!apds.colorDataReady()){
    delay(5);
  }

  mostrarPatronesRGB();

  //get the data and print the different channels
  apds.getColorData(&r, &g, &b, &c);
  Serial.print("red: ");
  Serial.print(r);
  
  Serial.print(" green: ");
  Serial.print(g);
  
  Serial.print(" blue: ");
  Serial.print(b);
  
  Serial.print(" clear: ");
  Serial.println(c);
  Serial.println();

  if(flagSimulacionColor == 1){

	r = (uint16_t)RGB_r;
	g = (uint16_t)RGB_g;
	b = (uint16_t)RGB_b;

	Serial.print("red: ");
  	Serial.print(r);
  
  	Serial.print(" green: ");
  	Serial.print(g);
  
  	Serial.print(" blue: ");
  	Serial.print(b);
  
  	Serial.print(" clear: ");
  	Serial.println(c);
  	Serial.println();

  }

  //delay(1000);

	/*

	if(r > g && r > b){

		Serial.println("ROJO");
		SerialBT.println("ROJO");
		colorDetectado = "ROJO";

	}else if(g > b && g > r){

		Serial.println("VERDE");
		SerialBT.println("VERDE");
		colorDetectado = "VERDE";

	}else if(b > g && r < UMBRAL_ROJO){

		Serial.println("AZUL");
		SerialBT.println("AZUL");
		colorDetectado = "AZUL";

	}else{
		Serial.println("No identificado");
		colorDetectado = "INDEFINIDO";
	}
*/

	float valorMasToleranciaROJO_r = (float)ROJO_PATRON.r + ((float)ROJO_PATRON.r * (tolerencia/100));
	float valorMenosToleranciaROJO_r = (float)ROJO_PATRON.r - ((float)ROJO_PATRON.r * (tolerencia/100));
	float valorMasToleranciaROJO_g = (float)ROJO_PATRON.g + ((float)ROJO_PATRON.g * (tolerencia/100));
	float valorMenosToleranciaROJO_g = (float)ROJO_PATRON.g - ((float)ROJO_PATRON.g * (tolerencia/100));
	float valorMasToleranciaROJO_b = (float)ROJO_PATRON.b + ((float)ROJO_PATRON.b * (tolerencia/100));
	float valorMenosToleranciaROJO_b = (float)ROJO_PATRON.b - ((float)ROJO_PATRON.b * (tolerencia/100));

	float valorMasToleranciaVERDE_r = (float)VERDE_PATRON.r + ((float)VERDE_PATRON.r * (tolerencia/100));
	float valorMenosToleranciaVERDE_r = (float)VERDE_PATRON.r - ((float)VERDE_PATRON.r * (tolerencia/100));
	float valorMasToleranciaVERDE_g = (float)VERDE_PATRON.g + ((float)VERDE_PATRON.g * (tolerencia/100));
	float valorMenosToleranciaVERDE_g = (float)VERDE_PATRON.g - ((float)VERDE_PATRON.g * (tolerencia/100));
	float valorMasToleranciaVERDE_b = (float)VERDE_PATRON.b + ((float)VERDE_PATRON.b * (tolerencia/100));
	float valorMenosToleranciaVERDE_b = (float)VERDE_PATRON.b - ((float)VERDE_PATRON.b * (tolerencia/100));

	float valorMasToleranciaAZUL_r = (float)AZUL_PATRON.r + ((float)AZUL_PATRON.r * (tolerencia/100));
	float valorMenosToleranciaAZUL_r = (float)AZUL_PATRON.r - ((float)AZUL_PATRON.r * (tolerencia/100));
	float valorMasToleranciaAZUL_g = (float)AZUL_PATRON.g + ((float)AZUL_PATRON.g * (tolerencia/100));
	float valorMenosToleranciaAZUL_g = (float)AZUL_PATRON.g - ((float)AZUL_PATRON.g * (tolerencia/100));
	float valorMasToleranciaAZUL_b = (float)AZUL_PATRON.b + ((float)AZUL_PATRON.b * (tolerencia/100));
	float valorMenosToleranciaAZUL_b = (float)AZUL_PATRON.b - ((float)AZUL_PATRON.b * (tolerencia/100));
	
	/*
	Serial.print("valorMasToleranciaROJO_r = ");
	Serial.println(valorMasToleranciaROJO_r);
	Serial.print("valorMenosToleranciaROJO_r = ");
	Serial.println(valorMenosToleranciaROJO_r);
*/
	Serial.printf("tolerancias: r | %f - %f |  g | %f - %f |  b | %f - %f |\r\n", valorMasToleranciaROJO_r, valorMenosToleranciaROJO_r, valorMasToleranciaROJO_g, valorMenosToleranciaROJO_g, valorMasToleranciaROJO_b, valorMenosToleranciaROJO_b);
	Serial.printf("tolerancias: r | %f - %f |  g | %f - %f |  b | %f - %f |\r\n", valorMasToleranciaVERDE_r, valorMenosToleranciaVERDE_r, valorMasToleranciaVERDE_g, valorMenosToleranciaVERDE_g, valorMasToleranciaVERDE_b, valorMenosToleranciaVERDE_b);
	Serial.printf("tolerancias: r | %f - %f |  g | %f - %f |  b | %f - %f |\r\n", valorMasToleranciaAZUL_r, valorMenosToleranciaAZUL_r, valorMasToleranciaAZUL_g, valorMenosToleranciaAZUL_g, valorMasToleranciaAZUL_b, valorMenosToleranciaAZUL_b);
	

/*
	if((r >= ROJO_PATRON.r + ((float)ROJO_PATRON.r * (tolerencia/100)) || r <= ROJO_PATRON.r - ((float)ROJO_PATRON.r * (tolerencia/100))) &&
		(g >= ROJO_PATRON.g + ((float)ROJO_PATRON.g * (tolerencia/100)) || g <= ROJO_PATRON.g - ((float)ROJO_PATRON.g * (tolerencia/100))) &&
			(b >= ROJO_PATRON.b + ((float)ROJO_PATRON.b * (tolerencia/100)) || b <= ROJO_PATRON.b - ((float)ROJO_PATRON.b * (tolerencia/100)))){

		Serial.println("ROJO");
		SerialBT.println("ROJO");
		colorDetectado = "ROJO";

	}else if((r >= VERDE_PATRON.r + ((float)VERDE_PATRON.r * (tolerencia/100)) || r <= VERDE_PATRON.r - ((float)VERDE_PATRON.r * (tolerencia/100))) &&
			(g >= VERDE_PATRON.g + ((float)VERDE_PATRON.g * (tolerencia/100)) || g <= VERDE_PATRON.g - ((float)VERDE_PATRON.g * (tolerencia/100))) &&
				(b >= VERDE_PATRON.b + ((float)VERDE_PATRON.b * (tolerencia/100)) || b <= VERDE_PATRON.b - ((float)VERDE_PATRON.b * (tolerencia/100)))){

		Serial.println("VERDE");
		SerialBT.println("VERDE");
		colorDetectado = "VERDE";

	}else if((r >= AZUL_PATRON.r + ((float)AZUL_PATRON.r * (tolerencia/100)) || r <= AZUL_PATRON.r - ((float)AZUL_PATRON.r * (tolerencia/100))) &&
			(g >= AZUL_PATRON.g + ((float)AZUL_PATRON.g * (tolerencia/100)) || g <= AZUL_PATRON.g - ((float)AZUL_PATRON.g * (tolerencia/100))) &&
				(b >= AZUL_PATRON.b + ((float)AZUL_PATRON.b * (tolerencia/100)) || b <= AZUL_PATRON.b - ((float)AZUL_PATRON.b * (tolerencia/100)))){

		Serial.println("AZUL");
		SerialBT.println("AZUL");
		colorDetectado = "AZUL";

	}else{
		Serial.println("No identificado");
		colorDetectado = "INDEFINIDO";
	}

	*/
	/*
	if((r >= valorMasToleranciaROJO_r || r <= valorMenosToleranciaROJO_r) &&
		(g >= valorMasToleranciaROJO_g || g <= valorMenosToleranciaROJO_g) &&
			(b >= valorMasToleranciaROJO_b || b <= valorMenosToleranciaROJO_b)){

		Serial.println("ROJO");
		SerialBT.println("ROJO");
		colorDetectado = "ROJO";

	}else if((r >= valorMasToleranciaVERDE_r || r <= valorMenosToleranciaVERDE_r) &&
		(g >= valorMasToleranciaVERDE_g || g <= valorMenosToleranciaVERDE_g) &&
			(b >= valorMasToleranciaVERDE_b || b <= valorMenosToleranciaVERDE_b)){

		Serial.println("VERDE");
		SerialBT.println("VERDE");
		colorDetectado = "VERDE";

	}else if((r >= valorMasToleranciaAZUL_r || r <= valorMenosToleranciaAZUL_r) &&
		(g >= valorMasToleranciaAZUL_g || g <= valorMenosToleranciaAZUL_g) &&
			(b >= valorMasToleranciaAZUL_b || b <= valorMenosToleranciaAZUL_b)){

		Serial.println("AZUL");
		SerialBT.println("AZUL");
		colorDetectado = "AZUL";

	}else{
		Serial.println("No identificado");
		colorDetectado = "INDEFINIDO";
	}
	*/

	if(r <= valorMasToleranciaROJO_r && r >= valorMenosToleranciaROJO_r){
		Serial.println("ROJO - 1. Cumple");
		if(g <= valorMasToleranciaROJO_g && g >= valorMenosToleranciaROJO_g){
			Serial.println("ROJO - 2. Cumple");
			if(b <= valorMasToleranciaROJO_b && b >= valorMenosToleranciaROJO_b){
				Serial.println("ROJO - 3. Cumple");
				
				Serial.println("ROJO");
				SerialBT.println("ROJO");
				colorDetectado = "ROJO";
			}
		}
		
			



	}else if(r <= valorMasToleranciaVERDE_r && r >= valorMenosToleranciaVERDE_r){
		Serial.println("VERDE - 1. Cumple");
		if(g <= valorMasToleranciaVERDE_g && g >= valorMenosToleranciaVERDE_g){
			Serial.println("VERDE - 2. Cumple");
			if(b <= valorMasToleranciaVERDE_b && b >= valorMenosToleranciaVERDE_b){
				Serial.println("VERDE - 3. Cumple");
						
				Serial.println("VERDE");
				SerialBT.println("VERDE");
				colorDetectado = "VERDE";
			
			}

		}
	
		
			



	}else if(r <= valorMasToleranciaAZUL_r && r >= valorMenosToleranciaAZUL_r){
		Serial.println("AZUL - 1. Cumple");
		if(g <= valorMasToleranciaAZUL_g && g >= valorMenosToleranciaAZUL_g){
			Serial.println("AZUL - 2. Cumple");
			if(b <= valorMasToleranciaAZUL_b && b >= valorMenosToleranciaAZUL_b){

				Serial.println("AZUL - 3. Cumple");

				Serial.println("AZUL");
				SerialBT.println("AZUL");
				colorDetectado = "AZUL";	
			
			}
		}
	
		
			

		

	}else{
		Serial.println("No identificado");
		colorDetectado = "INDEFINIDO";
	}

}

void autocalibracion(int colorCalibrado, int demoraPasosMotorCC){

	//colorCalibrado
	//1 = ROJO
	//2 = VERDE
	//3 = AZUL

	//create some variables to store the color data in
	uint16_t r, g, b, c;
	int r_prom, g_prom, b_prom, c_prom;
	int r_suma, g_suma, b_suma, c_suma = 0;
	int i, j;
	int matrizValoresRGB[10][3] = {};
	int x = 0;
	int posInicial = 2;
	int posFinal = 6;
	int contador = 0;
	float tolerencia = (float)toleranciaColor;

	r_suma = 0;
	g_suma = 0;
	b_suma = 0;
	c_suma = 0;

	if(colorCalibrado == 1){
		Serial.println("Calibrando ROJO - Ingrese solo una muestra");
	}else if(colorCalibrado == 2){
		Serial.println("Calibrando VERDE - Ingrese solo una muestra");
	}else if(colorCalibrado == 3){
		Serial.println("Calibrando AZUL - Ingrese solo una muestra");
	}


	ledcWrite(pwmChannel, 255);

	while(proximidad < PROXIMIDAD_UMBRAL){

		proximidad = apds.readProximity();
		delay(50);

		//Serial.println(proximidad);



	}
	
	//si detectó un objeto frena la cinta
	if(proximidad >= PROXIMIDAD_UMBRAL){
		Serial.println("*******Objeto detectado*********");
		Serial.println("Stop");
		
		
		//frenar motor y leer RGB	
		digitalWrite(motor1Pin1, HIGH);
		digitalWrite(motor1Pin2, HIGH);

		ledcWrite(pwmChannel, 255);

	}

	delay(500);
	//Luego lee el color


	//wait for color data to be ready
	while(!apds.colorDataReady()){
		delay(5);
	}

	for(i = 0; i < 10; i++){

		for(j = 0; j < 1; j++){

			//get the data and print the different channels
			apds.getColorData(&r, &g, &b, &c);
			Serial.print("red: ");
			Serial.print(r);
			
			Serial.print(" green: ");
			Serial.print(g);
			
			Serial.print(" blue: ");
			Serial.print(b);
			
			Serial.print(" clear: ");
			Serial.println(c);
			Serial.println();

		}
		

		microAvanceMotorCC(demoraPasosMotorCC);

		matrizValoresRGB[i][0] = r;
		matrizValoresRGB[i][1] = g;
		matrizValoresRGB[i][2] = b;

		r_suma += r;
		g_suma += g;
		b_suma += b;

		

	}


	
	//obtiene los promedios
	r_prom = r_suma/i;
	g_prom = g_suma/i;
	b_prom = b_suma/i;

	Serial.printf("Promedios --> red: %i  green: %i  blue: %i\r\n", r_prom, g_prom, b_prom);

	//limpia las sumas para comenzar desde cero
	r_suma = 0;
	g_suma = 0;
	b_suma = 0;

	//descarta los primeros y últimos valores del array
	for(x = posInicial; x < posFinal; x++){

		r_suma += matrizValoresRGB[x][0];
		Serial.printf("matrizValorRGB = %i\n", matrizValoresRGB[x][0]);
		g_suma += matrizValoresRGB[x][1];
		b_suma += matrizValoresRGB[x][2];
		contador++;

	}

	Serial.printf("la suma de r es = %i\r\n", r_suma);
	Serial.printf("valor de x es = %i\r\n", x);


	//obtiene los promedios
	r_prom = r_suma/contador;
	g_prom = g_suma/contador;
	b_prom = b_suma/contador;

	//limpia las sumas para comenzar desde cero
	r_suma = 0;
	g_suma = 0;
	b_suma = 0;

	//Serial.println(r_suma);
	//Serial.println(g_suma);
	//Serial.println(b_suma);

	Serial.printf("Promedios de la matriz--> red: %i  green: %i  blue: %i\r\n", r_prom, g_prom, b_prom);
	

	//asignación de componentes RGB según la muestra
/*
	if(colorCalibrado == 1){

		ROJO_PATRON.r = (int)r;
		ROJO_PATRON.g = (int)g;
		ROJO_PATRON.b = (int)b;

	}else if(colorCalibrado == 2){

		VERDE_PATRON.r = (int)r;
		VERDE_PATRON.g = (int)g;
		VERDE_PATRON.b = (int)b;


	}else if(colorCalibrado == 3){

		AZUL_PATRON.r = (int)r;
		AZUL_PATRON.g = (int)g;
		AZUL_PATRON.b = (int)b;


	}
*/


	if(colorCalibrado == 1){

		ROJO_PATRON.r = (int)r_prom;
		ROJO_PATRON.g = (int)g_prom;
		ROJO_PATRON.b = (int)b_prom;

	}else if(colorCalibrado == 2){

		VERDE_PATRON.r = (int)r_prom;
		VERDE_PATRON.g = (int)g_prom;
		VERDE_PATRON.b = (int)b_prom;


	}else if(colorCalibrado == 3){

		AZUL_PATRON.r = (int)r_prom;
		AZUL_PATRON.g = (int)g_prom;
		AZUL_PATRON.b = (int)b_prom;


	}

	delay(5000);

	Serial.println("Finalizó de calibrar el color");

	//avanza la cinta
	digitalWrite(motor1Pin1, HIGH);
	digitalWrite(motor1Pin2, LOW);
	ledcWrite(pwmChannel, 255);

	delay(2000);
	


}

void mostrarPatronesRGB(void){

	Serial.println("ROJO_PATRON = ");
	Serial.printf("r = %i, g = %i, b = %i\r\n", ROJO_PATRON.r, ROJO_PATRON.g, ROJO_PATRON.b);
	Serial.println("VERDE_PATRON = ");
	Serial.printf("r = %i, g = %i, b = %i\r\n", VERDE_PATRON.r, VERDE_PATRON.g, VERDE_PATRON.b);
	Serial.println("AZUL_PATRON = ");
	Serial.printf("r = %i, g = %i, b = %i\r\n", AZUL_PATRON.r, AZUL_PATRON.g, AZUL_PATRON.b);

}

void simulacionRGB(String colorSimulado){

	if(colorSimulado == "ROJO"){

		RGB_r = ROJO_PATRON.r + 2;
		RGB_g = ROJO_PATRON.g + 2;
		RGB_b = ROJO_PATRON.b + 2;

	}else if(colorSimulado == "VERDE"){

		RGB_r = VERDE_PATRON.r + 2;
		RGB_g = VERDE_PATRON.g + 2;
		RGB_b = VERDE_PATRON.b + 2;

	}else if(colorSimulado == "AZUL"){

		RGB_r = AZUL_PATRON.r + 2;
		RGB_g = AZUL_PATRON.g + 2;
		RGB_b = AZUL_PATRON.b + 2;


	}



}

void microAvanceMotorCC(int){

	//avanzar motor
	digitalWrite(motor1Pin1, HIGH);
	digitalWrite(motor1Pin2, LOW);

	ledcWrite(pwmChannel, 255);

	delay(demoraEntrePasos);//lapso de avance

	//frenar motor
	digitalWrite(motor1Pin1, HIGH);
	digitalWrite(motor1Pin2, HIGH);

	ledcWrite(pwmChannel, 255);

	delay(demoraEntrePasos);//lapso de frenado (motor quieto)


}

void detectarColorAvanzado(int demoraPasosMotorCC){

	//create some variables to store the color data in
	uint16_t r, g, b, c;
	int r_prom, g_prom, b_prom, c_prom;
	int r_suma, g_suma, b_suma, c_suma = 0;
	int i, j;
	int matrizValoresRGB[10][3] = {};
	int x = 0;
	int posInicial = 2;
	int posFinal = 6;
	int contador = 0;
	float tolerencia = (float)toleranciaColor;
	int flagCumpleCondiciones = 0;

	r_suma = 0;
	g_suma = 0;
	b_suma = 0;
	c_suma = 0;


	//wait for color data to be ready
	while(!apds.colorDataReady()){
		delay(5);
	}

	mostrarPatronesRGB();

	for(i = 0; i < 10; i++){

		for(j = 0; j < 1; j++){

			//get the data and print the different channels
			apds.getColorData(&r, &g, &b, &c);
			Serial.print("red: ");
			Serial.print(r);
			
			Serial.print(" green: ");
			Serial.print(g);
			
			Serial.print(" blue: ");
			Serial.print(b);
			
			Serial.print(" clear: ");
			Serial.println(c);
			Serial.println();

		}
		

		microAvanceMotorCC(demoraPasosMotorCC);

		matrizValoresRGB[i][0] = r;
		matrizValoresRGB[i][1] = g;
		matrizValoresRGB[i][2] = b;

		r_suma += r;
		g_suma += g;
		b_suma += b;

		

	}


	
	//obtiene los promedios
	r_prom = r_suma/i;
	g_prom = g_suma/i;
	b_prom = b_suma/i;

	Serial.printf("Promedios --> red: %i  green: %i  blue: %i\r\n", r_prom, g_prom, b_prom);

	//limpia las sumas para comenzar desde cero
	r_suma = 0;
	g_suma = 0;
	b_suma = 0;

	//descarta los primeros y últimos valores del array
	for(x = posInicial; x < posFinal; x++){

		r_suma += matrizValoresRGB[x][0];
		Serial.printf("matrizValorRGB = %i\n", matrizValoresRGB[x][0]);
		g_suma += matrizValoresRGB[x][1];
		b_suma += matrizValoresRGB[x][2];
		contador++;

	}

	Serial.printf("la suma de r es = %i\r\n", r_suma);
	Serial.printf("valor de x es = %i\r\n", x);


	//obtiene los promedios
	r_prom = r_suma/contador;
	g_prom = g_suma/contador;
	b_prom = b_suma/contador;

	//limpia las sumas para comenzar desde cero
	r_suma = 0;
	g_suma = 0;
	b_suma = 0;

	//Serial.println(r_suma);
	//Serial.println(g_suma);
	//Serial.println(b_suma);

	Serial.printf("Promedios de la matriz--> red: %i  green: %i  blue: %i\r\n", r_prom, g_prom, b_prom);
	

	//************************************************
	// Detección del color
	//************************************************

	float valorMasToleranciaROJO_r = (float)ROJO_PATRON.r + ((float)ROJO_PATRON.r * (tolerencia/100));
	float valorMenosToleranciaROJO_r = (float)ROJO_PATRON.r - ((float)ROJO_PATRON.r * (tolerencia/100));
	float valorMasToleranciaROJO_g = (float)ROJO_PATRON.g + ((float)ROJO_PATRON.g * (tolerencia/100));
	float valorMenosToleranciaROJO_g = (float)ROJO_PATRON.g - ((float)ROJO_PATRON.g * (tolerencia/100));
	float valorMasToleranciaROJO_b = (float)ROJO_PATRON.b + ((float)ROJO_PATRON.b * (tolerencia/100));
	float valorMenosToleranciaROJO_b = (float)ROJO_PATRON.b - ((float)ROJO_PATRON.b * (tolerencia/100));

	float valorMasToleranciaVERDE_r = (float)VERDE_PATRON.r + ((float)VERDE_PATRON.r * (tolerencia/100));
	float valorMenosToleranciaVERDE_r = (float)VERDE_PATRON.r - ((float)VERDE_PATRON.r * (tolerencia/100));
	float valorMasToleranciaVERDE_g = (float)VERDE_PATRON.g + ((float)VERDE_PATRON.g * (tolerencia/100));
	float valorMenosToleranciaVERDE_g = (float)VERDE_PATRON.g - ((float)VERDE_PATRON.g * (tolerencia/100));
	float valorMasToleranciaVERDE_b = (float)VERDE_PATRON.b + ((float)VERDE_PATRON.b * (tolerencia/100));
	float valorMenosToleranciaVERDE_b = (float)VERDE_PATRON.b - ((float)VERDE_PATRON.b * (tolerencia/100));

	float valorMasToleranciaAZUL_r = (float)AZUL_PATRON.r + ((float)AZUL_PATRON.r * (tolerencia/100));
	float valorMenosToleranciaAZUL_r = (float)AZUL_PATRON.r - ((float)AZUL_PATRON.r * (tolerencia/100));
	float valorMasToleranciaAZUL_g = (float)AZUL_PATRON.g + ((float)AZUL_PATRON.g * (tolerencia/100));
	float valorMenosToleranciaAZUL_g = (float)AZUL_PATRON.g - ((float)AZUL_PATRON.g * (tolerencia/100));
	float valorMasToleranciaAZUL_b = (float)AZUL_PATRON.b + ((float)AZUL_PATRON.b * (tolerencia/100));
	float valorMenosToleranciaAZUL_b = (float)AZUL_PATRON.b - ((float)AZUL_PATRON.b * (tolerencia/100));
	
	Serial.printf("tolerancias: r | %f - %f |  g | %f - %f |  b | %f - %f |\r\n", valorMasToleranciaROJO_r, valorMenosToleranciaROJO_r, valorMasToleranciaROJO_g, valorMenosToleranciaROJO_g, valorMasToleranciaROJO_b, valorMenosToleranciaROJO_b);
	Serial.printf("tolerancias: r | %f - %f |  g | %f - %f |  b | %f - %f |\r\n", valorMasToleranciaVERDE_r, valorMenosToleranciaVERDE_r, valorMasToleranciaVERDE_g, valorMenosToleranciaVERDE_g, valorMasToleranciaVERDE_b, valorMenosToleranciaVERDE_b);
	Serial.printf("tolerancias: r | %f - %f |  g | %f - %f |  b | %f - %f |\r\n", valorMasToleranciaAZUL_r, valorMenosToleranciaAZUL_r, valorMasToleranciaAZUL_g, valorMenosToleranciaAZUL_g, valorMasToleranciaAZUL_b, valorMenosToleranciaAZUL_b);

	//reasigna los valores (componente = componente_promedio) para compatibilidad con los condicionales
	r = r_prom;
	g = g_prom;
	b = b_prom;



	if(flagCumpleCondiciones == 0){//si no ha cumplido las tres condiciones (no definió color) comienza a evaluar

		if(r <= valorMasToleranciaROJO_r && r >= valorMenosToleranciaROJO_r){
			Serial.println("ROJO - 1. Cumple");
			if(g <= valorMasToleranciaROJO_g && g >= valorMenosToleranciaROJO_g){
				Serial.println("ROJO - 2. Cumple");
				if(b <= valorMasToleranciaROJO_b && b >= valorMenosToleranciaROJO_b){
					Serial.println("ROJO - 3. Cumple");
					
					Serial.println("ROJO");
					SerialBT.println("ROJO");
					colorDetectado = "ROJO";
					flagCumpleCondiciones = 1;
				}
			}
		}

	}
	
	
	if(flagCumpleCondiciones == 0){//si no ha cumplido las tres condiciones (no definió color) comienza a evaluar

		if(r <= valorMasToleranciaVERDE_r && r >= valorMenosToleranciaVERDE_r){
			Serial.println("VERDE - 1. Cumple");
			if(g <= valorMasToleranciaVERDE_g && g >= valorMenosToleranciaVERDE_g){
				Serial.println("VERDE - 2. Cumple");
				if(b <= valorMasToleranciaVERDE_b && b >= valorMenosToleranciaVERDE_b){
					Serial.println("VERDE - 3. Cumple");
							
					Serial.println("VERDE");
					SerialBT.println("VERDE");
					colorDetectado = "VERDE";
					flagCumpleCondiciones = 1;
				
				}
			}
		}
	}
	
	if(flagCumpleCondiciones == 0){//si no ha cumplido las tres condiciones (no definió color) comienza a evaluar
		if(r <= valorMasToleranciaAZUL_r && r >= valorMenosToleranciaAZUL_r){
			Serial.println("AZUL - 1. Cumple");
			if(g <= valorMasToleranciaAZUL_g && g >= valorMenosToleranciaAZUL_g){
				Serial.println("AZUL - 2. Cumple");
				if(b <= valorMasToleranciaAZUL_b && b >= valorMenosToleranciaAZUL_b){

					Serial.println("AZUL - 3. Cumple");

					Serial.println("AZUL");
					SerialBT.println("AZUL");
					colorDetectado = "AZUL";
					flagCumpleCondiciones = 1;	
				
				}
			}	
		}
	}
	
	if(flagCumpleCondiciones == 0){//si luego de las evaluaciones no cumplió las condiciones: color indefinido
		Serial.println("No identificado");
		colorDetectado = "INDEFINIDO";
	}
}


void setearColorNeopixel(int r, int g, int b){


	for(int i = 0; i < NUMPIXELS; i++){

		pixels.setPixelColor(i, r, g, b);

	}

	pixels.show();
	
}