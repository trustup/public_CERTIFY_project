import socket
import ssl

import threading
import time
import logging

from flask import Flask, request

import MUDFileSigner as Verifier
import requests
import random
import os
import yaml
from Database import Database, MUD
from confluent_kafka import Consumer, Producer
import json
import sys
current_dir = os.path.dirname(os.path.abspath(__file__))

# Go up one level to the Implementation directory
parent_dir = os.path.abspath(os.path.join(current_dir, '..'))

# Add the Implementation directory to sys.path
sys.path.append(parent_dir)

from Policy_Translator import mudtranslator
import traceback
app = Flask(__name__)

manager = None

class MUDManager:

    def __init__(self):

        logging.basicConfig(filename="MUD.log", level=logging.DEBUG)


        # Read config.yaml

        with open('config.yaml') as f:
            config = yaml.load(f,Loader=yaml.FullLoader)
            self.SetInterval = config['MUDManager']['SetInterval']
            self.interval = config['MUDManager']['Interval']
            self.broker = config['MUDManager']['broker']
            self.receivetopic = config['MUDManager']['receivetopic']
            self.sendtopic = config['MUDManager']['sendtopic']
        self.db = Database()

        if self.SetInterval:
            self.periodicCheck()

        pconf = {
            'bootstrap.servers': self.broker,
        }
        self.producer = Producer(pconf)

        threading.Thread(target=self.start).start()
        #self.start()

    def periodicCheck(self):
        logging.info("Checking if some MUDFile content changes")

        muds = self.db.getAllMUDs()
        if muds is not None:
            for mud in muds:
                if not self.compareMUD(mud.mudurl,mud.mudfile):
                    logging.info("Detected MUD File change on manufacturer side -> Executing MUD Enforcement process")
                    mud.mudfile = self.getMUDFile(mud.mudurl)
                    self.db.updateMUD(mud)
                    logging.info("Updating database with new MUD File for MUD URL: " + mud.mudurl)
                    self.sendMUDFile(mud)

        logging.info("Checking MUDFiles process complete")
        threading.Timer(float(self.interval), function=self.periodicCheck).start()

    def sendMUDFile(self, mud):

        dic = {
            "mudurl": mud.mudurl,
            "mudfile": mud.mudfile,
            "device_identifier": mud.device_identifier
        }
        mud_json = json.dumps(dic)

        self.producer.poll(1)
        self.producer.produce(self.sendtopic, value=mud_json)
        self.producer.flush()

        pass

    def validateURL(self,mudurl):


        return False

    def compareMUD(self,mudurl,mudfileDB):

        mudfile = self.getMUDFile(mudurl)
        if mudfile == mudfileDB:
            return True

        return False

    def getMUDFile(self,mudurl):
        r = requests.get(mudurl)

        return r.text

    def verifyMUDFile(self,mudurl):

        mudsignurl = mudurl.replace(".json",".p7s")

        logging.info("Downloading mudfile from: " + mudurl)
        r = requests.get(mudurl)
        mudfile = "temp" + str(random.randint(1,100000000)) + ".json"

        open(mudfile,"w+").write(r.text)

        logging.info("Downloading singature for mudfile from: " + mudsignurl)
        r = requests.get(mudsignurl)

        mudcrt = "temp" + str(random.randint(1,100000000)) + ".p7s"

        open(mudcrt,"wb+").write(r.content)

        logging.info("Verifying MUD File")

        #ret = Verifier._verify(mudcrt,mudfile,"pubkey.crt")
        ret = True
        os.remove(mudcrt)
        os.remove(mudfile)
        logging.info("MUD Verified")
        return True
        return ret


    def newMUD(self,device_identifier, mudurl):

        if not self.verifyMUDFile(mudurl):
            logging.info("Fail at verifing MUD File sign")
            return

        mudfile = self.getMUDFile(mudurl)

        mud = MUD(mudurl=mudurl, device_identifier=device_identifier, mudfile=mudfile)

        if self.db.getMUD(mudurl) is None:
            logging.info("Adding new MUD to database")
            self.db.createMUD(mudurl,mudfile,device_identifier)
        logging.info("Sending MUD File to Domain and Device Manager")
        #self.sendMUDFile(mud)
        dic = {
            "mudurl": mud.mudurl,
            "mudfile": mud.mudfile,
            "device_identifier": mud.device_identifier
        }
        mud_json = json.dumps(dic)
        return mud_json


    def newConnexion(self, message):
        logging.info("Received new message from kafka")
        logging.info("Message:\n"+message)
        logging.info("Parsing message")
        js = json.loads(message)

        mudurl = js["mudurl"]
        device_identifier = js["device_identifier"]
        self.newMUD(device_identifier,mudurl)

        return

    def start(self):
        pass
'''
        conf = {
            'bootstrap.servers': self.broker,
            # usually of the form cell-1.streaming.<region>.oci.oraclecloud.com:9092
            'group.id': 'MUD Manager',
        }
        # Create Consumer instance
        consumer = Consumer(conf)
        # Subscribe to topic
        consumer.subscribe([self.receivetopic])
        try:
            logging.info("MUD Server is starting")
            logging.info("Subscribing to Kafka topic: " + self.receivetopic)

            while True:
                msg = consumer.poll(1.0)
                if msg is None:
                    # No message available within timeout.
                    # Initial message consumption may take up to
                    # `session.timeout.ms` for the consumer group to
                    # rebalance and start consuming
                    print("Waiting for message or event/error in poll()")
                    continue
                elif msg.error():
                    logging.info('error: {}'.format(msg.error()))
                else:
                    # Check for Kafka message
                    record_key = "Null" if msg.key() is None else msg.key().decode('utf-8')
                    record_value = msg.value().decode('utf-8')
                    logging.info("Consumed record with key " + record_key + " and value " + record_value)
                    print(record_value)
                    try:
                        print("parsing new connexion")
                        print(type(record_value))
                        t = threading.Thread(target=self.newConnexion, args=(record_value,))
                        t.run()
                    except Exception as e:
                        print(str(e))
                        traceback.print_exc()
                        pass
                

        except KeyboardInterrupt:
            pass
        finally:
            logging.info(print("Leave group and commit final offsets"))
            consumer.close()
            logging.info("MUD Server is closing")
'''

@app.route('/mud', methods=['POST'])
def newConnexion():

    js = request.get_json()
    print(js)
    logging.info("Received new message from REST API")
    logging.info("Message:\n"+str(js))
    logging.info("Parsing message")


    mudurl = js['mudurl']
    device_identifier = js['device_identifier']

    response = manager.newMUD(device_identifier,mudurl)
    print(response)

    return response

if __name__ == '__main__':
    manager = MUDManager()
    app.run(host='0.0.0.0', use_reloader=False, debug=True, port=8888)


