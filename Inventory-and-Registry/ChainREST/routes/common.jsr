const {Wallets, X509Identity, GatewayOptions, Gateway} = require("fabric-network");



async function initGatewayOptions(config) {
    const wallet = await Wallets.newInMemoryWallet();
    const x509Identity = {
        credentials: {
            certificate: "",
            privateKey: ""
        },
        mspId: config.identity.mspid,
        type: "X.509",
    };
    await wallet.put(config.identity.mspid, x509Identity);
    const gatewayOptions = {
        identity: config.identity.mspid,
        wallet,
        discovery: {
            enabled: config.settings.enableDiscovery,
            asLocalhost: config.settings.asLocalhost,
        },
    };
    return gatewayOptions;
}


async function initGateway() {
    try {
        //gatewayOptions
        const gatewayOptions = await initGatewayOptions(conf);
        const gateway = new Gateway();
        const currentDate = new Date();
        const timestamp = currentDate.getTime();

        conf.connectionProfile['name'] = 'umu.fabric.' + timestamp;
        conf.connectionProfile['version'] = '1.0.0' + timestamp;

        await gateway.connect(conf.connectionProfile, gatewayOptions);
        const network = await gateway.getNetwork(conf.channelName);
        const contract =  await network.getContract(conf.contractName);
        return contract;
    } catch (error) {
        console.log("Hyperledger Error: " + error.toString())
        throw error;
    } finally {
    }
}

const conf = {
    channelName: "mychannel",
    contractName: "inventoring",
    connectionProfile: {
        name: "umu.fabric",
        version: "1.0.0",
        client: {
            organization: "Org1",
            connection: {
                timeout: {
                    peer: {
                        endorser: 3000
                    }
                }
            }
        },
        channels: {
            mychannel: {
                orderers: ["orderer.example.com"],
                peers: {
                    "peer0.org1.example.com": {
                        endorsingPeer: true,
                        chaincodeQuery: true,
                        ledgerQuery: true,
                        eventSource: true,
                        discover: true
                    }
                }
            },
        },
        organizations: {
            Org1: {
                mspid: "Org1MSP",
                peers: ["peer0.org1.example.com"],
                certificateAuthorities: ["ca.org1.example.com"]
            }
        },
        orderers: {
            "orderer.example.com": {
                url: "grpcs://orderer.example.com:7050",
                tlsCACerts: {
                    path:
                        "/usr/src/app/crypto-config/ordererOrganizations/example.com/orderers/orderer.example.com/msp/tlscacerts/tlsca.example.com-cert.pem",
                },
            }
        },
        peers: {
            "peer0.org1.example.com": {
                "url": "grpcs://peer0.org1.example.com:7051",
                tlsCACerts: {
                    path:
                        "/usr/src/app/crypto-config/peerOrganizations/org1.example.com/peers/peer0.org1.example.com/msp/tlscacerts/tlsca.org1.example.com-cert.pem",
                },
            },
        },
    },
    certificateAuthorities: {
        "ca.org1.example.com": {
            "url": "https://ca.org1.example.com:7054",
            "httpOptions": {
                "verify": false
            },
            "registrar": [{
                "enrollId": "admin",
                "enrollSecret": "adminpw"
            }]
        }
    },
    identity: {
        mspid: "Org1MSP", // user
        certificate: "",

        privateKey: ""
    },
    settings: {
        enableDiscovery: true,
        asLocalhost: false,
    }
};

module.exports = { initGateway };
