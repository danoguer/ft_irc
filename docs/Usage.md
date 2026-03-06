# IRC Server Usage

## Starting the server

```bash
make re

PORT=6667
PASSWORD=elefante
./ircserv $PORT $PASSWORD
```

## Using the server with netcat

1. Connect to the server:
   ```bash
   nc localhost $PORT
   ```

2. Register with the server:
   ```
   PASS <password>
   NICK <nickname>
   USER <username> 0 * :<realname>
   ```

3. See Commands.md for a list of supported commands and usage examples.

### Using the server with HexChat

1. Open HexChat.
2. Fill in your nickname and real name.
3. To add a new network, press Add, set a name, and press Edit.
4. Click on the network address, click Edit, and input the server address and port: `localhost/<port>`.
5. Uncheck the `Use SSL for all channels in this network` option.
6. If a password was set, enter it on the Password field.
7. Click Close, then Connect.

In HexChat, commands have to started with a slash (`\`), for example type `\JOIN prueba` to join a channel. Writing without a slash just sends a message to the currently selected channel or user (using `PRIVMSG`).

Refer to Commands.md for a list of supported commands and usage options.
