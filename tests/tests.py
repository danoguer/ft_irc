#!/usr/bin/env python3
"""
Comprehensive integration tests for the ft_irc server.

Usage:
    # The script builds the server, starts it, runs all tests, then shuts it down.
    python3 tests.py

    # Or test against an already-running server:
    python3 tests.py --host localhost --port 6667 --password mypassword --no-server
"""

import socket
import time
import subprocess
import signal
import sys
import os
import argparse

# ─── Configuration ────────────────────────────────────────────────────────────

PORT = 7667           # test port (avoid conflict with real IRC on 6667)
PASSWORD = "testpassword"
HOST = "localhost"
TIMEOUT = 2           # socket recv timeout in seconds

# ─── Helpers ──────────────────────────────────────────────────────────────────

class IRCClient:
    """Minimal IRC client for testing."""

    def __init__(self, host=HOST, port=PORT, timeout=TIMEOUT):
        self.host = host
        self.port = port
        self.timeout = timeout
        self.sock = None
        self._buffer = ""

    def connect(self):
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.settimeout(self.timeout)
        self.sock.connect((self.host, self.port))
        return self

    def send(self, raw):
        """Send a raw IRC line (\\r\\n appended automatically)."""
        self.sock.sendall((raw + "\r\n").encode())
        # small delay so the server has time to process
        time.sleep(0.1)
        return self

    def recv_all(self, timeout=None):
        """Read all available data until timeout. Returns list of lines."""
        if timeout is None:
            timeout = self.timeout
        self.sock.settimeout(timeout)
        lines = []
        try:
            while True:
                data = self.sock.recv(4096).decode("utf-8", errors="replace")
                if not data:
                    break
                self._buffer += data
                while "\r\n" in self._buffer:
                    line, self._buffer = self._buffer.split("\r\n", 1)
                    lines.append(line)
        except socket.timeout:
            pass
        return lines

    def recv_until(self, code, timeout=None):
        """Read lines until we see one containing `code`."""
        if timeout is None:
            timeout = self.timeout
        deadline = time.time() + timeout
        lines = []
        while time.time() < deadline:
            remaining = max(0.1, deadline - time.time())
            self.sock.settimeout(remaining)
            try:
                data = self.sock.recv(4096).decode("utf-8", errors="replace")
                if not data:
                    break
                self._buffer += data
                while "\r\n" in self._buffer:
                    line, self._buffer = self._buffer.split("\r\n", 1)
                    lines.append(line)
                    if code in line:
                        return lines
            except socket.timeout:
                break
        return lines

    def register(self, nick, user=None, realname=None, password=PASSWORD):
        """Full registration handshake. Returns welcome lines."""
        if user is None:
            user = nick
        if realname is None:
            realname = nick
        self.send(f"PASS {password}")
        self.send(f"NICK {nick}")
        self.send(f"USER {user} 0 * :{realname}")
        # wait for end of MOTD (376) or ERR_NOMOTD (422) to know registration is done
        lines = self.recv_until("376", timeout=5)
        if not any("376" in l for l in lines):
            # try 422 (no MOTD)
            more = self.recv_until("422", timeout=2)
            lines.extend(more)
        # extra settle time to make sure server has fully processed registration
        time.sleep(0.2)
        return lines

    def close(self):
        if self.sock:
            try:
                self.sock.close()
            except Exception:
                pass
            self.sock = None

    def __enter__(self):
        return self.connect()

    def __exit__(self, *args):
        self.close()


def has_code(lines, code):
    """Check if any line contains the given numeric code."""
    code_str = f" {code} "
    return any(code_str in line for line in lines)


def find_line(lines, code):
    """Return the first line containing the given numeric code, or None."""
    code_str = f" {code} "
    for line in lines:
        if code_str in line:
            return line
    return None


def has_prefix(lines, prefix):
    """Check if any line starts with the given prefix."""
    return any(line.startswith(prefix) for line in lines)


def contains(lines, substring):
    """Check if any line contains the substring."""
    return any(substring in line for line in lines)


# ─── Test Framework ───────────────────────────────────────────────────────────

class TestResults:
    def __init__(self):
        self.passed = 0
        self.failed = 0
        self.errors = []

    def ok(self, name):
        self.passed += 1
        print(f"  ✓ {name}")

    def fail(self, name, detail=""):
        self.failed += 1
        msg = f"  ✗ {name}"
        if detail:
            msg += f"  — {detail}"
        print(msg)
        self.errors.append(name)

    def check(self, name, condition, detail=""):
        if condition:
            self.ok(name)
        else:
            self.fail(name, detail)

    def summary(self):
        total = self.passed + self.failed
        print(f"\n{'='*60}")
        print(f"Results: {self.passed}/{total} passed, {self.failed} failed")
        if self.errors:
            print("Failed tests:")
            for e in self.errors:
                print(f"  - {e}")
        print(f"{'='*60}")
        return self.failed == 0


results = TestResults()

# ─── Test Suites ──────────────────────────────────────────────────────────────

def test_registration():
    """Test the PASS/NICK/USER registration flow."""
    print("\n=== Registration ===")

    # 1. Successful registration
    with IRCClient() as c:
        lines = c.register("reguser1")
        results.check("REG: 001 welcome", has_code(lines, "001"))
        results.check("REG: 002 yourhost", has_code(lines, "002"))
        results.check("REG: 003 created", has_code(lines, "003"))
        results.check("REG: 004 myinfo", has_code(lines, "004"))
        # MOTD should be either present (375/376) or missing (422)
        results.check("REG: MOTD sent", has_code(lines, "376") or has_code(lines, "422"))

    # 2. Wrong password
    with IRCClient() as c:
        c.send("PASS wrongpassword")
        c.send("NICK badpw")
        c.send("USER badpw 0 * :Bad PW")
        lines = c.recv_all(timeout=1)
        results.check("REG: wrong password → 464", has_code(lines, "464"))
        # Should NOT get 001 because pass was wrong and registration can't complete
        results.check("REG: no welcome on bad password", not has_code(lines, "001"))

    # 3. No password → registration never completes
    with IRCClient() as c:
        c.send("NICK nopw")
        c.send("USER nopw 0 * :No Password")
        lines = c.recv_all(timeout=1)
        results.check("REG: no welcome without PASS", not has_code(lines, "001"))

    # 4. Missing NICK args → 431
    with IRCClient() as c:
        c.send("PASS " + PASSWORD)
        c.send("NICK")
        lines = c.recv_all(timeout=1)
        results.check("REG: NICK no args → 431", has_code(lines, "431"))

    # 5. Missing USER args → 461
    with IRCClient() as c:
        c.send("PASS " + PASSWORD)
        c.send("NICK usertest5")
        c.send("USER only_one_arg")
        lines = c.recv_all(timeout=1)
        results.check("REG: USER too few args → 461", has_code(lines, "461"))

    # 6. PASS with no args → 461
    with IRCClient() as c:
        c.send("PASS")
        lines = c.recv_all(timeout=1)
        results.check("REG: PASS no args → 461", has_code(lines, "461"))

    # 7. Duplicate NICK → 433
    with IRCClient() as c1, IRCClient() as c2:
        c1.register("dupenick")
        c2.send("PASS " + PASSWORD)
        c2.send("NICK dupenick")
        lines = c2.recv_all(timeout=1)
        results.check("REG: duplicate nick → 433", has_code(lines, "433"))

    # 8. Invalid nickname → 432
    with IRCClient() as c:
        c.send("PASS " + PASSWORD)
        c.send("NICK 123invalid")
        lines = c.recv_all(timeout=1)
        results.check("REG: bad nick → 432", has_code(lines, "432"))

    # 9. PASS after registration → 462
    with IRCClient() as c:
        reg_lines = c.register("alrdyreg")
        ok = has_code(reg_lines, "001")
        if not ok:
            results.fail("REG: PASS after reg → 462",
                          f"registration failed, got: {reg_lines}")
        else:
            c.send("PASS " + PASSWORD)
            lines = c.recv_all(timeout=1)
            results.check("REG: PASS after reg → 462", has_code(lines, "462"),
                           f"got: {lines}")

    # 10. USER after registration → 462
    with IRCClient() as c:
        reg_lines = c.register("alrdyrg2")
        ok = has_code(reg_lines, "001")
        if not ok:
            results.fail("REG: USER after reg → 462",
                          f"registration failed, got: {reg_lines}")
        else:
            c.send("USER x 0 * :x")
            lines = c.recv_all(timeout=1)
            results.check("REG: USER after reg → 462", has_code(lines, "462"),
                           f"got: {lines}")

    # 11. Unknown command before registration → 421
    with IRCClient() as c:
        c.send("FOOBAR")
        lines = c.recv_all(timeout=1)
        results.check("REG: unknown cmd → 421", has_code(lines, "421"))

    # 12. Known post-reg command before registration → 451
    with IRCClient() as c:
        c.send("JOIN #test")
        lines = c.recv_all(timeout=1)
        results.check("REG: JOIN before reg → 451", has_code(lines, "451"))


def test_nick_change():
    """Test changing nickname after registration."""
    print("\n=== NICK Change ===")

    with IRCClient() as c:
        c.register("oldnick")
        c.send("NICK newnick")
        lines = c.recv_all(timeout=1)
        # Should get :oldnick!... NICK newnick
        results.check("NICK: change broadcasts", contains(lines, "NICK") and contains(lines, "newnick"))

    # Too-long nick
    with IRCClient() as c:
        c.register("shorty")
        c.send("NICK " + "a" * 10)  # 10 chars, max is 9
        lines = c.recv_all(timeout=1)
        results.check("NICK: too long → 432", has_code(lines, "432"))


def test_join():
    """Test JOIN command."""
    print("\n=== JOIN ===")

    # 1. Join creates channel, user gets op
    with IRCClient() as c:
        c.register("joiner")
        c.send("JOIN #newchan")
        lines = c.recv_all(timeout=1)
        results.check("JOIN: gets JOIN echo", contains(lines, "JOIN #newchan"))
        results.check("JOIN: gets 353 NAMREPLY", has_code(lines, "353"))
        results.check("JOIN: gets 366 ENDOFNAMES", has_code(lines, "366"))
        # Creator should be @joiner in the names list
        nameline = find_line(lines, "353")
        results.check("JOIN: creator is op (@)", nameline is not None and "@joiner" in nameline)

    # 2. JOIN no args → 461
    with IRCClient() as c:
        c.register("joiner2")
        c.send("JOIN")
        lines = c.recv_all(timeout=1)
        results.check("JOIN: no args → 461", has_code(lines, "461"))

    # 3. JOIN invalid channel name → 403
    with IRCClient() as c:
        c.register("joiner3")
        c.send("JOIN nochanprefix")
        lines = c.recv_all(timeout=1)
        results.check("JOIN: bad name → 403", has_code(lines, "403"))

    # 4. Second user joins existing channel
    with IRCClient() as c1, IRCClient() as c2:
        c1.register("creator")
        c1.send("JOIN #multi")
        c1.recv_all(timeout=1)  # consume join messages

        c2.register("newcomer")
        c2.send("JOIN #multi")
        # c2 should get join + names
        lines2 = c2.recv_all(timeout=1)
        results.check("JOIN: second user gets JOIN echo", contains(lines2, "JOIN #multi"))
        results.check("JOIN: second user gets names", has_code(lines2, "353"))

        # c1 should also get notification of c2 joining
        lines1 = c1.recv_all(timeout=1)
        results.check("JOIN: first user notified", contains(lines1, "newcomer") and contains(lines1, "JOIN"))


def test_part():
    """Test PART command."""
    print("\n=== PART ===")

    # 1. Basic part
    with IRCClient() as c:
        c.register("parter")
        c.send("JOIN #parttest")
        c.recv_all(timeout=1)
        c.send("PART #parttest :goodbye")
        lines = c.recv_all(timeout=1)
        results.check("PART: gets PART echo", contains(lines, "PART #parttest"))
        results.check("PART: reason included", contains(lines, "goodbye"))

    # 2. PART no args → 461
    with IRCClient() as c:
        c.register("parter2")
        c.send("PART")
        lines = c.recv_all(timeout=1)
        results.check("PART: no args → 461", has_code(lines, "461"))

    # 3. PART channel not on → 442
    with IRCClient() as c:
        c.register("parter3")
        c.send("JOIN #parttest2")
        c.recv_all(timeout=1)
        c.send("PART #parttest2")
        c.recv_all(timeout=1)
        c.send("PART #parttest2")
        lines = c.recv_all(timeout=1)
        # channel was cleaned up (empty), so it's 403 now
        results.check("PART: not on channel → 403 or 442",
                       has_code(lines, "442") or has_code(lines, "403"))

    # 4. PART nonexistent channel → 403
    with IRCClient() as c:
        c.register("parter4")
        c.send("PART #doesntexist")
        lines = c.recv_all(timeout=1)
        results.check("PART: no such channel → 403", has_code(lines, "403"))

    # 5. Multi-user part notification
    with IRCClient() as c1, IRCClient() as c2:
        c1.register("stayer")
        c1.send("JOIN #partnotify")
        c1.recv_all(timeout=1)

        c2.register("leaver")
        c2.send("JOIN #partnotify")
        c2.recv_all(timeout=1)
        c1.recv_all(timeout=1)  # consume c2 join notification

        c2.send("PART #partnotify :later")
        c2.recv_all(timeout=1)

        lines1 = c1.recv_all(timeout=1)
        results.check("PART: other user notified", contains(lines1, "PART") and contains(lines1, "leaver"))


def test_privmsg():
    """Test PRIVMSG command."""
    print("\n=== PRIVMSG ===")

    # 1. Direct message
    with IRCClient() as c1, IRCClient() as c2:
        c1.register("sender")
        c2.register("receiver")

        c1.send("PRIVMSG receiver :hello there")
        lines = c2.recv_all(timeout=1)
        results.check("PRIVMSG: DM received", contains(lines, "hello there"))
        results.check("PRIVMSG: DM has sender prefix", contains(lines, "sender!"))

    # 2. Channel message
    with IRCClient() as c1, IRCClient() as c2:
        c1.register("chatter1")
        c2.register("chatter2")
        c1.send("JOIN #chat")
        c1.recv_all(timeout=1)
        c2.send("JOIN #chat")
        c2.recv_all(timeout=1)
        c1.recv_all(timeout=1)  # consume c2 join notification

        c1.send("PRIVMSG #chat :hello channel")
        lines = c2.recv_all(timeout=1)
        results.check("PRIVMSG: channel msg received by other", contains(lines, "hello channel"))

        # sender should NOT receive their own message
        lines_self = c1.recv_all(timeout=0.5)
        results.check("PRIVMSG: sender doesn't get echo",
                       not contains(lines_self, "hello channel"))

    # 3. PRIVMSG no args → 461
    with IRCClient() as c:
        c.register("noargs")
        c.send("PRIVMSG")
        lines = c.recv_all(timeout=1)
        results.check("PRIVMSG: no args → 461", has_code(lines, "461"))

    # 4. PRIVMSG to nonexistent nick → 401
    with IRCClient() as c:
        c.register("lonely")
        c.send("PRIVMSG ghostuser :hi")
        lines = c.recv_all(timeout=1)
        results.check("PRIVMSG: no such nick → 401", has_code(lines, "401"))

    # 5. PRIVMSG to nonexistent channel → 403
    with IRCClient() as c:
        c.register("lonely2")
        c.send("PRIVMSG #nochan :hi")
        lines = c.recv_all(timeout=1)
        results.check("PRIVMSG: no such channel → 403", has_code(lines, "403"))

    # 6. PRIVMSG to channel user is not a member of → 404
    with IRCClient() as c1, IRCClient() as c2:
        c1.register("member_pm")
        c1.send("JOIN #restricted")
        c1.recv_all(timeout=1)

        c2.register("outsidr")
        time.sleep(0.2)
        c2.send("PRIVMSG #restricted :sneak msg")
        lines = c2.recv_all(timeout=1)
        results.check("PRIVMSG: not member → 404", has_code(lines, "404"),
                       f"got: {lines}")


def test_topic():
    """Test TOPIC command."""
    print("\n=== TOPIC ===")

    # 1. Set and query topic
    with IRCClient() as c:
        c.register("topicuser")
        c.send("JOIN #topictest")
        c.recv_all(timeout=1)

        c.send("TOPIC #topictest :This is a test topic")
        lines = c.recv_all(timeout=1)
        results.check("TOPIC: set broadcasts", contains(lines, "This is a test topic"))

        c.send("TOPIC #topictest")
        lines = c.recv_all(timeout=1)
        results.check("TOPIC: query returns 332", has_code(lines, "332"))
        results.check("TOPIC: query has topic text", contains(lines, "This is a test topic"))

    # 2. Query topic when none set → 331
    with IRCClient() as c:
        c.register("topicqry")
        c.send("JOIN #notopicyet")
        c.recv_all(timeout=1)
        time.sleep(0.2)
        c.send("TOPIC #notopicyet")
        lines = c.recv_all(timeout=1)
        results.check("TOPIC: no topic → 331", has_code(lines, "331"),
                       f"got: {lines}")

    # 3. TOPIC no args → 461
    with IRCClient() as c:
        c.register("topicna")
        time.sleep(0.2)
        c.send("TOPIC")
        lines = c.recv_all(timeout=1)
        results.check("TOPIC: no args → 461", has_code(lines, "461"),
                       f"got: {lines}")

    # 4. TOPIC on channel not on → 442
    with IRCClient() as c1, IRCClient() as c2:
        c1.register("topicmkr")
        c1.send("JOIN #topicperm")
        c1.recv_all(timeout=1)

        c2.register("topicout")
        time.sleep(0.2)
        c2.send("TOPIC #topicperm :hacked")
        lines = c2.recv_all(timeout=1)
        results.check("TOPIC: not on channel → 442", has_code(lines, "442"),
                       f"got: {lines}")

    # 5. +t restricts topic to ops only
    with IRCClient() as c1, IRCClient() as c2:
        c1.register("topicop")
        c1.send("JOIN #topicrestrict")
        c1.recv_all(timeout=1)

        c2.register("topicpleb")
        c2.send("JOIN #topicrestrict")
        c2.recv_all(timeout=1)
        c1.recv_all(timeout=1)

        c1.send("MODE #topicrestrict +t")
        c1.recv_all(timeout=1)
        c2.recv_all(timeout=1)

        c2.send("TOPIC #topicrestrict :I'm not op")
        lines = c2.recv_all(timeout=1)
        results.check("TOPIC: +t non-op → 482", has_code(lines, "482"))


def test_mode():
    """Test MODE command."""
    print("\n=== MODE ===")

    # 1. Query channel mode → 324
    with IRCClient() as c:
        c.register("modeuser")
        c.send("JOIN #modetest")
        c.recv_all(timeout=1)
        c.send("MODE #modetest")
        lines = c.recv_all(timeout=1)
        results.check("MODE: query → 324", has_code(lines, "324"))

    # 2. Set +i invite only
    with IRCClient() as c:
        c.register("modeop")
        c.send("JOIN #modei")
        c.recv_all(timeout=1)
        c.send("MODE #modei +i")
        lines = c.recv_all(timeout=1)
        results.check("MODE: +i broadcasts", contains(lines, "+i"))

    # 3. Set +k with key
    with IRCClient() as c:
        c.register("modekop")
        c.send("JOIN #modek")
        c.recv_all(timeout=1)
        c.send("MODE #modek +k secretkey")
        lines = c.recv_all(timeout=1)
        results.check("MODE: +k broadcasts", contains(lines, "+k") and contains(lines, "secretkey"))

    # 4. Non-op can't set modes → 482
    with IRCClient() as c1, IRCClient() as c2:
        c1.register("realop")
        c1.send("JOIN #modenopower")
        c1.recv_all(timeout=1)

        c2.register("notop")
        c2.send("JOIN #modenopower")
        c2.recv_all(timeout=1)
        c1.recv_all(timeout=1)

        c2.send("MODE #modenopower +i")
        lines = c2.recv_all(timeout=1)
        results.check("MODE: non-op → 482", has_code(lines, "482"))

    # 5. +o grants operator
    with IRCClient() as c1, IRCClient() as c2:
        c1.register("opgranter")
        c1.send("JOIN #modeo")
        c1.recv_all(timeout=1)

        c2.register("newop")
        c2.send("JOIN #modeo")
        c2.recv_all(timeout=1)
        c1.recv_all(timeout=1)

        c1.send("MODE #modeo +o newop")
        lines = c1.recv_all(timeout=1)
        results.check("MODE: +o broadcasts", contains(lines, "+o") and contains(lines, "newop"))

        # now newop should be able to set modes
        c2.send("MODE #modeo +t")
        lines2 = c2.recv_all(timeout=1)
        results.check("MODE: newly opped user can set modes", contains(lines2, "+t"))

    # 6. +l user limit
    with IRCClient() as c:
        c.register("limitop")
        c.send("JOIN #modelimit")
        c.recv_all(timeout=1)
        c.send("MODE #modelimit +l 2")
        lines = c.recv_all(timeout=1)
        results.check("MODE: +l broadcasts", contains(lines, "+l"))

    # 7. User mode query → 221
    with IRCClient() as c:
        c.register("umodeuser")
        c.send("MODE umodeuser")
        lines = c.recv_all(timeout=1)
        results.check("MODE: user mode → 221", has_code(lines, "221"))

    # 8. Unknown mode char → 472
    with IRCClient() as c:
        c.register("unkmode")
        c.send("JOIN #modeunk")
        c.recv_all(timeout=1)
        c.send("MODE #modeunk +x")
        lines = c.recv_all(timeout=1)
        results.check("MODE: unknown mode → 472", has_code(lines, "472"))

    # 9. MODE no args → 461
    with IRCClient() as c:
        c.register("modenoarg")
        c.send("MODE")
        lines = c.recv_all(timeout=1)
        results.check("MODE: no args → 461", has_code(lines, "461"))


def test_invite():
    """Test INVITE command."""
    print("\n=== INVITE ===")

    # 1. Basic invite
    with IRCClient() as c1, IRCClient() as c2:
        c1.register("inviter")
        c1.send("JOIN #invtest")
        c1.recv_all(timeout=1)

        c2.register("invitee")
        c1.send("INVITE invitee #invtest")
        lines1 = c1.recv_all(timeout=1)
        results.check("INVITE: inviter gets 341", has_code(lines1, "341"))

        lines2 = c2.recv_all(timeout=1)
        results.check("INVITE: invitee gets INVITE msg", contains(lines2, "INVITE"))

    # 2. Invite + join to invite-only channel
    with IRCClient() as c1, IRCClient() as c2:
        c1.register("invop")
        c1.send("JOIN #invchan")
        c1.recv_all(timeout=1)
        c1.send("MODE #invchan +i")
        c1.recv_all(timeout=1)

        c2.register("invguest")

        # Without invite → 473
        c2.send("JOIN #invchan")
        lines = c2.recv_all(timeout=1)
        results.check("INVITE: +i without invite → 473", has_code(lines, "473"))

        # Invite, then join
        c1.send("INVITE invguest #invchan")
        c1.recv_all(timeout=1)
        c2.recv_all(timeout=1)

        c2.send("JOIN #invchan")
        lines = c2.recv_all(timeout=1)
        results.check("INVITE: +i with invite → JOIN succeeds", contains(lines, "JOIN #invchan"))

    # 3. INVITE no args → 461
    with IRCClient() as c:
        c.register("invnoargs")
        c.send("INVITE")
        lines = c.recv_all(timeout=1)
        results.check("INVITE: no args → 461", has_code(lines, "461"))

    # 4. INVITE nonexistent nick → 401
    with IRCClient() as c:
        c.register("invbad")
        c.send("JOIN #invbadchan")
        c.recv_all(timeout=1)
        c.send("INVITE ghostnick #invbadchan")
        lines = c.recv_all(timeout=1)
        results.check("INVITE: no such nick → 401", has_code(lines, "401"))

    # 5. Non-op can't invite to +i channel → 482
    with IRCClient() as c1, IRCClient() as c2, IRCClient() as c3:
        c1.register("invop2")
        c1.send("JOIN #invperm")
        c1.recv_all(timeout=1)
        c1.send("MODE #invperm +i")
        c1.recv_all(timeout=1)

        c2.register("invpleb2")
        c1.send("INVITE invpleb2 #invperm")
        c1.recv_all(timeout=1)
        c2.recv_all(timeout=1)
        c2.send("JOIN #invperm")
        c2.recv_all(timeout=1)
        c1.recv_all(timeout=1)

        c3.register("invtgt2")
        c2.send("INVITE invtgt2 #invperm")
        lines = c2.recv_all(timeout=1)
        results.check("INVITE: non-op +i → 482", has_code(lines, "482"))


def test_kick():
    """Test KICK command."""
    print("\n=== KICK ===")

    # 1. Basic kick
    with IRCClient() as c1, IRCClient() as c2:
        c1.register("kicker")
        c1.send("JOIN #kicktest")
        c1.recv_all(timeout=1)

        c2.register("kicked")
        c2.send("JOIN #kicktest")
        c2.recv_all(timeout=1)
        c1.recv_all(timeout=1)

        c1.send("KICK #kicktest kicked :bye bye")
        lines1 = c1.recv_all(timeout=1)
        results.check("KICK: op gets KICK echo", contains(lines1, "KICK"))

        lines2 = c2.recv_all(timeout=1)
        results.check("KICK: target gets KICK msg", contains(lines2, "KICK"))
        results.check("KICK: reason included", contains(lines2, "bye bye"))

    # 2. Non-op can't kick → 482
    with IRCClient() as c1, IRCClient() as c2, IRCClient() as c3:
        c1.register("kickop2")
        c1.send("JOIN #kickperm")
        c1.recv_all(timeout=1)

        c2.register("kickpleb")
        c2.send("JOIN #kickperm")
        c2.recv_all(timeout=1)
        c1.recv_all(timeout=1)

        c3.register("kickvctm")
        c3.send("JOIN #kickperm")
        c3.recv_all(timeout=1)
        c1.recv_all(timeout=1)
        c2.recv_all(timeout=1)

        c2.send("KICK #kickperm kickvctm :nope")
        lines = c2.recv_all(timeout=1)
        results.check("KICK: non-op → 482", has_code(lines, "482"))

    # 3. KICK no args → 461
    with IRCClient() as c:
        c.register("kickna")
        time.sleep(0.2)
        c.send("KICK")
        lines = c.recv_all(timeout=1)
        results.check("KICK: no args → 461", has_code(lines, "461"),
                       f"got: {lines}")

    # 4. KICK nonexistent channel → 403
    with IRCClient() as c:
        c.register("kicknoch")
        c.send("KICK #ghostchan somebody")
        lines = c.recv_all(timeout=1)
        results.check("KICK: no such channel → 403", has_code(lines, "403"))

    # 5. KICK user not on channel → 441
    with IRCClient() as c1, IRCClient() as c2:
        c1.register("kickop3")
        c1.send("JOIN #kick441")
        c1.recv_all(timeout=1)

        c2.register("notonit")
        c1.send("KICK #kick441 notonit")
        lines = c1.recv_all(timeout=1)
        results.check("KICK: target not on channel → 441", has_code(lines, "441"))


def test_channel_key():
    """Test channel key (+k) enforcement on JOIN."""
    print("\n=== Channel Key (+k) ===")

    with IRCClient() as c1, IRCClient() as c2, IRCClient() as c3:
        c1.register("keyop")
        c1.send("JOIN #keychan")
        c1.recv_all(timeout=1)
        c1.send("MODE #keychan +k mykey")
        c1.recv_all(timeout=1)

        # Wrong key → 475
        c2.register("wrongkey")
        c2.send("JOIN #keychan badkey")
        lines = c2.recv_all(timeout=1)
        results.check("KEY: wrong key → 475", has_code(lines, "475"))

        # No key → 475
        c2.send("JOIN #keychan")
        lines = c2.recv_all(timeout=1)
        results.check("KEY: no key → 475", has_code(lines, "475"))

        # Correct key → join succeeds
        c3.register("rightkey")
        c3.send("JOIN #keychan mykey")
        lines = c3.recv_all(timeout=1)
        results.check("KEY: correct key → JOIN succeeds", contains(lines, "JOIN #keychan"))


def test_channel_limit():
    """Test channel user limit (+l) enforcement on JOIN."""
    print("\n=== Channel Limit (+l) ===")

    with IRCClient() as c1, IRCClient() as c2, IRCClient() as c3:
        c1.register("lmtop")
        c1.send("JOIN #limitchan")
        c1.recv_all(timeout=1)
        c1.send("MODE #limitchan +l 2")
        c1.recv_all(timeout=1)

        c2.register("lmtjoin")
        time.sleep(0.2)
        c2.send("JOIN #limitchan")
        lines = c2.recv_all(timeout=1)
        results.check("LIMIT: second user can join", contains(lines, "JOIN #limitchan"),
                       f"got: {lines}")

        c3.register("lmtblock")
        time.sleep(0.2)
        c3.send("JOIN #limitchan")
        lines = c3.recv_all(timeout=1)
        results.check("LIMIT: third user blocked → 471", has_code(lines, "471"),
                       f"got: {lines}")


def test_motd():
    """Test MOTD command."""
    print("\n=== MOTD ===")

    with IRCClient() as c:
        c.register("motduser")
        c.send("MOTD")
        lines = c.recv_all(timeout=1)
        # Should get either MOTD (375+372+376) or ERR_NOMOTD (422)
        has_motd = has_code(lines, "375") and has_code(lines, "376")
        has_no_motd = has_code(lines, "422")
        results.check("MOTD: returns MOTD or 422", has_motd or has_no_motd)


def test_partial_messages():
    """Test that partial messages are buffered correctly."""
    print("\n=== Partial Message Buffering ===")

    with IRCClient() as c:
        # Send registration in fragments
        c.sock.sendall(b"PASS testpas")
        time.sleep(0.1)
        c.sock.sendall(b"sword\r\nNICK fra")
        time.sleep(0.1)
        c.sock.sendall(b"guser\r\nUSER fraguser 0 * :Frag\r\n")
        lines = c.recv_until("376", timeout=3)
        if not any("376" in l for l in lines):
            more = c.recv_until("422", timeout=1)
            lines.extend(more)
        results.check("BUFFER: fragmented registration succeeds", has_code(lines, "001"))


def test_multiple_commands_one_packet():
    """Test multiple commands sent in a single TCP packet."""
    print("\n=== Multiple Commands in One Packet ===")

    with IRCClient() as c:
        packet = f"PASS {PASSWORD}\r\nNICK bulkuser\r\nUSER bulkuser 0 * :Bulk\r\n"
        c.sock.sendall(packet.encode())
        lines = c.recv_until("376", timeout=3)
        if not any("376" in l for l in lines):
            more = c.recv_until("422", timeout=1)
            lines.extend(more)
        results.check("BULK: all commands processed", has_code(lines, "001"))


def test_empty_channel_cleanup():
    """Test that channels are cleaned up when all users leave."""
    print("\n=== Empty Channel Cleanup ===")

    with IRCClient() as c:
        c.register("cleanup1")
        c.send("JOIN #ephemeral")
        c.recv_all(timeout=1)
        c.send("PART #ephemeral")
        c.recv_all(timeout=1)

        # Re-joining should make us op again (channel was destroyed and recreated)
        c.send("JOIN #ephemeral")
        lines = c.recv_all(timeout=1)
        nameline = find_line(lines, "353")
        results.check("CLEANUP: re-join after empty → user is op",
                       nameline is not None and "@cleanup1" in nameline)


def test_operator_privileges_after_kick():
    """Test that kicked users lose operator status."""
    print("\n=== Operator After Kick ===")

    with IRCClient() as c1, IRCClient() as c2:
        c1.register("kickop4")
        c1.send("JOIN #opkick")
        c1.recv_all(timeout=1)

        c2.register("demoted")
        c2.send("JOIN #opkick")
        c2.recv_all(timeout=1)
        c1.recv_all(timeout=1)

        # Give op then kick
        c1.send("MODE #opkick +o demoted")
        c1.recv_all(timeout=1)
        c2.recv_all(timeout=1)

        c1.send("KICK #opkick demoted :removed")
        c1.recv_all(timeout=1)
        c2.recv_all(timeout=1)

        # demoted rejoins — should NOT be op
        c2.send("JOIN #opkick")
        lines = c2.recv_all(timeout=1)
        nameline = find_line(lines, "353")
        # demoted should appear without @
        has_at_demoted = nameline is not None and "@demoted" in nameline
        results.check("OPKICK: rejoining after kick → not op", not has_at_demoted)


def test_nick_collision_during_use():
    """Test that two registered users can't have the same nick."""
    print("\n=== Nick Collision ===")

    with IRCClient() as c1, IRCClient() as c2:
        c1.register("uniqnick")
        c2.register("othrnick")
        time.sleep(0.2)

        c2.send("NICK uniqnick")
        lines = c2.recv_all(timeout=1)
        results.check("COLLISION: nick in use → 433", has_code(lines, "433"),
                       f"got: {lines}")


def test_deop():
    """Test removing operator with -o."""
    print("\n=== De-op (-o) ===")

    with IRCClient() as c1, IRCClient() as c2:
        c1.register("deopop")
        c1.send("JOIN #deoptest")
        c1.recv_all(timeout=1)

        c2.register("deoptgt")
        c2.send("JOIN #deoptest")
        c2.recv_all(timeout=1)
        c1.recv_all(timeout=1)
        time.sleep(0.2)

        c1.send("MODE #deoptest +o deoptgt")
        c1.recv_all(timeout=1)
        c2.recv_all(timeout=1)
        time.sleep(0.2)

        # deoptgt should be able to set modes now
        c2.send("MODE #deoptest +t")
        lines = c2.recv_all(timeout=1)
        results.check("DEOP: opped user can set modes", contains(lines, "+t"),
                       f"got: {lines}")

        # Now deop them
        c1.send("MODE #deoptest -o deoptgt")
        c1.recv_all(timeout=1)
        c2.recv_all(timeout=1)
        time.sleep(0.2)

        # deoptgt should NOT be able to set modes anymore
        c2.send("MODE #deoptest -t")
        lines = c2.recv_all(timeout=1)
        results.check("DEOP: de-opped user can't set modes → 482", has_code(lines, "482"),
                       f"got: {lines}")


def test_remove_key():
    """Test removing channel key with -k."""
    print("\n=== Remove Key (-k) ===")

    with IRCClient() as c1, IRCClient() as c2:
        c1.register("rkeyop")
        c1.send("JOIN #rkeychan")
        c1.recv_all(timeout=1)
        c1.send("MODE #rkeychan +k secret")
        c1.recv_all(timeout=1)

        # Set, then remove key
        c1.send("MODE #rkeychan -k secret")
        c1.recv_all(timeout=1)

        # c2 should be able to join without key
        c2.register("rkeyuser")
        c2.send("JOIN #rkeychan")
        lines = c2.recv_all(timeout=1)
        results.check("RKEY: join after -k succeeds", contains(lines, "JOIN #rkeychan"))


def test_remove_limit():
    """Test removing channel limit with -l."""
    print("\n=== Remove Limit (-l) ===")

    with IRCClient() as c1, IRCClient() as c2, IRCClient() as c3:
        c1.register("rlmtop")
        c1.send("JOIN #rlimitchan")
        c1.recv_all(timeout=1)
        c1.send("MODE #rlimitchan +l 1")
        c1.recv_all(timeout=1)
        time.sleep(0.2)

        # Should be full
        c2.register("rlmtblk")
        c2.send("JOIN #rlimitchan")
        lines = c2.recv_all(timeout=1)
        results.check("RLIMIT: blocked at limit → 471", has_code(lines, "471"),
                       f"got: {lines}")

        # Remove limit
        c1.send("MODE #rlimitchan -l")
        c1.recv_all(timeout=1)
        time.sleep(0.2)

        # Now join should work
        c3.register("rlmtok")
        c3.send("JOIN #rlimitchan")
        lines = c3.recv_all(timeout=1)
        results.check("RLIMIT: join after -l succeeds", contains(lines, "JOIN #rlimitchan"),
                       f"got: {lines}")


# ─── Main ────────────────────────────────────────────────────────────────────

def main():
    global HOST, PORT, PASSWORD

    parser = argparse.ArgumentParser(description="ft_irc integration tests")
    parser.add_argument("--host", default=HOST)
    parser.add_argument("--port", type=int, default=PORT)
    parser.add_argument("--password", default=PASSWORD)
    parser.add_argument("--no-server", action="store_true",
                        help="Don't start/stop the server, test against an existing one")
    args = parser.parse_args()

    HOST = args.host
    PORT = args.port
    PASSWORD = args.password

    server_proc = None

    if not args.no_server:
        # Build
        print("Building ircserv…")
        build = subprocess.run(["make", "re"], cwd=os.path.dirname(os.path.abspath(__file__)),
                               capture_output=True, text=True)
        if build.returncode != 0:
            print("Build failed:")
            print(build.stderr)
            sys.exit(1)
        print("Build OK\n")

        # Start server
        server_path = os.path.join(os.path.dirname(os.path.abspath(__file__)), "ircserv")
        server_proc = subprocess.Popen(
            [server_path, str(PORT), PASSWORD],
            stdout=subprocess.PIPE, stderr=subprocess.PIPE
        )
        time.sleep(0.5)  # give server time to bind

        if server_proc.poll() is not None:
            print("Server failed to start!")
            print(server_proc.stderr.read().decode())
            sys.exit(1)
        print(f"Server started (PID {server_proc.pid}) on port {PORT}")

    try:
        print(f"\nTesting IRC server at {HOST}:{PORT}")
        print("=" * 60)

        test_registration()
        test_nick_change()
        test_join()
        test_part()
        test_privmsg()
        test_topic()
        test_mode()
        test_invite()
        test_kick()
        test_channel_key()
        test_channel_limit()
        test_motd()
        test_partial_messages()
        test_multiple_commands_one_packet()
        test_empty_channel_cleanup()
        test_operator_privileges_after_kick()
        test_nick_collision_during_use()
        test_deop()
        test_remove_key()
        test_remove_limit()

        success = results.summary()

    finally:
        if server_proc:
            server_proc.send_signal(signal.SIGTERM)
            server_proc.wait(timeout=5)
            print(f"\nServer stopped (PID {server_proc.pid})")

    sys.exit(0 if success else 1)


if __name__ == "__main__":
    main()
