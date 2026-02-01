Network layer:
* handle and maintain client network connections
* receive messages from client, wait until complete message (\r\n), then strip \r\n and send to executor layer
* receive messages from executor layer and send to client
* receive kill signal from executor layer and cleanly kill client
* BONUS: initiate file transfer between clients


Parser layer:
* receive message, parse and call function from executor layer


Server layer:
* execute messages (i.e. for each message, modify the server state and generate one or many return messages)
* maintain server state


message bytes -> network layer -> message string -> parser -> message object ->
-> server -> any number of return message objects AND MODIFY STATE -> return message strings -> network layer -> return message bytes


# SERVER STATE

Server needs to keep track of:

* Channels:
    - name
    - topic
    - mode
    - password
    - invite only
    - member list
    - operator list

* Users:
    - user_id
    - name
    - is registered
    - is authenticated

* Server:
    - channels
    - config
    - do we want persistent server state? I think not

Note on user auth: handled server-wide (actually network wide), not for each channel. 

Difference between authenticated and registered:
authentication (like SASL or NickServ IDENTIFY) proves you are logged in with your account, while registration links a unique, permanent nickname to that account, preventing others from using it and giving you identity and privileges on the network. The difference is that registration creates the unique identity (your nick), and authentication proves you own it when you connect; without registering, anyone can use your desired name, leading to redirection or restrictions

# STRUCTURES:

## Network - executor interop:

message:    # this class is for messages in both directions
    int client_id
    str message

kill_signal:
    int client_id


## Server internals

message:
    int client_id
    str prefix
    str command
    str payload

server state required:
