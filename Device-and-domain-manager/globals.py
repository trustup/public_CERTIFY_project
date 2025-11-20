import logging
import os
from dotenv import load_dotenv #XXX Note: This is a workaround for not changing environment variables. If we dockerize this and use plain environment variables it will not be needed
from sys import stdout, stderr

DADM_PORT=6000
SOFTWARE_REPO_URL="http://localhost:8080/"
INV_REG_URL="http://10.0.0.8:3002/chain/"
IDM_URL="https://localhost:9082/fluidos/idm/"
DWG_SW_REPO_DEVICE_KEY="aaa"
DWG_SW_REPO_CUSTOMER_KEY="aaa"
SUUA_URL="https://suua.api.certify.digital-worx.de"

#Loading environment variables
load_dotenv()
LOG_LEVEL="DEBUG"
LOG_OUTPUT="STDOUT"


def init_global_settings():
    global DADM_PORT
    global SOFTWARE_REPO_URL
    global INV_REG_URL
    global LOG_LEVEL
    global LOG_OUTPUT
    global DWG_SW_REPO_DEVICE_KEY
    global DWG_SW_REPO_CUSTOMER_KEY
    global SUUA_URL
    LOG_LEVEL=os.environ.get('LOGLEVEL', 'DEBUG').upper()
    LOG_OUTPUT=os.environ.get('LOGLEVEL', "STDOUT").upper()
    if LOG_OUTPUT=="STDOUT":
        logging.basicConfig(stream=stdout, level=LOG_LEVEL, format='%(asctime)s %(message)s')
    elif LOG_OUTPUT=="STDERR":
        logging.basicConfig(stream=stderr, level=LOG_LEVEL, format='%(asctime)s %(message)s')
    else:
        logging.basicConfig(filename=LOG_OUTPUT, level=LOG_LEVEL, format='%(asctime)s %(message)s')
    env = os.getenv('DADM_PORT')
    if env is not None and env != '':
        DADM_PORT = int(env)
        logging.info("DADM_PORT env variable found: " + str(DADM_PORT))
    env = os.getenv('SOFTWARE_REPO_URL')
    if env is not None and env != '':
        SOFTWARE_REPO_URL = env if env.endswith("/") else env+"/"
        logging.info("SOFTWARE_REPO_URL env variable found: " + SOFTWARE_REPO_URL)
    env = os.getenv('INV_REG_URL')
    if env is not None and env != '':
        INV_REG_URL = env if env.endswith("/") else env+"/"
        logging.info("INV_REG_URL env variable found: " + INV_REG_URL)
    env = os.getenv('IDM_URL')
    if env is not None and env != '':
        IDM_URL = env if env.endswith("/") else env+"/"
        logging.info("IDM_URL env variable found: " + IDM_URL)
    env = os.getenv('IDM_URL')
    
    if env is not None and env != '':
        IDM_URL = env if env.endswith("/") else env+"/"
        logging.info("IDM_URL env variable found: " + IDM_URL)
    env = os.getenv('DWG_SW_REPO_DEVICE_KEY')
    if env is not None and env != '':
        DWG_SW_REPO_DEVICE_KEY = env 
        logging.info("DWG_SW_REPO_DEVICE_KEY env variable found: " + DWG_SW_REPO_DEVICE_KEY)
    env = os.getenv('DWG_SW_REPO_CUSTOMER_KEY')
    if env is not None and env != '':
        DWG_SW_REPO_CUSTOMER_KEY = env 
        logging.info("DWG_SW_REPO_CUSTOMER_KEY env variable found: " + DWG_SW_REPO_CUSTOMER_KEY)
    env = os.getenv('SUUA_URL')
    if env is not None and env != '':
        SUUA_URL = env if env.endswith("/") else env+"/"
        logging.info("SUUA_URL env variable found: " + SUUA_URL)
    
    print("endglobals")
