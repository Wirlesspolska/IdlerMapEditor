//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#include "main.h"

#include "marketplace_window.h"
#include "gui.h"
#include "editor.h"
#include "map_json_export.h"
#include "settings.h"
#include "marketplace_client.h"
#include "client_version.h"

#include <wx/sizer.h>
#include <wx/msgdlg.h>
#include <wx/statbmp.h>
#include <wx/mstream.h>
#include <wx/image.h>

enum {
	ID_MP_CATEGORY = wxID_HIGHEST + 4200,
	ID_MP_REFRESH,
	ID_MP_UPLOAD,
	ID_MP_NICKNAME,
	ID_MP_SEARCH,
	ID_MP_SORT,
	ID_MP_VERSION,
	ID_MP_MATCH_CURRENT,
	ID_MP_CARD_BASE = wxID_HIGHEST + 4300,
	ID_MP_PRESENCE_TIMER
};

MarketplacePresenceService g_marketplacePresence;

MarketplacePresenceService::MarketplacePresenceService() :
	timer(nullptr),
	hostWindow(nullptr),
	online(false),
	usersOnline(0),
	bound(false) {
}

MarketplacePresenceService::~MarketplacePresenceService() {
	Stop();
}

void MarketplacePresenceService::Start(wxWindow* host) {
	hostWindow = host;
	if (!hostWindow) {
		return;
	}
	if (!g_settings.getInteger(Config::MARKETPLACE_ENABLED)) {
		return;
	}

	const std::string url = g_settings.getString(Config::MARKETPLACE_URL);
	if (!url.empty()) {
		client.SetBaseUrl(wxstr(url));
	}

	wxString nick, key, hwid, uid;
	if (MarketplaceIdentity::LoadSession(nick, key, hwid, uid)) {
		sessionKey = key;
	}

	if (!timer) {
		timer = newd wxTimer(hostWindow, ID_MP_PRESENCE_TIMER);
		hostWindow->Bind(wxEVT_TIMER, &MarketplacePresenceService::OnTimer, this, ID_MP_PRESENCE_TIMER);
		bound = true;
		// Short first tick after the UI is up — never block OnInit with sockets.
		timer->StartOnce(2500);
	}
}

void MarketplacePresenceService::Stop() {
	if (timer) {
		timer->Stop();
		delete timer;
		timer = nullptr;
	}
	if (bound && hostWindow) {
		hostWindow->Unbind(wxEVT_TIMER, &MarketplacePresenceService::OnTimer, this, ID_MP_PRESENCE_TIMER);
		bound = false;
	}
	hostWindow = nullptr;
}

void MarketplacePresenceService::OnTimer(wxTimerEvent& WXUNUSED(event)) {
	Tick();
	if (timer && g_settings.getInteger(Config::MARKETPLACE_ENABLED)) {
		timer->Start(60000, wxTIMER_CONTINUOUS);
	}
}

void MarketplacePresenceService::Tick() {
	if (!g_settings.getInteger(Config::MARKETPLACE_ENABLED) || !g_gui.root || !hostWindow) {
		return;
	}

	const std::string url = g_settings.getString(Config::MARKETPLACE_URL);
	if (!url.empty()) {
		client.SetBaseUrl(wxstr(url));
	}

	wxString error;
	long count = 0;
	long trust = 0;
	bool ok = false;

	if (!sessionKey.IsEmpty()) {
		ok = client.Heartbeat(sessionKey, count, trust, error);
	}
	if (!ok) {
		ok = client.Health(count, error);
	}

	online = ok;
	usersOnline = ok ? count : 0;

	wxStatusBar* bar = g_gui.root->GetStatusBar();
	if (bar && bar->GetFieldsCount() >= 5) {
		if (online) {
			bar->SetStatusText(wxString::Format(wxT("[ON] Marketplace: %ld online"), usersOnline), 4);
		} else {
			bar->SetStatusText(wxT("[OFF] Marketplace: offline"), 4);
		}
	}
}

BEGIN_EVENT_TABLE(MarketplaceWindow, wxDialog)
EVT_LISTBOX(ID_MP_CATEGORY, MarketplaceWindow::OnCategorySelect)
EVT_CHOICE(ID_MP_VERSION, MarketplaceWindow::OnVersionFilter)
EVT_CHECKBOX(ID_MP_MATCH_CURRENT, MarketplaceWindow::OnVersionFilter)
EVT_BUTTON(ID_MP_REFRESH, MarketplaceWindow::OnRefresh)
EVT_BUTTON(ID_MP_UPLOAD, MarketplaceWindow::OnUpload)
EVT_BUTTON(ID_MP_NICKNAME, MarketplaceWindow::OnSetNickname)
EVT_CLOSE(MarketplaceWindow::OnClose)
END_EVENT_TABLE()

MarketplaceWindow::MarketplaceWindow(wxWindow* parent) :
	wxDialog(parent, wxID_ANY, "Share Marketplace", wxDefaultPosition, wxSize(960, 680), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER | wxSTAY_ON_TOP),
	categoryList(nullptr),
	cardsPanel(nullptr),
	cardsSizer(nullptr),
	statusLabel(nullptr),
	versionHintLabel(nullptr),
	searchCtrl(nullptr),
	sortChoice(nullptr),
	versionChoice(nullptr),
	matchCurrentVersionCheck(nullptr),
	refreshButton(nullptr),
	uploadButton(nullptr),
	nicknameButton(nullptr) {
	client.SetBaseUrl(wxstr(g_settings.getString(Config::MARKETPLACE_URL)));

	wxString nick, key, hwid, uid;
	if (MarketplaceIdentity::LoadSession(nick, key, hwid, uid)) {
		nickname = nick;
		sessionKey = key;
		hwidHash = hwid;
		userId = uid;
	} else {
		nickname = wxstr(g_settings.getString(Config::MARKETPLACE_NICKNAME));
		hwidHash = MarketplaceIdentity::ComputeHwidHash();
	}

	BuildUi();
	LoadCategories();
	LoadVersionFilter();
	RefreshListings();
}

MarketplaceWindow::~MarketplaceWindow() {
}

void MarketplaceWindow::BuildUi() {
	wxBoxSizer* root = newd wxBoxSizer(wxVERTICAL);

	wxBoxSizer* left = newd wxBoxSizer(wxVERTICAL);
	left->Add(newd wxStaticText(this, wxID_ANY, "Categories"), 0, wxALL, 6);
	categoryList = newd wxListBox(this, ID_MP_CATEGORY);
	left->Add(categoryList, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 6);
	nicknameButton = newd wxButton(this, ID_MP_NICKNAME, nickname.IsEmpty() ? "Set Nickname..." : ("Nick: " + nickname));
	left->Add(nicknameButton, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 6);
	root->Add(left, 0, wxEXPAND | wxALL, 4);

	wxBoxSizer* right = newd wxBoxSizer(wxVERTICAL);
	wxBoxSizer* toolbar = newd wxBoxSizer(wxHORIZONTAL);
	searchCtrl = newd wxTextCtrl(this, ID_MP_SEARCH, "", wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER);
	sortChoice = newd wxChoice(this, ID_MP_SORT);
	sortChoice->Append("new");
	sortChoice->Append("downloads");
	sortChoice->SetSelection(0);
	refreshButton = newd wxButton(this, ID_MP_REFRESH, "Refresh");
	uploadButton = newd wxButton(this, ID_MP_UPLOAD, "Upload Selection...");
	toolbar->Add(newd wxStaticText(this, wxID_ANY, "Search"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
	toolbar->Add(searchCtrl, 1, wxRIGHT, 8);
	toolbar->Add(sortChoice, 0, wxRIGHT, 8);
	toolbar->Add(refreshButton, 0, wxRIGHT, 8);
	toolbar->Add(uploadButton, 0);
	right->Add(toolbar, 0, wxEXPAND | wxALL, 6);

	wxBoxSizer* versionBar = newd wxBoxSizer(wxHORIZONTAL);
	versionBar->Add(newd wxStaticText(this, wxID_ANY, "VERSION"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
	versionChoice = newd wxChoice(this, ID_MP_VERSION);
	versionChoice->Append("All versions");
	versionChoice->SetSelection(0);
	versionBar->Add(versionChoice, 0, wxRIGHT, 8);
	matchCurrentVersionCheck = newd wxCheckBox(this, ID_MP_MATCH_CURRENT, "Match loaded VERSION only");
	matchCurrentVersionCheck->SetValue(true);
	versionBar->Add(matchCurrentVersionCheck, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
	const wxString currentVer = CurrentClientVersionName();
	versionHintLabel = newd wxStaticText(this, wxID_ANY, currentVer.IsEmpty() ? "No VERSION loaded" : ("Loaded: " + currentVer));
	versionBar->Add(versionHintLabel, 0, wxALIGN_CENTER_VERTICAL);
	versionChoice->Enable(false);
	right->Add(versionBar, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 6);

	statusLabel = newd wxStaticText(this, wxID_ANY, "Ready");
	right->Add(statusLabel, 0, wxLEFT | wxRIGHT | wxBOTTOM, 6);

	cardsPanel = newd wxScrolledWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxVSCROLL);
	cardsPanel->SetScrollRate(0, 16);
	cardsSizer = newd wxBoxSizer(wxVERTICAL);
	cardsPanel->SetSizer(cardsSizer);
	right->Add(cardsPanel, 1, wxEXPAND | wxALL, 6);

	root->Add(right, 1, wxEXPAND | wxALL, 4);
	SetSizer(root);

	searchCtrl->Bind(wxEVT_TEXT_ENTER, &MarketplaceWindow::OnRefresh, this);
}

void MarketplaceWindow::LoadCategories() {
	categoryList->Clear();
	categoryList->Append("All");

	std::vector<MarketplaceCategory> cats;
	wxString error;
	if (client.GetCategories(cats, error)) {
		for (const MarketplaceCategory& cat : cats) {
			categoryList->Append(cat.name + " [" + cat.id + "]");
		}
	} else {
		// Offline fallback matching server defaults
		categoryList->Append("Buildings [buildings]");
		categoryList->Append("Shops [shops]");
		categoryList->Append("Dungeons [dungeons]");
		categoryList->Append("Decor [decor]");
		categoryList->Append("Roads [roads]");
		categoryList->Append("Other [other]");
		statusLabel->SetLabel("Categories offline — " + error);
	}
	categoryList->SetSelection(0);
}

void MarketplaceWindow::LoadVersionFilter() {
	if (!versionChoice) {
		return;
	}

	const int prevSel = versionChoice->GetSelection();
	wxString prevLabel = prevSel >= 0 ? versionChoice->GetString(prevSel) : wxString();

	versionChoice->Clear();
	versionChoice->Append("All versions");
	knownVersions.clear();

	wxString error;
	if (client.GetVersions(knownVersions, error)) {
		for (const MarketplaceVersion& ver : knownVersions) {
			versionChoice->Append(wxString::Format("%s  (%ld)", ver.clientVersion, ver.count));
		}
	}

	const wxString current = CurrentClientVersionName();
	if (versionHintLabel) {
		versionHintLabel->SetLabel(current.IsEmpty() ? "No VERSION loaded" : ("Loaded: " + current));
	}

	int restore = 0;
	if (!prevLabel.IsEmpty()) {
		const int found = versionChoice->FindString(prevLabel);
		if (found != wxNOT_FOUND) {
			restore = found;
		}
	}
	versionChoice->SetSelection(restore);
}

wxString MarketplaceWindow::SelectedCategoryId() const {
	const int sel = categoryList->GetSelection();
	if (sel <= 0) {
		return wxEmptyString;
	}
	const wxString label = categoryList->GetString(sel);
	const int open = label.Find('[');
	const int close = label.Find(']');
	if (open != wxNOT_FOUND && close > open) {
		return label.Mid(open + 1, close - open - 1);
	}
	return wxEmptyString;
}

wxString MarketplaceWindow::CurrentClientVersionName() const {
	if (!g_gui.IsVersionLoaded()) {
		return wxEmptyString;
	}
	return wxstr(g_gui.GetCurrentVersion().getName());
}

long MarketplaceWindow::CurrentClientVersionId() const {
	if (!g_gui.IsVersionLoaded()) {
		return 0;
	}
	return static_cast<long>(g_gui.GetCurrentVersionID());
}

void MarketplaceWindow::ApplyVersionFilterToQuery(MarketplaceListFilter& filter) const {
	if (matchCurrentVersionCheck && matchCurrentVersionCheck->GetValue()) {
		filter.clientVersionId = CurrentClientVersionId();
		filter.clientVersion = CurrentClientVersionName();
		return;
	}

	if (!versionChoice) {
		return;
	}
	const int sel = versionChoice->GetSelection();
	if (sel <= 0) {
		return;
	}
	// Index 0 = All; knownVersions starts at choice index 1
	const size_t idx = static_cast<size_t>(sel - 1);
	if (idx < knownVersions.size()) {
		filter.clientVersion = knownVersions[idx].clientVersion;
		filter.clientVersionId = knownVersions[idx].clientVersionId;
	}
}

void MarketplaceWindow::ClearCards() {
	if (!cardsSizer) {
		return;
	}
	cardsSizer->Clear(true);
	buttonToListingId.clear();
	cardsPanel->Layout();
	cardsPanel->FitInside();
}

void MarketplaceWindow::PopulateCards(const std::vector<MarketplaceListing>& listings) {
	ClearCards();
	currentListings = listings;

	wxGridSizer* grid = newd wxGridSizer(0, 3, 10, 10);
	int buttonId = ID_MP_CARD_BASE;

	for (const MarketplaceListing& listing : listings) {
		wxPanel* card = newd wxPanel(cardsPanel, wxID_ANY, wxDefaultPosition, wxSize(240, 220));
		wxBoxSizer* cardSizer = newd wxBoxSizer(wxVERTICAL);

		wxStaticBitmap* preview = newd wxStaticBitmap(card, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize(160, 160));
		std::vector<uint8_t> png;
		wxString error;
		if (client.DownloadPreview(listing.id, png, error) && !png.empty()) {
			wxMemoryInputStream stream(png.data(), png.size());
			wxImage image;
			if (image.LoadFile(stream, wxBITMAP_TYPE_PNG)) {
				image.Rescale(160, 160, wxIMAGE_QUALITY_NEAREST);
				preview->SetBitmap(wxBitmap(image));
			}
		}

		cardSizer->Add(preview, 0, wxALIGN_CENTER | wxTOP, 6);
		cardSizer->Add(newd wxStaticText(card, wxID_ANY, listing.title), 0, wxALL | wxALIGN_CENTER, 2);
		const wxString verLabel = listing.clientVersion.IsEmpty() ? "?" : listing.clientVersion;
		cardSizer->Add(newd wxStaticText(card, wxID_ANY, wxString::Format("%s  ·  %s  ·  %ld dl", listing.uploaderNick, verLabel, listing.downloads)), 0, wxLEFT | wxRIGHT | wxALIGN_CENTER, 2);
		cardSizer->Add(newd wxStaticText(card, wxID_ANY, listing.category), 0, wxLEFT | wxRIGHT | wxALIGN_CENTER, 2);

		wxButton* useButton = newd wxButton(card, buttonId, "Use as Brush");
		buttonToListingId[buttonId] = listing.id;
		useButton->Bind(wxEVT_BUTTON, &MarketplaceWindow::OnCardClick, this);
		cardSizer->Add(useButton, 0, wxALL | wxALIGN_CENTER, 6);

		card->SetSizer(cardSizer);
		grid->Add(card, 0, wxEXPAND);
		++buttonId;
	}

	cardsSizer->Add(grid, 1, wxEXPAND | wxALL, 4);
	cardsPanel->Layout();
	cardsPanel->FitInside();
}

void MarketplaceWindow::RefreshListings() {
	statusLabel->SetLabel("Loading listings...");
	statusLabel->Refresh();
	LoadVersionFilter();

	MarketplaceListFilter filter;
	filter.category = SelectedCategoryId();
	filter.query = searchCtrl ? searchCtrl->GetValue() : wxString();
	filter.sort = sortChoice ? sortChoice->GetStringSelection() : "new";
	filter.page = 1;
	ApplyVersionFilterToQuery(filter);

	if (matchCurrentVersionCheck && matchCurrentVersionCheck->GetValue() && filter.clientVersion.IsEmpty() && filter.clientVersionId <= 0) {
		statusLabel->SetLabel("Load a client VERSION first (or uncheck Match loaded VERSION).");
		ClearCards();
		return;
	}

	std::vector<MarketplaceListing> listings;
	wxString error;
	if (!client.GetListings(filter, listings, error)) {
		statusLabel->SetLabel("Failed: " + error);
		ClearCards();
		return;
	}

	PopulateCards(listings);
	wxString filterNote;
	if (filter.clientVersionId > 0 || !filter.clientVersion.IsEmpty()) {
		filterNote = wxString::Format(" · VERSION %s", filter.clientVersion.IsEmpty() ? wxString::Format("#%ld", filter.clientVersionId) : filter.clientVersion);
	}
	statusLabel->SetLabel(wxString::Format("%zu listings%s", listings.size(), filterNote));
}

bool MarketplaceWindow::EnsureSession(wxString& error) {
	if (!sessionKey.IsEmpty() && !hwidHash.IsEmpty()) {
		return true;
	}

	if (nickname.IsEmpty()) {
		wxTextEntryDialog dlg(this, "Choose a marketplace nickname (bound to this machine):", "Marketplace Nickname", nickname);
		if (dlg.ShowModal() != wxID_OK) {
			error = "Nickname required.";
			return false;
		}
		nickname = dlg.GetValue().Trim(true).Trim(false);
	}

	if (nickname.IsEmpty()) {
		error = "Nickname required.";
		return false;
	}

	hwidHash = MarketplaceIdentity::ComputeHwidHash();
	MarketplaceSessionInfo info;
	if (!client.Register(nickname, hwidHash, info, error)) {
		return false;
	}

	sessionKey = info.sessionKey;
	userId = info.userId;
	g_settings.setString(Config::MARKETPLACE_NICKNAME, nstr(nickname));
	MarketplaceIdentity::SaveSession(nickname, sessionKey, hwidHash, userId);
	nicknameButton->SetLabel("Nick: " + nickname);
	return true;
}

void MarketplaceWindow::EnsureSessionInteractive() {
	wxString error;
	if (!EnsureSession(error)) {
		wxMessageBox(error, "Marketplace", wxOK | wxICON_WARNING, this);
	}
}

void MarketplaceWindow::OnCategorySelect(wxCommandEvent& WXUNUSED(event)) {
	RefreshListings();
}

void MarketplaceWindow::OnVersionFilter(wxCommandEvent& WXUNUSED(event)) {
	if (matchCurrentVersionCheck && versionChoice) {
		versionChoice->Enable(!matchCurrentVersionCheck->GetValue());
	}
	RefreshListings();
}

void MarketplaceWindow::OnRefresh(wxCommandEvent& WXUNUSED(event)) {
	client.SetBaseUrl(wxstr(g_settings.getString(Config::MARKETPLACE_URL)));
	RefreshListings();
}

void MarketplaceWindow::OnSetNickname(wxCommandEvent& WXUNUSED(event)) {
	wxTextEntryDialog dlg(this, "Marketplace nickname:", "Nickname", nickname);
	if (dlg.ShowModal() != wxID_OK) {
		return;
	}
	nickname = dlg.GetValue().Trim(true).Trim(false);
	sessionKey.clear();
	userId.clear();
	wxString error;
	if (!EnsureSession(error)) {
		wxMessageBox(error, "Marketplace", wxOK | wxICON_ERROR, this);
		return;
	}
	nicknameButton->SetLabel("Nick: " + nickname);
}

void MarketplaceWindow::OnCardClick(wxCommandEvent& event) {
	const int id = event.GetId();
	std::map<int, wxString>::const_iterator it = buttonToListingId.find(id);
	if (it == buttonToListingId.end()) {
		return;
	}

	Editor* editor = g_gui.GetCurrentEditor();
	if (!editor) {
		wxMessageBox("Open a map first.", "Marketplace", wxOK | wxICON_INFORMATION, this);
		return;
	}

	wxButton* button = dynamic_cast<wxButton*>(event.GetEventObject());
	if (button) {
		button->SetLabel("Downloading...");
		button->Enable(false);
	}
	statusLabel->SetLabel("Downloading...");
	statusLabel->Refresh();
	wxYield();

	wxString error;
	EnsureSession(error); // optional for download; helps rate limits

	const MarketplaceListing* listingMeta = nullptr;
	for (const MarketplaceListing& listing : currentListings) {
		if (listing.id == it->second) {
			listingMeta = &listing;
			break;
		}
	}

	const wxString currentVer = CurrentClientVersionName();
	const long currentId = CurrentClientVersionId();
	if (listingMeta && !listingMeta->clientVersion.IsEmpty()) {
		const bool idMismatch = listingMeta->clientVersionId > 0 && currentId > 0 && listingMeta->clientVersionId != currentId;
		const bool nameMismatch = !currentVer.IsEmpty() && listingMeta->clientVersion != currentVer;
		if (idMismatch || nameMismatch) {
			const int answer = wxMessageBox(
				wxString::Format(
					"This patch was uploaded for VERSION %s (id %ld).\n"
					"You have %s (id %ld) loaded.\n\n"
					"Item IDs / spr-dat may not match. Use anyway?",
					listingMeta->clientVersion,
					listingMeta->clientVersionId,
					currentVer.IsEmpty() ? wxString("none") : currentVer,
					currentId
				),
				"VERSION mismatch",
				wxYES_NO | wxICON_WARNING,
				this
			);
			if (answer != wxYES) {
				if (button) {
					button->SetLabel("Use as Brush");
					button->Enable(true);
				}
				statusLabel->SetLabel("Download cancelled (VERSION mismatch)");
				return;
			}
		}
	}

	std::string jsonText;
	if (!client.DownloadPatch(it->second, sessionKey, jsonText, error)) {
		statusLabel->SetLabel("Download failed: " + error);
		if (button) {
			button->SetLabel("Use as Brush");
			button->Enable(true);
		}
		return;
	}

	if (!MapJsonImporter::importToCopyBuffer(g_gui.copybuffer, jsonText, error)) {
		statusLabel->SetLabel("Import failed: " + error);
		if (button) {
			button->SetLabel("Use as Brush");
			button->Enable(true);
		}
		return;
	}

	g_gui.PreparePaste();
	statusLabel->SetLabel("Marketplace patch ready — click map to place");
	g_gui.SetStatusText("Marketplace patch ready — click map to place");
	if (button) {
		button->SetLabel("In Hand");
		button->Enable(true);
	}
}

void MarketplaceWindow::OnUpload(wxCommandEvent& WXUNUSED(event)) {
	Editor* editor = g_gui.GetCurrentEditor();
	if (!editor || !editor->hasSelection()) {
		wxMessageBox("Select an area on the map first.", "Marketplace Upload", wxOK | wxICON_INFORMATION, this);
		return;
	}

	const wxString clientVersion = CurrentClientVersionName();
	const long clientVersionId = CurrentClientVersionId();
	if (clientVersion.IsEmpty() || clientVersionId <= 0) {
		wxMessageBox("Load a client VERSION before uploading (spr/dat identity is required).", "Marketplace Upload", wxOK | wxICON_WARNING, this);
		return;
	}

	wxString error;
	if (!EnsureSession(error)) {
		wxMessageBox(error, "Marketplace", wxOK | wxICON_ERROR, this);
		return;
	}

	wxString category = SelectedCategoryId();
	if (category.IsEmpty()) {
		category = "other";
	}

	wxTextEntryDialog titleDlg(
		this,
		wxString::Format("Listing title (will tag VERSION %s):", clientVersion),
		"Upload to Marketplace",
		"My patch"
	);
	if (titleDlg.ShowModal() != wxID_OK) {
		return;
	}
	const wxString title = titleDlg.GetValue().Trim(true).Trim(false);
	if (title.IsEmpty()) {
		return;
	}

	statusLabel->SetLabel("Exporting selection...");
	statusLabel->Refresh();
	wxYield();

	MapJsonExportOptions options;
	options.includeEmptyTiles = false;
	options.includeSpawns = true;
	options.includeHouses = true;
	options.relativeCoords = true;
	options.simplifyUniformGround = true;
	options.clientVersion = nstr(clientVersion);
	options.clientVersionId = static_cast<int>(clientVersionId);

	std::string patchJson;
	if (!MapJsonExporter::exportSelectionToString(*editor, options, patchJson)) {
		wxMessageBox("Failed to export selection to JSON.", "Marketplace", wxOK | wxICON_ERROR, this);
		return;
	}

	std::vector<uint8_t> previewPng;
	if (!editor->exportSelectionPreviewPng(previewPng, 512)) {
		wxMessageBox("Failed to render selection preview PNG.", "Marketplace", wxOK | wxICON_ERROR, this);
		return;
	}

	if (previewPng.size() > 512 * 1024) {
		wxMessageBox("Preview PNG exceeds 512 KB.", "Marketplace", wxOK | wxICON_ERROR, this);
		return;
	}
	if (patchJson.size() + previewPng.size() > 2 * 1024 * 1024) {
		wxMessageBox("Upload exceeds 2 MB limit.", "Marketplace", wxOK | wxICON_ERROR, this);
		return;
	}

	const wxString contentSha = MarketplaceIdentity::Sha256Hex(patchJson);
	statusLabel->SetLabel(wxString::Format("Uploading (VERSION %s)...", clientVersion));
	statusLabel->Refresh();
	wxYield();

	wxString listingId;
	if (!client.UploadListing(sessionKey, title, category, clientVersion, clientVersionId, patchJson, previewPng, contentSha, listingId, error)) {
		wxMessageBox(error, "Marketplace Upload Failed", wxOK | wxICON_ERROR, this);
		statusLabel->SetLabel("Upload failed");
		return;
	}

	statusLabel->SetLabel("Uploaded: " + listingId + " [" + clientVersion + "]");
	RefreshListings();
}

void MarketplaceWindow::OnClose(wxCloseEvent& event) {
	if (event.CanVeto()) {
		Hide();
		event.Veto();
		return;
	}
	g_gui.ClearMarketplaceWindow();
	event.Skip();
}
