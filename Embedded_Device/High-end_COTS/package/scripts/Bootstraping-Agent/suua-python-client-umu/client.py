import json
import random
import string
import requests
from datetime import datetime
import hmac
import hashlib
import sys
from time import sleep
from authenticator import authenticate



## generate verifiable credentials
# def authenticate(policy):
#     retval = open("example_output_vc.json", "r").read()

# example_policy = open("example_policy.json", "r").read()
# v_cred = authenticate(example_policy)


# Specify the path to your JSON file
CONFIG_DATA = "config.json"
SOFTWARE_FILE_PATH = "software_file.txt"

# localhost
#AUTH_BASE_API_URL = "http://localhost:6769"
#SUUA_BASE_API_URL = "http://localhost:8002"
#SR_BASE_API_URL = "https://sr.certify.digital-worx.de"
#DDM_BASE_API_URL = "http://localhost:4321"

# #cloud
AUTH_BASE_API_URL = "https://dev.auth.asvin.de"
SUUA_BASE_API_URL="https://suua.api.certify.digital-worx.de"
SR_BASE_API_URL="https://sr.certify.digital-worx.de"
DDM_BASE_API_URL="http://10.0.0.8:4321"

with open("example_output_vc.json", "r") as file:
    v_cred = json.load(file)
    

with open("../politik.json", "r") as file:
    policy1 = json.load(file)
    policy2 = policy1


# Read JSON data from the file
with open(CONFIG_DATA, "r") as file:
    config_data = json.load(file)
    if not config_data["suua"]["device_key"] or not config_data["suua"]["customer_key"]:
        print("Error! customer or device key is not defined in config.json")
        sys.exit(1)

def generate_signature(payload, key):
    hashed = hmac.new(key.encode('utf-8'), payload.encode('utf-8'), hashlib.sha256)
    return hashed.hexdigest()

    
def login():
    timestamp = int(datetime.now().timestamp())
    payload = str(timestamp) + config_data["suua"]["device_key"]
    device_signature = generate_signature(payload, config_data["suua"]["customer_key"])
    payload = json.dumps({
      "device_key": config_data["suua"]["device_key"],
      "device_signature": device_signature,
      "timestamp": timestamp
    })

    response = requests.post(AUTH_BASE_API_URL+"/auth/login", headers={
      'Content-Type': 'application/json'
        }, data=payload)
    if response.status_code != 200:
        return None
    print("Token generation successful")
    return response.json()['token']

def register_device():
    v_cred = authenticate(policy1)
    device_register_data = json.dumps({
        "mac": config_data["device"]["mac"],
        "firmware_version": config_data["device"]["firmware_version"],
        "name": config_data["device"]["name"],
        "v_cred": v_cred
    })
    response = requests.post(SUUA_BASE_API_URL+"/api/device/register", data=device_register_data, headers=headers)
    
    if response.status_code == 200:
        print("Register device API successful:", response.text)
        return response.json()
    else:
        print("Error! device register API failed:", response.status_code)
        return None


def rollout_success(rollout_id):
    v_cred = authenticate(policy2)
    rollout_success_data = json.dumps({
        "mac": config_data["device"]["mac"],
        "firmware_version": config_data["device"]["firmware_version"],
        "rollout_id": rollout_id,
        "v_cred": v_cred
    })
    response = requests.post(SUUA_BASE_API_URL+"/api/device/success/rollout", data=rollout_success_data, headers=headers)
    
    if response.status_code == 200:
        print("Rollout success API successful:", response.text)
        return response.text
    else:
        print("Error! rollout success API failed:", response.status_code)
        return None

def next_rollout():
    next_rollout_data = json.dumps({
        "mac": config_data["device"]["mac"],
        "firmware_version": config_data["device"]["firmware_version"],
        "v_cred": v_cred
    })
    response = requests.post(SUUA_BASE_API_URL+"/api/device/next/rollout", data=next_rollout_data, headers=headers)
    if response.status_code == 200:
        print("Next rollout API successful:", response.json())
        return response.json()
    else:
        print("Error! next rollout API failed: ", response.text)
        return None


def upload_software():
    software_file=[("firmware",("software_file.txt",open(SOFTWARE_FILE_PATH,"rb"),"text/plain"))]
    response = requests.post(SR_BASE_API_URL+"/firmware", files=software_file, headers={
        "x-access-token": token
    })
    if response.status_code == 200:
        print("Upload software API successful: ", response.json())
        return response.json()
    else:
        print("Error! upload software API failed: ", response.status_code)
        return None

'''
def retrieve_software(swId):
    response = requests.get(SR_BASE_API_URL+"/firmware/"+swId, headers=headers)
    if response.status_code == 200:
        print("Retrieve software API successful: ", response.text)
        return response.text
    else:
        print("Error! retrieve software API failed: ", response.status_code)
        return None
        '''

def get_software(sw_identifier):
    v_cred = authenticate(policy2)
    get_sw_data = json.dumps({
        "sw_identifier": sw_identifier,
        "v_cred": v_cred,
    })
    response = requests.post(DDM_BASE_API_URL+"/get_software", data=get_sw_data, headers=headers)
    if response.status_code == 200:
        print("Get software API successful:", response.text)
        return response.text
    else:
        print("Error! Get software API failed: ", response.status_code)
        return None


def check_suua_be_status():
    print("Call check SSUA status API")
    response = requests.get(SUUA_BASE_API_URL)
    if response.status_code == 200:
        print("SUUA status got successfully", response.text)
    else:
        print("Error in getting SUUA status:", response)

def check_sw_repo_status():
    print("Call check software repo status API")
    response = requests.get(SR_BASE_API_URL)
    if response.status_code == 200:
        print("Software Repository status API successful", response.text)
    else:
        print("Error! check software repo status API failed: ", response)

def install_software():
    print("software installed successfully")

def complete_software_update_flow():
    res = register_device()
    if res is None:
        sys.exit("Client Exit! Error in register device!")
    rolloutRes = next_rollout()
    if rolloutRes is None:
        sys.exit("Client Exit! Error in next rollout")
    sw_identifier = rolloutRes["sw_identifier"]
    print(sw_identifier)
    sw_identifier = "QmVJwv27ieRuya36ztgkRyP77ArbizmcuUajd52pf9wcYP"
    res = get_software(sw_identifier)
    install_software()
    if res is None:
        sys.exit("Client Exit! Error in retrieve software")
    res = rollout_success(rolloutRes["rollout_id"])
    if res is None:
        sys.exit("Client Exit! Error in rollout success")

def listen_and_apply_software_updates():
    listen_for_update = True
    while listen_for_update:
        #rolloutRes = next_rollout()
        rolloutRes = {"sw_identifier":"QmVJwv27ieRuya36ztgkRyP77ArbizmcuUajd52pf9wcYP"} #Mocked
        if rolloutRes is None:
            sleep(60)
        else:
            sw_identifier = rolloutRes["sw_identifier"]
            print(sw_identifier)
            sw_identifier = "QmVJwv27ieRuya36ztgkRyP77ArbizmcuUajd52pf9wcYP" #TODO Remove hardcoded identifier
            res = get_software(sw_identifier)
            if res is None:
                sys.exit("Client Exit! Error in retrieve software")
            install_software()
            res = rollout_success(rolloutRes["rollout_id"])
            if res is None:
                sys.exit("Client Exit! Error in rollout success")
            print(res)
            listen_for_update=False
    
    



def print_help():
    print("python3 client.py  [command] [arguments...]")
    print("Commands:")
    print("   help                                  Print this message.")
    print("   version                               Get API version information.")
    print("   status be/sr                          Get server status")
    print("   device register/rolnext/rolsuc        Connect to device APIs ")
    print("   software upload/retrieve/install      Connect to software APIs")
    print("   demo register/listen_updates          Do demo flows")
    
token = login()
if token is None:
    print("Error! Failed to get Auth token")
    sys.exit(1)

headers = {
    "x-access-token": token,
    "content-type": "application/json"
}

def main():
    import sys
    if len(sys.argv) < 3:
        print_help()
        return
    
    command = sys.argv[1]
    
    if command == "status":
        argument = sys.argv[2]
        if argument == "be":
            check_suua_be_status()
        elif argument == "sr":
            check_sw_repo_status() 
        else:
            print_help()

    elif command == "device":
        argument = sys.argv[2]
        if argument == "register":
            register_device()
        elif argument == "rolsuc":
            rollout_success(config_data["device"]["rollout_id"])
        elif argument == "rolnext":
            next_rollout()
        else:
            print_help()

    elif command == "software":
        argument = sys.argv[2]
        if argument == "upload":
            upload_software() 
        elif argument == "get":
            get_software("QmVJwv27ieRuya36ztgkRyP77ArbizmcuUajd52pf9wcYP")
        elif argument == "install":
            complete_software_update_flow()   
        else:
            print_help()
    elif command == "demo":
        argument = sys.argv[2]
        if argument == "register":
            register_device() 
        elif argument == "listen_updates":
            listen_and_apply_software_updates()
        else:
            print_help()
    else:
        print_help()

if __name__ == "__main__":
    main()

