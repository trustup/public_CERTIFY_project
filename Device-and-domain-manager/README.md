 # Device and Domain Manager 

 ## Setting Up Environment
The requirements.txt file contains the dependencies necessary for running the code. You can install (in a python venv if needed) them through 
```
pip install -r requirements.txt
```

 ## Configuration
 You can create a *.env* file that defines environment variables that configure the behaviour of the module. They are parsed before starting the services in the *globals.py* file. The current variables are:
- LOG_LEVEL (e.g. DEBUG): Define level of logging
- LOG_OUTPUT (e.g. STDOUT): Define output stream for logging. STODOUT or STDERR, otherwise will be treated as a filename. 
- DADM_PORT (e.g. 4321): Port where the server will listen
- SOFTWARE_REPO_URL (e.g. http://localhost:8081): Url where software repository is listening
- INV_REG_URL (e.g. http://localhost:8080/chain/): Url where inventory and registry is listening
- IDM_URL (e.g. https://localhost:9082/fluidos/idm/): Url where idm is listening for verification requests
- SUUA_URL (e.g. https://suua.api.certify.digital-worx.de/): Url where SUUA API is offered
 

 ## Running the Server

 To launch the Flask server, which will handle all the routes for registering and querying devices and software, use the following command:

 ```bash
 python DeviceDomainManager.py
 ```

## API Calls
The API calls cover functionality described in the flows (such as software upgrade demo) as REST APIs.

 ### Error Handling
 - **200 OK**: Success, with a field including data as needed.
 - **400 Bad Request**: Invalid inputs or missing fields.
 - **401/403 Unauthorized**: Invalid or missing credentials.
 - **500 Internal Server Error**: Errors related to server or backend logic.



 ### Testing API calls
 You can get a summary of the available API calls through:
 ```bash
 curl -X GET http://localhost:$DADM_PORT/ 
 ```


 To register a device:

 ```bash
 curl -X POST http://localhost:$DADM_PORT/register_device \
 -H "Content-Type: application/json" \
 -d '{
     "mac": "00:1B:44:11:3A:B7",
     "firmware_version": "1.0.0",
     "name": "Test Device",
     "device_id": "12345", 
     "v_cred": {
         "type": "Credentials",
         "data": "Valid"
     }
 }'
 ```

To register software:

 ```bash
 curl -X POST http://localhost:$DADM_PORT/register_software \
 -H "Content-Type: application/json" \
 -d '{
     "sw_identifier": "SW100",
     "sw_version": "v1.0",
     "name": "Software Name"
 }'
 ```

 To update status:

 ```bash
 curl -X POST http://localhost:$DADM_PORT/update_status \
 -H "Content-Type: application/json" \
 -d '{
     "device_id": "12345", // Mandatory for inventory updates
     "update_id": "update123",
     "status": "success"
 }'
 ```

