//******************************************************************
//*       Sketch ESP32Sim800LTank V0.9
//*
//*               16.02.2021
//*
//*             Felix Wienberg
//*
//*             GitHub: wienbef
//*
//* This sktech measures the actually volume of a tank. This information
//* is being send to a server using the POST-method. The connection to
//* the internet is established with the SIM800L-module on the Board 
//* LILYGO® TTGO T-Call ESP32 SIM800L.
//******************************************************************
#define debug; // DEBUGVARIABLE for serial connection

#define SerialMon Serial // Serial for PC-connection

#define SerialAT Serial1 // Serial for SIM800L

//******************************************************************
//* Variables
//******************************************************************

// For the distance
int trigger = 0; //IO-Adress. maybe change
int echo = 2; //IO-Adress. maybe change
long duration = 0; // Duration
float distance = 0;  // Distance
float offset = 0; // if the measurement-sensor is higher than the tank
float distancemax = 0; // Maximum


// For the volume of the lying cylinder (tank)
float menge = 0; // Volume
int intmenge = 0; // Volume in integer because of the high volume
float maxvolume = 20000; // Maximum Volume of the tank

//  Specify this variables. In this case the tank is like a DIN 6608/2-tank with 20k litres
float cylinderlength = 6.366; // Change the length of the tank
float radius = 1;// Change the radius of the tank


// For the GPRS-connection & internet-connection
//...in the following
// Configure TinyGSM library
#define TINY_GSM_MODEM_SIM800      // Modem is SIM800
#define TINY_GSM_RX_BUFFER   1024  // Set RX buffer to 1Kb
#include <Wire.h>
#include <TinyGsmClient.h>

// This part musst be change to specific datas of the used network
const char apn[]      = "internet.telekom"; // APN (example: internet.vodafone.pt) use https://wiki.apnchanger.org
const char gprsUser[] = "telekom"; // GPRS User
const char gprsPass[] = "tm"; // GPRS Password

const char simPIN[]   = "";  // empty, if no PIN

// TTGO T-Call pins for SIM800L
#define MODEM_RST            5
#define MODEM_PWKEY          4
#define MODEM_POWER_ON       23
#define MODEM_TX             27
#define MODEM_RX             26

// Datas of the Server
// This part musst be change to specific datas of the used server
const char server[] = "server.com"; // domain name: example.com, maker.ifttt.com, etc
const char resource[] = "/volume.php"; // resource path, for example: /post-data.php
const int  port = 80; // server port number


// for the deep sleep
#define uS_TO_S_FACTOR 1000000ULL  // Conversion factor for micro seconds to seconds
#define TIME_TO_SLEEP  432000        // Time ESP32 will go to sleep (in days) (432000 = 5 days)
//#define TIME_TO_SLEEP 60 // for debugging: 1 minute



//******************************************************************
//* Start Setup
//******************************************************************
void setup() {
  
  #ifdef debug
    SerialMon.begin(115200);
    delay(10);
    SerialMon.println("hello world");
  #endif


//******************************************************************
//* The following part ist for the distance measurement and calculating the volume
//******************************************************************
  pinMode(trigger, OUTPUT);
  pinMode(echo, INPUT);

  // starting measurement
  digitalWrite(trigger, LOW);  
  delay(5);
  digitalWrite(trigger, HIGH); 
  delay(20);
  digitalWrite(trigger, LOW); 

  #ifdef debug
    SerialMon.println("starting measurement");
  #endif
  
  duration = pulseIn(echo, HIGH); //Measuring
  distance = (duration / 2) / 28.6; //Calculating
  distance = distance + offset; //Offsetcalculating
  distance = distance / 100; //distance in metres
  #ifdef debug
    SerialMon.println("end of measurment");
    SerialMon.print("result: ");
    SerialMon.println(distance);
  #endif

  distancemax = (radius * 2) + offset;
  
  //volume of the tank
  if(distance < distancemax){ // check if distance ist in the range from tank
    menge = cylindercalculating(cylinderlength, radius, distance, maxvolume); // calculating volume
    #ifdef debug
      SerialMon.println("calculating volume");
    #endif
  }else{
    menge = 0;
    #ifdef debug
      SerialMon.println("volume is not in range");
    #endif
  }

  intmenge = (int)menge; // convert to integer
  #ifdef debug
    SerialMon.println("calculting finished");
    SerialMon.print("result volume in float: ");
    SerialMon.println(menge);
    SerialMon.print("result volume in int: ");
    SerialMon.println(intmenge);
  #endif
if(true){ // Enable Serverconnection
//******************************************************************
//* The following part ist for the GPRS-Connection
//******************************************************************
  TinyGsm modem(SerialAT);
  TinyGsmClient client(modem);

// Set modem reset, enable, power pins
  pinMode(MODEM_PWKEY, OUTPUT);
  pinMode(MODEM_RST, OUTPUT);
  pinMode(MODEM_POWER_ON, OUTPUT);
  digitalWrite(MODEM_PWKEY, LOW);
  digitalWrite(MODEM_RST, HIGH);
  digitalWrite(MODEM_POWER_ON, HIGH);

// Set GSM module baud rate and UART pins
  SerialAT.begin(115200, SERIAL_8N1, MODEM_RX, MODEM_TX);
  delay(3000);


  #ifdef debug
    SerialMon.println("Initializing modem...");
  #endif
// init SIM800 module, it takes quite some time
// maybe use restart() instead of init()
  if(!modem.init()){
    #ifdef debug
      SerialMon.println("modeminit not worked");  
    #endif
    delay(3000);
    ESP.restart();
  }

// Unlock your SIM card with a PIN if needed
  if (strlen(simPIN) && modem.getSimStatus() != 3 ) {
    modem.simUnlock(simPIN);
  }

// Connecting to the internet
  #ifdef debug
    SerialMon.println("Connecting to APN");
  #endif

  if(!modem.waitForNetwork()){
    #ifdef debug
      SerialMon.println("no network"); 
    #endif 
    delay(3000);
    ESP.restart();
  }

  if (!modem.gprsConnect(apn, gprsUser, gprsPass)) {
    #ifdef debug
      SerialMon.println("APN fail");
    #endif
    delay(3000);
    ESP.restart();
  }else{
  #ifdef debug
    SerialMon.println(" OK");
  //******************************************************************
  //* The following part ist for the HTTP-POST. This is in the IF-Statement of the internet-connection. Without internet-connection there ist no server-connection
  //******************************************************************

    SerialMon.print("Connecting to ");
    SerialMon.print(server);
  #endif
  
    if (!client.connect(server, port)) {
      #ifdef debug
        SerialMon.println(" fail");
      #endif
      modem.gprsDisconnect();
    }else{
      #ifdef debug
        SerialMon.println(" OK");
      #endif

      String httpRequestData = "menge="; // Datas to send
      httpRequestData += intmenge; // append
      #ifdef debug
        SerialMon.println(httpRequestData);
      #endif

      client.print(String("POST ") + resource + " HTTP/1.1\r\n");
      client.print(String("Host: ") + server + "\r\n");
      client.println("Connection: close");
      client.println("Content-Type: application/x-www-form-urlencoded");
      client.print("Content-Length: ");
      client.println(httpRequestData.length());
      client.println();
      client.println(httpRequestData);

      unsigned long timeout = millis();
      //send
      while (client.connected() && millis() - timeout < 10000L) {
  // Print available data (HTTP response from server)
        while (client.available()) {
          char c = client.read();
          #ifdef debug
            SerialMon.print(c);
          #endif
          timeout = millis();
        }
      }

      // connection stop...
      client.stop();
      #ifdef debug
        SerialMon.println(F("Server disconnected"));
      #endif
      delay(500);
      modem.gprsDisconnect();
      #ifdef debug
        SerialMon.println(F("GPRS disconnected"));
      #endif

      
    }
  }
  digitalWrite(MODEM_POWER_ON, LOW); // Shutdown of the SIM800L
  delay(500);
}

//******************************************************************
//* The following part ist for the deep sleep
//******************************************************************

esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR); // sleeptime

#ifdef debug
  //esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
  SerialMon.println("wakeupper");
  //SerialMon.flush();
#endif
//SerialAT.flush();
delay(1000);

esp_deep_sleep_start(); // go sleep



}

void loop() {
  // put your main code here, to run repeatedly:
  // nothing to do...

}


float cylindercalculating(float lengthofcylinder, float cylinderradius, float measuredistance, float tankvolume){
  
  // Volume = length(radius^2 * arccos ((radius-heigth)/radius) - (radius-heigth) * square root(2*radius*heigth - heigth^2))
  
  float diff = cylinderradius - measuredistance; // difference between radius and distance
  float squareradius = cylinderradius * cylinderradius; // square of radius
  float squaredistance = measuredistance * measuredistance; // square of distance
  float quotient = diff / cylinderradius;
  float arccos = acos(quotient);
  float diffpart1 = squareradius * arccos;
  float product = cylinderradius * measuredistance;
  product = product * 2;
  float undersquareroot = product - squaredistance;
  float squareroot = sqrt(undersquareroot);
  float diffpart2 = diff * squareroot;
  diff = diffpart1 - diffpart2;
  
  float cylindervolume = lengthofcylinder * diff;
  cylindervolume = cylindervolume * 1000; // in litres
  cylindervolume = tankvolume - cylindervolume; // difference to tankvolume
  
  return cylindervolume;
 }
