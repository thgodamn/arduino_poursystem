#include <Time.h>
#include <Wire.h>
#include <DS1302RTC.h>
#include <Ardprintf.h>

DS1302RTC RTC( 27, 29,  31 ); // пины в скобках (CE, IO, CLK)

int pinMatrix[100][100];  // пины матрицы
int n = 20;               // кол-во зон
int pinZone[20];          // пины зон
//пины датчиков
int rainPin = 11;
int groundPin = 12;

//input (for listener and parser) входящее сообщение
int input[40];
int min_read = 40;
int curset = 0;   // указывает на раздел настроек
int last_in = 0;  // длина сообщения

//время полива зон, начала полива зон (в часах и минутах)
int run_zone[20];
int start_zone_minutes[4][20];
int start_zone_hours[4][20];

//когда был дождь часы минуты
int rain_hour = 0;
int rain_minute = 0;
int cd_rain = 1;            // период ожидания (в часах)
int ground_humidity = 85;   // процент поддерживаемой влажости почвы

bool weekdays[7];   // дни полива
//зоны и датчики
bool zone_rain[20];
bool zone_ground[20];

bool system_mode = false;   // система работает или нет
bool everyday = true;       // поливать каждый день
bool rain = false;          // датчик дождя
bool ground = false;        // датчик почвы
bool pour = false;          // поливать или нет


//функционал
int ardprintf(const char *, ...);

void debug () {
  ardprintf("kolichestvo zon: %d", n);
  ardprintf("rain_sensor pin: %d; ground_sensor pint: %d;", rainPin, groundPin);

  ardprintf("run_zone[*%d] = %n", n);
  for (int i = 0; i < n; i++)
    ardprintf("(%d)%d %n", i, run_zone[i]);
  
  ardprintf("");
  for (int i = 0; i < 4; i++) {
    ardprintf("start_zone_hours[%d][*%d] = %n", i, n);
    for (int j = 0; j < n; j++)
      ardprintf("(%d)%d,%n", j, start_zone_hours[i][j]);

    ardprintf("   %n");
    ardprintf("start_zone_minutes[%d][*%d] = %n", i, n);
    for (int j = 0; j < n; j++)
      ardprintf("(%d)%d,%n", j, start_zone_minutes[i][j]);

    ardprintf("");
  }

  
  ardprintf("weekdays[*%d]: %n", 7);
  ardprintf("Monday %d, Tuesday %d, Wednesday %d, Thursday %d, Friday %d, Saturday %d, Sunday %d", weekdays[0],
                      weekdays[1],
                      weekdays[2],
                      weekdays[3],
                      weekdays[4],
                      weekdays[5],
                      weekdays[6]);

  ardprintf("zone_rain[*%d] = %n", n);
  for (int i = 0; i < n; i++)
    ardprintf("(%d)%d %n", i, zone_rain[i]);
  ardprintf("");
  ardprintf("zone_ground[*%d] = %n", n);
  for (int i = 0; i < n; i++)
    ardprintf("(%d)%d %n", i, zone_ground[i]);
  ardprintf("");
  
  //конечная
  ardprintf("");
}

void input_clear() {
  for (int i = 0; i < sizeof(input); i++)
    input[i] = 0;
}

int input_num (int start, int end_n) {
  int num = 0;
  for (int i = start; i <= end_n; i++) {
    num *= 10;
    num += input[i];
  }
  return num;
}

/*
  -1 system_off
  0 system_on
  1 time
  2 zone
  3 day
  4 sensor
  _1 main setup, sensor rain
  _2 runtime, sensor ground
  ..
*/

void parser() {
  //debug command
  ardprintf("input[*40] = %n");
  for (int i = 0; i < min_read; i++)
    ardprintf("(%d)%d %n", i, input[i]);
  ardprintf("");
  
  for (int i = 0; i < min_read; i++)
  curset = input[0] * 100 + input[1] * 10 + input[2];
  int first = 3;
  switch (curset) {
    case -100:
      system_mode = false;
      break;
    case 0:
      system_mode = true;
      break;
    case 100:
      Serial.println(now());
      break;
    case 110: { //set_time_00h00m00s00d00m0000y;  110|01020304050006
        setTime(input_num(3, 4), //00h (3-4)
                input_num(5, 6), //00m (5-6)
                input_num(7, 8), //00s (7-8)
                input_num(9, 10), //00d (9-10)
                input_num(11, 12), //00m (11-12)
                input_num(13, 16)); //0000y (13-16)
        RTC.set(now());
        break;
      }
    //case 200:
    //  Serial.println("create_zone_!!!");    //зоны у нас и так созданы
    //  break;
    case 210: {   //set_zone_01z3p19h25m (zone)(period)(hour)(minute);  210|01031925
        int set_zone = input_num(3, 4); //01 (3-4)
        int set_period = input_num(5, 6);             //03 (5-6)
        int set_hour = input_num(7, 8); //19(7-8)
        int set_minute = input_num(9, 10); //25(9-10)
        start_zone_minutes[set_period][set_zone] = set_minute;
        start_zone_hours[set_period][set_zone] = set_hour;
        break;
      }
    case 220:  //set_runtime_05z15m;  220|0515
      run_zone[input_num(3, 4)] = input_num(5, 6);
      break;
    case 310: {  //set_day_1010110(или everyday)  310|1010110 / 310|e
        if (input[first] == 'e')   // 'e' - '0' = 101 - 48 = 53
          for (int i = 0; i < 7; i++)
            weekdays[i] = true;
        else
          for (int i = 0; i < first + 7; i++)
            weekdays[i] = input[i + first];
      }
    case 410: {  //zone_rain_1(0)0212   410|1(true/false)0212
        if (input[first] == 1)
          for (int i = first + 1; i < last_in; i += 2)
            zone_rain[input_num(i, i + 1)] = true;
        else
          for (int i = first + 1; i < last_in; i += 2)
            zone_rain[input_num(i, i + 1)] = false;
      }
    case 420: {  //zone_ground   420|1(true/false)0212
        if (input[first] == 1)
          for (int i = first + 1; i < last_in; i += 2)
            zone_ground[input_num(i, i + 1)] = true;
        else
          for (int i = first + 1; i < last_in; i += 2)
            zone_ground[input_num(i, i + 1)] = false;
      }
    case 421:    //ground_humidity_80
      ground_humidity = input_num(first, first + 1);
      break;
    case 999:
      debug();
      break;
  }

  //отчистка input
  input_clear();
  curset = 0;
  last_in = 0;
}

/*void buttons () {
  bool plus,minus,next,last;
  if (plus) {
    
  }
  if (minus) {
    
  }
  if (next) {
    
  }
  if (last) {
    
  }
}*/

void listener () {
  while (Serial.available() > 0) {
    int incomingByte = Serial.read();
    if (incomingByte == '#')
      parser();  
    else if (incomingByte >= 48 && incomingByte <= 57) {
      input[last_in] = incomingByte - '0';
      last_in++;
    } else {
      input[last_in] = incomingByte;
      last_in++;
    }
  }
}


void sensor_read () {
  //считываем данные с датчиков
  int rain_sensor = analogRead(rainPin);
  int ground_sensor = analogRead(groundPin);

  //rain
  if (rain_sensor == 1 && !rain) {
    rain = true;
    rain_hour = hour();
    rain_minute = minute();
  }
  if (hour() * 60 + minute() > (rain_hour + cd_rain) * 60 + rain_minute)
    rain = false;

  //ground
  if (ground_sensor == 1) {
    ground = true;
  }
  else {
    ground = false;
  };
}

// функция полива и проверки данных с датчиков
void sensor_pour () {
  if (!rain || !ground) {
    //weekday(), воскресенье 1 день
    if (weekdays[weekday()]) {
      for (int i = 0; i < 4; i++) // кол-во старт-таймов на зону
        for (int j = 0; j < n; j++)
          if (start_zone_hours[i][j] * 60 + start_zone_minutes[i][j] <= hour() * 60 + minute() &&
              start_zone_hours[i][j] * 60 + start_zone_minutes[i][j] + run_zone[j] < 1439 &&
              start_zone_hours[i][j] * 60 + start_zone_minutes[i][j] + run_zone[j] >= hour() * 60 + minute())
            digitalWrite(pinZone[j], HIGH);

      // в случае если время достигает ночи (00:00), а полив должен продолжаться
          else if (start_zone_hours[i][j] * 60 + start_zone_minutes[i][j] + run_zone[j] >= 1439 &&
                  (start_zone_hours[i][j] * 60 + start_zone_minutes[i][j] + run_zone[j] - 1440 >= hour() * 60 + minute() ||
                   start_zone_hours[i][j] * 60 + start_zone_minutes[i][j] <= hour() * 60 + minute()))
            digitalWrite(pinZone[j], HIGH);
          else digitalWrite(pinZone[j], LOW);

    }
  }
}

void setup () {
  Serial.begin(9600);
  
  setSyncProvider(RTC.get);//взятие времени ??
  for (int i = 0; i < n; i++) {
    pinMode(pinZone[i], OUTPUT);
    run_zone[i] = 10; // продолжительность полива 10 мин
    
    start_zone_hours[0][i] = 5; // 5:30 - утро
    start_zone_minutes[0][i] = 30; // 5:30 - утро
    
    start_zone_hours[1][i] = 19; // 19:00 - вечер
    start_zone_minutes[1][i] = 30; // 5:30 - утро
  }

  for (int i = 0; i < 7; i++)
    weekdays[i] = 1;

  pinMode(rainPin, INPUT);
  pinMode(groundPin, INPUT);
  //..
}

void loop () {

  //..code..
  listener(); //слушатель (сбор всей команды) + parser();
  sensor_read(); //читает данные с датчиков
  sensor_pour(); // проверяем параметры датчика
  
}
