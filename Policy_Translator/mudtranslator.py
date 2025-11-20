import sys
import argparse
import re
import json
from Policy_Translator import msplObject
from Policy_Translator import msplgenerator
import pyangbind.lib.pybindJSON as pybindJSON
import uuid
import xmltodict

def parse_args():
    parser = argparse.ArgumentParser(description='parser for mud translation module')
    #fichero mud en json
    parser.add_argument("-i", "--input")
    args = parser.parse_args()
    return args

toacls = "ietf-access-control-list:acls.acl"
toaces = "aces.ace"

fromdevicepolicy = "from-device-policy"
fromdevicepolicy_accesslists = "access-lists"
fromdevicepolicy_accesslists_accesslist = "access-list"

todevicepolicy = "to-device-policy"
todevicepolicy_accesslists = "access-lists"
toevicepolicy_accesslists_accesslist = "access-list"


#ace { #name, matches, actions, statistics }
matches = "matches"
actions = "actions"
actions_forwarding = "forwarding" #accept, drop, reject
actions_logging = "logging" #non mandatory

statistics = "statistics" #non mandatory
statistics_matched_packets = "matched-packets"
statistics_matched_octets = "matched-octets"


#eth matches fields
ETH = "eth"
DESTINATION_MAC = "destination-mac-address"
DESTINATION_MAC_MASK = "destination-mac-address-mask"
SOURCE_MAC = "source-mac-address"
SOURCE_MAC_MASK = "source-mac-address-mask"
ETHERTYPE = "ethertype"
ethfields = [DESTINATION_MAC, DESTINATION_MAC_MASK, SOURCE_MAC, SOURCE_MAC_MASK, ETHERTYPE]

#ipv4 fields
IPV4 = "ipv4"
IPV4_DSCP = "dscp"
IPV4_ECN = "ecn"
IPV4_LENGHT = "lenght"
IPV4_TTL = "ttl"
IPV4_PROTOCOL = "protocol"
IPV4_IHL = "ihl"
IPV4_FLAGS = "flags"
IPV4_OFFSET = "offset"
IPV4_IDENTIFICATION = "identification"
IPV4_DESTINATION_NETWORK = "destination-ipv4-network"
IPV4_SOURCE_NETWORK = "source-ipv4-network"
# module: ietf-acldns
#      augment /acl:acls/acl:acl/acl:aces/acl:ace/
#        acl:matches/acl:l3/acl:ipv4/acl:ipv4:
IPV4_SRC_DNSNAME = "src-dnsname"
IPV4_SRC_IETF_DNSNAME = "ietf-acldns:src-dnsname"
IPV4_DST_DNSNAME = "dst-dnsname"
IPV4_DST_IETF_DNSNAME = "ietf-acldns:dst-dnsname"
ipv4fields = [IPV4_DSCP, IPV4_ECN, IPV4_LENGHT, IPV4_TTL, IPV4_PROTOCOL, IPV4_IHL, IPV4_FLAGS, IPV4_OFFSET, IPV4_IDENTIFICATION,\
               IPV4_DESTINATION_NETWORK, IPV4_SOURCE_NETWORK, IPV4_SRC_DNSNAME, IPV4_DST_DNSNAME, IPV4_SRC_IETF_DNSNAME, IPV4_DST_IETF_DNSNAME, "src-address", "dst-address"]

#ipv6_fields
IPV6 = "ipv6"
IPV6_DSCP = "dscp"
IPV6_ECN = "ecn"
IPV6_LENGHT = "lenght"
IPV6_TTL = "ttl"
IPV6_PROTOCOL = "protocol"
IPV6_DESTINATION_NETWORK = "destination-ipv6-network"
IPV6_SOURCE_NETWORK = "source-ipv6-network"
IPV6_FLOW_LABEL = "flow-label"
# augment /acl:acls/acl:acl/acl:aces/acl:ace/
#        acl:matches/acl:l3/acl:ipv6/acl:ipv6:
IPV6_SRC_DNSNAME = "src-dnsname"
IPV6_SRC_IETF_DNSNAME = "ietf-acldns:src-dnsname"
IPV6_DST_DNSNAME = "dst-dnsname"
IPV6_DST_IETF_DNSNAME = "ietf-acldns:dst-dnsname"
ipv6fields = [IPV6_DSCP, IPV6_ECN, IPV6_LENGHT, IPV6_TTL, IPV6_PROTOCOL, IPV6_DESTINATION_NETWORK, IPV6_SOURCE_NETWORK, IPV6_FLOW_LABEL,\
               IPV6_SRC_DNSNAME, IPV6_DST_DNSNAME, IPV6_SRC_IETF_DNSNAME, IPV6_DST_IETF_DNSNAME]

#tcp fields
TCP = "tcp"
TCP_SEQUENCE_NUMBER = "sequence-number"
TCP_ACKNOWLEDGE_NUMBER = "acknowledgement-number"
TCP_DATA_OFFSET = "data-offset"
TCP_RESERVED = "reserved"
TCP_FLAGS = "flags"
TCP_WINDOW_SIZE = "window-size"
TCP_URGENT_POINTER = "urgent-pointer"
TCP_OPTIONS = "options"
TCP_SOURCE_PORT = "source-port"
TCP_RANGE_LOWER_PORT = "lower-port"
TCP_RANGE_UPPER_PORT = "upper-port"
TCP_OPERATOR_OPERATOR = "operator"
TCP_OPERATOR_PORT = "port"
TCP_DESTINATION_PORT = "destination-port"
#augment /acl:acls/acl:acl/acl:aces/acl:ace/acl:matches
# /acl:l4/acl:tcp/acl:tcp:
TCP_DIRECTION_INITIATED = "direction-initiated"
TCP_IETF_MUD_DIRECTION_INITIATED = "ietf-mud:direction-initiated"
tcpfields = [TCP_SEQUENCE_NUMBER,TCP_ACKNOWLEDGE_NUMBER,TCP_DATA_OFFSET,TCP_RESERVED,TCP_FLAGS,TCP_WINDOW_SIZE,TCP_URGENT_POINTER,TCP_OPTIONS,\
TCP_SOURCE_PORT,TCP_RANGE_LOWER_PORT,TCP_RANGE_UPPER_PORT,TCP_OPERATOR_OPERATOR,TCP_OPERATOR_PORT,TCP_DESTINATION_PORT, TCP_IETF_MUD_DIRECTION_INITIATED, TCP_DIRECTION_INITIATED]

#udp fields
UDP = "udp"
UDP_LENGTH = "lenght"
UDP_SOURCE_PORT = "source-port"
UDP_RANGE_LOWER_PORT = "lower-port"
UDP_RANGE_UPPER_PORT = "upper-port"
UDP_OPERATOR = "operator"
UDP_PORT = "port"
UDP_DESTINATION_PORT = "destination-port"
udpfields = [UDP,UDP_LENGTH,UDP_SOURCE_PORT,UDP_RANGE_LOWER_PORT,UDP_RANGE_UPPER_PORT,UDP_OPERATOR,UDP_PORT,UDP_DESTINATION_PORT]

#icmp fields
ICMP = "icmp"
ICMP_TYPE = "type"
ICMP_CODE = "code"
ICMP_REST_OF_HEADER = "rest-of-header"

EGRESS_INTERFACE = "egress-interface"
INGRESS_INTERFACE = "igress-interface"

#matches augment
#augment /acl:acls/acl:acl/acl:aces/acl:ace/acl:matches:
MUD = "mud"
MUD_MANUFACTURER = "manufacturer"
MUD_SAME_MANUFACTURER = "same-manufacturer"
MUD_MODEL = "model"
MUD_LOCAL_NETWORKS = "local-networks"
MUD_CONTROLLER = "controller"
MUD_MY_CONTROLLER = "my-controller"

#matches layer { eth, ipv4, ipv6, tcp, udp, icmp }
matcheslayer = [ETH, IPV4, IPV6, TCP, UDP, ICMP]


def extract_udp_rule(rule):
    response = {"srcport": None, "dstport": None, "protocoltype": 17, "lenght": None}
    for attribute in rule:
        if not attribute in udpfields:
            print("Attribute " + attribute + "is no recognised as udp attribute in acl")
        if attribute == UDP_LENGTH:
            #print("Attribute length", rule[attribute], "do the corresponding action")
            #do the corresponding action for filtering a UDP packet of x lenght in MSPL
            #
            response["lenght"] = rule[attribute]
        if attribute == UDP_SOURCE_PORT:
            if UDP_OPERATOR in rule[attribute] or UDP_PORT in rule[attribute]:
                #print("Operator for port filtering", rule[attribute][UDP_OPERATOR])
                #print("Port specified for udp filtering", rule[attribute][UDP_PORT])
                response["srcport"] = rule[attribute][UDP_OPERATOR] + " " + str(rule[attribute][UDP_PORT])
            elif UDP_RANGE_LOWER_PORT in rule[attribute] or UDP_RANGE_UPPER_PORT in rule[attribute]:
                #print("lower port range", rule[attribute][UDP_RANGE_LOWER_PORT])
                #print("upper port range", rule[attribute][UDP_RANGE_UPPER_PORT])
                response["srcport"] = str(rule[attribute][UDP_RANGE_LOWER_PORT]) + "-" + str(rule[attribute][UDP_RANGE_UPPER_PORT])
        if attribute == UDP_DESTINATION_PORT:
            if UDP_OPERATOR in rule[attribute] or UDP_PORT in rule[attribute]:
                #print("Operator for port filtering", rule[attribute][UDP_OPERATOR])
                #print("Port specified for udp filtering", rule[attribute][UDP_PORT])
                response["dstport"] = rule[attribute][UDP_OPERATOR] + " " + str(rule[attribute][UDP_PORT])
            elif UDP_RANGE_LOWER_PORT in rule[attribute] or UDP_RANGE_UPPER_PORT in rule[attribute]:
                #print("lower port range", rule[attribute][UDP_RANGE_LOWER_PORT])
                #print("upper port range", rule[attribute][UDP_RANGE_UPPER_PORT])
                response["dstport"] = str(rule[attribute][UDP_RANGE_LOWER_PORT]) + "-" + str(rule[attribute][UDP_RANGE_UPPER_PORT])
    return response



def extract_tcp_rule(rule, todevice):
    response = {"srcport": None, "dstport": None, "protocoltype": 6, "direction-initiated": None}
    #todevice == true - direction of traffic comes to device, todevice == false - direction of traffic comes from device
    #flags
    sport = False
    dstport = False

    for attribute in rule:
        if not attribute in tcpfields:
            print("Attribute " + attribute + "is no recognised as tcp attribute in acl")
            #should be a return or an error message
        if attribute == TCP_SOURCE_PORT:
            #comprobaciones de range o operator
            #source-port { lower-port, upper-port || operator }
            #operator values in ietf-packet-standard -> lte, gte, eq, neq 
            if TCP_OPERATOR_OPERATOR in rule[attribute] or TCP_OPERATOR_PORT in rule[attribute]:
                #print("Operator for port filtering",rule[attribute][TCP_OPERATOR_OPERATOR])
                #print("Port specified for tcp filtering",rule[attribute][TCP_OPERATOR_PORT])
                response["srcport"] = rule[attribute][TCP_OPERATOR_OPERATOR] + " " + str(rule[attribute][TCP_OPERATOR_PORT])
            elif TCP_RANGE_LOWER_PORT in rule[attribute] or TCP_RANGE_UPPER_PORT in rule[attribute]:
                #print("lower port range", rule[attribute][TCP_RANGE_LOWER_PORT])
                #print("upper port range", rule[attribute][TCP_RANGE_UPPER_PORT])
                response["srcport"] = str(rule[attribute][TCP_RANGE_LOWER_PORT]) + "-" + str(rule[attribute][TCP_RANGE_UPPER_PORT])
        if attribute == TCP_DESTINATION_PORT:
            if TCP_OPERATOR_OPERATOR in rule[attribute] or TCP_OPERATOR_PORT in rule[attribute]:
                #print("Operator for port filtering",rule[attribute][TCP_OPERATOR_OPERATOR])
                #print("Port specified for tcp filtering",rule[attribute][TCP_OPERATOR_PORT])
                response["dstport"] = rule[attribute][TCP_OPERATOR_OPERATOR] + " " + str(rule[attribute][TCP_OPERATOR_PORT])
            elif TCP_RANGE_LOWER_PORT in rule[attribute] or TCP_RANGE_UPPER_PORT in rule[attribute]:
                #print("lower port range", rule[attribute][TCP_RANGE_LOWER_PORT])
                #print("upper port range", rule[attribute][TCP_RANGE_UPPER_PORT])
                response["dstport"] = str(rule[attribute][TCP_RANGE_LOWER_PORT]) + "-" + rule[attribute][TCP_RANGE_UPPER_PORT]
        if attribute == TCP_DIRECTION_INITIATED or attribute == TCP_IETF_MUD_DIRECTION_INITIATED:
            #ojo si la cadena que matchea es la de TCP_DIRECTION_INITIATED MAS SIMPLE
            #print("Direction initiated",rule[attribute])
            response["direction-initiated"] = rule[attribute]
    return response



def extract_ipv4_rule(rule):
    #input {'ietf-acldns:dst-dnsname': 'test.example.com', 'protocol': 6 .......}
    response = {"srcnetwork": None, "dstnetwork": None, "srcdnsname": None, "dstdnsname": None, "protocol": None}
    for attribute in rule:
        #print("Attribute ", attribute)
        #1 recorrido for (si se quiere ganar en rapidez eliminar esta comprobacion)
        if not attribute in ipv4fields:
            #print("Attribute " + attribute + " is not recognised as ipv6 attribute in acl")
            #return "false" or recognisable error
            return
        if attribute == IPV4_SOURCE_NETWORK:
            #print("matches source ipv4 network " + rule[attribute])
            response["srcnetwork"] = rule[attribute]
        #puesto para el threatmud - quitar si se remodela
        elif attribute == "src-address":
            #print("Rule src address ", rule[attribute])
            #print("matches source ipv4 network " + rule[attribute])
            response["srcnetwork"] = rule[attribute]
        elif attribute == IPV4_DESTINATION_NETWORK:
            #print("matches destination ipv4 network " + rule[attribute])
            response["dstnetwork"] = rule[attribute]
        #puesto para el threatmud - quitar si se remodela
        elif attribute == "dst-address":
            #print("matches destination ipv4 network " + rule[attribute])
            response["dstnetwork"] = rule[attribute]
        elif attribute == IPV4_SRC_IETF_DNSNAME:
            #print("matches source ipv4 domain name " + rule[attribute])
            response["srcdnsname"] = rule[attribute]
        elif attribute == IPV4_DST_IETF_DNSNAME:
            #print("matches destination ipv4 domain name " + rule[attribute])
            response["dstdnsname"] = rule[attribute]
        elif attribute == IPV4_PROTOCOL:
            #print("matches protocol number of ipv4 packets " + str(rule[attribute]))
            response["protocol"] = rule[attribute]
        else:
            return "not important attribute " + attribute

    return response

def extract_ipv6_rule(rule):
    #input {'ietf-acldns:dst-dnsname': 'test.example.com', 'protocol': 6 .......}
    response = {"srcnetwork": None, "dstnetwork": None, "srcdnsname": None, "dstdnsname": None, "protocol": None}
    for attribute in rule:
        #1 recorrido for (si se quiere ganar en rapidez eliminar esta comprobacion)
        if not attribute in ipv6fields:
            #print("Attribute " + attribute + " is not recognised as ipv6 attribute in acl")
            #return "false" or recognisable error
            return
        if attribute == IPV6_SOURCE_NETWORK:
            #print("matches source ipv6 network " + rule[attribute])
            response["srcnetwork"] = rule[attribute]
        elif attribute == IPV6_DESTINATION_NETWORK:
            #print("matches destination ipv6 network " + rule[attribute])
            response["dstnetwork"] = rule[attribute]
        elif attribute == IPV6_SRC_IETF_DNSNAME:
            #print("matches source ipv6 domain name " + rule[attribute])
            response["srcdnsname"] = rule[attribute]
        elif attribute == IPV6_DST_IETF_DNSNAME:
            #print("matches destination ipv6 domain name " + rule[attribute])
            response["dstdnsname"] = rule[attribute]
        elif attribute == IPV6_PROTOCOL:
            #print("matches protocol number of ipv6 packets " + str(rule[attribute]))
            response["protocol"] = rule[attribute]
        else:
            return "not important attribute " + attribute

    return response

def extract_eth_rule(rule):
    #input {'destination-mac-address': '.....', 'source-mac-address': '.....'}
    for attribute in rule:
        if not attribute in ethfields:
            print("Attribute " + attribute + " is not recognised as eth attribute in acl")
            #should be a return or an error message
        if attribute == DESTINATION_MAC:
            print("eth dest address ", rule[attribute])
        elif attribute == DESTINATION_MAC_MASK:
            print("eth dest mask address ", rule[attribute])
        elif attribute == SOURCE_MAC:
            print("eth src address ", rule[attribute])
        elif attribute == SOURCE_MAC_MASK:
            print("eth src mask address ", rule[attribute])
        elif attribute == ETHERTYPE:
            print("eth type ", str(rule[attribute]))
        else:
            return "not important attribute"
    return


def extract_icmp_rule(rule):
    return

def extract_rule(ace, todevice):
    #to device true if the direction of packets is to device
    #false if the direction from packets is from device
    name = ace["name"]
    matches = ace["matches"]
    #for field in matches:
        #print("field",field)
    #para sacar interseccion de los dos arrays
    intersection = [e for e in matcheslayer if e in matches]
    #print(intersection)
    tcp = False
    udp = False
    ipv6 = False
    ipv4 = False
    icmp = False
    responses = {"eth": None, "ipv4": None, "ipv6": None, "tcp": None, "udp": None, "icmp": None}
    #print("Intersection ", intersection)
    for i in intersection:
        if i == ETH:
            extract_ipv6_rule(matches[IPV6])
        elif i == IPV4:
            response = extract_ipv4_rule(matches[IPV4])
            #print("response IPV4", response)
            responses["ipv4"] = response
            ipv4 = True
        elif i == IPV6:
            response = extract_ipv6_rule(matches[IPV6])
            #print("response IPV6", response)
            responses["ipv6"] = response
            ipv6 = True
        elif i == TCP:
            response = extract_tcp_rule(matches[TCP], todevice=todevice)
            #print("response TCP : ", response)
            tcp = True
            responses["tcp"] = response
        elif i == UDP:
            response = extract_udp_rule(matches[UDP])
            responses["udp"] = response
            udp = True
        elif i == ICMP:
            extract_ipv6_rule(matches[IPV6])
            icmp = True
        else:
            #print("No filter layer type found " + i)
            return
    #print("action for rule:",ace["actions"])
    if tcp and not ipv4 and not ipv6:
        #only tcp packet filtering condition
        direction = None
        if todevice == True:
            direction = "INBOUND"
        else:
            direction = "OUTBOUND"
        
        actions = ace["actions"]
        action = actions["forwarding"]
        if actions["forwarding"] == "accept":
            action = "allow"
            #print("yes")
        crule = msplObject.yc_configuration_rule_mspl__configuration_rule_set_configuration_configuration_rule()
        ca = msplObject.yc_configuration_action_mspl__configuration_rule_set_configuration_configuration_rule_configuration_action()
        fa = msplObject.yc_filtering_action_mspl__configuration_rule_set_configuration_configuration_rule_configuration_action_filtering_action()
        #print("action", action)
        fa.filtering_action_type = action
        ca.filtering_action = fa
        fca = msplgenerator.createconfaction(action)
        cc = msplObject.yc_configuration_condition_mspl__configuration_rule_set_configuration_configuration_rule_configuration_condition()
        fc = msplgenerator.createFILTERINGCONFCONDITION(msplgenerator.createPACKETFILTERcondition(None, None, 6, responses["tcp"]["srcport"], responses["tcp"]["dstport"], direction, None, None))
        #print("OJO A LA REGLA")
        cc.filtering_configuration_condition = fc
        #print(pybindJSON.dumps(cc))
        #print(msplgenerator.dicttoxml(json.loads(pybindJSON.dumps(cc))))
        crule.name = name
        crule.configuration_action = ca
        crule.configuration_condition = cc
        crule.external_data.priority = 1
        #print(pybindJSON.dumps(crule))
        #print(msplgenerator.dicttoxml(json.loads(pybindJSON.dumps(crule))))
        return crule
    if udp and not ipv4 and not ipv6:
        #only tcp packet filtering condition
        direction = None
        if todevice == True:
            direction = "INBOUND"
        else:
            direction = "OUTBOUND"
        
        actions = ace["actions"]
        action = actions["forwarding"]
        if actions["forwarding"] == "accept":
            action = "allow"
            #print("yes")
        crule = msplObject.yc_configuration_rule_mspl__configuration_rule_set_configuration_configuration_rule()
        ca = msplObject.yc_configuration_action_mspl__configuration_rule_set_configuration_configuration_rule_configuration_action()
        fa = msplObject.yc_filtering_action_mspl__configuration_rule_set_configuration_configuration_rule_configuration_action_filtering_action()
        #print("action", action)
        fa.filtering_action_type = action
        ca.filtering_action = fa
        fca = msplgenerator.createconfaction(action)
        cc = msplObject.yc_configuration_condition_mspl__configuration_rule_set_configuration_configuration_rule_configuration_condition()
        fc = msplgenerator.createFILTERINGCONFCONDITION(msplgenerator.createPACKETFILTERcondition(None, None, 17, responses["udp"]["srcport"], responses["udp"]["dstport"], direction, None, None))
        #print("OJO A LA REGLA")
        cc.filtering_configuration_condition = fc
        #print(pybindJSON.dumps(cc))
        #print(msplgenerator.dicttoxml(json.loads(pybindJSON.dumps(cc))))
        crule.name = name
        crule.configuration_action = ca
        crule.configuration_condition = cc
        crule.external_data.priority = 1
        #print(pybindJSON.dumps(crule))
        #print(msplgenerator.dicttoxml(json.loads(pybindJSON.dumps(crule))))
        return crule
    if tcp and ipv6 and not ipv4:
        #print("responsetcp", responses["tcp"])
        #print("responsesipv6", responses["ipv6"])
        #only tcp packet filtering condition
        direction = None
        if todevice == True:
            direction = "INBOUND"
        else:
            direction = "OUTBOUND"
        
        actions = ace["actions"]
        action = actions["forwarding"]
        if actions["forwarding"] == "accept":
            action = "allow"
            #print("yes")
        crule = msplObject.yc_configuration_rule_mspl__configuration_rule_set_configuration_configuration_rule()
        ca = msplObject.yc_configuration_action_mspl__configuration_rule_set_configuration_configuration_rule_configuration_action()
        fa = msplObject.yc_filtering_action_mspl__configuration_rule_set_configuration_configuration_rule_configuration_action_filtering_action()
        #print("action", action)
        fa.filtering_action_type = action
        ca.filtering_action = fa
        fca = msplgenerator.createconfaction(action)
        cc = msplObject.yc_configuration_condition_mspl__configuration_rule_set_configuration_configuration_rule_configuration_condition()
        fc = msplgenerator.createFILTERINGCONFCONDITION(msplgenerator.createPACKETFILTERcondition(responses["ipv6"]["srcnetwork"],responses["ipv6"]["dstnetwork"], 6, responses["tcp"]["srcport"], responses["tcp"]["dstport"], direction, responses["ipv6"]["srcdnsname"], responses["ipv6"]["dstdnsname"]))
        #print("OJO A LA REGLA")
        cc.filtering_configuration_condition = fc
        #print(pybindJSON.dumps(cc))
        #print(msplgenerator.dicttoxml(json.loads(pybindJSON.dumps(cc))))
        crule.name = name
        crule.configuration_action = ca
        crule.configuration_condition = cc
        crule.external_data.priority = 1
        #print(pybindJSON.dumps(crule))
        #print(msplgenerator.dicttoxml(json.loads(pybindJSON.dumps(crule))))
        return crule
    if udp and ipv6 and not ipv4:
        #print("responsetcp", responses["tcp"])
        #print("responsesipv6", responses["ipv6"])
        #only tcp packet filtering condition
        direction = None
        if todevice == True:
            direction = "INBOUND"
        else:
            direction = "OUTBOUND"
        
        actions = ace["actions"]
        action = actions["forwarding"]
        if actions["forwarding"] == "accept":
            action = "allow"
            #print("yes")
        crule = msplObject.yc_configuration_rule_mspl__configuration_rule_set_configuration_configuration_rule()
        ca = msplObject.yc_configuration_action_mspl__configuration_rule_set_configuration_configuration_rule_configuration_action()
        fa = msplObject.yc_filtering_action_mspl__configuration_rule_set_configuration_configuration_rule_configuration_action_filtering_action()
        #print("action", action)
        fa.filtering_action_type = action
        ca.filtering_action = fa
        fca = msplgenerator.createconfaction(action)
        cc = msplObject.yc_configuration_condition_mspl__configuration_rule_set_configuration_configuration_rule_configuration_condition()
        fc = msplgenerator.createFILTERINGCONFCONDITION(msplgenerator.createPACKETFILTERcondition(responses["ipv6"]["srcnetwork"],responses["ipv6"]["dstnetwork"], 6, responses["udp"]["srcport"], responses["udp"]["dstport"], direction, responses["ipv6"]["srcdnsname"], responses["ipv6"]["dstdnsname"]))
        #print("OJO A LA REGLA")
        cc.filtering_configuration_condition = fc
        #print(pybindJSON.dumps(cc))
        #print(msplgenerator.dicttoxml(json.loads(pybindJSON.dumps(cc))))
        crule.name = name
        crule.configuration_action = ca
        crule.configuration_condition = cc
        crule.external_data.priority = 1
        #print(pybindJSON.dumps(crule))
        #print(msplgenerator.dicttoxml(json.loads(pybindJSON.dumps(crule))))
        return crule
    if udp and ipv4 and not ipv6:
        #print("responsetcp", responses["tcp"])
        #print("responsesipv6", responses["ipv6"])
        #only tcp packet filtering condition
        direction = None
        if todevice == True:
            direction = "INBOUND"
        else:
            direction = "OUTBOUND"
        
        actions = ace["actions"]
        action = actions["forwarding"]
        if actions["forwarding"] == "accept":
            action = "allow"
            #print("yes")
        crule = msplObject.yc_configuration_rule_mspl__configuration_rule_set_configuration_configuration_rule()
        ca = msplObject.yc_configuration_action_mspl__configuration_rule_set_configuration_configuration_rule_configuration_action()
        fa = msplObject.yc_filtering_action_mspl__configuration_rule_set_configuration_configuration_rule_configuration_action_filtering_action()
        #print("action", action)
        fa.filtering_action_type = action
        ca.filtering_action = fa
        fca = msplgenerator.createconfaction(action)
        cc = msplObject.yc_configuration_condition_mspl__configuration_rule_set_configuration_configuration_rule_configuration_condition()
        fc = msplgenerator.createFILTERINGCONFCONDITION(msplgenerator.createPACKETFILTERcondition(responses["ipv4"]["srcnetwork"],responses["ipv4"]["dstnetwork"], 6, responses["udp"]["srcport"], responses["udp"]["dstport"], direction, responses["ipv4"]["srcdnsname"], responses["ipv4"]["dstdnsname"]))
        #print("OJO A LA REGLA")
        cc.filtering_configuration_condition = fc
        #print(pybindJSON.dumps(cc))
        #print(msplgenerator.dicttoxml(json.loads(pybindJSON.dumps(cc))))
        crule.name = name
        crule.configuration_action = ca
        crule.configuration_condition = cc
        crule.external_data.priority = 1
        #print(pybindJSON.dumps(crule))
        #print(msplgenerator.dicttoxml(json.loads(pybindJSON.dumps(crule))))
        return crule
    if tcp and ipv4 and not ipv6:
        #print("responsetcp", responses["tcp"])
        #print("responsesipv6", responses["ipv6"])
        #only tcp packet filtering condition
        direction = None
        if todevice == True:
            direction = "INBOUND"
        else:
            direction = "OUTBOUND"
        
        actions = ace["actions"]
        action = actions["forwarding"]
        if actions["forwarding"] == "accept":
            action = "allow"
            #print("yes")
        crule = msplObject.yc_configuration_rule_mspl__configuration_rule_set_configuration_configuration_rule()
        ca = msplObject.yc_configuration_action_mspl__configuration_rule_set_configuration_configuration_rule_configuration_action()
        fa = msplObject.yc_filtering_action_mspl__configuration_rule_set_configuration_configuration_rule_configuration_action_filtering_action()
        #print("action", action)
        fa.filtering_action_type = action
        ca.filtering_action = fa
        fca = msplgenerator.createconfaction(action)
        cc = msplObject.yc_configuration_condition_mspl__configuration_rule_set_configuration_configuration_rule_configuration_condition()
        fc = msplgenerator.createFILTERINGCONFCONDITION(msplgenerator.createPACKETFILTERcondition(responses["ipv4"]["srcnetwork"],responses["ipv4"]["dstnetwork"], 6, responses["tcp"]["srcport"], responses["tcp"]["dstport"], direction, responses["ipv4"]["srcdnsname"], responses["ipv4"]["dstdnsname"]))
        #print("OJO A LA REGLA")
        cc.filtering_configuration_condition = fc
        #print(pybindJSON.dumps(cc))
        #print(msplgenerator.dicttoxml(json.loads(pybindJSON.dumps(cc))))
        crule.name = name
        crule.configuration_action = ca
        crule.configuration_condition = cc
        crule.external_data.priority = 1
        #print(pybindJSON.dumps(crule))
        #print(msplgenerator.dicttoxml(json.loads(pybindJSON.dumps(crule))))
        return crule

def extract_from_mspl_module(jsonmodule):
    mspls_xml = []
    try:
        mspls = jsonmodule["mspl"]
        for mspl in mspls:
            rooted = {'root': mspl}
            mspls_xml.append(xmltodict.unparse(rooted))
    except:
        print("error parsing input json")
    return mspls_xml




def threatmudtranslate():
    return


def translate(mud):
    access_control_list = mud['ietf-access-control-list:acls']
    acls = access_control_list['acl']
    # for index, acl in enumerate(access_control_list['acl']):
    #     #print("acl ", index, acl)
    
    ##print("MUD ", mud)
    try:
        acl_todevice = mud['ietf-mud:mud'][todevicepolicy][todevicepolicy_accesslists][toevicepolicy_accesslists_accesslist]
    except:
        print("Is Threat MUD")
        acl_todevice = mud['ietf-threatmud:mud'][todevicepolicy][todevicepolicy_accesslists][toevicepolicy_accesslists_accesslist]
    to_device = []
    for acl in acl_todevice:
        print("name: " + acl['name'])
        to_device.append(acl['name'])

    try:
        acl_fromdevice = mud['ietf-mud:mud'][fromdevicepolicy][fromdevicepolicy_accesslists][fromdevicepolicy_accesslists_accesslist]
    except:
        acl_fromdevice = mud['ietf-threatmud:mud'][fromdevicepolicy][fromdevicepolicy_accesslists][fromdevicepolicy_accesslists_accesslist]
    from_device = []
    for acl in acl_fromdevice:
        print("name: " + acl['name'])
        from_device.append(acl['name'])
    rules = []
    for i, acl in enumerate(acls):
        #print("acl", i, "name:",acl['name'], ", type:", acl['type'])
        for index,ace in enumerate(acl['aces']['ace']):
            #print("\tace ", index, ace)
            rules.append(extract_rule(ace, acl['name'] in to_device))
        #print("")
    #print("rules", rules)
    rule_set_conf = msplObject.yc_rule_set_configuration_mspl__configuration_rule_set_configuration()

    mspl = msplgenerator.create_mspl_object("mspl_example", "Filtering_L4", "rule_set"+str(uuid.uuid4()), "deny")
    configuration = msplObject.yc_configuration_mspl__configuration()
    for rule in rules:
        #print("rule ", rule)
        msplgenerator.add_rule_to_configurationrule(mspl, rule)
    #print(pybindJSON.dumps(mspl))
    mspls_to_return = []
    if rules:
        #in bytes
        #mspl_from_mud_acls = msplgenerator.dicttoxml(json.loads(pybindJSON.dumps(mspl)))
        #in str
        mspl_from_mud_acls = replace_underscores_with_dashes(str(msplgenerator.dicttoxml(json.loads(pybindJSON.dumps(mspl)))))
        mspls_to_return.append(mspl_from_mud_acls)
    #print(mspls_to_return)
    if 'umu-mspl-list:mspls' in mud:
        extracted_mud_mspls = extract_from_mspl_module(mud['umu-mspl-list:mspls'])
        sanitized_mspls = []
        #for sustitution of 'root' tag for 'mspl' tag
        for emspl in extracted_mud_mspls:
            sanitized_mspls.append(replace_root_with_mspl(emspl))
        extracted_mud_mspls = sanitized_mspls
        if len(extracted_mud_mspls) > 0:
            mspls_to_return.extend(extracted_mud_mspls)
    return mspls_to_return

#ietf-mud:mud, ietf-access-control-list:acls, #ietf-threatmud:mud, umu-mspl-list:mspls({mspl[]})

def translate_mud(mud):
    if 'ietf-access-control-list:acls' in mud:
        #translate acls
        access_control_list = mud['ietf-access-control-list:acls']
        acls = access_control_list['acl']
        try:
            acl_todevice = mud['ietf-mud:mud'][todevicepolicy][todevicepolicy_accesslists][toevicepolicy_accesslists_accesslist]
        except:
            print("Is Threat MUD")
            acl_todevice = mud['ietf-threatmud:mud'][todevicepolicy][todevicepolicy_accesslists][toevicepolicy_accesslists_accesslist]
        to_device = []
        for acl in acl_todevice:
            #print("name: " + acl['name'])
            to_device.append(acl['name'])

        try:
            acl_fromdevice = mud['ietf-mud:mud'][fromdevicepolicy][fromdevicepolicy_accesslists][fromdevicepolicy_accesslists_accesslist]
        except:
            acl_fromdevice = mud['ietf-threatmud:mud'][fromdevicepolicy][fromdevicepolicy_accesslists][fromdevicepolicy_accesslists_accesslist]
        from_device = []
        for acl in acl_fromdevice:
            #print("name: " + acl['name'])
            from_device.append(acl['name'])
        rules = []
        for i, acl in enumerate(acls):
            #print("acl", i, "name:",acl['name'], ", type:", acl['type'])
            for index,ace in enumerate(acl['aces']['ace']):
                #print("\tace ", index, ace)
                rules.append(extract_rule(ace, acl['name'] in to_device))
            #print("")
        #print("rules", rules)
        rule_set_conf = msplObject.yc_rule_set_configuration_mspl__configuration_rule_set_configuration()

        mspl = msplgenerator.create_mspl_object("mspl_example", "Filtering_L4", "rule_set"+str(uuid.uuid4()), "deny")
        configuration = msplObject.yc_configuration_mspl__configuration()
        for rule in rules:
            msplgenerator.add_rule_to_configurationrule(mspl, rule)
        #print(pybindJSON.dumps(mspl))
        mspl_from_mud_acls = msplgenerator.dicttoxml(json.loads(pybindJSON.dumps(mspl)), custom_root="mspl")
        #print(mspl_from_mud_acls)
        return mspl_from_mud_acls
    else:
        return None

def translate_acls():
    return


def main_test_translatefunction():
    #args = parse_args()
    #mudfile = open(args.input)
    mudfile = open(sys.argv[1])
    mud = json.load(mudfile)
    #buenaprint(translate_mud(mud))
    #mala
    lista = translate(mud)
    for l in lista:
        print(f"{l}\n")

def replace_root_with_mspl(xml_string):
    updated_xml = re.sub(r'<root>', '<mspl>', xml_string)
    # Replace </root> with </mspl>
    updated_xml = re.sub(r'</root>', '</mspl>', updated_xml)
    return updated_xml

def replace_underscores_with_dashes(xml_string):
    # Replace opening tags
    updated_xml = re.sub(r'<(/?)(\w+_\w+)', lambda match: f'<{match.group(1)}{match.group(2).replace("_", "-")}', xml_string)
    # Replace closing tags
    updated_xml = re.sub(r'</(\w+_\w+)', lambda match: f'</{match.group(1).replace("_", "-")}', updated_xml)
    # Replace <root> with <mspl>
    updated_xml = re.sub(r'<root>', '<mspl>', updated_xml)
    # Replace </root> with </mspl>
    updated_xml = re.sub(r'</root>', '</mspl>', updated_xml)
    return updated_xml


def main_fixed():
    args = parse_args()
    mudfile = open(args.input)
    mud = json.load(mudfile)
    access_control_list = mud['ietf-access-control-list:acls']
    acls = access_control_list['acl']

    acl_todevice = mud['ietf-mud:mud'][todevicepolicy][todevicepolicy_accesslists][toevicepolicy_accesslists_accesslist]
    to_device = []
    for acl in acl_todevice:
        #print("name: " + acl['name'])
        to_device.append(acl['name'])

    acl_fromdevice = mud['ietf-mud:mud'][fromdevicepolicy][fromdevicepolicy_accesslists][fromdevicepolicy_accesslists_accesslist]
    from_device = []
    for acl in acl_fromdevice:
        #print("name: " + acl['name'])
        from_device.append(acl['name'])
    rules = []
    for i, acl in enumerate(acls):
        #print("acl", i, "name:",acl['name'], ", type:", acl['type'])
        for index,ace in enumerate(acl['aces']['ace']):
            #print("\tace ", index, ace)
            rules.append(extract_rule(ace, acl['name'] in to_device))
        #print("")
    #print("rules", rules)
    rule_set_conf = msplObject.yc_rule_set_configuration_mspl__configuration_rule_set_configuration()

    mspl = msplgenerator.create_mspl_object("mspl_example", "Filtering_L4", "rule_set"+str(uuid.uuid4()), "deny")
    configuration = msplObject.yc_configuration_mspl__configuration()
    for rule in rules:
        #print("rule ", rule)
        msplgenerator.add_rule_to_configurationrule(mspl, rule)
    #print(pybindJSON.dumps(mspl))
    mspl_from_mud_acls = msplgenerator.dicttoxml(json.loads(pybindJSON.dumps(mspl)))
    #print(mspl_from_mud_acls)
    mspls_to_return = []
    mspls_to_return.append(mspl_from_mud_acls.decode("utf-8"))
    if 'umu-mspl-list:mspls' in mud:
        extracted_mud_mspls = extract_from_mspl_module(mud['umu-mspl-list:mspls'])
        if len(extracted_mud_mspls) > 0:
            mspls_to_return = mspls_to_return.append(extracted_mud_mspls.decode("utf-8"))
    print(mspls_to_return)
    #que puede pasar
    #que no haya ninguna acl
    #que no haya ningun ace dentro de un acl
    #(el campo name me la pela - no en el futuro) que no haya campo match o size(match) == 0 || que no haya campo actions o size(actions) == 0

    
    
    
def testjsontoxml():
    args = parse_args()
    mudfile = open(args.input)
    mud = json.load(mudfile)
    extracted_mud_mspls = extract_from_mspl_module(mud['umu-mspl-list:mspls'])
    return

if __name__ == "__main__":
    #main_fixed()
    #testjsontoxml()
    main_test_translatefunction()
