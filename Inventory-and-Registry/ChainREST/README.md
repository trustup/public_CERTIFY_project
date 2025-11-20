# ChainREST
This is a nodejs http REST service to publis and retreive a json (IoT event from Precept IoT Devices)
## scripts
- generate-docker-image: generates the docker image to run the nodejs REST 
- run-chain-REST: run the docker-compose file which run the docker image generated with the above script
- run.sh: run both scripts
## routes
The flow is produce by 3 main files:
- chain: which is the main thread and for each request generate new worker/thread
  - worker-get: code containinng a get to the blockchain to be called by the main thread in a new worker
  - worker-put: code containning a put/save to the blockchain to be called by the main thread in a new worker
</p>
Then we have common.js which contains common code, mostly to connect to the blockchain. And index.js which contains the response of get '/'(it is not developed nor used)
 