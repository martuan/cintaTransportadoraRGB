/*

El programa se encarga de detectar un objeto, medir su componente RGB para luego clasificarlo por color.
Utiliza driver para motor de CC, servomotor para clasificación, sensor RGB y LED de alta potencia.
Programado en ESP32.

Autor: Martín Cioffi
Fecha: 23-11-2023
Versión: 1.00
Repositorio: 




*/



#include <BluetoothSerial.h>
#include "Adafruit_APDS9960.h"
#include <ESP32Servo.h>
#include "ColoresPatrones.h"


#define VELOCIDAD_INICIAL 150//200
#define VELOCIDAD_FINAL 250//255

//**************************************
//*********** BLUETOOTH  *****************
//**************************************

BluetoothSerial SerialBT;

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

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

	Serial.println(proximidad);
	if(proximidad >= PROXIMIDAD_UMBRAL){
		Serial.println("*******Objeto detectado*********");
		Serial.println("Stop");
		
		
		//frenar motor y leer RGB	
		digitalWrite(motor1Pin1, HIGH);
		digitalWrite(motor1Pin2, HIGH);

		ledcWrite(pwmChannel, 255);

		detectarColor();

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
	/*
	case 'D':
		
		if(valorParam.toInt() >= 10 && valorParam.toInt() <= 200){
			demoraDelay = valorParam.toInt();
		}else{
			Serial.println("Valor inaceptable");
		}
		
		

	break;
*/

	case 'C'://autocalibracion

		color = valorParam.toInt();
		autocalibracion(color);

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
  int tolerencia = 10;

  //wait for color data to be ready
  while(!apds.colorDataReady()){
    delay(5);
  }

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

	if(r >= ROJO_PATRON.r + (ROJO_PATRON.r * (tolerencia/100)) || r <= ROJO_PATRON.r - (ROJO_PATRON.r * (tolerencia/100)))
		if(g >= ROJO_PATRON.g + (ROJO_PATRON.g * (tolerencia/100)) || g <= ROJO_PATRON.g - (ROJO_PATRON.g * (tolerencia/100)))
			if(b >= ROJO_PATRON.b + (ROJO_PATRON.b * (tolerencia/100)) || b <= ROJO_PATRON.b - (ROJO_PATRON.b * (tolerencia/100))){

		Serial.println("ROJO");
		SerialBT.println("ROJO");
		colorDetectado = "ROJO";

	}else if(r >= VERDE_PATRON.r + (VERDE_PATRON.r * (tolerencia/100)) || r <= VERDE_PATRON.r - (VERDE_PATRON.r * (tolerencia/100)))
			if(g >= VERDE_PATRON.g + (VERDE_PATRON.g * (tolerencia/100)) || g <= VERDE_PATRON.g - (VERDE_PATRON.g * (tolerencia/100)))
				if(b >= VERDE_PATRON.b + (VERDE_PATRON.b * (tolerencia/100)) || b <= VERDE_PATRON.b - (VERDE_PATRON.b * (tolerencia/100))){

		Serial.println("VERDE");
		SerialBT.println("VERDE");
		colorDetectado = "VERDE";

	}else if(r >= AZUL_PATRON.r + (AZUL_PATRON.r * (tolerencia/100)) || r <= AZUL_PATRON.r - (AZUL_PATRON.r * (tolerencia/100)))
			if(g >= AZUL_PATRON.g + (AZUL_PATRON.g * (tolerencia/100)) || g <= AZUL_PATRON.g - (AZUL_PATRON.g * (tolerencia/100)))
				if(b >= AZUL_PATRON.b + (AZUL_PATRON.b * (tolerencia/100)) || b <= AZUL_PATRON.b - (AZUL_PATRON.b * (tolerencia/100))){

		Serial.println("AZUL");
		SerialBT.println("AZUL");
		colorDetectado = "AZUL";

	}else{
		Serial.println("No identificado");
		colorDetectado = "INDEFINIDO";
	}
}

void autocalibracion(int colorCalibrado){

	//colorCalibrado
	//1 = ROJO
	//2 = VERDE
	//3 = AZUL

	if(colorCalibrado == 1){
		Serial.println("Calibrando ROJO - Ingrese solo una muestra");
	}else if(colorCalibrado == 2){
		Serial.println("Calibrando VERDE - Ingrese solo una muestra");
	}else if(colorCalibrado == 3){
		Serial.println("Calibrando AZUL - Ingrese solo una muestra");
	}
	//create some variables to store the color data in
	uint16_t r, g, b, c;

	//wait for color data to be ready
	while(!apds.colorDataReady()){
		delay(5);
	}

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

	//asignación de componentes RGB según la muestra

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

}