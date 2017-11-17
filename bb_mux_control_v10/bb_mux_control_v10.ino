//order: S0,S1,S2,S3
int centralPins[] = {12,11,10,9};
int auxPins[] = {8,7,6,5};
int SIG_pin = A0;
int EN_pin = 4;
int mode;
int values[50];
int average;
int sum;

int myChannel[16][4] = {
  {0,0,0,0}, //channel 0
  {1,0,0,0}, //channel 1
  {0,1,0,0}, //channel 2
  {1,1,0,0}, //channel 3
  {0,0,1,0}, //channel 4
  {1,0,1,0}, //channel 5
  {0,1,1,0}, //channel 6
  {1,1,1,0}, //channel 7
  {0,0,0,1}, //channel 8 
  {1,0,0,1}, //channel 9
  {0,1,0,1}, //channel 10
  {1,1,0,1}, //channel 11
  {0,0,1,1}, //channel 12
  {1,0,1,1}, //channel 13
  {0,1,1,1}, //channel 14
  {1,1,1,1}  //channel 15
};

String commandString;
boolean stringComplete = false;

String upstreamStr;
boolean complete = false;
int lRange = 0;
int hRange = 0;

void setup() {
  for (int i = 0; i < 4; i++)
  {
    pinMode(centralPins[i],OUTPUT);
    pinMode(auxPins[i],OUTPUT);
    digitalWrite(centralPins[i],LOW);
    digitalWrite(auxPins[i],LOW);
  }
  pinMode(EN_pin,OUTPUT);  //EN pin
  digitalWrite(EN_pin,LOW); //EN pin 
  Serial.begin(9600);
  commandString.reserve(200);
}

void loop() {  
  if (complete == true){
    Serial.println(upstreamStr);
    complete = false;
  }
  serialEvent();
  if (stringComplete){
    processString(commandString);
    upstreamStr = "";     //empty the sent string so no addition information will be overlap
    Calculation(mode,lRange,hRange); 
    lRange = 0;
    hRange = 0;
    commandString = "";
    stringComplete = false;
  }
}

int readChannel(int channel){
  for (int j = 0; j < 4; j++){
    digitalWrite(centralPins[j],myChannel[channel][j]);
  }
  int SIG_value = analogRead(SIG_pin);
  return SIG_value;
}

void serialEvent() {
  String temp;
  while (Serial.available()) {
    char inChar = (char)Serial.read();
    if (inChar == '*') {
      stringComplete = true;
      break;
    }
    else{
      commandString += inChar;
    }
  }
}

void processString(String pro_String){
  if (pro_String.equalsIgnoreCase("many")){
    mode = 5;
  }
  else if (pro_String.equalsIgnoreCase("all")){
    mode = 1;
  }else if (pro_String.equalsIgnoreCase("off")){
      digitalWrite(EN_pin,HIGH);
  }else if (pro_String.equalsIgnoreCase("on")){
     digitalWrite(EN_pin,LOW);
  }else{
      String temp;
      for (int h = 0; h < commandString.length(); h++){
        if (commandString[h] == ','){
          lRange = hRange;
          temp = ""; //empty    
        }else{
          temp += commandString[h];
          hRange = (temp.toInt());
        }
      }
      mode = 2;
  }
}

void Calculation(int mode1, int low, int high){
    int pin = low;
    if (mode1 == 1){
      //Serial.println("Hey I'm here");
      for (int i = 0; i < 8; i++){ //Read channel 0-7 of the central mux
        for (int j = 0; j < 16; j++){
          for (int sel = 0; sel < 4; sel++){ //Control the channel of the connecting mux
            digitalWrite(auxPins[sel],myChannel[j][sel]);
          }
          //delay(10);
          for (int k = 0; k < 50; k++){ //Take reading of the value 50 times and take the average
            values[k] = (readChannel(i));
            sum += values[k];
          }
          average = sum/50;
          sum = 0; //clear the sum
          upstreamStr += pin;
          upstreamStr += ":";
          upstreamStr += average;
          upstreamStr += "&";       
          pin++;    
        }
      }
    }
    //high is the sampling rate
    //low is the index of the node
    if (mode1 == 2){
      int central_mux_channel, aux_mux_channel,delay_step;
      int duration = 2000000;    //desired time to wait to acquire data in us
      delay_step = (1000000/high);    //(1/f_sampling)
      int numberOfSamples = duration/delay_step;   //desired # of samples
      elapsedMicros delay_modifier;    //keeping track of the delay
      central_mux_channel = low/16;
      aux_mux_channel = low - (central_mux_channel*16);
      for (int sel = 0; sel < 4; sel++){ //Control the channel of the connecting mux
         digitalWrite(auxPins[sel],myChannel[aux_mux_channel][sel]);
      }
      int sampleI = 0;
      while (sampleI < numberOfSamples){
        while (delay_modifier >= delay_step){
          average = readChannel(central_mux_channel);
          upstreamStr += low;
          upstreamStr += ":";
          upstreamStr += average;
          upstreamStr += "&";
          sampleI += 1;
          delay_modifier = 0;
        }
      }      
    }
    complete = true; 
}

