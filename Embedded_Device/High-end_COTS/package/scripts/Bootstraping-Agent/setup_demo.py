import os
import base64
import subprocess
import json
import uuid

debug = True

# Define the URL and file path
issuerUrl = '192.168.178.63:9082'
holderUrl = '192.168.178.63:8082'

generateDidEndpoint = '/fluidos/idm/generateDID'
genDidPath = 'generateDID.json'

doEnrolmentEndpoint = '/fluidos/idm/doEnrolment'
doEnrolPath = 'doEnrolment.json'

digest_path = 'Digest_doc.rdf'

vc_path = 'vc_receibed.jsonld'

publicKeyPath = 'demo.pk'

curlGenDidCommand = 'curl --silent -k --header "Content-Type: application/json" --data @' + genDidPath + ' https://' + issuerUrl + generateDidEndpoint 
curlDoEnrlCommand = 'curl --silent -k --header "Content-Type: application/json" --data @' + doEnrolPath + ' https://' + holderUrl + doEnrolmentEndpoint 

bootstrapping_agent_name = "security_api_interface"

bootstrapCommand = [bootstrapping_agent_name, 'bootstrap']
retreiveCertificateCommand = [bootstrapping_agent_name, 'getcert']
signatureSetupCommand = ['signature_setup']


getMudUrlCommand = [bootstrapping_agent_name, 'getMudUrl']

def setGendidParams():
    with open(genDidPath, 'r+') as f:
        data = json.load(f)
        data['name'] = str(uuid.uuid1())
        f.seek(0)
        json.dump(data, f, indent=4)
        f.truncate()
        f.close()

def initiateBootstrapping():

    digestFile = open(digest_path, 'r').read()

    bootstrapStatus = subprocess.Popen(bootstrapCommand + [digestFile], stdout=subprocess.PIPE)
    if (bootstrapStatus.wait() != 0):
        print("Error during bootstrapping")
        print(bootstrapStatus.stdout.read().decode('utf-8'))
        return False
    else:
        if debug: print(bootstrapStatus.stdout.read().decode('utf-8'))
        print("Bootstrappin successfull")

    setGendidParams()

    response = os.popen(curlGenDidCommand).read()
    if debug: print(response)

    responseJson = json.loads(response)

    try:
        didId = responseJson['didDoc']['id']
    except Exception:
        print("Error in response from server: " + response)
        return False

    verifications = responseJson['didDoc']['verificationMethod']
    for verification in verifications:
        if verification['type'] == "Bls12381G1Key2022":
            publicKey = verification['publicKeyBase58']
            break

    if debug: 
        print("Did id: " + didId)
        print("Public key: " + publicKey)


    with open(publicKeyPath, "w") as pkFile:
        pkFile.write(publicKey)
        pkFile.close()

    getCertStatus = subprocess.Popen(retreiveCertificateCommand, stdout=subprocess.PIPE)
    if (getCertStatus.wait() != 0):
        print("Error retreiving certificate")
        return False

    certString = getCertStatus.stdout.read().decode('ascii').split("certificate: ")[1]
    certLines = certString.splitlines()

    if debug: print("Certificate: " + str(certLines))

    enrolmentDic = {}
    enrolmentDic['url'] = 'https://issuer:9082' 
    enrolmentDic['theirDID'] = didId 
    enrolmentDic['idProofs'] = [] 
    for line in certLines[1:]:
        name = line.split("poc/")[1].split(" ")[0]
        value = line.split("poc/")[1].split(" ")[1].split('"')[1]
        enrolmentDic['idProofs'].append({"attrName" : name, "attrValue" : value})

    with open(doEnrolPath , 'w') as f:
        json.dump(enrolmentDic, f, ensure_ascii=False, indent=4)
        f.close()


    response = os.popen(curlDoEnrlCommand).read()

    responseJson = json.loads(response)
    try:
        signature = responseJson['credential']['proof']['proofValue']
    except Exception as e:
        print("Error in server response: " + json.dumps(responseJson, ensure_ascii=False, indent=4))
        return False

    with open(vc_path, 'w') as f:
        json.dump(responseJson, f, ensure_ascii=False, indent=4)
        f.close()

    if debug: print(str(responseJson))

    storeSignatureStatus = subprocess.Popen(signatureSetupCommand + [signature], stdout=subprocess.PIPE)
    if (storeSignatureStatus.wait() != 0):
        print("Error storing signature")
        return False

    if debug: print("Stored signature: " + signature)

    digestLines = digestFile.splitlines()
    epoch = digestLines[0].split('"')[0] + '"' + responseJson['credential']['proof']['created'] + '"' + digestLines[0].split('"')[2]

    verifyStatus = subprocess.run(["verify_signature", publicKey, epoch, signature] + digestLines[1:], stdout=subprocess.PIPE)
    # if (verifyStatus.wait() != 0):
    #     print("Could not validate")

    if debug:
        print("Epoch: " + epoch)
        print("Attrs: " + str(digestLines[1:]))
        print(verifyStatus.stdout.decode('ascii'))
    return True

def launchERA():
    os.system("python3 era_agent.py &")
    print("ERA launched successfully")

# def getBootParams():

#     getMskStatus = subprocess.Popen(getMskCommand, stdout=subprocess.PIPE)
#     if (getMskStatus.wait() != 0):
#         print("Error retreiving MSK")
#         return None, None, False

#     Msk = getMskStatus.stdout.read().split(b'msk:')[1]
#     base64Msk = base64.b64encode(Msk).decode('ascii')


#     getMudUrlStatus = subprocess.Popen(getMudUrlCommand, stdout=subprocess.PIPE)
#     if (getMudUrlStatus.wait() != 0):
#         print("Error retreiving Mud URL")
#         return None, None, False

#     mudUrl = getMudUrlStatus.stdout.read().split(b'mudUrl:')[1].decode('ascii')

#     return base64Msk, mudUrl, True

if __name__ == '__main__':
    launchERA()
    initiateBootstrapping()

