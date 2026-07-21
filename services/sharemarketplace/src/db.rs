use crate::error::{AppError, AppResult};
use rusqlite::{params, Connection, OptionalExtension};
use std::path::{Path, PathBuf};
use std::sync::{Arc, Mutex};
use uuid::Uuid;

pub const CATEGORIES: &[(&str, &str)] = &[
    ("buildings", "Buildings"),
    ("shops", "Shops"),
    ("dungeons", "Dungeons"),
    ("decor", "Decor"),
    ("roads", "Roads"),
    ("other", "Other"),
];

#[derive(Clone)]
pub struct Db {
    conn: Arc<Mutex<Connection>>,
    blob_root: PathBuf,
}

#[derive(Debug, Clone)]
pub struct UserRow {
    pub id: String,
    pub nickname: String,
    pub hwid_hash: String,
    pub session_key: String,
    pub trust: i64,
    pub last_heartbeat_unix: i64,
    pub last_upload_unix: i64,
}

#[derive(Debug, Clone, serde::Serialize)]
#[serde(rename_all = "camelCase")]
pub struct ListingCard {
    pub id: String,
    pub title: String,
    pub category: String,
    pub preview_url: String,
    pub downloads: i64,
    pub uploader_nick: String,
    pub created_at: i64,
    pub size_bytes: i64,
    /// Client/items VERSION name (e.g. "10.98") — spr/dat identity.
    pub client_version: String,
    pub client_version_id: i64,
}

#[derive(Debug, Clone, serde::Serialize)]
#[serde(rename_all = "camelCase")]
pub struct VersionInfo {
    pub client_version: String,
    pub client_version_id: i64,
    pub count: i64,
}

impl Db {
    pub fn open(data_dir: &Path) -> AppResult<Self> {
        std::fs::create_dir_all(data_dir).map_err(|e| AppError::Internal(e.into()))?;
        let blob_root = data_dir.join("blobs");
        std::fs::create_dir_all(&blob_root).map_err(|e| AppError::Internal(e.into()))?;

        let db_path = data_dir.join("sharemarketplace.sqlite");
        let conn = Connection::open(db_path)?;
        conn.execute_batch(
            r#"
            PRAGMA journal_mode=WAL;
            CREATE TABLE IF NOT EXISTS users (
                id TEXT PRIMARY KEY,
                nickname TEXT NOT NULL UNIQUE,
                hwid_hash TEXT NOT NULL,
                session_key TEXT NOT NULL UNIQUE,
                trust INTEGER NOT NULL DEFAULT 0,
                last_heartbeat_unix INTEGER NOT NULL DEFAULT 0,
                last_upload_unix INTEGER NOT NULL DEFAULT 0,
                created_at INTEGER NOT NULL
            );
            CREATE TABLE IF NOT EXISTS listings (
                id TEXT PRIMARY KEY,
                user_id TEXT NOT NULL,
                title TEXT NOT NULL,
                category TEXT NOT NULL,
                content_sha256 TEXT NOT NULL,
                preview_sha256 TEXT NOT NULL,
                patch_sha256 TEXT NOT NULL,
                size_bytes INTEGER NOT NULL,
                downloads INTEGER NOT NULL DEFAULT 0,
                created_at INTEGER NOT NULL,
                client_version TEXT NOT NULL DEFAULT '',
                client_version_id INTEGER NOT NULL DEFAULT 0,
                FOREIGN KEY(user_id) REFERENCES users(id)
            );
            CREATE INDEX IF NOT EXISTS idx_listings_category ON listings(category);
            CREATE INDEX IF NOT EXISTS idx_listings_client_version ON listings(client_version);
            CREATE INDEX IF NOT EXISTS idx_listings_client_version_id ON listings(client_version_id);
            CREATE INDEX IF NOT EXISTS idx_listings_downloads ON listings(downloads DESC);
            CREATE INDEX IF NOT EXISTS idx_listings_created ON listings(created_at DESC);
            CREATE TABLE IF NOT EXISTS download_events (
                user_id TEXT NOT NULL,
                unix_ts INTEGER NOT NULL
            );
            CREATE INDEX IF NOT EXISTS idx_download_events_user_ts ON download_events(user_id, unix_ts);
            "#,
        )?;

        // Additive migration for DBs created before client VERSION columns.
        let _ = conn.execute_batch(
            r#"
            ALTER TABLE listings ADD COLUMN client_version TEXT NOT NULL DEFAULT '';
            ALTER TABLE listings ADD COLUMN client_version_id INTEGER NOT NULL DEFAULT 0;
            CREATE INDEX IF NOT EXISTS idx_listings_client_version ON listings(client_version);
            CREATE INDEX IF NOT EXISTS idx_listings_client_version_id ON listings(client_version_id);
            "#,
        );

        Ok(Self {
            conn: Arc::new(Mutex::new(conn)),
            blob_root,
        })
    }

    pub fn blob_path(&self, sha256_hex: &str) -> PathBuf {
        let prefix = &sha256_hex[..std::cmp::min(2, sha256_hex.len())];
        self.blob_root.join(prefix).join(sha256_hex)
    }

    pub fn store_blob(&self, sha256_hex: &str, bytes: &[u8]) -> AppResult<()> {
        let path = self.blob_path(sha256_hex);
        if let Some(parent) = path.parent() {
            std::fs::create_dir_all(parent).map_err(|e| AppError::Internal(e.into()))?;
        }
        if !path.exists() {
            std::fs::write(&path, bytes).map_err(|e| AppError::Internal(e.into()))?;
        }
        Ok(())
    }

    pub fn read_blob(&self, sha256_hex: &str) -> AppResult<Vec<u8>> {
        let path = self.blob_path(sha256_hex);
        std::fs::read(path).map_err(|_| AppError::NotFound)
    }

    pub fn register_user(&self, nickname: &str, hwid_hash: &str, session_key: &str, now: i64) -> AppResult<UserRow> {
        let nick = nickname.trim();
        if nick.is_empty() || nick.len() > 32 {
            return Err(AppError::BadRequest("nickname must be 1-32 characters".into()));
        }
        if hwid_hash.len() < 32 || hwid_hash.len() > 128 {
            return Err(AppError::BadRequest("invalid hwidHash".into()));
        }

        let conn = self.conn.lock().map_err(|_| AppError::Internal(anyhow::anyhow!("db lock")))?;
        let existing: Option<(String, String)> = conn
            .query_row(
                "SELECT id, hwid_hash FROM users WHERE nickname = ?1",
                params![nick],
                |r| Ok((r.get(0)?, r.get(1)?)),
            )
            .optional()?;

        if let Some((id, bound_hwid)) = existing {
            if bound_hwid != hwid_hash {
                return Err(AppError::Conflict("nickname already bound to another machine".into()));
            }
            // Re-issue session for same nick+hwid
            conn.execute(
                "UPDATE users SET session_key = ?1, last_heartbeat_unix = ?2 WHERE id = ?3",
                params![session_key, now, id],
            )?;
            return self.get_user_by_id_locked(&conn, &id);
        }

        let id = Uuid::new_v4().to_string();
        conn.execute(
            "INSERT INTO users (id, nickname, hwid_hash, session_key, trust, last_heartbeat_unix, last_upload_unix, created_at)
             VALUES (?1, ?2, ?3, ?4, 0, ?5, 0, ?5)",
            params![id, nick, hwid_hash, session_key, now],
        )?;
        self.get_user_by_id_locked(&conn, &id)
    }

    fn get_user_by_id_locked(&self, conn: &Connection, id: &str) -> AppResult<UserRow> {
        conn.query_row(
            "SELECT id, nickname, hwid_hash, session_key, trust, last_heartbeat_unix, last_upload_unix FROM users WHERE id = ?1",
            params![id],
            |r| {
                Ok(UserRow {
                    id: r.get(0)?,
                    nickname: r.get(1)?,
                    hwid_hash: r.get(2)?,
                    session_key: r.get(3)?,
                    trust: r.get(4)?,
                    last_heartbeat_unix: r.get(5)?,
                    last_upload_unix: r.get(6)?,
                })
            },
        )
        .map_err(|_| AppError::NotFound)
    }

    pub fn user_by_session(&self, session_key: &str) -> AppResult<UserRow> {
        let conn = self.conn.lock().map_err(|_| AppError::Internal(anyhow::anyhow!("db lock")))?;
        conn.query_row(
            "SELECT id, nickname, hwid_hash, session_key, trust, last_heartbeat_unix, last_upload_unix FROM users WHERE session_key = ?1",
            params![session_key],
            |r| {
                Ok(UserRow {
                    id: r.get(0)?,
                    nickname: r.get(1)?,
                    hwid_hash: r.get(2)?,
                    session_key: r.get(3)?,
                    trust: r.get(4)?,
                    last_heartbeat_unix: r.get(5)?,
                    last_upload_unix: r.get(6)?,
                })
            },
        )
        .map_err(|_| AppError::Unauthorized)
    }

    pub fn heartbeat(&self, user_id: &str, now: i64) -> AppResult<()> {
        let conn = self.conn.lock().map_err(|_| AppError::Internal(anyhow::anyhow!("db lock")))?;
        conn.execute(
            "UPDATE users SET last_heartbeat_unix = ?1 WHERE id = ?2",
            params![now, user_id],
        )?;
        Ok(())
    }

    pub fn users_online(&self, now: i64, ttl_secs: i64) -> AppResult<i64> {
        let cutoff = now - ttl_secs;
        let conn = self.conn.lock().map_err(|_| AppError::Internal(anyhow::anyhow!("db lock")))?;
        let count: i64 = conn.query_row(
            "SELECT COUNT(*) FROM users WHERE last_heartbeat_unix >= ?1",
            params![cutoff],
            |r| r.get(0),
        )?;
        Ok(count)
    }

    pub fn mark_upload(&self, user_id: &str, now: i64) -> AppResult<()> {
        let conn = self.conn.lock().map_err(|_| AppError::Internal(anyhow::anyhow!("db lock")))?;
        conn.execute(
            "UPDATE users SET last_upload_unix = ?1 WHERE id = ?2",
            params![now, user_id],
        )?;
        Ok(())
    }

    pub fn count_downloads_since(&self, user_id: &str, since: i64) -> AppResult<i64> {
        let conn = self.conn.lock().map_err(|_| AppError::Internal(anyhow::anyhow!("db lock")))?;
        let count: i64 = conn.query_row(
            "SELECT COUNT(*) FROM download_events WHERE user_id = ?1 AND unix_ts >= ?2",
            params![user_id, since],
            |r| r.get(0),
        )?;
        Ok(count)
    }

    pub fn record_download(&self, user_id: Option<&str>, listing_id: &str, now: i64) -> AppResult<()> {
        let conn = self.conn.lock().map_err(|_| AppError::Internal(anyhow::anyhow!("db lock")))?;
        conn.execute(
            "UPDATE listings SET downloads = downloads + 1 WHERE id = ?1",
            params![listing_id],
        )?;
        if let Some(uid) = user_id {
            conn.execute(
                "INSERT INTO download_events (user_id, unix_ts) VALUES (?1, ?2)",
                params![uid, now],
            )?;
        }
        Ok(())
    }

    pub fn insert_listing(
        &self,
        user_id: &str,
        title: &str,
        category: &str,
        content_sha256: &str,
        preview_sha256: &str,
        patch_sha256: &str,
        size_bytes: i64,
        client_version: &str,
        client_version_id: i64,
        now: i64,
    ) -> AppResult<String> {
        let id = Uuid::new_v4().to_string();
        let conn = self.conn.lock().map_err(|_| AppError::Internal(anyhow::anyhow!("db lock")))?;
        conn.execute(
            "INSERT INTO listings (id, user_id, title, category, content_sha256, preview_sha256, patch_sha256, size_bytes, downloads, created_at, client_version, client_version_id)
             VALUES (?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8, 0, ?9, ?10, ?11)",
            params![
                id,
                user_id,
                title,
                category,
                content_sha256,
                preview_sha256,
                patch_sha256,
                size_bytes,
                now,
                client_version,
                client_version_id
            ],
        )?;
        Ok(id)
    }

    pub fn list_versions(&self) -> AppResult<Vec<VersionInfo>> {
        let conn = self.conn.lock().map_err(|_| AppError::Internal(anyhow::anyhow!("db lock")))?;
        let mut stmt = conn.prepare(
            "SELECT client_version, client_version_id, COUNT(*) as cnt
             FROM listings
             WHERE client_version != ''
             GROUP BY client_version, client_version_id
             ORDER BY client_version_id DESC, client_version DESC",
        )?;
        let mut rows = stmt.query([])?;
        let mut out = Vec::new();
        while let Some(row) = rows.next()? {
            out.push(VersionInfo {
                client_version: row.get(0)?,
                client_version_id: row.get(1)?,
                count: row.get(2)?,
            });
        }
        Ok(out)
    }

    pub fn list_listings(
        &self,
        category: Option<&str>,
        client_version: Option<&str>,
        client_version_id: Option<i64>,
        q: Option<&str>,
        sort: &str,
        page: i64,
        page_size: i64,
    ) -> AppResult<Vec<ListingCard>> {
        let offset = (page.max(1) - 1) * page_size;
        let order = if sort == "downloads" {
            "l.downloads DESC, l.created_at DESC"
        } else {
            "l.created_at DESC"
        };

        let mut sql = String::from(
            "SELECT l.id, l.title, l.category, l.downloads, u.nickname, l.created_at, l.size_bytes, l.client_version, l.client_version_id
             FROM listings l JOIN users u ON u.id = l.user_id WHERE 1=1",
        );
        let mut binds: Vec<rusqlite::types::Value> = Vec::new();

        if let Some(cat) = category {
            if !cat.is_empty() {
                sql.push_str(" AND l.category = ?");
                binds.push(rusqlite::types::Value::Text(cat.to_string()));
            }
        }
        // Prefer exact version id when provided (spr/dat identity); else match version name.
        if let Some(vid) = client_version_id {
            if vid > 0 {
                sql.push_str(" AND l.client_version_id = ?");
                binds.push(rusqlite::types::Value::Integer(vid));
            }
        } else if let Some(ver) = client_version {
            if !ver.is_empty() {
                sql.push_str(" AND l.client_version = ?");
                binds.push(rusqlite::types::Value::Text(ver.to_string()));
            }
        }
        if let Some(query) = q {
            let trimmed = query.trim();
            if !trimmed.is_empty() {
                sql.push_str(" AND (l.title LIKE ? OR u.nickname LIKE ? OR l.client_version LIKE ?)");
                let like = format!("%{trimmed}%");
                binds.push(rusqlite::types::Value::Text(like.clone()));
                binds.push(rusqlite::types::Value::Text(like.clone()));
                binds.push(rusqlite::types::Value::Text(like));
            }
        }
        sql.push_str(&format!(" ORDER BY {order} LIMIT ? OFFSET ?"));
        binds.push(rusqlite::types::Value::Integer(page_size));
        binds.push(rusqlite::types::Value::Integer(offset));

        let conn = self.conn.lock().map_err(|_| AppError::Internal(anyhow::anyhow!("db lock")))?;
        let mut stmt = conn.prepare(&sql)?;
        let mut rows = stmt.query(rusqlite::params_from_iter(binds))?;

        let mut out = Vec::new();
        while let Some(row) = rows.next()? {
            let id: String = row.get(0)?;
            out.push(ListingCard {
                id: id.clone(),
                title: row.get(1)?,
                category: row.get(2)?,
                preview_url: format!("/v1/listings/{id}/preview"),
                downloads: row.get(3)?,
                uploader_nick: row.get(4)?,
                created_at: row.get(5)?,
                size_bytes: row.get(6)?,
                client_version: row.get(7)?,
                client_version_id: row.get(8)?,
            });
        }
        Ok(out)
    }

    pub fn listing_hashes(&self, id: &str) -> AppResult<(String, String)> {
        let conn = self.conn.lock().map_err(|_| AppError::Internal(anyhow::anyhow!("db lock")))?;
        conn.query_row(
            "SELECT preview_sha256, patch_sha256 FROM listings WHERE id = ?1",
            params![id],
            |r| Ok((r.get(0)?, r.get(1)?)),
        )
        .map_err(|_| AppError::NotFound)
    }
}
