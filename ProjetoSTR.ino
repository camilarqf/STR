#include <NilRTOS.h>
#include <Servo.h>
#include <NilSerial.h>
#define Serial NilSerial

Servo servo;
int sensorChuva = A0; // sensor de chuva na porta analógica A0
int vazio = 13; //led indicativo de reservatório vazio
int cheio = 12; //led indicativo de reservatório cheio
int ledChuva = 11; //led indicativo de presença de chuva
int ledPeriodoChuva = 5;// led indicativo da detecção de chuva a cada minuto
int valSensorChuva; //ariável parar armazenar o valor lido do sensor de chuva
int valSensorNivel; // variável para armazenar o valor lido pelo sensor de nível
int sensorNivel = 7; // sensor de nível na porta digital 7

//unsigned long tempoChuva = 43200000; // variável para  verificar se choveu nas últimas 12h
unsigned long tempoChuva = 60000; // tempo de chuva a cada 1 minuto
unsigned long chuvaMillis = 0; // variável que armazena o tempo em que a última chuva foi detectada 
unsigned long atualMillis = 0; //variável que armazena tempo atual
boolean b_chuva;
boolean a_servo = false; // controla a abertura e fechamento da tampa (ativa e desativa o servo)
int espera; // armazena o tempo necessário para o delay de abertura da tampa

SEMAPHORE_DECL(sem, 0); // declara semáforo

NIL_WORKING_AREA(waChuva, 64); //declarar pilha de 64 bytes
NIL_THREAD(Chuva, arg) { //inicia a thread que verifica a presença de chuva pelo sensor de chuva
  while (TRUE) {
  
    unsigned long t = millis(); //pega o tempo atual
    valSensorChuva = analogRead(sensorChuva); //lê o resultado do sensor de chuva na porta A0

    
    if (valSensorChuva > 900 && valSensorChuva < 1024) { //chuva fraca, a tampa do reservatório se mantem fechada

      a_servo = false; //servo desativado
      digitalWrite(ledChuva, LOW); //led de chuva apagado
      Serial.print("Sensor de Chuva:");
      Serial.println(valSensorChuva);
      Serial.println("Tampa Fechada!");
      Serial.println("");

    }

    if (valSensorChuva > 0 && valSensorChuva < 900) { //chuva com intensidade média

      chuvaMillis = millis(); //registra o tempo atual da chuva
      a_servo = true; //ativa servo
      
      digitalWrite(ledChuva, HIGH); //le de chuva acende 
      Serial.print("Sensor de Chuva:");
      Serial.println(valSensorChuva);
      Serial.println("Tampa Aberta!");
      Serial.println("");

    }
  /* dorme para que threads de prioridade mais baixa possam ser executadas
   *  aguarda pelo tempo restante do período da tarefa, o tempo é determinado pelo período de 1 segundo subtraído pelo tempo de execução da tarefa  
   */
    nilThdSleepMilliseconds(1000 - (millis() - t)); 
  }
}

NIL_WORKING_AREA(waNivel, 64); //declarar pilha de 64 bytes
NIL_THREAD(Nivel, arg) { //inicializa thread que verifica o nível do reservatório
  while (TRUE) {
    
    unsigned long t = millis();  //pega o tempo atual
    valSensorNivel = digitalRead(sensorNivel); //lê o resultado do sensor de nível na porta 7

    switch (valSensorNivel) {

      case 0: 
       a_servo = false; //desativa servo
        digitalWrite(vazio, LOW); //led vazio apagado
        digitalWrite(cheio, HIGH); //led cheio acende
        Serial.print("Reservatório já está cheio!");
        Serial.println("Tampa Fechada!");
        Serial.println("");
        break;

      case 1:
        digitalWrite(vazio, HIGH); //led vazio acende
        digitalWrite(cheio, LOW); //led cheio apagado
        Serial.print("Reservatório abaixo da capacidade!");
        Serial.println("");
        break;
    }
   nilSemSignal(&sem); //retorna o sinal para a thread1 se o reservatório está cheio ou vazio
   //led que verifica periodicidade da chuva desligado
    nilThdSleepMilliseconds(1000 - (millis() - t));
  }
}

NIL_WORKING_AREA(waServoMotor, 64); //declarar pilha de 64 bytes
NIL_THREAD(ServoMotor, arg) { //inicializa thread que controla o servo motor
  while (TRUE) {

    unsigned long ti = millis(); //pega o tempo atual

    if (a_servo == true) { //ativa o servo

      nilThdSleepMilliseconds(espera); //aguarda pelo tempo armazenado na variável espera 
      servo.write(180); //servo gira 180 graus
    }

   if (a_servo == false) { //desativa servo
      servo.write(0); //servo gira para posição de origem
    }
    if ( millis()- ti < 30000) { 
      nilThdSleepMilliseconds(1000); // dorme para que threads de prioridade mais baixa possam ser executadas
    }
    else{ // se o deadline falhar 
      NIL_MSG_TMO;
      }
   }
}


NIL_WORKING_AREA(waPChuva, 64); //declarar pilha de 64 bytes
NIL_THREAD(PChuva, arg) { //inicializa thread que verifica se choveu a cada 1 minuto
  while (TRUE) {

    unsigned long t = millis(); //pega o tempo atual
    atualMillis = millis(); //pega o tempo atual

    if (atualMillis - chuvaMillis > tempoChuva) { //se o tempo atual - o tempo da ultima chuva for maior que 1 minuto o tempo de espera para abertura da tampa será maior 
      /*tempo para o descarte das primeiras águas da chuva para eliminar as impurezas
       
       */
      espera = 10000; //tempo de espera de 10 segundos 
      Serial.print("Mais de 1 minuto sem chover"); 
      digitalWrite(ledPeriodoChuva, LOW); //led que verifica periodicidade da chuva desligado
      Serial.println("");
    }
    if (atualMillis - chuvaMillis < tempoChuva) { //se o tempo atual - o tempo da ultima chuva for menor que 1 minuto o tempo de espera para abertura da tampa será menor

      espera = 1000; //tempo de espera de 1 segundos
      Serial.print("Choveu há menos de 1 minuto");
      digitalWrite(ledPeriodoChuva, HIGH); //led que verifica periodicidade da chuva ligado
      Serial.println("");
    }
  /* dorme para que threads de prioridade mais baixa possam ser executadas
   *  aguarda pelo tempo restante do período da tarefa, o tempo é determinado pelo período de 60 segundos subtraído pelo tempo de execução da tarefa  
   */
    nilThdSleepMilliseconds(60000 - (millis() - t));
  }

}
//tabela de prioridade das threads
NIL_THREADS_TABLE_BEGIN()
NIL_THREADS_TABLE_ENTRY(NULL, Chuva, NULL, waChuva, sizeof(waChuva))
NIL_THREADS_TABLE_ENTRY(NULL, Nivel, NULL, waNivel, sizeof(waNivel))
NIL_THREADS_TABLE_ENTRY(NULL, ServoMotor, NULL, waServoMotor, sizeof(waServoMotor))
NIL_THREADS_TABLE_ENTRY(NULL, PChuva, NULL, waPChuva, sizeof(waPChuva))
NIL_THREADS_TABLE_END()



void setup() {

  servo.attach(9); //configurar servo na porta digital 9
  Serial.begin(9600); 
  servo.write(0); //inicializar o servo desativado
  //configuração dos leds
  pinMode(cheio, OUTPUT); 
  pinMode(vazio, OUTPUT);
  pinMode(ledChuva, OUTPUT);
  pinMode(ledPeriodoChuva, OUTPUT);
  pinMode(sensorNivel, HIGH);
  nilSysBegin();

}

void loop() {
  // put your main code here, to run repeatedly:

}
