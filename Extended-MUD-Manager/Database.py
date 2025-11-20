from datetime import date,timedelta,datetime
from sqlalchemy import create_engine, Column, Integer, String, Boolean, DateTime, ForeignKey
from sqlalchemy.ext.declarative import declarative_base
from sqlalchemy.orm import sessionmaker, relationship


Base = declarative_base()

class MUD(Base):
    __tablename__ = 'mud'

    id = Column(Integer, primary_key=True)
    mudurl = Column(String)
    mudfile = Column(String)
    device_identifier = Column(String)

class TMUD(Base):
    __tablename__ = 'tmud'

    id = Column(Integer, primary_key=True)
    tmudurl = Column(String)
    tmudfile = Column(String)
    device_identifier = Column(String)
    mspl = Column(String)

class Database:

    def __init__(self):
        # Create an engine that stores data in the local directory's DeviceDomainManager.db file.
        self.engine = create_engine('sqlite:///MUDManager.db')

        # Create all tables in the engine
        Base.metadata.create_all(self.engine)

        # Create a Session class which will be used to create a session
        Session = sessionmaker(bind=self.engine)
        self.session = Session()

    def getAllMUDs(self):
        try:
            all_muds = self.session.query(MUD).all()
            return all_muds
        except:
            return None

    def updateMUD(self,mud):
        try:
            mud_to_update = self.session.query(MUD).filter_by(id=mud.id).first()
            if mud_to_update:
                mud_to_update.mudurl = mud.mudurl
                mud_to_update.mudfile = mud.mudfile
                mud_to_update.device_identifier = mud.device_identifier

                self.session.commit()
                return True
            return False
        except:
            return None
    def getMUD(self, mudurl):
        try:
            mud = self.session.query(MUD).filter_by(mudurl=mudurl).first()
            return mud
        except:
            return None
    def createMUD(self, mudurl, mudfile, device_identifier):
        # Create and add new entry
        new_mud = MUD(mudurl=mudurl, mudfile=mudfile, device_identifier=device_identifier)
        try:
            if self.session.query(MUD).filter_by(mudurl=mudurl).first() is None:
                self.session.add(new_mud)
                self.session.commit()
                return True
            return False
        except:
            return False

    def deleteMUD(self, mud):
        try:
            mud_to_delete = self.session.query(MUD).filter_by(id=mud.id).first()
            if mud_to_delete:
                self.session.delete(mud_to_delete)
                self.session.commit()
                return True
            return False
        except:
            return False

    def printMUD(self, mud):
        print("mudurl: " + mud.mudurl + ", mudfile: " + mud.mudfile + ", device_identifier: " + mud.device_identifier)

    def getAllTMUDs(self):
        try:
            all_tmuds = self.session.query(TMUD).all()
            return all_tmuds
        except:
            return None

    def updateTMUD(self,tmud):
        try:
            tmud_to_update = self.session.query(TMUD).filter_by(id=tmud.id).first()
            if tmud_to_update:
                tmud_to_update.mudurl = tmud.mudurl
                tmud_to_update.mudfile = tmud.mudfile
                tmud_to_update.device_identifier = tmud.device_identifier
                tmud_to_update.mspl = tmud.mspl
                self.session.commit()
                return True
            return False
        except:
            return None
    def getTMUD(self, tmudurl):
        try:
            tmud = self.session.query(TMUD).filter_by(tmudurl=tmudurl).first()
            return tmud
        except:
            return None
    def createTMUD(self, tmudurl, tmudfile, device_identifier, mspl):
        # Create and add new entry
        new_tmud = TMUD(tmudurl=tmudurl, tmudfile=tmudfile, device_identifier=device_identifier, mspl=mspl)
        try:
            if self.session.query(TMUD).filter_by(tmudurl=tmudurl).first() is None:
                self.session.add(new_tmud)
                self.session.commit()
                return True
            return False
        except:
            return False

    def deleteTMUD(self, tmud):
        try:
            tmud_to_delete = self.session.query(TMUD).filter_by(id=tmud.id).first()
            if tmud_to_delete:
                self.session.delete(tmud_to_delete)
                self.session.commit()
                return True
            return False
        except:
            return False
