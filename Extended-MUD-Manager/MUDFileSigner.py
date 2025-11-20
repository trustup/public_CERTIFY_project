#from OpenSSL import crypto, SSL
#from OpenSSL.crypto import _lib, _ffi
from os.path import join
import random
import argparse
import subprocess

CN = None
pubkey = "pubkey.crt"
privkey = "privkey.key"

k = None
cert = None

args = None

def setArgParser():
    parser = argparse.ArgumentParser(description="Signer and verifier for MUD Files")
    parser.add_argument('-CN', type=str,
                        help="Common name for the CA")

    parser.add_argument('-m', '--mudfile', type=str,
                        help="path to mudfile")

    parser.add_argument('--sign', action='store_true',
                        help="Sign mudfile")

    parser.add_argument('--verify', action='store_true',
                        help="Verify mudfile")

    parser.add_argument('--CA', action="store_true",
                        help="Generate self signed CA")

    parser.add_argument('-pubk', type=str,
                        help="path to pubkey")

    parser.add_argument('-privk', type=str,
                        help="path to privkey")

    parser.add_argument('-cafile', type=str,
                        help="path to cakey")

    parser.add_argument('-inter', type=str,
                        help="path to intermediate cert")

    return parser

def generateCA():
    print("hey")
    if args.CN == None:
        return False
    print(args.CN)
    k = crypto.PKey()
    k.generate_key(crypto.TYPE_RSA, 2048)
    serialnumber = random.getrandbits(64)

    cert = crypto.X509()
    cert.get_subject().C = input("Country: ")
    cert.get_subject().ST = input("State: ")
    cert.get_subject().L = input("City: ")
    cert.get_subject().O = input("Organization: ")
    cert.get_subject().OU = input("Organizational Unit: ")
    cert.get_subject().CN = args.CN[0]
    cert.set_serial_number(serialnumber)
    cert.gmtime_adj_notBefore(0)
    cert.gmtime_adj_notAfter(31536000)
    cert.set_issuer(cert.get_subject())
    cert.set_pubkey(k)
    cert.sign(k, 'sha512')

    pub=crypto.dump_certificate(crypto.FILETYPE_PEM, cert)
    priv=crypto.dump_privatekey(crypto.FILETYPE_PEM, k)


    open(pubkey,"w+t").write(pub.decode("utf-8"))
    open(privkey,"w+t").write(priv.decode("utf-8"))
    print("*** Generated pubkey and privkey at: " + pubkey + " and " + privkey + " respectively")
    return True

def sign(args):

    if args.inter:
        "openssl cms -sign -signer signer.pem/pubkey.crt -in mudfile.json -inkey privkey.key -outform DER -certfile intermediate.pem -out mudfile.p7s"

        mudpf7 = args.mudfile.replace(".json",".p7s")

        command = 'openssl cms -sign -signer {signer} -in {mudfile} -inkey {privkey} -outform DER -certfile {intermediate} -out {mudcrt}'.format(
            signer=args.pubk,mudfile=args.mudfile, privkey=args.privk, intermediate=args.inter, mudcrt=mudpf7)

        print("*** executing sign command: " + command)

        process = subprocess.Popen(
            ['openssl', 'cms', '-sign', '-signer', args.pubk, '-in', args.mudfile,
             '-inkey', args.privk, '-outform', 'DER', 'certfile', args.inter , '-out', mudpf7],
            stdout=subprocess.PIPE,
            universal_newlines=True)

        for line in process.stdout.readlines():
            print(line)

    else:
        "openssl cms -sign -signer signer.pem/pubkey.crt -in mudfile.json -inkey privkey.key -outform DER -out mudfile.p7s"

        mudpf7 = args.mudfile.replace(".json", ".p7s")

        command = 'openssl cms -sign -signer {signer} -in {mudfile} -inkey {privkey} -outform DER -out {mudcrt}'.format(
            signer=args.pubk, mudfile=args.mudfile, privkey=args.privk, mudcrt=mudpf7)

        print("*** executing sign command: " + command)

        process = subprocess.Popen(['openssl', 'cms', '-sign', '-signer', args.pubk, '-in', args.mudfile,
                                    '-inkey', args.privk, '-outform', 'DER', '-out', mudpf7],
                                   stdout=subprocess.PIPE,
                                   universal_newlines=True)

        for line in process.stdout.readlines():
            print(line)


    return False

def _verify(mudcrt, mudfile, CAfile):
    process = subprocess.Popen(['openssl', 'cms', '-verify', '-in', mudcrt, '-inform', 'DER',
                                '-content', mudfile, '-binary', '-CAfile', CAfile, '-out', '/dev/null'],
                               stdout=subprocess.PIPE,
                               universal_newlines=True)

    for line in process.stdout.readlines():
        print(line)
        if line.contains("Verification successful"):

            return True

    return False

def verify(args):

    #openssl cms -verify -in mudfile.p7s -inform DER -content mudfile.json -binary -CAfile pubkey.crt -out /dev/null

    mudpf7 = args.mudfile.replace(".json", ".p7s")

    command = 'openssl cms -verify -in {mudcrt} -inform DER -content {mudfile} -binary -CAfile {cafile} -out /dev/null'.format(
        cafile=args.cafile[0], mudfile=args.mudfile, mudcrt=mudpf7)

    print("*** executing verify command: " + command)

    process = subprocess.Popen(['openssl', 'cms', '-verify', '-in', mudpf7, '-inform', 'DER',
                                '-content', args.mudfile, '-binary', '-CAfile', args.cafile, '-out', '/dev/null'],
                               stdout=subprocess.PIPE,
                               universal_newlines=True)

    for line in process.stdout.readlines():
        print(line)
        if line.contains("Verification successful"):
            return True

    return False

if __name__ == "__main__":



    parser = setArgParser()
    args = parser.parse_args()
    if args.CA:
        if not generateCA():
            print("*** CN argument requiered for --CA option")
            quit()

    if args.sign:
        if not args.mudfile:
            print("*** mudfile option is required for --sign option")
            quit()
        elif not args.pubk:
            print("*** pubkey is required for --sign option")
            quit()
        elif not args.privk:
            print("*** privkey is required for --sign option")
            quit()

        sign(args)


    if args.verify:
        if not args.mudfile:
            print("*** mudfile option is required for --verify option")
            quit()


        verify(args)

