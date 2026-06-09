FROM debian:bookworm-slim AS builder

RUN apt-get update && apt-get install -y \
    build-essential \
    clang \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY . .

RUN make

FROM debian:bookworm-slim

WORKDIR /app

COPY --from=builder /app/ircserv .

RUN useradd -m -s /bin/bash ircuser && \
    chown -R ircuser:ircuser /app

USER ircuser

CMD ./ircserv ${IRC_PORT} ${IRC_PASSWORD}
