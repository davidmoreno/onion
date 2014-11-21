#!/usr/bin/python

#
# pip install tinyrpc
#

from tinyrpc.protocols.jsonrpc import JSONRPCProtocol
from tinyrpc.transports.http import HttpPostClientTransport
from tinyrpc import RPCClient

rpc_client = RPCClient(
    JSONRPCProtocol(),
    HttpPostClientTransport('http://localhost:8080/')
)

local = rpc_client.get_proxy()


s='   This is a message  '
stripped = local.strip(str=s)

assert stripped == s.strip()

print 'rpc ok'

import sys
sys.exit(0)
