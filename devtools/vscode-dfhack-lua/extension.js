// DFHack Lua debug adapter shim.
//
// The luadebugger DFHack plugin already speaks the Debug Adapter Protocol
// directly on a loopback TCP socket, so this extension does no protocol
// translation at all: it simply points VSCode at the running server via a
// DebugAdapterServer descriptor.
const vscode = require('vscode');

function activate(context) {
    const factory = {
        createDebugAdapterDescriptor(session) {
            const config = session.configuration || {};
            const host = config.host || '127.0.0.1';
            const port = config.port || 43030;
            return new vscode.DebugAdapterServer(port, host);
        }
    };
    context.subscriptions.push(
        vscode.debug.registerDebugAdapterDescriptorFactory('dfhack-lua', factory));
}

function deactivate() {}

module.exports = { activate, deactivate };
