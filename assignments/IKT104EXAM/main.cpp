/* 
 * mbed Microcontroller Library
 * Copyright (c) 2019 ARM Limited
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mbed.h"
#include "DFRobot_RGBLCD.h"
#include "HTS221Sensor.h"
#include "json.hpp"

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <map>
#include <time.h>

#include "cert.h"
#include "wifi.h"

// Blinking rate in milliseconds
#define BLINKING_RATE 250ms

#define IPGAPIKEY "<Your API key>"
#define OPENAPIKEY "<Your API key>"
#define IPGAPI api.ipgeolocation.io
#define OPENAPI api.openweathermap.org

#define JSON_NOEXCEPTION

#define SAMPLE_FLAG1 (1UL << 0)
#define SAMPLE_FLAG2 (1UL << 10)
#define SAMPLE_FLAG3 (1UL << 20)
EventFlags eventflags;

DevI2C i2c(PB_11, PB_10);
HTS221Sensor sensor(&i2c);
DigitalOut led(LED1);

PwmOut piz(LED1); 
DigitalIn btn1(PA_1); DigitalIn btn2(PA_0); DigitalIn btn3(PD_14); 
InterruptIn btn4(PA_4);
AnalogIn potentiometer(PC_0);

DFRobot_RGBLCD lcd(16,2,D14,D15); 

typedef struct 
{
  Mutex mutex;
  string timezone, date_time_txt, date_time;
  float date_time_unix;
} Data;

typedef struct 
{
  Mutex mutex;
  string description;
  float windspeed, temp;
} Weather_data;


//frequency array
float frequency[]={659,554,659,554,550,494,554,587,494,659,554,440};

Thread t1(osPriorityHigh, 5000);
Thread t2(osPriorityAboveNormal, 5000);

Mail<Data, 16> date_time_mail;
Mail<Weather_data, 16> weather_mail;

EventQueue queue(4 * EVENTS_EVENT_SIZE);

Timer t;
Timeout timeout;
char curr_time[32] = { NULL };
tm * now;

static volatile int state_counter = 0;
static volatile bool running = true;
static volatile bool running_weather = true;
static volatile bool timeout_set, set_alarm_mode, hour_minute = false;

string delimiter, token;

using namespace std;
using namespace chrono;
using json = nlohmann::json;

void interrupt_state()
{   

    if (state_counter >= 3){
        state_counter = 0;
    }
    else{
        state_counter++;
    }
}

void interrupt_set_timeout()
{
    timeout_set = !timeout_set;
}

void interrupt_switch_hour_minutes()
{
    hour_minute = !hour_minute;
}

void Alarm(void) 
{

    piz.resume();

    for (int i=0; i<=11; i++) {
        piz.period(1/(frequency[i])); // set PWM period
        piz.write(0.8f); // set duty cycle
    }

}

void Snooze(void)
{
    if (timeout.remaining_time() > 0s)
    {
        timeout.attach(&Alarm, timeout.remaining_time() + 300s);
    }
}

void Alarm_detach(void)
{
    piz.suspend();
    timeout.detach();
}

void TempHum_func(void)
{   
    uint8_t id;
    float humidity, temperature;

    sensor.init(nullptr);
    sensor.enable();

    sensor.read_id(&id);

    printf("HTS221 humidity & temperature = 0x%X\r\n", id);

    lcd.init();

    while(true){
    
        sensor.get_humidity(&humidity);
        sensor.get_temperature(&temperature);

        lcd.clear();
        lcd.setCursor(0,0);
        lcd.printf("[temp] %.2f", temperature);
        lcd.setCursor(0,1);
        lcd.printf("[hum] %.2f", humidity); 

        ThisThread::sleep_for(BLINKING_RATE);
    }

}

void Ping_IPGAPI(void)
{
    while (running) 
    {   
        //eventflags.wait_all(SAMPLE_FLAG1);
        Data *mail = date_time_mail.try_alloc();

        // Get pointer to default network interface
        NetworkInterface *network = NetworkInterface::get_default_instance();
        if (!network) {
            printf("Failed to get default network interface\n");
            while (1);
        }
        // Connect to network
        printf("Connecting to the network...\n");
        nsapi_size_or_error_t result = network->connect();
        // Check if the connection is successful
        if (result != 0) {
            printf("Failed to connect to network: %d\n", result);
            while (1);
            } 
        printf("Connection to network successful!\n");
        // Create and allocate resources for socket.
        // TLSSocket is used for HTTPS/SSL
        TLSSocket *socket = new TLSSocket();
        // Set the root certificate of the web site.
        // See include/cert.h for how to download the cert.
        result = socket->set_root_ca_cert(cert);
        if (result != 0) {
            printf("Error: socket->set_root_ca_cert() returned %d\n", result);
        }
        socket->open(network);
        // Create destination address
        SocketAddress address;
        // Get IP address of host by name
        result = network->gethostbyname("api.ipgeolocation.io", &address);
        // Check result
        if (result != 0) {
            printf("Failed to get IP address of host: %d\n", result);
            while (1);
        }
        printf("Got address of host\n");
        // Set server port number, 443 for HTTPS/SSL
        address.set_port(443);
        // Connect to server at the given address
        result = socket->connect(address);
        // Check result
        if (result != 0) {
            printf("Failed to connect to server: %d\n", result);
            while (1);
        }
        printf("Connection to server successful!\n");

        // Create HTTP request
        const char request[] = "GET /timezone?apiKey=68ff965c3f954b04b85e9793cbd86b5a&tz=Norway/Bergen HTTP/1.1\r\n"
                                "Host: api.ipgeolocation.io\r\n"
                                "Connection: close\r\n"
                                "\r\n";
        //sprintf(request, "GET /timezone?apiKey=%f=Norway/Bergen HTTP/1.1\r\n", IPGAPIKEY);

        // Send request
        result = send_request(socket, request);
        // Check result
        if (result != 0) {
            printf("Failed to send request: %d\n", result);
            while (1);
        }
        // We need to read the response into memory. The destination is called a buffer.
        // If you make this buffer static it will be placed in BSS and won't use stack memory.
        static char buffer[4096];
        result = read_response(socket, buffer, sizeof(buffer));
        // Check result
        if (result != 0) {
            printf("Failed to read response: %d\n", result);
            while (1);
        }
        socket->close();
        network->disconnect();
        delete(socket);
     
        // Find the start and end of the JSON data.
        // If the json response is an array you need to replace this with [ and ]
        char* json_begin = strchr(buffer, '{');
        char* json_end = strrchr(buffer, '}');
        // Check if we actually got JSON in the response
        if (json_begin == nullptr || json_end == nullptr) {
            printf("Failed to find JSON in response\n");
            while (1);
        }
        // End the string after the end of the JSON data in case the response contains trailing data
        json_end[1] = 0;
        printf("JSON response:\n");
        printf("%s\n", json_begin);
        // Parse response as JSON, starting from the first {
        json document = json::parse(json_begin);
      
        string timezone, date_time_txt, date_time;
        float date_time_unix;

         // Get data from JSON object
        document["timezone"].get_to(timezone);
        document["date_time"].get_to(date_time);
        document["date_time_txt"].get_to(date_time_txt);
        document["date_time_unix"].get_to(date_time_unix);

        mail->timezone = timezone.c_str();
        mail->date_time = date_time.c_str();
        mail->date_time_txt = date_time_txt.c_str();
        mail->date_time_unix = date_time_unix;
        
        date_time_mail.put(mail);

        running = false;
    }
}
/*
void Ping_OPENAPI(void)
{
    while (running_weather) 
    {   
        //eventflags.wait_all(SAMPLE_FLAG2);

        Weather_data *mail = weather_mail.try_alloc();

        // Get pointer to default network interface
        NetworkInterface *network = NetworkInterface::get_default_instance();
        if (!network) {
            printf("Failed to get default network interface\n");
            while (1);
        }
        // Connect to network
        printf("Connecting to the network...\n");
        nsapi_size_or_error_t result = network->connect();
        // Check if the connection is successful
        if (result != 0) {
            printf("Failed to connect to network: %d\n", result);
            while (1);
            } 
        printf("Connection to network successful!\n");
        // Create and allocate resources for socket.
        // TLSSocket is used for HTTPS/SSL
        TLSSocket *socket = new TLSSocket();
        // Set the root certificate of the web site.
        // See include/cert.h for how to download the cert.
        result = socket->set_root_ca_cert(cert2);
        if (result != 0) {
            printf("Error: socket->set_root_ca_cert() returned %d\n", result);
        }
        socket->open(network);
        // Create destination address
        SocketAddress address;
        // Get IP address of host by name
        result = network->gethostbyname("api.openweathermap.org", &address);
        // Check result
        if (result != 0) {
            printf("Failed to get IP address of host: %d\n", result);
            while (1);
        }
        printf("Got address of host\n");
        // Set server port number, 443 for HTTPS/SSL
        address.set_port(443);
        // Connect to server at the given address
        result = socket->connect(address);
        // Check result
        if (result != 0) {
            printf("Failed to connect to server: %d\n", result);
            while (1);
        }
        printf("Connection to server successful!\n");

        // Create HTTP request
        const char request[] = "GET /data/2.5/weather?q=Bergen&appid=587bf2de43f4a0a6438620de28169b1e&units=metric HTTP/1.1\r\n"
                                "Host: api.openweathermap.org\r\n"
                                "Connection: close\r\n"
                                "\r\n";
        //sprintf(request, "GET /timezone?apiKey=%f=Norway/Bergen HTTP/1.1\r\n", IPGAPIKEY);

        // Send request
        result = send_request(socket, request);
        // Check result
        if (result != 0) {
            printf("Failed to send request: %d\n", result);
            while (1);
        }
        // We need to read the response into memory. The destination is called a buffer.
        // If you make this buffer static it will be placed in BSS and won't use stack memory.
        static char buffer[4096];
        result = read_response(socket, buffer, sizeof(buffer));
        // Check result
        if (result != 0) {
            printf("Failed to read response: %d\n", result);
            while (1);
        }
        socket->close();
        network->disconnect();
        delete(socket);
     
        // Find the start and end of the JSON data.
        // If the json response is an array you need to replace this with [ and ]
        char* json_begin = strchr(buffer, '{');
        char* json_end = strrchr(buffer, '}');
        // Check if we actually got JSON in the response
        if (json_begin == nullptr || json_end == nullptr) {
            printf("Failed to find JSON in response\n");
            while (1);
        }
        // End the string after the end of the JSON data in case the response contains trailing data
        json_end[1] = 0;
        printf("JSON response:\n");
        printf("%s\n", json_begin);
        // Parse response as JSON, starting from the first {
        json document = json::parse(json_begin);
      
        string description;
        float speed, temp;

         // Get data from JSON object
        document["weather"][0]["description"].get_to(description);
        document["wind"]["speed"].get_to(speed);
        document["main"]["temp"].get_to(temp);
       

        mail->description = description.c_str();
        mail->windspeed = speed;
        mail->temp = temp;
     
        
        weather_mail.put(mail);

        running_weather = false;
    }
}
*/
int main()
{

    string date_time, date_time_txt;
    string description;


    uint8_t a_hours, a_minutes = 0;
    uint8_t id;

    float date_time_unix; 
    float windspeed, speed, temp; 
    float humidity, temperature;
    float read_pot;

    sensor.init(nullptr);
    sensor.enable();

    sensor.read_id(&id);

    printf("HTS221 humidity & temperature = 0x%X\r\n", id);

    btn1.mode(PullUp);
    btn2.mode(PullUp);
    btn3.mode(PullUp);
    btn4.fall(callback(interrupt_state));

    //eventflags.set(SAMPLE_FLAG1);
    t1.start(callback(Ping_IPGAPI));
    t1.join();
    ThisThread::sleep_for(4s);
    //eventflags.set(SAMPLE_FLAG2);
    //t2.start(callback(Ping_OPENAPI));
    //t2.join();

    // Yes, I dumped the whole second request in main constr., since I could not get the two threads to run in sequence. 
    Weather_data *mail = weather_mail.try_alloc();

    // Get pointer to default network interface
    NetworkInterface *network = NetworkInterface::get_default_instance();
    if (!network) {
        printf("Failed to get default network interface\n");
        while (1);
    }
    // Connect to network
    printf("Connecting to the network...\n");
    nsapi_size_or_error_t result = network->connect();
    // Check if the connection is successful
    if (result != 0) {
        printf("Failed to connect to network: %d\n", result);
        while (1);
        } 
    printf("Connection to network successful!\n");
    // Create and allocate resources for socket.
    // TLSSocket is used for HTTPS/SSL
    TLSSocket *socket = new TLSSocket();
    // Set the root certificate of the web site.
    // See include/cert.h for how to download the cert.
    result = socket->set_root_ca_cert(cert2);
    if (result != 0) {
        printf("Error: socket->set_root_ca_cert() returned %d\n", result);
    }
    socket->open(network);
    // Create destination address
    SocketAddress address;
    // Get IP address of host by name
    result = network->gethostbyname("api.openweathermap.org", &address);
    // Check result
    if (result != 0) {
        printf("Failed to get IP address of host: %d\n", result);
        while (1);
    }
    printf("Got address of host\n");
    // Set server port number, 443 for HTTPS/SSL
    address.set_port(443);
    // Connect to server at the given address
    result = socket->connect(address);
    // Check result
    if (result != 0) {
        printf("Failed to connect to server: %d\n", result);
        while (1);
    }
    printf("Connection to server successful!\n");

    // Create HTTP request
    const char request[] = "GET /data/2.5/weather?q=Bergen&appid=587bf2de43f4a0a6438620de28169b1e&units=metric HTTP/1.1\r\n"
                            "Host: api.openweathermap.org\r\n"
                            "Connection: close\r\n"
                            "\r\n";
    //sprintf(request, "GET /timezone?apiKey=%f=Norway/Bergen HTTP/1.1\r\n", IPGAPIKEY);

    // Send request
    result = send_request(socket, request);
    // Check result
    if (result != 0) {
        printf("Failed to send request: %d\n", result);
        while (1);
    }
    // We need to read the response into memory. The destination is called a buffer.
    // If you make this buffer static it will be placed in BSS and won't use stack memory.
    static char buffer[4096];
    result = read_response(socket, buffer, sizeof(buffer));
    // Check result
    if (result != 0) {
        printf("Failed to read response: %d\n", result);
        while (1);
    }
    socket->close();
    network->disconnect();
    delete(socket);
     
    // Find the start and end of the JSON data.
    // If the json response is an array you need to replace this with [ and ]
    char* json_begin = strchr(buffer, '{');
    char* json_end = strrchr(buffer, '}');
    // Check if we actually got JSON in the response
    if (json_begin == nullptr || json_end == nullptr) {
        printf("Failed to find JSON in response\n");
        while (1);
    }
    // End the string after the end of the JSON data in case the response contains trailing data
    json_end[1] = 0;
    printf("JSON response:\n");
    printf("%s\n", json_begin);
    // Parse response as JSON, starting from the first {
    json document = json::parse(json_begin);
      
    // Get data from JSON object
    document["weather"][0]["description"].get_to(description);
    document["wind"]["speed"].get_to(speed);
    document["main"]["temp"].get_to(temp);
       
    mail->description = description.c_str();
    mail->windspeed = speed;
    mail->temp = temp;
     
        
    weather_mail.put(mail);
    //eventflags.set(SAMPLE_FLAG3);

    // Since the requirement is to instantiate the clock at 00:00... 00:00:00 UTC on 1 January 1970
    set_time(0);

    lcd.init();

    while (true) 
    {   

        time_t seconds = time(NULL);

        lcd.clear();
        lcd.setCursor(0,0);
        lcd.printf("Time:%s", localtime((&seconds)));
     
        Data *mail = date_time_mail.try_get();

        if ( mail != nullptr ){
            date_time_txt = mail->date_time_txt.c_str();

            delimiter = ",";
            token = date_time_txt.substr(0, date_time_txt.find(delimiter));
            date_time = mail->date_time.c_str();
            date_time_unix = mail->date_time_unix;
           
            set_time(date_time_unix);

            date_time_mail.free(mail);
        }

        Weather_data *weat_mail = weather_mail.try_get();

        if ( weat_mail != nullptr)
        {

            description = weat_mail->description.c_str();

            windspeed = weat_mail->windspeed;
            temp = weat_mail->temp;         

            weather_mail.free(weat_mail);
        }

        if (state_counter == 0)
        {

            char buffer[32];
            strftime(buffer, 32, "%R", localtime(&seconds));

            lcd.clear();
            lcd.setCursor(0,0);
            lcd.printf("Time:%s", buffer);
    
            lcd.setCursor(0,1);
            lcd.printf("Date: %s", date_time.c_str());
            lcd.setCursor(10,0);
            lcd.printf("%s", token.c_str()); 

        }

        if (state_counter == 1)
        {
            char buffer[32];
            strftime(buffer, 32, "%H:%M", localtime(&seconds));
            now = localtime(&seconds);
            

            lcd.clear();
            lcd.setCursor(0,0);
            lcd.printf("Set Alarm:");

            //lcd.setCursor(0,1);
            //printf("%d:%d", current_hours, current_minutes);
                
            if (btn1.read() == 0) 
            {
                interrupt_switch_hour_minutes();
            }
            if (btn2.read() == 0) 
            {
                interrupt_set_timeout();
            }
            if (btn3.read() == 0) 
            {
                Snooze();
                lcd.clear();
                lcd.printf("New time is: %llu", duration_cast<milliseconds>(timeout.remaining_time()/1000).count());
                ThisThread::sleep_for(1s);
            }  

            //The potentiometer is not completely accurate; the range is 1-23 for hour, and 3-58 for minutes on my end. 
            if (hour_minute)
            {
                read_pot = potentiometer.read();
                a_hours = 24 * read_pot;

                lcd.setCursor(0,1);
                lcd.printf("%d:%d", a_hours, a_minutes);
            }
            else
            {
                read_pot = potentiometer.read();
                a_minutes = 60 * read_pot;

                lcd.setCursor(0,1);
                lcd.printf("%d:%d", a_hours, a_minutes);
             }       

            if (timeout_set)
            {
                time_t h_diff = a_hours - now->tm_hour;
                time_t m_diff = a_minutes - now->tm_min;

                time_t extra = (h_diff*3600) + (m_diff*60) + now->tm_sec;

                lcd.clear();
                lcd.printf("Alarm set.");
                lcd.setCursor(0,1);
                lcd.printf("Time left:  %d", extra);

                timeout.attach(&Alarm, extra);
                timeout_set = false;

                ThisThread::sleep_for(1s);
                continue;
            }
        }

        if (state_counter == 2)
        {
            sensor.get_humidity(&humidity);
            sensor.get_temperature(&temperature);

            lcd.clear();
            lcd.setCursor(0,0);
            lcd.printf("[temp] %.2f", temperature);
            lcd.setCursor(0,1);
            lcd.printf("[hum] %.2f", humidity); 
        }

        if (state_counter == 3)
        {

            lcd.clear();
            lcd.setCursor(0,0);
            lcd.printf("%s", description.c_str());
            lcd.setCursor(0,1);
            lcd.printf("T[C]:%f, S[m/s]:%f", temp, windspeed);
        }

        //printf("%d ", state_counter);

        ThisThread::sleep_for(BLINKING_RATE);
    } 
}
