#!/usr/bin/env python3
"""Save OpenStory's UDP stdout/stderr and fault records from one console."""
import argparse
import ipaddress
from pathlib import Path
import socket
import time

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--bind', type=ipaddress.IPv4Address, required=True)
parser.add_argument('--ps5', type=ipaddress.IPv4Address, required=True)
parser.add_argument('--port', type=int, default=9978)
parser.add_argument('--seconds', type=int, default=1800)
parser.add_argument('--output', type=Path, required=True)
args = parser.parse_args()
if not 1 <= args.port <= 65535 or not 1 <= args.seconds <= 86400:
    parser.error('port must be 1..65535 and seconds must be 1..86400')
args.output.parent.mkdir(parents=True, exist_ok=True)
with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as listener:
    listener.bind((str(args.bind), args.port))
    listener.settimeout(1)
    print(f'Listening on {args.bind}:{args.port}/UDP for {args.ps5}; saving {args.output}', flush=True)
    deadline = time.monotonic() + args.seconds
    written = 0
    with args.output.open('ab', buffering=0) as log:
        while time.monotonic() < deadline and written < 8 * 1024 * 1024:
            try:
                data, peer = listener.recvfrom(4096)
            except socket.timeout:
                continue
            if peer[0] != str(args.ps5):
                continue
            log.write(data)
            written += len(data)
    print(f'Saved {written} bytes.', flush=True)
