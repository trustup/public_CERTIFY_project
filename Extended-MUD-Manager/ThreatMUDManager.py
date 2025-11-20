import os
import threading
import logging
import traceback
import json
import random

import requests
import yaml
from confluent_kafka import Consumer, Producer
from Database import TMUD, Database
import sys
from flask import Flask, request

current_dir = os.path.dirname(os.path.abspath(__file__))

# Go up one level to the Implementation directory
parent_dir = os.path.abspath(os.path.join(current_dir, '..'))

# Add the Implementation directory to sys.path
sys.path.append(parent_dir)

app = Flask(__name__)

manager = None

from Policy_Translator import mudtranslator


class ThreatMUDManager:

    def __init__(self):

        logging.basicConfig(filename="TMUD.log", level=logging.DEBUG)

        # Read config.yaml

        with open('config.yaml') as f:
            config = yaml.load(f,Loader=yaml.FullLoader)
            self.SetInterval = config['TMUDManager']['SetInterval']
            self.interval = config['TMUDManager']['Interval']
            self.broker = config['TMUDManager']['broker']
            self.receivetopic = config['TMUDManager']['receivetopic']
            self.sendtopic = config['TMUDManager']['sendtopic']

        self.db = Database()

        if self.SetInterval:
            self.periodicCheck()

        pconf = {
            'bootstrap.servers': self.broker,
        }
        #self.producer = Producer(pconf)
        #self.start()

    def periodicCheck(self):
        logging.info("Checking if some MUDFile content changes")

        tmuds = self.db.getAllTMUDs()
        if tmuds is not None:
            for tmud in tmuds:
                if not self.compareTMUD(tmud.tmudurl, tmud.tmudfile):
                    logging.info("Detected TMUD File change on manufacturer side -> Executing TMUD Enforcement process")
                    tmud.tmudfile = self.getTMUDFile(tmud.tmudurl)
                    self.db.updateTMUD(tmud)
                    logging.info("Updating database with new TMUD File for TMUD URL: " + tmud.tmudurl)
                    self.sendTMUDFile(tmud)

        logging.info("Checking TMUDFiles process complete")
        threading.Timer(float(self.interval), function=self.periodicCheck).start()

    def sendTMUDFile(self, tmud):
        
        out = {
                "tmudmspl": tmud.mspl
        }

        tmud_json = json.dumps(out)

        self.producer.poll(1)
        self.producer.produce(self.sendtopic, value=tmud_json)
        self.producer.flush()
        pass

    def validateURL(self, tmudurl):

        return False

    def compareTMUD(self, tmudurl, tmudfileDB):

        tmudfile = self.getTMUDFile(tmudurl)
        if tmudfile == tmudfileDB:
            return True

        return False

    def getTMUDFile(self, tmudurl):
        r = requests.get(tmudurl)

        return r.text

    def verifyTMUDFile(self, tmudurl):

        tmudsignurl = tmudurl.replace(".json", ".p7s")

        logging.info("Downloading tmudfile from: " + tmudurl)
        r = requests.get(tmudurl)
        tmudfile = "temp" + str(random.randint(1, 100000000)) + ".json"

        open(tmudfile, "w+").write(r.text)

        logging.info("Downloading singature for tmudfile from: " + tmudsignurl)
        r = requests.get(tmudsignurl)

        tmudcrt = "temp" + str(random.randint(1, 100000000)) + ".p7s"

        open(tmudcrt, "wb+").write(r.content)

        logging.info("Verifying TMUD File")

        # ret = Verifier._verify(tmudcrt,tmudfile,"pubkey.crt")
        ret = True
        os.remove(tmudcrt)
        os.remove(tmudfile)
        logging.info("TMUD Verified")
        return True
        return ret

    def getTranslation(self, tmudfile):

        return mudtranslator.translate(json.loads(tmudfile))

    def newTMUD(self, tmudurl):

        tmudfile = self.getTMUDFile(tmudurl)

        js = json.loads(tmudfile)
        print(tmudfile)
        device_identifier = js["ietf-threatmud:mud"]["models-affected"]

        logging.info(f"Translating tmudfile: {tmudurl}")
        mspl = self.getTranslation(tmudfile)
        print(mspl)
        logging.info(f"Transaltion from {tmudurl} -> {mspl}")
        tmud = TMUD(tmudurl=tmudurl, device_identifier=device_identifier, tmudfile=tmudfile, mspl=mspl)

        if self.db.getTMUD(tmudurl) is None:
            logging.info("Adding new TMUD to database")

            self.db.createTMUD(tmudurl, tmudfile, device_identifier, mspl)



        logging.info("Sending TMUD File in MSPL ")
        #self.sendTMUDFile(tmud)
        dic = {
            "tmudurl": tmudurl,
            "tmudfile": tmudfile,
            "mspl": mspl
        }
        mud_json = json.dumps(dic)
        return mud_json

    def newConnexion(self, message):
        logging.info("Received new message from kafka")
        logging.info("Message:\n" + message)
        logging.info("Parsing message")
        js = json.loads(message)

        tmudurl = js["threatuuid"]

        


        self.newTMUD(tmudurl)

        return

    def start(self):

        conf = {
            'bootstrap.servers': self.broker,
            # usually of the form cell-1.streaming.<region>.oci.oraclecloud.com:9092
            'group.id': 'TMUD Manager',
        }
        # Create Consumer instance
        consumer = Consumer(conf)
        # Subscribe to topic
        consumer.subscribe([self.receivetopic])
        try:
            logging.info("TMUD Server is starting")
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
                    try:
                        print("parsing new connexion")
                        print(record_value)
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
            logging.info("TMUD Server is closing")

@app.route('/tmud', methods=['POST'])
def newtmud():

    js = request.get_json()
    print(js)
    logging.info("Received new message from REST API")
    logging.info("Message:\n"+str(js))
    logging.info("Parsing message")


    tmudurl = js['tmudurl']
    

    response = manager.newTMUD(tmudurl)
    print(response)

    return response

if __name__ == '__main__':
    manager = ThreatMUDManager()
    app.run(host='0.0.0.0', use_reloader=False, debug=True, port=8889)
