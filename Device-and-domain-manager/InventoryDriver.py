import logging
import json
import urllib.request
import os

HOST = os.getenv("INVENTORY_HOST", "155.54.95.211")
PORT = os.getenv("INVENTORY_PORT", "3000")

# URLs for specific endpoints
CREATE_EVENT_URL = f"http://{HOST}:{PORT}/chain/createEvent"
READ_EVENTS_URL = f"http://{HOST}:{PORT}/chain/readEvents"

def register_in_inventory(data):
    """
    Register an event in the inventory system by sending a POST request.
    """
    try:
        #data_string = json.dumps(str(data))
        request = urllib.request.Request(CREATE_EVENT_URL, data=data, method='POST')
        request.add_header('Content-Type', 'application/json')
        f = urllib.request.urlopen(request)
    except Exception as e:
        logging.debug("Unable to connect with Inventory & Registry: %s", e)
        return {"error": "Unable to connect with Inventory & Registry"}, 500
    else:
        with f as response:
            response_data = json.loads(response.read().decode('utf-8'))
            if "transaction_id" in response_data:
                return response_data, 200
            else:
                logging.debug("Error response from chain: %s", json.dumps(response_data))
                return {"error": "No transaction ID returned"}, 500

def get_events_by_device_id(device_id):
    """
    Retrieve events from the inventory system by device ID using a GET request.
    """
    get_url = f"{READ_EVENTS_URL}?deviceId={device_id}"
    
    request = urllib.request.Request(get_url, method='GET')
    request.add_header('Content-Type', 'application/json')
    try:
        f = urllib.request.urlopen(request)
    except Exception as e:
        logging.debug("Unable to connect with Inventory & Registry: %s", e)
        return {"error": "Unable to connect with Inventory & Registry"}, 500
    else:
        with f as response:
            response_data = json.loads(response.read().decode('utf-8'))
            return response_data, 200
