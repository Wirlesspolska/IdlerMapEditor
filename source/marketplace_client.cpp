//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#include "main.h"

#include "marketplace_client.h"
#include "gui.h"
#include "json.h"
#include "settings.h"

#include <wx/socket.h>
#include <wx/filename.h>
#include <wx/stdpaths.h>
#include <wx/file.h>
#include <wx/wfstream.h>
#include <wx/txtstrm.h>

#include <sstream>
#include <cstring>

#ifdef __WINDOWS__
#include <windows.h>
#endif

namespace {

// Compact SHA-256 (public-domain style implementation)
class Sha256 {
public:
	Sha256() {
		reset();
	}

	void update(const uint8_t* data, size_t len) {
		for (size_t i = 0; i < len; ++i) {
			data_[datalen_] = data[i];
			datalen_++;
			if (datalen_ == 64) {
				transform();
				bitlen_ += 512;
				datalen_ = 0;
			}
		}
	}

	void final(uint8_t hash[32]) {
		uint32_t i = datalen_;
		if (datalen_ < 56) {
			data_[i++] = 0x80;
			while (i < 56) {
				data_[i++] = 0x00;
			}
		} else {
			data_[i++] = 0x80;
			while (i < 64) {
				data_[i++] = 0x00;
			}
			transform();
			memset(data_, 0, 56);
		}

		bitlen_ += datalen_ * 8;
		data_[63] = static_cast<uint8_t>(bitlen_);
		data_[62] = static_cast<uint8_t>(bitlen_ >> 8);
		data_[61] = static_cast<uint8_t>(bitlen_ >> 16);
		data_[60] = static_cast<uint8_t>(bitlen_ >> 24);
		data_[59] = static_cast<uint8_t>(bitlen_ >> 32);
		data_[58] = static_cast<uint8_t>(bitlen_ >> 40);
		data_[57] = static_cast<uint8_t>(bitlen_ >> 48);
		data_[56] = static_cast<uint8_t>(bitlen_ >> 56);
		transform();

		for (i = 0; i < 4; ++i) {
			hash[i] = (state_[0] >> (24 - i * 8)) & 0xff;
			hash[i + 4] = (state_[1] >> (24 - i * 8)) & 0xff;
			hash[i + 8] = (state_[2] >> (24 - i * 8)) & 0xff;
			hash[i + 12] = (state_[3] >> (24 - i * 8)) & 0xff;
			hash[i + 16] = (state_[4] >> (24 - i * 8)) & 0xff;
			hash[i + 20] = (state_[5] >> (24 - i * 8)) & 0xff;
			hash[i + 24] = (state_[6] >> (24 - i * 8)) & 0xff;
			hash[i + 28] = (state_[7] >> (24 - i * 8)) & 0xff;
		}
	}

private:
	void reset() {
		datalen_ = 0;
		bitlen_ = 0;
		state_[0] = 0x6a09e667;
		state_[1] = 0xbb67ae85;
		state_[2] = 0x3c6ef372;
		state_[3] = 0xa54ff53a;
		state_[4] = 0x510e527f;
		state_[5] = 0x9b05688c;
		state_[6] = 0x1f83d9ab;
		state_[7] = 0x5be0cd19;
	}

	static uint32_t rotr(uint32_t x, uint32_t n) {
		return (x >> n) | (x << (32 - n));
	}

	void transform() {
		static const uint32_t k[64] = {
			0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
			0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
			0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
			0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
			0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
			0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
			0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
			0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
		};

		uint32_t m[64];
		for (uint32_t i = 0, j = 0; i < 16; ++i, j += 4) {
			m[i] = (data_[j] << 24) | (data_[j + 1] << 16) | (data_[j + 2] << 8) | (data_[j + 3]);
		}
		for (uint32_t i = 16; i < 64; ++i) {
			const uint32_t s0 = rotr(m[i - 15], 7) ^ rotr(m[i - 15], 18) ^ (m[i - 15] >> 3);
			const uint32_t s1 = rotr(m[i - 2], 17) ^ rotr(m[i - 2], 19) ^ (m[i - 2] >> 10);
			m[i] = m[i - 16] + s0 + m[i - 7] + s1;
		}

		uint32_t a = state_[0];
		uint32_t b = state_[1];
		uint32_t c = state_[2];
		uint32_t d = state_[3];
		uint32_t e = state_[4];
		uint32_t f = state_[5];
		uint32_t g = state_[6];
		uint32_t h = state_[7];

		for (uint32_t i = 0; i < 64; ++i) {
			const uint32_t S1 = rotr(e, 6) ^ rotr(e, 11) ^ rotr(e, 25);
			const uint32_t ch = (e & f) ^ ((~e) & g);
			const uint32_t temp1 = h + S1 + ch + k[i] + m[i];
			const uint32_t S0 = rotr(a, 2) ^ rotr(a, 13) ^ rotr(a, 22);
			const uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
			const uint32_t temp2 = S0 + maj;
			h = g;
			g = f;
			f = e;
			e = d + temp1;
			d = c;
			c = b;
			b = a;
			a = temp1 + temp2;
		}

		state_[0] += a;
		state_[1] += b;
		state_[2] += c;
		state_[3] += d;
		state_[4] += e;
		state_[5] += f;
		state_[6] += g;
		state_[7] += h;
	}

	uint8_t data_[64];
	uint32_t datalen_;
	uint64_t bitlen_;
	uint32_t state_[8];
};

std::string toHex(const uint8_t* data, size_t len) {
	static const char* digits = "0123456789abcdef";
	std::string out;
	out.resize(len * 2);
	for (size_t i = 0; i < len; ++i) {
		out[i * 2] = digits[(data[i] >> 4) & 0xf];
		out[i * 2 + 1] = digits[data[i] & 0xf];
	}
	return out;
}

bool jsonHas(const json::mObject& obj, const char* key) {
	return obj.find(key) != obj.end();
}

std::string jsonStr(const json::mObject& obj, const char* key, const std::string& fallback = std::string()) {
	json::mObject::const_iterator it = obj.find(key);
	if (it == obj.end() || it->second.type() != json::str_type) {
		return fallback;
	}
	return it->second.get_str();
}

long jsonLong(const json::mObject& obj, const char* key, long fallback = 0) {
	json::mObject::const_iterator it = obj.find(key);
	if (it == obj.end()) {
		return fallback;
	}
	if (it->second.type() == json::int_type) {
		return static_cast<long>(it->second.get_int());
	}
	if (it->second.type() == json::real_type) {
		return static_cast<long>(it->second.get_real());
	}
	return fallback;
}

bool parseJsonObject(const std::string& body, json::mObject& out, wxString& error) {
	std::istringstream stream(body);
	json::mValue root;
	if (!json::read(stream, root) || root.type() != json::obj_type) {
		error = "Invalid JSON response.";
		return false;
	}
	out = root.get_obj();
	return true;
}

wxString escapeJson(const wxString& input) {
	wxString out;
	out.reserve(input.length() + 8);
	for (size_t i = 0; i < input.length(); ++i) {
		const wxChar c = input[i];
		switch (c) {
			case wxT('"'):
				out += "\\\"";
				break;
			case wxT('\\'):
				out += "\\\\";
				break;
			case wxT('\n'):
				out += "\\n";
				break;
			case wxT('\r'):
				out += "\\r";
				break;
			case wxT('\t'):
				out += "\\t";
				break;
			default:
				out += c;
				break;
		}
	}
	return out;
}

} // namespace

wxString MarketplaceIdentity::Sha256Hex(const std::string& data) {
	Sha256 hasher;
	hasher.update(reinterpret_cast<const uint8_t*>(data.data()), data.size());
	uint8_t hash[32];
	hasher.final(hash);
	return wxstr(toHex(hash, 32));
}

wxString MarketplaceIdentity::SessionFilePath() {
	// Use standard paths only — safe during early startup (no g_gui dependency).
	const wxString dir = wxStandardPaths::Get().GetUserDataDir();
	wxFileName path(dir, "marketplace_session.json");
	return path.GetFullPath();
}

wxString MarketplaceIdentity::ComputeHwidHash() {
	std::ostringstream material;

#ifdef __WINDOWS__
	HKEY key = nullptr;
	if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Cryptography", 0, KEY_READ | KEY_WOW64_64KEY, &key) == ERROR_SUCCESS) {
		char guid[256] = {};
		DWORD size = sizeof(guid);
		DWORD type = 0;
		if (RegQueryValueExA(key, "MachineGuid", nullptr, &type, reinterpret_cast<LPBYTE>(guid), &size) == ERROR_SUCCESS) {
			material << "mg:" << guid << ";";
		}
		RegCloseKey(key);
	}

	char volumeName[MAX_PATH + 1] = {};
	char fsName[MAX_PATH + 1] = {};
	DWORD serial = 0;
	DWORD maxComponent = 0;
	DWORD flags = 0;
	if (GetVolumeInformationA("C:\\", volumeName, MAX_PATH, &serial, &maxComponent, &flags, fsName, MAX_PATH)) {
		material << "vs:" << std::hex << serial << ";";
	}
#else
	material << "host:" << wxGetHostName().mb_str() << ";";
	material << "user:" << wxGetUserId().mb_str() << ";";
#endif

	material << "app:rmevip-marketplace;";
	return Sha256Hex(material.str());
}

bool MarketplaceIdentity::LoadSession(wxString& nickname, wxString& sessionKey, wxString& hwidHash, wxString& userId) {
	const wxString path = SessionFilePath();
	if (!wxFileExists(path)) {
		return false;
	}

	wxFile file(path, wxFile::read);
	if (!file.IsOpened()) {
		return false;
	}

	wxString content;
	file.ReadAll(&content);
	file.Close();

	json::mObject obj;
	wxString error;
	if (!parseJsonObject(std::string(content.mb_str()), obj, error)) {
		return false;
	}

	nickname = wxstr(jsonStr(obj, "nickname"));
	sessionKey = wxstr(jsonStr(obj, "sessionKey"));
	hwidHash = wxstr(jsonStr(obj, "hwidHash"));
	userId = wxstr(jsonStr(obj, "userId"));
	return !sessionKey.IsEmpty() && !hwidHash.IsEmpty();
}

bool MarketplaceIdentity::SaveSession(const wxString& nickname, const wxString& sessionKey, const wxString& hwidHash, const wxString& userId) {
	const wxString path = SessionFilePath();
	wxFileName fn(path);
	if (!fn.Mkdir(fn.GetPath(), wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL) && !wxDirExists(fn.GetPath())) {
		return false;
	}

	const wxString jsonText = wxString::Format(
		"{\n"
		"  \"nickname\": \"%s\",\n"
		"  \"sessionKey\": \"%s\",\n"
		"  \"hwidHash\": \"%s\",\n"
		"  \"userId\": \"%s\"\n"
		"}\n",
		escapeJson(nickname),
		escapeJson(sessionKey),
		escapeJson(hwidHash),
		escapeJson(userId)
	);

	wxFile file(path, wxFile::write);
	if (!file.IsOpened()) {
		return false;
	}
	file.Write(jsonText);
	file.Close();
	return true;
}

MarketplaceClient::MarketplaceClient() :
	baseUrl("http://127.0.0.1:8787") {
	// Do not touch g_settings here — this object may be constructed during
	// static init (e.g. g_marketplacePresence) before Settings is ready.
}

void MarketplaceClient::SetBaseUrl(const wxString& url) {
	baseUrl = url;
	if (baseUrl.EndsWith("/")) {
		baseUrl.RemoveLast();
	}
}

MarketplaceClient::ParsedUrl MarketplaceClient::ParseBaseUrl() const {
	ParsedUrl parsed;
	wxString url = baseUrl;
	if (url.StartsWith("http://")) {
		url = url.Mid(7);
	} else if (url.StartsWith("https://")) {
		// Raw sockets: treat as http host:port (TLS not supported in v1 client).
		url = url.Mid(8);
	}

	wxString hostPort = url;
	wxString path;
	const int slash = url.Find('/');
	if (slash != wxNOT_FOUND) {
		hostPort = url.Left(slash);
		path = url.Mid(slash);
	}

	parsed.port = 80;
	const int colon = hostPort.Find(':');
	if (colon != wxNOT_FOUND) {
		parsed.host = hostPort.Left(colon);
		long port = 80;
		hostPort.Mid(colon + 1).ToLong(&port);
		parsed.port = static_cast<unsigned short>(port);
	} else {
		parsed.host = hostPort;
	}

	parsed.pathPrefix = path;
	if (parsed.pathPrefix.EndsWith("/")) {
		parsed.pathPrefix.RemoveLast();
	}
	parsed.ok = !parsed.host.IsEmpty();
	return parsed;
}

bool MarketplaceClient::SplitHttpResponse(const wxString& raw, int& statusCode, std::string& body) {
	const int sep = raw.Find("\r\n\r\n");
	if (sep == wxNOT_FOUND) {
		return false;
	}
	const wxString header = raw.Left(sep);
	const wxString bodyWx = raw.Mid(sep + 4);
	body.assign(bodyWx.mb_str(wxConvUTF8), bodyWx.Len());

	// HTTP/1.1 200 OK
	statusCode = 0;
	const int sp1 = header.Find(' ');
	if (sp1 == wxNOT_FOUND) {
		return false;
	}
	const wxString rest = header.Mid(sp1 + 1);
	long code = 0;
	if (!rest.BeforeFirst(' ').ToLong(&code)) {
		return false;
	}
	statusCode = static_cast<int>(code);
	return true;
}

bool MarketplaceClient::HttpRequest(const wxString& method, const wxString& pathAndQuery, const wxString& contentType, const std::string& body, const wxString& bearer, int& statusCode, std::string& responseBody, wxString& error, int timeoutSeconds) {
	const ParsedUrl parsed = ParseBaseUrl();
	if (!parsed.ok) {
		error = "Invalid marketplace URL.";
		return false;
	}

	wxSocketClient socket;
	socket.SetTimeout(timeoutSeconds);
	socket.SetFlags(wxSOCKET_WAITALL);

	wxIPV4address addr;
	addr.Hostname(parsed.host);
	addr.Service(parsed.port);
	if (!socket.Connect(addr, true)) {
		error = wxString::Format("Could not connect to %s:%d", parsed.host, parsed.port);
		return false;
	}

	wxString fullPath = parsed.pathPrefix + pathAndQuery;
	if (!fullPath.StartsWith("/")) {
		fullPath.Prepend("/");
	}

	wxString request;
	request.Printf(
		"%s %s HTTP/1.1\r\n"
		"Host: %s:%d\r\n"
		"Connection: close\r\n"
		"Accept: */*\r\n",
		method,
		fullPath,
		parsed.host,
		parsed.port
	);

	if (!bearer.IsEmpty()) {
		request += "Authorization: Bearer " + bearer + "\r\n";
	}
	if (!contentType.IsEmpty()) {
		request += "Content-Type: " + contentType + "\r\n";
	}
	if (!body.empty() || method == "POST" || method == "PUT") {
		request += wxString::Format("Content-Length: %zu\r\n", body.size());
	}
	request += "\r\n";

	std::string requestBytes(request.mb_str(wxConvUTF8));
	requestBytes.append(body);

	socket.Write(requestBytes.data(), requestBytes.size());
	if (socket.Error()) {
		error = "Failed to send HTTP request.";
		socket.Close();
		return false;
	}

	wxString raw;
	char buffer[8192];
	while (socket.IsConnected() || socket.IsData()) {
		socket.Read(buffer, sizeof(buffer));
		const size_t n = socket.LastCount();
		if (n == 0) {
			break;
		}
		raw.Append(wxString(buffer, wxConvISO8859_1, n));
	}
	socket.Close();

	if (!SplitHttpResponse(raw, statusCode, responseBody)) {
		error = "Malformed HTTP response.";
		return false;
	}
	return true;
}

bool MarketplaceClient::Health(long& usersOnline, wxString& error) {
	int status = 0;
	std::string body;
	if (!HttpRequest("GET", "/v1/health", "", "", "", status, body, error, 5)) {
		return false;
	}
	if (status != 200) {
		error = wxString::Format("Health check failed (%d).", status);
		return false;
	}
	json::mObject obj;
	if (!parseJsonObject(body, obj, error)) {
		return false;
	}
	usersOnline = jsonLong(obj, "usersOnline", 0);
	return true;
}

bool MarketplaceClient::Register(const wxString& nickname, const wxString& hwidHash, MarketplaceSessionInfo& out, wxString& error) {
	const std::string payload = std::string(
		wxString::Format(
			"{\"nickname\":\"%s\",\"hwidHash\":\"%s\"}",
			escapeJson(nickname),
			escapeJson(hwidHash)
		).mb_str()
	);

	int status = 0;
	std::string body;
	if (!HttpRequest("POST", "/v1/auth/register", "application/json", payload, "", status, body, error)) {
		return false;
	}
	if (status != 200) {
		json::mObject obj;
		if (parseJsonObject(body, obj, error)) {
			error = wxstr(jsonStr(obj, "error", "register failed"));
		} else {
			error = wxString::Format("Register failed (%d).", status);
		}
		return false;
	}

	json::mObject obj;
	if (!parseJsonObject(body, obj, error)) {
		return false;
	}
	out.sessionKey = wxstr(jsonStr(obj, "sessionKey"));
	out.userId = wxstr(jsonStr(obj, "userId"));
	out.trust = jsonLong(obj, "trust", 0);
	if (out.sessionKey.IsEmpty()) {
		error = "Register response missing sessionKey.";
		return false;
	}
	return true;
}

bool MarketplaceClient::Heartbeat(const wxString& sessionKey, long& usersOnline, long& trust, wxString& error) {
	int status = 0;
	std::string body;
	if (!HttpRequest("POST", "/v1/auth/heartbeat", "application/json", "{}", sessionKey, status, body, error, 5)) {
		return false;
	}
	if (status != 200) {
		error = wxString::Format("Heartbeat failed (%d).", status);
		return false;
	}
	json::mObject obj;
	if (!parseJsonObject(body, obj, error)) {
		return false;
	}
	usersOnline = jsonLong(obj, "usersOnline", 0);
	trust = jsonLong(obj, "trust", 0);
	return true;
}

bool MarketplaceClient::GetCategories(std::vector<MarketplaceCategory>& out, wxString& error) {
	int status = 0;
	std::string body;
	if (!HttpRequest("GET", "/v1/categories", "", "", "", status, body, error)) {
		return false;
	}
	if (status != 200) {
		error = wxString::Format("Categories failed (%d).", status);
		return false;
	}

	std::istringstream stream(body);
	json::mValue root;
	if (!json::read(stream, root) || root.type() != json::array_type) {
		error = "Invalid categories response.";
		return false;
	}

	out.clear();
	const json::mArray& arr = root.get_array();
	for (json::mArray::const_iterator it = arr.begin(); it != arr.end(); ++it) {
		if (it->type() != json::obj_type) {
			continue;
		}
		const json::mObject& obj = it->get_obj();
		MarketplaceCategory cat;
		cat.id = wxstr(jsonStr(obj, "id"));
		cat.name = wxstr(jsonStr(obj, "name"));
		if (!cat.id.IsEmpty()) {
			out.push_back(cat);
		}
	}
	return true;
}

bool MarketplaceClient::GetVersions(std::vector<MarketplaceVersion>& out, wxString& error) {
	int status = 0;
	std::string body;
	if (!HttpRequest("GET", "/v1/versions", "", "", "", status, body, error)) {
		return false;
	}
	if (status != 200) {
		error = wxString::Format("Versions failed (%d).", status);
		return false;
	}

	std::istringstream stream(body);
	json::mValue root;
	if (!json::read(stream, root) || root.type() != json::array_type) {
		error = "Invalid versions response.";
		return false;
	}

	out.clear();
	const json::mArray& arr = root.get_array();
	for (json::mArray::const_iterator it = arr.begin(); it != arr.end(); ++it) {
		if (it->type() != json::obj_type) {
			continue;
		}
		const json::mObject& obj = it->get_obj();
		MarketplaceVersion ver;
		ver.clientVersion = wxstr(jsonStr(obj, "clientVersion"));
		if (ver.clientVersion.IsEmpty()) {
			ver.clientVersion = wxstr(jsonStr(obj, "client_version"));
		}
		ver.clientVersionId = jsonLong(obj, "clientVersionId", jsonLong(obj, "client_version_id", 0));
		ver.count = jsonLong(obj, "count", 0);
		if (!ver.clientVersion.IsEmpty()) {
			out.push_back(ver);
		}
	}
	return true;
}

bool MarketplaceClient::GetListings(const MarketplaceListFilter& filter, std::vector<MarketplaceListing>& out, wxString& error) {
	wxString path = "/v1/listings?page=" + wxString::Format("%d", filter.page > 0 ? filter.page : 1);
	if (!filter.category.IsEmpty()) {
		path += "&category=" + filter.category;
	}
	if (filter.clientVersionId > 0) {
		path += "&clientVersionId=" + wxString::Format("%ld", filter.clientVersionId);
	} else if (!filter.clientVersion.IsEmpty()) {
		path += "&clientVersion=" + filter.clientVersion;
	}
	if (!filter.query.IsEmpty()) {
		path += "&q=" + filter.query;
	}
	if (!filter.sort.IsEmpty()) {
		path += "&sort=" + filter.sort;
	}

	int status = 0;
	std::string body;
	if (!HttpRequest("GET", path, "", "", "", status, body, error)) {
		return false;
	}
	if (status != 200) {
		error = wxString::Format("Listings failed (%d).", status);
		return false;
	}

	std::istringstream stream(body);
	json::mValue root;
	if (!json::read(stream, root) || root.type() != json::array_type) {
		error = "Invalid listings response.";
		return false;
	}

	out.clear();
	const json::mArray& arr = root.get_array();
	for (json::mArray::const_iterator it = arr.begin(); it != arr.end(); ++it) {
		if (it->type() != json::obj_type) {
			continue;
		}
		const json::mObject& obj = it->get_obj();
		MarketplaceListing listing;
		listing.id = wxstr(jsonStr(obj, "id"));
		listing.title = wxstr(jsonStr(obj, "title"));
		listing.category = wxstr(jsonStr(obj, "category"));
		listing.previewUrl = wxstr(jsonStr(obj, "preview_url"));
		if (listing.previewUrl.IsEmpty()) {
			listing.previewUrl = wxstr(jsonStr(obj, "previewUrl"));
		}
		listing.uploaderNick = wxstr(jsonStr(obj, "uploader_nick"));
		if (listing.uploaderNick.IsEmpty()) {
			listing.uploaderNick = wxstr(jsonStr(obj, "uploaderNick"));
		}
		listing.clientVersion = wxstr(jsonStr(obj, "clientVersion"));
		if (listing.clientVersion.IsEmpty()) {
			listing.clientVersion = wxstr(jsonStr(obj, "client_version"));
		}
		listing.clientVersionId = jsonLong(obj, "clientVersionId", jsonLong(obj, "client_version_id", 0));
		listing.downloads = jsonLong(obj, "downloads", 0);
		listing.createdAt = jsonLong(obj, "created_at", jsonLong(obj, "createdAt", 0));
		listing.sizeBytes = jsonLong(obj, "size_bytes", jsonLong(obj, "sizeBytes", 0));
		if (!listing.id.IsEmpty()) {
			out.push_back(listing);
		}
	}
	return true;
}

bool MarketplaceClient::DownloadPreview(const wxString& listingId, std::vector<uint8_t>& outPng, wxString& error) {
	int status = 0;
	std::string body;
	if (!HttpRequest("GET", "/v1/listings/" + listingId + "/preview", "", "", "", status, body, error)) {
		return false;
	}
	if (status != 200) {
		error = wxString::Format("Preview download failed (%d).", status);
		return false;
	}
	outPng.assign(body.begin(), body.end());
	return !outPng.empty();
}

bool MarketplaceClient::DownloadPatch(const wxString& listingId, const wxString& sessionKey, std::string& outJson, wxString& error) {
	int status = 0;
	std::string body;
	if (!HttpRequest("GET", "/v1/listings/" + listingId + "/download", "", "", sessionKey, status, body, error, 30)) {
		return false;
	}
	if (status != 200) {
		error = wxString::Format("Download failed (%d).", status);
		return false;
	}
	outJson.swap(body);
	return !outJson.empty();
}

bool MarketplaceClient::UploadListing(const wxString& sessionKey, const wxString& title, const wxString& category, const wxString& clientVersion, long clientVersionId, const std::string& patchJson, const std::vector<uint8_t>& previewPng, const wxString& contentSha256, wxString& outListingId, wxString& error) {
	const wxString boundary = wxString::Format("----rmeBoundary%08x", static_cast<unsigned>(wxGetLocalTime()));
	const std::string bound = std::string(boundary.mb_str());

	const std::string meta = std::string(
		wxString::Format(
			"{\"title\":\"%s\",\"category\":\"%s\",\"contentSha256\":\"%s\",\"clientVersion\":\"%s\",\"clientVersionId\":%ld}",
			escapeJson(title),
			escapeJson(category),
			escapeJson(contentSha256),
			escapeJson(clientVersion),
			clientVersionId
		).mb_str()
	);

	std::string body;
	body.reserve(meta.size() + patchJson.size() + previewPng.size() + 512);

	auto appendField = [&](const char* name, const char* filename, const char* type, const char* data, size_t len) {
		body += "--" + bound + "\r\n";
		body += "Content-Disposition: form-data; name=\"";
		body += name;
		body += "\"";
		if (filename) {
			body += "; filename=\"";
			body += filename;
			body += "\"";
		}
		body += "\r\n";
		if (type) {
			body += "Content-Type: ";
			body += type;
			body += "\r\n";
		}
		body += "\r\n";
		body.append(data, len);
		body += "\r\n";
	};

	appendField("meta", nullptr, "application/json", meta.data(), meta.size());
	appendField("preview", "preview.png", "image/png", reinterpret_cast<const char*>(previewPng.data()), previewPng.size());
	appendField("patch", "patch.json", "application/json", patchJson.data(), patchJson.size());
	body += "--" + bound + "--\r\n";

	if (body.size() > 2 * 1024 * 1024) {
		error = "Upload exceeds 2 MB limit.";
		return false;
	}

	int status = 0;
	std::string response;
	const wxString contentType = "multipart/form-data; boundary=" + boundary;
	if (!HttpRequest("POST", "/v1/listings", contentType, body, sessionKey, status, response, error, 60)) {
		return false;
	}
	if (status != 200) {
		json::mObject obj;
		if (parseJsonObject(response, obj, error)) {
			error = wxstr(jsonStr(obj, "error", "upload failed"));
		} else {
			error = wxString::Format("Upload failed (%d).", status);
		}
		return false;
	}

	json::mObject obj;
	if (!parseJsonObject(response, obj, error)) {
		return false;
	}
	outListingId = wxstr(jsonStr(obj, "id"));
	return !outListingId.IsEmpty();
}
