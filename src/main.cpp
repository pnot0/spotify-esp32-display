//essentials libs
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <WebSocketsServer.h>
#include <HTTPClient.h>
#include "SPIFFS.h"

//user and webpage
#include "index.h"
#include "secrets.h"

//image fetch and spiffs
#include "list_spiffs.h"
#include "web_fetch.h"

//graphics libs
#include <TFT_eSPI.h> 
#include <SPI.h>
#include <TJpg_Decoder.h>

String code;
String codeVerifier;

String previousTrack;
String currentTrack;
String currentArtist;
String currentImageUrl;

String remainderProgressSeconds;
String remainderDurationSeconds;

int progressSeconds;
int durationSeconds;
int progressMinutes;
int durationMinutes;

int loadingFailCount;

String acessToken;
String refreshToken;
int expires;
unsigned long lastTime;

WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);
HTTPClient http;

TFT_eSPI tft = TFT_eSPI();

//Static IP to be referenced in spotify callback
IPAddress localIP(192,168,15,3);

//Gateway IP of you localnet
IPAddress gateway(192,168,15,1);

IPAddress subnet(255,255,255,0);
IPAddress primaryDNS(8,8,8,8);
IPAddress secondaryDNS(1,1,1,1);

String redirectUrl = "http%3A%2F%2F"+localIP.toString()+"%2F";

bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap)
{
  if ( y >= tft.height() ) return 0;
  tft.pushImage(x, y, w, h, bitmap);
  return 1;
}


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
  tft.fillScreen(TFT_DARKCYAN);
  tft.setTextColor(TFT_WHITE);  
  tft.setTextSize(2);

  delay(10);
  //Start SPIFFS

  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS initialisation failed!");
    while (1) yield();
  }
  //Setting up for images
  TJpgDec.setJpgScale(2);
  TJpgDec.setSwapBytes(true);
  TJpgDec.setCallback(tft_output);

  //Configure wifi
  if (!WiFi.config(localIP, gateway, subnet, primaryDNS, secondaryDNS)) {
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
  tft.println("Enter the IP");
  tft.println("and connect your Spotify:");
  tft.println(WiFi.localIP().toString());

  server.on("/", sendPage);

  server.begin();
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);

  http.setConnectTimeout(5000);
  http.setTimeout(5000);
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
    refreshToken = tokenDoc["refresh_token"].as<String>();
    expires = tokenDoc["expires_in"].as<int>();
  }
}

void getRefreshToken(String refreshToken){
  http.begin("https://accounts.spotify.com/api/token");
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");

  delay(300);
  String requestBody = "client_id=" + clientID + "&grant_type=refresh_token" + "&refresh_token=" + refreshToken;

  int httpResponseCode = http.POST(requestBody);
  if(httpResponseCode>0 && httpResponseCode == 200){
    const char* responseJson = http.getString().c_str();
    JsonDocument tokenDoc;
    deserializeJson(tokenDoc, responseJson);
    acessToken = tokenDoc["access_token"].as<String>();
    refreshToken = tokenDoc["refresh_token"].as<String>();
    expires = tokenDoc["expires_in"].as<int>();
  }
}

String formatRemainder(int x){
  if(x % 60 < 10){
    return "0" + String(x % 60);
  }else{
    return String(x % 60);
  }
}

void loadImage(){
  bool loaded_ok = getFile(currentImageUrl, "/IMG.jpg");
  if(!loaded_ok && currentTrack != previousTrack && currentTrack != "null"){
    if(loadingFailCount == 4){
      tft.setTextColor(TFT_RED, 0x5AEB);
      tft.setCursor(45,45,2);
      tft.println("img fetch failed");
      tft.setTextColor(TFT_WHITE);
    }else{
      loadingFailCount++;
      SPIFFS.remove("/IMG.jpg"); 
      loadImage();
    }
    delay(1000 * loadingFailCount); 
  }else{
    loadingFailCount = 0;
    tft.fillScreen(0x5AEB);
  }
}

void getCurrentTrack(String acessToken){
  http.begin("https://api.spotify.com/v1/me/player/currently-playing");
  http.addHeader("Authorization","Bearer " + acessToken);
  int httpResponseCode = http.GET();
  if(httpResponseCode>0 && httpResponseCode == 204){
    tft.setTextColor(TFT_RED);
    tft.setCursor(0,0,1);
    tft.setTextSize(3);
    tft.fillScreen(0x5AEB);
    tft.println("no track is  playing");
    delay(500);
  }

  if(httpResponseCode>0 && httpResponseCode == 200){
    const char* responseJson = http.getString().c_str();
    JsonDocument currentTrackDoc;
    deserializeJson(currentTrackDoc, responseJson);

    currentTrack = currentTrackDoc["item"]["name"].as<String>();
    currentArtist = currentTrackDoc["item"]["album"]["artists"][0]["name"].as<String>();
    currentImageUrl = currentTrackDoc["item"]["album"]["images"][1]["url"].as<String>();

    progressSeconds = currentTrackDoc["progress_ms"].as<int>() / 1000;
    durationSeconds = currentTrackDoc["item"]["duration_ms"].as<int>() / 1000;
    progressMinutes = progressSeconds / 60;
    durationMinutes = durationSeconds / 60;
    remainderProgressSeconds = formatRemainder(progressSeconds);
    remainderDurationSeconds = formatRemainder(durationSeconds);

    if(currentTrack != previousTrack && currentTrack != "null"){

      tft.fillScreen(0x5AEB);
      tft.setTextColor(TFT_WHITE);

      tft.setCursor(4, 4, 4);
      tft.println(currentTrack);

      tft.setTextColor(TFT_YELLOW);
      tft.println(currentArtist);  

      tft.setCursor(45,45,2);
      tft.println("loading...");
      tft.drawRect(43,43,153,153,TFT_BLACK);
      loadImage();
      listSPIFFS();
      tft.setTextColor(TFT_WHITE);
      TJpgDec.drawFsJpg(45, 45, "/IMG.jpg");
      SPIFFS.remove("/IMG.jpg"); 

      tft.setCursor(4, 4, 4);
      tft.println(currentTrack);

      tft.setTextColor(TFT_YELLOW);
      tft.println(currentArtist);
      
      tft.setCursor(160, 192, 4);
      tft.setTextColor(TFT_WHITE, TFT_BLACK);  
      tft.println(String(durationMinutes) + ":" + remainderDurationSeconds);

      previousTrack = currentTrack;
    }
    tft.setCursor(16, 192, 4);
    tft.setTextColor(TFT_WHITE, TFT_BLACK); 
    tft.println(String(progressMinutes) + ":" + remainderProgressSeconds);
  }
}

void loop() {
  webSocket.loop();
  server.handleClient();

  if(!code.isEmpty() && !codeVerifier.isEmpty() && acessToken.isEmpty()){
    getToken();
  }

  if(!acessToken.isEmpty()){
    tft.setTextSize(1);
    getCurrentTrack(acessToken);
  }

  if(!refreshToken.isEmpty()){
    if(((millis()/1000) - lastTime) > expires-10){
      getRefreshToken(refreshToken);
      lastTime = millis()/1000;
    }
  }
}