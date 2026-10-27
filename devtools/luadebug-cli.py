#!/usr/bin/env python3
"""Minimal interactive DAP client for the DFHack luadebugger plugin.

Usage:
    python luadebug-cli.py [host] [port]

Commands:
    b <path>:<line>     set a breakpoint (replaces all breakpoints in that file)
    bc <path>           clear breakpoints in a file
    c                   continue
    n                   next (step over)
    s                   step in
    o                   step out
    pause               break at the next Lua line
    t                   list threads
    bt [frameid]        print stack trace; also prints scopes of frame
    v <ref>             expand a variablesReference
    p <expr>            evaluate an expression in the selected frame (or global)
    f <frameid>         select a frame for evaluation
    q                   disconnect and quit
"""
import json
import socket
import sys
import threading


class DapClient:
    def __init__(self, host, port):
        self.sock = socket.create_connection((host, port))
        self.seq = 0
        self.buf = b""
        self.cv = threading.Condition()
        self.responses = {}
        self.events = []
        self.alive = True
        threading.Thread(target=self._read_loop, daemon=True).start()

    def _read_loop(self):
        while True:
            try:
                data = self.sock.recv(65536)
            except OSError:
                data = b""
            if not data:
                with self.cv:
                    self.alive = False
                    self.cv.notify_all()
                return
            self.buf += data
            while True:
                msg = self._try_parse()
                if msg is None:
                    break
                self._handle(msg)

    def _try_parse(self):
        head, sep, _ = self.buf.partition(b"\r\n\r\n")
        if not sep:
            return None
        length = None
        for line in head.split(b"\r\n"):
            if line.lower().startswith(b"content-length:"):
                length = int(line.split(b":", 1)[1].strip())
        if length is None:
            self.buf = b""
            return None
        if len(self.buf) < len(head) + 4 + length:
            return None
        body = self.buf[len(head) + 4:len(head) + 4 + length]
        self.buf = self.buf[len(head) + 4 + length:]
        return json.loads(body)

    def _handle(self, msg):
        with self.cv:
            if msg.get("type") == "response":
                self.responses[msg["request_seq"]] = msg
            else:
                self.events.append(msg)
            self.cv.notify_all()

    def request(self, command, arguments=None, wait=True):
        self.seq += 1
        seq = self.seq
        body = json.dumps({
            "seq": seq, "type": "request",
            "command": command, "arguments": arguments or {},
        }).encode()
        self.sock.sendall(
            b"Content-Length: %d\r\n\r\n" % len(body) + body)
        if not wait:
            return None
        with self.cv:
            while seq not in self.responses and self.alive:
                self.cv.wait(timeout=30)
            return self.responses.pop(seq, None)

    def drain_events(self):
        with self.cv:
            ev, self.events = self.events, []
        return ev

    def wait_for_stop(self, timeout=120):
        """Wait until a 'stopped' event arrives."""
        with self.cv:
            end = threading.Event()
            import time
            deadline = time.time() + timeout
            while self.alive:
                for i, ev in enumerate(self.events):
                    if ev.get("event") == "stopped":
                        return self.events.pop(i)
                remaining = deadline - time.time()
                if remaining <= 0:
                    return None
                self.cv.wait(timeout=min(remaining, 0.25))
            return None


def show(msg, indent=""):
    if not msg:
        print(indent + "<no response>")
        return
    if msg.get("type") == "response":
        ok = "ok" if msg.get("success") else "FAIL"
        extra = msg.get("message") or ""
        body = json.dumps(msg.get("body"), indent=indent + "  ")
        print(f"{indent}[{ok}] {msg.get('command')} {extra}\n{indent}{body}")
    else:
        print(indent + json.dumps(msg, indent=indent + "  "))


def main():
    host = sys.argv[1] if len(sys.argv) > 1 else "127.0.0.1"
    port = int(sys.argv[2]) if len(sys.argv) > 2 else 43030
    c = DapClient(host, port)
    print(f"connected to {host}:{port}")

    show(c.request("initialize", {"adapterID": "luadebug-cli"}))
    c.request("attach", {"stopOnEntry": False})
    show(c.request("configurationDone"))

    sel_frame = None
    bp_files = {}

    while c.alive:
        for ev in c.drain_events():
            if ev.get("event") == "stopped":
                b = ev.get("body", {})
                print(f"\n*** stopped: {b.get('reason')} thread {b.get('threadId')}")
            elif ev.get("event") == "output":
                print(ev.get("body", {}).get("output", ""), end="")
            else:
                print(f"\n*** event {ev.get('event')}: {ev.get('body')}")
        try:
            line = input("\ndap> ").strip()
        except (EOFError, KeyboardInterrupt):
            break
        if not line:
            continue
        cmd, _, rest = line.partition(" ")

        if cmd == "b" and ":" in rest:
            path, ln = rest.rsplit(":", 1)
            bp_files[path] = [{"line": int(ln)}]
            show(c.request("setBreakpoints", {
                "source": {"path": path}, "breakpoints": bp_files[path]}))
        elif cmd == "bc":
            bp_files[rest] = []
            show(c.request("setBreakpoints", {
                "source": {"path": rest}, "breakpoints": []}))
        elif cmd == "c":
            show(c.request("continue", {"threadId": 1}))
            ev = c.wait_for_stop()
            if ev:
                print(f"stopped: {ev.get('body')}")
        elif cmd in ("n", "s", "o"):
            show(c.request({"n": "next", "s": "stepIn", "o": "stepOut"}[cmd],
                           {"threadId": 1}))
            ev = c.wait_for_stop()
            if ev:
                print(f"stopped: {ev.get('body')}")
        elif cmd == "pause":
            show(c.request("pause", {"threadId": 1}))
        elif cmd == "t":
            show(c.request("threads"))
        elif cmd == "bt":
            r = c.request("stackTrace", {"threadId": int(rest or 1)})
            for f in (r or {}).get("body", {}).get("stackFrames", []):
                src = (f.get("source") or {}).get("path") or \
                      (f.get("source") or {}).get("name", "?")
                print(f"  #{f['id']} {f['name']} {src}:{f['line']}")
        elif cmd == "f":
            sel_frame = int(rest)
            r = c.request("scopes", {"frameId": sel_frame})
            for s in (r or {}).get("body", {}).get("scopes", []):
                print(f"  {s['name']} -> ref {s['variablesReference']}")
        elif cmd == "v":
            r = c.request("variables", {"variablesReference": int(rest)})
            for v in (r or {}).get("body", {}).get("variables", []):
                ref = f" (ref {v['variablesReference']})" \
                    if v.get("variablesReference") else ""
                print(f"  {v['name']} = {v['value']}{ref}")
        elif cmd == "p":
            args = {"expression": rest}
            if sel_frame:
                args["frameId"] = sel_frame
            show(c.request("evaluate", args))
        elif cmd in ("q", "quit", "exit"):
            c.request("disconnect", {}, wait=False)
            break
        else:
            print(__doc__)

    c.sock.close()


if __name__ == "__main__":
    main()
