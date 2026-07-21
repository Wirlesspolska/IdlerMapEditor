# sharemarketplace

Rust HTTP service that hosts shared `rme-map-patch` JSON uploads with editor-generated PNG previews.

## Local (dev)

```bash
cargo run --release
```

| Variable | Default | Meaning |
|----------|---------|---------|
| `SHAREMARKETPLACE_BIND` | `127.0.0.1:8787` | Listen address |
| `SHAREMARKETPLACE_DATA` | `./data` | Database + blob root |
| `SHAREMARKETPLACE_TLS_ALLOW_HOST` | _(empty)_ | Exact FQDN allowed for Caddy TLS ask |
| `SHAREMARKETPLACE_TLS_ALLOW_SUFFIX` | _(empty)_ | Suffix (e.g. `.example.com`) for on-demand subdomains |

## Docker / Linux host

Requires Docker Engine + Compose plugin.

### Quick start (API only — no domain)

```bash
cd services/sharemarketplace
chmod +x deploy.sh
./deploy.sh local
curl -fsS http://127.0.0.1:8787/v1/health
```

Editor: `MARKETPLACE_URL=http://127.0.0.1:8787` (or your VPS IP `:8787`).

Or plain compose:

```bash
docker compose up -d --build
```

### Subdomain + HTTPS (Caddy)

```bash
chmod +x deploy.sh
./deploy.sh up --domain example.com --subdomain marketplace --email you@example.com
```

Creates `marketplace.example.com` with automatic HTTPS (Caddy + Let's Encrypt).

Point DNS **before or after** start:

```text
A   marketplace.example.com   → YOUR_VPS_IPV4
```

Editor setting:

```text
MARKETPLACE_URL=https://marketplace.example.com
```

### Explicit host

```bash
./deploy.sh up --host marketplace.example.com --email you@example.com
```

### Wildcard / multi-subdomain

Serves the apex and any subdomain under the domain. Certs are issued on demand; the app’s `/v1/tls-ask` allow-list must approve the hostname.

```bash
./deploy.sh up --wildcard example.com --email you@example.com
```

DNS:

```text
A   example.com     → YOUR_VPS_IPV4
A   *.example.com   → YOUR_VPS_IPV4
```

Editor can use `https://example.com` or `https://marketplace.example.com` (any approved host).

### App only (your own reverse proxy)

```bash
./deploy.sh up --host marketplace.example.com --no-caddy
```

Publishes `0.0.0.0:8787`. Terminate TLS yourself and proxy to that port.

### Ops

```bash
./deploy.sh status
./deploy.sh logs -f
./deploy.sh dns-hint
./deploy.sh url
./deploy.sh down
```

Data persists in the Docker volume `marketplace_data` (`/data` in the container: sqlite + blobs).

## API (v1)

- `GET /v1/health` → `{ ok, usersOnline }`
- `GET /v1/tls-ask?domain=` → `200` allow / `400` deny (Caddy on-demand TLS)
- `POST /v1/auth/register` `{ nickname, hwidHash }` → `{ sessionKey, userId, trust }`
- `POST /v1/auth/heartbeat` Bearer session → `{ usersOnline, trust }`
- `GET /v1/categories`
- `GET /v1/versions` → distinct client/items VERSION tags on listings
- `GET /v1/listings?category=&clientVersion=&clientVersionId=&q=&sort=downloads|new&page=`
- `GET /v1/listings/:id/preview` → PNG
- `GET /v1/listings/:id/download` → JSON patch
- `POST /v1/listings` multipart: `meta` (must include `clientVersion` + `clientVersionId`), `preview`, `patch`

Listings are tagged with the editor’s loaded items VERSION (spr/dat). Filter by version so patches from other clients don’t show by default.

Limits: 2 MB total upload, 512 KB PNG, 1 upload/hour at trust 0, presence TTL 90s.
