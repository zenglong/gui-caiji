#ifndef _ZL_PROGRESS_EVENT_H_
#define _ZL_PROGRESS_EVENT_H_

#include <wx/event.h>

DECLARE_EVENT_TYPE(wxEVT_MY_PROGRESS_EVENT,-1);

class wxMyProgressEvent : public wxCommandEvent
{
    public:
        wxMyProgressEvent(wxEventType cmdType = wxEVT_NULL,int id=0,int processPos=0,int processNum=1,int total_processPos=0,int total_processNum=1)
            :wxCommandEvent(cmdType,id)
			{ m_processPos = processPos; m_processNum = processNum; m_total_processPos = total_processPos; m_total_processNum = total_processNum;}

        wxMyProgressEvent(const wxMyProgressEvent& event)
            :wxCommandEvent(event)
            {
				m_processPos = event.m_processPos;
				m_processNum = event.m_processNum;
				m_total_processPos = event.m_total_processPos;
				m_total_processNum = event.m_total_processNum;
			}

        virtual wxEvent* Clone() const { return new wxMyProgressEvent(*this);}

    public:
        void SetEventNum(int processPos=0,int processNum=1,int total_processPos=0,int total_processNum=1) 
				{ m_processPos = processPos; m_processNum = processNum; m_total_processPos = total_processPos; m_total_processNum = total_processNum;}

    public:
		int m_processPos;
        int m_processNum;
		int m_total_processPos;
		int m_total_processNum;
};

//定义事件回调方法指针
typedef void (wxEvtHandler::*wxMyProgressEventFunction)(wxMyProgressEvent&);

#define EVT_MY_PROGRESS_EVENT(id, fn)                                            \
	DECLARE_EVENT_TABLE_ENTRY( wxEVT_MY_PROGRESS_EVENT, id, wxID_ANY,  \
		(wxObjectEventFunction)(wxEventFunction)                     \
		(wxCommandEventFunction) wxStaticCastEvent(                  \
		wxMyProgressEventFunction, &fn ), (wxObject*) NULL ),


#endif //_ZL_PROGRESS_EVENT_H_
