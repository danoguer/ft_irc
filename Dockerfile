FROM debian:bookworm-slim AS builder

RUN apt-get update && apt-get install -y \
    build-essential \
    clang \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY . .

RUN make && make bonus

FROM debian:bookworm-slim

WORKDIR /app

RUN apt-get update && apt-get install -y \
    netcat-openbsd \
    && rm -rf /var/lib/apt/lists/*

COPY --from=builder /app/ircserv .
COPY --from=builder /app/ircbot .

RUN useradd -m -s /bin/bash ircuser && \
    chown -R ircuser:ircuser /app

USER ircuser

CMD ["./ircserv"]
