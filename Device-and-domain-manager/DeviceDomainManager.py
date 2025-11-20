import time
import globals
globals.init_global_settings() #Import and init global settings from environment variables
import logging
import yaml
import Database
from confluent_kafka import Consumer, Producer
import json
# Import Flask for API REST server, create app variable and import routes from routes.py file
from flask import Flask, request, jsonify
import routes
import requests
import InventoryDriver
import threading
import os
import sys
import ERADriver

current_dir = os.path.dirname(os.path.abspath(__file__))

# Go up one level to the Implementation directory
parent_dir = os.path.abspath(os.path.join(current_dir, '..'))

# Add the Implementation directory to sys.path
sys.path.append(parent_dir)

from Policy_Translator import mudtranslator
from lxml import etree
from datetime import datetime

app = Flask(__name__)
app.add_url_rule('/register_device', view_func=routes.register_device, methods=['POST'])
app.add_url_rule('/register_software', view_func=routes.register_software, methods=['POST'])
app.add_url_rule('/get_software', view_func=routes.get_software, methods=['POST'])
app.add_url_rule('/update_status', view_func=routes.update_status, methods=['POST'])

dadm = None

class DeviceDomainManager:

    def __init__(self):

        # Read config.yaml

        with open('config.yaml') as f:
            config = yaml.load(f, Loader=yaml.FullLoader)
            self.broker = config['DeviceDomainManager']['Broker']
            self.bootstrapping = config['DeviceDomainManager']['EnrolmentIP']

            self.topicKnownDevice = config['DeviceDomainManager']['topicKnownDevice']
            self.topicThreatMUD = config['DeviceDomainManager']['topicTMUDFile']

            self.topicSendMudFile = config['DeviceDomainManager']['topicsendMUDFile']
            self.topicsiemACK = config['DeviceDomainManager']['topicSIEMACK']

            self.topicDADM_Notification = config['DeviceDomainManager']['topicDADMNotification']
            self.topicThreatRequest = config['DeviceDomainManager']['topicThreatRequest']

            self.topics = [self.topicKnownDevice, self.topicThreatRequest]

        self.db = Database.Database()

        pconf = {
            'bootstrap.servers': self.broker,
        }
        self.producer = Producer(pconf)

        threading.Thread(target=self.start).start()
        #self.start()

    def configBroker(self):
        '''
        conf = {
            'bootstrap.servers': self.broker,
            # usually of the form cell-1.streaming.<region>.oci.oraclecloud.com:9092
            'security.protocol': 'SSL',
            'ssl.ca.location': './ca.pem',  # from step 6 of Prerequisites section
            'ssl.certificate.location': './darcy.pem',
            'ssl.key.location': './darcy.key',
            'ssl.key.password': 'confluent',
            'group.id': 'DARCY1',
            'auto.offset.reset': 'latest',
            'enable.ssl.certificate.verification': False
            # optionally instead of giving path as shown above, you can do 1. pip install certifi 2. import certifi and
            # 3. 'ssl.ca.location': certifi.where()
        }
        '''

        conf = {
            'bootstrap.servers': self.broker,
            'group.id': 'DomainDeviceManager'
        }

        # Create Consumer instance
        self.consumer = Consumer(conf)


    def reconfigureDevice(self, mitigation_data, device):
        
        message = ERADriver.compose_message_change_signature()

        logging.info(f"Sending reconfiguration for device {device.identifier}. Reconfiguration message: {message}")
        ERADriver.send_message(device.ip, message, edk = device.edk)
        

    def getTranslation(self, mudfile):
        return mudtranslator.translate(json.loads(mudfile))


    def sendSIEMNotification(self, device):
        data = {
            "device_id": device,
            "status": "attestation quote failed"
        }
        self.producer.poll(1)
        self.producer.produce(self.topicDADM_Notification, value=json.dumps(data))
        self.producer.flush()

    def sendMUDURL(self, mudurl, device_identifier):

        dic = {
            'mudurl': mudurl,
            'device_identifier': device_identifier
        }

        headers = {
            'Content-Type': 'application/json'
        }
        response = requests.post("http://localhost:8888/mud", json=dic, headers=headers)

        js = response.json()
        print(js)
        self.process_message_topic_MUDFile(js)

        pass

    def send_mud_file(self, device):
        logging.info(f"Sending MUD File from device {device.identifier} to SIEM-SOAR")
        data = {
            "device_id": device.identifier,
            "mudmspl": device.mitigation_actions
        }
        self.producer.poll(1)
        self.producer.produce(self.topicSendMudFile, value=json.dumps(data))
        self.producer.flush()

    def sendEnforcementCompleted(self, reconfigured_data, device):
        logging.info(f"Sending signal to SIEM-SOAR about enforcement completed for device: {device.identifier} ")

        data = {
            "device_id": device.identifier,
            "message": "Enforcement completed for device",
            "translated_mspl": reconfigured_data
        }

        self.producer.poll(1)
        self.producer.produce(self.topicThreatMUD, value=json.dumps(data))
        self.producer.flush()

    def parseMitigationsTMUD(self, mitigation_actions):
        
        print("ma0" + mitigation_actions[0])
        print("ma1" + mitigation_actions[1])
        root = etree.XML(mitigation_actions[0].encode("utf-8"))

        # Extract models-affected
        models_affected = root.xpath('//models-affected/text()')

        # Initialize an empty list to store the dictionaries
        config_actions = []

        # Iterate through each configuration-action element
        for action in root.xpath('//configuration-action'):
            action_dict = {}

            # Iterate through each child of the action element
            for child in action:
                if child.tag not in action_dict:
                    action_dict[child.tag] = {}
                for grandchild in child:
                    # Handle multiple elements with the same tag (e.g., software-fix-url)
                    if grandchild.tag in action_dict[child.tag]:
                        if isinstance(action_dict[child.tag][grandchild.tag], list):
                            action_dict[child.tag][grandchild.tag].append(grandchild.text)
                        else:
                            action_dict[child.tag][grandchild.tag] = [action_dict[child.tag][grandchild.tag],
                                                                      grandchild.text]
                    else:
                        action_dict[child.tag][grandchild.tag] = grandchild.text

            # Add the action dictionary to the list
            config_actions.append(action_dict)

        config_conditions = []

        # Iterate through each configuration-condition element
        for condition in root.xpath('//configuration-condition'):
            condition_dict = {}

            # Iterate through each child of the condition element
            for child in condition:
                if child.tag not in condition_dict:
                    condition_dict[child.tag] = {}
                for grandchild in child:
                    if grandchild.tag not in condition_dict[child.tag]:
                        condition_dict[child.tag][grandchild.tag] = {}
                    for greatgrandchild in grandchild:
                        condition_dict[child.tag][grandchild.tag][greatgrandchild.tag] = greatgrandchild.text

            # Add the condition dictionary to the list
            config_conditions.append(condition_dict)

        mitigation_data = {
            "action": config_actions,
            "condition": config_conditions,
            "models": models_affected
        }
        print(mitigation_data)
        return mitigation_data
    def parseMitigationsMUD(self, mitigation_actions):

        print(mitigation_actions)
        print(type(mitigation_actions))
        tree = etree.fromstring(mitigation_actions.encode("utf-8"))

        config_actions = []
        for action in tree.findall('.//configuration-action'):
            action_dict = {}

            # Iterate through each child of the action element
            for child in action:
                action_dict[child.tag] = {grandchild.tag: grandchild.text for grandchild in child}

            # Add the action dictionary to the list
            config_actions.append(action_dict)

        print(config_actions)

        return config_actions

    def process_message_topic_MUDFile(self, message):
        print(message)

        js = message

        device_identifier = js["device_identifier"]
        mudfile = js["mudfile"]

        logging.info("New MUDFile comming from MUD Manager for device: " + device_identifier)
        device = self.db.getDevice(device_identifier)
        if device is not None:
            device.mudfile = mudfile
            logging.info("Sending new MUD File to translation")
            device.mitigation_actions = self.getTranslation(mudfile)[0].replace('\n','')

            logging.info("Updating device MUD File in database")
            self.db.updateDevice(device)

            mitigation_data = self.parseMitigationsMUD(device.mitigation_actions)
            self.reconfigureDevice(mitigation_data, device)
            self.send_mud_file(device)

        pass

    def process_message_topic_Bootsrapping_monitor(self, message):
        print(message)

        js = json.loads(message)
        device_identifier = js["device_id"]
        status = js["status"]
        logging.info(f"Received message from Bootstrapping Monitor with status: {status}")
        if status == "ok":
            device = self.db.getDevice(device_identifier)
            print(f"ok from device {device.identifier}")
            self.publishConfigurationAtInventory(device)
            self.sendBoostrappingACK(device)
        else:
            logging.info("Unable to register configuration at Inventory: Bootstrapping Monitor send KO")

    def getTMUDFile(self, tmudurl):
        # Get TMUD File from URL
        try:
            data = {
                'tmudurl': tmudurl
            }
            response = requests.post("http://localhost:8889/tmud", json=data)
            if response.status_code == 200:
                logging.info("TMUD File retrieved successfully")
                return response.text
            else:
                logging.error("Failed to retrieve TMUD file")
                return None
        except requests.exceptions.RequestException as e:
            logging.error(f"Error retrieving TMUD file: {e}")
            return None




    def process_message_topic_Bootsrapping_monitor_Threat_MUD(self, message):
        print(message)

        js = json.loads(message)


        mspl = js['mspl']
        mitigation_data = self.parseMitigationsTMUD(mspl)

        software_configuration_condition = mitigation_data['condition'][0].get('software-configuration-condition', {})
        software_identification_condition = software_configuration_condition.get('software-identification-condition',
                                                                                 {})
        software_id = software_identification_condition.get('software-id', None)
        software_name = software_identification_condition.get('software-name', None)
        software_version = software_identification_condition.get('software-version', None)

        software_protection_action = mitigation_data['action'][0].get('software-protection-action', {})
        software_protection_type = software_protection_action.get('software-protection-type', None)
        software_fixed_version = software_protection_action.get('software-fixed-version', None)
        software_fix_urls = software_protection_action.get('software-fix-url', None)

        print(software_name)
        print(software_id)
        print(software_fix_urls[0])

        self.triggerUpdateMitigation(sw_url=software_fix_urls[0], sw_name=software_name, sw_identifier=software_id)
        # extract devices from mitigation data
        
        devices = self.db.getDevicesByModel(software_name)
        print(devices)
        device = devices[0]
        for model in mitigation_data['models']:
            devices = self.db.getDevicesByModel(model)
            for device in devices:
                logging.info(f"Reconfiguring device {device.identifier} with model {model}")
                reconfigured_data = self.reconfigureDevice(mitigation_data, device)
                logging.info(f"Sending Threat MSPL TO SIEM-SOAR")
                self.sendEnforcementCompleted(mspl, device)
        

    def sendTMUDFileSIEM(self, tmud):
        logging.info("Sending TMUD File to SIEM-SOAR")
        
        self.producer.poll(1)
        self.producer.produce(self.topicThreatMUD, value=json.dumps(tmud))
        self.producer.flush()
        logging.info("TMUD File sent to SIEM-SOAR")

    def handleSIEMRequest(self,message):
        js = json.loads(message)
        type = js["request_type"]
        if type == "MUD":
            logging.info("Received request from SIEM-SOAR for MUD File")
            device_identifier = js["device_id"]
            device = self.db.getDevice(device_identifier)
            if device is not None:
                self.send_mud_file(device)
            else:
                logging.info("Device not found in database")
        elif type == "Threat MUD":
            logging.info("Received request from SIEM-SOAR for Threat MUD File")
            threat_identifier = js["threat_id"]
            tmudurl = f"http://localhost:8091/{threat_identifier}"
            js = self.getTMUDFile(tmudurl)
            if js is not None:

                self.process_message_topic_Bootsrapping_monitor_Threat_MUD(js)

    def publishConfigurationAtInventory(self, device):

        logging.info("Registering at inventory new Configuration of device")

        data = {
            'device_id': device.identifier,
            'device': {
                'identifier': device.identifier,
                'mac': device.mac,
                'ip': device.ip,
                'model': device.model,
                'mudurl': device.mudurl,
                'mudfile': device.mudfile,
                'mitigation_actions': device.mitigation_actions
            }
        }
        js = json.dumps(data).encode("utf-8")

        #transaction_id, status_code = InventoryDriver.register_in_inventory(js)

        transaction_id = "1234567890"  # Placeholder for transaction ID
        status_code = 200  # Placeholder for status code
        if status_code == 200:
            logging.info("Configuration registration success")
            device.inventory_id = transaction_id
            print(transaction_id)
            self.db.updateDevice(device)
        else:
            logging.info("Configuration registration fails")

    def sendBoostrappingACK(self, device):

        post_data = {
            "value": "Device registration success",
            "uuid": device.identifier
        }
        response = requests.post(f"http://155.54.95.211:8098/ack", json=json.dumps(post_data))
        # Check response for ACK
        print(response.status_code)

        if response.status_code == 200:
            self.publishConfigurationAtInventory(device)
        else:
            logging.info(f"An error ocurred sending the MSK to device with ip:{device.identifier}")


        pass

    def printAllDevices(self):
        devices = self.db.getAllDevices()
        for device in devices:
            self.db.printDevice(device)
    
    # Trigger update after receiving MSPL requesting for software update
    # Expected parameters:
    #   "sw_identifier": string, "sw_url": string, "sw_name": string
    def triggerUpdateMitigation(self,sw_identifier,sw_url,sw_name):
        tok,ok=routes.login_DWG()
        if not ok:
            logging.error("Failed login at DWG Software Repo")
            return {"error": "Could not connect with SUAA: login failed"}
        headers = {
        "x-access-token": tok,
        "content-type": "application/json"
        }
        body={}
        body["file_id"]=sw_identifier
        body["priority"]="1"
        body["sw_url"]=sw_url
        body["sw_name"]=sw_name
        current_datetime = datetime.now()
        formatted_datetime = current_datetime.strftime("%Y-%m-%d %H:%M:%S")
        body["start_date"]=formatted_datetime
        try:
            response=requests.post(globals.SUUA_URL+"api/rollout",headers=headers,json=body) # Note: this assumes URL ends with
            if response.status_code == 200:
                logging.info("Started rollout")
                return {"result": "ok"}
            else:
                logging.info("Could not start rollout, status code: %s", str(response.status_code))
                return {"result": "not ok: "+ str(response.status_code)}
        except requests.exceptions.RequestException as e:  # This is the correct syntax
            logging.error("Unable to connect with Software Repository: %s", e)
            return {"error": "Could not connect with SUUA: connection error"}
        
    
    def start(self):
        self.configBroker()

        
        try:
            
            logging.info("Device and Domain Manager is starting")

            logging.info("Starting broker interface")
            
            logging.info("Broker topics: " + ','.join(self.topics))
            self.consumer.subscribe(self.topics)

            while True:
                # poll messages
                message = self.consumer.poll(1)

                if message != None:
                    if message.topic() == self.topicKnownDevice:
                        logging.info("New message received from Network Boostrapping monitor")
                        self.process_message_topic_Bootsrapping_monitor(message.value())

                    elif message.topic() == self.topicThreatRequest:
                        logging.info("New message received from SIEM-SOAR with a Request")
                        self.handleSIEMRequest(message.value())
                        #self.process_message_topic_Bootsrapping_monitor_Threat_MUD(message.value())

        except KeyboardInterrupt:
            pass
        finally:
            logging.info(print("Leave group and commit final offsets"))
            self.consumer.close()
            logging.info("Device and Domain manager is closing")

        




@app.route('/attestation_quote', methods=['POST'])
def attestation_quote():
    
    if request.method == 'POST':
        #jsonn = request.get_json()
        #device_id = jsonn['device_id']
        #status = jsonn['status']
        device_id = request.args.get('device_id')
        status = request.args.get('status')
        logging.info(f"Attestation quote message received for device: {device_id} with status: {status}")
        if status == "1":
            status_code = 200
            #transaction_id, status_code = InventoryDriver.register_in_inventory(jsonn)
            if status_code == 200:
                logging.info(f"Attestation quote message received for device: {device_id} and its verified")
                return jsonify("Data registration in inventory and regsitry successfull"), 200
        else:
            logging.info(f"Attestation quote message received for device: {device_id} and it not verified | sending report to SIEM")
            dadm.sendSIEMNotification(device_id)
            return jsonify("Data regsitration in inventory and registry fails"), 400

@app.route('/policy_hash', methods=['GET'])
def policy_hash():
    device_id = request.args.get('device_id')
    device = dadm.db.getDevice(device_id)
    if device != None:
        js = json.loads(device.mudfile)
        logging.info(f"Returning policy_hash from device: {device_id} with policy_hash: {js["ietf-mud:mud"]["so-hash"]}")
        return jsonify(js["ietf-mud:mud"]["so-hash"])
    else:
        return jsonify("unable to get policy hash for the requested device")
import ast
@app.route('/fingerprint', methods=['GET'])
def figerprint():
    device_id = request.args.get('device_id')
    device = dadm.db.getDevice(device_id)
    if device != None:
        js = json.loads(device.mudfile)
        logging.info(f"Returning policy_hash from device: {device_id} with policy_hash: {js["ietf-mud:mud"]["integrity-fingerprint"]}")
        
        data = js["ietf-mud:mud"]["integrity-fingerprint"]
        ret = ast.literal_eval(data)
        return jsonify(ret)
    else:
        return jsonify("unable to get fingerprint for the requested device")

@app.route('/inventory_attestation', methods=['GET','POST'])
def inventoryGET():
    if request.method == 'GET':
        device_id = request.args.get('device_id')
        response_data, status_code = InventoryDriver.get_events_by_device_id(device_id)
        if status_code == 200:
            return jsonify(InventoryDriver.get_events_by_device_id(device_id)), 200
        else:
            return jsonify("Unable to connect to Inventory and registry")

    elif request.method == 'POST':
        jsonn = request.get_json()
        device_id = jsonn['device_id']
        transaction_id, status_code = InventoryDriver.register_in_inventory(jsonn)
        if status_code == 200:
            return jsonify("Data registration in inventory and regsitry successfull"), 200
        else:
            return jsonify("Data regsitration in inventory and registry fails"), 400

@app.route('/siem_reconfigure', methods=['POST'])
def siem_reconfigure():

    if request.method == 'POST':
        jsonn = request.get_json()
        logging.info("Message received from SIEM-SOAR for reconfiguration with message: " + str(jsonn))
        status_code = 200
        #transaction_id, status_code = InventoryDriver.register_in_inventory(jsonn)
        if status_code == 200:
            return jsonify("Data registration in inventory and regsitry successfull"), 200
        else:
            return jsonify("Data regsitration in inventory and registry fails"), 400

@app.route('/', methods=['GET'])
def list_endpoints():
    endpoints = [
        {
            'method': 'POST',
            'route': '/register_device',
            'description': 'Route to register device for the software upgrade use case.',
            'parameters': {
                'type': 'JSON',
                'fields': [
                    'mac: string',
                    'firmware_version: string',
                    'name: string',
                    'group_id: string',
                    'device_id: string',
                    'v_cred: JSON'
                ]
            }
        },
        {
            'method': 'POST',
            'route': '/register_software',
            'description': 'Route to register software versions in software upgrade case.',
            'parameters': {
                'type': 'JSON',
                'fields': [
                    'sw_identifier: string',
                    'sw_version: string',
                    'name: string'
                ]
            }
        },
        {
            'method': 'POST',
            'route': '/get_software',
            'description': 'Route to register software versions in software upgrade case.',
            'parameters': {
                'type': 'JSON',
                'fields': [
                    'sw_identifier: string',
                    'v_cred: JSON'
                ]
            }
        },
        {
            'method': 'POST',
            'route': '/update_status',
            'description': 'Update device status after update in the software upgrade use case.',
            'parameters': {
                'type': 'JSON',
                'fields': [
                    'device_id: string',
                    'update_id: string',
                    'status: string'
                ]
            }
        },
        {
            'method': 'POST',
            'route': '/boostrapping',
            'description': 'New device is finished the bootstrapping proccess',
            'parameters': {
                'type': 'JSON',
                'fields': [
                    'uuid: string',
                    'device: string',
                    'ip_address: string',
                    "mud-url: string"
                ]
            }
        }
    ]
    return endpoints

@app.route('/boostrapping', methods=['POST'])
def process_message_topic_Bootstrap():
    jsonn = request.get_json()
    
    
    print(jsonn)

    device = dadm.db.parseDeviceJson(jsonn)

    if dadm.db.getDevice(device.identifier) is None:
        # new device bootstrap
        logging.info(
            "New device in bootstrap process with identifier: " + device.identifier)
        # check if there is a bootstrap process for this device

        # Retrieve MUD File

        logging.info(
            "Retrieving MUD File from device with identifier: " + device.identifier + " | mudurl: " + device.mudurl)

        dadm.db.createdevice(device)

        dadm.sendMUDURL(device.mudurl, device.identifier)
        

        return jsonify("Device registration success")

    else:
        # device already registered in network
        logging.info("Device already registered in network")
        return  jsonify("Device already registered in network")

def bootstraptest(device):
    
    
    device = dadm.db.parseDeviceJson(device)
    
    if dadm.db.getDevice(device.identifier) is None:
        logging.info("New device in bootstrap process")
        logging.info("Retrieving MUD File")
        dadm.db.createdevice(device)
        dadm.sendMUDURL(device.mudurl, device.identifier)
       

        
        
        dadm.sendBoostrappingACK(device)

        return jsonify("Device registration success")

def TestreconfigureDevice():
    '''
    configure_data = {
        "action": mitigation_data['action'],
        "data": mitigation_data['data'] 020100020101
    }
    '''
    message = ERADriver.compose_message_wipe()
    ERADriver.send_message("10.1.0.9", message=message, port=33333, edk = "123")

@app.route('/testtmud', methods=['GET'])
def testTMUD():
    data = {
                "request_type": "Threat MUD",
                "threat_id": "cb4d19e3-f51b-15cd-1278-a53533312401",
                "additional_data": "data"
    }

    
    dadm.handleSIEMRequest(json.dumps(data))
    return jsonify("Test TMUD processed successfully")
            

if __name__ == '__main__':
    
    dadm = DeviceDomainManager()
    #TestreconfigureDevice()
    
    device_data = {
        "ip_address": "88.22.219.94",
        "device": "OpenSSH",
        "uuid": "RaspberryPi-3",
        "mud-url": "http://localhost:8091/MUD_Collins_Bootstrapping",
        "edk": "6e40ba3066bfd9fe17dbea6df5ec5e7ff15d8cb0ffd79c86fc2568636f32b782"
    }
    #bootstraptest(device_data)
    
    #testTMUD()

    app.run(host='0.0.0.0', use_reloader=False, debug=True, port=4321)
