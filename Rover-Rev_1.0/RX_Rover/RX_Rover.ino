#include <Servo.h>
#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"


// Пины моторов (in1 (Вперед) и in2 (Назад) - одна сторона, in3 (Вперед) и in4 (Назад) - вторая сторона)
#define in1 3
#define in2 4
#define in3 5
#define in4 6

unsigned long timer1, timer2;     // Таймеры для расчитывания тайм-аута сигнала

// Инициализация сервоприводов
Servo servo1;
Servo servo2;

RF24 radio(10, 9);  // "создать" модуль на пинах 9 и 10 Для Уно
//RF24 radio(9,53); // для Меги

byte address[][6] = {"1Node", "2Node", "3Node", "4Node", "5Node", "6Node"}; //возможные номера труб

byte prevByte1 = 0, prevByte2 = 0;
int degree1 = 0, degree2 = 0;

void setup() {
  Serial.begin(9600);         // открываем порт для связи с ПК

  // Привязка сервоприводов к пинам
  servo1.attach(7);
  servo2.attach(8);

  pinMode(in1, OUTPUT);
  pinMode(in2, OUTPUT);
  pinMode(in3, OUTPUT);
  pinMode(in4, OUTPUT);

  radio.begin();              // активировать модуль
  radio.setAutoAck(1);        // режим подтверждения приёма, 1 вкл 0 выкл
  radio.setRetries(0, 15);    // (время между попыткой достучаться, число попыток)
  radio.enableAckPayload();   // разрешить отсылку данных в ответ на входящий сигнал
  radio.setPayloadSize(32);   // размер пакета, в байтах

  radio.openReadingPipe(1, address[0]);   // хотим слушать трубу 0
  radio.setChannel(0x70);     // выбираем канал (в котором нет шумов!)

  radio.setPALevel (RF24_PA_MAX);   // уровень мощности передатчика. На выбор RF24_PA_MIN, RF24_PA_LOW, RF24_PA_HIGH, RF24_PA_MAX
  radio.setDataRate (RF24_250KBPS); // скорость обмена. На выбор RF24_2MBPS, RF24_1MBPS, RF24_250KBPS
  //должна быть одинакова на приёмнике и передатчике!
  //при самой низкой скорости имеем самую высокую чувствительность и дальность!!

  radio.powerUp();        // начать работу
  radio.startListening(); // начинаем слушать эфир, мы приёмный модуль
}

void loop() {
  timer2 = millis();      // Запуск таймера
  byte pipeNo;            // Переменная для хранения получаемых данных
  int Array[4];                             // Массив для хранения всех получаемых данных
  while (radio.available(&pipeNo)) {        // слушаем эфир со всех труб
    timer1 = timer2;                    // Обнуление таймера
    radio.read(&Array, sizeof(Array));  // чиатем входящий сигнал
    //Serial.println("Recieved: " + String(Array[0]) + " " + String(Array[1]) + " " + String(Array[2]) + " " + String(Array[3]));
  }

  if(timer2 - timer1 > 500) {       // Проверка на долгое отсутствие сигнала
    // Обнуление значений для остановки робота в случае потери сигнала
    Array[0] = 0;
    Array[1] = 0;
    Array[2] = 0;
    Array[3] = 0;
  }

  if (Array[0] < 20 && Array[0] > -20 && Array[1] < 20 && Array[1] > -20) {  // Стоп
    digitalWrite(in1, LOW);
    digitalWrite(in2, LOW);
    digitalWrite(in3, LOW);
    digitalWrite(in4, LOW);
  } else {
    //        РОВЕР
    int x = Array[0], y = Array[1];                 // Присваивание значений к своим переменным ([0] - ось X, [1] - ось Y)
    int leftForward = 0, rightForward = 0, leftBackward = 0, rightBackward = 0;   // Переменные для управления моторами

    /*
                                              Для понимания, как работает алгоритм:
        Ось X - Назад [-255; 0) / Вперед (0; 255]
        Ось Y - Влево [-255; 0) / Вправо (0; 255]
        
        Для того, чтобы правильно управлять скоростью левой и правой стороны, задаётся следующее правило:
                                                        Вперед (X | X)
                                                             |
                                                             |
                                                             |
                                Влево (|X| + Y | |X| - Y)----+---- Вправо (|X| + Y | |X| - Y)
                                                             |
                                                             |
                                                             |
                                                        Назад (|X| | |X|)

        Например:
        При повороте налево, джойстик выдаст такие значения: (255, -255)
        Получается, что скорость левой стороны = 255 + (-255) = 255 - 255 = 0
        А скорость правой стороны = 255 - (-255) = 255 + 255 = 255 (не 510, потому что 255 - максимальное значение)
        Левая сторона = 0, а правая сторона = 255, из-за чего крутятся только правые моторы и совершается поворот направо

        И так же работает поворот направо.

        При X < 0 (то есть при движении назад), алгоритм продолжает работать правильно, только необходимо использовать модуль от X ( |X| ),
        Чтобы получить положительные значения.

    */
    if (x < 0) {        
      leftBackward = abs(x) + y;
      rightBackward = abs(x) - y;
    } else {
      leftForward = x - y;
      rightForward = x + y;
    }

    // Проверка на отрицательность
    if(leftForward < 0) leftForward = 0;
    if(rightForward < 0) rightForward = 0;
    if(leftBackward < 0) leftBackward = 0;
    if(rightBackward < 0) rightBackward = 0;

    // Проверка на превышение максимального значения скорости
    if(leftForward > 255) leftForward = 255;
    if(rightForward > 255) rightForward = 255;
    if(leftBackward > 255) leftBackward = 255;
    if(rightBackward > 255) rightBackward = 255;

    //        КНОПКИ
    if (Array[2] == 1) {
      if (prevByte1 == 0) {
        prevByte1 = 1;
        degree1 += 180;
        if (degree1 >= 360) {
          degree1 = 0;
        }
        servo1.write(degree1);
      }
    } else if (Array[2] == 0 and prevByte1 == 1) {
      prevByte1 = 0;
    }
    if (Array[3] == 1) {
      if (prevByte2 == 0) {
        prevByte2 = 1;
        degree2 += 180;
        if (degree2 >= 360) {
          degree2 = 0;
        }
        servo2.write(degree2);
      }
    } else if (Array[3] == 0 and prevByte2 == 1) {
      prevByte2 = 0;
    }

    //Serial.println(String(leftForward) + "\t" + String(leftBackward) + "\t" + String(rightForward) + "\t" + String(rightBackward));


    // Присваивание скорости моторам
    analogWrite(in1, leftForward);
    analogWrite(in2, leftBackward);
    analogWrite(in3, rightForward);
    analogWrite(in4, rightBackward);
  }
}
