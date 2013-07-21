/*
https://github.com/peterix/dfhack
Copyright (c) 2009-2012 Petr Mrázek (peterix@gmail.com)

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any
damages arising from the use of this software.

Permission is granted to anyone to use this software for any
purpose, including commercial applications, and to alter it and
redistribute it freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must
not claim that you wrote the original software. If you use this
software in a product, an acknowledgment in the product documentation
would be appreciated but is not required.

2. Altered source versions must be plainly marked as such, and
must not be misrepresented as being the original software.

3. This notice may not be removed or altered from any source
distribution.
*/

using dfproto;
using System;
using System.Net;
using System.Net.Sockets;
using System.IO;
using System.Text;
using System.Runtime.Serialization;
using System.Runtime.Serialization.Formatters.Binary;
using System.Collections.Generic;
using System.Diagnostics;

namespace DFHack
{
    using message_type = ProtoBuf.IExtensible;

    public enum command_result
    {
        CR_LINK_FAILURE = -3,    // RPC call failed due to I/O or protocol error
        CR_NEEDS_CONSOLE = -2,   // Attempt to call interactive command without console
        CR_NOT_IMPLEMENTED = -1, // Command not implemented, or plugin not loaded
        CR_OK = 0,               // Success
        CR_FAILURE = 1,          // Failure
        CR_WRONG_USAGE = 2,      // Wrong arguments or ui state
        CR_NOT_FOUND = 3         // Target object not found (for RPC mainly)
    }

    public enum DFHackReplyCode
    {
        RPC_REPLY_RESULT = -1,
        RPC_REPLY_FAIL = -2,
        RPC_REPLY_TEXT = -3,
        RPC_REQUEST_QUIT = -4
    }

    class RPCHandshakeHeader
    {
        //public string magic;
        //public int version;

        public static string REQUEST_MAGIC = "DFHack?\n";
        public static string RESPONSE_MAGIC = "DFHack!\n";
    }

    struct RPCMessageHeader
    {
        public const int MAX_MESSAGE_SIZE = 8 * 1048576;

        public Int16 id;
        public Int32 size;

        public byte[] ConvertToBtyes()
        {
            List<byte> output = new List<byte>();
            output.AddRange(BitConverter.GetBytes(id));
            output.AddRange(new byte[2]);
            output.AddRange(BitConverter.GetBytes(size));
            return output.ToArray();
        }
        string BytesToString(byte[] input)
        {
            string output = "";
            foreach (byte item in input)
            {
                if (output.Length > 0)
                    output += ",";
                output += item;
            }
            return output;
        }
    }

    // simple function to store 
    struct DFCoord
    {
        public int x, y, z;
        //convert 
        UnityEngine.Vector3 ToVector3()
        {
            return new UnityEngine.Vector3(x + 0.5f, z + 0.5f, -y - 0.5f);
        }
        static DFCoord FromVector3(UnityEngine.Vector3 input)
        {
            DFCoord output = new DFCoord((int)input.x, (int)(-input.z), (int)input.y);
            return output;
        }

        DFCoord(int inx, int iny, int inz)
        {
            x = inx;
            y = iny;
            z = inz;
        }

        public bool isValid()
        {
            return x != -30000;
        }
        public void clear() 
        { 
            x = y = z = -30000; 
        }

        public static bool operator <(DFCoord a, DFCoord b)
        {
            if (a.x != b.x) return (a.x < b.x);
            if (a.y != b.y) return (a.y < b.y);
            return a.z < b.z;
        }
        public static bool operator >(DFCoord a, DFCoord b)
        {
            if (a.x != b.x) return (a.x > b.x);
            if (a.y != b.y) return (a.y > b.y);
            return a.z > b.z;
        }
        public static DFCoord operator +(DFCoord a, DFCoord b)
        {
            return new DFCoord(a.x + b.x, a.y + b.y, a.z + b.z);
        }
        public static DFCoord operator -(DFCoord a, DFCoord b)
        {
            return new DFCoord(a.x - b.x, a.y - b.y, a.z - b.z);
        }
        public static DFCoord operator /(DFCoord a, int number)
        {
            return new DFCoord((a.x < 0 ? a.x - number : a.x) / number, (a.y < 0 ? a.y - number : a.y) / number, a.z);
        }
        public static DFCoord operator *(DFCoord a, int number)
        {
            return new DFCoord(a.x * number, a.y * number, a.z);
        }
        public static DFCoord operator %(DFCoord a, int number)
        {
            return new DFCoord((a.x + number) % number, (a.y + number) % number, a.z);
        }
        public static DFCoord operator -(DFCoord a, int number)
        {
            return new DFCoord(a.x, a.y, a.z - number);
        }
        public static DFCoord operator +(DFCoord a, int number)
        {
            return new DFCoord(a.x, a.y, a.z + number);
        }
    }
    public struct DFCoord2d
    {
        public int x;
        public int y;

        public DFCoord2d(int _x, int _y)
        {
            x = _x;
            y = _y;
        }

        public bool isValid()
        { 
            return x != -30000; 
        }
        public void clear()
        {
            x = y = -30000;
        }

        public static bool operator <(DFCoord2d a, DFCoord2d b)
        {
            if (a.x != b.x) return (a.x < b.x);
            return a.y < b.y;
        }
        public static bool operator >(DFCoord2d a, DFCoord2d b)
        {
            if (a.x != b.x) return (a.x > b.x);
            return a.y > b.y;
        }

        public static DFCoord2d operator +(DFCoord2d a, DFCoord2d b)
        {
            return new DFCoord2d(a.x + b.x, a.y + b.y);
        }
        public static DFCoord2d operator -(DFCoord2d a, DFCoord2d b)
        {
            return new DFCoord2d(a.x - b.x, a.y - b.y);
        }


        public static DFCoord2d operator /(DFCoord2d a, int number)
        {
            return new DFCoord2d((a.x < 0 ? a.x - number : a.x) / number, (a.y < 0 ? a.y - number : a.y) / number);
        }
        public static DFCoord2d operator *(DFCoord2d a, int number)
        {
            return new DFCoord2d(a.x * number, a.y * number);
        }
        public static DFCoord2d operator %(DFCoord2d a, int number)
        {
            return new DFCoord2d((a.x + number) % number, (a.y + number) % number);
        }
        public static DFCoord2d operator &(DFCoord2d a, int number)
        {
            return new DFCoord2d(a.x & number, a.y & number);
        }
    }
    /* Protocol description:
     *
     * 1. Handshake
     *
     *   Client initiates connection by sending the handshake
     *   request header. The server responds with the response
     *   magic. Currently both versions must be 1.
     *
     * 2. Interaction
     *
     *   Requests are done by exchanging messages between the
     *   client and the server. Messages consist of a serialized
     *   protobuf message preceeded by RPCMessageHeader. The size
     *   field specifies the length of the protobuf part.
     *
     *   NOTE: As a special exception, RPC_REPLY_FAIL uses the size
     *         field to hold the error code directly.
     *
     *   Every callable function is assigned a non-negative id by
     *   the server. Id 0 is reserved for BindMethod, which can be
     *   used to request any other id by function name. Id 1 is
     *   RunCommand, used to call console commands remotely.
     *
     *   The client initiates every call by sending a message with
     *   appropriate function id and input arguments. The server
     *   responds with zero or more RPC_REPLY_TEXT:CoreTextNotification
     *   messages, followed by RPC_REPLY_RESULT containing the output
     *   of the function if it succeeded, or RPC_REPLY_FAIL with the
     *   error code if it did not.
     *
     * 3. Disconnect
     *
     *   The client terminates the connection by sending an
     *   RPC_REQUEST_QUIT header with zero size and immediately
     *   closing the socket.
     */

    public class RPCFunctionBase
    {

        public message_type p_in_template;
        public message_type p_out_template;

        public message_type make_in()
        {
            return (message_type)Activator.CreateInstance(p_in_template.GetType());
        }

        public message_type input()
        {
            if (p_in == null) p_in = make_in();
            return p_in;
        }

        public message_type make_out()
        {
            return (message_type)Activator.CreateInstance(p_out_template.GetType());
        }

        public message_type output()
        {
            if (p_out == null) p_out = make_out();
            return p_out;
        }

        public void reset(bool free = false)
        {
            if (free)
            {
                p_in = null;
                p_out = null;
            }
            else
            {
                if (p_in != null)
                    p_in = (message_type)Activator.CreateInstance(p_in.GetType());
                if (p_out != null)
                    p_out = (message_type)Activator.CreateInstance(p_out.GetType());
            }
        }

        public RPCFunctionBase(message_type input, message_type output)
        {
            p_in_template = input;
            p_out_template = output;
            p_in = null;
            p_out = null;
        }
        ~RPCFunctionBase()
        {
            p_in = null;
            p_out = null;
        }

        message_type p_in;
        message_type p_out;
    }

    public class RemoteFunctionBase : RPCFunctionBase
    {
        public bool bind(RemoteClient client, string name,
                      string proto = "")
        {
            return bind(client.default_output(), client, name, proto);
        }
        public bool bind(color_ostream output,
                  RemoteClient client, string name,
                  string proto = "")
        {
            if (isValid())
            {
                if (p_client == client && this.name == name && this.proto == proto)
                    return true;

                output.printerr("Function already bound to %s::%s\n",
                             this.proto, this.name);
                return false;
            }

            this.name = name;
            this.proto = proto;
            this.p_client = client;

            return client.bind(output, this, name, proto);
        }

        public bool isValid() { return (id >= 0); }

        public RemoteFunctionBase(message_type input, message_type output)
            : base(input, output)
        {
            p_client = null;
            id = -1;
        }

        protected color_ostream default_ostream()
        {
            return p_client.default_output();
        }

        bool sendRemoteMessage(Socket socket, Int16 id, MemoryStream msg)
        {
            List<byte> buffer = new List<byte>();

            RPCMessageHeader header = new RPCMessageHeader();
            header.id = id;
            header.size = (Int32)msg.Length;
            buffer.AddRange(header.ConvertToBtyes());
            buffer.AddRange(msg.ToArray());

            int fullsz = buffer.Count;

            int got = socket.Send(buffer.ToArray());
            return (got == fullsz);
        }

        protected command_result execute<Input, Output>(color_ostream outString, Input input, Output output)
            where Input : message_type, new()
            where Output : message_type, new()
        {
            if (!isValid())
            {
                outString.printerr("Calling an unbound RPC function %s::%s.\n",
                             this.proto, this.name);
                return command_result.CR_NOT_IMPLEMENTED;
            }

            if (p_client.socket == null)
            {
                outString.printerr("In call to %s::%s: invalid socket.\n",
                             this.proto, this.name);
                return command_result.CR_LINK_FAILURE;
            }

            MemoryStream sendStream = new MemoryStream();

            ProtoBuf.Serializer.Serialize<Input>(sendStream, input);

            long send_size = sendStream.Length;

            if (send_size > RPCMessageHeader.MAX_MESSAGE_SIZE)
            {
                outString.printerr("In call to %s::%s: message too large: %d.\n",
                                this.proto, this.name, send_size);
                return command_result.CR_LINK_FAILURE;
            }

            if (!sendRemoteMessage(p_client.socket, id, sendStream))
            {
                outString.printerr("In call to %s::%s: I/O error in send.\n",
                                this.proto, this.name);
                return command_result.CR_LINK_FAILURE;
            }

            color_ostream_proxy text_decoder = new color_ostream_proxy(outString);
            CoreTextNotification text_data;

            output = new Output();
            //return command_result.CR_OK;

            while (true)
            {
                RPCMessageHeader header = new RPCMessageHeader();
                byte[] buffer = new byte[8];

                if (!RemoteClient.readFullBuffer(p_client.socket, buffer, 8))
                {
                    outString.printerr("In call to %s::%s: I/O error in receive header.\n",
                                    this.proto, this.name);
                    return command_result.CR_LINK_FAILURE;
                }

                header.id = BitConverter.ToInt16(buffer, 0);
                header.size = BitConverter.ToInt32(buffer, 4); //because something, somewhere, is fucking retarded

                //outString.print("Received %d:%d.\n", header.id, header.size);


                if ((DFHackReplyCode)header.id == DFHackReplyCode.RPC_REPLY_FAIL)
                    if (header.size == (int)command_result.CR_OK)
                        return command_result.CR_FAILURE;
                    else
                        return (command_result)header.size;

                if (header.size < 0 || header.size > RPCMessageHeader.MAX_MESSAGE_SIZE)
                {
                    outString.printerr("In call to %s::%s: invalid received size %d.\n",
                                    this.proto, this.name, header.size);
                    return command_result.CR_LINK_FAILURE;
                }

                byte[] buf = new byte[header.size];

                if (!RemoteClient.readFullBuffer(p_client.socket, buf, header.size))
                {
                    outString.printerr("In call to %s::%s: I/O error in receive %d bytes of data.\n",
                                    this.proto, this.name, header.size);
                    return command_result.CR_LINK_FAILURE;
                }

                switch ((DFHackReplyCode)header.id)
                {
                    case DFHackReplyCode.RPC_REPLY_RESULT:
                        output = ProtoBuf.Serializer.Deserialize<Output>(new MemoryStream(buf));
                        if (output == null)
                        {
                            outString.printerr("In call to %s::%s: error parsing received result.\n",
                                            this.proto, this.name);
                            return command_result.CR_LINK_FAILURE;
                        }
                        return command_result.CR_OK;

                    case DFHackReplyCode.RPC_REPLY_TEXT:
                        text_data = ProtoBuf.Serializer.Deserialize<CoreTextNotification>(new MemoryStream(buf));

                        if (text_data != null)
                        {
                            text_decoder.decode(text_data);
                        }
                        else
                            outString.printerr("In call to %s::%s: received invalid text data.\n",
                                            this.proto, this.name);
                        break;

                    default:
                        break;
                }
            }
        }


        public string name, proto;
        public RemoteClient p_client;
        public Int16 id;
    }

    public class RemoteFunction<Input, Output> : RemoteFunctionBase
        where Input : message_type, new()
        where Output : message_type, new()
    {
        public new Input make_in() { return (Input)(base.make_in()); }
        public new Input input() { return (Input)(base.input()); }
        public new Output make_out() { return (Output)(base.make_out()); }
        public new Output output() { return (Output)(base.output()); }

        public RemoteFunction() : base(new Input(), new Output()) { }

        public command_result execute()
        {
            if (p_client == null)
                return command_result.CR_NOT_IMPLEMENTED;
            else
                return base.execute<Input, Output>(default_ostream(), input(), output());
        }
        public command_result execute(color_ostream stream)
        {
            return base.execute<Input, Output>(stream, input(), output());
        }
        public command_result execute(Input input, Output output)
        {
            if (p_client == null)
                return command_result.CR_NOT_IMPLEMENTED;
            else
                return base.execute<Input, Output>(default_ostream(), input, output);
        }
        public command_result execute(color_ostream stream, Input input, Output output)
        {
            return base.execute<Input, Output>(stream, input, output);
        }
    }

    public class RemoteFunction<Input> : RemoteFunctionBase
        where Input : message_type, new()
    {
        public new Input make_in() { return (Input)(base.make_in()); }
        public new Input input() { return (Input)(base.input()); }

        public RemoteFunction() : base(new Input(), new EmptyMessage()) { }

        public command_result execute()
        {
            if (p_client == null)
                return command_result.CR_NOT_IMPLEMENTED;
            else
                return base.execute<Input, EmptyMessage>(default_ostream(), input(), (EmptyMessage)output());
        }
        public command_result execute(color_ostream stream)
        {
            return base.execute<Input, EmptyMessage>(stream, input(), (EmptyMessage)output());
        }
        public command_result execute(Input input)
        {
            if (p_client == null)
                return command_result.CR_NOT_IMPLEMENTED;
            else
                return base.execute<Input, EmptyMessage>(default_ostream(), input, (EmptyMessage)output());
        }
        public command_result execute(color_ostream stream, Input input)
        {
            return base.execute<Input, EmptyMessage>(stream, input, (EmptyMessage)output());
        }
    };

    public class RemoteClient
    {
        public static bool readFullBuffer(Socket socket, byte[] buf, int size)
        {
            if (!socket.Connected)
                return false;

            if (size == 0)
                return true;

            int cnt = socket.Receive(buf, size, SocketFlags.None);
            if (cnt <= 0) return false;

            return true;
        }

        public bool bind(color_ostream outStream, RemoteFunctionBase function,
                  string name, string proto)
        {
            if (!active || socket == null)
                return false;

            bind_call.reset();

            {
                var input = bind_call.input();

                input.method = name;
                if (proto.Length == 0)
                    input.plugin = proto;
                input.input_msg = function.p_in_template.GetType().ToString();
                input.output_msg = function.p_out_template.GetType().ToString();
            }

            if (bind_call.execute(outStream) != command_result.CR_OK)
                return false;

            function.id = (Int16)bind_call.output().assigned_id;

            return true;
        }

        public RemoteClient(color_ostream default_output = null)
        {
            p_default_output = default_output;
            active = false;
            socket = null;
            suspend_ready = false;

            if (p_default_output == null)
            {
                delete_output = true;
                p_default_output = new color_ostream();
            }
            else
                delete_output = false;
        }
        ~RemoteClient()
        {
            disconnect();
            socket = null;

            if (delete_output)
                p_default_output = null;
        }

        public static int GetDefaultPort()
        {
            string port = System.Environment.GetEnvironmentVariable("DFHACK_PORT");
            if (port == null) port = "0";

            int portval = Int32.Parse(port);
            if (portval <= 0)
                return 5000;
            else
                return portval;
        }

        public color_ostream default_output()
        {
            return p_default_output;
        }

        private static Socket ConnectSocket(string server, int port)
        {
            Socket s = null;
            IPHostEntry hostEntry = null;

            // Get host related information.
            hostEntry = Dns.GetHostEntry(server);

            // Loop through the AddressList to obtain the supported AddressFamily. This is to avoid 
            // an exception that occurs when the host IP Address is not compatible with the address family 
            // (typical in the IPv6 case). 
            foreach (IPAddress address in hostEntry.AddressList)
            {
                IPEndPoint ipe = new IPEndPoint(address, port);
                Socket tempSocket =
                    new Socket(ipe.AddressFamily, SocketType.Stream, ProtocolType.Tcp);

                tempSocket.Connect(ipe);

                if (tempSocket.Connected)
                {
                    s = tempSocket;
                    break;
                }
                else
                {
                    continue;
                }
            }
            return s;
        }

        static bool partialArrayCompare(byte[] A, byte[] B) //compares the intersection of the two arrays, ignoring the rest.
        {
            int size = A.Length;
            if (size > B.Length) size = B.Length;
            for (int i = 0; i < size; i++)
            {
                if (A[i] != B[i])
                    return false;
            }
            return true;
        }

        public bool connect(int port = -1)
        {
            Debug.Assert(!active);

            if (port <= 0)
                port = GetDefaultPort();

            socket = ConnectSocket("localhost", port);
            if (socket == null)
            {
                default_output().printerr("Could not connect to localhost: %d\n", port);
                return false;
            }

            active = true;

            List<byte> headerList = new List<byte>();

            headerList.AddRange(Encoding.ASCII.GetBytes(RPCHandshakeHeader.REQUEST_MAGIC));
            headerList.AddRange(BitConverter.GetBytes((Int32)1));

            byte[] header = headerList.ToArray();

            if (socket.Send(header) != header.Length)
            {
                default_output().printerr("Could not send handshake header.\n");
                socket.Close();
                socket = null;
                return active = false;
            }

            if (!readFullBuffer(socket, header, header.Length))
            {
                default_output().printerr("Could not read handshake header.\n");
                socket.Close();
                socket = null;
                return active = false;
            }

            if (!partialArrayCompare(header, Encoding.ASCII.GetBytes(RPCHandshakeHeader.RESPONSE_MAGIC)) ||
                BitConverter.ToInt32(header, Encoding.ASCII.GetBytes(RPCHandshakeHeader.RESPONSE_MAGIC).Length) != 1)
            {
                default_output().printerr("Invalid handshake response: %s.\n", System.Text.Encoding.Default.GetString(header));
                socket.Close();
                socket = null;
                return active = false;
            }

            if (bind_call == null) bind_call = new RemoteFunction<CoreBindRequest, CoreBindReply>();
            bind_call.name = "BindMethod";
            bind_call.p_client = this;
            bind_call.id = 0;

            if (runcmd_call == null) runcmd_call = new RemoteFunction<CoreRunCommandRequest>();
            runcmd_call.name = "RunCommand";
            runcmd_call.p_client = this;
            runcmd_call.id = 1;

            return true;
        }

        public void disconnect()
        {
            if (active && socket != null)
            {
                RPCMessageHeader header;
                header.id = (Int16)DFHackReplyCode.RPC_REQUEST_QUIT;
                header.size = 0;
                if (socket.Send(header.ConvertToBtyes()) != header.ConvertToBtyes().Length)
                    default_output().printerr("Could not send the disconnect message.\n");
                socket.Close();
            }
            socket = null;

        }

        public command_result run_command(string cmd, List<string> args)
        {
            return run_command(default_output(), cmd, args);
        }
        public command_result run_command(color_ostream output, string cmd, List<string> args)
        {
            if (!active || socket == null)
            {
                output.printerr("In RunCommand: client connection not valid.\n");
                return command_result.CR_FAILURE;
            }

            runcmd_call.reset();

            runcmd_call.input().command = cmd;
            for (int i = 0; i < args.Count; i++)
                runcmd_call.input().arguments.Add(args[i]);

            return runcmd_call.execute(output);
        }

        //    // For executing multiple calls in rapid succession.
        //    // Best used via RemoteSuspender.
        public int suspend_game()
        {
            if (!active)
                return -1;

            if (!suspend_ready)
            {
                suspend_ready = true;
                suspend_call.bind(this, "CoreSuspend");
                resume_call.bind(this, "CoreResume");
            }

            if (suspend_call.execute(default_output()) == command_result.CR_OK)
                return suspend_call.output().value;
            else
                return -1;
        }
        public int resume_game()
        {
            if (!suspend_ready)
                return -1;

            if (resume_call.execute(default_output()) == command_result.CR_OK)
                return resume_call.output().value;
            else
                return -1;
        }

        //private:
        bool active, delete_output;
        public Socket socket;
        color_ostream p_default_output;

        RemoteFunction<dfproto.CoreBindRequest, dfproto.CoreBindReply> bind_call;
        RemoteFunction<dfproto.CoreRunCommandRequest> runcmd_call;

        bool suspend_ready;
        RemoteFunction<EmptyMessage, IntMessage> suspend_call = new RemoteFunction<EmptyMessage,IntMessage>();
        RemoteFunction<EmptyMessage, IntMessage> resume_call = new RemoteFunction<EmptyMessage,IntMessage>();
    }

    class RemoteSuspender
    {
        RemoteClient client;
        public RemoteSuspender(RemoteClient clientIn)
        {
            client = clientIn;
            if (client == null || client.suspend_game() <= 0) client = null;
        }
        ~RemoteSuspender()
        {
            if (client != null) client.resume_game();
        }
    };
}
