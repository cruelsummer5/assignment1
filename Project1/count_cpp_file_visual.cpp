#include <wx/wx.h>
#include <fstream>
#include <filesystem>
#include <vector>
#include <string>
#include <chrono>
#include <algorithm>

using namespace std;
namespace fs = std::experimental::filesystem;

struct DirCxxBreif
{
	DirCxxBreif() :
		fileCount(0),
		lineCount(0),
		emptyLineCount(0),
		headerFileCount(0),
		sourceFileCount(0)
	{}
	int fileCount;
	int lineCount;
	int emptyLineCount;
	int headerFileCount;
	int sourceFileCount;
};

pair<int, int> countLines(const string& filename)
{
	ifstream file(filename);
	int count = 0;
	int emptyCount = 0;
	string line;
	while (getline(file, line)) {
		++count;
		if (line.empty()) ++emptyCount;
	}

	return { count, emptyCount };
}

bool isCppHeaderFile(const fs::directory_entry& entry)
{
	return entry.path().extension() == ".h" || entry.path().extension() == ".hpp" || entry.path().extension() == ".hxx";
}

bool isCppSourceFile(const fs::directory_entry& entry)
{
	return entry.path().extension() == ".cc" || entry.path().extension() == ".cpp" || entry.path().extension() == ".cxx" || entry.path().extension() == ".c++";
}

class MyFrame : public wxFrame
{
public:
	MyFrame(const wxString& title, const wxPoint& pos, const wxSize& size);

private:
	void OnExit(wxCommandEvent& event);
	void OnSelect(wxCommandEvent& event);
	void countCppFiles(const fs::path& dir, DirCxxBreif& dcb);

	wxButton* m_selectBtn;
	wxTextCtrl* m_outputCtrl;

	wxDECLARE_EVENT_TABLE();
};

enum
{
	ID_Quit = 1,
	ID_Select
};

wxBEGIN_EVENT_TABLE(MyFrame, wxFrame)
EVT_MENU(ID_Quit, MyFrame::OnExit)
EVT_BUTTON(ID_Select, MyFrame::OnSelect)
wxEND_EVENT_TABLE()

MyFrame::MyFrame(const wxString& title, const wxPoint& pos, const wxSize& size)
	: wxFrame(NULL, wxID_ANY, title, pos, size)
{
	wxMenu* menuFile = new wxMenu;
	menuFile->Append(ID_Quit, "&Exit\tAlt-X",
		"Quit this program");

	wxMenuBar* menuBar = new wxMenuBar;
	menuBar->Append(menuFile, "&File");

	SetMenuBar(menuBar);

	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);

	m_selectBtn = new wxButton(this, ID_Select, "Select Directory");
	sizer->Add(m_selectBtn, wxSizerFlags().Border(wxALL, 10));

	m_outputCtrl = new wxTextCtrl(this, wxID_ANY, wxEmptyString,
		wxDefaultPosition, wxDefaultSize,
		wxTE_MULTILINE | wxTE_READONLY | wxHSCROLL);
	sizer->Add(m_outputCtrl, wxSizerFlags(1).Expand().Border(wxALL, 10));

	SetSizer(sizer);
	SetAutoLayout(true);
}

void MyFrame::OnExit(wxCommandEvent& event)
{
	Close(true);
}

void MyFrame::OnSelect(wxCommandEvent& event)
{
	wxDirDialog dlg(this, "Select Directory", wxEmptyString, wxDD_DEFAULT_STYLE | wxDD_DIR_MUST_EXIST);
	if (dlg.ShowModal() == wxID_OK) {
		m_outputCtrl->Clear();
		wxString path = dlg.GetPath();
		DirCxxBreif dcb;

		auto start_time = chrono::high_resolution_clock::now();
		countCppFiles(string(path.mb_str()), dcb);
		auto end_time = chrono::high_resolution_clock::now();

		auto duration = chrono::duration_cast<chrono::milliseconds>(end_time - start_time);

		wxString inform;
		inform << "\n\nelapsed time: " << duration.count() << "ms\n"
				<< dcb.fileCount << " C++ files, " << dcb.lineCount << " lines, "
				<< dcb.emptyLineCount << " empty lines, " << dcb.headerFileCount
				<< " header files, " << dcb.sourceFileCount << " source files\n";

		m_outputCtrl->AppendText(inform);
	}
}

void MyFrame::countCppFiles(const fs::path& dir, DirCxxBreif& dcb)
{
	for (const auto& entry : fs::directory_iterator(dir))
	{
		bool isCppFile = false;
		if (fs::is_directory(entry)) countCppFiles(entry.path(), dcb);
		else if (isCppHeaderFile(entry))
		{
			++dcb.headerFileCount;
			isCppFile = true;
		}
		else if (isCppSourceFile(entry))
		{
			++dcb.sourceFileCount;
			isCppFile = true;
		}

		if (isCppFile)
		{
			dcb.fileCount++;

			auto pr = countLines(entry.path().string());
			dcb.lineCount += pr.first;
			dcb.emptyLineCount += pr.second;
			wxString fileStatus = entry.path().string() + ": " + to_string(pr.first) 
									+ "lines, " +  to_string(pr.second) + " empty lines\n";

			m_outputCtrl->AppendText(fileStatus);
		}
	}
}

class MyApp : public wxApp
{
public:
	virtual bool OnInit();
};

bool MyApp::OnInit()
{
	MyFrame* frame = new MyFrame("Count Cpp File Window", wxPoint(50, 50), wxSize(550, 540));
	frame->Show(true);
	return true;
}

wxIMPLEMENT_APP(MyApp);