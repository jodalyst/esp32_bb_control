from collections import OrderedDict
import random
import math
import requests
import sys
import glob
import serial
import json
import struct
import os
import time
# import matplotlib.pyplot as plt
import sys
import time
import requests
import math
from threading import Thread, Lock
from flask import Flask, render_template, session, request
from flask_socketio import SocketIO, emit, join_room, leave_room,close_room, rooms, disconnect
from datetime import datetime
import serial.tools.list_ports



#lcl = '/home/pi/bb_deployment' #change if on another system
lcl = '~/bb_deployment'
hal = 'https://eesjs1.net/testsite/bb_logger'


#time.sleep(25)##sleep at very start to get loaded up for network purposes!

'''Automatically find Teensy on COM Port
    Written by Joe Steinmeyer, 2017
    Edited by Joel Voldman, 2017
'''

'''
def get_teensy_port():
    teensy_port = list(serial.tools.list_ports.grep("16c0"))
    if len(teensy_port) == 1:
        print("Automatically found Teensy: {}".format(teensy_port[0]))
        return teensy_port[0].device
    else:
        ports = list(serial.tools.list_ports.comports())
        port_dict = {i:[ports[i],ports[i].vid] for i in range(len(ports))}
        teensy_id=None
        for p in port_dict:
            print("{}:   {} (Vendor ID: {})".format(p,port_dict[p][0],port_dict[p][1]))
            if port_dict[p][1]==5824:
                teensy_id = p
        if teensy_id== None:
            return False
        else:
            print("Teensy Found: Device {}".format(p))
            return port_dict[teensy_id][0].device
'''

def get_teensy_port():
    teensy_port = list(serial.tools.list_ports.grep("16c0"))
    if len(teensy_port) == 1:
        print("Automatically found Teensy: {}".format(teensy_port[0]))
        return teensy_port[0][0]
    else:
        ports = list(serial.tools.list_ports.comports())
        port_dict = {i:[ports[i],ports[i].vid] for i in range(len(ports))}
        teensy_id=None
        for p in port_dict:
            print("{}:   {} (Vendor ID: {})".format(p,port_dict[p][0],port_dict[p][1]))
            if port_dict[p][1]==5824:
                teensy_id = p
        if teensy_id== None:
            return False
        else:
            print("Teensy Found: Device {}".format(p))
            return port_dict[teensy_id][0][0]

def get_esp_port():
    teensy_port = list(serial.tools.list_ports.grep(""))
    if len(teensy_port) == 1:
        print("Automatically found ESP32: {}".format(teensy_port[0]))
        return teensy_port[0][0]
    else:
        ports = list(serial.tools.list_ports.comports())
        port_dict = {i:[ports[i],ports[i].vid] for i in range(len(ports))}
        teensy_id=None
        for p in port_dict:
            print("{}:   {} (Vendor ID: {})".format(p,port_dict[p][0],port_dict[p][1]))
            if port_dict[p][1]==4292:
                teensy_id = p
        if teensy_id== None:
            return False
        else:
            print("ESP32: Device {}".format(p))
            return port_dict[teensy_id][0][0]

'''
#Then connect as usual!
#I have a few different serial port threading objects you can use that are pretty robust!jj
'''




s = 5 #create object to hold the s serial object

unique = 456
ser = get_esp_port()
if not ser:
    print("No Microcontroller Found!")
s = serial.Serial(ser) #auto-connects already I guess?
print(s)
print("Serial Connected!")
old_time = time.time()
while True:
    print("Loop Time: {}".format(time.time()-old_time))
    old_time = time.time() #update time
    #current = datetime.now().isoformat()
    #current = current.replace(":","_")
    #string_to_write = input() #7,3;single\n" 
    string_to_write = "all*"
    s.write(bytes(string_to_write,'UTF-8'))
    #time.sleep(4)   #time running in the arduino code. Modify if needed
    no_more_data = False
    #this is a serious cludge:
    all_data = ""
    while not no_more_data:
        time.sleep(0.1)
        data_left = s.inWaiting()
        if (data_left >0): 
            all_data += s.read(data_left).decode()
        else:
            no_more_data = True
    x = all_data
    x = x.split("&")
    x = x[:-1]
    bins=[]
    for y in x:
        parts = y.split(":")
        bins.append((int(parts[0]),int(parts[1])))
    uname,password = [i.strip() for i in open(os.path.expanduser(lcl+'/bb_login'))]
    print(uname)
    payload = {'user':uname,'board':str(bins)}
    try:
        r = requests.post(hal, json=payload, timeout=1.0)
        print(r.status_code)
    except:
        print("issue happened: {}".format(r.status_code))
    time.sleep(5)


