
#include <WiFi.h>
#include "index.h"
#include <WebServer.h>
#include <WebSocketsServer.h>

const char *ssid = "CasaNucci";
const char *password = "CASANUCCI171";
String codes[2];

String code = "";

WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);

//Static IP to be referenced in spotify callback
IPAddress localIP(192,168,15,3);

//Gateway IP of you localnet
IPAddress gateway(192,168,15,1);

IPAddress subnet(255,255,0,0);
IPAddress primaryDNS(8,8,8,8);

void sendPage(){
  server.send(200, "text/html", INDEX);
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t welength){
  if(type == WStype_TEXT){
      String payloadString = (const char *)payload;
      if(payloadString[0] == '0'){
        payloadString.remove(0,1);
        Serial.println("this is the code "+ payloadString + "\n");
      }else{
        Serial.println("this is the verifier "+ payloadString);
      }
  }
  
  //This gets the code verifier and code, the basis for the token
}

void setup()
{
  Serial.begin(9600);
  pinMode(5, OUTPUT);  // set the LED pin mode

  delay(10);

  // We start by connecting to a WiFi network

  //Configure wifi
  if (!WiFi.config(localIP, gateway, subnet, primaryDNS)) {
    Serial.println("STA Failed to configure");
  }

  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  server.on("/", sendPage);

  server.begin();
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
}

void loop() {
  webSocket.loop();
  server.handleClient();
}
