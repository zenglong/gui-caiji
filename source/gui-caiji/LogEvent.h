#ifndef _ZL_LOG_EVENT_H_
#define _ZL_LOG_EVENT_H_

#include <wx/event.h>

DECLARE_EVENT_TYPE(wxEVT_MY_LOG_EVENT,-1);

class wxMyLogEvent : public wxCommandEvent
{
    public:
        wxMyLogEvent(wxEventType cmdType = wxEVT_NULL,int id=0,wxString msg="",int color=0)
            :wxCommandEvent(cmdType,id),
			m_strEventMsg(msg),
			m_msgcolorEnum(color)
            {}

        wxMyLogEvent(const wxMyLogEvent& event)
            :wxCommandEvent(event)
            {
				m_strEventMsg = event.m_strEventMsg;
				m_msgcolorEnum = event.m_msgcolorEnum;
			}

        virtual wxEvent* Clone() const { return new wxMyLogEvent(*this);}

    public:
        void SetEventMsg(const wxString& msg,int msgcolorEnum) { m_strEventMsg = msg; m_msgcolorEnum = msgcolorEnum;}
        wxString GetEventMsg() const { return m_strEventMsg;}
    public:
        wxString m_strEventMsg;
		int m_msgcolorEnum;
};

//定义事件回调方法指针
typedef void (wxEvtHandler::*wxMyLogEventFunction)(wxMyLogEvent&);

#define EVT_MY_LOG_EVENT(id, fn)                                            \
	DECLARE_EVENT_TABLE_ENTRY( wxEVT_MY_LOG_EVENT, id, wxID_ANY,  \
		(wxObjectEventFunction)(wxEventFunction)                     \
		(wxCommandEventFunction) wxStaticCastEvent(                  \
		wxMyLogEventFunction, &fn ), (wxObject*) NULL ),


#endif //_ZL_LOG_EVENT_H_
