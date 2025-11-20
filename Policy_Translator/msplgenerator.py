from Policy_Translator import msplObject
import json
#import pyangbind.lib.pybindJSON as pybindJSON
import pyangbind.lib.pybindJSON as pybindJSON
#from pyangbind.lib.xmlserialise import pybindXMLEncoder
#from pyangbind.lib.serialiseXML import pybindIETFXMLEncoder
from dicttoxml import dicttoxml
import pyangbind.lib.serialise as serialise
import uuid

def testfilteringaction():
    rule_set_configuration = msplObject.yc_rule_set_configuration_mspl__configuration_rule_set_configuration()
    conf_rule = msplObject.yc_configuration_rule_mspl__configuration_rule_set_configuration_configuration_rule()
    action = msplObject.yc_configuration_action_mspl__configuration_rule_set_configuration_configuration_rule_configuration_action()
    filteringaction = msplObject.yc_filtering_action_mspl__configuration_rule_set_configuration_configuration_rule_configuration_action_filtering_action()
    filteringaction._set_filtering_action_type('deny')
    action._set_filtering_action(filteringaction)
    conf_rule._set_configuration_action(action)
    conf_rule._set_name("regla 1")
    conf_rule._set_configuration_condition(msplObject.yc_configuration_condition_mspl__configuration_rule_set_configuration_configuration_rule_configuration_condition())
    rules = []
    rules.append(conf_rule)
    rule_set_configuration._set_name("configuration set")
    rule_set_configuration._set_default_action("deny")
    #rule_set_configuration.configuration_rule.add(conf_rule._get_name(),conf_rule)
    #rule_set_configuration.configuration_rule.add(conf_rule)
    rule_set_configuration.configuration_rule.add(conf_rule._get_name())
    rule_set_configuration.configuration_rule[conf_rule._get_name()] = conf_rule  


    #conf rule 2
    conf_rule_2 = msplObject.yc_configuration_rule_mspl__configuration_rule_set_configuration_configuration_rule()
    conf_action2 = msplObject.yc_configuration_action_mspl__configuration_rule_set_configuration_configuration_rule_configuration_action()
    filtering2 = msplObject.yc_filtering_action_mspl__configuration_rule_set_configuration_configuration_rule_configuration_action_filtering_action()
    filtering2._set_filtering_action_type('(allow')
    conf_action2._set_filtering_action(filtering2)
    conf_rule_2._set_name("regla 2")
    conf_rule_2._set_configuration_action(conf_action2)
    rule_set_configuration.configuration_rule.add(conf_rule_2._get_name())
    rule_set_configuration.configuration_rule[conf_rule_2._get_name()] = conf_rule_2
    #print(("rule_set_configuration: " + pybindJSON.dumps(rule_set_configuration))
    #print((type(pybindJSON.dumps(rule_set_configuration)))
    jo = json.loads(pybindJSON.dumps(rule_set_configuration))
    
    #print((jo)
    #print((dicttoxml(jo))
    ##print(("xml: ", pybindIETFXMLEncoder.encode(rule_set_configuration))

def create_conf_rule(confcondition,confaction=None, priority=None, simpleaction=None):
    #print(("confaction ", confaction)
    default_priority = 1
    conf_rule = msplObject.yc_configuration_rule_mspl__configuration_rule_set_configuration_configuration_rule()
    if confaction != None:
        conf_rule._set_configuration_action(confaction)
    else:
        #print(("Setting configuration action"+simpleaction)
        action = createconfaction(simpleaction)
        conf_rule._set_configuration_action(action)
    conf_rule._set_configuration_condition(confcondition)
    external_data = None
    if priority != None:
        external_data = msplObject.yc_external_data_mspl__configuration_rule_set_configuration_configuration_rule_external_data()
        external_data._set_priority(priority)
    else:
        external_data = msplObject.yc_external_data_mspl__configuration_rule_set_configuration_configuration_rule_external_data()
        external_data._set_priority(default_priority)
    conf_rule._set_external_data(external_data)
    return conf_rule


def createconfaction(filteringpacket_action):
    action = msplObject.yc_configuration_action_mspl__configuration_rule_set_configuration_configuration_rule_configuration_action()
    filteringaction = msplObject.yc_filtering_action_mspl__configuration_rule_set_configuration_configuration_rule_configuration_action_filtering_action()
    filteringaction._set_filtering_action_type(filteringpacket_action)
    action._set_filtering_action(filteringaction)
    return action

def createFILTERINGCONFCONDITION(pfc=None, statefulcondition = None, aplication_layer_condition = None):
    conf_p_f_c = msplObject.yc_filtering_configuration_condition_mspl__configuration_rule_set_configuration_configuration_rule_configuration_condition_filtering_configuration_condition()
    conf_p_f_c._set_packet_filter_condition(pfc)
    return conf_p_f_c

def createPACKETFILTERcondition(ipsource=None, ipdest=None, protocol=None, sourceport=None, destport=None, direction=None, srcdnsname=None, dstdnsname=None):
    packet_f_condition = msplObject.yc_packet_filter_condition_mspl__configuration_rule_set_configuration_configuration_rule_configuration_condition_filtering_configuration_condition_packet_filter_condition()
    if ipsource != None:
        packet_f_condition._set_source_address(ipsource)    
    if ipdest != None:
        packet_f_condition._set_destination_address(ipdest)
    if protocol != None:
        packet_f_condition._set_protocol_type(protocol)
    if sourceport != None:
        packet_f_condition._set_source_port(sourceport)
    if destport != None:
        packet_f_condition._set_destination_port(destport)
    if direction != None:
        packet_f_condition._set_direction(direction)
    if srcdnsname != None:
        packet_f_condition._set_source_dnsname(srcdnsname)
    if dstdnsname != None:
        packet_f_condition._set_destination_dnsname(dstdnsname)
    return packet_f_condition

def create_mspl_object(msplname, conf_capability, rule_set_name, default_action):
    mspl = msplObject.mspl()
    mspl.configuration = msplObject.yc_configuration_mspl__configuration()
    mspl.configuration.rule_set_configuration = msplObject.yc_rule_set_configuration_mspl__configuration_rule_set_configuration()
    mspl.name = msplname
    mspl.configuration.capability = conf_capability
    mspl.configuration.rule_set_configuration._set_name(rule_set_name)
    mspl.configuration.rule_set_configuration._set_default_action("deny")
    return mspl

def add_rule_to_configurationrule(mspl, rule):
    mspl.configuration.rule_set_configuration.configuration_rule[rule.name] = rule



#testfilteringaction()
#testall()
