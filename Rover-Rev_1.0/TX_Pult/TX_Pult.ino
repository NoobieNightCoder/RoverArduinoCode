// Переменная с номером канала связи (номер канала - последние 2 символа)
#define Chastota 0x70

#include <SPI.h>          // библиотека для работы с шиной SPI
#include "nRF24L01.h"     // библиотека радиомодуля
#include "RF24.h"         // ещё библиотека радиомодуля

// Пины осей джойстика
#define VLx A0
#define VLy A1

// Пины кнопок
#define Button1 2
#define Button2 3

RF24 radio(10, 9); // "создать" модуль на пинах 9 и 10 Для Уно
//RF24 radio(9,53); // для Меги

byte address[][6] = {"1Node", "2Node", "3Node", "4Node", "5Node", "6Node"}; //возможные номера труб

int Array[4];

void setup() {
  Serial.begin(9600);         // открываем порт для связи с ПК

  pinMode(VLx, INPUT);
  pinMode(VLy, INPUT);

  pinMode(Button1, INPUT);
  pinMode(Button2, INPUT);

  radio.begin();              // активировать модуль
  radio.setAutoAck(1);        // режим подтверждения приёма, 1 вкл 0 выкл
  radio.setRetries(0, 15);    // (время между попыткой достучаться, число попыток)
  radio.enableAckPayload();   // разрешить отсылку данных в ответ на входящий сигнал
  radio.setPayloadSize(32);   // размер пакета, в байтах

  radio.openWritingPipe(address[0]);      // мы - труба 0, открываем канал для передачи данных
  radio.setChannel(Chastota);             // выбираем канал (в котором нет шумов!)

  radio.setPALevel (RF24_PA_MAX);   // уровень мощности передатчика. На выбор RF24_PA_MIN, RF24_PA_LOW, RF24_PA_HIGH, RF24_PA_MAX
  radio.setDataRate (RF24_250KBPS); // скорость обмена. На выбор RF24_2MBPS, RF24_1MBPS, RF24_250KBPS
  //должна быть одинакова на приёмнике и передатчике!
  //при самой низкой скорости имеем самую высокую чувствительность и дальность!!

  radio.powerUp();        // начать работу
  radio.stopListening();  // не слушаем радиоэфир, мы передатчик
}

void loop() {
  int Lx, Ly;               // Значения джойстика
  byte ser1, ser2;          // Значения кнопок
  
  // Получение значений джойстика
  Lx = map(analogRead(VLx), 0, 1023, -255, 255);
  Ly = map(analogRead(VLy), 0, 1023, -255, 255);

  // Получение значений кнопок
  ser1 = digitalRead(Button1);
  ser2 = digitalRead(Button2);

  // Запись всех значений в массив для последующей отправки
  Array[0] = Lx;
  Array[1] = Ly;
  Array[2] = ser1;
  Array[3] = ser2;
  
  //Serial.println("Sent: " + String(Array[0]) + " " + String(Array[1]) + " " + String(Array[2]) + " " + String(Array[3]));
  radio.write(&Array, sizeof(Array));  // Отправка массива данных
}
