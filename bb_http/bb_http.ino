#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

#include <WiFiMulti.h>

#define SERIAL_BAUD       115200
#define WIFI_DELAY        500
#define MAX_SSID_LEN      32
#define MAX_CONNECT_TIME  30000

char ssid[MAX_SSID_LEN] = "";

WiFiMulti wm;

/* Scan available networks and sort them in order to their signal strength. */
void scanAndSort() {
  memset(ssid, 0, MAX_SSID_LEN);
  int n = WiFi.scanNetworks(); //Scans the network for all WiFi
  Serial.println("Scan done!");
  if (n == 0) {
    Serial.println("No networks found!");
  } else {
    Serial.print(n);
    Serial.println(" networks found.");
    int indices[n];
    for (int i = 0; i < n; i++) {
      indices[i] = i;
    }
    for (int i = 0; i < n; i++) {
      for (int j = i + 1; j < n; j++) {

        //Sorts based on signal strengths
        if (WiFi.RSSI(indices[j]) > WiFi.RSSI(indices[i])) {
          std::swap(indices[i], indices[j]);
        }
      }
    }
    for (int i = 0; i < n; ++i) {
      Serial.print(WiFi.SSID(indices[i]));
      Serial.print(" ");
      Serial.print(WiFi.RSSI(indices[i]));
      Serial.print(" ");
      Serial.print(WiFi.encryptionType(indices[i]));
      Serial.println();

      //Chooses open WiFi from the sorted list
      if (WiFi.encryptionType(indices[i]) == WIFI_AUTH_OPEN) {
        Serial.println("Found non-encrypted network. Store it and exit to connect.");
        memset(ssid, 0, MAX_SSID_LEN);
        strncpy(ssid, WiFi.SSID(indices[i]).c_str(), MAX_SSID_LEN);
        break;
      }
    }
  }
}

void setup() {
  Serial.begin(SERIAL_BAUD);
  Serial.println("Started.");
}

void loop() {

  //if (WiFi.status() != WL_CONNECTED) {
  if (wm.run() != WL_CONNECTED) {
  //while (wm.run() != WL_CONNECTED) {
    WiFi.softAPdisconnect();
    WiFi.disconnect();
    WiFi.mode(WIFI_STA);
    delay(WIFI_DELAY);

    scanAndSort();
    delay(WIFI_DELAY);

    //Connects to the first WiFi network in the sorted list
    if (strlen(ssid) > 0) {
      Serial.print("Going to connect for : ");
      Serial.println(ssid);

      WiFi.begin(ssid); //renee
      wm.addAP(ssid, "none"); //renee
      unsigned short try_cnt = 0;

      Serial.println("wm run: "+wm.run());
      Serial.println();

      //while (WiFi.status() != WL_CONNECTED && try_cnt < MAX_CONNECT_TIME / WIFI_DELAY) {
      while (wm.run() != WL_CONNECTED && try_cnt < MAX_CONNECT_TIME / WIFI_DELAY) {
        delay(WIFI_DELAY);
        Serial.print(".");
        try_cnt++;
      }
      //if (WiFi.status() == WL_CONNECTED) {
      //if (wm.run() == WL_CONNECTED) {
      while (wm.run() == WL_CONNECTED) {
        Serial.println("");
        Serial.println("WiFi connected");
        Serial.println("IP address: ");
        Serial.println(WiFi.localIP());

        HTTPClient http;
        // Performs GET request to ask for any incomplete tasks
        http.begin("http://iesc-s3.mit.edu/breadboard/deviceInquiry?dev_id=123");
        int httpCode = http.GET();
        http.end();
        if (httpCode > 0) {

          String payload = http.getString();
          Serial.println(payload);

          // }

          //   http.end();
          int current_index = 0;
          int new_index = payload.indexOf("&", current_index);
          int task_id = payload.substring(current_index, new_index).toInt();
          String task = payload.substring(new_index);

          if (task == "ALL") //Understands the type of request
          {
            StaticJsonBuffer<300> JSONbuffer;
            JsonObject& JSONencoder = JSONbuffer.createObject();

            JSONencoder["sensorType"] = "Temperature";

            JsonArray& values = JSONencoder.createNestedArray("values");
            values.add(20);
            values.add(21);
            values.add(23);

            JsonArray& timestamps = JSONencoder.createNestedArray("timestamps");
            timestamps.add("10:10");
            timestamps.add("10:20");
            timestamps.add("10:30");

            char JSONmessageBuffer[300];
            JSONencoder.prettyPrintTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));
            Serial.println(JSONmessageBuffer);

            HTTPClient http;

            //POSTs to the specified URL the data from the calculations
            http.begin("http://iesc-s3.mit.edu/breadboard/reportValues");
            http.addHeader("Content-Type", "application/json");

            int httpCode = http.POST(JSONmessageBuffer);
            String payload = http.getString();

            Serial.println("httpCode: ");
            Serial.println(httpCode);
            Serial.println("payLoad: ");
            Serial.println(payload);

            http.end();
          }
        }

        delay(3000);
      } //else {

        Serial.println("Error in WiFi connection");

      //}

      delay(3000);
    }

    else {
      Serial.println("Cannot established connection on given time.");
    }
  } else {
    Serial.println("No non-encrypted WiFi found.");
  }
}
