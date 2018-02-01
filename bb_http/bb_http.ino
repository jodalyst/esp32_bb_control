#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <WiFiMulti.h>

#include <time.h>

#define SERIAL_BAUD       115200
#define WIFI_DELAY        500
#define MAX_SSID_LEN      32
#define MAX_CONNECT_TIME  30000

char ssid[MAX_SSID_LEN] = "";

WiFiMulti wm;
//clock_t currentTime = clock();

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

  //Connect to Wifi if not connnected
  if (wm.run() != WL_CONNECTED) {
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
      //Serial.println("6s08");

      WiFi.begin(ssid);
      wm.addAP(ssid, "none");
      //wm.addAP("6s08", "REDACTED");
      unsigned short try_cnt = 0;

      Serial.print("wm run: "); Serial.println(wm.run());
      Serial.print("");

      while (wm.run() != WL_CONNECTED && try_cnt < MAX_CONNECT_TIME / WIFI_DELAY) {
        delay(WIFI_DELAY);
        Serial.print(".");
        try_cnt++;
      }
      Serial.println("");
      Serial.println("WiFi connected");
      Serial.println("IP address: ");
      Serial.println(WiFi.localIP());
    }

    delay(3000);
  }
  //Serial.println("Error in WiFi connection");
  //delay(3000);

  else {
    Serial.println("Cannot establish connection on given time.");
    delay(3000);
  }

  while (wm.run() == WL_CONNECTED) { //&& clock()-currentTime >= 2000){

    HTTPClient http;
    WiFiClient client;
    String op = "";
    //int taskdone = 0;
    String response = "";

    if (!client.connect("iesc-s3.mit.edu", 80)) {
      Serial.println("connection to server failed");
    }
    else {

      client.println("GET http://iesc-s3.mit.edu/breadboard/deviceInquiry?dev_id=123 HTTP/1.1");
      client.println("Host: iesc-s3.mit.edu");
      client.println("\r\n\r\n");

      unsigned long count = millis();
      while (client.connected()) {
        String line = client.readStringUntil('\n');
        Serial.print(line);
        if (line == "\r") {
          Serial.println("headers received");
          break;
        }
        if (millis() - count > 4000) break;
      }
      Serial.print("-----------");
      Serial.print("\n");
      Serial.print("Response...");
      count = millis();
      while (!client.available()) {
        delay(100);
        Serial.print(".");
        if (millis() - count > 4000) break;
      }
      Serial.println();
      //String op;
      while (client.available()) {
        op += (char)client.read();
      }
      Serial.println(op);
      client.stop();
      Serial.println("closing connection");
    }

    if (op.indexOf("No") != -1) {
      //pass
      Serial.println("No Task");
      delay(2000);
    }
    else {

      int current_index = 0;
      int star_index = 0;
      String task_id = op.substring(0, op.indexOf("&"));//.toInt();
      String task = op.substring(op.indexOf("&") + 1);
      String node;
      String sampleRate;
      String numSamples;
      /*
         Format of task(s):

         ALL
         SINGLE*node*sampleRate*#ofSamples
         ALLMULTI*sampleRate*#ofSamples
      */

      Serial.print("Task ID: ");
      Serial.println(task_id);
      //Serial.println("raw task id: "+op.substring(0, op.indexOf("&")));
      Serial.print("");
      Serial.print("Task: ");
      Serial.println(task);

      if (task == "ALL") {
        //POST
//        client.println("POST http://iesc-s3.mit.edu/breadboard/reportValues HTTP/1.1");
//        client.println("Host: iesc-s3.mit.edu");
//        client.println("Content-Type: application/x-www-form-urlencoded");
//        client.println("");
//        client.println("value=abc&dev_id=123&task_id=123123123");
//        client.println("\r\n\r\n");

        response = "A:1,2,3";
      }
      else if (task.indexOf("SINGLE") != -1) {
        //try{
        star_index = task.indexOf("*");
        node = task.substring(star_index + 1, task.indexOf("*", star_index + 1));
        star_index = task.indexOf("*", star_index + 1);
        sampleRate = task.substring(star_index + 1, task.indexOf("*", star_index + 1));
        star_index = task.indexOf("*", star_index + 1);
        numSamples = task.substring(star_index + 1);

        Serial.print("Node: "); Serial.println(node.toInt());
        Serial.print("Sample rate: "); Serial.println(sampleRate.toInt());
        Serial.print("Number of Samples: "); Serial.println(numSamples.toInt());
        Serial.print("SINGLE");

        response = "S:4,5,6";
        //              }
        //              catch(Exception e){
        //                Serial.println("Syntax error");
        //              }
      }
      else if (task.indexOf("MULTI") != -1) {
        star_index = task.indexOf("*");
        sampleRate = task.substring(star_index + 1, task.indexOf("*", star_index + 1));
        star_index = task.indexOf("*", star_index + 1);
        numSamples = task.substring(star_index + 1);

        Serial.print("Sample rate: "); Serial.println(sampleRate.toInt());
        Serial.print("Number of Samples: "); Serial.println(numSamples.toInt());
        Serial.print("ALLMULTI");

        response = "M:7,8,9";
      }
      else {
        Serial.print("Task ID/Task not recognized");
      }

      if (response.length() > 0) {
        //breadboard/deviceInquiry?dev_id=123

        String dict = "{\"dev_id\": \"123\", \"task_id\": \""+task_id+"\", \"values\": \""+response+"\"}";


        http.begin("http://iesc-s3.mit.edu/breadboard/reportValues");
        http.addHeader("Content-Type", "application/json");
        //http.POST("{\"dev_id\": \"123\", \"task_id\": \"151742017254111\", \"values\": \"A:1,2,3\"}");
        http.POST(dict);
//        client.println("POST http://iesc-s3.mit.edu/breadboard/reportValues HTTP/1.1");
//        client.println("Host: iesc-s3.mit.edu");
//        client.println("Content-Type: application/json");
//        client.println();

//        client.println("{\"dev_id: \"123\", \"task_id\": \"");
//        client.println(task_id); client.println("\", \"values\": \"");
//        client.println(response); client.println("\"}");
        //client.println("{\"dev_id\": \"123\", \"task_id\": \"151742017254111\", \"values\": \"A:1,2,3\"}");
        //client.println(dict);
        //client.println("\r\n\r\n");

        op = "";
        op = http.getString();
        Serial.print(dict);
        Serial.print("\nPost Response: "); Serial.println(op);

        /*unsigned long count = millis();
        while (client.connected()) {
          String line = client.readStringUntil('\n');
          Serial.print(line);
          if (line == "\r") {
            Serial.println("headers received");
            break;
          }
          if (millis() - count > 4000) break;
        }
        Serial.print("\n");
        Serial.print("POST Response...");
        count = millis();
        while (!client.available()) {
          delay(100);
          Serial.print(".");
          if (millis() - count > 4000) break;
        }
        Serial.println();
        //String op;
        while (client.available()) {
          op += (char)client.read();
        }
        Serial.print(op);
        client.stop();
        Serial.print("closing connection \n");*/

        if (op.length() > 0){
          Serial.print("\nsomething posted");
          if (op.indexOf("SUCCESS") != -1)
            Serial.print("\nPOST completed successfully!");
        }
        else{
          Serial.print("error in POST\n");
        }
        }
      }
    }
    //currentTime = clock();
}
/*else {
  Serial.println("No non-encrypted WiFi found.");
  } */

//}
