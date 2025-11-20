import untangle
import xmltodict
import msplObject
import json
import sys
import xml.etree.ElementTree as E
import xml_to_dict_test
import pyangbind.lib.pybindJSON as pybindJSON
from pyangbind.lib.serialise import pybindJSONDecoder

def testlibrary():
    obj = untangle.parse('../test/results/scada.xml')
    print(obj)
    childs = obj.mspl.children
    print(childs)
    for son in childs:
        for a in dir(son):
            print("a",a)
        son.cdata = son.cdata.replace(" ", "")
        print(type(son),"-",son.cdata,"-")
    # for son in childs:
    #     print("primer recorrido",son.get_elements())
    #     print(son.get_elements())
    #     print(son.__contains__("name"))
    #     print("name", son['name'])
    #     #print(son.add_cdata("me lo invento"))
    #     son.cdata = son.cdata.replace(" ","")
    #     print(son.cdata, "tomalo")

    for son in childs:
        print("TESTING",son.get_elements())
        print("T2 name->", son._name)
    
    cr = obj.mspl.get_elements("configuration")
    print(cr)
    print(cr[0])
    configuration_rules = cr[0].rule_set_configuration.get_elements("configuration_rule")
    print("CONFIGURATION RULES",configuration_rules)
    print("Eh", cr[0].rule_set_configuration)



def parseXmlToJson(xml):
    response = {}

    for child in list(xml):
        if len(list(child)) > 0:
            response[child.tag] = parseXmlToJson(child)
        else:
            response[child.tag] = child.text or ''

        # one-liner equivalent
        # response[child.tag] = parseXmlToJson(child) if len(list(child)) > 0 else child.text or ''

    return response

def manual_xml_to_json():
    tree = E.parse(sys.argv[1])
    parsed = parseXmlToJson(tree)
    print(parsed)

def translatexmlmspltoobject():
    xml_file = open(sys.argv[1], "r")
    xml_output = xml_file.read()
    print(xml_output)
    data_dict = xmltodict.parse(xml_output)
    print(data_dict)
    json_data = json.dumps(data_dict)
    print(json_data)

def translate():
    xml_file = open(sys.argv[1], "r")
    xml_output = xml_file.read().encode()
    jsonist = xml_to_dict_test.translate(xml_output)
    print(jsonist)
    print(type(jsonist))
    print("str ", str(jsonist))
    json_loads_obj = json.loads(str(jsonist))
    print("json loads ", json_loads_obj)
    print(json_loads_obj["mspl"])
    mspl = pybindJSON.loads(json.dumps(json_loads_obj["mspl"]), msplObject, "mspl")
    print(mspl.name)
    print(mspl.configuration)

if __name__ == "__main__":
    #testlibrary()
    #translatexmlmspltoobject()
    #manual_xml_to_json()
    translate()
