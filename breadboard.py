import sqlite3
from flask import Flask
from flask import request
from flask import jsonify
import time
from werkzeug.debug import DebuggedApplication
application = Flask(__name__)

application.debug=True
application.wsgi_app = DebuggedApplication(application.wsgi_app, True)
response = 0

#for humans only:
'''
HTML form that allows user to enter in the id of the device they need to access and the type of request they need.
POSTs the values to the route /postValues
'''

@application.route("/")
def hello():
    return r'''<h3>Enter your breadboard request:</h3>
<form action="/breadboard/humanRequest" method="POST">
  Dev id:<br>
  <input type="text" name="dev_id" id="dev_id" value=""><br>
  Request:<br>
  <input type="text" name="request" id="request" value="ALL"><br><br>
  <input type="submit" id="submit_button" value="Submit">
</form>'''

db = "bb_records.db"

#for humans only:
'''
The POST part lets the HTML form to post values to the server and this in turn adds a new row to the SQL database of the incomplete tasks. Returns a task ID.
The GET part allows the user to send multiple periodic GET requests to check if the task has been completed. It takes the task ID as argument and returns the values.
'''

@application.route("/humanRequest", methods=['GET', 'POST'])
def postValues():
    response = 1
    if request.method == 'POST':
        rqst = request.form
        tme = str(time.time())
        tsk_id = int(time.time()*100000)
        dev_id = str(rqst['dev_id'])
        type = str(rqst['request'])
        con = sqlite3.connect(db)
        c = con.cursor()
        c.execute("INSERT INTO records VALUES (?,?,?,?,?,?)",(tme,dev_id,tsk_id,-1,type,""))
        con.commit()
        con.close()
        return str(tsk_id)

    if request.method == 'GET':
        tsk = request.args.get('task_id')
        conn = sqlite3.connect(db)
        cursr = conn.cursor()
        row = cursr.execute("SELECT * FROM records WHERE task_id = ?",(tsk,)).fetchone()
        if str(row) == 'None':
            return 'Task not complete'
        else:
            return str(row)

#for devices only
'''
This route allows a path for the ESP32 to send GET requests asking if the server has any task for it. 
Takes in device ID as argument and returns the task in the ESP32 readable form.
'''

@application.route("/deviceInquiry", methods=['GET'])
def deviceInquiry():
    dv_id = request.args.get('dev_id')
    conn = sqlite3.connect(db)
    c = conn.cursor()
    row = c.execute('SELECT * FROM records WHERE response_state = ? AND dev_id = ?',(-1,dv_id)).fetchone()
    if str(row) == 'None':
        return 'No task or incomplete task'
    else:
        rtrn = str(row[2])+'&'+str(row[4]) #return task_id and task to carry out
        return str(rtrn)

##for devices only
'''
This route gives a spot for the ESP32 to POST the values after the analysis of the breadboard.
It accepts the measured values, device id, and task id and creates a new row in the thing table to give to the user.
It also updates the ask table to change the response of that task id to finished.
'''

@application.route("/reportValues", methods=['POST'])
def reportValues():
    stuff = request.get_json()
    values = str(stuff['values'])
    timeo = str(time.time())
    extra = str(stuff['timestamps'])
    dev_id = str(stuff['dev_id'])
    tsk_id = str(stuff['task_id'])
    response = 0
    conn = sqlite3.connect('bb_records.db')
    c = conn.cursor()
    c.execute("INSERT INTO thing VALUES (?,?,?,?,?,?)",(time,dev_id,tsk_id,response,values,extra))
    conn.commit()
    conn.close()

    con = sqlite3.connect('human.db')
    cursr = con.cursor()
    cursr.execute("UPDATE ask SET response = ? WHERE task_id = ?",(response,tsk_id))
    con.commit()
    con.close()

if __name__ == "__main__":
    application.run(host='0.0.0.0')
