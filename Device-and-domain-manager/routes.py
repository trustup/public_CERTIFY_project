import logging
from flask import abort, request
import json
import urllib.request
import InventoryDriver
import random
import string
from globals import SOFTWARE_REPO_URL, IDM_URL, DWG_SW_REPO_DEVICE_KEY, DWG_SW_REPO_CUSTOMER_KEY
from datetime import datetime
import hmac
import hashlib
import ssl

def pretty_print_type(data_type):
    type_map = {
        str: "string",
        int: "integer",
        float: "float",
        bool: "boolean",
        dict: "json",
        list: "list"
    }
    return type_map.get(data_type, data_type.__name__)

def generate_signature(payload, key):
    hashed = hmac.new(key.encode('utf-8'), payload.encode('utf-8'), hashlib.sha256)
    return hashed.hexdigest()

def login_DWG():
    
    url = "https://dev.auth.asvin.de/auth/login"
    timestamp = int(datetime.now().timestamp())
    payload = str(timestamp) + DWG_SW_REPO_DEVICE_KEY
    device_signature = generate_signature(payload, DWG_SW_REPO_CUSTOMER_KEY)
    payload = json.dumps({
    "device_key": DWG_SW_REPO_DEVICE_KEY,
    "device_signature": device_signature,
    "timestamp": timestamp
    })
   
    loginRequest = urllib.request.Request(url, headers={
      'Content-Type': 'application/json'
        }, data=payload.encode('utf-8'))   
    try:
        f=urllib.request.urlopen(loginRequest)
    except Exception as e:
        logging.debug("Unable to connect with Login Service: %s", e)
        return "Unable to connect with Login Service", False
    else: 
        with f as response: #TODO Handle errors (ask DWGs)
            
            return json.loads(response.read().decode('utf-8'))['token'], True

# Helper method to check parameters of request in JSON body. Expects the JSON data, and a list of "parameters", which is a list of two elements: parameter name, and expected type.
# Returns True, None if no error, otherwise returns False, Error message
def checkMandatoryParameters(jsondata,parameters):
    #TODO Check that additional unexpected parameters are not included
    errorMsg='Parameter {} is mandatory and has to be of type {}'
    for param in parameters:
        if param[0] not in jsondata or not isinstance(jsondata[param[0]], param[1]):
            return False, errorMsg.format(param[0],pretty_print_type(param[1]))
    return True, None

# Method for veryfing Verifiable Credentials. Returns a tuple (v0,v1,v2):
# v0: True if no errors and validation result is ok, False otherwise
# v1,v2: Potential response in case of error or wrong verification result, with v1 being body of response with error message and v2 the code
def verify_identity(v_cred):
    #TODO Call real verification method through ARIES API
    #MOCK
    return True,None,None
    #MOCK

    stringyfied_v_cred=json.dumps(v_cred)
    body={"credential":stringyfied_v_cred}
    body = json.dumps(body).encode('utf-8')
    request = urllib.request.Request(IDM_URL+"verifyCredential", data=body, method='POST')
    request.add_header('Content-Type', 'application/json')
    try:
        gcontext = ssl.SSLContext() 
        f=urllib.request.urlopen(request, context=gcontext)
    except Exception as e:
        logging.debug("Unable to connect with IdM Agent: %s", e)
        return False, {"error": "Unable to connect with IdM Agent"}, 500
    else: 
        with f as response:
            response_data = json.loads(response.read().decode('utf-8')) 
            if "result" not in response_data:
                return False, {"error":"Unexpected response by IdM Agent: "+json.dumps(response_data)}, 503
            elif response_data["result"]==False:
                return False, {"error":"Identitiy verification failed, unauthorized: " + response_data["error"] if "error" in response_data  else "unespecified issue"}, 403
            else:
                return True,None,None
            

# Mock successful register call internally to avoid connection with I&R.
def mock_register_in_inventory_success(data):
    characters = string.ascii_letters + string.digits 
    return  {"transaction_id": ''.join(random.choice(characters) for i in range(16))}, 200


# POST: Route to register device for the software upgrade use case. Receives a JSON with data for registering plus v_creds for verifying identity
# Expected parameters:
#   JSON body: "device_id":string, "mac": string, "firmware_version": string, "name": string, "group_id":string, "v_cred": JSON following W3C Verifiable Credentials format
# Returns: 
#   Code 200, JSON with field "transaction_id" 
#   Code 401/403 invalid credentials or not authorized operation
#   Code 400 or 500 (with variants if applicable), Bad request or internal error + JSON with error message
def register_device():
    request_data = request.get_json()
    logging.debug("Received request to /register_device, body: %s", json.dumps(request_data))
    #Parse Request (this portion of the code will happen in all calls)
    #ok, error_msg = checkMandatoryParameters(request_data, [["device_id",str],["mac",str],["firmware_version",str],["name",str],["group_id",str],["v_cred",dict]])
    ok, error_msg = checkMandatoryParameters(request_data, [["device_id",str],["mac",str],["firmware_version",str],["name",str],["v_cred",dict]])
    if not ok:
        logging.debug("Bad request: %s",error_msg)
        return {"error": error_msg}, 400
    #Verify identity calling the Verify method of identity Agent
    ok, msg, code = verify_identity(request_data["v_cred"])
    if not ok:
        logging.debug("Failed identity check")
        return msg, code
    #TODO Should keep a registry for devices, e.g. for further checks in get_software. This can be left to future.
    # Register in Inventory & Registry through corresponding POST API call 
    del request_data["v_cred"]
    data_json = json.dumps(request_data).encode('utf-8')
    return mock_register_in_inventory_success(data_json)
    #return InventoryDriver.register_in_inventory(data_json)

    

# POST: Route to register software versions in software upgrade case. Receives a JSON with data for registering.
# Expected parameters:
#   JSON body: "sw_identifier": string, "sw_version": string, "name": string
# Returns: 
#   Code 200, JSON with field "transaction_id" 
#   Code 400 or 500 (with variants if applicable), Bad request or internal error + JSON with error message
def register_software():
    request_data = request.get_json()
    logging.debug("Received request to /register_software, body: %s", json.dumps(request_data))
    # Parse Request (this portion of the code will happen in all calls)
    ok, error_msg = checkMandatoryParameters(request_data, [["sw_identifier",str],["sw_version",str],["name",str]])
    if not ok:
        logging.debug("Bad request: %s",error_msg)
        return {"error": error_msg}, 400
    #TODO Register in Inventory & Registry through corresponding POST API call 
    # If some error finish, otherwise return transaction id (coming from I&R API call?)
    data_json = json.dumps(request_data).encode('utf-8')
    return mock_register_in_inventory_success(data_json)
    #return InventoryDriver.register_in_inventory(data_json)


# POST: Route retrieve a software file in the software upgrade use case.
# Expected parameters:
#   JSON body: "sw_identifier": string, "v_cred": JSON following W3C Verifiable Credentials format
# Returns: 
#   Code 200, binary file
#   Code 401/403 invalid credentials or not authorized operation
#   Code 400 or 500 (with variants if applicable), Bad request or internal error + JSON with error message
def get_software():
    request_data = request.get_json()
    logging.debug("Received request to /get_software, body: %s", json.dumps(request_data))
    # Parse Request (this portion of the code will happen in all calls)
    ok, error_msg = checkMandatoryParameters(request_data, [["sw_identifier",str],["v_cred",dict]])
    if not ok:
        logging.debug("Bad request: %s",error_msg)
        return {"error": error_msg}, 400
    # Verify identity calling the Verify method of identity Agent
    ok, msg, code = verify_identity(request_data["v_cred"])
    if not ok:
        logging.debug("Failed identity check")
        return msg, code
    #TODO Further checks of device_id allowed to retrieve software. This can be left to future.
    # Retrieve through get_software API call: Software_repo IP+port as environment variables, check specific route (/<software_id> I think)
    #return "RANDOM Bytes", 200
    #'''
    tok,ok=login_DWG()
    if not ok:
        logging.debug("Failed login at DWG Software Repo")
        return {"error": "Could not connect with software repo: login failed"}, 500
    headers = {
    "x-access-token": tok,
    "content-type": "application/json"
    }
    softwareRequest = urllib.request.Request(SOFTWARE_REPO_URL+request_data["sw_identifier"], method='GET', headers=headers) # Note: this assumes URL ends with /, managed when reading environment variable
    try:
        f=urllib.request.urlopen(softwareRequest)
    except Exception as e:
        logging.debug("Unable to connect with Software Repository: %s %s", e, softwareRequest.get_full_url())
        return {"error": "Unable to connect with Software Repository"}, 500
    else: 
        with f as response:
            response_data = response.read()  #TODO Check if 404 is returned and act accordingly
            return response_data, 200
    #'''


# POST: Update device status after update in the software upgrade use case. Receives a JSON with update data
# Expected parameters:
#   JSON body: "device_id": string, "update_id": string, "status": string 
# Returns: 
#   Code 200, JSON with field "transaction_id" 
#   Code 400 or 500 (with variants if applicable), Bad request or internal error + JSON with error message
def update_status():
    request_data = request.get_json()
    logging.debug("Received request to /update_status, body: %s", json.dumps(request_data))    
    # Parse Request (this portion of the code will happen in all calls)
    ok, error_msg = checkMandatoryParameters(request_data, [["device_id",str],["update_id",str],["status",str]])
    if not ok:
        logging.debug("Bad request: %s",error_msg)
        return {"error": error_msg}, 400
    #TODO Register in Inventory & Registry through corresponding POST API call 
    # If some error finish, otherwise return transaction id (coming from I&R API call?)
    data_json = json.dumps(request_data).encode('utf-8')
    return mock_register_in_inventory_success(data_json)
    #return InventoryDriver.register_in_inventory(data_json)
