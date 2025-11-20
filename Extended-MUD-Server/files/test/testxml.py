import xml.etree.ElementTree as ET

CONF_FILE = "conf-rule-scheme.xml"
#CONF_FILE  = "/home/juanfran/Documentos/ELECTRON/MUDS/ThreadMud-Manager/conf-rule-scheme.xml"
#CONF_MSPL = "/home/juanfran/Documentos/ELECTRON/MUDS/ThreadMud-Manager/mspl-squeleton.xml"
CONF_MSPL = "mspl-squeleton.xml"

if __name__ == "__main__":
    xmlelement = None
    tree = ET.parse(CONF_FILE)
    root = tree.getroot()
    print(root.tag)
    for child in root:
        print(child.tag)
    a = [elem.tag for elem in root.iter()]
    print(a)
    for element in tree.iter('source-address'):
        element.text = "chusta"
    print(tree.findall('packet-filter-condition'))
    for element in tree.iter('source-address'):
        print(element.text)
    print(tree.find('name').text)
    
    treee = ET.parse(CONF_MSPL)
    raizmspl = treee.getroot()
    for element in treee.findall("name"):
        print(element.text)
    for element in treee.find("configuration").getchildren():
        if element.tag == "rule-set-configuration":
            element.find("Name").text = "rule_uuid"
            element.find("configuration-rules").append(root)
        elif element.tag == "capability":
            print(element.find("Name").text)
    print([elem.tag for elem in raizmspl.iter()])
    print(raizmspl)
    print(ET.tostring(raizmspl, encoding="utf8", method='xml'))