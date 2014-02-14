#ifndef   _ZL_MyTaskBarIcon_H_
#define   _ZL_MyTaskBarIcon_H_

#include "wx/taskbar.h"
#include <shellapi.h>

class wxTaskBarIconWindow: public wxFrame {
    public:
        wxTaskBarIconWindow(wxTaskBarIcon* icon): wxFrame(NULL, wxID_ANY,
            wxEmptyString, wxDefaultPosition, wxDefaultSize, 0), m_icon(icon){}

        WXLRESULT MSWWindowProc(WXUINT msg, WXWPARAM wParam, WXLPARAM lParam) {
                return 0;
        }

    private:
        wxTaskBarIcon* m_icon;
};

class MyTaskBarIcon : public wxTaskBarIcon
{
public:
	static const int ICON_ERROR = NIIF_ERROR;
    static const int ICON_INFO = NIIF_INFO;
    static const int ICON_NONE = NIIF_NONE;
    static const int ICON_WARNING = NIIF_WARNING;

    #if _WIN32_IE>=0x0600
        static const int ICON_NOSOUND = NIIF_NOSOUND; // _WIN32_IE=0x0600
    #endif

#if defined(__WXOSX__) && wxOSX_USE_COCOA
    MyTaskBarIcon(wxTaskBarIconType iconType = DEFAULT_TYPE)
    :   wxTaskBarIcon(iconType)
#else
    MyTaskBarIcon()
#endif
    {}

    void OnLeftButtonClick(wxTaskBarIconEvent&);
    void OnMenuRestore(wxCommandEvent&);
	void OnHideToTray(wxCommandEvent&);
    void OnMenuExit(wxCommandEvent&);
    virtual wxMenu *CreatePopupMenu();

	bool ShowBalloon(wxString title, wxString msg, int iconID = ICON_INFO,
            unsigned int timeout = 3000);

    DECLARE_EVENT_TABLE()
};

enum
{
    PU_RESTORE = 10001,
	PU_HIDE = 10002,
    PU_EXIT
};

#endif //_ZL_MyTaskBarIcon_H_