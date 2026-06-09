# IRC Server Usage

This guide explains step-by-step how to configure, deploy, and interact with the IRC server using the automated container environment.

---

## 🛠️ 1. Initial Configuration (`.env`)

The project uses an environment variables file to avoid hardcoding ports or passwords into the code or the infrastructure layout.

Before starting the services, create a file named `.env` in the root directory of the project (at the same level as `docker-compose.yml`) with the following structure:

```env
# The port the IRC server will listen on (both inside and outside the container)
IRC_PORT=6667

# The password required for clients to authenticate successfully
IRC_PASSWORD=MiPasswordUltraSecreto123
```

> ⚠️ **Note:** Remember to add `.env` to your `.gitignore` file to ensure local credentials are never accidentally pushed to your public repository.

---

## 🚀 2. How to Start the Server

To compile the C++ server and automatically spin up the entire network infrastructure, open your terminal in the root of the repository and run:

```bash
docker compose up
```

If you prefer to free up your terminal and let the server run quietly in the background, use the *detached* mode:

```bash
docker compose up -d
```

---

## 🔬 3. Automatic Verification (Smoke Test)

When you run `docker compose up`, an automated client container will perform a quick integration test to verify that the server is responding correctly. The environment is working flawlessly if the last lines of the log show this exact output:

```text
ft_irc_container  | New client connected, fd: 4
ft_irc_client exited with code 0
```

### What does this result mean?
1. **`New client connected, fd: 4`**: The automated client successfully located the server through Docker's internal network, opened a TCP socket, and your C++ code accepted it correctly.
2. **`exited with code 0`**: The client sent the initial protocol commands (`NICK`, `USER`, `CAP LS`) using the exact delimiter format expected by your parser (`\n`), waited for the buffer to be processed, and shut down successfully (code `0`).
3. The main server **remains completely alive and listening** in the background, ready to handle human connections.

---

## 🔌 4. How to Connect Manually

Once the automated client finishes its test, you can connect to the server yourself from your host machine using any standard network tool or an IRC client.

### Option A: Quick connection via terminal (Netcat)
```bash
nc localhost 6667
```

### Option B: Connection via an IRC Client (HexChat, Irssi, WeeChat)
Configure a new network connection in your client with the following details:
* **Server / Address:** `localhost` or `127.0.0.1`
* **Port:** The port defined in your `.env` file (default is `6667`)
* **Password:** The password defined in your `.env` file (`IRC_PASSWORD`)

---

## 🛑 5. How to Stop the Server

When you are done testing your code and want to shut down the environment cleanly, run the following command in your terminal:

```bash
docker compose down
```

This command will gracefully stop all active processes, remove the containers from memory, and tear down the isolated virtual private network created by Docker, leaving your host system clean and free of occupied sockets.

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
