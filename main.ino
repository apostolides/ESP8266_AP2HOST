#include <Arduino.h> 
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include "LittleFS.h"
#include "index.h"

// Sample Access Point name and password.
const char* ap_name = "ESP8266AP";
const char* ap_pass = "mypassword12345";

int tries = 0;
// Max WiFi connection attempts with a pair of credentials.
int max_tries = 6;

boolean ap_mode = true;

String wifi_ssid;
String wifi_pass;

// ESP8266 Web Server serving a sample html form to retrive WiFi credentials.
ESP8266WebServer server(80);

void setup(){
  Serial.begin(115200);
  
  pinMode(2,OUTPUT); // Built-in led will blink only if device is connected on desired network as an indicator that WiFi is working. 
  
  delay(3000);

  if(!LittleFS.begin()){ 
    Serial.println("An Error has occurred while mounting LittleFS");  
  }
  
  // Check if WiFi credentials exist. If not serve as an access point to retrieve them.
  File ssid_file = LittleFS.open("SSID","r");
  File pass_file = LittleFS.open("PASS", "r");
  if(ssid_file && pass_file){
    // Found some credentials, try them.
    wifi_ssid = ssid_file.readStringUntil('\0');
    wifi_pass = pass_file.readStringUntil('\0');
    // Connect to WiFi.
    WiFi.begin(wifi_ssid.c_str(), wifi_pass.c_str());
    while(tries < max_tries){
      tries++;
      // Connected to WiFi, device will not act as an access point. 
      if(WiFi.status() == WL_CONNECTED){
        ap_mode = false;
        break;
      }
      delay(1000);
    }
  }
   ssid_file.close();
   pass_file.close();
  
  // Device will act as an access point to retrieve WiFi credentials.
  if(ap_mode){
    WiFi.softAP(ap_name,ap_pass);
    delay(1000);
    IPAddress IP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(IP);
    delay(1000);

    // Define server endpoints.
    server.on("/", serve_page);
    server.on("/post",HTTP_POST,handle_post);
    // Serve webpages.
    server.begin();
  }
}

// Access Point Server Endpoints

// Index Page Callback.
void serve_page(){
  server.send(200,"text/html",index_html);
}

// POST Request Callback.
void handle_post(){
    String body = server.arg("plain");
    int delimeter = body.lastIndexOf('-');
    wifi_ssid = body.substring(0,delimeter);
    wifi_pass = body.substring(delimeter+1,body.length());

    WiFi.begin(wifi_ssid.c_str(), wifi_pass.c_str());
    delay(1000);
    tries = -2;
    while(tries < max_tries){
      tries++;
      if(WiFi.status()==WL_CONNECTED){
        // Connected on WiFi, save credentials to filesystem.
        File file = LittleFS.open("SSID","w");
        file.print(wifi_ssid);
        file.close();
        file = LittleFS.open("PASS","w");
        file.print(wifi_pass);
        file.close();
        
        WiFi.softAPdisconnect(true);
        
        server.send(200);    
        ap_mode = false;

        return;
      }
      delay(1000);
    }
    server.send(401);
}

void loop(){
  if(ap_mode){ // If still acting as an Access Point.
    // Handle clients on "access point" web server.
    server.handleClient();
  }
  else{ // WiFi connection is established.
    // Normal Arduino Loop code goes here.
    digitalWrite(2,HIGH);
    delay(1000);
    digitalWrite(2,LOW);
    delay(1000);
  }
}
