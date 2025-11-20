const { parentPort } = require("worker_threads");
const { initGateway, conf } = require("./common");

parentPort.once("message", async (reqString) => {

    console.log("Worker started for PUT execution");
    let req = JSON.parse(reqString);
    let deviceId = req.query.deviceId; 
    let eventData = req.body; 

    let contract = await initGateway(conf);
    
    const submitResult = await contract.submitTransaction(
      "createEvent",
      JSON.stringify(eventData)
    );
  parentPort.postMessage(submitResult.toString()); 

});
