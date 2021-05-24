/*********
  Alain Ferraro
  Complete project details at 
*********/

#include <ArduinoOTA.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <jsonlib.h>
#include "FS.h"
#include "logging.h"

#define RESET_INITIAL_NTPTIME false // Permet de réinitialisé le temps initial | Allows to reset initial NTP time

/*********
  Network parameters
*********/

// Replace with your network details
const char* ssid = "xxxxxxxxxx";
const char* password = "xxxxxxxxxxxxxxxxxxxxxxxxxx";
String newHostname = "Arrosage";

/*********
  Pins
*********/

const int analogInPin = A0; // ESP8266 Analog Pin ADC0 = A0

// PWM sur GPIO4 D2
const int ledPin = 4; // PWM output pin
const byte OC1B_PIN = ledPin;

/*********
  Watering parameters
*********/

int minIndex = 418;
int maxIndex = 859;
int seuilIndex = 600;
int wateringSleep = 5;
unsigned long sequence = 10203L;

/*********
  Program parameters
*********/

const char *pgmVersion = "1.0.5";
boolean wateringSleeping = true;
boolean pgmRunning = false;
unsigned long next2doepoch = 0L;
boolean wateringOn = false;
int task2doNum = 0;
int wateringSleepPgm = 0;
unsigned long sequencePgm = 0L;
unsigned long int nowPgm = 0;
String descr = "Description";

/*********
  Program variables
*********/

boolean pumpOn = false;
int sensorValue = 0; // value read from the pot
String moistureString;
String header;

// Current time
unsigned long currentTime = millis();
// Previous time
unsigned long previousTime = 0;
// Define timeout time in milliseconds (example: 2000ms = 2s)
const long timeoutTime = 2000;

boolean htmllog = false;

/*********
  Network parameters
*********/

// Web Server on port 80
WiFiServer server(80);
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", 3600, 60000);

/*********
  Sequence
*********/

void task2do(boolean actionOn)
{
    if (task2doNum == 1)
    {
        actionTest(actionOn);
        return;
    }

    if (task2doNum == 2)
    {
        setPump(actionOn);
        return;
    }
}

// compute epoch for next state
int compNext()
{
    unsigned long time2wait = sequencePgm % 100;
    if (time2wait == 0)
    {
        return time2wait;
    }
    if (wateringSleeping)
    {
        time2wait = wateringSleepPgm;
        next2doepoch = nowPgm + wateringSleepPgm;
    }
    else
    {
        next2doepoch = nowPgm + time2wait;
        // prepare next watering
        sequencePgm = sequencePgm / 100;
    }
    return time2wait;
}

boolean sequencer()
{
    int time2wait = 0;
    if (!pgmRunning and (sequencePgm == 0))
    {
        return false;
    }

    nowPgm = timeClient.getEpochTime();
    if (!pgmRunning and (sequencePgm > 0))
    {
        // program start
        if (htmllog)
        {
            writeLog("Program start : ");
            // writeLogln(sequencePgm, timeClient);
            writeLogln(String(sequencePgm), timeClient);
            writeLog("Moisture : ");
            writeLogln(moistureString, timeClient);
        }
        else
        {
            Serial.println("Program start");
            Serial.print("sequencePgm: ");
            Serial.println(sequencePgm);
        }
        task2do(true);
        wateringSleeping = false;
        time2wait = compNext();
        pgmRunning = true;
        return true;
    }
    if (nowPgm >= next2doepoch)
    {
        if (wateringSleeping)
        {
            task2do(true);
            wateringSleeping = false;
        }
        else
        {
            task2do(false);
            wateringSleeping = true;
        }
        time2wait = compNext();
        /*
        Serial.print("time2wait: ");
        Serial.println(time2wait);
        Serial.print("sequencePgm: ");
        Serial.println(sequencePgm);
        */
        if ((sequencePgm == 0) and (time2wait == 0))
        {
            // program end
            if (htmllog)
            {
                writeLogln("Program stop", timeClient);
                writeLog("Moisture : ");
                writeLogln(moistureString, timeClient);
            }
            else
            {
                Serial.println("Program stop");
            }
            pgmRunning = false;
            wateringSleeping = true;
            resetInitialTime();
            return false;
        }
    }
    return true;
}

void actionTest(boolean actionOn)
{
    if (actionOn)
    {
        Serial.println("Action On");
    }
    else
    {
        Serial.println("Action Off");
    }
}

void actionInit(unsigned long seq, int sleeptime, int todo)
{
    sequencePgm = seq;
    task2doNum = todo;
    wateringSleepPgm = sleeptime;
    pgmRunning = false;
    // Serial.print("sequencePgm: ");
    // Serial.println(sequencePgm);
}

void t2do(bool actionOn, void (*pf)(bool))
{
    pf(actionOn);
}

void testCall()
{
    t2do(true, actionTest);
    t2do(false, setPump);
}

/*********
  WiFi
*********/

void initWiFi()
{
    // analogWrite(ledPin, 100);
    WiFi.mode(WIFI_STA);
    WiFi.hostname(newHostname.c_str());
    WiFi.begin(ssid, password);
    Serial.print("Connecting to WiFi ..");
    while (WiFi.status() != WL_CONNECTED)
    {
        Serial.print('.');
        delay(1000);
    }
    // analogWrite(ledPin, 20);
    Serial.println(WiFi.localIP());
    //The ESP8266 tries to reconnect automatically when the connection is lost
    WiFi.setAutoReconnect(true);
    WiFi.persistent(true);
}

/*********
  Time management
*********/

int lastEvent;
int start = millis();
long int _now = 0;
int _hour;
int _min;

void resetInitialTime()
{
    timeClient.forceUpdate();
    _now = timeClient.getEpochTime();
    _hour = (_now % 86400L) / 3600;
    _min = (_now % 3600) / 60;
    Serial.print("The UTC time is ");
    Serial.print(_hour);
    Serial.print(':');
    if (_min < 10)
    {
        Serial.print('0');
    }
    Serial.println(_min);
    savePersistent();
}

bool persistentExist()
{
    File f = SPIFFS.open("/persistent.txt", "r");
    if (!f)
    {
        return false;
    }
    return true;
}

bool loadPersistent()
{
    File f = SPIFFS.open("/lastEvent.txt", "w");
    if (f)
    {
        SPIFFS.remove("/lastEvent.txt");
    }
    // Recharge le dernier horodatage depuis la zone SPIFFS | Reload last date/time from SPIFFS area
    f = SPIFFS.open("/persistent.txt", "r");
    if (!f)
    {
        Serial.println("Failed to open file, force NTP");
        return false;
    }
    // Serial.print("loadPersistent : ");
    String jsonStr = f.readStringUntil('}') + "}";
    // Serial.println(jsonStr);
    minIndex = jsonExtract(jsonStr, "minindex").toInt();
    maxIndex = jsonExtract(jsonStr, "maxindex").toInt();
    seuilIndex = jsonExtract(jsonStr, "seuilindex").toInt();
    wateringSleep = jsonExtract(jsonStr, "wateringsleep").toInt();
    sequence = jsonExtract(jsonStr, "sequence").toInt();
    lastEvent = jsonExtract(jsonStr, "epoch").toInt();
    descr = String(jsonExtract(jsonStr, "descr"));
    return true;
}

bool savePersistent()
{
    File f = SPIFFS.open("/persistent.txt", "w");
    if (!f)
    {
        Serial.println("file open failed");
        return false;
    }
    Serial.println("savePersistent");
    f.println(params2json());
    f.close();
    return true;
}

long int calculateTimeSpent()
{
    loadPersistent();
    // timeClient.forceUpdate();
    _now = timeClient.getEpochTime();
    int timeSpent = _now - lastEvent;
    Serial.print("Time spent since last update (s): ");
    Serial.println(timeSpent);
    return timeSpent;
}

/*********
  App functions
*********/

String getMoisture()
{
    int deltaIndex = 0L;
    int moisture = 0L;
    // read the analog in value
    sensorValue = analogRead(analogInPin);
    if (sensorValue < minIndex)
    {
        return "100";
    }
    if (sensorValue >= maxIndex)
    {
        return "0";
    }
    deltaIndex = maxIndex - sensorValue;
    moisture = (deltaIndex * 100) / (maxIndex - minIndex);
    return (String(moisture));
}

//equivalent of analogWrite on pin 10
void setPump(boolean pump)
{
    Serial.print("Set Pump :  ");
    if (pump)
    {
        Serial.println("On");
        digitalWrite(ledPin, HIGH);
        pumpOn = true;
    }
    else
    {
        Serial.println("Off");
        digitalWrite(ledPin, LOW);
        pumpOn = false;
    }
    if (htmllog)
    {
        if (pump)
        {
            writeLogln("Set Pump On", timeClient);
        }
        else
        {
            writeLogln("Set Pump Off", timeClient);
        }
    }
}

/*********
  Web functions
*********/

String getParam(String name)
{
    String result;
    result = "";
    result = header.substring(header.indexOf(name));
    result = result.substring(0, result.indexOf('\n'));
    // Serial.println(result);
    result = result.substring(0, result.indexOf(' '));
    // Serial.println(result);
    if (result.indexOf("&") >= 0)
    {
        result = result.substring(0, result.indexOf('&'));
        // Serial.println(result);
    }
    result = result.substring(result.indexOf('=') + 1);
    // Serial.println(result);
    return result;
}

void webHeader(WiFiClient client)
{
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: application/json;charset=UTF-8");
    client.println("Connection: close");
    client.println();
}

void webBody(WiFiClient client)
{
    client.println(params2json());
}

void webOutput(WiFiClient client)
{
    webHeader(client);
    webBody(client);
}

void webServer()
{
    WiFiClient client = server.available();
    if (client)
    {
        currentTime = millis();
        previousTime = currentTime;
        Serial.println("New client");
        // bolean to locate when the http request ends
        boolean blank_line = true;
        while (client.connected() && currentTime - previousTime <= timeoutTime)
        {
            currentTime = millis();
            if (client.available())
            {
                char c = client.read();
                header += c;

                if (c == '\n' && blank_line)
                {
                    boolean data2save = false;
                    if (header.indexOf("minindex") >= 0)
                    {
                        minIndex = getParam("minindex").toInt();
                        writeLogln("Set minIndex to " + getParam("minindex"), timeClient);
                        data2save = true;
                    }
                    if (header.indexOf("maxindex") >= 0)
                    {
                        maxIndex = getParam("maxindex").toInt();
                        writeLogln("Set maxIndex to " + getParam("maxindex"), timeClient);
                        data2save = true;
                    }
                    if (header.indexOf("seuilindex") >= 0)
                    {
                        seuilIndex = getParam("seuilindex").toInt();
                        writeLogln("Set seuilIndex to " + getParam("seuilindex"), timeClient);
                        data2save = true;
                    }
                    if (header.indexOf("sequence") >= 0)
                    {
                        sequence = getParam("sequence").toInt();
                        writeLogln("Set sequence to " + getParam("sequence"), timeClient);
                        data2save = true;
                    }
                    if (header.indexOf("wateringsleep") >= 0)
                    {
                        wateringSleep = getParam("wateringsleep").toInt();
                        writeLogln("Set wateringSleep to " + getParam("wateringsleep"), timeClient);
                        data2save = true;
                    }
                    if (header.indexOf("descr") >= 0)
                    {
                        descr = getParam("descr");
                        writeLogln("Set descr to " + getParam("descr"), timeClient);
                        data2save = true;
                    }
                    if (header.indexOf("wateringpgm") >= 0)
                    {
                        if (!pgmRunning)
                        {
                            unsigned long wPgm = getParam("wateringpgm").toInt();
                            actionInit(wPgm, wateringSleep, 2);
                        }
                    }
                    if (header.indexOf("pgmtest") >= 0)
                    {
                        if (!pgmRunning)
                        {
                            actionInit(30405L, 5, 2);
                        }
                    }
                    if (header.indexOf("log.html") >= 0)
                    {
                        webOutputLog(client);
                        header = "";
                        break;
                    }

                    if (data2save)
                    {
                        savePersistent();
                    }

                    webOutput(client);
                    header = "";
                    break;
                }
                if (c == '\n')
                {
                    // when starts reading a new line
                    blank_line = true;
                }
                else if (c != '\r')
                {
                    // when finds a character on the current line
                    blank_line = false;
                }
            }
        }
        // closing the client connection
        delay(1);
        client.stop();
        Serial.println("Client disconnected.");
    }
}

/*********
  OTA
*********/

void setupOta()
{
    // Hostname defaults to esp8266-[ChipID]
    // ArduinoOTA.setHostname("Demo OTA ESP8266");

    // No authentication by default
    // ArduinoOTA.setPassword((const char *)"123");

    ArduinoOTA.onStart([]() {
        Serial.println("Start");
    });

    ArduinoOTA.onEnd([]() {
        Serial.println("\nEnd");
    });

    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    });

    ArduinoOTA.onError([](ota_error_t error) {
        Serial.printf("Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR)
            Serial.println("Auth Failed");
        else if (error == OTA_BEGIN_ERROR)
            Serial.println("Begin Failed");
        else if (error == OTA_CONNECT_ERROR)
            Serial.println("Connect Failed");
        else if (error == OTA_RECEIVE_ERROR)
            Serial.println("Receive Failed");
        else if (error == OTA_END_ERROR)
            Serial.println("End Failed");
    });

    Serial.println("## ArduinoOTA.begin ##");

    ArduinoOTA.begin();
    Serial.println("## Ready");
    Serial.print("## IP address: ");
    Serial.println(WiFi.localIP());
}

/*********
  Tools
*********/

void spiffsInfo()
{
    // Get all information about SPIFFS
    FSInfo fsInfo;
    SPIFFS.info(fsInfo);

    Serial.println("File system info");

    // Taille de la zone de fichier
    Serial.print("Total space:      ");
    Serial.print(fsInfo.totalBytes);
    Serial.println("byte");

    // Espace total utilise
    Serial.print("Total space used: ");
    Serial.print(fsInfo.usedBytes);
    Serial.println("byte");

    // Taille d un bloc et page
    Serial.print("Block size:       ");
    Serial.print(fsInfo.blockSize);
    Serial.println("byte");

    Serial.print("Page size:        ");
    Serial.print(fsInfo.totalBytes);
    Serial.println("byte");

    Serial.print("Max open files:   ");
    Serial.println(fsInfo.maxOpenFiles);

    // Taille max. d un chemin
    Serial.print("Max path lenght:  ");
    Serial.println(fsInfo.maxPathLength);

    Serial.println();
}

String params2json()
{
    String result = "{\n";
    result = result + "\"minindex\":";
    result = result + minIndex;
    result = result + ",\n\"maxindex\":";
    result = result + maxIndex;
    result = result + ",\n\"seuilindex\":";
    result = result + seuilIndex;
    result = result + ",\n\"sensorvalue\":";
    result = result + sensorValue;
    result = result + ",\n\"moisture\":";
    result = result + moistureString;
    result = result + ",\n\"pumpon\":";
    if (pumpOn)
    {
        result = result + "1";
    }
    else
    {
        result = result + "0";
    }
    result = result + ",\n\"epoch\":";
    result = result + timeClient.getEpochTime();
    result = result + ",\n\"sequence\":";
    result = result + sequence;
    result = result + ",\n\"wateringsleep\":";
    result = result + wateringSleep;
    result = result + ",\n\"pgmrunning\":";
    if (pgmRunning)
    {
        result = result + "1";
    }
    else
    {
        result = result + "0";
    }
    result = result + ",\n\"wateringsleeping\":";
    if (wateringSleeping)
    {
        result = result + "1";
    }
    else
    {
        result = result + "0";
    }
    result = result + ",\n\"sequencepgm\":";
    result = result + sequencePgm;
    result = result + ",\n\"wateringsleeppgm\":";
    result = result + wateringSleepPgm + ",\n";
    result = result + "\"pgmversion\":\"" + pgmVersion + "\",\n";
    result = result + "\"descr\":\"" + descr + "\",\n";
    result = result + "\"timedata\":\"";
    result = result + getStringDate(timeClient) + " ";
    result = result + timeClient.getFormattedTime();
    result = result + "\"\n}\n";
    return result;
}

/*********
  Setup
*********/

void setup()
{
    // initialize serial communication at 115200
    pinMode(ledPin, OUTPUT);
    Serial.begin(115200);
    SPIFFS.begin();
    delay(10);
    spiffsInfo();
    htmllog = initLog();

    initWiFi();
    delay(5000);
    timeClient.begin();

    if (htmllog)
    {
        writeLog("## Start of Program v: ");
        writeLog(pgmVersion);
        writeLogln(" ##", timeClient);
        writeLog("Connecting to ");
        writeLogln(ssid, timeClient);
    }
    else
    {
        Serial.println("## Start of Program ##");
        Serial.println();
        Serial.print("Connecting to ");
        Serial.println(ssid);
    }

    setupOta();

    // Starting the web server
    server.begin();
    Serial.println("Web server running. Waiting for the ESP IP...");
    // delay(10000);
    // Printing the ESP IP address
    Serial.println(WiFi.localIP());

    timeClient.forceUpdate();
    if (RESET_INITIAL_NTPTIME)
    {
        resetInitialTime();
    }
    if (!persistentExist())
    {
        Serial.println("First Boot, store NTP time on SPIFFS area");
        savePersistent();
    }
    else
    {
        Serial.println("NTP time already stored on SPIFFS area");
        calculateTimeSpent();
    }
    timeClient.update();
    Serial.println(timeClient.getFormattedTime());
    calculateTimeSpent();
    // actionInit(sequence, wateringSleep, 2);
}

/*********
  Loop
*********/

void loop()
{
    ArduinoOTA.handle();
    Serial.println(timeClient.getFormattedTime());
    calculateTimeSpent();
    // read the analog in value
    moistureString = getMoisture();
    // sensorValue = analogRead(analogInPin);
    // print the readings in the Serial Monitor
    Serial.print("sensor = ");
    Serial.println(sensorValue);
    if ((sensorValue > seuilIndex) and (!pgmRunning))
    {
        actionInit(sequence, wateringSleep, 2);
    }

    // Listenning for new clients
    webServer();
    // sequencer
    sequencer();

    delay(1000);
}
