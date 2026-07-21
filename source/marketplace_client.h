//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#ifndef RME_MARKETPLACE_CLIENT_H_
#define RME_MARKETPLACE_CLIENT_H_

#include "main.h"

#include <vector>
#include <string>

struct MarketplaceCategory {
	wxString id;
	wxString name;
};

struct MarketplaceVersion {
	wxString clientVersion;
	long clientVersionId = 0;
	long count = 0;
};

struct MarketplaceListing {
	wxString id;
	wxString title;
	wxString category;
	wxString previewUrl;
	wxString uploaderNick;
	wxString clientVersion;
	long clientVersionId = 0;
	long downloads = 0;
	long createdAt = 0;
	long sizeBytes = 0;
};

struct MarketplaceSessionInfo {
	wxString sessionKey;
	wxString userId;
	long trust = 0;
};

struct MarketplaceListFilter {
	wxString category;
	wxString clientVersion;
	long clientVersionId = 0; // >0 preferred over name
	wxString query;
	wxString sort = "new";
	int page = 1;
};

class MarketplaceClient {
public:
	MarketplaceClient();

	void SetBaseUrl(const wxString& url);
	wxString GetBaseUrl() const {
		return baseUrl;
	}

	bool Health(long& usersOnline, wxString& error);
	bool Register(const wxString& nickname, const wxString& hwidHash, MarketplaceSessionInfo& out, wxString& error);
	bool Heartbeat(const wxString& sessionKey, long& usersOnline, long& trust, wxString& error);
	bool GetCategories(std::vector<MarketplaceCategory>& out, wxString& error);
	bool GetVersions(std::vector<MarketplaceVersion>& out, wxString& error);
	bool GetListings(const MarketplaceListFilter& filter, std::vector<MarketplaceListing>& out, wxString& error);
	bool DownloadPreview(const wxString& listingId, std::vector<uint8_t>& outPng, wxString& error);
	bool DownloadPatch(const wxString& listingId, const wxString& sessionKey, std::string& outJson, wxString& error);
	bool UploadListing(const wxString& sessionKey, const wxString& title, const wxString& category, const wxString& clientVersion, long clientVersionId, const std::string& patchJson, const std::vector<uint8_t>& previewPng, const wxString& contentSha256, wxString& outListingId, wxString& error);

private:
	struct ParsedUrl {
		wxString host;
		unsigned short port = 80;
		wxString pathPrefix; // e.g. "" or "/prefix"
		bool ok = false;
	};

	ParsedUrl ParseBaseUrl() const;
	bool HttpRequest(const wxString& method, const wxString& pathAndQuery, const wxString& contentType, const std::string& body, const wxString& bearer, int& statusCode, std::string& responseBody, wxString& error, int timeoutSeconds = 15);
	static bool SplitHttpResponse(const wxString& raw, int& statusCode, std::string& body);

	wxString baseUrl;
};

// AppData session persistence + HWID hash
namespace MarketplaceIdentity {
	wxString SessionFilePath();
	wxString ComputeHwidHash();
	bool LoadSession(wxString& nickname, wxString& sessionKey, wxString& hwidHash, wxString& userId);
	bool SaveSession(const wxString& nickname, const wxString& sessionKey, const wxString& hwidHash, const wxString& userId);
	wxString Sha256Hex(const std::string& data);
}

#endif
