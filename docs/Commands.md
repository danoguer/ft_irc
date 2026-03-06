# IRC Command Reference

## PASS
Authenticate with server password.
Expected args: `<password>`
Must be sent before NICK/USER.
```
PASS hunter2
```

## NICK
Set or change your nickname.
Expected args: `<nickname>`
Nickname must be 1-9 characters, start with letter/special, and be unique.
```
NICK alice
```

## USER
Set username and realname (registration).
Expected args: `<username> <mode> <unused> <realname>`
Must be sent during initial registration (along with NICK).
```
USER alice 0 * :Alice Wonderland
```

## PRIVMSG
Send a message to a user or channel.
Expected args: `<target> <text>`
If target begins with #, target is a channel; otherwise it's a nickname.
```
PRIVMSG bob :Hello!
PRIVMSG #general :Hi everyone!
```

## NOTICE
Send a notice to a user or channel.
Same as PRIVMSG but must NOT generate automatic replies (per RFC 2812).
Expected args: `<target> <text>`
```
NOTICE bob :You have mail.
NOTICE #general :Server maintenance tonight.
```

## JOIN
Join a channel (or create it if it doesn't exist).
Expected args: `<channel> [key]`
Channel creator becomes operator.
```
JOIN #general
JOIN #private hunter2
```

## PART
Leave a channel.
Expected args: `<channel> [reason]`
```
PART #general :Goodbye!
```

## MODE
Query or change channel modes.
Supported channel modes:
	- `i` — invite-only channel
	- `t` — channel topic can only be set by operators
	- `k` — channel key (password)
	- `o` — give/take operator privileges
	- `l` — set a user limit
Usage:
	- `MODE <channel>` — query current modes
	- `MODE <channel> <+/-modes> [params]` — set/unset modes
```
MODE #general
MODE #general +i
MODE #general +k secretkey
MODE #general +o bob
MODE #general -l
```

## INVITE
Invite a user to a channel.
Expected args: `<nickname> <channel>`
If channel is +i (invite-only), only operators can invite.
```
INVITE bob #general
```

## KICK
Remove a user from a channel (operators only).
Expected args: `<channel> <nickname> [reason]`
```
KICK #general bob :Spamming
```

## TOPIC
View or change the channel topic.
Expected args: `<channel> [topic]`
If mode +t is set, only operators can change the topic.
An empty topic string clears the topic.
```
TOPIC #general
TOPIC #general :Welcome to General!
```

## MOTD
Display the Message of the Day.
No arguments required.
```
MOTD
```

## CAP
Capability negotiation (IRCv3).
We don't implement CAP, but we register it as a command that does nothing because otherwise some clients like irssi get ERR_UNKNOWNCOMMAND and disconnect.
```
CAP LS
```

## PING
Client sends PING, server must reply with PONG.
Expected args: `<token>`
Without this, clients will think the server is dead and disconnect.
```
PING 12345
```

## QUIT
Disconnect from server.
Expected args: `[quit message]`
Optional quit message sent to all channels user is in.
```
QUIT :Leaving
```
