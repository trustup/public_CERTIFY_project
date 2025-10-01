import json
import subprocess
import base64

debug = True

politiksFile = '../politik.json'
vcFile = '../vc_receibed.jsonld'
publicKeyPath = '../demo.pk'

retreiveCertificateCommand = ['security_api_interface', 'getcert']

def getIndices(policy, digest):
    # disclosedIndices = [0]
    disclosedIndices = []
    for politik in policy['disclosedAttributes']:
        indices = [index for index, string in enumerate(digest) if politik in string]
        if len(indices) > 1:
            raise Exception("Redefinition of attribute in digest")
        if len(indices) == 0:
            raise Exception("Unknown attribute: ", politik)
        disclosedIndices.append(indices[0] - 1)
    disclosedIndices.sort()
    retval = "" 
    for index in disclosedIndices:
        retval += str(index) + ","
    return retval[:-1]

def getSelectiveDisclosure(vc, allowed_tags):
    retJson = {}
    for key, value in vc.items():
        if key in allowed_tags:
            retJson[key] = value
    return retJson

def authenticate(policy):
    getCertStatus = subprocess.Popen(retreiveCertificateCommand, stdout=subprocess.PIPE)
    if (getCertStatus.wait() != 0):
        print("Error retreiving certificate")
        exit()
    
    digest = getCertStatus.stdout.read().decode('ascii').split("certificate: ")[1]
    if debug: print("digest: " + digest)
    
    inputVc = json.load(open(vcFile, "r"))['credential']
    attributes = digest.splitlines()[1:]
    digestEpoch = digest.splitlines()[0]
    
    indices = getIndices(policy, digest.splitlines())
    epoch = digestEpoch.split('"')[0] + '"' + inputVc['proof']['created'] + '"' + digestEpoch.split('"')[2]
    publicKey = open(publicKeyPath, 'r').read()
    nonce = policy['nonce']
    if debug: print(indices)
    
    
    inputVc['credentialSubject'] = getSelectiveDisclosure(inputVc['credentialSubject'], policy['disclosedAttributes'])
    inputVc['proof']['nonce'] = nonce
    inputVc['proof']['type'] = "PsmsBlsSignatureProof2022"
    inputVc['type'] += ["VerifiablePresentation"]
    # inputVc['proof']['verificationMethod'] = "did:key:" + publicKey
    
    
    if debug: print("________________________________________________________________________________________________________________")  
    
    # res = subprocess.run(['generate_zktoken', publicKey, indices, epoch] + attributes, stdout=subprocess.PIPE)
    zkTokenStatus = subprocess.Popen(['generate_zktoken', publicKey, indices, nonce, epoch] + attributes, stdout=subprocess.PIPE)
    if (zkTokenStatus.wait() != 0):
        print("Error generating zkToken")
    
    zkTokenResult = zkTokenStatus.stdout.read()
    print(zkTokenResult)
    zkTokenBytes = zkTokenResult.split(b'zktoken:')[1]

    if debug: print(zkTokenResult.split(b'zktoken:')[0].decode('ascii'))
    if debug: print(zkTokenBytes)
    
    if debug: print([hex(x) for x in list(zkTokenBytes)])
    
    inputVc['proof']['proofValue'] = base64.b64encode(zkTokenBytes).decode('utf-8')
    if debug: print(inputVc['proof']['proofValue'])
    
    outputVcString = json.dumps(inputVc, ensure_ascii=False, indent=4)
    outputVcMetaString = outputVcString.replace('"', '\\"')
    outputVcMetaString = outputVcMetaString .replace('\n', '\\n')
    
    if debug: print(outputVcMetaString)

    # return outputVcString
    return inputVc

if __name__ == "__main__":
    try:
        politiks = json.load(open(politiksFile, "r"))
        print(authenticate(politiks))
        print("Success")
    except Exception as e:
        print("Something went wrong")
    
