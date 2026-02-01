## References:

- ircdocs.horse
- RFC 1459 and 2810-2813

## Registration flow

Popular clients send the following:
irssi	CAP LS → PASS → NICK → USER → CAP END
WeeChat	CAP LS → PASS → NICK → USER → CAP END
HexChat	PASS → NICK → USER
mIRC	PASS → NICK → USER

The CAP commands are for IRCv3 capability negotiation (which we don't implement, so we'll return a message not ).