
#include <WiFi.h>
#include <ArduinoJson.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <HTTPClient.h>

#include "index.h"
#include "secrets.h"

//const char *ssid = "your ssid";
//const char *password = "your pass";

String clientID = "f8c8cb559b5245328a674b2e1771478e";

String code;
String codeVerifier;

String acessToken;
String refreshToken;
String expiresIn;
String expires;

WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);

//Static IP to be referenced in spotify callback
IPAddress localIP(192,168,15,3);

//Gateway IP of you localnet
IPAddress gateway(192,168,15,1);

IPAddress subnet(255,255,0,0);
IPAddress primaryDNS(8,8,8,8);

String redirectUrl = "http%3A%2F%2F192.168.15.3%2F";

void sendPage(){
  server.send(200, "text/html", INDEX);
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t welength){
  if(type == WStype_TEXT){
      String payloadString = (const char *)payload;
      if(payloadString[0] == '0'){
        payloadString.remove(0,1);
        code = payloadString;
      }else{
        codeVerifier = payloadString;
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
  if(!code.isEmpty() && !codeVerifier.isEmpty()){
    //We have code, now to act
    HTTPClient http;
    //JsonDocument doc;
       
    http.begin("https://accounts.spotify.com/api/token");
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    /*
    doc["client_id"] = clientID;
    doc["grant_type"] = "authorization_code";
    doc["code"] = code;
    doc["redirect_uri"] = redirectUrl;
    doc["code_verifier"] = codeVerifier;
    */

    delay(300);
    String requestBody = "client_id=" + clientID + "&grant_type=authorization_code" + "&code=" + code + "&redirect_uri=" + redirectUrl + "&code_verifier=" + codeVerifier;

    //Serial.println(requestBody);
    int httpResponseCode = http.POST(requestBody);
    if(httpResponseCode>0){
      String response = http.getString();                       
       
      Serial.println(httpResponseCode);   
      Serial.println(response);
    }
  }
}
