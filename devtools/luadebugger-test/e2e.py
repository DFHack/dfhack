#!/usr/bin/env python3
"""End-to-end test for the DFHack Lua debugger.

Spins up a stand-in for the C++ plugin: a TCP listener speaking DAP
Content-Length framing on one side, and a lua5.3 subprocess running the
real plugins/lua/luadebugger.lua on the other (message bodies shuttled
over the child's stdio, one per line). A scripted DAP client then drives
an actual debugging session over the socket.

Usage: python3 e2e.py [path-to-lua-interpreter]
"""
import json
import os
import socket
import subprocess
import sys
import threading
import time

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.abspath(os.path.join(HERE, '..', '..'))
LUA = sys.argv[1] if len(sys.argv) > 1 else 'lua53'
PORT = 43035

passed = failed = 0


def check(name, cond, extra=''):
    global passed, failed
    if cond:
        passed += 1
        print(f'  ok: {name}')
    else:
        failed += 1
        print(f'  FAIL: {name} {extra}')


class LuaShim:
    """Plays the role of plugins/luadebugger.cpp."""

    def __init__(self):
        self.proc = subprocess.Popen(
            [LUA, os.path.join(HERE, 'e2e_driver.lua')],
            stdin=subprocess.PIPE, stdout=subprocess.PIPE,
            stderr=sys.stderr)
        self.listener = socket.socket()
        self.listener.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.listener.bind(('127.0.0.1', PORT))
        self.listener.listen(1)
        self.client = None
        self.buf = b''
        self.client_cv = threading.Condition()
        self.inbound = []          # parsed DAP messages from the client
        threading.Thread(target=self._accept_loop, daemon=True).start()
        threading.Thread(target=self._lua_out_loop, daemon=True).start()

    def _accept_loop(self):
        conn, _ = self.listener.accept()
        self.client = conn
        while True:
            data = conn.recv(65536)
            if not data:
                break
            self.buf += data
            while True:
                body = self._try_parse()
                if body is None:
                    break
                # forward message body to the lua debuggee's mailbox
                self.proc.stdin.write(body + b'\n')
                self.proc.stdin.flush()
                with self.client_cv:
                    self.inbound.append(json.loads(body))
                    self.client_cv.notify_all()

    def _try_parse(self):
        head, sep, _ = self.buf.partition(b'\r\n\r\n')
        if not sep:
            return None
        length = None
        for line in head.split(b'\r\n'):
            if line.lower().startswith(b'content-length:'):
                length = int(line.split(b':', 1)[1].strip())
        if length is None or len(self.buf) < len(head) + 4 + length:
            return None
        body = self.buf[len(head) + 4:len(head) + 4 + length]
        self.buf = self.buf[len(head) + 4 + length:]
        return body

    def _lua_out_loop(self):
        # each stdout line from the child is a DAP body for the client;
        # anything else is a driver diagnostic
        for raw in self.proc.stdout:
            line = raw.strip()
            if not line:
                continue
            if not line.startswith(b'{'):
                print(f'    [driver] {line.decode(errors="replace")}',
                      file=sys.stderr)
                continue
            if self.client:
                try:
                    self.client.sendall(
                        b'Content-Length: %d\r\n\r\n' % len(line) + line)
                except OSError:
                    return

    def run_command(self, text):
        """Send a driver command straight to the child's stdin."""
        self.proc.stdin.write(text.encode() + b'\n')
        self.proc.stdin.flush()


class Client:
    def __init__(self):
        self.sock = socket.create_connection(('127.0.0.1', PORT))
        self.seq = 0
        self.buf = b''
        self.cv = threading.Condition()
        self.responses = {}
        self.events = []
        threading.Thread(target=self._read_loop, daemon=True).start()

    def _read_loop(self):
        while True:
            data = self.sock.recv(65536)
            if not data:
                return
            self.buf += data
            while True:
                head, sep, _ = self.buf.partition(b'\r\n\r\n')
                if not sep:
                    break
                length = None
                for line in head.split(b'\r\n'):
                    if line.lower().startswith(b'content-length:'):
                        length = int(line.split(b':', 1)[1].strip())
                if length is None or \
                        len(self.buf) < len(head) + 4 + length:
                    break
                body = self.buf[len(head) + 4:len(head) + 4 + length]
                self.buf = self.buf[len(head) + 4 + length:]
                msg = json.loads(body)
                with self.cv:
                    if msg.get('type') == 'response':
                        self.responses[msg['request_seq']] = msg
                    else:
                        self.events.append(msg)
                    self.cv.notify_all()

    def request(self, command, arguments=None, timeout=30):
        self.seq += 1
        seq = self.seq
        body = json.dumps({'seq': seq, 'type': 'request',
                           'command': command,
                           'arguments': arguments or {}}).encode()
        self.sock.sendall(
            b'Content-Length: %d\r\n\r\n' % len(body) + body)
        end = time.time() + timeout
        with self.cv:
            while seq not in self.responses and time.time() < end:
                self.cv.wait(timeout=0.25)
            return self.responses.pop(seq, None)

    def wait_event(self, name, timeout=30):
        end = time.time() + timeout
        with self.cv:
            while time.time() < end:
                for i, ev in enumerate(self.events):
                    if ev.get('event') == name:
                        return self.events.pop(i)
                self.cv.wait(timeout=0.25)
        return None


def main():
    shim = LuaShim()
    c = Client()
    target = os.path.join(HERE, 'target.lua')
    time.sleep(0.5)  # let the lua child initialize

    r = c.request('initialize', {'adapterID': 'e2e'})
    check('initialize response', r and r.get('success'))
    check('capabilities present',
          r and r.get('body', {})
            .get('supportsConfigurationDoneRequest'))

    r = c.request('attach', {'stopOnEntry': False})
    check('attach response', r and r.get('success'))

    r = c.request('setBreakpoints', {
        'source': {'path': target}, 'breakpoints': [{'line': 10}]})
    check('setBreakpoints response',
          r and r.get('success') and
          r['body']['breakpoints'][0].get('verified'))

    r = c.request('configurationDone')
    check('configurationDone response', r and r.get('success'))

    shim.run_command('RUN ' + target)

    ev = c.wait_event('stopped')
    check('stopped event at breakpoint', ev and
          ev['body'].get('reason') == 'breakpoint')
    tid = ev['body'].get('threadId') if ev else None

    r = c.request('stackTrace', {'threadId': tid})
    frames = (r or {}).get('body', {}).get('stackFrames', [])
    check('stackTrace non-empty', len(frames) > 0, str(r))
    check('stopped at line 10',
          frames and frames[0].get('line') == 10, str(frames[:1]))
    frame_id = frames[0]['id'] if frames else None

    r = c.request('scopes', {'frameId': frame_id})
    scopes = (r or {}).get('body', {}).get('scopes', [])
    locals_ref = next((s['variablesReference'] for s in scopes
                       if s.get('name') == 'Locals'), None)
    check('locals scope', locals_ref is not None, str(scopes))

    r = c.request('variables', {'variablesReference': locals_ref})
    varnames = [v['name'] for v in
                (r or {}).get('body', {}).get('variables', [])]
    check('local acc visible', 'acc' in varnames, str(varnames))
    check('loop var i visible', 'i' in varnames, str(varnames))

    r = c.request('evaluate', {
        'expression': 'acc + i', 'frameId': frame_id})
    check('evaluate acc+i',
          r and r.get('success') and
          r['body'].get('result') == '1', str(r))

    r = c.request('next', {'threadId': tid})
    check('next response', r and r.get('success'))
    ev = c.wait_event('stopped')
    check('stopped after next',
          ev and ev['body'].get('reason') == 'step', str(ev))

    r = c.request('continue', {'threadId': tid})
    check('continue response', r and r.get('success'))

    term = c.wait_event('terminated', timeout=15) or \
        c.wait_event('stopped', timeout=15)
    # the target ends after the single bp is passed twice more (i=2,3)
    # then the coroutine finishes; just make sure we can disconnect
    r = c.request('disconnect')
    check('disconnect response', r and r.get('success'))

    print(f'\n=== {passed} passed, {failed} failed ===')
    sys.exit(1 if failed else 0)


if __name__ == '__main__':
    main()
