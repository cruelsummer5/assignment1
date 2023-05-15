#include <wx/wx.h>
#include <fstream>
#include <filesystem>
#include <deque>
#include <string>
#include <chrono>
#include <algorithm>
#include <wx/event.h>


using namespace std;
namespace fs = std::experimental::filesystem;

static constexpr int TIMER_INTERVAL = 30;
static constexpr int BATCH_MAXN = 10;

// ----------------------------------------------------------------------------
// helper functions which calculate cpp files used by MyThread::Entry()
// ----------------------------------------------------------------------------

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

// ----------------------------------------------------------------------------
// a thread which calculate cpp files and executes GUI calls using wxQueueEvent
// ----------------------------------------------------------------------------

// declare a new type of event, to be used by our MyThread class:
//wxDECLARE_EVENT(wxEVT_COMMAND_MYTHREAD_COMPLETED, wxThreadEvent);
//wxDECLARE_EVENT(wxEVT_COMMAND_MYTHREAD_UPDATE, wxThreadEvent);


class MyFrame;

class MyThread : public wxThread
{
public:
	MyThread(MyFrame *handler, const wxString& path)
		: wxThread(wxTHREAD_JOINABLE),
		  _pHandler(handler),
	      _path(path)
	{}
	~MyThread();

protected:
	virtual ExitCode Entry();
	void countCppFiles(const fs::path& dir, DirCxxBreif& dcb, int& cnt);

private:
	MyFrame *_pHandler;

	std::vector<wxString> _fileMsg; //vector which stores messages to be printed on screen
	wxCriticalSection _fileMsgCS;    // protects the _fileMsg

	wxString _totalMsg;
	wxCriticalSection _totalMsgCS;    // protects the _fileMsg

	wxString _path;

	friend class MyFrame;
};


// ----------------------------------------------------------------------------
// the main application frame
// ----------------------------------------------------------------------------

class MyFrame : public wxFrame
{
public:
	MyFrame(const wxString& title, const wxPoint& pos, const wxSize& size);

private:
	void DoStartThread(const wxString& path);
	void OnExit(wxCommandEvent& event);
	void OnSelect(wxCommandEvent& event);

	void OnThreadUpdate(wxCommandEvent&);
	void OnThreadCompletion(wxCommandEvent&);
	void showData(const vector<wxString>& fileMsg);

	void OnTimer(wxTimerEvent& event);

	MyThread* _pThread;

	wxButton* m_selectBtn;
	wxTextCtrl* m_outputCtrl;

	wxTimer* _timer;

	friend class MyThread;

	wxDECLARE_EVENT_TABLE();
};

enum
{
	ID_Quit = 1,
	ID_Select,
	TIMER_ID
};

wxBEGIN_EVENT_TABLE(MyFrame, wxFrame)
EVT_MENU(ID_Quit, MyFrame::OnExit)
EVT_BUTTON(ID_Select, MyFrame::OnSelect)
EVT_TIMER(TIMER_ID, MyFrame::OnTimer)
wxEND_EVENT_TABLE()

//wxDEFINE_EVENT(wxEVT_COMMAND_MYTHREAD_COMPLETED, wxThreadEvent);
//wxDEFINE_EVENT(wxEVT_COMMAND_MYTHREAD_UPDATE, wxThreadEvent);

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
	if (_pThread) 
	{
		_pThread->Wait();
		delete _pThread;
	}
	Close(true);
}

void MyFrame::DoStartThread(const wxString& path)
{
	_pThread = new MyThread(this, path);

	if (_pThread->Run() != wxTHREAD_NO_ERROR)
	{
		wxLogError("Can't create the thread!");
		delete _pThread;
		_pThread = NULL;
	}

	// after the call to wxThread::Run(), the m_pThread pointer is "unsafe":
	// at any moment the thread may cease to exist (because it completes its work).
	// To avoid dangling pointers OnThreadExit() will set m_pThread
	// to NULL when the thread dies.
}

void MyFrame::OnSelect(wxCommandEvent& event)
{
	wxDirDialog dlg(this, "Select Directory", wxEmptyString, wxDD_DEFAULT_STYLE | wxDD_DIR_MUST_EXIST);
	if (!_pThread && dlg.ShowModal() == wxID_OK) {
		m_outputCtrl->Clear();
		wxString path = dlg.GetPath();

		DoStartThread(path);

		//创建定时器,指定定时器ID使用哪个定时器
		_timer = new wxTimer(this, TIMER_ID);
		//启动定时器，参数是定时的时间间隔
		_timer->Start(TIMER_INTERVAL);
	}
}

void MyFrame::OnThreadUpdate(wxCommandEvent&)
{
	/*
	vector<wxString> fileMsg;
	{
		wxCriticalSectionLocker enter(_pThread->_fileMsgCS);
		swap(fileMsg, _pThread->_fileMsg);
	}
	CallAfter(&MyFrame::showData, fileMsg);
	*/
}

void MyFrame::showData(const vector<wxString>& fileMsg)
{
	for (int i = 0; i < fileMsg.size(); i++)
	{
		m_outputCtrl->AppendText(fileMsg[i]);
	}
}

void MyFrame::OnThreadCompletion(wxCommandEvent&)
{
	/*
	_pThread->Wait();
	delete _pThread;
	_pThread = nullptr;
	wxMessageOutputDebug().Printf("MYFRAME: MyThread exited!\n");
	*/
}

void MyFrame::OnTimer(wxTimerEvent&)
{
	vector<wxString> tmp;
	{
		wxCriticalSectionLocker enter(_pThread->_fileMsgCS);
		auto& fileMsg = _pThread->_fileMsg;
		if (fileMsg.size() == 0) {
			wxCriticalSectionLocker enter(_pThread->_totalMsgCS);
			if (!_pThread->_totalMsg.empty())
			{
				m_outputCtrl->AppendText(_pThread->_totalMsg);
				_timer->Stop();
				_pThread->Wait();
				delete _pThread;
				_pThread = nullptr;
			}
		} 
		else if (fileMsg.size() <= BATCH_MAXN) 
		{
			swap(tmp, fileMsg);
		}
		else
		{
			tmp = vector<wxString>(fileMsg.end() - BATCH_MAXN, fileMsg.end());
			fileMsg.erase(fileMsg.end() - BATCH_MAXN, fileMsg.end());
		}
	}

	for (int i = 0; i < tmp.size(); i++) m_outputCtrl->AppendText(tmp[i]);
}

wxThread::ExitCode MyThread::Entry()
{
	if (!TestDestroy())
	{
		DirCxxBreif dcb;
		int cnt = 0;

		auto start_time = chrono::high_resolution_clock::now();
		countCppFiles(string(_path.mb_str()), dcb, cnt);
		auto end_time = chrono::high_resolution_clock::now();

		auto duration = chrono::duration_cast<chrono::milliseconds>(end_time - start_time);

		wxCriticalSectionLocker enter(_totalMsgCS);
		_totalMsg << "\n\nelapsed time: " << duration.count() << "ms\n"
			<< dcb.fileCount << " C++ files, " << dcb.lineCount << " lines, "
			<< dcb.emptyLineCount << " empty lines, " << dcb.headerFileCount
			<< " header files, " << dcb.sourceFileCount << " source files\n";

		//wxQueueEvent(_pHandler, new wxThreadEvent(wxEVT_COMMAND_MYTHREAD_UPDATE));
	}


	// signal the event handler that this thread is going to be destroyed
	// NOTE: here we assume that using the m_pHandler pointer is safe,
	//       (in this case this is assured by the MyFrame destructor)
	//wxQueueEvent(_pHandler, new wxThreadEvent(wxEVT_COMMAND_MYTHREAD_COMPLETED));

	return (wxThread::ExitCode)0;     // success
}

void MyThread::countCppFiles(const fs::path& dir, DirCxxBreif& dcb, int& cnt)
{

	for (const auto& entry : fs::directory_iterator(dir))
	{
		bool isCppFile = false;
		if (fs::is_directory(entry)) countCppFiles(entry.path(), dcb, cnt);
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
				+ "lines, " + to_string(pr.second) + " empty lines\n";

			{
				wxCriticalSectionLocker enter(_fileMsgCS);
				_fileMsg.push_back(fileStatus);
			}
		}
	}
}

MyThread::~MyThread()
{
}


// ----------------------------------------------------------------------------
// the application object
// ----------------------------------------------------------------------------

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