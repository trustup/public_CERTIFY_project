from sqlalchemy import create_engine, Column, Integer, String, Boolean, ForeignKey
from sqlalchemy.ext.declarative import declarative_base
from sqlalchemy.orm import sessionmaker, relationship
import json

# Create a base class for our classes definitions
Base = declarative_base()


# Define the Device model
class Device(Base):
    __tablename__ = 'device'

    id = Column(Integer, primary_key=True)
    ip = Column(String)
    mac = Column(String)
    model = Column(String)
    identifier = Column(String)
    mudurl = Column(String)
    mudfile = Column(String)
    mitigation_actions = Column(String)
    inventory_id = Column(String)
    edk = Column(String)

class Threat(Base):
    __tablename__ = 'threat'
    id = Column(Integer, primary_key=True)
    identifier = Column(String)
    threat_mud_url = Column(String)
    threat_mud_file = Column(String)
    mitigation_actions = Column(String)

class Database():
    def __init__(self):

        # Create an engine that stores data in the local directory's DeviceDomainManager.db file.
        self.engine = create_engine('sqlite:///DeviceDomainManager.db')

        # Create all tables in the engine
        Base.metadata.create_all(self.engine)

        # Create a Session class which will be used to create a session
        Session = sessionmaker(bind=self.engine)
        self.session = Session()

    def createDevice(self, ip, mac, model, identifier, mudurl, mudfile, mitigation_actions, inventory_id, edk):
        # Create and add new entry
        new_device = Device(ip=ip, mac=mac, model=model, identifier=identifier, mudurl=mudurl,
                            mudfile=mudfile, mitigation_actions=mitigation_actions, inventory_id=inventory_id, edk=edk)
        try:
            if self.session.query(Device).filter_by(identifier=identifier).first() is None:
                self.session.add(new_device)
                self.session.commit()
                return True
            return False
        except:
            return False

    def createThreat(self, identifier, threat_mud_url, threat_mud_file, mitigation_actions):

        new_threat = Threat(identifier=identifier, threat_mud_url=threat_mud_url,
                            threat_mud_file=threat_mud_file, mitigation_actions=mitigation_actions)
        try:
            if self.session.query(Threat).filter_by(threat_mud_url=threat_mud_url).first() is None:
                self.session.add(new_threat)
                self.session.commit()
                return True
            return False
        except:
            return False


    def createdevice(self, device):
        return self.createDevice(device.ip, device.mac, device.model, device.identifier, device.mudurl, device.mudfile,
                                 device.mitigation_actions, device.inventory_id, device.edk)

    def createThreat(self, threat):
        return self.createThreat(threat.identifier, threat.threat_mud_url, threat.threat_mud_file,
                                 threat.mitigation_actions)

    def getDevice(self, identifier):
        try:
            device = self.session.query(Device).filter_by(identifier=identifier).first()
            return device
        except:
            return None

    def getDevicesByModel(self, model):
        try:
            devices = self.session.query(Device).filter_by(model=model).all()
            return devices
        except:
            return None

    def getThreat(self, threat_mud_url):
        try:
            threat = self.session.query(Device).filter_by(threat_mud_url=threat_mud_url).first()
            return threat
        except:
            return None

    def getAllDevices(self):
        try:
            devices = self.session.query(Device).all()
            return devices
        except:
            return None

    def getAllThreats(self):
        try:
            threats = self.session.query(Threat).all()
            return threats
        except:
            return None


    def updateDevice(self, device):

        try:
            device_to_update = self.session.query(Device).filter_by(id=device.id).first()
            if device_to_update:
                device_to_update.ip = device.ip
                device_to_update.mac = device.mac
                device_to_update.model = device.model
                device_to_update.identifier = device.identifier
                device_to_update.mudurl = device.mudurl
                device_to_update.mudfile = device.mudfile
                device_to_update.mitigation_actions = device.mitigation_actions
                device_to_update.inventory_id = device.inventory_id
                device_to_update.edk = device.edk
                self.session.commit()
                return True
            return False
        except:
            return False

    def updateThreat(self, threat):

        try:
            threat_to_update = self.session.query(Threat).filter_by(id=threat.id).first()
            if threat_to_update:
                threat_to_update.identifier = threat.identifier
                threat_to_update.threat_mud_url = threat.threat_mud_url
                threat_to_update.threat_mud_file = threat.threat_mud_file
                threat_to_update.mitigation_actions = threat.mitigation_actions
                self.session.commit()
                return True
            return False
        except:
            return False

    def deleteDevice(self, device):
        try:
            device_to_delete = self.session.query(Device).filter_by(id=device.id).first()
            if device_to_delete:
                self.session.delete(device_to_delete)
                self.session.commit()
                return True
            return False
        except:
            return False

    def deleteThreat(self, threat):
        try:
            threat_to_delete = self.session.query(Threat).filter_by(id=threat.id).first()
            if threat_to_delete:
                self.session.delete(threat_to_delete)
                self.session.commit()
                return True
            return False
        except:
            return False

    def parseDeviceJson(self, devicejson):
        device_data = devicejson
        return Device(ip=device_data['ip_address'], mac=None, model=device_data['device'],
                      identifier=device_data['uuid'], mudurl=device_data['mud-url'],
                      mudfile=None, mitigation_actions=None, inventory_id=None, edk = device_data['edk'])

    def printDevice(self, device):
        print(
            "IP: " + device.ip + ", MAC: " + device.mac + ", model: " + device.model + ", identifier: " + device.identifier + ", mudurl: " + device.mudurl + ", mudfile: " + str(device.mudfile) + ", mitigation_actions: " + str(device.mitigation_actions) + ", inventory_id: " + str(device.inventory_id))
