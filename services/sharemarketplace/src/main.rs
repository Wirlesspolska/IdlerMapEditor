mod db;
mod error;

use axum::extract::{Multipart, Path, Query, State};
use axum::http::{header, HeaderMap, StatusCode};
use axum::response::{IntoResponse, Response};
use axum::routing::{get, post};
use axum::{Json, Router};
use db::{Db, CATEGORIES};
use error::{AppError, AppResult};
use rand::RngCore;
use serde::{Deserialize, Serialize};
use sha2::{Digest, Sha256};
use std::net::SocketAddr;
use std::path::PathBuf;
use std::sync::Arc;
use tower_http::cors::{Any, CorsLayer};
use tower_http::trace::TraceLayer;

const MAX_UPLOAD_TOTAL: usize = 2 * 1024 * 1024;
const MAX_PREVIEW_BYTES: usize = 512 * 1024;
const MAX_PREVIEW_DIM: u32 = 512;
const PRESENCE_TTL_SECS: i64 = 90;
const UPLOAD_COOLDOWN_TRUST0_SECS: i64 = 3600;
const DOWNLOAD_LIMIT_PER_HOUR: i64 = 60;

#[derive(Clone)]
struct AppState {
    db: Db,
    /// If set (e.g. `.example.com`), Caddy on-demand TLS ask may approve matching hosts.
    tls_allow_suffix: Option<String>,
    /// Exact host always allowed for TLS ask (e.g. `marketplace.example.com`).
    tls_allow_host: Option<String>,
}

#[tokio::main]
async fn main() -> anyhow::Result<()> {
    tracing_subscriber::fmt()
        .with_env_filter(
            tracing_subscriber::EnvFilter::try_from_default_env()
                .unwrap_or_else(|_| "sharemarketplace=info,tower_http=info".into()),
        )
        .init();

    // Docker / public listen: set SHAREMARKETPLACE_BIND=0.0.0.0:8787
    let bind = std::env::var("SHAREMARKETPLACE_BIND").unwrap_or_else(|_| "127.0.0.1:8787".into());
    let data = std::env::var("SHAREMARKETPLACE_DATA").unwrap_or_else(|_| "./data".into());
    let tls_allow_suffix = std::env::var("SHAREMARKETPLACE_TLS_ALLOW_SUFFIX")
        .ok()
        .map(|s| s.trim().to_string())
        .filter(|s| !s.is_empty());
    let tls_allow_host = std::env::var("SHAREMARKETPLACE_TLS_ALLOW_HOST")
        .ok()
        .map(|s| s.trim().to_ascii_lowercase())
        .filter(|s| !s.is_empty());
    let db = Db::open(PathBuf::from(data).as_path())?;
    let state = AppState {
        db,
        tls_allow_suffix,
        tls_allow_host,
    };

    let cors = CorsLayer::new()
        .allow_origin(Any)
        .allow_methods(Any)
        .allow_headers(Any);

    let app = Router::new()
        .route("/v1/health", get(health))
        .route("/v1/tls-ask", get(tls_ask))
        .route("/v1/auth/register", post(register))
        .route("/v1/auth/heartbeat", post(heartbeat))
        .route("/v1/categories", get(categories))
        .route("/v1/versions", get(list_versions))
        .route("/v1/listings", get(list_listings).post(create_listing))
        .route("/v1/listings/:id/preview", get(get_preview))
        .route("/v1/listings/:id/download", get(get_download))
        .layer(TraceLayer::new_for_http())
        .layer(cors)
        .with_state(Arc::new(state));

    let addr: SocketAddr = bind.parse()?;
    tracing::info!("sharemarketplace listening on http://{addr}");
    let listener = tokio::net::TcpListener::bind(addr).await?;
    axum::serve(listener, app).await?;
    Ok(())
}

fn now_unix() -> i64 {
    chrono::Utc::now().timestamp()
}

fn sha256_hex(bytes: &[u8]) -> String {
    let mut hasher = Sha256::new();
    hasher.update(bytes);
    hex::encode(hasher.finalize())
}

fn random_session_key() -> String {
    let mut bytes = [0u8; 48];
    rand::thread_rng().fill_bytes(&mut bytes);
    hex::encode(bytes)
}

fn bearer_token(headers: &HeaderMap) -> AppResult<String> {
    let value = headers
        .get(header::AUTHORIZATION)
        .and_then(|v| v.to_str().ok())
        .ok_or(AppError::Unauthorized)?;
    let token = value
        .strip_prefix("Bearer ")
        .or_else(|| value.strip_prefix("bearer "))
        .ok_or(AppError::Unauthorized)?
        .trim();
    if token.is_empty() {
        return Err(AppError::Unauthorized);
    }
    Ok(token.to_string())
}

fn is_png(bytes: &[u8]) -> bool {
    bytes.len() >= 8 && bytes[..8] == [0x89, b'P', b'N', b'G', b'\r', b'\n', 0x1a, b'\n']
}

/// Cheap IHDR width/height check without full image decode.
fn png_dimensions(bytes: &[u8]) -> Option<(u32, u32)> {
    if bytes.len() < 24 || !is_png(bytes) {
        return None;
    }
    // IHDR chunk starts at offset 8; width/height at 16/20
    let w = u32::from_be_bytes(bytes[16..20].try_into().ok()?);
    let h = u32::from_be_bytes(bytes[20..24].try_into().ok()?);
    Some((w, h))
}

fn validate_patch_json(bytes: &[u8]) -> AppResult<()> {
    let value: serde_json::Value =
        serde_json::from_slice(bytes).map_err(|_| AppError::BadRequest("patch is not valid JSON".into()))?;
    let obj = value
        .as_object()
        .ok_or_else(|| AppError::BadRequest("patch root must be object".into()))?;
    let format = obj.get("format").and_then(|v| v.as_str()).unwrap_or("");
    if !format.is_empty() && format != "rme-map-patch" {
        return Err(AppError::BadRequest("unsupported patch format".into()));
    }
    let version = obj.get("version").and_then(|v| v.as_i64()).unwrap_or(1);
    if version != 1 {
        return Err(AppError::BadRequest("unsupported patch version".into()));
    }
    Ok(())
}

fn valid_category(id: &str) -> bool {
    CATEGORIES.iter().any(|(cid, _)| *cid == id)
}

#[derive(Serialize)]
struct HealthResponse {
    ok: bool,
    usersOnline: i64,
}

async fn health(State(state): State<Arc<AppState>>) -> AppResult<Json<HealthResponse>> {
    let online = state.db.users_online(now_unix(), PRESENCE_TTL_SECS)?;
    Ok(Json(HealthResponse {
        ok: true,
        usersOnline: online,
    }))
}

#[derive(Deserialize)]
struct TlsAskQuery {
    domain: Option<String>,
}

/// Caddy on-demand TLS permission check: 200 allow, 400 deny.
async fn tls_ask(State(state): State<Arc<AppState>>, Query(query): Query<TlsAskQuery>) -> StatusCode {
    let Some(domain) = query.domain.filter(|d| !d.is_empty()) else {
        return StatusCode::BAD_REQUEST;
    };
    let domain = domain.trim().to_ascii_lowercase();
    if domain.contains("..") || domain.starts_with('.') || domain.ends_with('.') {
        return StatusCode::BAD_REQUEST;
    }

    if let Some(exact) = &state.tls_allow_host {
        if &domain == exact {
            return StatusCode::OK;
        }
    }
    if let Some(suffix) = &state.tls_allow_suffix {
        let suffix = suffix.to_ascii_lowercase();
        let ok = if suffix.starts_with('.') {
            domain.ends_with(&suffix) && domain.len() > suffix.len()
        } else {
            domain == suffix || domain.ends_with(&format!(".{suffix}"))
        };
        if ok {
            return StatusCode::OK;
        }
    }

    // If no allow-list configured, deny (safe default for public deploys).
    StatusCode::BAD_REQUEST
}

#[derive(Deserialize)]
struct RegisterRequest {
    nickname: String,
    hwidHash: String,
}

#[derive(Serialize)]
struct RegisterResponse {
    sessionKey: String,
    userId: String,
    trust: i64,
}

async fn register(
    State(state): State<Arc<AppState>>,
    Json(body): Json<RegisterRequest>,
) -> AppResult<Json<RegisterResponse>> {
    let session = random_session_key();
    let user = state
        .db
        .register_user(&body.nickname, &body.hwidHash, &session, now_unix())?;
    Ok(Json(RegisterResponse {
        sessionKey: user.session_key,
        userId: user.id,
        trust: user.trust,
    }))
}

#[derive(Serialize)]
struct HeartbeatResponse {
    usersOnline: i64,
    trust: i64,
}

async fn heartbeat(
    State(state): State<Arc<AppState>>,
    headers: HeaderMap,
) -> AppResult<Json<HeartbeatResponse>> {
    let token = bearer_token(&headers)?;
    let user = state.db.user_by_session(&token)?;
    let now = now_unix();
    state.db.heartbeat(&user.id, now)?;
    let online = state.db.users_online(now, PRESENCE_TTL_SECS)?;
    Ok(Json(HeartbeatResponse {
        usersOnline: online,
        trust: user.trust,
    }))
}

#[derive(Serialize)]
struct CategoryDto {
    id: String,
    name: String,
}

async fn categories() -> Json<Vec<CategoryDto>> {
    Json(
        CATEGORIES
            .iter()
            .map(|(id, name)| CategoryDto {
                id: (*id).into(),
                name: (*name).into(),
            })
            .collect(),
    )
}

#[derive(Deserialize)]
struct ListQuery {
    category: Option<String>,
    /// Client/items VERSION name filter (e.g. "10.98")
    #[serde(alias = "clientVersion")]
    version: Option<String>,
    #[serde(alias = "clientVersionId")]
    version_id: Option<i64>,
    q: Option<String>,
    sort: Option<String>,
    page: Option<i64>,
}

async fn list_versions(State(state): State<Arc<AppState>>) -> AppResult<Json<Vec<db::VersionInfo>>> {
    Ok(Json(state.db.list_versions()?))
}

async fn list_listings(
    State(state): State<Arc<AppState>>,
    Query(query): Query<ListQuery>,
) -> AppResult<Json<Vec<db::ListingCard>>> {
    let sort = query.sort.as_deref().unwrap_or("new");
    let page = query.page.unwrap_or(1);
    let cards = state.db.list_listings(
        query.category.as_deref(),
        query.version.as_deref(),
        query.version_id,
        query.q.as_deref(),
        sort,
        page,
        48,
    )?;
    Ok(Json(cards))
}

async fn get_preview(
    State(state): State<Arc<AppState>>,
    Path(id): Path<String>,
) -> AppResult<Response> {
    let (preview_sha, _) = state.db.listing_hashes(&id)?;
    let bytes = state.db.read_blob(&preview_sha)?;
    Ok((
        StatusCode::OK,
        [(header::CONTENT_TYPE, "image/png")],
        bytes,
    )
        .into_response())
}

async fn get_download(
    State(state): State<Arc<AppState>>,
    headers: HeaderMap,
    Path(id): Path<String>,
) -> AppResult<Response> {
    let now = now_unix();
    let user_id = match bearer_token(&headers) {
        Ok(token) => {
            let user = state.db.user_by_session(&token)?;
            let since = now - 3600;
            let count = state.db.count_downloads_since(&user.id, since)?;
            if count >= DOWNLOAD_LIMIT_PER_HOUR {
                return Err(AppError::RateLimited);
            }
            Some(user.id)
        }
        Err(_) => None,
    };

    let (_, patch_sha) = state.db.listing_hashes(&id)?;
    let bytes = state.db.read_blob(&patch_sha)?;
    state.db.record_download(user_id.as_deref(), &id, now)?;

    Ok((
        StatusCode::OK,
        [(header::CONTENT_TYPE, "application/json")],
        bytes,
    )
        .into_response())
}

#[derive(Deserialize)]
struct ListingMeta {
    title: String,
    category: String,
    contentSha256: String,
    /// Required — client/items VERSION (spr/dat), e.g. "10.98"
    #[serde(alias = "client_version")]
    clientVersion: String,
    #[serde(default, alias = "client_version_id")]
    clientVersionId: i64,
}

async fn create_listing(
    State(state): State<Arc<AppState>>,
    headers: HeaderMap,
    mut multipart: Multipart,
) -> AppResult<Json<serde_json::Value>> {
    let token = bearer_token(&headers)?;
    let user = state.db.user_by_session(&token)?;
    let now = now_unix();

    if user.trust <= 0 {
        let elapsed = now - user.last_upload_unix;
        if user.last_upload_unix > 0 && elapsed < UPLOAD_COOLDOWN_TRUST0_SECS {
            return Err(AppError::RateLimited);
        }
    }

    let mut meta: Option<ListingMeta> = None;
    let mut preview: Option<Vec<u8>> = None;
    let mut patch: Option<Vec<u8>> = None;
    let mut total = 0usize;

    while let Some(field) = multipart
        .next_field()
        .await
        .map_err(|e| AppError::BadRequest(format!("multipart: {e}")))?
    {
        let name = field.name().unwrap_or("").to_string();
        let data = field
            .bytes()
            .await
            .map_err(|e| AppError::BadRequest(format!("multipart read: {e}")))?;
        total = total.saturating_add(data.len());
        if total > MAX_UPLOAD_TOTAL {
            return Err(AppError::PayloadTooLarge);
        }
        match name.as_str() {
            "meta" => {
                meta = Some(
                    serde_json::from_slice(&data)
                        .map_err(|_| AppError::BadRequest("invalid meta JSON".into()))?,
                );
            }
            "preview" => preview = Some(data.to_vec()),
            "patch" => patch = Some(data.to_vec()),
            _ => {}
        }
    }

    let meta = meta.ok_or_else(|| AppError::BadRequest("missing meta".into()))?;
    let preview = preview.ok_or_else(|| AppError::BadRequest("missing preview".into()))?;
    let patch = patch.ok_or_else(|| AppError::BadRequest("missing patch".into()))?;

    let title = meta.title.trim();
    if title.is_empty() || title.len() > 80 {
        return Err(AppError::BadRequest("title must be 1-80 characters".into()));
    }
    if !valid_category(&meta.category) {
        return Err(AppError::BadRequest("unknown category".into()));
    }
    let client_version = meta.clientVersion.trim().to_string();
    if client_version.is_empty() || client_version.len() > 64 {
        return Err(AppError::BadRequest(
            "clientVersion is required (items VERSION / spr-dat identity)".into(),
        ));
    }
    if meta.clientVersionId < 0 {
        return Err(AppError::BadRequest("clientVersionId invalid".into()));
    }
    if preview.len() > MAX_PREVIEW_BYTES {
        return Err(AppError::PayloadTooLarge);
    }
    if !is_png(&preview) {
        return Err(AppError::BadRequest("preview must be PNG".into()));
    }
    let (w, h) = png_dimensions(&preview).ok_or_else(|| AppError::BadRequest("invalid PNG".into()))?;
    if w == 0 || h == 0 || w > MAX_PREVIEW_DIM || h > MAX_PREVIEW_DIM {
        return Err(AppError::BadRequest(format!(
            "preview dimensions must be 1-{MAX_PREVIEW_DIM}px"
        )));
    }

    validate_patch_json(&patch)?;

    let preview_sha = sha256_hex(&preview);
    let patch_sha = sha256_hex(&patch);
    let content_sha = sha256_hex(
        format!(
            "{preview_sha}:{patch_sha}:{}:{}:{}",
            meta.category, client_version, meta.clientVersionId
        )
        .as_bytes(),
    );
    if !meta.contentSha256.is_empty() && meta.contentSha256 != content_sha && meta.contentSha256 != patch_sha
    {
        // Accept client content hash as patch hash OR combined hash for flexibility.
        if meta.contentSha256 != patch_sha {
            tracing::warn!(
                "contentSha256 mismatch client={} server_combined={} patch={}",
                meta.contentSha256,
                content_sha,
                patch_sha
            );
        }
    }

    state.db.store_blob(&preview_sha, &preview)?;
    state.db.store_blob(&patch_sha, &patch)?;
    let id = state.db.insert_listing(
        &user.id,
        title,
        &meta.category,
        &content_sha,
        &preview_sha,
        &patch_sha,
        (preview.len() + patch.len()) as i64,
        &client_version,
        meta.clientVersionId,
        now,
    )?;
    state.db.mark_upload(&user.id, now)?;
    state.db.heartbeat(&user.id, now)?;

    Ok(Json(serde_json::json!({
        "id": id,
        "previewUrl": format!("/v1/listings/{id}/preview"),
        "clientVersion": client_version,
        "clientVersionId": meta.clientVersionId,
    })))
}
