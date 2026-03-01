#!/usr/bin/env bash
set -u

PORT=8123
PASSWORD="password" # not implemented yet

# build and start server
make re
./ircserv "$PORT" "$PASSWORD" &
SERVER_PID=$!

cleanup() {
	kill "$SERVER_PID" >/dev/null 2>&1 || true
}
trap cleanup EXIT

# Give the server a moment to bind the port
sleep 0.2

tests=(
	# PASS
	"PASS hunter2"
	"PASS :hunter2 with spaces"
	":ignored.prefix PASS secretpass"
	"PASS"

	# NICK
	"NICK myNick"
	"NICK :nickWithColon"
	":ignored.prefix NICK otherNick"
	"NICK"

	# USER (4 params; realname is commonly trailing)
	"USER u 0 * :Real Name"
	"USER u 0 * RealNameNoColon"
	":ignored.prefix USER u 0 * :Real Name With Spaces"
	"USER"

	# JOIN
	"JOIN #chan"
	"JOIN #chan key"
	"JOIN #a,#b key"
	":ignored.prefix JOIN #chan"

	# PRIVMSG (target + trailing)
	"PRIVMSG #chan :hello world"
	"PRIVMSG nick :hey"
	"PRIVMSG #chan :::starts with colons"
	":ignored.prefix PRIVMSG #chan :hello from prefixed client"

	# PING
	"PING 12345"
	"PING :server.example.com"
	":ignored.prefix PING 12345"
	"PING"

	# QUIT
	"QUIT"
	"QUIT :bye bye"
	":ignored.prefix QUIT :leaving"

    # Two commands
    "NICK pepe\nNICK paco"
)

run_one() {
	local line="$1"
    echo
	echo "==> $line"
	# -C converts \n to \r\n for IRC-style line endings
	# Each test uses a fresh connection
	printf '%s\n' "$line" | nc -C -w 1 localhost "$PORT" >/dev/null
}

for t in "${tests[@]}"; do
	run_one "$t" || true
done
