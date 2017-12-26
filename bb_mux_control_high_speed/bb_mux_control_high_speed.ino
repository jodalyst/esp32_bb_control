#include <stdio.h>
#include <string.h>
#include <ADC.h>

#define DEBUG true
#define write_buffer_size 6000

#define SEQUENTIAL_LIMIT 1000

//order: S0,S1,S2,S3
//This code is meant for the Teensy, but will be ported to the ESP32 as needed
//Updated last on 12/25/2017 jds


//will hold built-up-string for Serial writing.
char write_buffer[write_buffer_size];

int innerPins[] = {12,11,10,9};
int outerPins[] = {8,7,6,5};
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

//operation variables:
int type; //current type of reading being made (single node or all)
int count; //how many samples to take
int sampling_period; //the period of sampling in microseconds

ADC *adc = new ADC(); // adc object 

String commandString;
boolean stringComplete = false;


uint16_t meas[129];

String upstreamStr;  //String to send up
String downstreamStr; //string coming down from computer
boolean complete = false;
int low_val = 0;
int high_val = 0;

void setup() {
  meas[128]=0xFFFF; //initialize ending value
  for (int i = 0; i < 4; i++)
  {
    pinMode(innerPins[i],OUTPUT);
    pinMode(outerPins[i],OUTPUT);
    digitalWrite(innerPins[i],LOW);
    digitalWrite(outerPins[i],LOW);
  }
  pinMode(EN_pin,OUTPUT);  //EN pin
  digitalWrite(EN_pin,LOW); //EN pin 
  Serial.begin(115200); //set to 115200 since what the ESP32 will like
  commandString.reserve(200);
}

void loop() {  
  //simple command query checking
  if (complete){
    Serial.println(upstreamStr); //send message up to host computer
    complete = false;
  }
  serialEvent(); //check if an event occurred
  if (stringComplete){ //if it did happen
    process_string(commandString); //figure out what you need to do.
    Serial.print(type);Serial.print(" ");Serial.print(count);Serial.print(" ");Serial.println(sampling_period);
    calc_and_report(); 
    commandString = "";
    stringComplete = false;
  }
}


void set_inner_channel(int channel){
  for (int j = 0; j < 4; j++) digitalWrite(innerPins[j],myChannel[channel][j]);
}

void set_outer_channel(int channel){
    for (int j = 0; j < 4; j++) digitalWrite(outerPins[j],myChannel[channel][j]);
}

void serialEvent() {
  String temp;
  while (Serial.available()) {
    char inChar = (char)Serial.read();
    if (inChar == '*') {
      stringComplete = true;
      if(DEBUG) Serial.println("found");
      break;
    }
    else{
      commandString += inChar;
    }
  }
}
//Two types of incoming commands:
//all,count,sampling_period
//pin,count,sampling_period
bool process_string(String pro_String){
  type = -1; //set to -1 as sentinel for "all" message
  int current_index = 0;
  int next_index = pro_String.indexOf(",",current_index);
  if (next_index==-1 && DEBUG){
    Serial.println("incompatible string. Needs commas");
    return false;
  }
  String temp = pro_String.substring(current_index,next_index);
  if (temp!="all") type = temp.toInt();
  current_index = next_index;
  int next_index = pro_String.indexOf(",",current_index+1);
  if (next_index==-1 && DEBUG){
    Serial.println("incompatible string. Needs commas");
    return false;
  }
  count = pro_String.substring(current_index+1,next_index).toInt();
  if(count>SEQUENTIAL_LIMIT) count=SEQUENTIAL_INPUT;
  sampling_period = pro_String.substring(next_index+1).toInt();
  return true;
}




void full_read(){
    sprintf(write_buffer,"[");
    int iter_count = 0;
    for (int cm = 0; cm < 8; cm++){ //Read channel 0-7 of the inner mux
        set_inner_channel(cm);
        for (int om = 0; om < 16; om++){ 
            set_outer_channel(om);
        }
        sprintf(write_buffer+strlen(write_buffer),"%d%s",adc->analogRead(SIG_pin),iter_count<127?",":"");
        iter_count++;
    }
    sprintf(write_buffer+strlen(write_buffer),"]");
    Serial.println(write_buffer);
}

bool calc_and_report(){
    if (type==-1){//all reading
        unsigned long timeo;
        for (int c=0; c<count;c++){
            full_read();
            while (micros()-timeo<period); //delay
            time=micros();
        }
    }else{  //pin specific reading!
        int inner_mux_channel = pin/16;
        int outer_mux_channel = pin - (inner_mux_channel*16);
        unsigned long timeo;
        set_inner_channel(inner_mux_channel);//got to fix that.
        set_outer_channel(outer_mux_channel); 
        sprintf(write_buffer,"[");
        timeo = micros();
        for(int i=0; i<count; i++){
            sprintf(write_buffer+strlen(write_buffer),"%d%s",adc->analogRead(SIG_pin),i<count-1?",":"");
            while (micros()-timeo<period); //delay
            time=micros();
        }
        sprintf(write_buffer+strlen(write_Buffer),"]");
        Serial.println(write_buffer);
    }
}
