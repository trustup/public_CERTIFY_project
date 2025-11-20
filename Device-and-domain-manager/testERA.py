import requests

def TestreconfigureDevice():
    
    #logging.info(f"Sending reconfiguration for device {device.identifier}.")
    response = requests.post("http://localhost:4321/sendMUD", data="020100020101")

    if response.status_code == 200:
        print(f"Reconfiguration of device executed successfully")
        #logging.info(f"Reconfiguration of device {device.identifier} executed successfully")
        return js
    else:
        print(f"Reconfiguration of device fails")
        #logging.info(f"Reconfiguration of device {device.identifier} fails")
        return None


if __name__ == '__main__':
    TestreconfigureDevice()
