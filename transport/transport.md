# Transporter

## Description

A transporter is a protocol which handles the sending and receiving of data
between a target and a source

It just defines the

## Spec

### Packets

- PacketType payload for sending a payload, contents depend on implementation
- PacketType ack for acknowleding receiving a packet
- PacketType nack for receiving an invalid packet
- PacketType fin for telling the target it won't send anything anymore

### Sender

A sender handles the outgoing payloads
A concrete sender has to implement a send function which can send a packet

### Receiver

A receiver handles the incoming payloads
A concrete receiver has to implement a receive function which can receive a
packet

### Dispatcher

A dispatcher handles the completed in and outgoing payloads

### Flow

#### Two way data

```pseudo
source | dispatcher | dispatch data to sender     ->
source | sender     | prepare data                ->

repeat until done:
source | sender     | send payload to target      ->
target | receiver   | check payload               ->
if valid:
target | receiver   | collect and save payload         ->
else
target | receiver   | send ack to source         ->

done:
target | receiver   | send all data to dispatcher ->
target | dispatcher | handle data                 ->
target | dispatcher | send response to sender     ->

repeat until done:
target | sender     | send data to source         ->
source | dispatcher | send data to receiver       ->
source | receiver   | send ack to target          ->

source | receiver   | send all data to dispatcher ->
source | dispatcher | handle reponse
```

## Pseudo usages

```pseudo
enum dispatchers {
        request_picture = 0x00
}

[serializable]
[deserializable]
struct picture_response {
        buffer
        width
        height
}

[serializable]
[deserializable]
struct picture_request {
        aec
        width
        height
}

picture_request payload
var response = transporter.dispatch<picture_response>(dispatchers::request_picture, payload)
if (response.failed()) {
        // handle error using reponse.error()
}
var picture = response.value()
```
