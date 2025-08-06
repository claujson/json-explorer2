
#define _CRT_SECURE_NO_WARNINGS

#ifdef _DEBUG

#endif


//#include "mimalloc-new-delete.h"

#include <string>
#include <algorithm>


#include "claujson.h"

#include "smart_ptr.h"


#include <utility>

//
//

#define __WXMSW__

#include <wx/wx.h>

#include <wx/defs.h>
//
#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/string.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/menu.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/button.h>
#include <wx/textctrl.h>
#include <wx/sizer.h>
#include <wx/dataview.h>
#include <wx/stc/stc.h>
#include <wx/frame.h>


namespace wiz {

	claujson::parser p;
	claujson::writer w;

	inline static claujson::Arena const_str_pool;

	[[nodisgard]]
	bool Parse(const std::string& str, claujson::Document& d) {
		return p.parse_str(str, d, 1).first;
	}

	std::string ToString(const claujson::_Value& data) {
		std::string result;

		if (!data) { // key <- from array?
			return "";
		}

		bool err = false;

		switch (data.type()) {
		case claujson::_ValueType::STRING:
		case claujson::_ValueType::SHORT_STRING:
			result += "\"";
			result += data.get_string().get_std_string(err);
			result += "\"";
			break;
		case claujson::_ValueType::FLOAT:
			result += std::to_string(data.get_floating());
			break;
		case claujson::_ValueType::INT:
			result += std::to_string(data.get_integer());
			break;
		case claujson::_ValueType::UINT:
			result += std::to_string(data.get_unsigned_integer());
			break;
		case claujson::_ValueType::BOOL:
			result += data.get_boolean() ? "true" : "false";
			break;
		case claujson::_ValueType::NULL_:
			result += "null";
			break;
		}
		if (err) {
			return "ERROR";
		}
		return result;
	}


	inline int64_t GetIdx(claujson::StructuredPtr ut, int64_t position, int64_t start) {
		return start + position;
	}

	template <class T>
	T Max(const T& x, const T& y) {
		return x > y ? x : y;
	}

	std::string _ToStringEx(claujson::StructuredPtr ut, long long start, long long& count, long long count_limit) {

		if (count >= count_limit) {
			return std::string();
		}

		std::string str;

		// key = data, key = { }, data, { }, ...
		for (int i = start; i < ut.get_data_size() && count < count_limit; ++i) {
			++count;

			if (ut.is_object()) {
				//str += "\"";
				str += ToString(ut.get_const_key_list(i));
				str += " : ";
			}

			if (ut.get_value_list(i).is_object()) {
				str += " { ";

				str += _ToStringEx(ut.get_value_list(i).as_object(), 0, count, count_limit);
				if (ut.get_value_list(i).is_object()) {
					str += " } ";
				}
				else {
					str += " ] ";
				}
			}
			else if (ut.get_value_list(i).is_array()) {
				str += " [ ";

				str += _ToStringEx(ut.get_value_list(i).as_array(), 0, count, count_limit);
				if (ut.get_value_list(i).is_object()) {
					str += " } ";
				}
				else {
					str += " ] ";
				}
			}
			else {
				// key = data.
				if (ut.is_object()) {
					str += ToString(ut.get_value_list(i));
				}
				else {
					str += ToString(ut.get_value_list(i));
				}
			}

			if (i + 1 < ut.get_data_size() && count < count_limit) {
				str += " , ";
			}

			str += "\n";
		}

		return str;
	}

	std::string ToStringEx(claujson::StructuredPtr ut, long long start, bool& hint) {
		long long count = 0;
		std::string result = _ToStringEx(ut, start, count, 256);
		hint = count >= 256;
		return result;
	}
}


enum class Encoding {
	ANSI, UTF8
};
Encoding encoding = Encoding::UTF8;

//auto default_cp = GetConsoleCP();

inline std::string Convert(const wxString& str) {
	if (Encoding::UTF8 == encoding) {
		return str.ToUTF8().data();
	}
	else {
		return str.ToStdString();
	}
}
inline std::string Convert(wxString&& str) {
	if (Encoding::UTF8 == encoding) {
		return str.ToUTF8().data();
	}
	else {
		return str.ToStdString();
	}
}

wxString Convert(const std::string& str) {
	if (Encoding::UTF8 == encoding) {



		return wxString(str.c_str(), wxConvUTF8);
	}
	else {
		//?
		wxString temp(str.c_str(), wxCSConv(wxFONTENCODING_SYSTEM));

		if (!str.empty() && temp.empty()) {
			temp = wxString(str.c_str(), wxConvISO8859_1);
		}

		return temp;
	}
}
wxString Convert(std::string&& str) {
	if (Encoding::UTF8 == encoding) {
		return wxString(str.c_str(), wxConvUTF8);
	}
	else {

		wxString temp(str.c_str(), wxCSConv(wxFONTENCODING_SYSTEM));

		if (!str.empty() && temp.empty()) {
			temp = wxString(str.c_str(), wxConvISO8859_1);
		}

		return temp;
	}
}

///////////////////////////////////////////////////////////////////////////
#define NK_ENTER 13
#define NK_BACKSPACE 8

using namespace std;

class LangFrame : public wxFrame
{
private:

protected:

	wxStyledTextCtrl* m_code;
	wxButton* m_code_run_button;

protected:
	// parent`s info?

	wiz::SmartPtr2<claujson::StructuredPtr> global;
	claujson::StructuredPtr* now;
	int* ptr_dataViewListCtrlNo;
	long long* ptr_position;

	wxDataViewListCtrl* m_dataViewListCtrl[4];

	wxTextCtrl* textCtrl;

	int mode;

	//wxTextCtrl* dir_ctrl;

	//
	void AddNow(const std::vector<int64_t>& dirVec, claujson::Arena* pool) {
		std::string result = "/."; // dir_result?

		for (int i = 0; i < dirVec.size(); ++i) {
			if (dirVec[i] >= 0) {
				result += "/";
				result += "[";
				result += std::to_string(dirVec[i]);
				result += "]";
			}
			else { // < 0
				result += "/";
				result += "{";
				result += std::to_string(-dirVec[i]);
				result += "}";
			}
		}

		(now)->add_array_element(claujson::_Value(pool, result));
	}

	//
	void _FindByKey(const claujson::_Value& x, const claujson::_Value& key, std::vector<int64_t>& dirVec, claujson::Arena* pool) {
		if (x.is_primitive()) {
			return;
		}

		auto sz = x.as_structured_ptr().get_data_size();
		for (uint64_t i = 0; i < sz; ++i) {
			if (key.is_str()) {
				// found key...
				if (const auto& x_key = x.as_structured_ptr().get_key_list(i); x_key && x_key.get_string() == key.get_string()) {
					AddNow(dirVec, pool);
				}
			}
			else if (key.is_array()) {
				auto sz = key.as_array()->get_data_size();
				for (uint64_t j = 0; j < sz; ++j) {
					if (const auto& x_key = x.as_structured_ptr().get_key_list(i); x_key && x_key.get_string() == key.as_array()->get_value_list(j).get_string()) {
						AddNow(dirVec, pool);
						break;
					}
				}
			}
			if (x.as_structured_ptr().is_user_type()) {
				dirVec.push_back(i);
				if (x.is_object()) {
					dirVec.back() *= -1;
				}
				_FindByKey(x.as_structured_ptr().get_value_list(i), key, dirVec, pool);
				dirVec.pop_back();
			}
		}
	}

	void _FindBy_Value(const claujson::_Value& x, const claujson::_Value& key, std::vector<int64_t>& dirVec, claujson::Arena* pool) {
		if (x.is_primitive()) {
			return;
		}
		
		auto sz = x.as_structured_ptr().get_data_size();
		for (uint64_t i = 0; i < sz; ++i) {
			// found key...
			if (key.is_primitive()) {
				// found key...
				if (const auto& x_value = x.as_structured_ptr().get_value_list(i); x_value && x_value == key) {
					AddNow(dirVec, pool);
				}
			}
			else if (key.is_array()) {
				auto sz = key.as_array()->get_data_size();
				for (uint64_t j = 0; j < sz; ++j) {
					if (const auto& x_value = x.as_structured_ptr().get_value_list(i); x_value && x_value == key.as_array()->get_value_list(j)) {
						AddNow(dirVec, pool);
						break;
					}
				}
			}
			if (x.as_structured_ptr().is_user_type()) {
				dirVec.push_back(i);
				if (x.is_object()) {
					dirVec.back() *= -1;
				}
				_FindBy_Value(x.as_structured_ptr().get_value_list(i), key, dirVec, pool);
				dirVec.pop_back();
			}
		}
	}

	void FindByKey(const claujson::_Value& key, claujson::Arena* pool) {
		auto& x = global->get_value_list(0);
		std::vector<int64_t> dirVec{ 0 };
		_FindByKey(x, key, dirVec, pool);
	}
	void FindBy_Value(const claujson::_Value& value, claujson::Arena* pool) {
		std::vector<int64_t> dirVec{ 0 };
		_FindBy_Value(global->get_value_list(0), value, dirVec, pool);
	}

	inline static const claujson::_Value _find = claujson::_Value(&wiz::const_str_pool, "find"sv);
	inline static const claujson::_Value _find_by_key = claujson::_Value(&wiz::const_str_pool, "find_by_key"sv);
	inline static const claujson::_Value _find_by_value = claujson::_Value(&wiz::const_str_pool, "find_by_value"sv); 


	virtual void m_code_run_buttonOnButtonClick(wxCommandEvent& event) {
		
		try {
			static claujson::Document d;
			d.GetAllocator()->Clear();

			if (now) {
				now->Delete();
			}
			claujson::_Value temp = claujson::Array::Make(nullptr);
			(*now) = temp.as_array();
			if (now == nullptr) {
				return;
			}

			wxString wxNDJsonText = m_code->GetValue();
			std::string ndJsonText = Convert(wxNDJsonText);
			std::string jsonText;
			static claujson::Document query;
			query.GetAllocator()->Clear();

			std::string tempStr;
			bool isFirst = true;
			jsonText.reserve(16 + ndJsonText.size() * 2);

			jsonText = " [ ";
			for (int i = 0; i < ndJsonText.size(); ++i) {
				if (isspace(ndJsonText[i])) {
					if (ndJsonText[i] == '\n' || ndJsonText[i] == '\0') {
						if (!isFirst) {
							jsonText.push_back(','); // no trailing white enterkey!!! - warn..
						}
						isFirst = false;
						jsonText += tempStr;
						tempStr.clear();
					}
					continue;
				}
				tempStr.push_back(ndJsonText[i]);
			}
			if (tempStr.empty() == false) {
				if (!isFirst) {
					jsonText.push_back(',');
				}
				isFirst = false;
				jsonText += tempStr;
			}
			jsonText += " ] ";
			bool valid = wiz::p.parse_str(jsonText, query, 1).first; // 2? 3? 4? - # of thread.
			if (!valid) {
				return;
			}

			uint64_t sz = query.Get().as_array()->get_data_size();
			for (int i = 0; i < sz; ++i) {
				auto& x = query.Get().as_array()->get_value_list(i);
				if (!x.is_object()) {
					continue;
				}

				// found "find_by_key" : [ ] or "find_by_key" : ~~
				if (auto idx = x.find(_find_by_key); idx != claujson::StructuredPtr::npos) {
					if (x.as_object()->get_value_list(idx).is_str()) {
						FindByKey(x.as_object()->get_value_list(idx), d.GetAllocator());
					}
					else if (x.as_object()->get_value_list(idx).is_array()) {
						FindByKey(x.as_object()->get_value_list(idx), d.GetAllocator());
					}
					else {
						return;
					}
				}
				// found "find_by_value" : [ ] or "find_by_value" : ~~
				else if (auto idx = x.find(_find_by_value); idx != claujson::StructuredPtr::npos) {
					if (x.as_object()->get_value_list(idx).is_primitive()) {
						FindBy_Value(x.as_object()->get_value_list(idx), d.GetAllocator());
					}
					else if (x.as_object()->get_value_list(idx).is_array()) {
						FindBy_Value(x.as_object()->get_value_list(idx), d.GetAllocator());
					}
					else {
						return;
					}
				}
				// found "find" : [] or "find" : ~~
				else if (auto idx = x.find(_find); idx != claujson::StructuredPtr::npos) {
					if (x.as_object()->get_value_list(idx).is_primitive()) {
						if (x.as_object()->get_value_list(idx).is_str()) {
							FindByKey(x.as_object()->get_value_list(idx), d.GetAllocator());
						}
						FindBy_Value(x.as_object()->get_value_list(idx), d.GetAllocator());
					}
					else if (x.as_object()->get_value_list(idx).is_array()) {
						FindByKey(x.as_object()->get_value_list(idx), d.GetAllocator());
						FindBy_Value(x.as_object()->get_value_list(idx), d.GetAllocator());
					}
					else {
						return;
					}
				}
			}
		}
		catch (...) {
			// print error to textbox?
		}
	}


public:

	LangFrame(claujson::StructuredPtr* now, int* dataViewListCtrlNo,
		long long *position, wxDataViewListCtrl* m_dataViewListCtrl1, wxDataViewListCtrl* m_dataViewListCtrl2,
		wxDataViewListCtrl* m_dataViewListCtrl3, wxDataViewListCtrl* m_dataViewListCtrl4, wxTextCtrl* textCtrl, wxTextCtrl* dirCtrl,
		wiz::SmartPtr2<claujson::StructuredPtr> global, int mode, wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxEmptyString, 
		const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize(770, 381), long style = wxDEFAULT_FRAME_STYLE | wxTAB_TRAVERSAL);

	~LangFrame();

};

LangFrame::LangFrame(claujson::StructuredPtr* now, int* dataViewListCtrlNo,
	long long* position, wxDataViewListCtrl* m_dataViewListCtrl1, wxDataViewListCtrl* m_dataViewListCtrl2,
	wxDataViewListCtrl* m_dataViewListCtrl3, wxDataViewListCtrl* m_dataViewListCtrl4, wxTextCtrl* textCtrl, wxTextCtrl* dirCtrl,
	wiz::SmartPtr2<claujson::StructuredPtr> global, int mode, wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, 
	const wxSize& size, long style) : wxFrame(parent, id, title, pos, size, style), now(now),
	ptr_dataViewListCtrlNo(dataViewListCtrlNo), ptr_position(position), textCtrl(textCtrl), global(global), mode(mode)
{
	m_dataViewListCtrl[0] = m_dataViewListCtrl1;
	m_dataViewListCtrl[1] = m_dataViewListCtrl2;
	m_dataViewListCtrl[2] = m_dataViewListCtrl3;
	m_dataViewListCtrl[3] = m_dataViewListCtrl4;

	this->SetSizeHints(wxDefaultSize, wxDefaultSize);

	wxBoxSizer* bSizer;
	bSizer = new wxBoxSizer(wxVERTICAL);

	m_code = new wxStyledTextCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, wxEmptyString);
	m_code->SetText(wxString(wxT(
		"\"ClauJson Explorer \"\n\"#		 vztpv@naver.com\"\n"), wxConvUTF8));
	m_code->SetUseTabs(true);
	m_code->SetTabWidth(4);
	m_code->SetIndent(4);
	m_code->SetTabIndents(true);
	m_code->SetBackSpaceUnIndents(true);
	m_code->SetViewEOL(false);
	m_code->SetViewWhiteSpace(false);
	m_code->SetMarginWidth(2, 0);
	m_code->SetIndentationGuides(true);
	m_code->SetMarginType(1, wxSTC_MARGIN_SYMBOL);
	m_code->SetMarginMask(1, wxSTC_MASK_FOLDERS);
	m_code->SetMarginWidth(1, 16);
	m_code->SetMarginSensitive(1, true);
	m_code->SetProperty(wxT("fold"), wxT("1"));
	m_code->SetFoldFlags(wxSTC_FOLDFLAG_LINEBEFORE_CONTRACTED | wxSTC_FOLDFLAG_LINEAFTER_CONTRACTED);
	m_code->SetMarginType(0, wxSTC_MARGIN_NUMBER);
	m_code->SetMarginWidth(0, m_code->TextWidth(wxSTC_STYLE_LINENUMBER, wxT("_99999")));
	m_code->MarkerDefine(wxSTC_MARKNUM_FOLDER, wxSTC_MARK_BOXPLUS);
	m_code->MarkerSetBackground(wxSTC_MARKNUM_FOLDER, wxColour(wxT("BLACK")));
	m_code->MarkerSetForeground(wxSTC_MARKNUM_FOLDER, wxColour(wxT("WHITE")));
	m_code->MarkerDefine(wxSTC_MARKNUM_FOLDEROPEN, wxSTC_MARK_BOXMINUS);
	m_code->MarkerSetBackground(wxSTC_MARKNUM_FOLDEROPEN, wxColour(wxT("BLACK")));
	m_code->MarkerSetForeground(wxSTC_MARKNUM_FOLDEROPEN, wxColour(wxT("WHITE")));
	m_code->MarkerDefine(wxSTC_MARKNUM_FOLDERSUB, wxSTC_MARK_EMPTY);
	m_code->MarkerDefine(wxSTC_MARKNUM_FOLDEREND, wxSTC_MARK_BOXPLUS);
	m_code->MarkerSetBackground(wxSTC_MARKNUM_FOLDEREND, wxColour(wxT("BLACK")));
	m_code->MarkerSetForeground(wxSTC_MARKNUM_FOLDEREND, wxColour(wxT("WHITE")));
	m_code->MarkerDefine(wxSTC_MARKNUM_FOLDEROPENMID, wxSTC_MARK_BOXMINUS);
	m_code->MarkerSetBackground(wxSTC_MARKNUM_FOLDEROPENMID, wxColour(wxT("BLACK")));
	m_code->MarkerSetForeground(wxSTC_MARKNUM_FOLDEROPENMID, wxColour(wxT("WHITE")));
	m_code->MarkerDefine(wxSTC_MARKNUM_FOLDERMIDTAIL, wxSTC_MARK_EMPTY);
	m_code->MarkerDefine(wxSTC_MARKNUM_FOLDERTAIL, wxSTC_MARK_EMPTY);
	m_code->SetSelBackground(true, wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHT));
	m_code->SetSelForeground(true, wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHTTEXT));

	bSizer->Add(m_code, 9, wxEXPAND | wxALL, 5);

	//m_code->Hide();

	m_code_run_button = new wxButton(this, wxID_ANY, wxT("Run"), wxDefaultPosition, wxDefaultSize, 0);
	bSizer->Add(m_code_run_button, 1, wxALL | wxEXPAND, 5);


	this->SetSizer(bSizer);
	this->Layout();

	this->Centre(wxBOTH);

	// Connect Events

	m_code_run_button->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(LangFrame::m_code_run_buttonOnButtonClick), NULL, this);
}

LangFrame::~LangFrame()
{
	if (now) {
		now->Delete();
	}
	// Disconnect Events
	m_code_run_button->Disconnect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(LangFrame::m_code_run_buttonOnButtonClick), NULL, this);

}

class ChangeWindow : public wxDialog
{
private:
	claujson::StructuredPtr ut; 
	bool isStructuredPtr; // ut(true) or it(false)
	int idx; // ut-idx or it-idx. or ilist idx(type == insert)
	int type; // change 1, insert 2
	const int mode;
protected:
	wxTextCtrl* var_text;
	wxTextCtrl* val_text;
	wxButton* ok;

	// Virtual event handlers, overide them in your derived class
	virtual void okOnButtonClick(wxCommandEvent& event) {
		std::string var(Convert(var_text->GetValue()));
		std::string val(Convert(val_text->GetValue()));

		try {
			static claujson::Document document_key = claujson::Document(4 * 1024);
			static claujson::Document document_value = claujson::Document(4 * 1024);
			
			if (1 == type && !isStructuredPtr) { // change
				wiz::Parse(var, document_key);
				claujson::_Value& x = document_key.Get();
				wiz::Parse(val, document_value);
				claujson::_Value& y = document_value.Get();

				if (!y) {
					return; // error.
				}

				if (x && x.is_str() && ut.is_object() && y.is_primitive()) {
					if (idx != -1) {
						ut.change_key(idx, x.clone(ut.get_pool()));
						ut.get_value_list(idx) = y.clone(ut.get_pool());
					}
				}
				else if (!x && ut.is_array() && y.is_primitive()) {
					if (idx != -1) {
						ut.get_value_list(idx) = y.clone(ut.get_pool());
					}
				}
				else {
					return; // error
				}
			}
			else if (1 == type) { // change && isStructuredPtr
				wiz::Parse(var, document_key);
				claujson::_Value& x = document_key.Get();
				if (x && x.is_str() && ut.is_object()) {
					ut.change_key(idx, x.clone(ut.get_pool()));
				}
				else if (x && ut.is_array()) {
					return;
				}
			}
			else if (2 == type) { // insert
				 
				if (mode == 1 && !ut.empty()) { // case json is only one primitive.
					return; 
				}

				wiz::Parse(var, document_key);
				claujson::_Value& x = document_key.Get();
				wiz::Parse(val, document_value);
				claujson::_Value& y = document_value.Get();

				if (!y) {// ROOT as NONE
					return; // error
				}

				if (x.is_str() && ut.is_object()) {
					ut.add_object_element(x.clone(ut.get_pool()), y.clone(ut.get_pool()));
				}
				else if (!x && (ut.is_array() || (ut.get_parent() == nullptr && ut.empty()))) {
					ut.add_array_element(y.clone(ut.get_pool()));
				}
				else {
					return; // error
				}
			}
		}
		catch (...) {
			// print error to textbox?
			return;
		}

		Close();
	}

public:
	ChangeWindow(wxWindow* parent, const int mode, claujson::StructuredPtr ut, bool isStructuredPtr, int idx, int type,
		wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize(580, 198),
		long style = wxDEFAULT_FRAME_STYLE | wxTAB_TRAVERSAL);

	~ChangeWindow();
};

ChangeWindow::ChangeWindow(wxWindow* parent, const int mode, claujson::StructuredPtr ut, bool isStructuredPtr, int idx, int type,
	wxWindowID id, const wxPoint& pos, const wxSize& size, long style)
	: ut(ut), mode(mode), isStructuredPtr(isStructuredPtr), idx(idx), type(type), wxDialog(parent, id, "change/insert window", pos, size, style)
{

	wxBoxSizer* bSizer4;
	bSizer4 = new wxBoxSizer(wxVERTICAL);

	var_text = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0);
	bSizer4->Add(var_text, 1, wxALL | wxEXPAND, 5);

	val_text = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0);
	bSizer4->Add(val_text, 1, wxALL | wxEXPAND, 5);

	ok = new wxButton(this, wxID_ANY, wxT("OK"), wxDefaultPosition, wxDefaultSize, 0);
	bSizer4->Add(ok, 0, wxALL | wxEXPAND, 5);

	if (1 == type) {
		if (isStructuredPtr) {
			var_text->SetValue(Convert(wiz::ToString(ut.get_const_key_list(idx))));
		}
		else {
			var_text->SetValue(Convert(wiz::ToString(ut.get_const_key_list(idx))));
			val_text->SetValue(Convert(wiz::ToString(ut.get_value_list(idx))));
		}
	}


	this->SetSizer(bSizer4);
	this->Layout();

	// Connect Events
	ok->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(ChangeWindow::okOnButtonClick), NULL, this);
}

ChangeWindow::~ChangeWindow()
{
	// Disconnect Events
	ok->Disconnect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(ChangeWindow::okOnButtonClick), NULL, this);
}

///////////////////////////////////////////////////////////////////////////////
/// Class MainFrame
///////////////////////////////////////////////////////////////////////////////
class MainFrame : public wxFrame
{
private:

	bool hint = false;

	bool isMain = false;

	wiz::SmartPtr2<claujson::StructuredPtr> global;
	claujson::StructuredPtr now = nullptr;

	int dataViewListCtrlNo = -1;
	long long position = -1;

	std::vector<long long> part;
	static const long long part_size = 1024; //

	int run_count = 0;

	wiz::SmartPtr<bool> changed;

	std::vector<std::string> dir_vec;

	std::vector<std::string> dir_vec_backup; // compare new dir_text ? + refresh!

	wxString text_ctrl_backup; // compare new text_ctrl data ? + refresh !

	inline static claujson::_Value _str_op = claujson::_Value(&wiz::const_str_pool, "op"sv);
private:
	claujson::Arena pool_for_diff = claujson::Arena(1024 * 1024);

	// json1 <- original, json2 <- target.
	int checkDiff(const std::string& origin_text, const std::string& target_text, claujson::_Value* diff, bool hint) {
		// parse json1, json2
		static claujson::Document json1 = claujson::Document(1024 * 1024);
		static claujson::Document json2 = claujson::Document(1024 * 1024);

		if (wiz::p.parse_str(origin_text, json1, 1).first == 0) {
			return 0;
		}
		if (wiz::p.parse_str(target_text, json2, 1).first == 0) {
			//claujson::clean(json1);
			return 0;
		}

		// diff
		claujson::_Value _diff = claujson::diff(&pool_for_diff, json1.Get(), json2.Get());
		int valid = _diff.is_valid() && _diff.as_structured_ptr() && _diff.as_array()->empty() == false;
		
		// hint <- size > 256 ? 
		if (hint && valid) {
			auto x = _diff.as_array();
			for (int64_t i = 0; x && i < x->get_data_size(); ++i) {
				auto idx = x->get_value_list(i).as_object()->find(_str_op);
				// != replace // add, remove, replace..
				if (!(x->get_value_list(i).as_object()->get_value_list(idx).get_string() == "replace"sv)) {
					valid = 0;
					break;
				}
			}
		}

		*diff = std::move(_diff);

		//claujson::save_parallel("test.json", *diff, 0, true); // debug
		return valid;
	}

	// if fail, returns empty string.
	std::string MakeCompleteJsonText(claujson::StructuredPtr now, int start, const std::string& incomplete_text) {
		std::string complete_json;
		complete_json.reserve(512);

		std::vector<int> is_array; is_array.reserve(512);
		
		if (now.is_array()) {
			complete_json = "[";
			is_array.push_back('[');
		}
		else if (now.is_object()) {
			complete_json = "{";
			is_array.push_back('{');
		}

		{
			int state = 0;
			for (int i = 0; i < incomplete_text.size(); ++i) {
				complete_json.push_back(incomplete_text[i]);
				if (state == 0 && incomplete_text[i] != '\"') {
					//
				}
				else if (state == 0) {
					state = 1;
				}
				else if (state == 1 && incomplete_text[i] == '\"') {
					state = 0;
				}
				else if (state == 1 && incomplete_text[i] == '\\') {
					state = 2;
				}
				else if (state == 2) {
					state = 1;
				}

				if (state == 0) {
					if (incomplete_text[i] == '[') {
						is_array.push_back('[');
					}
					else if (incomplete_text[i] == '{') {
						is_array.push_back('{');
					}
					else if (incomplete_text[i] == ']') {
						if (is_array.empty()) {
							//complete_json = "[" + complete_json;
							
							// fail : totally not valid json.
							return complete_json; //  ""; // empty string is not json.
						}
						else {
							is_array.pop_back();
						}
					}
					else if (incomplete_text[i] == '}') {
						if (is_array.empty()) {
							//complete_json = "{" + complete_json;

							// fail : totally not valid json.
							return complete_json; //return ""; // empty string is not json.
						}
						else {
							is_array.pop_back();
						}
					}
				}
			}
			if (state != 0) {
				// fail : totally not valid json.
				return ""; // empty string is not json.
			}

			while (!is_array.empty()) {
				if (is_array.back() == '[') {
					complete_json += "]";
				}
				else if (is_array.back() == '{') {
					complete_json += "}";
				}

				is_array.pop_back();
			}
		}

		return complete_json;
	}

	// RefreshText with position <- 0
	void RefreshText(claujson::StructuredPtr now) {
		wxDataViewListCtrl* ctrl[4];
		ctrl[0] = m_dataViewListCtrl1;
		ctrl[1] = m_dataViewListCtrl2;
		ctrl[2] = m_dataViewListCtrl3;
		ctrl[3] = m_dataViewListCtrl4;

		long long start = 0;
		long long sum = part.back() * part_size;
		start = sum;

		wxString text = Convert(wiz::ToStringEx(now, start, hint));
		textCtrl->ChangeValue(text);
	}

	void RefreshText2(claujson::StructuredPtr now) {
		wxDataViewListCtrl* ctrl[4];
		ctrl[0] = m_dataViewListCtrl1;
		ctrl[1] = m_dataViewListCtrl2;
		ctrl[2] = m_dataViewListCtrl3;
		ctrl[3] = m_dataViewListCtrl4;

		long long start = 0;
		long long sum = 0;
		long long idx = 0;

		for (int i = 0; i < dataViewListCtrlNo; ++i) {
			idx = wiz::GetIdx(now, ctrl[i]->GetItemCount(), idx);
		}
		sum += wiz::GetIdx(now, position, idx);
		sum += part.back() * part_size;
		start = sum;
	
		wxString text = Convert(wiz::ToStringEx(now, start, hint));
		textCtrl->ChangeValue(text);
	}

	void changedEvent() {
		//m_code_run_result->ChangeValue(wxT("changed"));
		dir_text->SetLabelText(wxT(""));
		now = *global;
		dir_vec.clear();

		hint = false;

		dir_vec.push_back(".");
		position = -1;
		dataViewListCtrlNo = -1;

		part.clear();
		part.push_back(0); 

		RefreshText(now);
		RefreshTable(now);
	
		*changed = false;
	}
	void RefreshTable(claujson::StructuredPtr now, bool chk = true)
	{
		m_dataViewListCtrl1->DeleteAllItems();
		m_dataViewListCtrl2->DeleteAllItems();
		m_dataViewListCtrl3->DeleteAllItems();
		m_dataViewListCtrl4->DeleteAllItems();

		AddData(now, (part.back()) * part_size);

		if (chk == false) {
			return;
		}

		dataViewListCtrlNo = -1;
		position = -1;

		{
			wxDataViewListCtrl* ctrl[4];
			ctrl[0] = m_dataViewListCtrl1;
			ctrl[1] = m_dataViewListCtrl2;
			ctrl[2] = m_dataViewListCtrl3;
			ctrl[3] = m_dataViewListCtrl4;

			for (int i = 0; i < 4; ++i) {
				if (ctrl[i]->GetItemCount() > 0) {
					dataViewListCtrlNo = i;
					position = 0;

					ctrl[dataViewListCtrlNo]->SelectRow(position);
					ctrl[dataViewListCtrlNo]->SetFocus();
					break;
				}
			}
		}


	}
	void AddData(claujson::StructuredPtr global, int start)
	{
		{
			int size = global.get_data_size();

			if (size > part_size) {
				if (size > part_size * part.back() && size <= part_size * (part.back() + 1)) {
					size = size - part_size * part.back();
				}
				else {
					size = part_size;
				}
			}

			int size_per_unit = size / 4;

			const int last_size = size;
			int count = start;

			wxVector<wxVariant> value;

			int i = 0;

			for ( ; i < size_per_unit; ++i) {
				value.clear();

				if (global.get_value_list(i).is_structured()) {
					if (wiz::ToString(global.get_const_key_list(i)).empty()) {
						value.push_back(wxVariant(wxT("")));
					}
					else {
						value.push_back(wxVariant(Convert(wiz::ToString(global.get_const_key_list(i)))));
					}
					if (global.get_value_list(i).is_object()) {
						value.push_back(wxVariant(wxT("{}")));
					}
					else if (global.get_value_list(i).is_array()) {
						value.push_back(wxVariant(wxT("[]")));
					}
					else {
						value.push_back(wxVariant(wxT("ERROR")));
					}
				}
				else { // data2
					if (global.is_object()) {
						value.push_back(wxVariant(Convert(wiz::ToString(global.get_const_key_list(i)))));
						value.push_back(wxVariant(Convert(wiz::ToString(global.get_value_list(i)))));
					}
					else {
						value.push_back(wxVariant(wxT("")));
						value.push_back(wxVariant(Convert(wiz::ToString(global.get_value_list(i)))));
					}

				}

				m_dataViewListCtrl1->AppendItem(value);
				count++;
			}
			for ( ; i < size_per_unit * 2; ++i) {
				value.clear();

				if (global.get_value_list(i).is_structured()) {
					if (wiz::ToString(global.get_const_key_list(i)).empty()) {
						value.push_back(wxVariant(wxT("")));
					}
					else {
						value.push_back(wxVariant(Convert(wiz::ToString(global.get_const_key_list(i)))));
					}
					if (global.get_value_list(i).is_object()) {
						value.push_back(wxVariant(wxT("{}")));
					}
					else if (global.get_value_list(i).is_array()) {
						value.push_back(wxVariant(wxT("[]")));
					}
					else {
						value.push_back(wxVariant(wxT("ERROR")));
					}
				}
				else { // data2
					if (global.is_object()) {
						value.push_back(wxVariant(Convert(wiz::ToString(global.get_const_key_list(i)))));
						value.push_back(wxVariant(Convert(wiz::ToString(global.get_value_list(i)))));
					}
					else {
						value.push_back(wxVariant(wxT("")));
						value.push_back(wxVariant(Convert(wiz::ToString(global.get_value_list(i)))));
					}

				}


				m_dataViewListCtrl2->AppendItem(value);
				count++;
			}
			for ( ; i < size_per_unit * 3; ++i) {
				value.clear();
				if (global.get_value_list(i).is_structured()) {
					if (wiz::ToString(global.get_const_key_list(i)).empty()) {
						value.push_back(wxVariant(wxT("")));
					}
					else {
						value.push_back(wxVariant(Convert(wiz::ToString(global.get_const_key_list(i)))));
					}
					if (global.get_value_list(i).is_object()) {
						value.push_back(wxVariant(wxT("{}")));
					}
					else if (global.get_value_list(i).is_array()) {
						value.push_back(wxVariant(wxT("[]")));
					}
					else {
						value.push_back(wxVariant(wxT("ERROR")));
					}
				}
				else { // data2
					if (global.is_object()) {
						value.push_back(wxVariant(Convert(wiz::ToString(global.get_const_key_list(i)))));
						value.push_back(wxVariant(Convert(wiz::ToString(global.get_value_list(i)))));
					}
					else {
						value.push_back(wxVariant(wxT("")));
						value.push_back(wxVariant(Convert(wiz::ToString(global.get_value_list(i)))));
					}

				}


				m_dataViewListCtrl3->AppendItem(value);
				count++;
			}
			for ( ; i < last_size; ++i) {
				value.clear();

				if (global.get_value_list(i).is_structured()) {
					if (wiz::ToString(global.get_const_key_list(i)).empty()) {
						value.push_back(wxVariant(wxT("")));
					}
					else {
						value.push_back(wxVariant(Convert(wiz::ToString(global.get_const_key_list(i)))));
					}
					if (global.get_value_list(i).is_object()) {
						value.push_back(wxVariant(wxT("{}")));
					}
					else if (global.get_value_list(i).is_array()) {
						value.push_back(wxVariant(wxT("[]")));
					}
					else {
						value.push_back(wxVariant(wxT("ERROR")));
					}
				}
				else { // data2
					if (global.is_object()) {
						value.push_back(wxVariant(Convert(wiz::ToString(global.get_const_key_list(i)))));
						value.push_back(wxVariant(Convert(wiz::ToString(global.get_value_list(i)))));
					}
					else {
						value.push_back(wxVariant(wxT("")));
						value.push_back(wxVariant(Convert(wiz::ToString(global.get_value_list(i)))));
					}

				}


				m_dataViewListCtrl4->AppendItem(value);
				count++;
			}
		}
	}
protected:
	wxTextCtrl* textCtrl;
	
	wxMenuBar* menuBar;
	wxMenu* FileMenu;
	wxMenu* DoMenu;
	wxMenu* WindowMenu;
	
	wxButton* back_button; 
	wxButton* next;
	wxButton* before;
	wxTextCtrl* dir_text;
	wxButton* refresh_button;
	wxDataViewListCtrl* m_dataViewListCtrl1;
	wxDataViewListCtrl* m_dataViewListCtrl2;
	wxDataViewListCtrl* m_dataViewListCtrl3;
	wxDataViewListCtrl* m_dataViewListCtrl4;
	wxStatusBar* m_statusBar1;

	wxTextCtrl* m_code_run_result;
	wxStaticText* m_now_view_text;

	int mode = 0;

	void EnterDir(const std::string& name, std::vector<long long>& part) {
		dir_vec.push_back(name);

		part.push_back(0);

		std::string dir = "";
		for (int i = 0; i < dir_vec.size(); ++i) {
			dir += "/";
			dir += dir_vec[i];


			dir += part[i] > 0 ?  std::string("part") + std::to_string(part[i]) : "";
		}

		dir_text->ChangeValue(Convert(dir));


		dir_vec_backup.clear();
		text_ctrl_backup.Clear();
	}
	void BackDir() {
		if (!dir_vec.empty()) {
			dir_vec.pop_back();

			std::string dir = "";
			for (int i = 0; i < dir_vec.size(); ++i) {
				dir += "/";
				dir += dir_vec[i];
				dir += part[i] > 0 ?  std::string("part") + std::to_string(part[i]) : "";
			}

			dir_text->ChangeValue(Convert(dir));

			dir_vec_backup.clear();
			text_ctrl_backup.Clear();
		}
	}
	// Virtual event handlers, overide them in your derived class
	virtual void FileLoadMenuOnMenuSelection(wxCommandEvent& event) {
		if (!isMain) { return; }
		wxFileDialog* openFileDialog = new wxFileDialog(this);
		static int count = 0;

		if (openFileDialog->ShowModal() == wxID_OK) {
			wxString _fileName = openFileDialog->GetPath();
			std::string fileName(_fileName.c_str());

			static claujson::Document d;

			count++;

			d.GetAllocator()->Clear();

			if (auto x = wiz::p.parse(fileName, d, 0); x.first) {
				claujson::_Value& ut = d.Get();

				encoding = Encoding::UTF8;

				// remove origin..
				if (global->size() > 0) {
					global->erase(0, true);
				}
				now = nullptr;

				//	SetConsoleOutputCP(65001); // for windows
					//setlocale(LC_ALL, "en_US.UTF-8");
				std::string temp = std::to_string(count);

				if (ut.is_primitive()) {
					// todo!, mode? state?
					mode = 1;

					//global = wiz::SmartPtr2<claujson::StructuredPtr>(new claujson::StructuredPtr(new claujson::Array()));

					global->add_array_element(std::move(ut));
				}
				else {
					mode = 0;

					//global = wiz::SmartPtr2<claujson::StructuredPtr>(new claujson::StructuredPtr(new claujson::Array()));

					global->add_array_element(std::move(ut));

					//global = wiz::SmartPtr2<claujson::StructuredPtr>(ut.as_structured_ptr());
				}
				
				m_code_run_result->ChangeValue(wxString::FromUTF8(temp.c_str()) + wxT("Load Success! file is UTF-8"));
			}
			else {
				std::string temp = std::to_string(count);
				m_code_run_result->ChangeValue(wxString::FromUTF8(temp.c_str()) + wxT("Load Failed!"));
			}

			now = *global;

			dataViewListCtrlNo = -1;
			position = now.get_data_size() > 0? 0 : -1;

			run_count = 0;

			dir_vec = std::vector<std::string>();
			dir_vec.push_back(".");

			dir_text->ChangeValue(wxT(""));

			m_now_view_text->SetLabelText(wxT("View Mode A"));

			part.clear();
			part.push_back(0); hint = false;

			RefreshText(now);
			RefreshTable(now);

			SetTitle(wxT("ClauJson Explorer : ") + _fileName);

			*changed = true;
		}

		text_ctrl_backup.Clear();
		dir_vec_backup.clear();
		dir_text->Clear();
		openFileDialog->Destroy();
	}
	
	virtual void FileNewMenuOnMenuSelection(wxCommandEvent& event) {
		if (!isMain) { return; }
		
		encoding = Encoding::UTF8;
		//SetConsoleOutputCP(65001); // Windows..
		//setlocale(LC_ALL, "en_US.UTF-8");

		//global.remove(true);

		now = nullptr;

		//global = wiz::SmartPtr2<claujson::StructuredPtr>(new claujson::StructuredPtr(new claujson::Array()));


		if (global->size() > 0) {
			global->erase(0, true);
		}

		now = *global;

		dataViewListCtrlNo = -1;
		
		position = -1;

		run_count = 0;
		
		dir_vec = std::vector<std::string>();
		dir_text->ChangeValue(wxT("/"));
		
		m_now_view_text->SetLabelText(wxT("View Mode A"));


		part.clear(); hint = false;
		part.push_back(0);

		RefreshText(now);
		RefreshTable(now);

		m_code_run_result->ChangeValue(wxT("New Success! : UTF-8 encoding."));

		*changed = true;

		text_ctrl_backup.Clear(); dir_vec_backup.clear(); dir_text->Clear();
	}
	
	virtual void FileSaveMenuOnMenuSelection(wxCommandEvent& event) {
		if (!isMain) { return; }
		static claujson::Document d;
		
		d.GetAllocator()->Clear();

		wxFileDialog* saveFileDialog = new wxFileDialog(this, _("Save"), "", "",
			"", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

		if (saveFileDialog->ShowModal() == wxID_OK)
		{
			std::string fileName(saveFileDialog->GetPath().c_str());
			if (mode == 0) {
				claujson::Array* arr = nullptr;
				claujson::Object* obj = nullptr;
				claujson::_Value x(global->get_value_list(0).as_structured_ptr());

				global->erase(0);
				
				wiz::w.write_parallel2(fileName, x, 0, true);
				
				global->add_array_element(std::move(x));

				m_code_run_result->SetLabelText(saveFileDialog->GetPath() + wxT(" is saved.."));
			}
			else { // mode == 1
				if (global->empty() == false) {
					claujson::_Value x(global->get_value_list(0).clone(d.GetAllocator()));
					wiz::w.write(fileName, x);
					m_code_run_result->SetLabelText(saveFileDialog->GetPath() + wxT(" is saved.."));
				}
				else {
					m_code_run_result->SetLabelText(wxT("save is failed"));
				}
			}
		}
		saveFileDialog->Destroy();
	}
	virtual void FileExitMenuOnMenuSelection(wxCommandEvent& event) { Close(true); }
	virtual void InsertMenuOnMenuSelection(wxCommandEvent& event) {
		if (!isMain) { return; }
		
		if (*changed) { 
			changedEvent();
		}

		//if (-1 == position) {
		//	return;
		//}

		int idx = now.get_data_size();// / 4)* dataViewListCtrlNo;
		
		bool isStructuredPtr = false; // now.get_value_list(idx)->is_user_type();

		//if (dataViewListCtrlNo == -1) { return; }

		ChangeWindow* changeWindow = new ChangeWindow(this, mode, now, isStructuredPtr, idx, 2);

		changeWindow->ShowModal();

		if (now) {
			RefreshText(now);
			RefreshTable(now);
		}
	}
	virtual void ChangeMenuOnMenuSelection(wxCommandEvent& event) {
		if (!isMain) { return; }

		if (*changed) { changedEvent();
			return;
		}
		if (-1 == position) { return; }

		int idx = wiz::GetIdx(now, position, (now.get_data_size() / 4) * dataViewListCtrlNo);

		bool isStructuredPtr = now.get_value_list(idx).is_structured();

		ChangeWindow* changeWindow = new ChangeWindow(this, mode, now, isStructuredPtr,
			idx, 1);

		changeWindow->ShowModal();

		if (now) {
			RefreshText(now);
			RefreshTable(now);
		}
	}
	virtual void RemoveMenuOnMenuSelection(wxCommandEvent& event) {
		if (!isMain) { return; }
		
		if (*changed) { changedEvent();
			return;
		}
		if (-1 == position) { return; }
		
		int idx = wiz::GetIdx(now, position, ((now.get_data_size()) / 4) * dataViewListCtrlNo);
		
		int type = 1;

		bool isStructuredPtr = now.get_value_list(idx).is_structured();

		if (isStructuredPtr) {
			now.erase(idx, true);
		}
		else {
			if (now.is_object()) {
				now.erase(idx + 1, true);
				now.erase(idx, true);
			}
			else {
				now.erase(idx, true);
			}
		}
		RefreshText(now);
		RefreshTable(now);
	}
	virtual void back_buttonOnButtonClick(wxCommandEvent& event) {
		if (*changed) {
			changedEvent();
			if (!isMain) { return; }
		}
		if (now && now.get_parent()) {
			part.pop_back();

			RefreshText(now.get_parent());
			RefreshTable(now.get_parent());
			(now) = now.get_parent();
			BackDir();
		}
	}
	virtual void refresh_buttonOnButtonClick(wxCommandEvent& event) {
		auto real_position = position;
		auto real_dataViewListCtrlNo = dataViewListCtrlNo;
		auto start_dataViewListCtrlNo = 0;

		{
			wxDataViewListCtrl* ctrl[4];
			ctrl[0] = m_dataViewListCtrl1;
			ctrl[1] = m_dataViewListCtrl2;
			ctrl[2] = m_dataViewListCtrl3;
			ctrl[3] = m_dataViewListCtrl4;

			for (int i = 0; i < 4; ++i) {
				if (ctrl[i]->GetItemCount() > 0) {
					start_dataViewListCtrlNo = i;
					position = 0;
					break;
				}
			}
		}


		if (*changed) {
			changedEvent();
			if (!isMain) { return; }
		}

		if (!isMain) {
			if (now) {
				RefreshText(now);
				RefreshTable(now);
			}
			return;
		}

		if (now && !part.empty()) {
			std::string dir = "";
			for (int i = 0; i < dir_vec.size(); ++i) {
				dir += "/";
				dir += dir_vec[i];
				dir += part[i] > 0 ?  std::string("part") + std::to_string(part[i]) : "";
			}

			if (dir_vec_backup.empty() == false && dir_vec != dir_vec_backup) {
				//while (!dir_vec.empty()) {
				part.clear(); // .pop_back();
				dir_vec.clear(); // .pop_back();
				//}
				
				dir = ".";  now = *global; part.push_back(0);
				
				for (int i = 0; i < dir_vec_backup.size(); ++i) {

					std::string name = dir_vec_backup[i]; // {%int} or [%int]
					dir_vec.push_back(name);

					// error check!!! - todo
					try {
						long long idx = std::stoll(name.substr(1, name.size() - 2));
						long long part_no = idx / part_size;

						if (!(0 <= idx && idx < now.get_data_size())) {
							changedEvent(); // error fix..
							dir_vec_backup.clear();
							return;
						}

						(now) = now.get_value_list(idx).as_structured_ptr();

						part.push_back(part_no);
					}
					catch (...) {
						changedEvent(); // error fix..
						dir_vec_backup.clear();
						return;
					}
				}

				for (int i = 0; i < dir_vec.size(); ++i) {
					dir += "/";
					dir += dir_vec[i];

					dir += part[i] > 0 ? std::string("part") + std::to_string(part[i]) : "";
				}

				dir_text->ChangeValue(Convert(dir));

				RefreshText(now);
				RefreshTable(now);
				
				return;
			}

			dir_text->ChangeValue(Convert(dir));

			RefreshText(now);
			RefreshTable(now);

			{ // diff and patch...	
				wxDataViewListCtrl* ctrl[4];
				ctrl[0] = m_dataViewListCtrl1;
				ctrl[1] = m_dataViewListCtrl2;
				ctrl[2] = m_dataViewListCtrl3;
				ctrl[3] = m_dataViewListCtrl4;

				long long start = 0;
				claujson::_Value diff;

				if (real_position == 0 && real_dataViewListCtrlNo == start_dataViewListCtrlNo && text_ctrl_backup.empty() == false &&
					0 != checkDiff(MakeCompleteJsonText(now, start, Convert(textCtrl->GetValue())), 
									MakeCompleteJsonText(now, start, Convert(text_ctrl_backup)), &diff, hint)) {
					// patch to now.. using diff..
					claujson::_Value temp(now);
					claujson::patch(now.get_pool(), temp, diff);


					// refresh_table(...)
					{
						{
							wxDataViewListCtrl* ctrl[4];
							ctrl[0] = m_dataViewListCtrl1;
							ctrl[1] = m_dataViewListCtrl2;
							ctrl[2] = m_dataViewListCtrl3;
							ctrl[3] = m_dataViewListCtrl4;

							for (int i = 0; i < 4; ++i) {
								if (ctrl[i]->GetItemCount() > 0) {
									dataViewListCtrlNo = i;
									position = 0;

									//claujson::clean(diff);
									pool_for_diff.Clear();

									textCtrl->ChangeValue(Convert(wiz::ToStringEx(now, start, hint)));

									RefreshText(now);
									RefreshTable(now);
									text_ctrl_backup = textCtrl->GetValue();
									return;
								}
							}

							return; // 
						}
					}
					return;
				}
			//	claujson::clean(diff);

				RefreshText(now);
				RefreshTable(now);
				text_ctrl_backup = textCtrl->GetValue(); //
				pool_for_diff.Clear();
			}
		}
	}

	// keyboard
	virtual void m_dataViewListCtrl1OnChar(wxKeyEvent& event) {
		if (*changed) { changedEvent();
			if (!isMain) { return; }
		}
		dataViewListCtrlNo = 0; position = m_dataViewListCtrl1->GetSelectedRow();

		int size = now.get_data_size() ;

		if (size > part_size) {
			if (size >= part_size * part.back() && size < part_size * (part.back() + 1)) {
				size = size - part_size * part.back();
			}
			else {
				size = part_size;
			}
		}

		int length = size;


		if (WXK_ESCAPE == event.GetKeyCode()) {
			wxDataViewListCtrl* ctrl[4];
			ctrl[0] = m_dataViewListCtrl1;
			ctrl[1] = m_dataViewListCtrl2;
			ctrl[2] = m_dataViewListCtrl3;
			ctrl[3] = m_dataViewListCtrl4;

			for (int i = 0; i < 4; ++i) {
				ctrl[i]->UnselectAll();
			}
			position = -1;
		}
		else if (NK_ENTER == event.GetKeyCode() && position >= 0 && now.get_value_list(wiz::GetIdx(now, position, part.back() * part_size)).is_structured()) { // is_user_type <- position < now.get_data_size()
			now = now.get_value_list(wiz::GetIdx(now, position, part.back() * part_size)).as_structured_ptr();
			


			EnterDir(wiz::ToString(claujson::_Value(now)) + (now.get_parent().is_object()? "{" + std::to_string(wiz::GetIdx(now, position , part.back() * part_size))
				 + "}" : "[" + std::to_string(wiz::GetIdx(now, position, part.back() * part_size))  + "]"), part);


			RefreshText(now);
			RefreshTable(now);

		}
		else if (NK_BACKSPACE == event.GetKeyCode() && now.get_parent() != nullptr) {
			now = now.get_parent();
			part.pop_back();
			RefreshText(now);
			RefreshTable(now);
			BackDir();
		}
		else {
			wxDataViewListCtrl* ctrl[4];
			ctrl[0] = m_dataViewListCtrl1;
			ctrl[1] = m_dataViewListCtrl2;
			ctrl[2] = m_dataViewListCtrl3;
			ctrl[3] = m_dataViewListCtrl4;

			ctrl[dataViewListCtrlNo]->UnselectRow(position);

			if (WXK_UP == event.GetKeyCode() && dataViewListCtrlNo > -1 && position > 0)//< ctrl[dataViewListCtrlNo]->GetItemCount())
			{position--;
				RefreshText2(now);
			}
			else if (WXK_DOWN == event.GetKeyCode() && dataViewListCtrlNo > -1 && position >= 0 && position < ctrl[dataViewListCtrlNo]->GetItemCount() - 1)
			{position++;
				RefreshText2(now);
			}
			else if (WXK_LEFT == event.GetKeyCode() && dataViewListCtrlNo > 0 && position >= 0 && position < ctrl[dataViewListCtrlNo - 1]->GetItemCount())
			{
				
				dataViewListCtrlNo--; RefreshText2(now);
			}
			else if (WXK_RIGHT == event.GetKeyCode() && dataViewListCtrlNo < 3 && position >= 0 && position < ctrl[dataViewListCtrlNo + 1]->GetItemCount())
			{
				
				dataViewListCtrlNo++; RefreshText2(now);
			}

			ctrl[dataViewListCtrlNo]->SelectRow(position);
			ctrl[dataViewListCtrlNo]->SetFocus();
		}
	}
	virtual void m_dataViewListCtrl2OnChar(wxKeyEvent& event) {
		if (*changed) { changedEvent();
			if (!isMain) { return; }
		}

		int size = now.get_data_size();

		if (size > part_size) {
			if (size >= part_size * part.back() && size < part_size * (part.back() + 1)) {
				size = size - part_size * part.back();
			}
			else {
				size = part_size;
			}
		}

		int length = size;

		dataViewListCtrlNo = 1; position = m_dataViewListCtrl2->GetSelectedRow();
		if (WXK_ESCAPE == event.GetKeyCode()) {
			wxDataViewListCtrl* ctrl[4];
			ctrl[0] = m_dataViewListCtrl1;
			ctrl[1] = m_dataViewListCtrl2;
			ctrl[2] = m_dataViewListCtrl3;
			ctrl[3] = m_dataViewListCtrl4;

			for (int i = 0; i < 4; ++i) {
				ctrl[i]->UnselectAll();
			}
			position = -1;
		}
		else if (NK_ENTER == event.GetKeyCode() && dataViewListCtrlNo == 1 && position >= 0 &&
				now.get_value_list(wiz::GetIdx(now, position, (length) / 4 + part.back() * part_size)).is_structured()) {
			now = now.get_value_list(wiz::GetIdx(now, position, (length) / 4 + part.back() * part_size)).as_structured_ptr();

		
			EnterDir(wiz::ToString(claujson::_Value(now)) + (now.get_parent().is_object() ? "{" + std::to_string(wiz::GetIdx(now, position, (length) / 4 + part.back() * part_size)
				)  + "}" :
				"[" + std::to_string(wiz::GetIdx(now, position, (length) / 4 + part.back() * part_size) )  + "]"), part);

			RefreshText(now);
			RefreshTable(now);
		}
		else if (NK_BACKSPACE == event.GetKeyCode() && now.get_parent() != nullptr) {
			now = now.get_parent();
			part.pop_back();
			RefreshText(now);
			RefreshTable(now);
			BackDir();
		}
		else {
			wxDataViewListCtrl* ctrl[4];
			ctrl[0] = m_dataViewListCtrl1;
			ctrl[1] = m_dataViewListCtrl2;
			ctrl[2] = m_dataViewListCtrl3;
			ctrl[3] = m_dataViewListCtrl4;

			ctrl[dataViewListCtrlNo]->UnselectRow(position);

			if (WXK_UP == event.GetKeyCode() && dataViewListCtrlNo > -1 && position > 0)//< ctrl[dataViewListCtrlNo]->GetItemCount())
			{position--;
				RefreshText2(now);
			}
			else if (WXK_DOWN == event.GetKeyCode() && dataViewListCtrlNo > -1 && position >= 0 && position < ctrl[dataViewListCtrlNo]->GetItemCount() - 1)
			{ position++;
				RefreshText2(now);
			}
			else if (WXK_LEFT == event.GetKeyCode() && dataViewListCtrlNo > 0 && position >= 0 && position < ctrl[dataViewListCtrlNo - 1]->GetItemCount())
			{
				dataViewListCtrlNo--;
				RefreshText2(now);
			}
			else if (WXK_RIGHT == event.GetKeyCode() && dataViewListCtrlNo < 3 && position >= 0 && position < ctrl[dataViewListCtrlNo + 1]->GetItemCount())
			{
				
				dataViewListCtrlNo++;
				RefreshText2(now);
			}

			ctrl[dataViewListCtrlNo]->SelectRow(position);
			ctrl[dataViewListCtrlNo]->SetFocus();
		}

	}
	virtual void m_dataViewListCtrl3OnChar(wxKeyEvent& event) {
		if (*changed) { changedEvent();
			if (!isMain) { return; }
		}


		int size = now.get_data_size();

		if (size > part_size) {
			if (size >= part_size * part.back() && size < part_size * (part.back() + 1)) {
				size = size - part_size * part.back();
			}
			else {
				size = part_size;
			}
		}

		int length = size;

		dataViewListCtrlNo = 2; position = m_dataViewListCtrl3->GetSelectedRow();
		if (WXK_ESCAPE == event.GetKeyCode()) {
			wxDataViewListCtrl* ctrl[4];
			ctrl[0] = m_dataViewListCtrl1;
			ctrl[1] = m_dataViewListCtrl2;
			ctrl[2] = m_dataViewListCtrl3;
			ctrl[3] = m_dataViewListCtrl4;

			for (int i = 0; i < 4; ++i) {
				ctrl[i]->UnselectAll();
			}
			position = -1;
		}
		else if (NK_ENTER == event.GetKeyCode() && dataViewListCtrlNo == 2 && position >= 0
				&& now.get_value_list(wiz::GetIdx(now, position, (length) / 4 * 2 + part.back() * part_size) ).is_structured()) {
			now = now.get_value_list(wiz::GetIdx(now, position, (length) / 4 * 2 + part.back() * part_size) ).as_structured_ptr();
			

			
			EnterDir(wiz::ToString(claujson::_Value(now)) + (now.get_parent().is_object() ? "{" + std::to_string(wiz::GetIdx(now, position, (length) / 4 * 2 + part.back() * part_size)
				)  + "}" :
				"[" + std::to_string(wiz::GetIdx(now, position, (length) / 4 * 2 + part.back() * part_size) + (length) / 4 * 2 + part.back() * part_size)  + "]"), part);

			RefreshText(now);
			RefreshTable(now);
		}
		else if (NK_BACKSPACE == event.GetKeyCode() && now.get_parent() != nullptr) {
			now = now.get_parent();
			part.pop_back();

			RefreshText(now);
			RefreshTable(now);
			BackDir();

		}
		else {
			wxDataViewListCtrl* ctrl[4];
			ctrl[0] = m_dataViewListCtrl1;
			ctrl[1] = m_dataViewListCtrl2;
			ctrl[2] = m_dataViewListCtrl3;
			ctrl[3] = m_dataViewListCtrl4;

			ctrl[dataViewListCtrlNo]->UnselectRow(position);

			if (WXK_UP == event.GetKeyCode() && dataViewListCtrlNo > -1 && position > 0)//< ctrl[dataViewListCtrlNo]->GetItemCount())
			{ position--;
				RefreshText2(now);
			}
			else if (WXK_DOWN == event.GetKeyCode() && dataViewListCtrlNo > -1 && position >= 0 && position < ctrl[dataViewListCtrlNo]->GetItemCount() - 1)
			{position++;
				RefreshText2(now);
			}
			else if (WXK_LEFT == event.GetKeyCode() && dataViewListCtrlNo > 0 && position >= 0 && position < ctrl[dataViewListCtrlNo - 1]->GetItemCount())
			{
				
				dataViewListCtrlNo--;
				RefreshText2(now);
			}
			else if (WXK_RIGHT == event.GetKeyCode() && dataViewListCtrlNo < 3 && position >= 0 && position < ctrl[dataViewListCtrlNo + 1]->GetItemCount())
			{
				dataViewListCtrlNo++;
				RefreshText2(now);
			}

			ctrl[dataViewListCtrlNo]->SelectRow(position);
			ctrl[dataViewListCtrlNo]->SetFocus();
		}
	}
	virtual void m_dataViewListCtrl4OnChar(wxKeyEvent& event) {
		if (*changed) { changedEvent();
			if (!isMain) { return; }
		}

		int size = now.get_data_size();
		

		if (size > part_size) {
			if (size >= part_size * part.back() && size < part_size * (part.back() + 1)) {
				size = size - part_size * part.back();
			}
			else {
				size = part_size;
			}
		}

		int length = size;
		dataViewListCtrlNo = 3; position = m_dataViewListCtrl4->GetSelectedRow();
		if (WXK_ESCAPE == event.GetKeyCode()) {
			wxDataViewListCtrl* ctrl[4];
			ctrl[0] = m_dataViewListCtrl1;
			ctrl[1] = m_dataViewListCtrl2;
			ctrl[2] = m_dataViewListCtrl3;
			ctrl[3] = m_dataViewListCtrl4;

			for (int i = 0; i < 4; ++i) {
				ctrl[i]->UnselectAll();
			}
			position = -1;
		}
		else if (NK_ENTER == event.GetKeyCode() && dataViewListCtrlNo == 3 && position >= 0
			&& now.get_value_list(wiz::GetIdx(now, position, (length) / 4 * 3 + part.back() * part_size)).is_structured()) {
			now = now.get_value_list(wiz::GetIdx(now, position, (length) / 4 * 3 + part.back() * part_size)).as_structured_ptr();

		


			EnterDir(wiz::ToString(claujson::_Value(now)) + (now.get_parent().is_object() ? "{" + std::to_string(wiz::GetIdx(now, position, (length) / 4 * 3 + part.back() * part_size))  + "}" :
				"[" + std::to_string(wiz::GetIdx(now, position, (length) / 4 * 3 + part.back() * part_size) + (length) / 4 * 3 + part.back() * part_size)  + "]"), part);

			RefreshText(now);
			RefreshTable(now);
		}
		else if (NK_BACKSPACE == event.GetKeyCode() && now.get_parent() != nullptr) {
			now = now.get_parent();
			part.pop_back();

			RefreshText(now);
			RefreshTable(now);
			BackDir();

		}
		else {
			wxDataViewListCtrl* ctrl[4];
			ctrl[0] = m_dataViewListCtrl1;
			ctrl[1] = m_dataViewListCtrl2;
			ctrl[2] = m_dataViewListCtrl3;
			ctrl[3] = m_dataViewListCtrl4;

			ctrl[dataViewListCtrlNo]->UnselectRow(position);

			if (WXK_UP == event.GetKeyCode() && dataViewListCtrlNo > -1 && position > 0)//< ctrl[dataViewListCtrlNo]->GetItemCount())
			{position--;
				RefreshText2(now);
			}
			else if (WXK_DOWN == event.GetKeyCode() && dataViewListCtrlNo > -1 && position >= 0 && position < ctrl[dataViewListCtrlNo]->GetItemCount() - 1)
			{position++;
				RefreshText2(now);
			}
			else if (WXK_LEFT == event.GetKeyCode() && dataViewListCtrlNo > 0 && position >= 0 && position < ctrl[dataViewListCtrlNo - 1]->GetItemCount())
			{
				dataViewListCtrlNo--;
				RefreshText2(now);
			}
			else if (WXK_RIGHT == event.GetKeyCode() && dataViewListCtrlNo < 3 && position >= 0 && position < ctrl[dataViewListCtrlNo + 1]->GetItemCount())
			{
				dataViewListCtrlNo++;
				RefreshText2(now);
			}

			ctrl[dataViewListCtrlNo]->SelectRow(position);
			ctrl[dataViewListCtrlNo]->SetFocus();
		}
	}


	// Part - very long? array or object.
	virtual void nextOnButtonClick(wxCommandEvent& event) { 
		if (now) {
			long long length = now.get_data_size();

			if (!part.empty() && length > (part.back() + 1) * part_size) {
				part.back()++;

				RefreshText(now);
				RefreshTable(now);

				std::string dir = "";
				for (int i = 0; i < dir_vec.size(); ++i) {
					dir += "/";
					dir += dir_vec[i];
					dir += part[i] > 0 ?  std::string("part") + std::to_string(part[i]) : "";
				}

				dir_text->ChangeValue(Convert(dir));
			}
		}
	}

	virtual void beforeOnButtonClick(wxCommandEvent& event) {
		if (now) {

			if (!part.empty() && part.back() >= 1) {
				part.back()--;

				RefreshText(now);
				RefreshTable(now);

				std::string dir = "";
				for (int i = 0; i < dir_vec.size(); ++i) {
					dir += "/";
					dir += dir_vec[i];
					dir += part[i] > 0 ? std::string("part") + std::to_string(part[i]) : "";
				}

				dir_text->ChangeValue(Convert(dir));
			}
		}
	}


	// double click.
	virtual void m_dataViewListCtrl1OnDataViewListCtrlItemActivated(wxDataViewEvent& event) {
		if (*changed) { changedEvent();
			if (!isMain) { return; }
		}
		int size = now.get_data_size();

		if (size > part_size) {
			if (size >= part_size * part.back() && size < part_size * (part.back() + 1)) {
				size = size - part_size * part.back();
			}
			else {
				size = part_size;
			}
		}

		int length = size;
		dataViewListCtrlNo = 0; position = m_dataViewListCtrl1->GetSelectedRow();

		if (position >= 0 && now.get_value_list(wiz::GetIdx(now, position, (length) / 4 * 0 + part.back() * part_size) + (length) / 4 * 0 + part.back() * part_size).is_structured()) {
			now = now.get_value_list(wiz::GetIdx(now, position, (length) / 4 * 0 + part.back() * part_size) + part.back() * part_size).as_structured_ptr();

			EnterDir(wiz::ToString(claujson::_Value(now)) + (now.get_parent().is_object() ? "{" + 
					std::to_string(wiz::GetIdx(now, position, (length) / 4 * 0 + part.back() * part_size))  + "}" :
				"[" + std::to_string(wiz::GetIdx(now, position, (length) / 4 * 0 + part.back() * part_size) )  + "]"), part);


			RefreshText(now);
			RefreshTable(now);

		}
	}
	virtual void m_dataViewListCtrl2OnDataViewListCtrlItemActivated(wxDataViewEvent& event) {
		if (*changed) { changedEvent();
			if (!isMain) { return; }
		}
		int size = now.get_data_size();
		

		if (size > part_size) {
			if (size >= part_size * part.back() && size < part_size * (part.back() + 1)) {
				size = size - part_size * part.back();
			}
			else {
				size = part_size;
			}
		}

		int length = size;
		dataViewListCtrlNo = 1; position = m_dataViewListCtrl2->GetSelectedRow();
		if (dataViewListCtrlNo == 1 && position >= 0 && now.get_value_list(wiz::GetIdx(now, position , (length) / 4 + part.back() * part_size) ).is_structured()) {
			now = now.get_value_list(wiz::GetIdx(now, position, (length) / 4 + part.back() * part_size) ).as_structured_ptr();
			
			

			EnterDir(wiz::ToString(claujson::_Value(now)) + (now.get_parent().is_object() ? "{" + std::to_string(wiz::GetIdx(now, position, (length) / 4 * 1 + part.back() * part_size) ) + "}" :
				"[" + std::to_string(wiz::GetIdx(now, position, (length) / 4 * 1 + part.back() * part_size))  + "]"), part);
			

			RefreshText(now);
			RefreshTable(now);
		}
	}
	virtual void m_dataViewListCtrl3OnDataViewListCtrlItemActivated(wxDataViewEvent& event) {
		if (*changed) { changedEvent();
			if (!isMain) { return; }
		}

		int size = now.get_data_size();
		
		if (size > part_size) {
			if (size >= part_size * part.back() && size < part_size * (part.back() + 1)) {
				size = size - part_size * part.back();
			}
			else {
				size = part_size;
			}
		}

		int length = size;
		dataViewListCtrlNo = 2; position = m_dataViewListCtrl3->GetSelectedRow();
		if (dataViewListCtrlNo == 2 && position >= 0 && now.get_value_list(wiz::GetIdx(now, position, (length) / 4 * 2 + part.back() * part_size)).is_structured()) {
			now = now.get_value_list(wiz::GetIdx(now, position,  (length) / 4 * 2 + part.back() * part_size)).as_structured_ptr();

			EnterDir(wiz::ToString(claujson::_Value(now)) + (now.get_parent().is_object() ? "{" + std::to_string(wiz::GetIdx(now, position, (length) / 4 * 2 + part.back() * part_size))  + "}" :
				"[" + std::to_string(wiz::GetIdx(now, position, (length) / 4 * 2 + part.back() * part_size) ) + "]"), part);

			RefreshText(now);
			RefreshTable(now);

		}
	}
	virtual void m_dataViewListCtrl4OnDataViewListCtrlItemActivated(wxDataViewEvent& event) {
		if (*changed) { changedEvent();
			if (!isMain) { return; }
		}

		int size = now.get_data_size();

		if (size > part_size) {
			if (size >= part_size * part.back() && size < part_size * (part.back() + 1)) {
				size = size - part_size * part.back();
			}
			else {
				size = part_size;
			}
		}

		long long length = size;

		dataViewListCtrlNo = 3; position = m_dataViewListCtrl4->GetSelectedRow();
		if (dataViewListCtrlNo == 3 && position >= 0 && now.get_value_list(wiz::GetIdx(now, position, (length) / 4 * 3 + part.back() * part_size)).is_structured()) {
			now = now.get_value_list(wiz::GetIdx(now, position, (length) / 4 * 3 + part.back() * part_size)).as_structured_ptr();
			
		

			EnterDir(wiz::ToString(claujson::_Value(now)) + (now.get_parent().is_object() ? "{" + 
				std::to_string(wiz::GetIdx(now, position, (length / 4 * 3) + part.back() * part_size))  + "}" :
				"[" + std::to_string(wiz::GetIdx(now, position, (length / 4 * 3) + part.back() * part_size))  + "]"), part);

			RefreshText(now);
			RefreshTable(now);

		}
	}

	// right click.
	virtual void m_dataViewListCtrl1OnDataViewListCtrlItemContextMenu(wxDataViewEvent& event) {
		if (*changed) {
			changedEvent();
		}
		if (now && now.get_parent()) {
			now = now.get_parent();
			part.pop_back();
			RefreshText(now);
			RefreshTable(now);
			BackDir();
		}
	}
	virtual void m_dataViewListCtrl2OnDataViewListCtrlItemContextMenu(wxDataViewEvent& event) {
		if (*changed) {
			changedEvent();
		}
		if (now && now.get_parent()) {
			now = now.get_parent();
			part.pop_back();
			RefreshText(now);
			RefreshTable(now);
			BackDir();
		}
	}
	virtual void m_dataViewListCtrl3OnDataViewListCtrlItemContextMenu(wxDataViewEvent& event) {
		if (*changed) {
			changedEvent();
		}
		if (now && now.get_parent()) {
			now = now.get_parent();
			part.pop_back();
			RefreshText(now);
			RefreshTable(now);
			BackDir();
		}
	}
	virtual void m_dataViewListCtrl4OnDataViewListCtrlItemContextMenu(wxDataViewEvent& event) {
		if (*changed) {
			changedEvent();
		}
		if (now && now.get_parent()) {
			now = now.get_parent();
			part.pop_back();

			RefreshText(now);
			RefreshTable(now);
			BackDir();
		}
	}

	//
	virtual void m_dataViewListCtrl1OnDataViewListCtrlSelectionchanged(wxDataViewEvent& event) {
		if (*changed) { changedEvent();
			if (!isMain) { return; }
		}

		dataViewListCtrlNo = 0;

		wxDataViewListCtrl* ctrl[4];
		ctrl[0] = m_dataViewListCtrl1;
		ctrl[1] = m_dataViewListCtrl2;
		ctrl[2] = m_dataViewListCtrl3;
		ctrl[3] = m_dataViewListCtrl4;

		for (int i = 0; i < 4; ++i) {
			if (i != dataViewListCtrlNo) {
				ctrl[i]->UnselectAll();
			}
		}

		auto temp_position = m_dataViewListCtrl1->GetSelectedRow();

		if (temp_position != position) {
			ctrl[dataViewListCtrlNo]->UnselectRow(position);
			position = temp_position;
		}
	
		ctrl[dataViewListCtrlNo]->SelectRow(position);
		ctrl[dataViewListCtrlNo]->SetFocus();

		RefreshText2(now);
	}
	virtual void m_dataViewListCtrl2OnDataViewListCtrlSelectionchanged(wxDataViewEvent& event) {
		if (*changed) { changedEvent();
			if (!isMain) { return; }
		}
		dataViewListCtrlNo = 1;

		wxDataViewListCtrl* ctrl[4];
		ctrl[0] = m_dataViewListCtrl1;
		ctrl[1] = m_dataViewListCtrl2;
		ctrl[2] = m_dataViewListCtrl3;
		ctrl[3] = m_dataViewListCtrl4;

		for (int i = 0; i < 4; ++i) {
			if (i != dataViewListCtrlNo) {
				ctrl[i]->UnselectAll();
			}
		}

		auto temp_position = m_dataViewListCtrl2->GetSelectedRow();

		if (temp_position != position) {
			ctrl[dataViewListCtrlNo]->UnselectRow(position);
			position = temp_position;
		}

		ctrl[dataViewListCtrlNo]->SelectRow(position);
		ctrl[dataViewListCtrlNo]->SetFocus();

		RefreshText2(now);
	}
	virtual void m_dataViewListCtrl3OnDataViewListCtrlSelectionchanged(wxDataViewEvent& event) {
		if (*changed) { changedEvent();
			if (!isMain) { return; }
		}
		dataViewListCtrlNo = 2;	
		
		wxDataViewListCtrl* ctrl[4];
		ctrl[0] = m_dataViewListCtrl1;
		ctrl[1] = m_dataViewListCtrl2;
		ctrl[2] = m_dataViewListCtrl3;
		ctrl[3] = m_dataViewListCtrl4;

		for (int i = 0; i < 4; ++i) {
			if (i != dataViewListCtrlNo) {
				ctrl[i]->UnselectAll();
			}
		}

		auto temp_position = m_dataViewListCtrl3->GetSelectedRow();

		if (temp_position != position) {
			ctrl[dataViewListCtrlNo]->UnselectRow(position);
			position = temp_position;
		}

		ctrl[dataViewListCtrlNo]->SelectRow(position);
		ctrl[dataViewListCtrlNo]->SetFocus();

		RefreshText2(now);
	}
	virtual void m_dataViewListCtrl4OnDataViewListCtrlSelectionchanged(wxDataViewEvent& event) {
		if (*changed) { changedEvent();
			if (!isMain) { return; }
		}
		dataViewListCtrlNo = 3;

		wxDataViewListCtrl* ctrl[4];
		ctrl[0] = m_dataViewListCtrl1;
		ctrl[1] = m_dataViewListCtrl2;
		ctrl[2] = m_dataViewListCtrl3;
		ctrl[3] = m_dataViewListCtrl4;

		for (int i = 0; i < 4; ++i) {
			if (i != dataViewListCtrlNo) {
				ctrl[i]->UnselectAll();
			}
		}

		auto temp_position = m_dataViewListCtrl4->GetSelectedRow();

		if (temp_position != position) {
			ctrl[dataViewListCtrlNo]->UnselectRow(position);
			position = temp_position;
		}

		ctrl[dataViewListCtrlNo]->SelectRow(position);
		ctrl[dataViewListCtrlNo]->SetFocus();

		RefreshText2(now);
	}

	virtual void m_dataViewListCtrl1OnSize(wxSizeEvent& event) {
		m_dataViewListCtrl1->GetColumn(0)->SetWidth(m_dataViewListCtrl1->GetSize().GetWidth() / 2 * 0.92); // check...
		m_dataViewListCtrl1->GetColumn(1)->SetWidth(m_dataViewListCtrl1->GetSize().GetWidth() / 2 * 0.92);
		event.Skip();
	}
	virtual void m_dataViewListCtrl2OnSize(wxSizeEvent& event) {
		m_dataViewListCtrl2->GetColumn(0)->SetWidth(m_dataViewListCtrl2->GetSize().GetWidth() / 2 * 0.92); // check...
		m_dataViewListCtrl2->GetColumn(1)->SetWidth(m_dataViewListCtrl2->GetSize().GetWidth() / 2 * 0.92);
		event.Skip();
	}
	virtual void m_dataViewListCtrl3OnSize(wxSizeEvent& event) {
		m_dataViewListCtrl3->GetColumn(0)->SetWidth(m_dataViewListCtrl3->GetSize().GetWidth() / 2 * 0.92); // check...
		m_dataViewListCtrl3->GetColumn(1)->SetWidth(m_dataViewListCtrl3->GetSize().GetWidth() / 2 * 0.92);
		event.Skip();
	}
	virtual void m_dataViewListCtrl4OnSize(wxSizeEvent& event) {
		m_dataViewListCtrl4->GetColumn(0)->SetWidth(m_dataViewListCtrl4->GetSize().GetWidth() / 2 * 0.92); // check...
		m_dataViewListCtrl4->GetColumn(1)->SetWidth(m_dataViewListCtrl4->GetSize().GetWidth() / 2 * 0.92);
		event.Skip();
	}

	virtual void OtherWindowMenuOnMenuSelection(wxCommandEvent& event) {
		if (*changed) { changedEvent(); }

		if (!isMain) { return; }
		MainFrame* frame = new MainFrame(this->changed, this->mode, this->global, this->now, this);

		frame->dir_vec = this->dir_vec;

		frame->part = this->part;

		std::string dir = "";
		for (int i = 0; i < frame->dir_vec.size(); ++i) {
			dir += "/";
			dir += frame->dir_vec[i];

			dir += frame->part[i] > 0 ?  std::string(" part") + std::to_string(frame->part[i]) : "";
		}

		frame->dir_text->ChangeValue(Convert(dir));

		frame->RefreshTable(frame->now);
		frame->RefreshText(frame->now);

		frame->SetTitle(GetTitle() + wxT(" : other window"));

		frame->Show(true);
	}
	
	virtual void SearchWindowMenuSelection(wxCommandEvent& event) {
		if (*changed) { changedEvent(); }

		if (!isMain) { return; }
		int mode = 2;
		MainFrame* frame = new MainFrame(this->changed, mode, this->global, nullptr, this); // now <- nullptr

		frame->SetTitle(GetTitle() + wxT(" : Search window"));

		frame->Show(true);
	}

	virtual void LangMenuOnMenuSelection(wxCommandEvent& event) {
		if (*changed) { changedEvent(); }

		if (mode < 2) {
			return;
		}

		LangFrame* frame = new LangFrame(&this->now, &dataViewListCtrlNo, &position, m_dataViewListCtrl1, m_dataViewListCtrl2,
			m_dataViewListCtrl3, m_dataViewListCtrl4, textCtrl, dir_text, this->global, mode, this);
		
		frame->Show(true);
	}

public:

	MainFrame(wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxT("ClauJson Explorer"), const wxPoint& pos = wxDefaultPosition, 
		const wxSize& size = wxSize(1024, 512), long style = wxDEFAULT_FRAME_STYLE | wxTAB_TRAVERSAL);
private:
	MainFrame(wiz::SmartPtr<bool> changed, int mode, wiz::SmartPtr2<claujson::StructuredPtr> global, claujson::StructuredPtr now, wxWindow* parent, wxWindowID id = wxID_ANY, 
		const wxString& title = wxT("ClauJson Explorer"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize(1024, 512), 
		long style = wxDEFAULT_FRAME_STYLE | wxTAB_TRAVERSAL);
public:
	~MainFrame();
	
	void init(wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxT("ClauJson Explorer"), const wxPoint& pos = wxDefaultPosition, 
		const wxSize& size = wxSize(1024, 512), long style = wxDEFAULT_FRAME_STYLE | wxTAB_TRAVERSAL);

	void FirstFrame() {
		static claujson::Arena pool;
		isMain = true;
		mode = 1;
		global = wiz::SmartPtr2<claujson::StructuredPtr>(new claujson::StructuredPtr(claujson::Array::Make(&pool)));
		now = *global;
	}

	// no valid check??
	std::vector<std::string> parse_to_dir_vec(const wxString& x) {
		std::string str = Convert(x);
		std::vector<std::string> result;

		if (global->is_array() || global->is_object()) {
			// /./[%int] or {%int}/ ...
			long long start = 0;
			long long begin = -1;
			long long end = -1;

			while (true) {
				if (auto idx = str.find('/', start); idx != std::string::npos) {
					start = idx + 1;
					begin = start;
					end = str.find('/', start);

					if (begin >= 0 && begin < end) {
						if (str[begin] != '{' && str[begin] != '[' && str[begin] != '.') {
							return {};
						}
						result.push_back(str.substr(begin, end - begin));
						if (end > 0 && (str[end - 1] != '}' && str[end - 1] != ']' && str[end - 1] != '.')) {
							return {};
						}
					};
				}
				else {
					break;
				}
			}
		}
	
		return result;
	}
private:
	// text_ctrl
	virtual void m_textCtrlOnText(wxCommandEvent& event) { 
		text_ctrl_backup = event.GetString();
	}
	// dir_text
	virtual void m_textCtrl2OnText(wxCommandEvent& event) {
		if (event.GetString()[event.GetString().size() - 1] == '/') {
			dir_vec_backup = parse_to_dir_vec(event.GetString());
		}
	}
};
MainFrame::MainFrame(wiz::SmartPtr<bool> changed, int mode, wiz::SmartPtr2<claujson::StructuredPtr> global, claujson::StructuredPtr now, wxWindow* parent, wxWindowID id,
	const wxString& title, const wxPoint& pos, const wxSize& size, long style) : wxFrame(parent, id, title, pos, size, style)
{
	init(parent, id, title, pos, size, style);

	this->changed = changed;
	this->global = global;
	this->now = now;
	
	this->mode = mode;

	part.push_back(0);
}
MainFrame::MainFrame(wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style) : wxFrame(parent, id, title, pos, size, style)
{
	changed = new bool;
	
	*changed = false;

	part.push_back(0);

	init(parent, id, title, pos, size, style);
}

void MainFrame::init(wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style)
{
	this->SetSizeHints(wxDefaultSize, wxDefaultSize);

	menuBar = new wxMenuBar(0);
	FileMenu = new wxMenu();


	wxMenuItem* FileNewMenu;
	FileNewMenu = new wxMenuItem(FileMenu, wxID_ANY, wxString(wxT("New")), wxEmptyString, wxITEM_NORMAL);
	FileMenu->Append(FileNewMenu);


	wxMenuItem* FileLoadMenu;
	FileLoadMenu = new wxMenuItem(FileMenu, wxID_ANY, wxString(wxT("Load")), wxEmptyString, wxITEM_NORMAL);
	FileMenu->Append(FileLoadMenu);
	 
	wxMenuItem* FileSaveMenu;
	FileSaveMenu = new wxMenuItem(FileMenu, wxID_ANY, wxString(wxT("Save")), wxEmptyString, wxITEM_NORMAL);
	FileMenu->Append(FileSaveMenu);

	FileMenu->AppendSeparator();

	wxMenuItem* FileExitMenu;
	FileExitMenu = new wxMenuItem(FileMenu, wxID_ANY, wxString(wxT("Exit")), wxEmptyString, wxITEM_NORMAL);
	FileMenu->Append(FileExitMenu);

	FileMenu->AppendSeparator();

	menuBar->Append(FileMenu, wxT("File"));

	DoMenu = new wxMenu();
	wxMenuItem* InsertMenu;
	InsertMenu = new wxMenuItem(DoMenu, wxID_ANY, wxString(wxT("Insert")), wxEmptyString, wxITEM_NORMAL);
	DoMenu->Append(InsertMenu);

	wxMenuItem* ChangeMenu;
	ChangeMenu = new wxMenuItem(DoMenu, wxID_ANY, wxString(wxT("Change")), wxEmptyString, wxITEM_NORMAL);
	DoMenu->Append(ChangeMenu);

	wxMenuItem* RemoveMenu;
	RemoveMenu = new wxMenuItem(DoMenu, wxID_ANY, wxString(wxT("Remove")), wxEmptyString, wxITEM_NORMAL);
	DoMenu->Append(RemoveMenu);

	menuBar->Append(DoMenu, wxT("Do"));

	//wxMenuItem* CodeViewMenu;
	//CodeViewMenu = new wxMenuItem(ViewMenu, wxID_ANY, wxString(wxT("CodeView")), wxEmptyString, wxITEM_NORMAL);
	//ViewMenu->Append(CodeViewMenu);


	WindowMenu = new wxMenu();
	wxMenuItem* OtherWindowMenu;
	OtherWindowMenu = new wxMenuItem(WindowMenu, wxID_ANY, wxString(wxT("OtherWindow")), wxEmptyString, wxITEM_NORMAL);
	WindowMenu->Append(OtherWindowMenu);


	wxMenuItem* langMenu;
	langMenu = new wxMenuItem(WindowMenu, wxID_ANY, wxString(wxT("Lang")), wxEmptyString, wxITEM_NORMAL);
	WindowMenu->Append(langMenu);

	wxMenuItem* searchMenu;
	searchMenu = new wxMenuItem(WindowMenu, wxID_ANY, wxString(wxT("SearchWindow")), wxEmptyString, wxITEM_NORMAL);
	WindowMenu->Append(searchMenu);


	menuBar->Append(WindowMenu, wxT("Window"));


	this->SetMenuBar(menuBar);

	wxBoxSizer* bSizer;
	bSizer = new wxBoxSizer(wxVERTICAL);

	wxBoxSizer* bSizer2;
	bSizer2 = new wxBoxSizer(wxHORIZONTAL);

	back_button = new wxButton(this, wxID_ANY, wxT("▲"), wxDefaultPosition, wxDefaultSize, 0);
	back_button->SetFont(wxFont(15, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxEmptyString));
		
	bSizer2->Add(back_button, 1, wxALL | wxEXPAND, 5);
	
	next = new wxButton(this, wxID_ANY, wxT("▶"), wxDefaultPosition, wxDefaultSize, 0);
	next->SetFont(wxFont(15, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxEmptyString));

	bSizer2->Add(next, 0, wxALL | wxEXPAND, 5);

	before = new wxButton(this, wxID_ANY, wxT("◀"), wxDefaultPosition, wxDefaultSize, 0);
	before->SetFont(wxFont(15, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxEmptyString));
	bSizer2->Add(before, 0, wxALL | wxEXPAND, 5);

	dir_text = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE);
	//dir_text->Enable(false);

	bSizer2->Add(dir_text, 13, wxALL | wxEXPAND, 5);

	refresh_button = new wxButton(this, wxID_ANY, wxT("Refresh"), wxDefaultPosition, wxDefaultSize, 0);
	bSizer2->Add(refresh_button, 1, wxALL | wxEXPAND, 5);


	bSizer->Add(bSizer2, 1, wxEXPAND, 5);

	wxBoxSizer* bSizer3;
	bSizer3 = new wxBoxSizer(wxHORIZONTAL);

	m_dataViewListCtrl1 = new wxDataViewListCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxDV_HORIZ_RULES | wxDV_ROW_LINES | wxDV_SINGLE);
	bSizer3->Add(m_dataViewListCtrl1, 1, wxALL | wxEXPAND, 5);

	m_dataViewListCtrl2 = new wxDataViewListCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxDV_HORIZ_RULES | wxDV_ROW_LINES | wxDV_SINGLE);
	bSizer3->Add(m_dataViewListCtrl2, 1, wxALL | wxEXPAND, 5);

	m_dataViewListCtrl3 = new wxDataViewListCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxDV_HORIZ_RULES | wxDV_ROW_LINES | wxDV_SINGLE);
	bSizer3->Add(m_dataViewListCtrl3, 1, wxALL | wxEXPAND, 5);

	m_dataViewListCtrl4 = new wxDataViewListCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxDV_HORIZ_RULES | wxDV_ROW_LINES | wxDV_SINGLE);
	bSizer3->Add(m_dataViewListCtrl4, 1, wxALL | wxEXPAND, 5);

	wxBoxSizer* bSizer6;
	bSizer6 = new wxBoxSizer(wxVERTICAL);

	textCtrl = new wxTextCtrl(this, wxID_ANY, wxT(""), wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE);

	bSizer6->Add(textCtrl, 9, wxEXPAND | wxALL, 5);

	bSizer3->Add(bSizer6, 2, wxEXPAND, 5);


	bSizer->Add(bSizer3, 12, wxEXPAND, 5);


	m_dataViewListCtrl1->AppendTextColumn(wxT("name"));
	m_dataViewListCtrl1->AppendTextColumn(wxT("value"));

	m_dataViewListCtrl2->AppendTextColumn(wxT("name"));
	m_dataViewListCtrl2->AppendTextColumn(wxT("value"));

	m_dataViewListCtrl3->AppendTextColumn(wxT("name"));
	m_dataViewListCtrl3->AppendTextColumn(wxT("value"));

	m_dataViewListCtrl4->AppendTextColumn(wxT("name"));
	m_dataViewListCtrl4->AppendTextColumn(wxT("value"));

	m_statusBar1 = this->CreateStatusBar(1, wxST_SIZEGRIP, wxID_ANY);

	m_statusBar1->SetLabelText(wxT(""));

	wxBoxSizer* bSizer5;
	bSizer5 = new wxBoxSizer(wxHORIZONTAL);

	m_code_run_result = new wxTextCtrl(this, wxID_ANY, wxT(""), wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE);
	m_code_run_result->Enable(false);

	m_code_run_result->ChangeValue(wxT("UTF-8 encoding."));

	bSizer5->Add(m_code_run_result, 7, wxALL, 5);

	m_now_view_text = new wxStaticText(this, wxID_ANY, wxT("View Mode A"), wxDefaultPosition, wxDefaultSize, 0);
	m_now_view_text->Wrap(-1);
	bSizer5->Add(m_now_view_text, 1, wxALL, 5);


	bSizer->Add(bSizer5, 0, wxEXPAND, 5);


	this->SetSizer(bSizer);
	this->Layout();

	this->Centre(wxBOTH);

	// Connect Events

	next->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(MainFrame::nextOnButtonClick), NULL, this);

	before->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(MainFrame::beforeOnButtonClick), NULL, this);


	this->Connect(FileNewMenu->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainFrame::FileNewMenuOnMenuSelection));
	this->Connect(FileLoadMenu->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainFrame::FileLoadMenuOnMenuSelection));
	this->Connect(FileSaveMenu->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainFrame::FileSaveMenuOnMenuSelection));
	this->Connect(FileExitMenu->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainFrame::FileExitMenuOnMenuSelection));
	this->Connect(InsertMenu->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainFrame::InsertMenuOnMenuSelection));
	this->Connect(ChangeMenu->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainFrame::ChangeMenuOnMenuSelection));
	this->Connect(RemoveMenu->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainFrame::RemoveMenuOnMenuSelection));

	back_button->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(MainFrame::back_buttonOnButtonClick), NULL, this);

	refresh_button->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(MainFrame::refresh_buttonOnButtonClick), NULL, this);


	m_dataViewListCtrl1->Connect(wxEVT_SIZE, wxSizeEventHandler(MainFrame::m_dataViewListCtrl1OnSize), NULL, this);
	m_dataViewListCtrl2->Connect(wxEVT_SIZE, wxSizeEventHandler(MainFrame::m_dataViewListCtrl2OnSize), NULL, this);
	m_dataViewListCtrl3->Connect(wxEVT_SIZE, wxSizeEventHandler(MainFrame::m_dataViewListCtrl3OnSize), NULL, this);
	m_dataViewListCtrl4->Connect(wxEVT_SIZE, wxSizeEventHandler(MainFrame::m_dataViewListCtrl4OnSize), NULL, this);

	m_dataViewListCtrl1->Connect(wxEVT_CHAR, wxKeyEventHandler(MainFrame::m_dataViewListCtrl1OnChar), NULL, this);
	m_dataViewListCtrl2->Connect(wxEVT_CHAR, wxKeyEventHandler(MainFrame::m_dataViewListCtrl2OnChar), NULL, this);
	m_dataViewListCtrl3->Connect(wxEVT_CHAR, wxKeyEventHandler(MainFrame::m_dataViewListCtrl3OnChar), NULL, this);
	m_dataViewListCtrl4->Connect(wxEVT_CHAR, wxKeyEventHandler(MainFrame::m_dataViewListCtrl4OnChar), NULL, this);

	textCtrl->Connect(wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler(MainFrame::m_textCtrlOnText), NULL, this);
	dir_text->Connect(wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler(MainFrame::m_textCtrl2OnText), NULL, this);

	text_ctrl_backup.Clear();
	dir_vec_backup.clear();
	dir_text->Clear();
	// click 
	

	// double click
	m_dataViewListCtrl1->Connect(wxEVT_COMMAND_DATAVIEW_ITEM_ACTIVATED, wxDataViewEventHandler(MainFrame::m_dataViewListCtrl1OnDataViewListCtrlItemActivated), NULL, this);
	m_dataViewListCtrl2->Connect(wxEVT_COMMAND_DATAVIEW_ITEM_ACTIVATED, wxDataViewEventHandler(MainFrame::m_dataViewListCtrl2OnDataViewListCtrlItemActivated), NULL, this);
	m_dataViewListCtrl3->Connect(wxEVT_COMMAND_DATAVIEW_ITEM_ACTIVATED, wxDataViewEventHandler(MainFrame::m_dataViewListCtrl3OnDataViewListCtrlItemActivated), NULL, this);
	m_dataViewListCtrl4->Connect(wxEVT_COMMAND_DATAVIEW_ITEM_ACTIVATED, wxDataViewEventHandler(MainFrame::m_dataViewListCtrl4OnDataViewListCtrlItemActivated), NULL, this);

	// right click
	m_dataViewListCtrl1->Connect(wxEVT_COMMAND_DATAVIEW_ITEM_CONTEXT_MENU, wxDataViewEventHandler(MainFrame::m_dataViewListCtrl1OnDataViewListCtrlItemContextMenu), NULL, this);
	m_dataViewListCtrl2->Connect(wxEVT_COMMAND_DATAVIEW_ITEM_CONTEXT_MENU, wxDataViewEventHandler(MainFrame::m_dataViewListCtrl2OnDataViewListCtrlItemContextMenu), NULL, this);
	m_dataViewListCtrl3->Connect(wxEVT_COMMAND_DATAVIEW_ITEM_CONTEXT_MENU, wxDataViewEventHandler(MainFrame::m_dataViewListCtrl3OnDataViewListCtrlItemContextMenu), NULL, this);
	m_dataViewListCtrl4->Connect(wxEVT_COMMAND_DATAVIEW_ITEM_CONTEXT_MENU, wxDataViewEventHandler(MainFrame::m_dataViewListCtrl4OnDataViewListCtrlItemContextMenu), NULL, this);

	m_dataViewListCtrl1->Connect(wxEVT_COMMAND_DATAVIEW_SELECTION_CHANGED, wxDataViewEventHandler(MainFrame::m_dataViewListCtrl1OnDataViewListCtrlSelectionchanged), NULL, this);
	m_dataViewListCtrl2->Connect(wxEVT_COMMAND_DATAVIEW_SELECTION_CHANGED, wxDataViewEventHandler(MainFrame::m_dataViewListCtrl2OnDataViewListCtrlSelectionchanged), NULL, this);
	m_dataViewListCtrl3->Connect(wxEVT_COMMAND_DATAVIEW_SELECTION_CHANGED, wxDataViewEventHandler(MainFrame::m_dataViewListCtrl3OnDataViewListCtrlSelectionchanged), NULL, this);
	m_dataViewListCtrl4->Connect(wxEVT_COMMAND_DATAVIEW_SELECTION_CHANGED, wxDataViewEventHandler(MainFrame::m_dataViewListCtrl4OnDataViewListCtrlSelectionchanged), NULL, this);


	this->Connect(OtherWindowMenu->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainFrame::OtherWindowMenuOnMenuSelection));
	this->Connect(langMenu->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainFrame::LangMenuOnMenuSelection));
	this->Connect(searchMenu->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainFrame::SearchWindowMenuSelection));
	//this->Connect(CodeViewMenu->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainFrame::CodeViewMenuOnMenuSelection));
}

MainFrame::~MainFrame()
{
	if (isMain) {
		global.remove();
		changed.remove();
	}
	// Disconnect Events
	this->Disconnect(wxID_ANY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainFrame::FileNewMenuOnMenuSelection));
	this->Disconnect(wxID_ANY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainFrame::FileLoadMenuOnMenuSelection));
	this->Disconnect(wxID_ANY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainFrame::FileSaveMenuOnMenuSelection));
	this->Disconnect(wxID_ANY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainFrame::FileExitMenuOnMenuSelection));
	this->Disconnect(wxID_ANY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainFrame::InsertMenuOnMenuSelection));
	this->Disconnect(wxID_ANY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainFrame::ChangeMenuOnMenuSelection));
	this->Disconnect(wxID_ANY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainFrame::RemoveMenuOnMenuSelection));

	back_button->Disconnect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(MainFrame::back_buttonOnButtonClick), NULL, this);
	refresh_button->Disconnect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(MainFrame::refresh_buttonOnButtonClick), NULL, this);


	m_dataViewListCtrl1->Disconnect(wxEVT_SIZE, wxSizeEventHandler(MainFrame::m_dataViewListCtrl1OnSize), NULL, this);
	m_dataViewListCtrl2->Disconnect(wxEVT_SIZE, wxSizeEventHandler(MainFrame::m_dataViewListCtrl2OnSize), NULL, this);
	m_dataViewListCtrl3->Disconnect(wxEVT_SIZE, wxSizeEventHandler(MainFrame::m_dataViewListCtrl3OnSize), NULL, this);
	m_dataViewListCtrl4->Disconnect(wxEVT_SIZE, wxSizeEventHandler(MainFrame::m_dataViewListCtrl4OnSize), NULL, this);


	m_dataViewListCtrl1->Disconnect(wxEVT_CHAR, wxKeyEventHandler(MainFrame::m_dataViewListCtrl1OnChar), NULL, this);
	m_dataViewListCtrl2->Disconnect(wxEVT_CHAR, wxKeyEventHandler(MainFrame::m_dataViewListCtrl2OnChar), NULL, this);
	m_dataViewListCtrl3->Disconnect(wxEVT_CHAR, wxKeyEventHandler(MainFrame::m_dataViewListCtrl3OnChar), NULL, this);
	m_dataViewListCtrl4->Disconnect(wxEVT_CHAR, wxKeyEventHandler(MainFrame::m_dataViewListCtrl4OnChar), NULL, this);

	m_dataViewListCtrl1->Disconnect(wxEVT_COMMAND_DATAVIEW_ITEM_ACTIVATED, wxDataViewEventHandler(MainFrame::m_dataViewListCtrl1OnDataViewListCtrlItemActivated), NULL, this);
	m_dataViewListCtrl2->Disconnect(wxEVT_COMMAND_DATAVIEW_ITEM_ACTIVATED, wxDataViewEventHandler(MainFrame::m_dataViewListCtrl2OnDataViewListCtrlItemActivated), NULL, this);
	m_dataViewListCtrl3->Disconnect(wxEVT_COMMAND_DATAVIEW_ITEM_ACTIVATED, wxDataViewEventHandler(MainFrame::m_dataViewListCtrl3OnDataViewListCtrlItemActivated), NULL, this);
	m_dataViewListCtrl4->Disconnect(wxEVT_COMMAND_DATAVIEW_ITEM_ACTIVATED, wxDataViewEventHandler(MainFrame::m_dataViewListCtrl4OnDataViewListCtrlItemActivated), NULL, this);

	m_dataViewListCtrl1->Disconnect(wxEVT_COMMAND_DATAVIEW_ITEM_CONTEXT_MENU, wxDataViewEventHandler(MainFrame::m_dataViewListCtrl1OnDataViewListCtrlItemContextMenu), NULL, this);
	m_dataViewListCtrl2->Disconnect(wxEVT_COMMAND_DATAVIEW_ITEM_CONTEXT_MENU, wxDataViewEventHandler(MainFrame::m_dataViewListCtrl2OnDataViewListCtrlItemContextMenu), NULL, this);
	m_dataViewListCtrl3->Disconnect(wxEVT_COMMAND_DATAVIEW_ITEM_CONTEXT_MENU, wxDataViewEventHandler(MainFrame::m_dataViewListCtrl3OnDataViewListCtrlItemContextMenu), NULL, this);
	m_dataViewListCtrl4->Disconnect(wxEVT_COMMAND_DATAVIEW_ITEM_CONTEXT_MENU, wxDataViewEventHandler(MainFrame::m_dataViewListCtrl4OnDataViewListCtrlItemContextMenu), NULL, this);


	m_dataViewListCtrl1->Disconnect(wxEVT_COMMAND_DATAVIEW_SELECTION_CHANGED, wxDataViewEventHandler(MainFrame::m_dataViewListCtrl1OnDataViewListCtrlSelectionchanged), NULL, this);
	m_dataViewListCtrl2->Disconnect(wxEVT_COMMAND_DATAVIEW_SELECTION_CHANGED, wxDataViewEventHandler(MainFrame::m_dataViewListCtrl2OnDataViewListCtrlSelectionchanged), NULL, this);
	m_dataViewListCtrl3->Disconnect(wxEVT_COMMAND_DATAVIEW_SELECTION_CHANGED, wxDataViewEventHandler(MainFrame::m_dataViewListCtrl3OnDataViewListCtrlSelectionchanged), NULL, this);
	m_dataViewListCtrl4->Disconnect(wxEVT_COMMAND_DATAVIEW_SELECTION_CHANGED, wxDataViewEventHandler(MainFrame::m_dataViewListCtrl4OnDataViewListCtrlSelectionchanged), NULL, this);
	
	this->Disconnect(wxID_ANY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainFrame::OtherWindowMenuOnMenuSelection));
	this->Disconnect(wxID_ANY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainFrame::LangMenuOnMenuSelection));

	//m_code_run_button->Disconnect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(MainFrame::m_code_run_buttonOnButtonClick), NULL, this);
	
	next->Disconnect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(MainFrame::nextOnButtonClick), NULL, this);
	before->Disconnect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(MainFrame::beforeOnButtonClick), NULL, this);

	//this->Disconnect(wxID_ANY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainFrame::CodeViewMenuOnMenuSelection));


	textCtrl->Disconnect(wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler(MainFrame::m_textCtrlOnText), NULL, this);
	dir_text->Disconnect(wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler(MainFrame::m_textCtrl2OnText), NULL, this);
}

class TestApp : public wxApp {
public:
	virtual bool OnInit() {
		MainFrame* frame = new MainFrame(nullptr);
		frame->FirstFrame();
		frame->Show(true);
		return true;
	}
};

IMPLEMENT_APP(TestApp)


