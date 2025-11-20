


class MUDFile:

    def __init__(self, id, mudurl, device, last_check, created_at):


        self.id = id
        self.mudurl = mudurl
        self.device = device
        self.last_check = last_check
        self.created_at = created_at


    def getID(self):
        return self.id

    def getMUDUrl(self):
        return self.mudurl

    def getDevice(self):
        return self.device

    def getLast_Check(self):
        return self.last_check

    def getCreated_at(self):
        return self.created_at

    def setMUDUrl(self,mudurl):
        self.mudurl = mudurl

    def setDevice(self, device):
        self.device = device

    def setLast_Check(self,lcheck):
        self.last_check = lcheck

    def setCreatedat(self,createdat):
        self.created_at = createdat

