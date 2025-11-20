
const { parentPort } = require("worker_threads");
const { initGateway, conf } = require("./common");

let contract;

parentPort.once("message", async (reqString) => {
  try {
    console.log("Worker started for GET execution");
    let req = JSON.parse(reqString);
    let deviceId = req.query.deviceId; 

    contract = await initGateway(conf);
    const queryResult = await contract.evaluateTransaction(
      "readEventsByDeviceId",
      deviceId
    );
    parentPort.postMessage(queryResult.toString());
  } catch (error) {
    console.error("Worker error on GET: " + error.toString());
    parentPort.postMessage({ error: error.toString() });
  }
});
