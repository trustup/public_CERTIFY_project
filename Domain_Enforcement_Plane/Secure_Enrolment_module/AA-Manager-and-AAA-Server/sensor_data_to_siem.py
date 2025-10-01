import socket
import struct
import json
import unittest
import re
from kafka import KafkaProducer, errors


class TestKafkaConnectivity(unittest.TestCase):
    def setUp(self):
        self.bootstrap_servers = '10.0.0.8:9092'
        self.topic_name = 'AT_RAWDATA'

    def test_kafka_topic_available(self):
        print(f"\nConnecting to Kafka broker at {self.bootstrap_servers}...")
        try:
            producer = KafkaProducer(
                bootstrap_servers=self.bootstrap_servers,
                value_serializer=lambda v: json.dumps(v).encode('utf-8')
            )
            print("KafkaProducer created successfully.")
            partitions = producer.partitions_for(self.topic_name)
            self.assertIsNotNone(partitions, f"Topic '{self.topic_name}' is not available.")
        except errors.NoBrokersAvailable:
            self.fail("Kafka broker not available...")
        except Exception as e:
            self.fail(f"Unexpected error during Kafka test: {e}")
        finally:
            producer.close()


def parse_ip_header(data):
    ip_header = struct.unpack('!BBHHHBBH4s4s', data[:20])
    version_ihl = ip_header[0]
    ihl = version_ihl & 0xF
    iph_length = ihl * 4
    ttl = ip_header[5]
    protocol = ip_header[6]
    ip_checksum = ip_header[7]
    src_ip = socket.inet_ntoa(ip_header[8])
    dst_ip = socket.inet_ntoa(ip_header[9])
    return src_ip, dst_ip, iph_length, ttl, protocol, ip_checksum

def parse_tcp_header(data):
    tcp_header = struct.unpack('!HHLLBBHHH', data[:20])
    src_port = tcp_header[0]
    dst_port = tcp_header[1]
    sequence = tcp_header[2]
    acknowledgement = tcp_header[3]
    offset_reserved = tcp_header[4]
    tcp_header_length = (offset_reserved >> 4) * 4
    flags = tcp_header[5]
    window = tcp_header[6]
    checksum = tcp_header[7]
    return src_port, dst_port, sequence, acknowledgement, flags, tcp_header_length, window, checksum

def main():
    producer = KafkaProducer(
        bootstrap_servers='10.0.0.8:9092',
        value_serializer=lambda v: json.dumps(v).encode('utf-8')
    )

    raw_socket = socket.socket(socket.AF_INET, socket.SOCK_RAW, socket.IPPROTO_TCP)
    print("Listening for incoming TCP packets on port 5000...")

    while True:
        packet, addr = raw_socket.recvfrom(65535)

        src_ip, dst_ip, ip_header_len, ttl, protocol, ip_checksum = parse_ip_header(packet)

        tcp_start = ip_header_len
        tcp_segment = packet[tcp_start:tcp_start + 20]
        src_port, dst_port, seq, ack, flags, tcp_header_len, window, checksum = parse_tcp_header(tcp_segment)

        if dst_port != 5000:
            continue

        payload_start = tcp_start + tcp_header_len
        payload = packet[payload_start:]

        if not payload:
            continue  # skip empty packets...

        try:
            decoded_payload = payload.decode('utf-8', errors='ignore').strip()
        except:
            decoded_payload = "<binary>"

        pattern = r'time=\d+;temperature=[\d.]+;humidity=[\d.]+'
        re_match = re.search(pattern, decoded_payload)

        if re_match:
            decoded_payload = re_match.group(0)

        # set flag -> artwork transport ongoing
        transport_ongoing = 1

        data = {
            "artwork_transport_ongoing": transport_ongoing,
            "src_ip": src_ip,
            "dst_ip": dst_ip,
            "ttl": ttl,
            "protocol": protocol,
            "ip_checksum": ip_checksum,
            "src_port": src_port,
            "dst_port": dst_port,
            "flags": flags,
            "sequence": seq,
            "acknowledgement": ack,
            "window": window,
            "checksum": checksum,
            "payload": decoded_payload
        }

        try:
            producer.send('AT_RAWDATA', value=data)
            producer.flush()
            print(f"Data sent to Kafka: {data}")
        except Exception as e:
            print(f"Kafka error: {e}")

        #print("Data: ", data)

        with open("logs.txt", "a") as logfile:
            logfile.write(json.dumps(data) + "\n")

if __name__ == '__main__':
    #unittest.main()
    main()