# Multi-stage build for sharemarketplace (Linux amd64/arm64)
FROM rust:1-bookworm AS builder
WORKDIR /app

RUN apt-get update \
    && apt-get install -y --no-install-recommends pkg-config libssl-dev cmake \
    && rm -rf /var/lib/apt/lists/*

# Cache dependencies
COPY Cargo.toml ./
RUN mkdir -p src \
    && printf 'fn main() { println!("placeholder"); }\n' > src/main.rs \
    && cargo build --release \
    && rm -rf src \
    && rm -f target/release/sharemarketplace \
    && rm -f target/release/deps/sharemarketplace-* \
    && rm -rf target/release/.fingerprint/sharemarketplace-*

COPY src ./src
RUN cargo build --release \
    && strip target/release/sharemarketplace

FROM debian:bookworm-slim AS runtime
RUN apt-get update \
    && apt-get install -y --no-install-recommends ca-certificates curl tini \
    && rm -rf /var/lib/apt/lists/* \
    && useradd --system --uid 10001 --home /data --shell /usr/sbin/nologin marketplace

COPY --from=builder /app/target/release/sharemarketplace /usr/local/bin/sharemarketplace

RUN mkdir -p /data/blobs \
    && chown -R marketplace:marketplace /data

USER marketplace
ENV SHAREMARKETPLACE_BIND=0.0.0.0:8787 \
    SHAREMARKETPLACE_DATA=/data \
    RUST_LOG=sharemarketplace=info,tower_http=info

EXPOSE 8787
VOLUME ["/data"]

HEALTHCHECK --interval=30s --timeout=5s --start-period=15s --retries=5 \
    CMD curl -fsS "http://127.0.0.1:8787/v1/health" || exit 1

ENTRYPOINT ["/usr/bin/tini", "--"]
CMD ["sharemarketplace"]
