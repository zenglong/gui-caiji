#ifndef   _ZL_GLOBAL_H_
#define   _ZL_GLOBAL_H_

#include "wx/wx.h"
#include "wx/aui/aui.h"
#include "wx/treectrl.h"
#include <curl/curl.h>
#include <wx/sstream.h>
#include <stdio.h>
#include <string.h>
#include <wx/thread.h>
#include <wx/artprov.h>
#include <wx/regex.h>
#include <wx/xml/xml.h> 
#include <wx/dialog.h>
#include <wx/listbox.h>
#include "wx/wxsqlite3.h"
#include "wx/filename.h"
#include "stdlib.h"
#include "stdio.h"
#include <wx/richtext/richtextctrl.h>
#include <wx/hashmap.h>
#include "logEvent.h"
#include "ProgressEvent.h"
#include "MyHashString.h"
#include "MyTaskBarIcon.h"
#include <wx/grid.h>
#include <wx/fdrepdlg.h>
#include <wx/textdlg.h>
#include "caiji.h"
#include "time.h"
#include "BASE64_API.h"

#define DEBUG_INPUT_MAX 50
#define STRNULL '\0'

void InitHashMap();
void global_GetUrlContent(wxString remote_url);
//wxString TrimBoth( IN const wxString& str );
bool CheckTitleMust(wxString title , wxString must);
void delScript(wxString * str);
wxString GetAreaID(wxString strArg);
wxString GetAreaID_NoRandom(wxString strArg);
wxString GetAreaName_NoRandom(wxString strArg,wxString defaultRet);
int GetRandomNum();
bool global_checkTitleRepeat(wxString baseurl,wxString title,wxString catid);
char *global_rand_str(char *str,const int len);
/*myWinTools.cppÀïµÄº¯Êý*/
int myGetLenWC2GBK(wchar_t * ws);
int myConvWC2GBK(wchar_t * ws,char * buf,int buf_size);
int myGetFileSize(char * strFileName);

#endif //_ZL_GLOBAL_H_