from collections import OrderedDict
from bokeh.plotting import figure, show, output_file, ColumnDataSource
from bokeh.models import HoverTool
from bokeh.embed import components
import bokeh
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



lcl = '~/Smart-Breadboard/ws2'
hal = 'https://eesjs1.net/testsite/bb_logger'


'''Automatically find Teensy on COM Port
    Written by Joe Steinmeyer, 2017
    Edited by Joel Voldman, 2017
'''

def get_teensy_port():
    teensy_port = list(serial.tools.list_ports.grep("Teensy"))
    if len(teensy_port) == 1:
        print("Automatically found Teensy: {}".format(teensy_port[0].description))
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


def get_esp_port():
    teensy_port = list(serial.tools.list_ports.grep("SLAB_USBtoUART"))
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

#-------------------

# ports = serial_ports() #generate list of currently connected serial ports 
# print (ports)

# ser = ports[0]

# s = serial.Serial(ser)
# print(s)

##RENDERING INFORMATION:

left_nodes = 62
right_nodes = 62
left_bus = 2
right_bus = 2
node_v_spacing = 0.18
bus_h_spacing = 0.18
midline_gap = 0.25
node_to_bus_gap = 0.15


node_length = 1.0
node_height = 0.15
bus_height = node_v_spacing*left_nodes
bb_node = [[0,0,node_length,node_length],[0,node_height,node_height,0]]
bb_bus = [[0,0,node_height,node_height],[0,bus_height,bus_height,0]]

image_height = 0.3+bus_height
image_width = 0.3+midline_gap + 2*node_length+2*node_to_bus_gap+4*node_height

BB_x = []
BB_y = []

orientation='horizontal'

if orientation =='vertical':
    for q in range(left_nodes):
        BB_x.append([t for t in bb_node[0]])
        BB_y.append([t+q*node_v_spacing for t in bb_node[1]])
    for q in range(right_nodes):
        BB_x.append([t+node_length+midline_gap for t in bb_node[0]])
        BB_y.append([t+q*node_v_spacing for t in bb_node[1]])
    
    BB_x.append([t-node_to_bus_gap-node_height - bus_h_spacing for t in bb_bus[0]])
    BB_x.append([t-node_to_bus_gap-node_height for t in bb_bus[0]])
    BB_x.append([t+node_to_bus_gap+2*node_length + midline_gap for t in bb_bus[0]])
    BB_x.append([t+bus_h_spacing+node_to_bus_gap+2*node_length + midline_gap for t in bb_bus[0]])
    for y in range(4):
        BB_y.append(bb_bus[1])
    image_height = 0.3+bus_height
    image_width = 0.3+midline_gap + 2*node_length+2*node_to_bus_gap+4*node_height
    pixel_scaler = 500/6
else:
    for q in range(left_nodes):
        BB_y.append([t for t in bb_node[0]])
        BB_x.append([t+q*node_v_spacing for t in bb_node[1]])
    for q in range(right_nodes):
        BB_y.append([t+node_length+midline_gap for t in bb_node[0]])
        BB_x.append([t+q*node_v_spacing for t in bb_node[1]])
    
    BB_y.append([t-node_to_bus_gap -node_height- bus_h_spacing for t in bb_bus[0]])
    BB_y.append([t-node_to_bus_gap-node_height for t in bb_bus[0]])
    BB_y.append([t+node_to_bus_gap+2*node_length + midline_gap for t in bb_bus[0]])
    BB_y.append([t+bus_h_spacing+node_to_bus_gap+2*node_length + midline_gap for t in bb_bus[0]])
    for y in range(4):
        BB_x.append(bb_bus[1])
    image_width = 0.3+bus_height
    image_height = 0.3+midline_gap + 2*node_length+2*node_to_bus_gap+4*node_height
    pixel_scaler = 500/5

def color_getter(value,maximum):
    integer = int(math.floor(value*255/maximum))
    #print (integer)
    hexval = hex(integer)[2:]
    #print (hexval)
    if len(str(hexval))==1:
        return "#" +"0" +hexval+"0000"
    else:
        return "#" +hexval+"0000"






#FLASK SETUP SETUP:

async_mode = None
if async_mode is None:
    try:
        import eventlet
        async_mode = 'eventlet'
    except ImportError:
        pass

    if async_mode is None:
        try:
            from gevent import monkey
            async_mode = 'gevent'
        except ImportError:
            #fuck it
            pass

    if async_mode is None:
        async_mode = 'threading'

    print('async_mode is ' + async_mode)

# monkey patching is necessary because this application uses a background
# thread
if async_mode == 'eventlet':
    import eventlet
    eventlet.monkey_patch()
elif async_mode == 'gevent':
    from gevent import monkey
    monkey.patch_all()

#Start up Flask server:
app = Flask(__name__, template_folder = './',static_url_path='/static')
app.config['SECRET_KEY'] = 'secret!' #shhh don't tell anyone. Is a secret
socketio = SocketIO(app, async_mode = async_mode)
thread = None

s = 5 #create object to hold the s serial object


def dataThread():
    global s
    unique = 456
    # ser = get_teensy_port()
    ser = get_esp_port()
    if not ser:
        print("No Microcontroller Found!")
        return False
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
        #x = x[1]
        #print(x)
        x = x.split("&")
        x = x[:-1]
        bins=[]
        for y in x:
            parts = y.split(":")
            bins.append((int(parts[0]),int(parts[1])))
        #print(bins)
        uname,password = [i.strip() for i in open(os.path.expanduser(lcl+'/bb_login'))]
        payload = {'user':uname,'board':str(bins)}
        r = requests.post(hal, json=payload, timeout=1.0)
        print(r.status_code)
        print(r.json())
        node_voltage = list()
        time_x = list()
        count = 0

        #Create place holders when all is not called
        #organizing to operate different modes
        old_voltage = [0]*128  #create a list of zeros of 128 elements
        for y in bins:
            old_voltage[y[0]] = 3.3*y[1]/1023

        old_names = list(range(128))

        #print (old_names)

        #Reorganizing orders to match with the breadboard layout
        names = list()
        voltage = list()
        for i in old_names:
            index = old_names.index(i)
            if index >= 62:
                if index == 125:
                    names.append(63)
                    voltage.append(old_voltage[63])
                elif index == 124:
                    names.append(62)
                    voltage.append(old_voltage[62])
                elif (index == 126):
                    names.append(127)
                    voltage.append(old_voltage[127])
                elif (index == 127):
                    names.append(126)
                    voltage.append(old_voltage[126])
                else:
                    names.append(i + 2)
                    voltage.append(old_voltage[index + 2])
            else:
                names.append(i)
                voltage.append(old_voltage[index])

        #colors = ["#F1EEF6", "#D4B9DA", "#C994C7", "#DF65B0", "#DD1C77", "#980043"]    
        colors = [color_getter(v,3.3) for v in voltage]
        source = ColumnDataSource(
            data = dict(
                x=BB_x,
                y=BB_y,
                color=colors,
                name=names,
                voltage=voltage,
            )
        )
        TOOLS="hover,save"
        p = figure(title="Breadboard Voltages", tools=TOOLS)
        p.toolbar.logo=None
        p.patches('x', 'y',
            fill_color='color', fill_alpha=0.7,
            line_color="white", line_width=0.0,
            source=source)
        p.xgrid.grid_line_color = None
        p.ygrid.grid_line_color = None

        p.plot_height=int(pixel_scaler*image_height)
        p.plot_width=int(pixel_scaler*image_width)
        p.axis.visible = False
        hover = p.select(dict(type=HoverTool))
        hover.point_policy = "follow_mouse"
        hover.tooltips = OrderedDict([
            ("Name", "@name"),
            ("Voltage)", "@voltage V"),
        ])
        
        script, div = components(p)
        prep = script + div

        #val1 = amp1*math.sin(omega1*time.time())
        #val2 = amp2*math.sin(omega2*time.time())
        socketio.emit('update_{}'.format(unique),prep,broadcast =True)
        time.sleep(5)
        #print('sending')

@app.route('/')
def index():
    global thread
    print ("A user connected")
    if thread is None:
        thread = Thread(target=dataThread)
        thread.daemon = True
        thread.start()
    return render_template('base.html')

try:
    if __name__ == '__main__':
        socketio.run(app, port=3000, debug=True)
except:
    s.close()

