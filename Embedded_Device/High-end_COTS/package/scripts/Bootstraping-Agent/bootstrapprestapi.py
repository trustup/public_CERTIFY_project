from flask import Flask, request, jsonify
import setup_demo


app = Flask(__name__)
@app.route('/presentPsk', methods=['POST'])
def storeMsk():
    try:
        print(request.json)
        base64Psk = request.json['base64Psk']
        deviceId = request.json['device_id']
        ok = setup_demo.initiateBootstrapping(base64Psk)
        if ok:
            response = {
                "device_id" : deviceId,
                "ACK" : "ok"
            }

            isBoostrapped = True

            return jsonify(response)
        else:
            return 'error during bootstrapping', 500
    except Exception as e:
        print(e)
        return 'Error with presented Json', 400

@app.route('/getBootstrapParams', methods=['GET'])
def retreiveBootParams():
    try:
        base64Msk, mudUrl, ok = setup_demo.getBootParams()
        print("Base64MSK: " + base64Msk)
        print("Mud URL: " + mudUrl)
        if ok:
            response = {
                "base64Msk" : base64Msk,
                "mudURL" : mudUrl
            }
            return jsonify(response)
        else:
            raise Exception("Error retreiving parameters")
    except Exception as e:
        print(e)
        return 'Error retreiving parameters', 500

if __name__ == '__main__':
    app.run(debug=True, host="10.0.2.15", port=5024)
