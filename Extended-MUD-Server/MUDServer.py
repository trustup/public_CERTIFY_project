
from flask import Flask, request, abort, send_file
import json
import sys
import os

app = Flask(__name__)

@app.route('/list',methods = ['GET'])
def getList():
    lst = os.listdir('./files')
    ret = []
    print(lst)
    for i in lst:
        if ".json" in i:
            ret.append(i.replace(".json",""))
    print(ret)
    print(json.dumps(ret))
    return json.dumps(ret)

@app.route('/<hash>', methods = ['GET'])
def getThreatMudFile(hash):
    print("hash ", hash)
    splited = hash.split(".")
    if len(splited) == 1:
        try:
            file = open("./files/{}.json".format(hash))
            threat_mud_file = json.load(file)
            return json.dumps(threat_mud_file)
        except:
            #return "No Threat-MUD File related to threat {}".format(hash)
            abort(404)
    #check extension correcta***
    elif splited[1] == "p7s":
        try:
            return send_file("./files/{}".format(hash))
        except:
            #return "No Threat-MUD Signature related to threat {}".format(hash)
            abort(404)
    else:
        abort(404)

@app.route("/signature", methods = ['GET'])
def getSignature():
    return "signature"

if __name__ == "__main__":
    #app.run(debug=True, port=5050)
    app.run(debug=True, host="0.0.0.0", port=8091)
