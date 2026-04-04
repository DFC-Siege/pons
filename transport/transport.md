# Transporter

## Description

A transporter has to be able to send a buffer and receive a buffer.
It receives a on_receive callback to send the data.
A concrete transporter defines itself how it gets and sends its data via an
interupt or polling etc.

# Protocol

## Description

A protocol has to be able to process a buffer and send and receive buffers via a
transporter. Its goal is to implement a specific transport plan like chunked
transport or a sliding window transport.
It receives a transporter via dependency injection which it uses to send its
payloads.
It has a send function which receives the full payload to be sent and returns a
result with a buffer.
It also has a receive_callback where it unpacks the packet and either creates a
new receiving buffer or sends a completed buffer to the callback.

# Dispatcher

## Description

A dispatcher can send and receive commands. It receives a protocol using
dependency injection. it uses the receive_callback to be able to handle data
using registered handlers or send data from registered commands.

When sending a payload, its first wrapped using a session id and a command.
When receiving a payload, it tries to first unpack the payload by unpacking each
part, but how does it know if it used chunks or not????? MTU? or does the
wrapper struct need to be passed to the protocol? or a callback for unpacking?

The dispatcher has a map of commands and handlers. A handler is just a function
that receives data and returns a result.
