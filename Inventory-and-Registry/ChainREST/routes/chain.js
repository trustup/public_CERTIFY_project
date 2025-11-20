const { error } = require("console");
const express = require("express");
const router = express.Router();
const { Worker } = require("worker_threads");

const worker_get = require.resolve("./worker-get.js");
const worker_put = require.resolve("./worker-put.js");

// Express middleware to handle async errors
const asyncHandler = (fn) => (req, res, next) =>
  Promise.resolve(fn(req, res, next)).catch(next);

router.get("/", (req, res) => {
  res
    .status(200)
    .send(
      "This is the ledger endpoint.\nEndpoints:\nGET /chain/readEvents\nPOST /chain/createEvent"
    );
});
/**
 * @route GET /chain/readEvents
 * @group chain - Operaciones de chain
 * @param {string} deviceId.query.required - ID del dispositivo
 * @returns {object} 200 - Respuesta exitosa
 * @returns {Error}  default - Error inesperado
 */
router.get(
  "/readEvents",
  asyncHandler(async (req, res) => {
    console.log("GET /readEvents: " + new Date().toISOString());

    const worker = new Worker(worker_get);
    const requestData = {
    body: req.body,
    query: req.query,
    params: req.params,
};    startWorker(worker, res, JSON.stringify(requestData));
  })
);

/**
 * @route POST /chain/createEvent
 * @group chain - Operaciones de cadena
 * @param {object} event.body.required - Información del evento - application/json
 * @param {string} deviceId.query.required - ID del dispositivo
 * @returns {object} 200 - Éxito en la creación del evento
 * @returns {Error} default - Error inesperado
 */
router.post(
  "/createEvent",
  asyncHandler(async (req, res) => {
    console.log("POST /createEvent: " + new Date().toISOString());


    const worker = new Worker(worker_put);
    const requestData = {
    body: req.body,
    query: req.query,
    params: req.params,
};
    startWorker(worker, res, JSON.stringify(requestData));
  })
);

function startWorker(worker, res, message) {
  worker.on("message", (responseString) => {
    try {
      const response = JSON.parse(responseString);
      res.status(200).json(response);
    } catch (error) {
      res.status(500).send("Error parsing worker response: " + error);
    }
  });
  worker.on("error", (error) => {
    console.error("Worker error:", error);
    res.status(500).send("Server error: " + error.message);
  });
  worker.on("exit", (code) => {
    if (code !== 0) {
      console.error("Worker exited with code:", code);
      res.status(500).send("Worker process exited with code: " + code);
    }
  });
  worker.postMessage(message);
}

// Error handler
router.use((err, req, res, next) => {
  console.error(err.stack);
  res.status(500).send("Internal Server Error: " + error);
});

module.exports = router;
