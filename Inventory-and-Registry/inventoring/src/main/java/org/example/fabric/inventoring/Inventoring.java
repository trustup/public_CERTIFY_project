/*
 * SPDX-License-Identifier: Apache-2.0
 */

package org.example.fabric.inventoring;

/*
 * SPDX-License-Identifier: Apache-2.0
 */

import java.util.Map;
import java.util.List;
import java.util.ArrayList;
import org.hyperledger.fabric.contract.ContractInterface;
import org.hyperledger.fabric.contract.annotation.Contract;
import org.hyperledger.fabric.contract.annotation.Default;
import org.hyperledger.fabric.contract.annotation.Info;
import org.hyperledger.fabric.contract.annotation.Transaction;
import org.hyperledger.fabric.shim.ChaincodeStub;
import org.hyperledger.fabric.shim.ledger.QueryResultsIterator;
import org.hyperledger.fabric.shim.ledger.KeyValue;
import com.google.gson.*;
import java.util.HashMap;

@Contract(name = "InventoringContract", info = @Info(title = "Inventoring Contract", description = "A contract for handling events and updates for IoT devices", version = "1.0"))
@Default
public final class Inventoring implements ContractInterface {

    @Transaction(intent = Transaction.TYPE.SUBMIT)
public String createEvent(org.hyperledger.fabric.contract.Context ctx, String eventData) {
    ChaincodeStub stub = ctx.getStub();
    Map<String, String> result = new HashMap<>();
    Gson gson = new Gson(); // Gson instance for parsing JSON

    try {
        // Parsing the eventData to a Map<String, Object>
        Map<String, Object> dataMap = gson.fromJson(eventData, Map.class);
        
        // Attempting to extract deviceId and cast it to a String
        Object deviceIdObj = dataMap.get("device_id");
        if (deviceIdObj == null) {
            throw new IllegalArgumentException("Device ID is required and cannot be empty.");
        }
        String deviceId = deviceIdObj.toString();  // Convert deviceId to String

        // Check if deviceId after conversion is still valid
        if (deviceId.trim().isEmpty()) {
            throw new IllegalArgumentException("Device ID cannot be empty.");
        }

        String uniqueKey = deviceId + "_" + System.currentTimeMillis();
        stub.putStringState(uniqueKey, eventData);

        String txId = stub.getTxId();
        //result.put("message", "Event stored with key: " + uniqueKey);
        result.put("transaction_id", txId);
    } catch (Exception e) {
        result.put("error", "Failed to store event: " + e.getMessage());
    }

    return gson.toJson(result); // Converting the result map to JSON string
}
    @Transaction(intent = Transaction.TYPE.EVALUATE)
    public String readEventsByDeviceId(org.hyperledger.fabric.contract.Context ctx, String deviceId) {
        ChaincodeStub stub = ctx.getStub();
        Gson gson = new Gson();
        String startKey = deviceId + "_";
        String endKey = deviceId + "_~";

        List<Map> events = new ArrayList<>();
        QueryResultsIterator<KeyValue> results = stub.getStateByRange(startKey, endKey);

        for (KeyValue result : results) {
            String eventJson = result.getStringValue();
            Map eventMap = gson.fromJson(eventJson, Map.class);
            events.add(eventMap);
        }

        String eventsJson = gson.toJson(events);
        return eventsJson;
    }
}
