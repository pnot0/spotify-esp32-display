
//essentials
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <WebSocketsServer.h>
#include <HTTPClient.h>

//user and webpage
#include "index.h"
#include "secrets.h"

//graphics
#include <TFT_eSPI.h> 
#include <SPI.h>

//need to fork for git a specific version to upload so it doesnt include private shit

String code;
String codeVerifier;

String previousTrack;
String currentTrack;

String remainderProgressSeconds;
String remainderDurationSeconds;

int progressSeconds;
int durationSeconds;
int progressMinutes;
int durationMinutes;

String acessToken;
String refreshToken;
String expires;

WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);
HTTPClient http;
TFT_eSPI tft = TFT_eSPI();

//Static IP to be referenced in spotify callback
IPAddress localIP(192,168,15,3);

//Gateway IP of you localnet
IPAddress gateway(192,168,15,1);

IPAddress subnet(255,255,0,0);
IPAddress primaryDNS(8,8,8,8);

String redirectUrl = "http%3A%2F%2F"+localIP.toString()+"%2F";

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
  tft.init();
  tft.setRotation(3);
  tft.fillScreen(0x5AEB);

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

  tft.setCursor(0, 0, 2);
  tft.setTextColor(TFT_WHITE,TFT_BLACK);  
  tft.setTextSize(2);
  tft.println("Enter the IP");
  tft.println("and connect your Spotify:");
  tft.println(WiFi.localIP().toString());

  server.on("/", sendPage);

  server.begin();
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
}

void getToken(){
  http.begin("https://accounts.spotify.com/api/token");
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  
  delay(300);
  String requestBody = "client_id=" + clientID + "&grant_type=authorization_code" + "&code=" + code + "&redirect_uri=" + redirectUrl + "&code_verifier=" + codeVerifier;

  int httpResponseCode = http.POST(requestBody);
  if(httpResponseCode>0 && httpResponseCode == 200){
    const char* responseJson = http.getString().c_str();
    JsonDocument tokenDoc;
    deserializeJson(tokenDoc, responseJson);
    acessToken = tokenDoc["access_token"].as<String>();
    /*
    Serial.println("token: " + tokenDoc["access_token"].as<String>());
    Serial.println("refresh token: " + tokenDoc["refresh_token"].as<String>());
    Serial.println("expiration: " + tokenDoc["expires_in"].as<String>());
    */
  }
}

String formatRemainder(int x){
  if(x % 60 < 10){
    return "0" + String(x % 60);
  }else{
    return String(x % 60);
  }
}

void getCurrentTrack(String acessToken){
  http.begin("https://api.spotify.com/v1/me/player/currently-playing");
  http.addHeader("Authorization","Bearer " + acessToken);
  int httpResponseCode = http.GET();
  if(httpResponseCode>0 && httpResponseCode == 200){
    const char* responseJson = http.getString().c_str();
    JsonDocument currentTrackDoc;
    deserializeJson(currentTrackDoc, responseJson);

    currentTrack = currentTrackDoc["item"]["name"].as<String>();
    progressSeconds = currentTrackDoc["progress_ms"].as<int>() / 1000;
    durationSeconds = currentTrackDoc["item"]["duration_ms"].as<int>() / 1000;
    progressMinutes = progressSeconds / 60;
    durationMinutes = durationSeconds / 60;
    remainderProgressSeconds = formatRemainder(progressSeconds);
    remainderDurationSeconds = formatRemainder(durationSeconds);

    if(currentTrack != previousTrack){
      tft.setCursor(0, 0, 2);
      tft.fillScreen(0x5AEB);
      tft.println(currentTrack);
      previousTrack = currentTrack;
    }
    tft.setCursor(0, 96, 2);
    tft.println(String(progressMinutes) + ":" + remainderProgressSeconds);

    tft.setCursor(64, 96, 2);
    tft.println("/ "+String(durationMinutes) + ":" + remainderDurationSeconds);


  }
}

void loop() {
  webSocket.loop();
  server.handleClient();

  if(!code.isEmpty() && !codeVerifier.isEmpty() && acessToken.isEmpty()){
    getToken();
  }

  if(!acessToken.isEmpty()){
    getCurrentTrack(acessToken);
  }
}
