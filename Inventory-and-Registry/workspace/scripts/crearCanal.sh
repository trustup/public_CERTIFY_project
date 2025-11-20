#!/bin/bash
if [ $# -ne 2 ]; then
    echo "Usage: $0 [CHANNEL_NAME] [ANCHOR_NAME]"
    exit 1
fi

CHANNEL_NAME="$1"
ORDERER_CA=/etc/crypto-config/ordererOrganizations/example.com/orderers/orderer.example.com/msp/tlscacerts/tlsca.example.com-cert.pem

TX_FILE="/etc/channel-artifacts/mychannel.tx"
BLOCK_FILE="/etc/channel-artifacts/mychannel.block"
ANCHOR_FILE="/etc/channel-artifacts/$2"

if [ ! -f "$BLOCK_FILE" ]; then
    echo "Block file $BLOCK_FILE does not exist, creating channel..."
    peer channel create -o orderer.example.com:7050 -c "$CHANNEL_NAME" -f "$TX_FILE" --tls --cafile "$ORDERER_CA"
    mv $CHANNEL_NAME.block $BLOCK_FILE
    sleep 5
else
    echo "Block file $BLOCK_FILE exists, skipping channel creation..."
fi

peer channel join  -o orderer.example.com:7050  -b "$BLOCK_FILE" --tls --cafile "$ORDERER_CA"
peer channel update -o orderer.example.com:7050 -c "mychannel" --tls --cafile "$ORDERER_CA" -f $ANCHOR_FILE #/etc/hyperledger/configtx/MSPanchors.tx 
