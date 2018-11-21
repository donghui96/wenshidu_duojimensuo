double Fahrenheit(double celsius)
{
  return 1.8 * celsius + 32;
}    //摄氏温度度转化为华氏温度

double Kelvin(double celsius)
{
  return celsius + 273.15;
}     //摄氏温度转化为开氏温度

// 露点（点在此温度时，空气饱和并产生露珠）
// 参考: [url=http://wahiduddin.net/calc/density_algorithms.htm]http://wahiduddin.net/calc/density_algorithms.htm[/url]
double dewPoint(double celsius, double humidity)
{
  double A0 = 373.15 / (273.15 + celsius);
  double SUM = -7.90298 * (A0 - 1);
  SUM += 5.02808 * log10(A0);
  SUM += -1.3816e-7 * (pow(10, (11.344 * (1 - 1 / A0))) - 1) ;
  SUM += 8.1328e-3 * (pow(10, (-3.49149 * (A0 - 1))) - 1) ;
  SUM += log10(1013.246);
  double VP = pow(10, SUM - 3) * humidity;
  double T = log(VP / 0.61078); // temp var
  return (241.88 * T) / (17.558 - T);
}

// 快速计算露点，速度是5倍dewPoint()
// 参考: [url=http://en.wikipedia.org/wiki/Dew_point]http://en.wikipedia.org/wiki/Dew_point[/url]
double dewPointFast(double celsius, double humidity)
{
  double a = 17.271;
  double b = 237.7;
  double temp = (a * celsius) / (b + celsius) + log(humidity / 100);
  double Td = (b * temp) / (a - temp);
  return Td;
}


#include <Gizwits.h>
#include <Wire.h>
#include <SoftwareSerial.h>
#include <dht11.h>
#include <Servo.h>
#include <IRremote.h>
#include <SPI.h>
Servo myservo;
int PIR_sensor = A5;    //指定PIR模拟端口 A5
int LED = 13;           //指定LED端口 13
int val = 0;            //存储获取到的PIR数值

int d = 3;
int PIN_SENSOR = 5;
int RECV_PIN = 4;//红外接收输入
decode_results results;
IRrecv irrecv(RECV_PIN);
extern uint8_t SmallFont[];
dht11 DHT11;

#define DHT11PIN 2
SoftwareSerial mySerial(A2, A3); // A2 -> RX, A3 -> TX

Gizwits myGizwits;

#define   KEY1              6
#define   KEY2              7
#define   KEY1_SHORT_PRESS  1
#define   KEY1_LONG_PRESS   2
#define   KEY2_SHORT_PRESS  4
#define   KEY2_LONG_PRESS   8
#define   NO_KEY            0
#define   KEY_LONG_TIMER    3
unsigned long Last_KeyTime = 0;

unsigned long gokit_time_s(void)
{
  return millis() / 1000;
}

char gokit_key1down(void)
{
  unsigned long keep_time = 0;
  if (digitalRead(KEY1) == LOW)
  {
    delay(100);
    if (digitalRead(KEY1) == LOW)
    {
      keep_time = gokit_time_s();
      while (digitalRead(KEY1) == LOW)
      {
        if ((gokit_time_s() - keep_time) > KEY_LONG_TIMER)
        {
          Last_KeyTime = gokit_time_s();
          return KEY1_LONG_PRESS;
        }
      } //until open the key

      if ((gokit_time_s() - Last_KeyTime) > KEY_LONG_TIMER)
      {
        return KEY1_SHORT_PRESS;
      }
      return 0;
    }
    return 0;
  }
  return 0;
}

char gokit_key2down(void)
{
  unsigned long keep_time = 0;
  if (digitalRead(KEY2) == LOW)
  {
    delay(100);
    if (digitalRead(KEY2) == LOW)
    {
      keep_time = gokit_time_s();
      while (digitalRead(KEY2) == LOW) //until open the key
      {

        if ((gokit_time_s() - keep_time) > KEY_LONG_TIMER)
        {
          Last_KeyTime = gokit_time_s();
          return KEY2_LONG_PRESS;
        }
      }

      if ((gokit_time_s() - Last_KeyTime) > KEY_LONG_TIMER)
      {
        return KEY2_SHORT_PRESS;
      }
      return 0;
    }
    return 0;
  }
  return 0;
}

char gokit_keydown(void)
{
  char ret = 0;
  ret |= gokit_key2down();
  ret |= gokit_key1down();
  return ret;

}

/**
  KEY_Handle
  @param none
  @return none
*/
void KEY_Handle(void)
{
  /*  Press for over than 3 second is Long Press  */
  switch (gokit_keydown())
  {
    case KEY1_SHORT_PRESS:
      mySerial.println(F("KEY1_SHORT_PRESS , Production Test Mode "));
      myGizwits.setBindMode(WIFI_PRODUCTION_TEST);
      break;
    case KEY1_LONG_PRESS:
      mySerial.println(F("KEY1_LONG_PRESS ,Wifi Reset"));
      myGizwits.setBindMode(WIFI_RESET_MODE);
      break;
    case KEY2_SHORT_PRESS:
      mySerial.println(F("KEY2_SHORT_PRESS Soft AP mode"));
      myGizwits.setBindMode(WIFI_SOFTAP_MODE);
      //Soft AP mode
      break;
    case KEY2_LONG_PRESS:
      mySerial.println(F("KEY2_LONG_PRESS ,AirLink mode"));
      myGizwits.setBindMode(WIFI_AIRLINK_MODE);
      //AirLink mode
      break;
    default:
      break;
  }
}

/**
  Serial Init , Gizwits Init
  @param none
  @return none
*/
void setup() {
  // put your setup code here, to run once:
  mySerial.begin(115200);
digitalWrite(8, LOW);
pinMode(PIR_sensor, INPUT);   //设置PIR模拟端口为输入模式
  pinMode(LED, OUTPUT);         //设置端口2为输出模式

  //接继电器
  pinMode(KEY1, INPUT_PULLUP);
  pinMode(KEY2, INPUT_PULLUP);
  myservo.attach(9);//舵机针脚位9
  myservo.write(0); //舵机初始化0度
  SPI.begin();

  irrecv.enableIRIn(); // Start the receiver
  pinMode(d, OUTPUT);
  digitalWrite(d, HIGH);
  pinMode(PIN_SENSOR, INPUT);
  Serial.begin(9600);
  myGizwits.begin();

  mySerial.println("GoKit init  OK \n");
}

/**
  Wifi status printf
  @param none
  @return none
*/
void wifiStatusHandle()
{
  if (myGizwits.wifiHasBeenSet(WIFI_SOFTAP))
  {
    mySerial.println(F("WIFI_SOFTAP!"));
  }

  if (myGizwits.wifiHasBeenSet(WIFI_AIRLINK))
  {
    mySerial.println(F("WIFI_AIRLINK!"));
  }

  if (myGizwits.wifiHasBeenSet(WIFI_STATION))
  {
    mySerial.println(F("WIFI_STATION!"));
  }

  if (myGizwits.wifiHasBeenSet(WIFI_CON_ROUTER))
  {
    mySerial.println(F("WIFI_CON_ROUTER!"));
  }

  if (myGizwits.wifiHasBeenSet(WIFI_DISCON_ROUTER))
  {
    mySerial.println(F("WIFI_DISCON_ROUTER!"));
  }

  if (myGizwits.wifiHasBeenSet(WIFI_CON_M2M))
  {
    mySerial.println(F("WIFI_CON_M2M!"));
  }

  if (myGizwits.wifiHasBeenSet(WIFI_DISCON_M2M))
  {
    mySerial.println(F("WIFI_DISCON_M2M!"));
  }
}

/**
  Arduino loop
  @param none
  @return none
*/
void loop() {
  if (irrecv.decode(&results))

  {
    if (results.value == results.value)
    {
      Serial.println(results.value, HEX);
      Serial.println("2");
      myservo.write(180);
      digitalWrite(8, HIGH);
      delay(3500);
      digitalWrite(8, LOW);
      myservo.write(0);
      irrecv.resume();
    }
    delay(100);
    irrecv.resume();
  }
  int x = digitalRead(PIN_SENSOR);
  Serial.println(x);
  if (x == 0)
  { myservo.write(180);
    digitalWrite(8, HIGH);
    delay(3500);
    digitalWrite(8, LOW);
    myservo.write(0);
  }
  KEY_Handle();//key handle , network configure
  wifiStatusHandle();//WIFI Status Handle
  int chk = DHT11.read(DHT11PIN);

  Serial.print("Read sensor: ");
  switch (chk)
  {
    case DHTLIB_OK:
      Serial.println("OK");
      break;
    case DHTLIB_ERROR_CHECKSUM:
      Serial.println("Checksum error");
      break;
    case DHTLIB_ERROR_TIMEOUT:
      Serial.println("Time out error");
      break;
    default:
      Serial.println("Unknown error");
      break;
  }
  unsigned long varW_wendu = (float)DHT11.temperature ; //Add Sensor Data Collection
  myGizwits.write(VALUE_wendu, varW_wendu);
  unsigned long varW_shidu = (float)DHT11.humidity;//Add Sensor Data Collection
  myGizwits.write(VALUE_shidu, varW_shidu);
unsigned long varW_ren = 0;//Add Sensor Data Collection
  myGizwits.write(VALUE_ren, varW_ren);
   val = analogRead(PIR_sensor);    //读取A0口的电压值并赋值到val
           
  varW_ren = analogRead(PIR_sensor);  
  if (val > 150)//判断PIR数值是否大于150，
  {
    digitalWrite(LED,HIGH);  //大于表示感应到有人
   
  }
  else
  {
    digitalWrite(LED,LOW);
    
  }

  bool varR_deng = 0;
  if (myGizwits.hasBeenSet(EVENT_deng))
  {
    myGizwits.read(EVENT_deng, &varR_deng); //Address for storing data
    if (varR_deng == 1)
    {
      for (int i = 0; i < 100; i++ )
        myservo.write(180);
      digitalWrite(8, HIGH);
      delay(3500);
      digitalWrite(8, LOW);
      myservo.write(0);
    }
    else
      digitalWrite(8, LOW);
    myservo.write(0);
    mySerial.println(F("EVENT_deng"));
    mySerial.println(varR_deng, DEC);

  }
  myGizwits.process();
}
