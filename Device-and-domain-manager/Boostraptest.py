import json

import requests
from confluent_kafka import Consumer,Producer
from scapy.all import *
import json
if __name__ == '__main__':


    # Sample device object data
    device_data = {
        "ip_address": "192.168.10.4",
        "device": "Log4j",
        "uuid": "123x2xxxxxxxxxxxxxcxxxxxxxx1111",
        "mud-url": "http://localhost:8091/MUD_Collins_Bootstrapping"
    }

    # Serialize and save to a file
    with open('device.json', 'w') as json_file:
        json.dump(device_data, json_file, indent=4)
    topic = "bootstrap"
    conf = {
        'bootstrap.servers': "192.168.0.121:9092",
        'group.id': "traces-group",

    }
    pconf = {
        'bootstrap.servers': "192.168.0.121:9092",

    }

    # Create Consumer instance
    consumer = Consumer(conf)
    producer = Producer(pconf)



    # Subscribe to topic
    consumer.subscribe([topic])

    headers = {'Content-Type': 'application/json'}
    response = requests.post("http://localhost:4321/boostrapping", data=json.dumps(device_data), headers=headers)



    
    '''
    try:
        while True:
            msg = consumer.poll(1.0)
            if msg is None:
                # No message available within timeout.
                # Initial message consumption may take up to
                # `session.timeout.ms` for the consumer group to
                # rebalance and start consuming
                print("Waiting for message or event/error in poll()")
                continue
            else:
                # Check for Kafka message
                print(msg)
                record_key = "Null" if msg.key() is None else msg.key().decode('utf-8')
                record_value = msg.value().decode('utf-8')
                print(record_value)


    finally:
        consumer.close()

    '''
