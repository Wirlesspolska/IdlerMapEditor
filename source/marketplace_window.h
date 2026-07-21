//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#ifndef RME_MARKETPLACE_WINDOW_H_
#define RME_MARKETPLACE_WINDOW_H_

#include "main.h"
#include "marketplace_client.h"

#include <wx/dialog.h>
#include <wx/listbox.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/button.h>
#include <wx/scrolwin.h>
#include <wx/timer.h>
#include <wx/choice.h>
#include <wx/checkbox.h>

#include <vector>
#include <map>

class MarketplaceWindow : public wxDialog {
public:
	MarketplaceWindow(wxWindow* parent);
	~MarketplaceWindow();

	void RefreshListings();
	void EnsureSessionInteractive();

private:
	void BuildUi();
	void LoadCategories();
	void LoadVersionFilter();
	void ClearCards();
	void PopulateCards(const std::vector<MarketplaceListing>& listings);
	bool EnsureSession(wxString& error);
	wxString SelectedCategoryId() const;
	void ApplyVersionFilterToQuery(MarketplaceListFilter& filter) const;
	wxString CurrentClientVersionName() const;
	long CurrentClientVersionId() const;

	void OnCategorySelect(wxCommandEvent& event);
	void OnVersionFilter(wxCommandEvent& event);
	void OnRefresh(wxCommandEvent& event);
	void OnUpload(wxCommandEvent& event);
	void OnCardClick(wxCommandEvent& event);
	void OnClose(wxCloseEvent& event);
	void OnSetNickname(wxCommandEvent& event);

	MarketplaceClient client;
	wxListBox* categoryList;
	wxScrolledWindow* cardsPanel;
	wxBoxSizer* cardsSizer;
	wxStaticText* statusLabel;
	wxStaticText* versionHintLabel;
	wxTextCtrl* searchCtrl;
	wxChoice* sortChoice;
	wxChoice* versionChoice;
	wxCheckBox* matchCurrentVersionCheck;
	wxButton* refreshButton;
	wxButton* uploadButton;
	wxButton* nicknameButton;

	std::vector<MarketplaceListing> currentListings;
	std::vector<MarketplaceVersion> knownVersions;
	std::map<int, wxString> buttonToListingId;
	wxString sessionKey;
	wxString nickname;
	wxString hwidHash;
	wxString userId;

	DECLARE_EVENT_TABLE()
};

// Background presence indicator (status bar)
class MarketplacePresenceService {
public:
	MarketplacePresenceService();
	~MarketplacePresenceService();

	void Start(wxWindow* host);
	void Stop();
	void Tick();
	void OnTimer(wxTimerEvent& event);

	bool IsOnline() const {
		return online;
	}
	long UsersOnline() const {
		return usersOnline;
	}

private:
	MarketplaceClient client;
	wxTimer* timer;
	wxWindow* hostWindow;
	bool online;
	long usersOnline;
	wxString sessionKey;
	bool bound;
};

extern MarketplacePresenceService g_marketplacePresence;

#endif
