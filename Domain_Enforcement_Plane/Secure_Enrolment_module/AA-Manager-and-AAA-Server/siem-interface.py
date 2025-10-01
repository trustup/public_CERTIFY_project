from flask import Flask, jsonify
import json

REG_FILE = "registered_devices"


def get_device_list():

    device_list = []

    with open(REG_FILE, "r") as register:
        lines = register.readlines()

    for line in lines:
        fields = line.split(':')
        element = {"id": fields[0],
                   "address": fields[1],
                   "bootstrapp_status": fields[2][:-1]}
        device_list.append(element)

    res = {"devices": device_list}
    return json.dumps(res, indent=4)


def get_device(id):

    with open(REG_FILE, "r") as register:
        lines = register.readlines()

    for line in lines:
        fields = line.split(':')
        if fields[0] == id:
            res = {"genuine": "OK" if fields[2][0] == 's' else "KO"}
            break

    return json.dumps(res, indent=4)


app = Flask(__name__)


# GET endpoint with no parameters
@app.route('/request_status', methods=['GET'])
def status():
    return get_device_list()


# GET endpoint with an ID parameter
@app.route('/request_genuine/<string:item_id>', methods=['GET'])
def get_data(item_id):
    return get_device(item_id)


if __name__ == '__main__':
    app.run(host="0.0.0.0", port=7474)
