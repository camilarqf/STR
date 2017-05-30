#include <NilRTOS.h>
#include <Servo.h>
#include <NilSerial.h>
#define Serial NilSerial

Servo servo;
int sensorChuva = A0; // sensor de chuva na porta analógica A0
int vazio = 13; //led indicativo de reservatório vazio
int cheio = 12; //led indicativo de reservatório cheio
int valSensorChuva; //ariável parar armazenar o valor lido do sensor de chuva
int valSensorNivel; // variável para armazenar o valor lido pelo sensor de nível
int sensorNivel = 7; // sensor de nível na porta digital 7
//unsigned long tempoChuva = 43200000; // variável para  verificar se choveu nas últimas 12h
unsigned long tempoChuva = 60000;
unsigned long chuvaMillis = 0; //
unsigned long atualMillis = 0; //tempo atual
boolean b_chuva;
boolean a_servo = false;
int espera;

SEMAPHORE_DECL(sem, 0); // declara semáforo

NIL_WORKING_AREA(waChuva, 64); //declarar pilha de 64 bytes
NIL_THREAD(Chuva, arg) {
  while (TRUE) {

    unsigned long t = millis();
    valSensorChuva = analogRead(sensorChuva);


    if (valSensorChuva > 900 && valSensorChuva < 1024) { //chuva fraca, a tampa do reservatório se mantem fechada

      a_servo = false;
      Serial.print("Sensor de Chuva:");
      Serial.println(valSensorChuva);
      Serial.println("Tampa Fechada!");
      Serial.println("");

    }

    

    if (valSensorChuva > 0 && valSensorChuva < 900) { //chuva intensidade média

      chuvaMillis = millis();
      a_servo = true;

      Serial.print("Sensor de Chuva:");
      Serial.println(valSensorChuva);
      Serial.println("Tampa Aberta!");
      Serial.println("");

    }

    nilThdSleepMilliseconds(1000 - (millis() - t));
  }
}

NIL_WORKING_AREA(waNivel, 64); //declarar pilha de 64 bytes
NIL_THREAD(Nivel, arg) {
  while (TRUE) {
    unsigned long t = millis();
    valSensorNivel = digitalRead(sensorNivel);

    switch (valSensorNivel) {

      case 0:
       a_servo = false;
        digitalWrite(vazio, LOW);
        digitalWrite(cheio, HIGH);   
        Serial.print("Reservatório já está cheio!");
        Serial.println("Tampa Fechada!");
        Serial.println("");
        break;

      case 1:
        digitalWrite(vazio, HIGH);
        digitalWrite(cheio, LOW);
        Serial.print("Reservatório abaixo da capacidade!");
        Serial.println("");
        break;
    }
   nilSemSignal(&sem); //retorna o sinal para a thread1 se o reservatório está cheio ou vazio
    nilThdSleepMilliseconds(1000 - (millis() - t));
  }
}

NIL_WORKING_AREA(waServoMotor, 64); //declarar pilha de 64 bytes
NIL_THREAD(ServoMotor, arg) {
  while (TRUE) {

    unsigned long ti = millis();

    if (a_servo == true) {

      nilThdSleepMilliseconds(espera);
      servo.write(180);
    }

   if (a_servo == false) {
      servo.write(0);
    }
    if ( ti - millis() < 30000) {
      nilThdSleepMilliseconds(1000);
    }
    else{
      NIL_MSG_TMO;
      }
   }
}


NIL_WORKING_AREA(waPChuva, 64); //declarar pilha de 64 bytes
NIL_THREAD(PChuva, arg) {
  while (TRUE) {

    unsigned long t = millis();
    atualMillis = millis();

    if (atualMillis - chuvaMillis > tempoChuva) {

      espera = 10000;
      Serial.print("Mais de 1 minuto sem chover");
      Serial.println("");
    }
    if (atualMillis - chuvaMillis < tempoChuva) {

      espera = 1000;
      Serial.print("Choveu há menos de 1 minuto");

      Serial.println("");
    }

    nilThdSleepMilliseconds(60000 - (millis() - t));
  }

}

NIL_THREADS_TABLE_BEGIN()
NIL_THREADS_TABLE_ENTRY(NULL, Chuva, NULL, waChuva, sizeof(waChuva))
NIL_THREADS_TABLE_ENTRY(NULL, Nivel, NULL, waNivel, sizeof(waNivel))
NIL_THREADS_TABLE_ENTRY(NULL, ServoMotor, NULL, waServoMotor, sizeof(waServoMotor))
NIL_THREADS_TABLE_ENTRY(NULL, PChuva, NULL, waPChuva, sizeof(waPChuva))
NIL_THREADS_TABLE_END()



void setup() {

  servo.attach(9);
  Serial.begin(9600);
  servo.write(0);
  pinMode(cheio, OUTPUT);
  pinMode(vazio, OUTPUT);
  pinMode(sensorNivel, HIGH);
  nilSysBegin();

}

void loop() {
  // put your main code here, to run repeatedly:

}
