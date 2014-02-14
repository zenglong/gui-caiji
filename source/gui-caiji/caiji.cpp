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
#include "wx/image.h"
#include "wx/splash.h"
#include "wx/wxsqlite3.h"
#include "wx/filename.h"
#include "stdlib.h"
#include "stdio.h"
#include <wx/richtext/richtextctrl.h>
#include "logEvent.h"
#include "ProgressEvent.h"
#include "MyHashString.h"
#include "MyTaskBarIcon.h"
#include <wx/grid.h>
#include <wx/utils.h>
#include <wx/fdrepdlg.h>
#include <wx/wfstream.h>
#include <wx/config.h>
#include <wx/fileconf.h>
#include "caiji.h"

#define ZL_EXP_OS_IN_WINDOWS //用于告诉zengl嵌入式脚本语言当前的执行环境是windows系统，防止出现编译错误
#include "zengl_exportfuns.h" //测试zengl v1.3.2

DEFINE_EVENT_TYPE(wxEVT_MY_EVENT) //旧的日志事件，已经废弃，现在采用logevent.h中定义的wxEVT_MY_LOG_EVENT
DEFINE_EVENT_TYPE(wxEVT_MY_UPDATECATE) //当获取到网站的分类信息后用于更新treectrl树形控件里的分类列表的事件。
DEFINE_EVENT_TYPE(wxEVT_MY_CHECKS) //当点击分类列表根目录的左侧复选框时，进行反选分类列表。
DEFINE_EVENT_TYPE(wxEVT_MY_PROGRESS) //当从网站获取分类数据时，最底部的进度条事件。
DEFINE_EVENT_TYPE(wxEVT_MY_CLEARLOGS)  //当采集完一个分类时，用于回滚清理richtext日志面板里的内容的事件。
DEFINE_EVENT_TYPE(wxEVT_MY_CAIJI_PROGRESS) //旧的采集时的进度条事件，已经废弃，现在采用progressevent.h里定义的wxEVT_MY_PROGRESS_EVENT事件。
DEFINE_EVENT_TYPE(wxEVT_MY_PAUSE_THREAD) //采集数据到本地时提醒用户处理本地数据的事件。
DEFINE_EVENT_TYPE(wxEVT_MY_DEBUG_PAUSE_THREAD) //调试暂停事件
DEFINE_EVENT_TYPE(wxEVT_MY_SET_CURL_NUM) //设置抓包的数据量

CURL * curl = NULL; //全局的libcurl网络抓包函数需要的指针。
ZL_EXP_VOID * CUR_CAIJI_VM; //当前运行的采集脚本使用的VM虚拟机指针

FILE *fp = NULL ,*logfp=NULL,*zengl_debuglogfile = NULL;  //定义FILE类型指针，fp主要是text.xml，logfp是日志文件mylogs.txt
char * char_myglStr = NULL; //当网络curl获取到数据时，在回调函数中将数据先写入char_myglStr,char_myglStr是一个动态扩容的内存。采用zengl编程语言的内存管理办法
int char_myglTotalnum = 0; //char_myglStr对应的数据的size大小，使用strlen获取到的数据并不准确，容易出现漏字符的情况。
int char_myglTotalnum_forShow = 0;
int charlength = 0;   //char_myglStr字符长度，现在使用的是char_myglTotalnum，这个charlength是用strlen获取到的，是最开始开发调试用的，仅作参考。
int totallogsLength = 0; //日志面板的内容超过一定范围时就进行回滚清理操作，目前有个BUG就是这个值会一直变大，但是暂时没发现使用上的问题，等发现了再做处理。
wxString myglStr=_(""); //char_myglStr里的数据最终会转为wxString类型存放在myglStr里面。
int myglStyle = MY_RICHTEXT_NORMAL_STYLE; //myglStyle日志颜色样式,normal表示黑色普通样式，还有红色警告，绿色样式。
bool glIsStop = false;   //好像是之前开发时用于判断线程是否停止用的，目前没看到使用的地方，应该是被废弃了。
MyFrame *glmainFrame = NULL;  //全局主程序窗口的句柄。
wxSQLite3Database * glDB;  //全局数据库指针，用于和其他采集模块进行数据沟通用的。
wxString glSiteDBPath = "";  //全局网站数据库文件的路径。
wxString glCaijiWebName = ""; //全局网站名称。
bool glIsDialogInShow = false;  //用于判断当前程序是否有弹出的对话框，如果有弹出的对话框，那么在退出时发出警告，防止出现内存溢出的情况。
extern wxString gl_initManage_Url; //设置管理页面的URL 这里为采集上传接口文件名
wxString glEscapeSiteName; //全局的当前网站db文件名
wxString glCompanyCatID; //需要采集的公司分类ID
wxString gl_zenglrun_article_modname = "Module/article.zl"; //资讯模块的规则文件路径,在初始化脚本中已经修改。
wxString gl_zenglrun_brand_modname = "Module/brand.zl"; //品牌模块的规则文件路径,在初始化脚本中已经修改。
wxString gl_zenglrun_buy_modname = "Module/buy.zl"; //求购模块的规则文件路径,在初始化脚本中已经修改。
wxString gl_zenglrun_exhibit_modname = "Module/exhibit.zl"; //展会模块的规则文件路径,在初始化脚本中已经修改。
wxString gl_zenglrun_group_modname = "Module/group.zl"; //团购模块的规则文件路径,在初始化脚本中已经修改。
wxString gl_zenglrun_info_modname = "Module/info.zl"; //招商模块的规则文件路径,在初始化脚本中已经修改。
wxString gl_zenglrun_job_modname = "Module/job.zl"; //人才模块的规则文件路径,在初始化脚本中已经修改。
wxString gl_zenglrun_know_modname = "Module/know.zl"; //知道模块的规则文件路径,在初始化脚本中已经修改。
wxString gl_zenglrun_mall_modname = "Module/mall.zl"; //商城模块的规则文件路径,在初始化脚本中已经修改。
wxString gl_zenglrun_quote_modname = "Module/quote.zl"; //行情模块的规则文件路径,在初始化脚本中已经修改。
wxString gl_zenglrun_sell_modname = "Module/sell.zl"; //供应模块的规则文件路径,在初始化脚本中已经修改。
wxString gl_zenglrun_special_modname = "Module/special.zl"; //专题模块的规则文件路径,在初始化脚本中已经修改。
wxString gl_zenglrun_photo_modname = "Module/photo.zl"; //图库模块的规则文件路径,在初始化脚本中已经修改。
wxString gl_zenglrun_video_modname = "Module/video.zl"; //视频模块的规则文件路径,在初始化脚本中已经修改。
wxString gl_zenglrun_down_modname = "Module/down.zl"; //下载模块的规则文件路径,在初始化脚本中已经修改。
wxString gl_version_number = "v1.3.0"; //版本号信息
//脚本XOR普通异或加密密钥
//wxString gl_encrypt_str = "xingkekn&**&$&^^^#(*@(*#&$&*@&$*0029044*&$*@&$*&$&!!~~!!@))__*#**)#(*#(*(@#BUBUWEBudru!<!`ss`x)&idmmn!vnsme&-2-00/54-b-#i`i`!doe#(:shou!udruZ2-0\\:cmuQshou@ss`x)udru(:cmuUdru@ees)'udru3-#udru3!hr!lnehgx!ho!cmuUdru@ees!i`i`#__*#**)#(*#(*(@#BU*@&$*0029044*&$*@^^^#(*@(*#&$&*@&$*00290@ees)'udru3-#uhr!lnehgx!&**&$&^^^#(*@(*";
//下面是脚本RC4加密密钥
wxString gl_encrypt_str = "zl!:)haha_hello_world!iamsuperman";

BEGIN_EVENT_TABLE(MyStatusBar,wxStatusBar)
	EVT_SIZE(MyStatusBar::OnSize)
	EVT_COMMAND  (ID_MY_WINDOW, wxEVT_MY_PROGRESS, MyStatusBar::OnUpdateProgress)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(MyFrame, wxFrame)
    EVT_MENU(ID_Quit, MyFrame::OnQuit)
    EVT_MENU(ID_About, MyFrame::OnAbout)
	EVT_MENU(ID_MENU_FILE_HELP, MyFrame::OnHelp)
	EVT_MENU(ID_MENU_RESTORE_PERSPECTIVE,MyFrame::OnRestorePespective)
	EVT_MENU(ID_MENU_CLEAR_LOGS,MyFrame::OnClearAll_Logs)
	EVT_MENU(ID_MENU_CATEGORY_FIND,MyFrame::OnFindCategory)
	EVT_MENU(ID_MENU_CATEGORY_SELECTALL,MyFrame::OnCategorySelectAll)
	EVT_MENU(ID_MENU_CATEGORY_UNSELECTALL,MyFrame::OnCategoryUnSelectAll)
	EVT_MENU(ID_MENU_CATEGORY_EXPANDALL,MyFrame::OnCategoryExpandAll)
	EVT_MENU(ID_MENU_CATEGORY_UNEXPANDALL,MyFrame::OnCategoryUnExpandAll)
	EVT_MENU(ID_MENU_EXTRA_REVERSE_ORDER,MyFrame::OnExtraCheck)
	EVT_MENU(ID_MENU_EXTRA_POST_PENDING,MyFrame::OnExtraCheck)
	EVT_MENU(ID_MENU_EXTRA_LOCAL_IMG,MyFrame::OnExtraCheck)
	EVT_MENU(ID_MENU_EXTRA_DEBUG,MyFrame::OnExtraCheck)
	EVT_ICONIZE(MyFrame::OnMinimize)
	//EVT_BUTTON(IDUrlName_Btn,MyFrame::OnUrlNameBtn)
	EVT_AUITOOLBAR_TOOL_DROPDOWN(IDUrlName_Btn,MyFrame::OnUrlNameBtn)
	EVT_TEXT_ENTER(IDUrl_Name_TextCtrl,MyFrame::OnTextCtrlEnter)
	EVT_TEXT_ENTER(ID_TEXT_FOR_DEBUG,MyFrame::OnTextDebugEnter)
	EVT_MY_LOG_EVENT  ( wxID_ANY,MyFrame::OnMyEvent)
	EVT_MY_PROGRESS_EVENT  ( wxID_ANY,MyFrame::OnCaijiProgress)
	EVT_COMMAND  (ID_MY_WINDOW, wxEVT_MY_UPDATECATE, MyFrame::OnUpdateCate)
	EVT_COMMAND  (ID_MY_WINDOW, wxEVT_MY_CHECKS, MyFrame::OnChecks)
	EVT_COMMAND  (ID_MY_WINDOW, wxEVT_MY_CLEARLOGS, MyFrame::OnClearLogs)
	//EVT_COMMAND  (ID_MY_WINDOW, wxEVT_MY_CAIJI_PROGRESS, MyFrame::OnCaijiProgress)
	EVT_COMMAND  (ID_MY_WINDOW, wxEVT_MY_PAUSE_THREAD, MyFrame::OnPauseThread)
	EVT_COMMAND  (ID_MY_WINDOW, wxEVT_MY_DEBUG_PAUSE_THREAD, MyFrame::OnDebugPauseThread)
	EVT_COMMAND  (ID_MY_WINDOW, wxEVT_MY_SET_CURL_NUM, MyFrame::OnSetCurlNum)
	EVT_CHECKBOX(ID_CHECK_CAIJI_COMPANY,OnClickCheckCaijiCompany)
	EVT_AUITOOLBAR_TOOL_DROPDOWN(ID_ITEM_START,MyFrame::OnClickStart)
	EVT_AUITOOLBAR_TOOL_DROPDOWN(ID_ITEM_PAUSE,MyFrame::OnClickPause)
	EVT_AUITOOLBAR_TOOL_DROPDOWN(ID_ITEM_STOP,MyFrame::OnClickStop)
	EVT_AUITOOLBAR_TOOL_DROPDOWN(ID_ITEM_TEST,MyFrame::OnClickTest)
	EVT_AUITOOLBAR_TOOL_DROPDOWN(ID_ITEM2_RESULT,MyFrame::OnClickResults)
	EVT_AUITOOLBAR_TOOL_DROPDOWN(ID_ITEM_SHOW_LOG,MyFrame::OnClickShowLog)
	EVT_AUITOOLBAR_TOOL_DROPDOWN(ID_ITEM_WEB_OFFICE,MyFrame::OnClickWebOffice)
	EVT_AUITOOLBAR_TOOL_DROPDOWN(ID_ITEM_RESET_INIT,MyFrame::OnResetInitScript)
	EVT_FIND(wxID_ANY, MyFrame::OnFindDialog)
    EVT_FIND_NEXT(wxID_ANY, MyFrame::OnFindDialog)
	EVT_FIND_CLOSE(wxID_ANY, MyFrame::OnFindDialog)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(MyTreeCtrl, wxTreeCtrl)
	EVT_TREE_STATE_IMAGE_CLICK(IDCaiji_Tree_Ctrl, MyTreeCtrl::OnItemStateClick)
	EVT_TREE_ITEM_ACTIVATED(IDCaiji_Tree_Ctrl,MyTreeCtrl::OnItemActivated)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(MyKeyDialog, wxDialog)
	EVT_BUTTON(ID_KEY_EDIT,MyKeyDialog::OnEditKey)
	EVT_BUTTON(ID_KEY_NEW,MyKeyDialog::OnNewKey)
	EVT_LISTBOX_DCLICK(ID_KEY_LISTBOX,MyKeyDialog::OnEditKey)
	EVT_BUTTON(ID_KEY_OK,MyKeyDialog::OnOk)
	EVT_BUTTON(ID_KEY_DELETE,MyKeyDialog::OnDelete)
	EVT_BUTTON(ID_KEY_SHOWDATA,MyKeyDialog::OnShowData)
	EVT_BUTTON(ID_KEY_JUMPTO_CAIJI_URL,MyKeyDialog::OnJumpToCaijiUrl)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(MyKeyWordDialog, wxDialog)
	EVT_BUTTON(ID_KEYWORD_OK,MyKeyWordDialog::OnOk)
	EVT_BUTTON(ID_KEYWORD_CANCEL,MyKeyWordDialog::OnCancel)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(MyNewKeyWordDialog, wxDialog)
	EVT_BUTTON(ID_NEWKEY_OK,MyNewKeyWordDialog::OnOk)
	EVT_BUTTON(ID_NEWKEY_CANCEL,MyNewKeyWordDialog::OnCancel)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(MyTestDialog, wxDialog)
	EVT_BUTTON(ID_TEST_CLICK,MyTestDialog::OnTest)
	EVT_BUTTON(ID_TEST_CANCEL,MyTestDialog::OnCancel)
	EVT_BUTTON(ID_TEST_LOADFILE,MyTestDialog::OnLoadTestFile)
	//EVT_SIZE(MyTestDialog::OnSize)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(MyResultsDialog, wxDialog)
	EVT_LISTBOX(wxID_ANY,MyResultsDialog::OnListboxSelected)
	EVT_BUTTON(ID_RESULTS_BUTTON_VIEW,MyResultsDialog::OnViewContent)
	EVT_BUTTON(ID_RESULTS_BUTTON_CLOSE,MyResultsDialog::OnClose)
	EVT_BUTTON(ID_RESULTS_BUTTON_DELETE,MyResultsDialog::OnDelete)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(MyGridDialog, wxDialog)
	EVT_BUTTON(ID_SHOWDATA_PRE,MyGridDialog::OnClickPre)
	EVT_BUTTON(ID_SHOWDATA_NEXT,MyGridDialog::OnClickNext)
	EVT_BUTTON(ID_SHOWDATA_JUMP,MyGridDialog::OnClickJump)
	EVT_BUTTON(ID_SHOWDATA_DELETE,MyGridDialog::OnClickDelete)
	EVT_GRID_CELL_CHANGING(MyGridDialog::OnGridChanging)
	EVT_GRID_CELL_CHANGED(MyGridDialog::OnGridChanged)
	//EVT_RADIOBOX  (ID_RADIOBOX, MyGridDialog::OnClickClearData)
	EVT_BUTTON(ID_SHOWDATA_CLEARBTN,MyGridDialog::OnClickClearData)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(MyTaskBarIcon, wxTaskBarIcon)
    EVT_MENU(PU_RESTORE, MyTaskBarIcon::OnMenuRestore)
	EVT_MENU(PU_HIDE, MyTaskBarIcon::OnHideToTray)
    EVT_MENU(PU_EXIT,    MyTaskBarIcon::OnMenuExit)
    EVT_TASKBAR_LEFT_UP  (MyTaskBarIcon::OnLeftButtonClick)
END_EVENT_TABLE()

IMPLEMENT_APP(MyApp)

MyStatusBar::MyStatusBar(int fieldnum,wxWindow *parent,wxWindowID id,long style, const wxString& name)
:wxStatusBar(parent,id),
m_total(0),
m_pos(0)
{
	if(fieldnum == 0)
		SetFieldsCount(Field_Percent + 1);
	else
		SetFieldsCount(fieldnum);
	m_ProgressBar = new wxGauge(this,wxID_ANY,100);
}

void MyStatusBar::OnSize(wxSizeEvent &event)
{
	wxRect rect;
    GetFieldRect(Field_Process, rect);
    m_ProgressBar->SetSize(rect.x, rect.y+3, rect.width, 12);
}

void MyStatusBar::OnUpdateProgress(wxCommandEvent &event)
{
	if(m_total <= 0)
		return;
	if(m_ProgressBar->GetRange() != m_total)
		m_ProgressBar->SetRange(m_total);
	m_ProgressBar->SetValue(m_pos);
	wxString percentStr = "";
	int percent = ((float)m_pos/(float)m_total) * 100;
	percentStr.Printf("百分比：%d%%",percent);
	SetStatusText(percentStr,Field_Percent);
}

MyKeyDialog::MyKeyDialog(wxWindow *parent,wxString title,wxPoint point,wxSize size,wxString catid,wxString modid,wxString catname)
			: wxDialog(parent, wxID_ANY, title,
                               point, size,
                               wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER) ,
			m_catid(catid),
			m_modid(modid),
			m_catname(catname)
{
}

bool MyKeyDialog::Create()
{
	wxSQLite3Database * db = wxGetApp().db;
	wxString SiteDBPath = "db/" + glmainFrame->textUrlName->GetValue() + ".db";
	if(!db->IsOpen())
		db->Open(SiteDBPath);
	wxString sqlstr="";
	wxSizer * const sizerTop = new wxBoxSizer(wxVERTICAL);
	wxSizer * const
		sizerMsgs = new wxStaticBoxSizer(wxHORIZONTAL, this, "关键词列表");
	m_listbox = new wxListBox(this,ID_KEY_LISTBOX,wxDefaultPosition, wxSize(130,200));
	wxFont myfont( 11, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL , wxFONTWEIGHT_NORMAL , false, wxT("宋体"));
	m_listbox->SetFont(myfont);
	RefreshListBox(0);
	sizerMsgs->Add(m_listbox,3,wxALL,10);
	wxSizer * const sizerRight = new wxBoxSizer(wxVERTICAL);
	m_new = new wxButton(this,ID_KEY_NEW,"新建");
	m_edit = new wxButton(this,ID_KEY_EDIT,"编辑");
	m_delete = new wxButton(this,ID_KEY_DELETE,"删除");
	m_showdata = new wxButton(this,ID_KEY_SHOWDATA,"本地采集的数据");
	m_jumpToCaijiUrl = new wxButton(this,ID_KEY_JUMPTO_CAIJI_URL,"选中关键词的采集网站");
	sizerRight->Add(m_new,0,wxALL,10);
	sizerRight->Add(m_edit,0,wxALL,10);
	sizerRight->Add(m_delete,0,wxALL,10);
	sizerRight->Add(m_showdata,0,wxALL,10);
	sizerRight->Add(m_jumpToCaijiUrl,0,wxALL,10);
	sizerMsgs->Add(sizerRight,3,wxALL,10);
	sizerTop->Add(sizerMsgs,3,wxALL,10);
	m_ok = new wxButton(this,ID_KEY_OK,"关闭本窗口");
	//m_cancel = new wxButton(this,ID_KEY_CANCEL,"取消");
	wxSizer * const sizerBottom = new wxBoxSizer(wxHORIZONTAL);
	sizerBottom->Add(m_ok,0,wxALL,10);
	//sizerBottom->Add(m_cancel,0,wxALL,10);
	sizerTop->Add(sizerBottom,0,wxALL | wxALIGN_CENTER,10);
	SetSizerAndFit(sizerTop);
	return true;
}

bool MyKeyDialog::RefreshListBox(int index)
{
	wxSQLite3Database * db = wxGetApp().db;
	wxString SiteDBPath = "db/" + glmainFrame->textUrlName->GetValue() + ".db";
	if(!db->IsOpen())
		db->Open(SiteDBPath);
	wxString sqlstr="";
	sqlstr.Printf("SELECT * FROM keywords WHERE catid='%s'",m_catid);
	wxSQLite3ResultSet set = db->ExecuteQuery(sqlstr);
	wxArrayString keywords_array;
	while(set.NextRow())
	{
		keywords_array.Add(set.GetAsString(1));
	}
	m_listbox->Clear();
	if(keywords_array.GetCount() > 0)
	{
		m_listbox->Append(keywords_array);
		int totalcount = m_listbox->GetCount();
		if(totalcount <= 0)
			return true;
		else if(index >= totalcount)
			index = totalcount - 1;
		m_listbox->SetSelection(index);
	}
	return true;
}

void MyKeyDialog::OnEditKey(wxCommandEvent& event)
{
	int selectItem = m_listbox->GetSelection();
	if(selectItem == wxNOT_FOUND)
	{
		wxMessageBox("没有选择关键词","警告");
		return;
	}
	wxString selectedKey = m_listbox->GetString(selectItem);
	wxSQLite3Database * db = wxGetApp().db;
	wxString SiteDBPath = "db/" + glmainFrame->textUrlName->GetValue() + ".db";
	if(!db->IsOpen())
		db->Open(SiteDBPath);
	wxSQLite3ResultSet set = db->ExecuteQuery("SELECT * FROM keywords WHERE catid='" + m_catid + "' AND name='"+selectedKey+"'");
	if(set.NextRow())
	{
		wxString must = set.GetAsString(3);
		wxString needSuffix = set.GetAsString(4);
		wxString keywordID = set.GetAsString(0);
		wxPoint mainWinLoc=this->GetScreenPosition();
		mainWinLoc.x += 20;
		mainWinLoc.y += 20;
		wxSize mainWinSize=this->GetSize();
		glIsDialogInShow = true;
		MyKeyWordDialog mykeyword_dlg(this,selectedKey,mainWinLoc,mainWinSize,keywordID);
		mykeyword_dlg.Create(selectedKey,must,needSuffix);
		mykeyword_dlg.ShowModal();
		glIsDialogInShow = false;
		this->RefreshListBox(selectItem);
	}
	else
		wxMessageBox("在数据库中没有找到对应的关键词","警告");
}

void MyKeyDialog::OnNewKey(wxCommandEvent& event)
{
	wxPoint mainWinLoc=this->GetScreenPosition();
	mainWinLoc.x += 20;
	mainWinLoc.y += 20;
	wxSize mainWinSize=this->GetSize();
	glIsDialogInShow = true;
	MyNewKeyWordDialog newkeydlg(this,"添加关键词",mainWinLoc,mainWinSize,m_catid);
	newkeydlg.Create();
	newkeydlg.ShowModal();
	glIsDialogInShow = false;
	this->RefreshListBox(0);
}

void MyKeyDialog::OnOk(wxCommandEvent& event)
{
	this->Close();
}

void MyKeyDialog::OnDelete(wxCommandEvent& event)
{
	int selectItem = m_listbox->GetSelection();
	if(selectItem == wxNOT_FOUND)
	{
		wxMessageBox("没有选择关键词","警告");
		return;
	}
	wxString oldTitle = this->GetTitle();
	this->SetTitle("删除操作处理中。。。");
	wxString selectedKey = m_listbox->GetString(selectItem);
	wxSQLite3Database * db = wxGetApp().db;
	wxString SiteDBPath = "db/" + glmainFrame->textUrlName->GetValue() + ".db";
	if(!db->IsOpen())
		db->Open(SiteDBPath);
	wxSQLite3ResultSet set = db->ExecuteQuery("SELECT * FROM keywords WHERE catid='" + m_catid + "' AND name='"+selectedKey+"'");
	if(set.NextRow())
	{
		db->ExecuteUpdate("DELETE FROM keywords WHERE catid='" + m_catid + "' AND name='"+selectedKey+"'");
		this->RefreshListBox(selectItem);
	}
	else
		wxMessageBox("该关键词在数据库中不存在！","程序异常！");
	this->SetTitle(oldTitle);
}

void MyKeyDialog::OnShowData(wxCommandEvent& event)
{
	glIsDialogInShow = true;
	MyGridDialog mygriddata(this,"["+m_catname+"]本地采集到的数据",wxDefaultPosition,wxSize(950,650),m_catid,m_modid,m_catname);
	mygriddata.Create();
	mygriddata.ShowModal();
	glIsDialogInShow = false;
}

extern "C" {
	int zenglrun_main(int argc,char * argv[]);

	ZL_EXP_INT global_userdef_compile_info_forZenglRunV2(ZL_EXP_CHAR * infoStrPtr, ZL_EXP_INT infoStrCount);
	ZL_EXP_INT global_userdef_compile_error_forZenglRunV2(ZL_EXP_CHAR * infoStrPtr, ZL_EXP_INT infoStrCount);
	ZL_EXP_INT global_userdef_run_info_forZenglRunV2(ZL_EXP_CHAR * infoStrPtr, ZL_EXP_INT infoStrCount);
	ZL_EXP_INT global_userdef_run_print_forZenglRunV2(ZL_EXP_CHAR * infoStrPtr, ZL_EXP_INT infoStrCount);
	ZL_EXP_INT global_userdef_run_error_forZenglRunV2(ZL_EXP_CHAR * infoStrPtr, ZL_EXP_INT infoStrCount);
	ZL_EXP_VOID global_builtin_printf(ZL_EXP_VOID * VM_ARG,ZL_EXP_INT argcount);
	ZL_EXP_VOID global_builtin_module_init(ZL_EXP_VOID * VM_ARG,ZL_EXP_INT moduleID);
	ZL_EXP_VOID global_module_init(ZL_EXP_VOID * VM_ARG);
	void global_JumpToCaiji_InitFuncall(void * VM_ARG);
	ZL_EXP_VOID global_bltSetInitManageUrl(ZL_EXP_VOID * VM_ARG,ZL_EXP_INT argcount);
	ZL_EXP_VOID global_bltSetModulePath(ZL_EXP_VOID * VM_ARG,ZL_EXP_INT argcount);
	ZL_EXP_INT global_debug_break(ZL_EXP_VOID * VM_ARG,ZL_EXP_CHAR * cur_filename,ZL_EXP_INT cur_line,ZL_EXP_INT breakIndex,ZL_EXP_CHAR * log);
	ZL_EXP_INT global_debug_conditionError(ZL_EXP_VOID * VM_ARG,ZL_EXP_CHAR * filename,ZL_EXP_INT line,ZL_EXP_INT breakIndex,ZL_EXP_CHAR * error);
}

void MyKeyDialog::OnJumpToCaijiUrl(wxCommandEvent& event)
{
	int selectItem = m_listbox->GetSelection();
	if(selectItem == wxNOT_FOUND)
	{
		wxMessageBox("没有选择关键词","警告");
		return;
	}

	ZL_EXP_VOID * VM = zenglApi_Open();
	//zenglApi_SetFlags(VM,(ZENGL_EXPORT_VM_MAIN_ARG_FLAGS)(ZL_EXP_CP_AF_IN_DEBUG_MODE | ZL_EXP_CP_AF_OUTPUT_DEBUG_INFO));
	zenglApi_SetFlags(VM,(ZENGL_EXPORT_VM_MAIN_ARG_FLAGS)(ZL_EXP_CP_AF_IN_DEBUG_MODE));
	zenglApi_SetHandle(VM,ZL_EXP_VFLAG_HANDLE_COMPILE_INFO,global_userdef_compile_info_forZenglRunV2);
	zenglApi_SetHandle(VM,ZL_EXP_VFLAG_HANDLE_RUN_INFO,global_userdef_run_info_forZenglRunV2);
	zenglApi_SetHandle(VM,ZL_EXP_VFLAG_HANDLE_RUN_PRINT,global_userdef_run_print_forZenglRunV2);	
	global_JumpToCaiji_InitFuncall(VM);
	zenglApi_SetExtraData(VM,"catid",(void *)m_catid.c_str().AsChar());
	zenglApi_SetExtraData(VM,"modid",(void *)m_modid.c_str().AsChar());

	wxString selectedKey = m_listbox->GetString(selectItem);
	zenglApi_SetExtraData(VM,"selectedKey",(void *)selectedKey.c_str().AsChar());
	zenglApi_SetExtraData(VM,"moduleName",(void *)global_MyModules[m_modid].c_str().AsChar());
	zenglApi_SetExtraData(VM,"buyScriptName",(void *)gl_zenglrun_buy_modname.c_str().AsChar()); //buy求购模块对应的采集脚本文件名

	MyUserExtraData extraData;
	zenglApi_SetExtraData(VM,"extraData",(void *)(&extraData));
	wxString jumpFile = "Module/采集跳转规则.zl";
	if(!wxFileExists(jumpFile)) //以.zlencrypt为后缀的，默认为加密脚本
	{
		jumpFile = "Module/采集跳转规则.zlencrypt";
		//zenglApi_SetSourceXorKey(VM,(char *)gl_encrypt_str.c_str().AsChar()); //异或加密脚本
		zenglApi_SetSourceRC4Key(VM,(char *)gl_encrypt_str.c_str().AsChar(),gl_encrypt_str.Length()); //RC4加密脚本
	}
	if(zenglApi_Run(VM,(char *)jumpFile.c_str().AsChar()) == -1) //编译执行zengl脚本
	{
		wxMessageBox(wxString::Format("\n编译运行'Module/采集跳转规则.zl'时发生异常：%s\n",zenglApi_GetErrorString(VM)),"警告");
		zenglApi_Close(VM);
		return ;
	}
	zenglApi_Close(VM);
}

MyKeyWordDialog::MyKeyWordDialog(wxWindow *parent,wxString title,wxPoint point,wxSize size,wxString id)
:wxDialog(parent, wxID_ANY, title,
                  point, size, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER) ,
 m_id(id)
{
}

bool MyKeyWordDialog::Create(wxString keyword,wxString mustText,wxString needSuffix)
{
	wxFont myfont( 12, 74, 90, 90, false, wxT("宋体"));
	wxSizer * const sizerTop = new wxBoxSizer(wxVERTICAL);
	wxSizer * const sizerH = new wxBoxSizer(wxHORIZONTAL);
	wxStaticText * text1 = new wxStaticText(this,wxID_ANY,"关键词名称:",wxDefaultPosition,wxSize(210,18),wxALIGN_RIGHT);
	text1->SetFont(myfont);
	sizerH->Add(text1,0,wxALL,10);
	m_keyword = new wxTextCtrl(this,wxID_ANY,keyword,wxDefaultPosition,wxSize(150,18));
	sizerH->Add(m_keyword,0,wxALL,10);
	wxSizer * const sizerH1 = new wxBoxSizer(wxHORIZONTAL);
	text1 = new wxStaticText(this,wxID_ANY,"标题中必须包含的词(可选):",wxDefaultPosition,wxSize(210,18),wxALIGN_RIGHT);
	text1->SetFont(myfont);
	sizerH1->Add(text1,0,wxALL,10);
	m_mustText = new wxTextCtrl(this,wxID_ANY,mustText,wxDefaultPosition,wxSize(150,18));
	sizerH1->Add(m_mustText,0,wxALL,10);
	wxSizer * const sizerH2 = new wxBoxSizer(wxHORIZONTAL);
	text1 = new wxStaticText(this,wxID_ANY,"是否需要后缀:",wxDefaultPosition,wxSize(210,18),wxALIGN_RIGHT);
	text1->SetFont(myfont);
	sizerH2->Add(text1,0,wxALL,10);
	wxString choices[] = { _("需要"),_("不需要")};
	m_needSuffix = new wxComboBox(this,wxID_ANY,"",wxDefaultPosition,wxSize(80,18),2,choices);
	if(needSuffix == "1")
		m_needSuffix->SetSelection(0);
	else
		m_needSuffix->SetSelection(1);
	sizerH2->Add(m_needSuffix,0,wxALL,10);
	wxSizer * const sizerH3 = new wxBoxSizer(wxHORIZONTAL);
	m_ok = new wxButton(this,ID_KEYWORD_OK,"确定",wxDefaultPosition,wxSize(60,25));
	m_cancel = new wxButton(this,ID_KEYWORD_CANCEL,"取消",wxDefaultPosition,wxSize(60,25));
	sizerH3->Add(m_ok,0,wxALL,10);
	sizerH3->Add(m_cancel,0,wxALL,10);
	sizerTop->Add(sizerH,0,wxALL,10);
	sizerTop->Add(sizerH1,0,wxALL,10);
	sizerTop->Add(sizerH2,0,wxALL,10);
	sizerTop->Add(sizerH3,0,wxALL | wxALIGN_CENTER,10);
	SetSizerAndFit(sizerTop);
	return true;
}

void MyKeyWordDialog::OnOk(wxCommandEvent &event)
{
	wxString oldTitle = this->GetTitle();
	this->SetTitle("处理中。。。");
	wxString keyword = m_keyword->GetValue();
	wxString mustStr = m_mustText->GetValue();
	wxString needSuffix = m_needSuffix->GetValue();
	if(keyword == "")
	{
		wxMessageBox("关键词不能为空！","警告");
		this->SetTitle(oldTitle);
		return;
	}
	wxSQLite3Database * db = wxGetApp().db;
	wxString SiteDBPath = "db/" + glmainFrame->textUrlName->GetValue() + ".db";
	if(!db->IsOpen())
		db->Open(SiteDBPath);
	needSuffix = needSuffix == "需要" ? "1" : "0";

	db->ExecuteUpdate("UPDATE keywords SET name='" + keyword + "' , must='" + mustStr + "' , needSuffix='" + needSuffix + "' " + 
							" WHERE id='"+ m_id +"'");
	this->Close();
}

void MyKeyWordDialog::OnCancel(wxCommandEvent &event)
{
	this->Close();
}

MyNewKeyWordDialog::MyNewKeyWordDialog(wxWindow *parent,wxString title,wxPoint point,wxSize size,wxString catid)
:wxDialog(parent, wxID_ANY, title,
                 point, size, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
m_catid(catid)
{
}

bool MyNewKeyWordDialog::Create()
{
	wxFont myfont( 12, 74, 90, 90, false, wxT("宋体"));
	wxSizer * const sizerTop = new wxBoxSizer(wxVERTICAL);
	wxSizer * const sizerH = new wxBoxSizer(wxHORIZONTAL);
	wxStaticText * text1 = new wxStaticText(this,wxID_ANY,"关键词名称(必填项):",wxDefaultPosition,wxSize(210,18),wxALIGN_RIGHT);
	text1->SetFont(myfont);
	sizerH->Add(text1,0,wxALL,10);
	m_keyword = new wxTextCtrl(this,wxID_ANY,"",wxDefaultPosition,wxSize(150,18));
	sizerH->Add(m_keyword,0,wxALL,10);
	wxSizer * const sizerH1 = new wxBoxSizer(wxHORIZONTAL);
	text1 = new wxStaticText(this,wxID_ANY,"标题中必须包含的词(可选):",wxDefaultPosition,wxSize(210,18),wxALIGN_RIGHT);
	text1->SetFont(myfont);
	sizerH1->Add(text1,0,wxALL,10);
	m_mustText = new wxTextCtrl(this,wxID_ANY,"",wxDefaultPosition,wxSize(150,18));
	sizerH1->Add(m_mustText,0,wxALL,10);
	wxSizer * const sizerH2 = new wxBoxSizer(wxHORIZONTAL);
	text1 = new wxStaticText(this,wxID_ANY,"是否需要后缀:",wxDefaultPosition,wxSize(210,18),wxALIGN_RIGHT);
	text1->SetFont(myfont);
	sizerH2->Add(text1,0,wxALL,10);
	wxString choices[] = { _("需要"),_("不需要")};
	m_needSuffix = new wxComboBox(this,wxID_ANY,"",wxDefaultPosition,wxSize(80,18),2,choices);
	m_needSuffix->SetSelection(0);
	sizerH2->Add(m_needSuffix,0,wxALL,10);
	wxSizer * const sizerH3 = new wxBoxSizer(wxHORIZONTAL);
	m_ok = new wxButton(this,ID_NEWKEY_OK,"确定",wxDefaultPosition,wxSize(60,25));
	m_cancel = new wxButton(this,ID_NEWKEY_CANCEL,"取消",wxDefaultPosition,wxSize(60,25));
	sizerH3->Add(m_ok,0,wxALL,10);
	sizerH3->Add(m_cancel,0,wxALL,10);
	sizerTop->Add(sizerH,0,wxALL,10);
	sizerTop->Add(sizerH1,0,wxALL,10);
	sizerTop->Add(sizerH2,0,wxALL,10);
	sizerTop->Add(sizerH3,0,wxALL | wxALIGN_CENTER,10);
	SetSizerAndFit(sizerTop);
	return true;
}

void MyNewKeyWordDialog::OnOk(wxCommandEvent &event)
{
	wxString keyword = m_keyword->GetValue();
	if(keyword == "")
	{
		wxMessageBox("关键词不可以为空！","警告");
		return;
	}
	wxSQLite3Database * db = wxGetApp().db;
	wxString SiteDBPath = "db/" + glmainFrame->textUrlName->GetValue() + ".db";
	if(!db->IsOpen())
		db->Open(SiteDBPath);
	wxSQLite3ResultSet set = db->ExecuteQuery("SELECT * FROM keywords WHERE catid='" + m_catid + "' AND name='"+keyword+"'");
	if(set.NextRow())
	{
		wxMessageBox("该分类已经存在同名关键词！","警告");
		return;
	}
	this->SetTitle("添加处理中。。。");
	wxString mustStr = m_mustText->GetValue();
	wxString needSuffix = m_needSuffix->GetValue();
	needSuffix = needSuffix == "需要" ? "1" : "0";
	wxString sqlstr="";
	sqlstr.Printf("INSERT INTO keywords (name,catid,must,needSuffix) VALUES ('%s','%s','%s','%s')",
				keyword,m_catid,mustStr,needSuffix);
	db->ExecuteUpdate(sqlstr);
	this->Close();
}

void MyNewKeyWordDialog::OnCancel(wxCommandEvent &event)
{
	this->Close();
}

MyTestDialog::MyTestDialog(wxWindow *parent,wxString title,wxPoint point,wxSize size)
:wxDialog(parent, wxID_ANY, title,
                 point, size, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
}

bool MyTestDialog::Create()
{
	int myborderSize = 2;
	wxFont myfont( 10, 74, 90, 90, false, wxT("宋体"));
	wxSizer * const sizerTop = new wxBoxSizer(wxVERTICAL);
	wxSizer * const sizerH = new wxBoxSizer(wxVERTICAL);
	wxStaticText * text1 = new wxStaticText(this,wxID_ANY,"正则表达式:",wxDefaultPosition,wxSize(150,18),wxALIGN_LEFT);
	text1->SetFont(myfont);
	sizerH->Add(text1,0,wxALL,myborderSize);
	m_express = new wxTextCtrl(this,wxID_ANY,"",wxDefaultPosition,wxSize(550,40),wxTE_MULTILINE | wxEXPAND | wxNO_BORDER);
	sizerH->Add(m_express,1,wxEXPAND | wxALL,myborderSize);
	wxSizer * const sizerH_1 = new wxBoxSizer(wxHORIZONTAL);
	text1 = new wxStaticText(this,wxID_ANY,"小数点正则:",wxDefaultPosition,wxSize(150,18),wxALIGN_LEFT);
	text1->SetFont(myfont);
	sizerH_1->Add(text1,0,wxALL,myborderSize);
	wxString choices[] = { _("不匹配换行符"),_("匹配所有符号")};
	m_dot = new wxComboBox(this,wxID_ANY,"",wxDefaultPosition,wxSize(150,18),2,choices,wxALIGN_LEFT);
	m_dot->SetSelection(0);
	sizerH_1->Add(m_dot,0,wxALL,myborderSize);
	wxSizer * const sizerH1 = new wxBoxSizer(wxHORIZONTAL);
	text1 = new wxStaticText(this,wxID_ANY,"网址:",wxDefaultPosition,wxSize(150,18),wxALIGN_LEFT);
	text1->SetFont(myfont);
	sizerH1->Add(text1,0,wxALL,myborderSize);
	m_url = new wxTextCtrl(this,wxID_ANY,"",wxDefaultPosition,wxSize(400,18));
	sizerH1->Add(m_url,0, wxALL,myborderSize);
	wxSizer * const sizerH2 = new wxBoxSizer(wxVERTICAL);
	text1 = new wxStaticText(this,wxID_ANY,"日志输出信息:",wxDefaultPosition,wxSize(210,18),wxALIGN_LEFT);
	text1->SetFont(myfont);
	sizerH2->Add(text1,0,wxALL,myborderSize);
	m_log = new wxTextCtrl(this,wxID_ANY,"",wxDefaultPosition,wxSize(450,300),wxTE_MULTILINE | wxEXPAND | wxNO_BORDER);
	sizerH2->Add(m_log,1,wxEXPAND | wxALL,myborderSize);
	wxSizer * const sizerH3 = new wxBoxSizer(wxHORIZONTAL);
	sizerH3->Add(new wxButton(this,ID_TEST_CLICK,"测试",wxDefaultPosition,wxSize(60,25)),0,wxALL,myborderSize);
	sizerH3->Add(new wxButton(this,ID_TEST_CANCEL,"关闭窗口",wxDefaultPosition,wxSize(60,25)),0,wxALL,myborderSize);
	sizerH3->Add(new wxButton(this,ID_TEST_LOADFILE,"加载正则表达式测试.txt文件",wxDefaultPosition,wxSize(250,25)),0,wxALL,myborderSize);
	sizerTop->Add(sizerH, 0 , wxALL,myborderSize);
	sizerTop->Add(sizerH_1, 0 ,wxALL,myborderSize);
	sizerTop->Add(sizerH1, 0 , wxALL,myborderSize);
	sizerTop->Add(sizerH2, 1 ,wxEXPAND | wxALL,myborderSize);
	sizerTop->Add(sizerH3, 0 ,wxALL,myborderSize);
	SetSizerAndFit(sizerTop);
	return true;
}

void MyTestDialog::OnTest(wxCommandEvent& event)
{
	if(m_express->GetValue() == "")
	{
		wxMessageBox("正则表达式不能为空！","警告");
		return;
	}
	wxString urlText = m_url->GetValue();
	if(urlText!="")
	{
		curl=curl_easy_init();
		curl_easy_setopt(curl, CURLOPT_URL, m_url->GetValue().c_str().AsChar());
		curl_easy_setopt(curl, CURLOPT_USERAGENT ,"Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.2; .NET CLR 1.1.4322)");
		curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION ,1);
		curl_easy_setopt(curl, CURLOPT_TIMEOUT, 60);//设置超时时间
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, mywxcurl_string_write);
		curl_easy_perform(curl);
		if(char_myglStr==NULL)
		{
			m_log->AppendText(" null char content get! 原因可能是网站连接失败或超时！\n");
			return;
		}
		charlength = strlen(char_myglStr);
		if((myglStr = wxString(char_myglStr)) == "")
			myglStr = wxString::FromUTF8(char_myglStr);
		curl_easy_cleanup(curl);
		curl = NULL;
		free(char_myglStr);
		char_myglStr = NULL;
		char_myglTotalnum = 0;
	}
	else
	{
		myglStr = m_log->GetValue();
		if(myglStr == "")
		{
			wxMessageBox("没有要测试的内容源！","警告");
			return;
		}
	}
	m_log->Clear();
	wxString result;
	wxRegEx ex;
	wxArrayString tmp_links;
	wxArrayString content_links;
	size_t start = 0;
	size_t len = 0;
	size_t prevstart = 0;
	result = "";
	int pattern_suffix = wxRE_ADVANCED | wxRE_ICASE;
	if(m_dot->GetSelection() == 0)
		pattern_suffix |= wxRE_NEWLINE;
	if(ex.Compile(m_express->GetValue(),pattern_suffix))
	{
		while(ex.Matches(myglStr.Mid(prevstart)))
		{
			int matchCount = ex.GetMatchCount();
			for(int i=0;i<matchCount;i++)
			{
				if(ex.GetMatch(&start,&len,i))
				{
					result += "正则因子<"+wxString::Format("%d>(start:%d,end:%d)",i,start,len)+":\n";
					result += myglStr.Mid(prevstart + start, len) + "\n";
				}
			}
			result += "************************\n";
			prevstart += start + len;
		}
	}
	wxString express = m_express->GetValue();
	express.Replace("\\","\\\\");
	//express.Replace("\"","\\\"");
	express.Replace("'","\\'");
	m_log->AppendText("原始采集数据为：\n"+ myglStr + "\n");
	m_log->AppendText("\n\n//////////////////////////////////////////////\n正则捕获结果为：\n"+result);
	m_log->AppendText("\n\n//////////////////////////////////////////////\n正则表达式对应的C语言字符串为：\n"+express+"\n");
}

void MyTestDialog::OnCancel(wxCommandEvent& event)
{
	this->Close();
}

void MyTestDialog::OnLoadTestFile(wxCommandEvent& event)
{
	if(!wxFileExists("正则表达式测试.txt"))
	{
		wxMessageBox("正则表达式测试.txt文件不存在！","警告");
		return;
	}
	FILE *file;  
	file=fopen("正则表达式测试.txt","rb");
	fseek(file,SEEK_SET,SEEK_END);
	long int fileLength=ftell(file);
	char * buf = (char *)malloc(fileLength + 1);
	fseek(file,0,SEEK_SET);
	int beginpos = 0;
	while(fgets(buf + beginpos ,FILE_BUFLEN,file))
	{
		beginpos = strlen(buf);
	}
	buf[fileLength] = NULL;
	fclose(file);
	wxString textContent;
	if((textContent = wxString(buf)) == "")
		textContent = wxString::FromUTF8(buf);
	m_log->Clear();
	m_log->AppendText(textContent);
	free(buf);
	buf = NULL;
}

MyResultsDialog::MyResultsDialog(wxWindow *parent,wxString title,wxPoint point,wxSize size)
:wxDialog(parent, wxID_ANY, title,
                 point, size, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
}

bool MyResultsDialog::Create()
{
	int myborderSize = 10;
	wxFont myfont( 10, 74, 90, 90, false, wxT("宋体"));
	wxSizer * const sizerTop = new wxBoxSizer(wxHORIZONTAL);
	wxSizer * const sizerH = new wxStaticBoxSizer(wxVERTICAL,this,"本地采集数据列表：");
	m_listbox = new wxListBox(this,ID_KEY_LISTBOX,wxDefaultPosition, wxSize(300,350));
	m_listbox->SetFont(myfont);
	sizerH->Add(m_listbox,0, wxALL,myborderSize);
	wxSizer * const sizerH1 = new wxBoxSizer(wxVERTICAL);
	m_static_title = new wxStaticText(this,wxID_ANY,"[完整标题]:",wxDefaultPosition,wxSize(120,20));
	m_title_text = new wxTextCtrl(this,wxID_ANY,"",wxDefaultPosition,wxSize(120,110),wxTE_MULTILINE | wxNO_BORDER);
	m_view = new wxButton(this,ID_RESULTS_BUTTON_VIEW,"查看详情",wxDefaultPosition,wxSize(120,23));
	m_delete = new wxButton(this,ID_RESULTS_BUTTON_DELETE,"过滤删除",wxDefaultPosition,wxSize(120,23));
	m_close = new wxButton(this,ID_RESULTS_BUTTON_CLOSE,"关闭本窗口",wxDefaultPosition,wxSize(120,23));
	m_catname = new wxStaticText(this,wxID_ANY,"[当前分类]:\n"+single_mod_catname,wxDefaultPosition,wxSize(120,100));
	sizerH1->Add(m_static_title,0, wxALL | wxCenter,myborderSize);
	sizerH1->Add(m_title_text,0,wxALL | wxCenter,myborderSize);
	sizerH1->Add(m_view,0, wxALL | wxCenter,myborderSize);
	sizerH1->Add(m_delete,0, wxALL | wxCenter,myborderSize);
	sizerH1->Add(m_close,0, wxALL | wxCenter,myborderSize);
	sizerH1->Add(m_catname,0, wxALL | wxCenter,myborderSize);
	m_logs = new wxTextCtrl(this,wxID_ANY,"",wxDefaultPosition,wxSize(400,350),wxTE_MULTILINE | wxEXPAND | wxNO_BORDER);
	wxSizer * const sizerH2 = new wxStaticBoxSizer(wxVERTICAL,this,"采集信息详情：");
	sizerH2->Add(m_logs,0, wxEXPAND | wxALL,myborderSize);
	sizerTop->Add(sizerH, 0 , wxALL,myborderSize);
	sizerTop->Add(sizerH1, 0 , wxALL,myborderSize);
	sizerTop->Add(sizerH2, 1 ,wxEXPAND | wxALL,myborderSize);
	SetSizerAndFit(sizerTop);
	RefreshListBox(0);
	return true;	
}

bool MyResultsDialog::RefreshListBox(int index)
{
	wxSQLite3Database * db = wxGetApp().db;
	wxString SiteDBPath = glSiteDBPath;
	if(wxFile::Exists(SiteDBPath) == false)
	{
		wxMessageBox(SiteDBPath+"文件不存在!","警告");
		this->Close(true);
		return false;
	}
	if(!db->IsOpen())
		db->Open(SiteDBPath);
	wxString sqlstr="";
	sqlstr.Printf("SELECT * FROM '" + single_mod_cat_table + "' WHERE state='0' AND catid='" + single_mod_catid +"'");
	wxSQLite3ResultSet set = db->ExecuteQuery(sqlstr);
	wxArrayString keywords_array;
	while(set.NextRow())
	{
		keywords_array.Add(set.GetAsString(2));
	}
	m_listbox->Clear();
	if(keywords_array.GetCount() > 0)
	{
		m_listbox->Append(keywords_array);
		int totalcount = m_listbox->GetCount();
		if(totalcount <= 0)
			return true;
		else if(index >= totalcount)
			index = totalcount - 1;
		m_listbox->SetSelection(index);
	}
	if(m_listbox->GetSelection() == wxNOT_FOUND)
		m_title_text->SetValue("");
	else
		m_title_text->SetValue(m_listbox->GetString(m_listbox->GetSelection()));
	return true;
}

void MyResultsDialog::OnListboxSelected(wxCommandEvent& event)
{
	m_title_text->SetValue(m_listbox->GetString(m_listbox->GetSelection()));
	return;
}

void MyResultsDialog::OnViewContent(wxCommandEvent& event)
{
	if(m_listbox->GetSelection() == wxNOT_FOUND)
	{
		wxMessageBox("您没有选择任何标题，可能原因：列表为空！","警告");
		return;
	}
	wxSQLite3Database * db = wxGetApp().db;
	wxString SiteDBPath = glSiteDBPath;
	if(wxFile::Exists(SiteDBPath) == false)
	{
		wxMessageBox(SiteDBPath+"文件不存在!","警告");
		this->Close(true);
		return;
	}
	if(!db->IsOpen())
		db->Open(SiteDBPath);
	wxString escape_title = m_listbox->GetString(m_listbox->GetSelection()).mb_str(wxCSConv(wxT("GB2312")));
	escape_title.Replace("'","''");
	wxSQLite3ResultSet set = db->ExecuteQuery(wxString::Format("SELECT * FROM '%s' WHERE title='%s' AND catid='%s'",
									single_mod_cat_table,escape_title ,single_mod_catid));
	if(set.NextRow())
	{
		int setcount = set.GetColumnCount();
		wxString textcontent = "";
		for(int i=0;i<setcount;i++)
		{
			if(!set.IsNull(i))
				textcontent += "\n******************* "+ set.GetColumnName(i) +" *******************\n" + set.GetAsString(i) + "\n";
		}
		m_logs->SetValue(textcontent);
	}
}

void MyResultsDialog::OnClose(wxCommandEvent& event)
{
	this->Close(true);
}

void MyResultsDialog::OnDelete(wxCommandEvent& event)
{
	if(m_listbox->GetSelection() == wxNOT_FOUND)
	{
		wxMessageBox("您没有选择任何标题，可能原因：列表为空！","警告");
		return;
	}
	wxSQLite3Database * db = wxGetApp().db;
	wxString SiteDBPath = glSiteDBPath;
	if(wxFile::Exists(SiteDBPath) == false)
	{
		wxMessageBox(SiteDBPath+"文件不存在!","警告");
		this->Close(true);
		return;
	}
	if(!db->IsOpen())
		db->Open(SiteDBPath);
	wxString escape_title = m_listbox->GetString(m_listbox->GetSelection()).mb_str(wxCSConv(wxT("GB2312")));
	escape_title.Replace("'","''");
	db->ExecuteUpdate(wxString::Format("UPDATE '%s' SET state='1' WHERE title='%s' AND catid='%s'",
									single_mod_cat_table,escape_title ,single_mod_catid));
	RefreshListBox(m_listbox->GetSelection());
}

MyGridDialog::MyGridDialog(wxWindow *parent,wxString title,wxPoint point,wxSize size,wxString catid,wxString modid,wxString catname)
:wxDialog(parent, wxID_ANY, title,
                 point, size, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
m_catid(catid),
m_modid(modid),
m_catname(catname)
{
}

bool MyGridDialog::Create()
{
	wxSQLite3Database * db = wxGetApp().db;
	wxString SiteDBPath = glSiteDBPath;
	wxSQLite3ResultSet set;
	if(wxFile::Exists(SiteDBPath) == false)
	{
		wxMessageBox(SiteDBPath+"文件不存在!","警告");
		this->Close(true);
		return false;
	}
	if(!db->IsOpen())
		db->Open(SiteDBPath);
	if(!db->TableExists(global_MyModules[m_modid]))
	{
		wxMessageBox(global_MyModules[m_modid]+"数据库表不存在，原因多半是因为您还没有采集过["+m_catname+"]分类！","警告");
		this->Close(true);
		return false;
	}
	set = db->ExecuteQuery(wxString::Format("SELECT count(*) as totalcount FROM '%s' WHERE catid='%s'",global_MyModules[m_modid],m_catid));
	wxFont myfont( 13, 74, 90, 90, false, wxT("宋体"));
	int pagenum = 1;
	long totalcount;
	int itotalcount;
	if(set.NextRow())
	{
		set.GetAsString("totalcount").ToLong(&totalcount);
		itotalcount = (int)totalcount;
		pagenum = itotalcount/MYGRID_DATA_SHOWNUM;
		if(pagenum <= 0)
		{
			pagenum = 1;
		}
		else if(itotalcount % MYGRID_DATA_SHOWNUM != 0)
		{
			pagenum++;
		}
	}
	else
	{
		wxMessageBox(global_MyModules[m_modid]+"数据库表里查询总数时失败！","警告");
		this->Close(true);
		return false;
	}
	set = db->ExecuteQuery(wxString::Format("SELECT * FROM '%s' WHERE catid='%s' limit 0,%d",global_MyModules[m_modid],m_catid,MYGRID_DATA_SHOWNUM));
	int colcount = set.GetColumnCount();
	wxSizer * const sizerTop = new wxBoxSizer(wxVERTICAL);
	m_grid = new wxGrid(this,wxID_ANY,wxDefaultPosition,wxSize(950,500));
	m_grid->CreateGrid( 0, 0 );
	m_grid->AppendRows(100);
	m_grid->AppendCols(colcount);
	int i;
	for(i=0 ; i < colcount; i++)
	{
		m_grid->SetColLabelValue(i,set.GetColumnName(i));
	}
	int j=0;
	while(set.NextRow())
	{
		for(i=0;i < colcount;i++)
		{
			wxString tmpColumnName = set.GetColumnName(i);
			if(tmpColumnName == "content" || tmpColumnName == "company_introduce") //内容太多，就需要裁剪，防止出现卡的情况
				m_grid->SetCellValue(j,i,set.GetAsString(i).Mid(0,45) + "....");
			else
				m_grid->SetCellValue(j,i,set.GetAsString(i));
		}
		j++;
	}
	m_grid->AutoSizeColumn(2,false);
	m_grid->AutoSizeColumn(3,false);
	m_grid->AutoSizeColumn(5,false);
	m_grid->AutoSizeColumn(colcount - 1,false);
	sizerTop->Add(m_grid,0,wxALL,5);
	wxSizer * const hsizer = new wxBoxSizer(wxHORIZONTAL);
	hsizer->Add(new wxButton(this,ID_SHOWDATA_PRE,"上一页"),0,wxALL,5);
	m_page = new wxTextCtrl(this,wxID_ANY,"1",wxDefaultPosition,wxSize(30,20));
	hsizer->Add(m_page,0,wxALL,5);
	m_pagenum = new wxStaticText(this,wxID_ANY,wxString::Format("总共%d页%d条记录",pagenum,itotalcount));
	m_pagenum->SetFont(myfont);
	hsizer->Add(m_pagenum,0,wxALL,5);
	hsizer->Add(new wxButton(this,ID_SHOWDATA_NEXT,"下一页"),0,wxALL,5);
	hsizer->Add(new wxButton(this,ID_SHOWDATA_JUMP,"跳转"),0,wxALL,5);
	hsizer->Add(new wxButton(this,ID_SHOWDATA_DELETE,"删除"),0,wxALL,5);
	sizerTop->Add(hsizer,0,wxALL,5);
	wxSizer * const hsizer2 = new wxBoxSizer(wxHORIZONTAL);
	wxString choices[] =
    {
		"清除["+m_catname+"]分类的所有本地采集数据", "清除["+glMyModuleNames[m_modid]+"]模块的所有本地采集数据",
		"清楚本网站的所有本地采集数据"
    };
	m_radio = new wxRadioBox(this, ID_RADIOBOX, wxT("清除可选操作："),
                             wxPoint(10,10), wxDefaultSize,
                             WXSIZEOF(choices), choices,
                             1, wxRA_SPECIFY_ROWS ); 
	hsizer2->Add(m_radio,0,wxALL,5);
	hsizer2->Add(new wxButton(this,ID_SHOWDATA_CLEARBTN,"清除"),0,wxALL,5);
	sizerTop->Add(hsizer2,0,wxALL,5);
	SetSizerAndFit(sizerTop);
	SetInitialSize(wxSize(950,650));
	return true;
}

void MyGridDialog::OnClickPre(wxCommandEvent & event)
{
	wxString page = m_page->GetValue();
	long lpage;
	int ipage;
	page.ToLong(&lpage);
	ipage = (int)lpage;
	ipage--;
	m_page->SetValue(wxString::Format("%d",ipage));
	RefreshGridData();
}

void MyGridDialog::OnClickNext(wxCommandEvent & event)
{
	wxString page = m_page->GetValue();
	long lpage;
	int ipage;
	page.ToLong(&lpage);
	ipage = (int)lpage;
	ipage++;
	m_page->SetValue(wxString::Format("%d",ipage));
	RefreshGridData();
}

void MyGridDialog::OnClickJump(wxCommandEvent & event)
{
	wxString page = m_page->GetValue();
	long lpage;
	int ipage;
	page.ToLong(&lpage);
	ipage = (int)lpage;
	if(ipage < 1)
	{
		ipage = 1;
	}
	RefreshGridData();
}

void MyGridDialog::OnClickDelete(wxCommandEvent & event)
{
	wxString oldtitle = GetTitle();
	SetTitle(oldtitle + " 数据正在删除中。。。请稍等！！！");
	if ( m_grid->IsSelection() )
    {
        wxGridUpdateLocker locker(m_grid);
        for ( int n = 0; n < m_grid->GetNumberRows();  n++)
        {
            if ( m_grid->IsInSelection( n , 0 ) )
			{
				DeleteData(n);
			}
        }
    }
	RefreshGridData();
	SetTitle(oldtitle);
}

void MyGridDialog::OnClickClearData(wxCommandEvent & event)
{
	wxString radioStr = m_radio->GetString(m_radio->GetSelection());
	wxMessageDialog msgdlg(this,"温馨提示：确定要进行<"+radioStr+">该操作吗\n\n"
				"如果您确定的话，请选择\"是的，开始清除\"\n\n"
				"否则，请选择\"点错了，不好意思\"来取消本次清除操作\n\n",
				"清除前的温馨提示", wxYES_NO);
	msgdlg.SetYesNoLabels("是的，开始清除", "点错了，不好意思");
	int dlgret = msgdlg.ShowModal();
	if(dlgret == wxID_NO)
	{
		return;
	}
	wxSQLite3Database * db = wxGetApp().db;
	wxString SiteDBPath = glSiteDBPath;
	if(wxFile::Exists(SiteDBPath) == false)
	{
		wxMessageBox(SiteDBPath+"文件不存在!","警告");
		this->Close(true);
		return;
	}
	if(!db->IsOpen())
		db->Open(SiteDBPath);
	if(!db->TableExists(global_MyModules[m_modid]))
	{
		wxMessageBox(global_MyModules[m_modid]+"数据库表不存在","警告");
		this->Close(true);
		return;
	}

	if(radioStr == "清除["+m_catname+"]分类的所有本地采集数据")
	{
		db->ExecuteUpdate(wxString::Format("DELETE FROM '%s' WHERE catid='%s'",
								global_MyModules[m_modid],m_catid )
						  );
	}
	else if(radioStr ==  "清除["+glMyModuleNames[m_modid]+"]模块的所有本地采集数据")
	{
		db->ExecuteUpdate(wxString::Format("DELETE FROM '%s'",global_MyModules[m_modid]));
	}
	else if(radioStr == "清楚本网站的所有本地采集数据")
	{
		MyHashString::iterator it;
		wxString key,value;
		for( it = global_MyModules.begin(); it != global_MyModules.end(); ++it )
		{
			key = it->first;
			value = it->second;
			// do something useful with key and value
			if(!db->TableExists(value))
				continue;
			db->ExecuteUpdate(wxString::Format("DELETE FROM '%s'",value) );
		}
	}
	else
	{
		wxMessageBox("您没选择任何选项","警告");
		return;
	}
	RefreshGridData();
}

void MyGridDialog::DeleteData(int row)
{
	wxSQLite3Database * db = wxGetApp().db;
	wxString SiteDBPath = glSiteDBPath;
	if(wxFile::Exists(SiteDBPath) == false)
	{
		wxMessageBox(SiteDBPath+"文件不存在!","警告");
		this->Close(true);
		return;
	}
	if(!db->IsOpen())
		db->Open(SiteDBPath);
	if(!db->TableExists(global_MyModules[m_modid]))
	{
		wxMessageBox(global_MyModules[m_modid]+"数据库表不存在","警告");
		this->Close(true);
		return;
	}
	wxString id = m_grid->GetCellValue(row,0);
	if(id == "")
	{
		//long LId;
		//id.ToLong(&LId);
		//wxMessageBox(wxString::Format("第%d行记录不存在，没什么可以删除的！",(int)LId+1),"警告");
		return;
	}
	db->ExecuteUpdate(wxString::Format("DELETE FROM '%s' WHERE id='%s'",
							global_MyModules[m_modid],id));
}

void MyGridDialog::RefreshGridData()
{
	wxString page = m_page->GetValue();
	wxSQLite3Database * db = wxGetApp().db;
	wxString SiteDBPath = glSiteDBPath;
	wxSQLite3ResultSet set;
	if(wxFile::Exists(SiteDBPath) == false)
	{
		wxMessageBox(SiteDBPath+"文件不存在!","警告");
		this->Close(true);
		return;
	}
	if(!db->IsOpen())
		db->Open(SiteDBPath);
	if(!db->TableExists(global_MyModules[m_modid]))
	{
		wxMessageBox(global_MyModules[m_modid]+"数据库表不存在","警告");
		this->Close(true);
		return;
	}
	set = db->ExecuteQuery(wxString::Format("SELECT count(*) as totalcount FROM '%s' WHERE catid='%s'",global_MyModules[m_modid],m_catid));
	int pagenum = 1;
	long totalcount,lpage;
	int itotalcount,ipage;
	if(set.NextRow())
	{
		set.GetAsString("totalcount").ToLong(&totalcount);
		itotalcount = (int)totalcount;
		pagenum = itotalcount/MYGRID_DATA_SHOWNUM;
		if(pagenum <= 0)
		{
			pagenum = 1;
		}
		else if(itotalcount % MYGRID_DATA_SHOWNUM != 0)
		{
			pagenum++;
		}
	}
	else
	{
		wxMessageBox(global_MyModules[m_modid]+"数据库表里查询总数时失败！","警告");
		return;
	}
	page.ToLong(&lpage);
	ipage = (int)lpage;
	if(ipage > pagenum)
	{
		ipage = 1;
	}
	if(ipage < 1)
	{
		ipage = pagenum;
	}
	set = db->ExecuteQuery(wxString::Format("SELECT * FROM '%s' WHERE catid='%s' limit %d,%d",global_MyModules[m_modid],m_catid,(ipage - 1) * MYGRID_DATA_SHOWNUM ,MYGRID_DATA_SHOWNUM));
	int colcount = set.GetColumnCount();
	m_grid->ClearGrid();
	int i=0,j=0;
	while(set.NextRow())
	{
		for(i=0;i < colcount;i++)
		{
			if(i==3)
				m_grid->SetCellValue(j,i,set.GetAsString(i).Mid(0,45) + "....");
			else
				m_grid->SetCellValue(j,i,set.GetAsString(i));
		}
		j++;
	}
	m_page->SetValue(wxString::Format("%d",ipage));
	m_pagenum->SetLabel(wxString::Format("总共%d页%d条记录",pagenum,itotalcount));
}

void MyGridDialog::OnGridChanging(wxGridEvent & event)
{
	int row = event.GetRow(),
        col = event.GetCol();
	if(col == 3)
	{
		event.Veto();
		wxMessageBox("系统不允许修改信息内容字段，请修改其他字段！","警告");
		return;
	}
}

void MyGridDialog::OnGridChanged(wxGridEvent & event)
{
	int row = event.GetRow(),
        col = event.GetCol();
	if(col != 4)
	{
		event.Veto();
		wxMessageBox("当前系统只允许修改第5个state状态字段！","警告");
		return;
	}
	wxSQLite3Database * db = wxGetApp().db;
	wxString SiteDBPath = glSiteDBPath;
	if(wxFile::Exists(SiteDBPath) == false)
	{
		wxMessageBox(SiteDBPath+"文件不存在!","警告");
		this->Close(true);
		return;
	}
	if(!db->IsOpen())
		db->Open(SiteDBPath);
	if(!db->TableExists(global_MyModules[m_modid]))
	{
		wxMessageBox(global_MyModules[m_modid]+"数据库表不存在","警告");
		this->Close(true);
		return;
	}
	wxString id = m_grid->GetCellValue(row,0);
	if(id == "")
	{
		wxMessageBox("该行记录不存在，没什么可以修改的！","警告");
		return;
	}
	wxString colname = m_grid->GetColLabelValue(col);
	db->ExecuteUpdate(wxString::Format("UPDATE '%s' SET '%s'='%s' WHERE catid='%s' AND id='%s'",
							global_MyModules[m_modid],colname,m_grid->GetCellValue(row,col),m_catid,id));
}

bool MyApp::OnInit()
{
	wxImage::AddHandler(new wxPNGHandler);
	MyFrame *frame = new MyFrame( _("智能采集器2013PC桌面版 ") + gl_version_number + _(" 作者:zenglong 由zengl.com提供技术支持"), wxPoint(50, 50), wxSize(980, 650) );
	wxBitmap bitmap;
	bool ok = bitmap.LoadFile(wxT("splash.png"), wxBITMAP_TYPE_PNG); //启动画面
	if (ok)
    {
        new wxSplashScreen(bitmap,
            wxSPLASH_CENTRE_ON_SCREEN|wxSPLASH_TIMEOUT,
            6000, frame, wxID_ANY, wxDefaultPosition, wxDefaultSize,
            wxSIMPLE_BORDER|wxSTAY_ON_TOP);
    }
	frame->Center();
    frame->Show(true);
    SetTopWindow(frame);
	glmainFrame = frame;
	db = new wxSQLite3Database();
	glDB = db;
	settingDB = new wxSQLite3Database();
	GetAllAreas();
    return true;
}

void MyApp::OnFatalException()
{
	//((MyFrame *)GetTopWindow())->OnSave((wxCommandEvent)NULL);
	wxMessageBox(wxT("发生严重异常，已经保存数据，准备退出"),wxT("异常退出警告"));
}

MyTreeCtrl::MyTreeCtrl(wxWindow *parent, const wxWindowID id,
               const wxPoint& pos,
               const wxSize& size,
               long style)
			:wxTreeCtrl(parent, id, pos, size, style)
{
	return;
}

#include "icon1.xpm"
#include "icon3.xpm"
#include "checked.xpm"
#include "unchecked.xpm"
#include "sample.xpm"


MyFrame::MyFrame(const wxString& title, const wxPoint& pos, const wxSize& size)
       : wxFrame(NULL, -1, title, pos, size)
{
	m_mgr.SetManagedWindow(this);
    wxMenu *menuFile = new wxMenu;

	menuFile->Append( ID_MENU_FILE_HELP, _("帮助(&H)") );
    menuFile->Append( ID_About, _("关于(&A)...") );
    menuFile->AppendSeparator();
    menuFile->Append( ID_Quit, _("退出(&E)") );

	wxMenu *PersPective = new wxMenu;
	PersPective->Append( ID_MENU_RESTORE_PERSPECTIVE,_("恢复默认布局(&R)"));
	PersPective->Append( ID_MENU_CLEAR_LOGS,_("清理日志(&C)"));

	wxMenu *Category = new wxMenu;
	Category->Append(ID_MENU_CATEGORY_FIND,_("在左侧分类列表中查找分类\tCtrl+F"));
	Category->Append(ID_MENU_CATEGORY_SELECTALL,_("左侧分类列表全选"));
	Category->Append(ID_MENU_CATEGORY_UNSELECTALL,_("左侧分类列表全不选"));
	Category->Append(ID_MENU_CATEGORY_EXPANDALL,_("左侧分类列表全部展开"));
	Category->Append(ID_MENU_CATEGORY_UNEXPANDALL,_("左侧分类列表全部折叠"));

	wxMenu * ExtraFeature = new wxMenu;
	ExtraFeature->Append(ID_MENU_EXTRA_REVERSE_ORDER,_("倒序发布"),_("倒序发布"),true);
	ExtraFeature->Append(ID_MENU_EXTRA_POST_PENDING,_("发布到待审核"),_("发布到待审核"),true);
	ExtraFeature->Append(ID_MENU_EXTRA_LOCAL_IMG,_("图片下载到本地再上传"),_("将远程图片下载到本地再上传"),true);
	ExtraFeature->Append(ID_MENU_EXTRA_DEBUG,_("开启脚本调试"),_("开启脚本调试"),true);

    wxMenuBar *menuBar = new wxMenuBar;
    menuBar->Append( menuFile, _("文件及帮助(&F)") );
	menuBar->Append( PersPective , _("视图(&P)"));
	menuBar->Append( Category , _("分类列表操作(&C)"));
	menuBar->Append( ExtraFeature , _("采集额外功能(&E)"));

    SetMenuBar( menuBar );

	m_statusbar = new MyStatusBar(5,this,wxID_ANY);
	m_statusbar->SetStatusText("欢迎使用智能采集器",0);
	int statuswidth[5] = {150,100,300,150,150};
	m_statusbar->SetStatusText("事件进度条：",1);
	m_statusbar->SetStatusText("百分比",3);
	m_statusbar->SetStatusText("抓包事件",4);
	m_statusbar->SetStatusWidths(5,statuswidth);
	//wxStatusBarPane statusPane= m_statusbar->GetField(1);
	//statusPane.
	//m_gauge = new wxGauge((wxWindow *)&statusPane,wxID_ANY,100, wxDefaultPosition, wxDefaultSize, wxGA_HORIZONTAL|wxNO_BORDER);
	SetStatusBar((wxStatusBar *)m_statusbar);
	
    //SetStatusText( _("Welcome to wxWidgets!") );
	
	wxFont myfont( 12, 74, 90, 90, false, wxT("宋体"));
	treeCtrl = new MyTreeCtrl(this,IDCaiji_Tree_Ctrl,wxDefaultPosition, wxDefaultSize,wxTR_DEFAULT_STYLE | wxTR_EDIT_LABELS);
	wxIcon icons[4];
    icons[0] = wxIcon(icon3_xpm);
    icons[1] = wxIcon(icon1_xpm);
	//int width  = icons[1].GetWidth(),height = icons[1].GetHeight();
	int width=14,height=20;
	wxImageList* imageList = new wxImageList(width, height);
	imageList->Add(wxIcon(icons[0])); //设置文件夹图标 暂时没用这两个图标，留作空白间隙
	imageList->Add(wxIcon(icons[1])); //设置文件图标 
	treeCtrl->AssignImageList(imageList);
	icons[2] = wxIcon(unchecked_xpm);
	icons[3] = wxIcon(checked_xpm);
	width  = icons[2].GetWidth();
	height = icons[2].GetHeight();
	imageList = new wxImageList(width, height);
	imageList->Add(wxIcon(icons[2])); //设置非选中时的图标
	imageList->Add(wxIcon(icons[3])); //设置选中时的图标
	treeCtrl->AssignStateImageList(imageList); //设置tree控件的状态图标。

	wxTreeItemId rootId = treeCtrl->AddRoot(wxT("没有获取分类"), -1, -1,new MyTreeItemData(wxT(""),wxT(""),wxT(""),wxT("")));
	
	treeCtrl->SetItemState(rootId, 0); //设置初始状态图标为0即非选中时的图标
	treeCtrl->SetItemFont(rootId,myfont);
	                                    
	textForLog = new wxRichTextCtrl(this, -1, _(""),
					  wxDefaultPosition, wxSize(200,150),
					  wxNO_BORDER | wxTE_MULTILINE | wxTE_READONLY);
	textForDebug = new wxTextCtrl(this, ID_TEXT_FOR_DEBUG, _("输入调试命令"),wxDefaultPosition,wxSize(200,20),wxTE_PROCESS_ENTER | wxTE_PROCESS_TAB);
	
	wxAuiToolBar * tb = new wxAuiToolBar(this,wxID_ANY,wxDefaultPosition,wxSize(200,30), wxAUI_TB_DEFAULT_STYLE |
                                         wxAUI_TB_OVERFLOW |
                                         wxAUI_TB_TEXT |
                                         wxAUI_TB_HORZ_TEXT); //工具条
	wxBitmap tb_bmp1 = wxArtProvider::GetBitmap(wxART_NORMAL_FILE, wxART_OTHER, wxSize(16,16));
	textUrlName = new wxTextCtrl(tb, IDUrl_Name_TextCtrl, _("输入网址"),wxDefaultPosition,wxSize(200,20),wxTE_PROCESS_ENTER | wxTE_PROCESS_TAB);
	//textPatternTest = new wxTextCtrl(tb, ID_TEXT_PATTERN, _("输入规则"),wxDefaultPosition,wxSize(450,20),wxTE_PROCESS_TAB);
	//wxButton * urlNameBtn = new wxButton(tb,IDUrlName_Btn,_("获取分类信息"));
	tb->AddControl(textUrlName);
	tb->AddTool(IDUrlName_Btn,_("获取分类信息"),tb_bmp1);
	//tb->AddControl(urlNameBtn);
	tb->AddTool(ID_ITEM_START,_("开始"),tb_bmp1);
	tb->AddTool(ID_ITEM_PAUSE,_("暂停"),tb_bmp1);
	tb->AddTool(ID_ITEM_STOP,_("停止"),tb_bmp1);
	tb->AddTool(ID_ITEM_TEST,_("测试"),tb_bmp1);
	tb->AddTool(ID_ITEM_WEB_OFFICE,_("打开初始化脚本"),tb_bmp1);
	tb->AddTool(ID_ITEM_RESET_INIT,_("重新加载初始化脚本"),tb_bmp1);
	m_checkAutoPass = new wxCheckBox(tb,wxID_ANY,"是否开启自动过滤");
	tb->AddControl(m_checkAutoPass);
	m_toolbar = tb;
	wxAuiToolBar * tb2 = new wxAuiToolBar(this,wxID_ANY,wxDefaultPosition,wxSize(200,30), wxAUI_TB_DEFAULT_STYLE |
                                         wxAUI_TB_OVERFLOW |
                                         wxAUI_TB_TEXT |
                                         wxAUI_TB_HORZ_TEXT);
	m_textpostnum = new wxTextCtrl(tb2,wxID_ANY,_("输入每个分类的采集数量"),wxDefaultPosition,wxSize(200,20));
	tb2->AddControl(m_textpostnum);
	tb2->AddTool(ID_ITEM2_RESULT,_("处理本地采集结果"),tb_bmp1);
	m_checkShowBall = new wxCheckBox(tb2,wxID_ANY,"采集到本地时提示处理");
	tb2->AddControl(m_checkShowBall);
	m_textForTimeOut = new wxTextCtrl(tb2,wxID_ANY,_("60"),wxDefaultPosition,wxSize(40,20));
	tb2->AddControl(m_textForTimeOut);
	tb2->AddControl(new wxStaticText(tb2,wxID_ANY,"秒采集超时"));
	tb2->AddTool(ID_ITEM_SHOW_LOG,_("日志"),tb_bmp1);
	m_checkCaijiCompany = new wxCheckBox(tb2,ID_CHECK_CAIJI_COMPANY,"是否采集公司信息");
	tb2->AddControl(m_checkCaijiCompany);
	tb2->AddControl(new wxStaticText(tb2,wxID_ANY,"  最多采集"));
	m_textForMaxUploadIMG = new wxTextCtrl(tb2,wxID_ANY,_("所有"),wxDefaultPosition,wxSize(40,20));
	tb2->AddControl(m_textForMaxUploadIMG);
	tb2->AddControl(new wxStaticText(tb2,wxID_ANY,"张图片"));
	m_toolbar2 = tb2;
	//tb->AddControl(textPatternTest);
	m_mgr.AddPane(tb,wxAuiPaneInfo().Name(_("textUrlName")).MinSize(200,30).Caption(_("网址输入toolbar")).ToolbarPane().Top().Row(1));
	m_mgr.AddPane(tb2,wxAuiPaneInfo().Name(_("textResult")).MinSize(200,30).Caption(_("结果处理toolbar")).ToolbarPane().Top().Row(2));
	m_mgr.AddPane(treeCtrl, wxAuiPaneInfo().Left().MinSize(260,500).Caption(_("分类面板")));
	m_mgr.AddPane(textForLog, wxAuiPaneInfo().Center().MinSize(650,500).Caption(_("日志面板")).Name(_("text2")));
	m_mgr.AddPane(textForDebug, wxAuiPaneInfo().Bottom().MinSize(200,20).Caption(_("调试命令输入框")).Name(_("text3")));
	m_ProgressBar = new wxGauge(this,wxID_ANY,100);
	m_mgr.AddPane(m_ProgressBar,wxAuiPaneInfo().Bottom().MinSize(710,20).Caption(_("采集总进度: 0%")));
	m_mgr.Update();
	
	m_DefPerspective = m_mgr.SavePerspective();
	m_taskBarIcon = new MyTaskBarIcon();

    // we should be able to show up to 128 characters on recent Windows versions
    // (and 64 on Win9x)
	wxIcon icon(sample_xpm);
    if ( !m_taskBarIcon->SetIcon(icon,
                                 "智能采集器2013PC桌面版 "+gl_version_number+"\n"
								 "作者: zenglong\n"
								 "www.zengl.com提供技术支持\n"
								 "温馨提示：单击隐藏和显示！") )
    {
		wxMessageBox("系统不支持设置托盘图标！","警告");
    }
	SetIcon(sample_xpm);
	mythread = mythread2 = NULL;
	curl_global_init(CURL_GLOBAL_ALL); 
	logfp = fopen("mylogs.txt","w+");
	if(logfp == NULL)
	{
		wxMessageBox("打开日志文件失败！","警告");
	}
	else
		fclose(logfp);
	logfp = NULL;
	zengl_debuglogfile = fopen("zengl_debuglogs.txt","w+");
	m_dlgFind = NULL;
	ZL_EXP_VOID * VM = zenglApi_Open();
	zenglApi_SetFlags(VM,(ZENGL_EXPORT_VM_MAIN_ARG_FLAGS)(ZL_EXP_CP_AF_IN_DEBUG_MODE));
	zenglApi_SetModFunHandle(VM,0,"bltSetInitManageUrl",global_bltSetInitManageUrl);
	zenglApi_SetModFunHandle(VM,0,"bltSetModulePath",global_bltSetModulePath);
	if(zenglApi_Run(VM,"初始化脚本.zl") == -1)
		wxMessageBox(wxString::Format("'初始化脚本.zl'执行失败：%s",zenglApi_GetErrorString(VM)),"警告");
	zenglApi_Close(VM);

	//下面读取config.ini配置文件
	wxFileInputStream is(wxT("config.ini"));
	wxFileConfig *conf = new wxFileConfig(is);
	wxString value;
	conf->Read(_("ToolbarItems/UrlName"),&value);
	textUrlName->SetValue(value);
	conf->Read(_("ToolbarItems/PostNum"),&value);
	m_textpostnum->SetValue(value);
	conf->Read(_("ToolbarItems/TimeOut"),&value);
	m_textForTimeOut->SetValue(value);
	conf->Read(_("ToolbarItems/MaxUploadIMG"),&value);
	m_textForMaxUploadIMG->SetValue(value);
	long tmp;
	conf->Read(_("ToolbarItems/checkShowBall"),&tmp);
	if(tmp == 1)
		m_checkShowBall->SetValue(true);
	conf->Read(_("ToolbarItems/CaijiCompany"),&tmp);
	if(tmp == 1)
		m_checkCaijiCompany->SetValue(true);
	conf->Read(_("ToolbarItems/AutoPass"),&tmp);
	if(tmp == 1)
		m_checkAutoPass->SetValue(true);
	conf->Read(_("ExtraMenus/ReverseOrder"),&tmp);
	if(tmp == 1)
		ExtraFeature->Check(ID_MENU_EXTRA_REVERSE_ORDER,true);
	conf->Read(_("ExtraMenus/PostPending"),&tmp);
	if(tmp == 1)
		ExtraFeature->Check(ID_MENU_EXTRA_POST_PENDING,true);
	conf->Read(_("ExtraMenus/LocalImg"),&tmp);
	if(tmp == 1)
		ExtraFeature->Check(ID_MENU_EXTRA_LOCAL_IMG,true);
	conf->Read(_("ExtraMenus/Debug"),&tmp);
	if(tmp == 1)
		ExtraFeature->Check(ID_MENU_EXTRA_DEBUG,true);
	m_isDebugPause = false;
	delete conf;
}

MyFrame::~MyFrame()
{
	global_MyAreas.Clear();
	m_mgr.UnInit();
	if(mythread != NULL)
	{
		//if(!mythread->IsPaused())
			//mythread->Pause();
		mythread->Delete();
		mythread = NULL;
	}
	if(mythread2 != NULL)
	{
		//if(!mythread2->IsPaused())
			//mythread2->Pause();
		mythread2->Delete();
		mythread2 = NULL;
	}
	curl_global_cleanup();
	//if(gl_isQuerySql_forZenglRun)
	//{
		gl_sqlset_forZenglRun.~wxSQLite3ResultSet(); //这里不释放，会出现内存泄漏，此为全局sql结果集
	//	gl_isQuerySql_forZenglRun = 0;
	//}
	wxGetApp().db->Close();
	delete wxGetApp().db;
	settingDB->Close();
	delete settingDB;
	delete m_taskBarIcon;
	if(logfp != NULL)
	{
		fclose(logfp);
		logfp = NULL;
	}
	if(zengl_debuglogfile != NULL)
	{
		fclose(zengl_debuglogfile);
		zengl_debuglogfile = NULL;
	}
	if(m_dlgFind != NULL)
	{
		delete m_dlgFind;
		m_dlgFind = NULL;
	}

	//下面保存配置信息
	wxFileInputStream is(wxT("config.ini"));
	wxFileConfig *conf = new wxFileConfig(is);
	conf->Write("ToolbarItems/UrlName",textUrlName->GetValue());
	conf->Write("ToolbarItems/PostNum",m_textpostnum->GetValue());
	conf->Write("ToolbarItems/TimeOut",m_textForTimeOut->GetValue());
	conf->Write("ToolbarItems/MaxUploadIMG",m_textForMaxUploadIMG->GetValue());
	conf->Write("ToolbarItems/checkShowBall",m_checkShowBall->IsChecked());
	conf->Write("ToolbarItems/CaijiCompany",m_checkCaijiCompany->IsChecked());
	conf->Write("ToolbarItems/AutoPass",m_checkAutoPass->IsChecked());
	wxMenuBar * menubar = this->GetMenuBar();
	conf->Write("ExtraMenus/ReverseOrder",menubar->IsChecked(ID_MENU_EXTRA_REVERSE_ORDER));
	conf->Write("ExtraMenus/PostPending",menubar->IsChecked(ID_MENU_EXTRA_POST_PENDING));
	conf->Write("ExtraMenus/LocalImg",menubar->IsChecked(ID_MENU_EXTRA_LOCAL_IMG));
	conf->Write("ExtraMenus/Debug",menubar->IsChecked(ID_MENU_EXTRA_DEBUG));
	wxFileOutputStream os(wxT("config.ini")); 
	conf->Save(os);
	os.Close(); 
	delete conf;
}

void MyFrame::OnQuit(wxCommandEvent& WXUNUSED(event))
{
    Close(true);
}

void MyFrame::OnAbout(wxCommandEvent& WXUNUSED(event))
{
	wxString zlversion = wxString::Format("v%d.%d.%d",ZL_EXP_MAJOR_VERSION,ZL_EXP_MINOR_VERSION,ZL_EXP_REVISION);
	wxMessageBox( _("智能采集器2013PC桌面版 ")+ gl_version_number +_("\n本系统采用C++,wxWidgets以及zengl嵌入式编程语言"+zlversion+"开发\n 作者: zenglong\nwww.zengl.com提供技术支持"),
                  _("关于"),
                  wxOK | wxICON_INFORMATION, this );
}

void MyFrame::OnHelp(wxCommandEvent& WXUNUSED(event))
{
	wxExecute("hh.exe help.chm");
}

void MyFrame::OnRestorePespective(wxCommandEvent& WXUNUSED(event))
{
	m_mgr.LoadPerspective(m_DefPerspective);
}

extern "C"
{
	size_t mywxcurl_string_write(void* ptr, size_t size, size_t nmemb, void* pcharbuf)
    {
        size_t iRealSize = size * nmemb;
		//int length;
		if(char_myglStr == NULL)
		{
			char_myglTotalnum = iRealSize ;
			char_myglStr = (char *)malloc(char_myglTotalnum + 1);
			//char_myglStr[0] = NULL;
		}
		else
		{
			char_myglTotalnum += iRealSize;
			char_myglStr = (char *)realloc(char_myglStr , char_myglTotalnum + 1);
		}
		strncpy(char_myglStr+(char_myglTotalnum - iRealSize),(char *)ptr ,iRealSize);
		char_myglStr[char_myglTotalnum] = NULL;
		char_myglTotalnum_forShow = char_myglTotalnum;
		wxCommandEvent eventForSetCurlNum(wxEVT_MY_SET_CURL_NUM,ID_MY_WINDOW);
		wxQueueEvent(glmainFrame->GetEventHandler(),eventForSetCurlNum.Clone());
        return iRealSize;
    }

	size_t mywxcurl_file_write(void *ptr, size_t size, size_t nmemb, void *stream)
	{
		size_t iRealSize = size * nmemb;
		if(char_myglTotalnum == 0)
			char_myglTotalnum = iRealSize ;
		else
			char_myglTotalnum += iRealSize;
		size_t written = fwrite(ptr, size, nmemb, (FILE *)stream);
		char_myglTotalnum_forShow = char_myglTotalnum;
		wxCommandEvent eventForSetCurlNum(wxEVT_MY_SET_CURL_NUM,ID_MY_WINDOW);
		wxQueueEvent(glmainFrame->GetEventHandler(),eventForSetCurlNum.Clone());
		return iRealSize;
	}

	size_t write_data(void *ptr, size_t size, size_t nmemb, void *stream) 

	{

		int written = fwrite(ptr, size, nmemb, (FILE *)stream);

		return written;

	}
}


void MyFrame::OnUrlNameBtn(wxAuiToolBarEvent& WXUNUSED(event))
{
	if(mythread != NULL || mythread2 != NULL)
	{
		wxMessageBox("采集器的线程正在运行中，原因可能是正在获取分类信息或者正在进行采集！\n请等待相关操作结束再重试！","警告");
		return;
	}
	wxString strUrlName = TrimBoth(textUrlName->GetValue());
	if(strUrlName != textUrlName->GetValue())
	{
		textUrlName->SetValue(strUrlName);
	}
	//textForLog->AppendText(_("输入的网址是：")+strUrlName+'\n');
	wxMyLogEvent eventforlog( wxEVT_MY_LOG_EVENT,wxID_ANY,_("输入的网址是：")+strUrlName+",如果您的分类很多，达到上万个时，请设置好超时时间，上万个分类时可以设为1000秒的超时时间，系统也会自动设置，虽然不需要这么长，不过设长点保险，请耐心等待。。。。上万个分类估计需要一两分钟才能将数据传过来！\n",
								MY_RICHTEXT_NORMAL_STYLE);
	wxQueueEvent(glmainFrame->GetEventHandler(),eventforlog.Clone());
	glmainFrame->m_textForTimeOut->SetValue("1000");
	if(mythread == NULL && mythread2 == NULL)
	{
		mythread = new MyThread(this);
		mythread->Create();
		mythread->Run();
	}
	else
	{
		wxMessageBox("采集器的线程正在运行中，原因可能是正在获取分类信息或者正在进行采集！\n请等待相关操作结束再重试！","警告");
	}
    return;
}

void MyFrame::OnTextCtrlEnter(wxCommandEvent& WXUNUSED(event))
{
	wxAuiToolBarEvent event2;
	OnUrlNameBtn(event2);
}

void MyFrame::OnTextDebugEnter(wxCommandEvent& WXUNUSED(event))
{
	if(m_isDebugPause == true && mythread2->IsPaused())
	{
		m_isDebugPause =false;
		mythread2->Resume();
	}
}

void MyFrame::OnMyEvent(wxMyLogEvent& event)
{
	int totalnum = textForLog->GetNumberOfLines();
	if(totalnum > 150) 
	{
		wxCommandEvent eventForClearLogs( wxEVT_MY_CLEARLOGS , ID_MY_WINDOW);
		wxQueueEvent(glmainFrame->GetEventHandler(),eventForClearLogs.Clone());
	}
	/*if(textForLog->GetNumberOfLines() > 150) 
	{
		textForLog->Remove(0, textForLog->GetLineLength(textForLog->GetNumberOfLines()/2) + 1);
		textForLog->SetInsertionPointEnd();
    }*/
	textForLog->SetInsertionPointEnd();
	if(event.m_msgcolorEnum == MY_RICHTEXT_RED_STYLE)
	{
		textForLog->BeginTextColour(*wxRED);
	}
	else if(event.m_msgcolorEnum == MY_RICHTEXT_GREEN_STYLE)
	{
		wxColour green(27,124,11);
		textForLog->BeginTextColour(green);
	}
	else
	{
		wxColour black(0,0,0);
		textForLog->BeginTextColour(black);
	}
	totallogsLength += event.m_strEventMsg.length();
	textForLog->WriteText(event.m_strEventMsg);
	logfp = fopen("mylogs.txt","a");
	if(logfp == NULL)
	{
		wxMessageBox("打开日志文件失败！","警告");
	}
	else
	{
		wxScopedCharBuffer myglStrForUTF8 = event.m_strEventMsg.ToUTF8();
		fwrite(myglStrForUTF8.data(), 1 , myglStrForUTF8.length(),logfp);
		fclose(logfp);
		logfp = NULL;
	}
	//textForLog->AppendText(wxString::Format(_("length is %d res is %s \n"),charlength ,myglStr));
	textForLog->ShowPosition(textForLog->GetLastPosition()); 
	textForLog->ScrollLines(-1);
	//if(event.m_msgcolorEnum != MY_RICHTEXT_NORMAL_STYLE)
	//{
	textForLog->EndTextColour();
	//}
}

void MyFrame::OnClearLogs(wxCommandEvent& event)
{
	int totalnum = textForLog->GetNumberOfLines();
	if(totalnum > 150) 
	{
		int perlength = (float)totallogsLength / (float)totalnum;
		int clearlength = totallogsLength - perlength * 100;
		totallogsLength -= clearlength;
		textForLog->BeginSuppressUndo();
		textForLog->Delete(wxRichTextRange(0, clearlength));
		textForLog->BeginSuppressUndo();
	}
}

void MyFrame::OnClearAll_Logs(wxCommandEvent& event)
{
	textForLog->Clear();
}

void MyFrame::OnFindCategory(wxCommandEvent& event)
{
	if ( m_dlgFind )
    {
        wxDELETE(m_dlgFind);
    }
    else
    {
        m_dlgFind = new wxFindReplaceDialog
                        (
                            this,
                            &m_findData,
                            _("在分类列表中查找分类")
                        );
		m_findData.SetFlags(wxFR_DOWN);
        m_dlgFind->Show(true);
		glIsDialogInShow = true;
    }
}

void MyFrame::OnCategorySelectAll(wxCommandEvent& event)
{
	wxTreeItemId rootId = treeCtrl->GetRootItem();
	wxTreeItemIdValue cookie,cookie2;
	wxTreeItemId itemId = treeCtrl->GetFirstChild(rootId,cookie);
	do{
		if(itemId.IsOk())
		{
			wxTreeItemId childid = treeCtrl->GetFirstChild(itemId,cookie2);
			do{
				if(childid.IsOk())
					treeCtrl->SetItemState(childid,1); //设为1 即为选中状态
			}while(childid = treeCtrl->GetNextChild(childid,cookie2));
		}
	}while(itemId = treeCtrl->GetNextChild(itemId,cookie));
}

void MyFrame::OnCategoryUnSelectAll(wxCommandEvent& event)
{
	wxTreeItemId rootId = treeCtrl->GetRootItem();
	wxTreeItemIdValue cookie,cookie2;
	wxTreeItemId itemId = treeCtrl->GetFirstChild(rootId,cookie);
	do{
		if(itemId.IsOk())
		{
			wxTreeItemId childid = treeCtrl->GetFirstChild(itemId,cookie2);
			do{
				if(childid.IsOk())
					treeCtrl->SetItemState(childid,0); //设为0 即为不选中状态
			}while(childid = treeCtrl->GetNextChild(childid,cookie2));
		}
	}while(itemId = treeCtrl->GetNextChild(itemId,cookie));
}

void MyFrame::OnCategoryExpandAll(wxCommandEvent& event)
{
	treeCtrl->ExpandAll();
	return ;
}

void MyFrame::OnCategoryUnExpandAll(wxCommandEvent& event)
{
	treeCtrl->CollapseAll();
	return ;
}

void MyFrame::OnExtraCheck(wxCommandEvent& event)
{
	int menuid = event.GetId();
	wxMenuBar * menubar = this->GetMenuBar();
	if(menuid == ID_MENU_EXTRA_REVERSE_ORDER)
	{
		if(event.IsChecked() && menubar->IsChecked(ID_MENU_EXTRA_POST_PENDING))
		{
			wxMessageDialog msgdlg(this,"您已勾选了发布到待审核\n\n待审核时，DT后台审核数据后，更新时间很可能又会倒回来\n\n一般[供应] [商城]等不建议同时勾选倒序和待审核"
						"\n\n不过[团购]等又可以同时勾选(由DT系统决定)，所以请根据自己网站的实际情况来决定!",
						"倒序发布 - 温馨提示：", wxYES_NO | wxCANCEL);
			msgdlg.SetYesNoCancelLabels("确定同时勾选", "取消待审核只倒序" , "取消本次操作");
			int dlgret = msgdlg.ShowModal();
			if(dlgret==wxID_YES)
			{
				menubar->Check(menuid,event.IsChecked());
				wxMyLogEvent eventlog( wxEVT_MY_LOG_EVENT,wxID_ANY,_("勾选了倒序发布，同时发布到待审核\n"),MY_RICHTEXT_NORMAL_STYLE);
				wxQueueEvent(this->GetEventHandler(),eventlog.Clone());
				return ;
			}
			else if(dlgret == wxID_CANCEL)
			{
				wxMyLogEvent eventlog( wxEVT_MY_LOG_EVENT,wxID_ANY,_("取消倒序发布操作\n"),MY_RICHTEXT_NORMAL_STYLE);
				wxQueueEvent(this->GetEventHandler(),eventlog.Clone());
				menubar->Check(menuid,false);
				return ;
			}
			else //取消待审核只倒序
			{
				menubar->Check(menuid,event.IsChecked());
				menubar->Check(ID_MENU_EXTRA_POST_PENDING,false);
				wxMyLogEvent eventlog( wxEVT_MY_LOG_EVENT,wxID_ANY,_("勾选了倒序发布，取消发布到待审核\n"),MY_RICHTEXT_NORMAL_STYLE);
				wxQueueEvent(this->GetEventHandler(),eventlog.Clone());
				return;
			}
		}
		else
		{
			menubar->Check(menuid,event.IsChecked());
			wxString printStr;
			if(event.IsChecked())
				printStr = _("勾选了倒序发布\n");
			else
				printStr = _("取消倒序发布\n");
			wxMyLogEvent eventlog( wxEVT_MY_LOG_EVENT,wxID_ANY,printStr,MY_RICHTEXT_NORMAL_STYLE);
			wxQueueEvent(this->GetEventHandler(),eventlog.Clone());
			return ;
		}
	}
	else if(menuid == ID_MENU_EXTRA_POST_PENDING)
	{
		if(event.IsChecked() && menubar->IsChecked(ID_MENU_EXTRA_REVERSE_ORDER))
		{
			wxMessageDialog msgdlg(this,"您已勾选了倒序发布\n\n待审核时，DT后台审核数据后，更新时间很可能又会倒回来\n\n一般[供应] [商城]等不建议同时勾选倒序和待审核"
						"\n\n不过[团购]等又可以同时勾选(由DT系统决定)，所以请根据自己网站的实际情况来决定!",
						"发布到待审核 - 温馨提示", wxYES_NO | wxCANCEL);
			msgdlg.SetYesNoCancelLabels("确定同时勾选", "取消倒序只待审核" , "取消本次操作");
			int dlgret = msgdlg.ShowModal();
			if(dlgret==wxID_YES)
			{
				menubar->Check(menuid,event.IsChecked());
				wxMyLogEvent eventlog( wxEVT_MY_LOG_EVENT,wxID_ANY,_("勾选了发布到待审核，同时倒序发布\n"),MY_RICHTEXT_NORMAL_STYLE);
				wxQueueEvent(this->GetEventHandler(),eventlog.Clone());
				return ;
			}
			else if(dlgret == wxID_CANCEL)
			{
				wxMyLogEvent eventlog( wxEVT_MY_LOG_EVENT,wxID_ANY,_("取消发布到待审核操作\n"),MY_RICHTEXT_NORMAL_STYLE);
				wxQueueEvent(this->GetEventHandler(),eventlog.Clone());
				menubar->Check(menuid,false);
				return ;
			}
			else //取消倒序只待审核
			{
				menubar->Check(menuid,event.IsChecked());
				menubar->Check(ID_MENU_EXTRA_REVERSE_ORDER,false);
				wxMyLogEvent eventlog( wxEVT_MY_LOG_EVENT,wxID_ANY,_("勾选了发布到待审核，取消倒序发布\n"),MY_RICHTEXT_NORMAL_STYLE);
				wxQueueEvent(this->GetEventHandler(),eventlog.Clone());
				return ;
			}
		}
		else
		{
			menubar->Check(menuid,event.IsChecked());
			wxString printStr;
			if(event.IsChecked())
				printStr = _("勾选了发布到待审核\n");
			else
				printStr = _("取消发布到待审核\n");
			wxMyLogEvent eventlog( wxEVT_MY_LOG_EVENT,wxID_ANY,printStr,MY_RICHTEXT_NORMAL_STYLE);
			wxQueueEvent(this->GetEventHandler(),eventlog.Clone());
			return ;
		}
	}
	else if(menuid == ID_MENU_EXTRA_LOCAL_IMG)
	{
		menubar->Check(menuid,event.IsChecked());
		wxString printStr;
		if(event.IsChecked())
			printStr = _("勾选了图片下载到本地再上传\n");
		else
			printStr = _("取消图片下载到本地再上传，取消后，图片将由您的网站服务器远程下载\n");
		wxMyLogEvent eventlog( wxEVT_MY_LOG_EVENT,wxID_ANY,printStr,MY_RICHTEXT_NORMAL_STYLE);
		wxQueueEvent(this->GetEventHandler(),eventlog.Clone());
		return ;
	}
	else if(menuid  == ID_MENU_EXTRA_DEBUG)
	{
		menubar->Check(menuid,event.IsChecked());
		wxString printStr;
		if(event.IsChecked())
			printStr = _("勾选了开启脚本调试，采集脚本执行时就会在一开始发生中断，此时可以在调试输入框中输入相关调试命令(例如可以用h命令来查看具体的调试帮助信息)\n");
		else
			printStr = _("用户取消了脚本调试\n");
		wxMyLogEvent eventlog( wxEVT_MY_LOG_EVENT,wxID_ANY,printStr,MY_RICHTEXT_NORMAL_STYLE);
		wxQueueEvent(this->GetEventHandler(),eventlog.Clone());
		return ;
	}
}

wxTreeItemId MyFrame::findTreeItem(wxTreeCtrl* pTreeCtrl, const wxTreeItemId& root, 
								   const wxString& text, bool bIsNext,bool bCaseSensitive, bool bExactMatch) //在分类树中进行查找
{
	wxTreeItemId item=root, child,parent;
	wxTreeItemIdValue cookie;
	wxString findtext(text), itemtext;
	bool bFound;
	if(!bCaseSensitive) findtext.MakeLower();
 
	do
	{
		while(item.IsOk())
		{
			itemtext = pTreeCtrl->GetItemText(item);
			if(!bCaseSensitive) itemtext.MakeLower();
			bFound = bExactMatch ? (itemtext == findtext) : itemtext.Contains(findtext);
			if(bFound) return item;
			child = pTreeCtrl->GetFirstChild(item, cookie);
			if(child.IsOk()) child = findTreeItem(pTreeCtrl, child, text,bIsNext, bCaseSensitive, bExactMatch);
			if(child.IsOk()) return child;
			if(bIsNext)
				item = pTreeCtrl->GetNextSibling(item);
			else
				item = pTreeCtrl->GetPrevSibling(item);
		} // while(item.IsOk())
		parent = pTreeCtrl->GetItemParent(item);
		if(parent.IsOk())
		{
			if(bIsNext)
				item = pTreeCtrl->GetNextSibling(parent);
			else
				item = pTreeCtrl->GetPrevSibling(parent);
		}
		else
			break;
	}while(item.IsOk());
 
	return item;
}

void MyFrame::OnFindDialog(wxFindDialogEvent& event)
{
    wxEventType type = event.GetEventType();

    if ( type == wxEVT_COMMAND_FIND || type == wxEVT_COMMAND_FIND_NEXT )
    {
		wxTreeItemId item,item2,rootid,parentid;
		wxTreeItemIdValue cookie;
        wxString findText = event.GetFindString();
		if(findText == "")
			return;
		item=treeCtrl->GetFocusedItem();
		rootid = treeCtrl->GetRootItem();
		if(!item.IsOk())
			item = rootid;
		if(!treeCtrl->GetCount()) return; // empty tree control - i.e. just cleared it?
		wxFindReplaceFlags findFlags = (wxFindReplaceFlags)event.GetFlags();
		bool bIsNext = findFlags & wxFR_DOWN ? true : false;
		bool bCaseSensitive = findFlags & wxFR_MATCHCASE ? true : false;
		bool bExactMatch = findFlags & wxFR_WHOLEWORD ? true : false;
		wxString itemtext;
		bool bFound;
		if(!bCaseSensitive) findText.MakeLower();
		do
		{
			if(treeCtrl->HasChildren(item))
			{
				if(bIsNext)
				{
					item = treeCtrl->GetFirstChild(item, cookie);
					if(!item.IsOk())
						goto not_found_loc;
				}
				else
				{
					parentid = treeCtrl->GetPrevSibling(item);
					if(!parentid.IsOk())
						goto not_found_loc;
					if(treeCtrl->HasChildren(parentid))
					{
						item = treeCtrl->GetLastChild(parentid);
						if(!item.IsOk())
							goto not_found_loc;
					}
					else
						item = parentid;
				}
			}
			else
			{
				if(bIsNext)
				{
					item2 = treeCtrl->GetNextSibling(item);
					if(!item2.IsOk())
					{
						parentid = treeCtrl->GetNextSibling(treeCtrl->GetItemParent(item));
						if(parentid.IsOk())
							item = parentid;
						else
							goto not_found_loc;
					}
					else
						item = item2;
				}
				else
				{
					item2 = treeCtrl->GetPrevSibling(item);
					if(!item2.IsOk())
					{
						parentid = treeCtrl->GetItemParent(item);
						if(parentid.IsOk())
							item = parentid;
						else
							goto not_found_loc;
					}
					else
						item = item2;
				}
			}
			itemtext = treeCtrl->GetItemText(item);
			if(!bCaseSensitive) itemtext.MakeLower();
			bFound = bExactMatch ? (itemtext == findText) : itemtext.Contains(findText);
			if(bFound) 
			{
				treeCtrl->SelectItem(item, true);
				treeCtrl->EnsureVisible(item);
				treeCtrl->SetFocus();
				return;
			}
		}while(item.IsOk());
		//item = findTreeItem(treeCtrl, item, findText, bIsNext , bCaseSensitive, bExactMatch);
		if(!item.IsOk()) 
		{
not_found_loc:
			wxMessageBox("没找到该分类","系统提示");
			return;
		}
    }
    else if ( type == wxEVT_COMMAND_FIND_CLOSE )
    {
		wxFindReplaceDialog *dlg = event.GetDialog();
		dlg->Destroy();
        wxDELETE(m_dlgFind);
		m_dlgFind = NULL;
		glIsDialogInShow = false;
    }
    else
    {
        wxMessageBox(wxT("未知的对话框查找事件!"),"错误警告");
    }
}

void MyFrame::OnClickCheckCaijiCompany(wxCommandEvent& event)
{
	if(m_checkCaijiCompany->IsChecked())
	{
		wxMyLogEvent eventforlog( wxEVT_MY_LOG_EVENT,wxID_ANY);
		eventforlog.SetEventMsg(_("您勾选了采集公司，则在供应,商城,团购,招商,品牌,生意宝求购模块采集时会自动采集公司,每采集一个公司就会增加一个会员，所以请根据实际情况来定，采集过多公司会造成较多的非真实用户(因为这些用户是采集生成的，而不是别人注册的！)\n"),
			MY_RICHTEXT_NORMAL_STYLE);
		wxQueueEvent(glmainFrame->GetEventHandler(),eventforlog.Clone());
	}
}

void MyFrame::OnCaijiProgress(wxMyProgressEvent& event)
{
	int percent = 0;
	wxString percentStr="";
	if(m_statusbar->m_ProgressBar->GetRange() != event.m_processNum * 2)
		m_statusbar->m_ProgressBar->SetRange(event.m_processNum * 2);
	m_statusbar->m_ProgressBar->SetValue(event.m_processPos);
	percent = ((float)event.m_processPos/(float)(event.m_processNum * 2)) * 100;
	percentStr.Printf("百分比：%d%%",percent);
	m_statusbar->SetStatusText(percentStr,3);
	if(m_ProgressBar->GetRange() != event.m_total_processNum * 2)
		m_ProgressBar->SetRange(event.m_total_processNum * 2);
	m_ProgressBar->SetValue(event.m_total_processPos);
	percent = ((float)event.m_total_processPos/(float)(event.m_total_processNum * 2)) * 100;
	m_mgr.GetPane(m_ProgressBar).Caption(wxString::Format("采集总进度: %d%%",percent));
	m_mgr.Update();
}

void MyFrame::OnPauseThread(wxCommandEvent& event)
{
	//textForLog->AppendText(_("系统暂停！\n"));
	wxMyLogEvent eventforlog( wxEVT_MY_LOG_EVENT,wxID_ANY,_("系统暂停！可点击工具栏的[继续]按钮来继续...\n"),
								MY_RICHTEXT_NORMAL_STYLE);
	wxQueueEvent(glmainFrame->GetEventHandler(),eventforlog.Clone());
	if(mythread2 != NULL && !mythread2->IsPaused())
	{
		mythread2->Pause();
		wxAuiToolBarItem *item = m_toolbar->FindTool(ID_ITEM_START);
		item->SetLabel("继续");
		m_toolbar->Refresh(true); //刷新工具栏，让继续按钮得以显示出来
		m_taskBarIcon->ShowBalloon("采集数据到本地数据库结束，系统暂停，您可以处理本地数据再上传！","温馨提示",NIIF_INFO,
												10000);
	}
}

void MyFrame::OnDebugPauseThread(wxCommandEvent& event)
{
	//textForLog->AppendText(_("系统暂停！\n"));
	wxMyLogEvent eventforlog( wxEVT_MY_LOG_EVENT,wxID_ANY,_("\n>>> debug input:"),
								MY_RICHTEXT_NORMAL_STYLE);
	wxQueueEvent(glmainFrame->GetEventHandler(),eventforlog.Clone());
	if(m_isDebugPause == false && mythread2 != NULL && !mythread2->IsPaused())
	{
		m_isDebugPause = true;
		mythread2->Pause();
	}
}

void MyFrame::OnSetCurlNum(wxCommandEvent& event)
{
	if(char_myglTotalnum_forShow > 0)
		m_statusbar->SetStatusText(wxString::Format("抓包数据:%d字节",char_myglTotalnum_forShow),4);
}

void MyFrame::OnUpdateCate(wxCommandEvent& event)
{
	wxTreeItemId rootId = treeCtrl->GetRootItem();
	wxFont myfont( 12, 74, 90, 90, false, wxT("宋体"));
	wxSQLite3Database *db = wxGetApp().db;
	wxString SiteName = textUrlName->GetValue();
	wxString SiteDBPath = "db/" + glEscapeSiteName + ".db";
	if(!db->IsOpen())
		db->Open(SiteDBPath);
	
	wxSQLite3ResultSet set = db->ExecuteQuery(wxT("SELECT url,chname,type FROM website where id = 1"));
	if(!set.NextRow())
	{
		//textForLog->AppendText(_("从数据库website获取网站信息失败！\n"));
		//textForLog->ShowPosition(textForLog->GetLastPosition()); 
		//textForLog->ScrollLines(-1);
		wxMyLogEvent eventforlog( wxEVT_MY_LOG_EVENT,wxID_ANY,_("从数据库website获取网站信息失败！\n"),
								MY_RICHTEXT_RED_STYLE);
		wxQueueEvent(glmainFrame->GetEventHandler(),eventforlog.Clone());
		return;
	}
	wxString myurl = set.GetAsString("url");
	SiteName = TrimBoth(set.GetAsString(1));
	wxXmlDocument doc2;
	if (!doc2.Load("db/"+SiteName+".xml"))
	{
		/*textForLog->AppendText("加载db/"+SiteName+".xml文件失败\n");
		textForLog->ShowPosition(textForLog->GetLastPosition()); 
		textForLog->ScrollLines(-1);*/
		wxMyLogEvent eventforlog( wxEVT_MY_LOG_EVENT,wxID_ANY,"加载db/"+SiteName+".xml文件失败\n",
								MY_RICHTEXT_RED_STYLE);
		wxQueueEvent(glmainFrame->GetEventHandler(),eventforlog.Clone());
		return;
	}
	wxXmlNode * root2 = doc2.GetRoot();
	wxXmlNode *child2 = root2->GetChildren();
	if(global_MyModules.size() > 0)
	{
		global_MyModules.clear();
		glMyModuleNames.clear();
	}
	while(child2)
	{
		global_MyModules[child2->GetAttribute("id")] = child2->GetAttribute("module");
		glMyModuleNames[child2->GetAttribute("id")] = child2->GetAttribute("name");
		child2 = child2->GetNext();
	}
	treeCtrl->Delete(rootId);
	rootId = treeCtrl->AddRoot(set.GetAsString(1), -1, -1,
			new MyTreeItemData(wxT(""),_(""),_(""),set.GetAsString(0)));
	glCaijiWebName = set.GetAsString(1);
	glSiteDBPath = SiteDBPath;
	treeCtrl->SetItemState(rootId, 0); //设置初始状态图标为0即非选中时的图标
	treeCtrl->SetItemFont(rootId,myfont);
	wxTreeItemId itemId , parentItemid;
	wxCommandEvent eventForProgress( wxEVT_MY_PROGRESS , ID_MY_WINDOW);
	set = db->ExecuteQuery(wxT("SELECT count(*)as TotalCatNum FROM category"));
	set.NextRow();
	int totalcount,pernum=0;
	set.GetAsString("TotalCatNum").ToLong((long *)&totalcount);
	m_statusbar->m_total = totalcount;
	set = db->ExecuteQuery(wxT("SELECT catid,modid,catname FROM category"));
	wxString strModuleID = "0";
	while(set.NextRow())
	{
		if(strModuleID != set.GetAsString(1)) //将模块分类分割开
		{
			itemId = treeCtrl->AppendItem(rootId,
										glMyModuleNames[set.GetAsString(1)], -1, -1,
										new MyTreeItemData(set.GetAsString(1),"0","",""));
			treeCtrl->SetItemState(itemId, 0); //设置初始状态图标为0即非选中时的图标
			treeCtrl->SetItemFont(itemId,myfont);
			parentItemid = itemId ;
		}
		itemId = treeCtrl->AppendItem(parentItemid,
									set.GetAsString(2) + "<"+glMyModuleNames[set.GetAsString(1)]+">", -1, -1,
									new MyTreeItemData(set.GetAsString(1),set.GetAsString(0),set.GetAsString(2),""));
		treeCtrl->SetItemState(itemId, 0); //设置初始状态图标为0即非选中时的图标
		treeCtrl->SetItemFont(itemId,myfont);
		pernum++;
		if(pernum==1)
			treeCtrl->ExpandAll();
		strModuleID = set.GetAsString(1);
		m_statusbar->m_pos = pernum; //8 task
		wxQueueEvent(m_statusbar->GetEventHandler(),eventForProgress.Clone());
	}
	treeCtrl->ExpandAll();
	m_mgr.GetPane(treeCtrl).Caption(wxString::Format("分类面板 一共加载%d个分类",totalcount));
	m_mgr.Update();
	//treeCtrl->Fit();
	//m_mgr.Update();
	/*textForLog->AppendText("网站"+SiteName+"加载成功！\n");
	textForLog->ShowPosition(textForLog->GetLastPosition()); 
	textForLog->ScrollLines(-1); */
	wxMyLogEvent eventforlog( wxEVT_MY_LOG_EVENT,wxID_ANY,"网站"+SiteName+"加载成功！\n",
								MY_RICHTEXT_NORMAL_STYLE);
	wxQueueEvent(glmainFrame->GetEventHandler(),eventforlog.Clone());

	/*wxXmlDocument doc;
	if (!doc.Load("test.xml"))
	{
		textForLog->AppendText(_("加载test.xml文件失败\n"));
		//return NULL;
	}
	wxXmlNode *root = doc.GetRoot();
	rootId = treeCtrl->AddRoot(root->GetAttribute(_("name")), -1, -1,
                         new MyTreeItemData(wxT(""),_(""),root->GetAttribute(_("url")) ));
	treeCtrl->SetItemState(rootId, 0); //设置初始状态图标为0即非选中时的图标
	treeCtrl->SetItemFont(rootId,myfont);
	wxXmlNode *child = root->GetChildren();
	wxTreeItemId itemId;
	while(child)
	{
		itemId = treeCtrl->AppendItem(rootId,
                         child->GetAttribute(_("name")), -1, -1,
                         new MyTreeItemData(child->GetAttribute(_("modid")),child->GetAttribute(_("catid")),child->GetAttribute(_("")))
						 );
		treeCtrl->SetItemState(itemId, 0); //设置初始状态图标为0即非选中时的图标
		treeCtrl->SetItemFont(itemId,myfont);
		child = child->GetNext();
	}*/
}

void MyFrame::OnChecks(wxCommandEvent& event) //暂没使用
{
	wxTreeItemId rootId = treeCtrl->GetRootItem();
	wxTreeItemIdValue cookie;
	wxTreeItemId itemId = treeCtrl->GetFirstChild(rootId,cookie);
	do{
		treeCtrl->SetItemState(itemId,wxTREE_ITEMSTATE_NEXT);
	}while(itemId = treeCtrl->GetNextChild(itemId,cookie));
}

void MyFrame::OnClickStart(wxAuiToolBarEvent &event)
{
	wxString strStartLabel =m_toolbar->FindTool(ID_ITEM_START)->GetLabel();
	if(strStartLabel == "运行中")
	{
		//textForLog->AppendText(_("\n系统已经在运行状态中！\n"));
		//textForLog->ShowPosition(textForLog->GetLastPosition()); 
		//textForLog->ScrollLines(-1);
		wxMyLogEvent eventforlog( wxEVT_MY_LOG_EVENT,wxID_ANY,_("\n系统已经在运行状态中！\n"),MY_RICHTEXT_NORMAL_STYLE);
		wxQueueEvent(glmainFrame->GetEventHandler(),eventforlog.Clone());
		return;
	}
	else if(strStartLabel == "继续")
	{
		/*textForLog->AppendText(_("\n用户点击了继续，系统继续执行\n"));
		textForLog->ShowPosition(textForLog->GetLastPosition()); 
		textForLog->ScrollLines(-1);*/
		wxMyLogEvent eventforlog( wxEVT_MY_LOG_EVENT,wxID_ANY,_("\n用户点击了继续，系统继续执行\n"),MY_RICHTEXT_NORMAL_STYLE);
		wxQueueEvent(glmainFrame->GetEventHandler(),eventforlog.Clone());
	}
	if(single_mod_cat_table == "")
	{
		//textForLog->AppendText(_("点击了开始：")+glCaijiWebName+'\n');
		wxMyLogEvent eventforlog( wxEVT_MY_LOG_EVENT,wxID_ANY,_("点击了开始：")+glCaijiWebName+'\n',MY_RICHTEXT_NORMAL_STYLE);
		wxQueueEvent(glmainFrame->GetEventHandler(),eventforlog.Clone());
		if(glCaijiWebName == "" || glSiteDBPath == "")
		{
			wxMessageBox("您还没设置要采集的网站或者还没从网站获取分类信息！","警告");
			return;
		}
		if(m_textpostnum->GetValue()=="" || !m_textpostnum->GetValue().IsNumber())
		{
			wxMessageBox("请输入每个分类要采集的数量\n必须是整数，不要太大哦！","警告");
			m_textpostnum->SetValue("");
			m_textpostnum->SetFocus();
			return;
		}
		long caijinum;
		m_textpostnum->GetValue().ToLong(&caijinum);
		if(caijinum <= 0)
		{
			wxMessageBox("采集的数量必须大于0，不要太大哦！","警告");
			m_textpostnum->SetValue("");
			m_textpostnum->SetFocus();
			return;
		}
		if(m_checkShowBall->IsChecked()==false)
		{
			wxMessageDialog msgdlg(this,"温馨提示：是否需要采集数据到本地时的提醒服务\n\n"
						"使用本功能可以在数据采集到本地时提醒您，并暂停采集\n\n"
						"随后您可以使用工具栏的处理采集结果按钮来处理本地数据\n\n"
						"处理完后再上传，可以删除采集到的不需要的信息\n\n"
						"您也可以手动在工具栏点击采集数据到本地时提醒复选框\n\n"
						"本对话框只在没选中复选框时自动弹出！",
						"采集前的温馨提示", wxYES_NO | wxCANCEL);
			msgdlg.SetYesNoCancelLabels("不用，谢谢", "OK，好的" , "取消本次采集操作");
			int dlgret = msgdlg.ShowModal();
			if(dlgret==wxID_NO)
			{
				m_checkShowBall->SetValue(true);
			}
			else if(dlgret == wxID_CANCEL)
			{
				return;
			}
		}
		else
		{
			wxMessageDialog msgdlg(this,"温馨提示：您要采集的网站是["+glCaijiWebName+"]\n\n"
						"您设置了每个分类的最大信息采集数量为"+m_textpostnum->GetValue()+"\n\n"
						"您还手动设置了采集数据到本地时的提醒服务\n\n"
						"如果您确定这些信息正确的话，请选择\"是的，开始采集\"\n\n"
						"否则，请选择\"点错了，不好意思\"来取消本次采集操作\n\n",
						"采集前的温馨提示", wxYES_NO);
			msgdlg.SetYesNoLabels("是的，开始采集", "点错了，不好意思");
			int dlgret = msgdlg.ShowModal();
			if(dlgret == wxID_NO)
			{
				return;
			}
		}
	}
	if(mythread2 == NULL)
	{
		mythread2 = new MyThread2(this);
		mythread2->Create();
		mythread2->Run();
		wxAuiToolBarItem *item = m_toolbar->FindTool(ID_ITEM_START);
		item->SetLabel("运行中");
	}
	else
	{
		if(mythread2->IsPaused())
			mythread2->Resume();
		wxAuiToolBarItem *item = m_toolbar->FindTool(ID_ITEM_START);
		item->SetLabel("运行中");
	}
	m_mgr.Update();
	return;
}

void MyFrame::OnClickPause(wxAuiToolBarEvent &event)
{
	if(curl == NULL)
	{
		//textForLog->AppendText(_("点击了暂停，系统准备暂停\n"));
		wxMyLogEvent eventforlog( wxEVT_MY_LOG_EVENT,wxID_ANY,_("点击了暂停，系统准备暂停\n"),MY_RICHTEXT_NORMAL_STYLE);
		wxQueueEvent(glmainFrame->GetEventHandler(),eventforlog.Clone());
	}
	else
	{
		//textForLog->AppendText(_("点击了暂停，但是当前系统正在进行抓包工作，所以要等当前连接完成或超时的时候才暂停！\n"));
		wxMyLogEvent eventforlog( wxEVT_MY_LOG_EVENT,wxID_ANY,_("点击了暂停，但是当前系统正在进行抓包工作，所以要等当前连接完成或超时的时候才暂停！\n"),
						MY_RICHTEXT_NORMAL_STYLE);
		wxQueueEvent(glmainFrame->GetEventHandler(),eventforlog.Clone());
	}
	//textForLog->ShowPosition(textForLog->GetLastPosition()); 
	//textForLog->ScrollLines(-1);
	if(mythread2 != NULL && !mythread2->IsPaused())
	{
		mythread2->Pause();
		wxAuiToolBarItem *item = m_toolbar->FindTool(ID_ITEM_START);
		item->SetLabel("继续");
		m_mgr.Update();
	}
}

void MyFrame::OnClickStop(wxAuiToolBarEvent &event)
{
	if(curl == NULL)
	{
		//textForLog->AppendText(_("点击了停止，系统准备停止\n"));
		wxMyLogEvent eventforlog( wxEVT_MY_LOG_EVENT,wxID_ANY,_("点击了停止，系统准备停止\n"),MY_RICHTEXT_NORMAL_STYLE);
		wxQueueEvent(glmainFrame->GetEventHandler(),eventforlog.Clone());
	}
	else
	{
		//textForLog->AppendText(_("点击了停止，但是当前系统正在进行抓包工作，所以要等当前连接完成或超时的时候才停止！\n"));
		wxMyLogEvent eventforlog( wxEVT_MY_LOG_EVENT,wxID_ANY,_("点击了停止，但是当前系统正在进行抓包工作，所以要等当前连接完成或超时的时候才停止！\n"),
					MY_RICHTEXT_NORMAL_STYLE);
		wxQueueEvent(glmainFrame->GetEventHandler(),eventforlog.Clone());
	}
	//textForLog->ShowPosition(textForLog->GetLastPosition()); 
	//textForLog->ScrollLines(-1);
	if(mythread2 != NULL)
	{
		if(CUR_CAIJI_VM != NULL)
			zenglApi_Stop(CUR_CAIJI_VM);
		if(!mythread2->IsPaused())
			mythread2->Pause();
		mythread2->Delete();
		mythread2 = NULL;
		wxAuiToolBarItem *item = m_toolbar->FindTool(ID_ITEM_START);
		item->SetLabel("开始");
		/*textForLog->AppendText(wxString::Format("\n分类[%s]一共采集了%d条信息！发布成功%d条信息//////////////////////////////\n",
								single_mod_catname,single_mod_caijinum,single_mod_curpostnum));*/
		wxMyLogEvent eventforlog( wxEVT_MY_LOG_EVENT,wxID_ANY,wxString::Format("\n分类[%s]一共采集了%d条信息！发布成功%d条信息//////////////////////////////\n",single_mod_catname,single_mod_caijinum,single_mod_curpostnum),
					MY_RICHTEXT_NORMAL_STYLE);
		wxQueueEvent(glmainFrame->GetEventHandler(),eventforlog.Clone());
		wxTreeItemId rootid = treeCtrl->GetRootItem();
		wxString webname = treeCtrl->GetItemText(rootid);
		eventforlog.SetEventMsg(wxString::Format("\n网站[%s]一共采集了%d条信息！发布成功%d条信息//////////////////////////////\n",
								webname,global_mod_totalcaijinum,global_mod_totalpostnum),MY_RICHTEXT_NORMAL_STYLE);
		wxQueueEvent(glmainFrame->GetEventHandler(),eventforlog.Clone());
		/*textForLog->AppendText(wxString::Format("\n网站[%s]一共采集了%d条信息！发布成功%d条信息//////////////////////////////\n",
								webname,global_mod_totalcaijinum,global_mod_totalpostnum));*/
		//textForLog->ShowPosition(textForLog->GetLastPosition()); 
		//textForLog->ScrollLines(-1);
		global_mod_processNum = 0;
		global_mod_processPos = 0;
		global_mod_totalpostnum = 0;
		global_mod_totalcaijinum = 0;
		single_mod_processNum = 0;
		single_mod_processPos = 0;
		single_mod_curpostnum = 0;
		single_mod_caijinum = 0;
		single_mod_cat_table = "";
		single_mod_catname = "";
		single_mod_catid = "";
		m_mgr.Update();
	}
}

extern "C"
{
	void wxMessageBox_forZenglRun(char * str)
    {
        wxMessageBox(str,"打印信息");
    }
}

void MyFrame::OnClickTest(wxAuiToolBarEvent &event)
{
	/*ZENGL_EXPORT_VM_MAIN_ARGS vm_main_args = {global_userdef_compile_info_forZenglRunV2 , 
											  global_userdef_compile_error_forZenglRunV2,
											  global_userdef_run_info_forZenglRunV2,
											  global_userdef_run_print_forZenglRunV2,
											  global_userdef_run_error_forZenglRunV2,
											  global_module_init,
											  ZL_EXP_CP_AF_IN_DEBUG_MODE | 
											  ZL_EXP_CP_AF_OUTPUT_DEBUG_INFO};
	zenglApi_Load("test.zl",&vm_main_args); //测试zengl v1.2.0
	return;*/
	/*ZL_EXP_INT builtinID;
	ZL_EXP_VOID * VM;
	char * teststr;
	int testint;
	double testdouble;
	VM = zenglApi_Open();
	zenglApi_SetFlags(VM,(ZENGL_EXPORT_VM_MAIN_ARG_FLAGS)(ZL_EXP_CP_AF_IN_DEBUG_MODE | ZL_EXP_CP_AF_OUTPUT_DEBUG_INFO));
	zenglApi_SetHandle(VM,ZL_EXP_VFLAG_HANDLE_COMPILE_INFO,global_userdef_compile_info_forZenglRunV2);
	zenglApi_SetHandle(VM,ZL_EXP_VFLAG_HANDLE_RUN_INFO,global_userdef_run_info_forZenglRunV2);
	zenglApi_SetHandle(VM,ZL_EXP_VFLAG_HANDLE_RUN_PRINT,global_userdef_run_print_forZenglRunV2);
	builtinID = zenglApi_SetModInitHandle(VM,"builtin",global_builtin_module_init);
	zenglApi_SetModFunHandle(VM,builtinID,"bltTest",global_builtin_printf);
	if(zenglApi_Run(VM,"test.zl") == -1) //编译执行zengl脚本
	{
		wxMyLogEvent eventforlog( wxEVT_MY_LOG_EVENT,wxID_ANY,wxString::Format("\n错误：编译<test.zl>失败：%s\n",zenglApi_GetErrorString(VM)),
					MY_RICHTEXT_RED_STYLE);
		wxQueueEvent(glmainFrame->GetEventHandler(),eventforlog.Clone());
		zenglApi_Close(VM);
		return;
	}
	if((teststr = zenglApi_GetValueAsString(VM,"glmytest")) == ZL_EXP_NULL)
	{
		wxMyLogEvent eventforlog( wxEVT_MY_LOG_EVENT,wxID_ANY,wxString::Format("\n获取变量glmytest失败：%s\n",zenglApi_GetErrorString(VM)),
					MY_RICHTEXT_RED_STYLE);
		wxQueueEvent(glmainFrame->GetEventHandler(),eventforlog.Clone());
		zenglApi_Close(VM);
		return;
	}
	if(zenglApi_GetValueAsInt(VM,"i",&testint) == -1)
	{
		wxMyLogEvent eventforlog( wxEVT_MY_LOG_EVENT,wxID_ANY,wxString::Format("\n获取变量i失败：%s\n",zenglApi_GetErrorString(VM)),
					MY_RICHTEXT_RED_STYLE);
		wxQueueEvent(glmainFrame->GetEventHandler(),eventforlog.Clone());
		zenglApi_Close(VM);
		return;
	}
	if(zenglApi_GetValueAsDouble(VM,"floatnum",&testdouble) == -1)
	{
		wxMyLogEvent eventforlog( wxEVT_MY_LOG_EVENT,wxID_ANY,wxString::Format("\n获取变量floatnum失败：%s\n",zenglApi_GetErrorString(VM)),
					MY_RICHTEXT_RED_STYLE);
		wxQueueEvent(glmainFrame->GetEventHandler(),eventforlog.Clone());
		zenglApi_Close(VM);
		return;
	}
	wxMyLogEvent eventforlog( wxEVT_MY_LOG_EVENT,wxID_ANY,
				wxString::Format("the value of glmytest in test.zl is %s , i is %d , floatnum is %.16g\n",teststr,testint,testdouble),
				MY_RICHTEXT_GREEN_STYLE);
	wxQueueEvent(glmainFrame->GetEventHandler(),eventforlog.Clone());
	zenglApi_Reset(VM);
	builtinID = zenglApi_SetModInitHandle(VM,"builtin",global_builtin_module_init);
	//zenglApi_SetModFunHandle(VM,0,"printf",global_builtin_printf);
	if(zenglApi_Run(VM,"test2.zl") == -1) //编译执行zengl脚本
	{
		wxMyLogEvent eventforlog( wxEVT_MY_LOG_EVENT,wxID_ANY,wxString::Format("\n错误：编译<test2.zl>失败：%s\n",zenglApi_GetErrorString(VM)),
					MY_RICHTEXT_RED_STYLE);
		wxQueueEvent(glmainFrame->GetEventHandler(),eventforlog.Clone());
		zenglApi_Close(VM);
		return;
	}
	zenglApi_Reset(VM);
	zenglApi_Push(VM,ZL_EXP_FAT_INT,0,1415,0);
	zenglApi_Push(VM,ZL_EXP_FAT_STR,"test second arg",0,0);
	if(zenglApi_Call(VM,"test.zl","init","clsTest") == -1) //编译执行zengl脚本函数
	{
		wxMyLogEvent eventforlog( wxEVT_MY_LOG_EVENT,wxID_ANY,wxString::Format("\n错误：编译执行<test fun call>失败：%s\n",zenglApi_GetErrorString(VM)),
					MY_RICHTEXT_RED_STYLE);
		wxQueueEvent(glmainFrame->GetEventHandler(),eventforlog.Clone());
		zenglApi_Close(VM);
		return;
	}
	zenglApi_Close(VM);
	return;*/


	wxPoint mainWinLoc=this->GetScreenPosition();
	mainWinLoc.x += 180;
	mainWinLoc.y = 200;
	//wxSize mainWinSize=this->GetSize();
	glIsDialogInShow = true;
	MyTestDialog mytestdlg(this,"测试正则表达式 - （仅供程序员调试用！）",mainWinLoc,wxSize(400,200));
	mytestdlg.Create();
	mytestdlg.ShowModal();
	glIsDialogInShow = false;
	/*char *zengl_test[] = {"zenglrun","test.zlc"};
	//for(;;)
	global_my_weburl = "www.zenglong2.qq";
	single_mod_modid = "21";
	single_mod_catid = "4";
	single_mod_catname = "矿业新闻";
	zenglrun_main(2,zengl_test);*/
}

void MyFrame::OnClickResults(wxAuiToolBarEvent &event)
{
	if(glSiteDBPath == "")
	{
		wxMessageBox("您还没有从网站获取分类信息！","警告");
		return;
	}
	else if(wxFileExists(glSiteDBPath)==false)
	{
		wxMessageBox("本地数据库["+glSiteDBPath+"]文件不存在！","警告");
		return;
	}
	else if(single_mod_cat_table == "")
	{
		wxMessageBox("您还没开始采集数据，只有开始采集时，才可以处理采集的本地数据！","警告");
		return;
	}
	glIsDialogInShow = true;
	MyResultsDialog myresultsdlg(this,"采集结果数据处理",wxDefaultPosition,wxSize(400,350));
	myresultsdlg.Create();
	myresultsdlg.Center();
	myresultsdlg.ShowModal();
	glIsDialogInShow = false;
}

void MyFrame::OnClickShowLog(wxAuiToolBarEvent &event)
{
	wxExecute("notepad.exe mylogs.txt");
}

void MyFrame::OnClickWebOffice(wxAuiToolBarEvent &event)
{
	//wxLaunchDefaultBrowser("http://" + gl_initManage_Url);
	//wxLaunchDefaultBrowser("http://www.zengl.com");
	wxExecute("notepad.exe 初始化脚本.zl");
}

void MyFrame::OnResetInitScript(wxAuiToolBarEvent &event)
{
	ZL_EXP_VOID * VM = zenglApi_Open();
	zenglApi_SetFlags(VM,(ZENGL_EXPORT_VM_MAIN_ARG_FLAGS)(ZL_EXP_CP_AF_IN_DEBUG_MODE));
	zenglApi_SetModFunHandle(VM,0,"bltSetInitManageUrl",global_bltSetInitManageUrl);
	zenglApi_SetModFunHandle(VM,0,"bltSetModulePath",global_bltSetModulePath);
	if(zenglApi_Run(VM,"初始化脚本.zl") == -1)
		wxMessageBox(wxString::Format("'初始化脚本.zl'执行失败：%s",zenglApi_GetErrorString(VM)),"警告");
	else
		wxMessageBox("初始化脚本加载完毕","打印信息");
	zenglApi_Close(VM);
}

void MyFrame::OnMinimize(wxIconizeEvent &event)
{
	//Show(false);
}

MyThread::MyThread(MyFrame *frame)
		:wxThread()
{
	m_frame = frame;
}

void *MyThread::Entry()
{
	wxCommandEvent eventForProgress( wxEVT_MY_PROGRESS , ID_MY_WINDOW);
	eventForProgress.SetEventObject(m_frame->m_statusbar);
	int totaltask = 9,curtask = -1,totalprocess = 100;
	m_frame->m_statusbar->m_total = totalprocess; 
	m_frame->m_statusbar->m_pos = ((float)(++curtask)/(float)totaltask) * totalprocess; //0 task
	//m_frame->m_statusbar->GetEventHandler()->ProcessEvent( eventForProgress );
	wxQueueEvent(m_frame->m_statusbar->GetEventHandler(),eventForProgress.Clone());

	wxString SiteUrlName = m_frame->textUrlName->GetValue().mb_str(wxCSConv(wxT("gb2312")));
	SiteUrlName.Replace("http://","");
	if(SiteUrlName.Last()=='/')
		SiteUrlName.RemoveLast();
	m_frame->textUrlName->SetValue(SiteUrlName);
	wxSQLite3Database *db = wxGetApp().db;
	wxString EscapeSiteName = SiteUrlName;
	EscapeSiteName.Replace("/","_");
	glEscapeSiteName = EscapeSiteName;
	wxString sitedbpath = wxT("db/") + glEscapeSiteName + ".db";
	if(db->IsOpen())
		db->Close();
	if(wxFileExists(sitedbpath))
	{
		wxMessageDialog msgdlg(m_frame,"该网站的分类信息已经存在，如果想同步服务器上的数据请选择同步服务器数据\n\n如果想使用之前本地保存的数据则选择使用本地数据库\n\n注意如果分类数很多(上万个时)，则请稍等片刻。。。",
					"分类信息提示框", wxYES_NO | wxCANCEL);
		//msgdlg.SetOKCancelLabels("同步服务器数据","使用本地数据库");
		msgdlg.SetYesNoCancelLabels("使用本地数据库", "同步服务器数据" , "取消操作");
		int dlgret = msgdlg.ShowModal();
		if(dlgret==wxID_YES)
		{
			glmainFrame->m_textForTimeOut->SetValue("60");
			wxCommandEvent eventUpdateCate(wxEVT_MY_UPDATECATE , ID_MY_WINDOW);
			eventUpdateCate.SetEventObject(m_frame);
			m_frame->GetEventHandler()->ProcessEvent( eventUpdateCate );
			//wxQueueEvent(m_frame->GetEventHandler(),eventUpdateCate.Clone());
			return NULL;
		}
		else if(dlgret == wxID_CANCEL)
		{
			glmainFrame->m_textForTimeOut->SetValue("60");
			return NULL;
		}

		wxRemoveFile(sitedbpath);
	}

	m_frame->m_statusbar->m_pos = ((float)(++curtask)/(float)totaltask) * totalprocess; //1 task
	wxQueueEvent(m_frame->m_statusbar->GetEventHandler(),eventForProgress.Clone());

	struct curl_httppost *post=NULL;
	struct curl_httppost *last=NULL;
	long timeout;
	glmainFrame->m_textForTimeOut->GetValue().ToLong(&timeout);
	wxString strUrlName = SiteUrlName + "/mydetectTitle.php";
	curl=curl_easy_init();
	curl_easy_setopt(curl, CURLOPT_URL, strUrlName.c_str().AsChar()); 
	curl_easy_setopt(curl, CURLOPT_USERAGENT ,"Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.2; .NET CLR 1.1.4322)");
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION ,1);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, (int)timeout);//设置超时时间
	curl_easy_setopt(curl, CURLOPT_HEADER, 0);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, mywxcurl_string_write);
	//char * testdata = (char *)SiteUrlName.data();
	//curl_formadd(&post,&last,CURLFORM_COPYNAME,"SiteUrlName",CURLFORM_COPYCONTENTS, SiteUrlName.data().AsChar(), CURLFORM_END);
	//curl_formadd(&post,&last,CURLFORM_COPYNAME,"secret_loginForCaiji",CURLFORM_COPYCONTENTS, "disnglelngsin1988yxj", CURLFORM_END);
	curl_formadd(&post,&last,CURLFORM_COPYNAME,"_zlmy_action",CURLFORM_COPYCONTENTS, "getCatsXml", CURLFORM_END);
	curl_easy_setopt(curl, CURLOPT_HTTPPOST, post);

	if((fp=fopen("test.xml","w"))==NULL)
    {
        curl_easy_cleanup(curl);
		return NULL;
    }
	curl_easy_setopt(curl, CURLOPT_WRITEDATA , fp);
    curl_easy_perform(curl);

	m_frame->m_statusbar->m_pos = ((float)(++curtask)/(float)totaltask) * totalprocess; //2 task
	wxQueueEvent(m_frame->m_statusbar->GetEventHandler(),eventForProgress.Clone());

	if(char_myglStr==NULL)
	{
		charlength = 0;
		wxMyLogEvent event( wxEVT_MY_LOG_EVENT,wxID_ANY," null char content get!(原因可能是网址错了，或没上传采集接口，或者您的分类数很多，例如有几万个分类，请将工具栏的超时时间设为1000，如果1000不够就再设多点，再重试，谢谢！)\n",MY_RICHTEXT_RED_STYLE);
		wxQueueEvent(m_frame->GetEventHandler(),event.Clone());
		fclose(fp);
		return NULL;
	}
	charlength = strlen(char_myglStr);
	//fwrite( char_myglStr, 1 , char_myglTotalnum ,fp);
	myglStr = "";
	if((myglStr = wxString(char_myglStr)) == "")
		myglStr = wxString::FromUTF8(char_myglStr);
	
	if(myglStr == "" || myglStr=="null")
	{
		wxMyLogEvent event( wxEVT_MY_LOG_EVENT,wxID_ANY," 获取分类失败或者是没有分类信息！请检查采集接口是否部署好了\n",MY_RICHTEXT_RED_STYLE);
		wxQueueEvent(m_frame->GetEventHandler(),event.Clone());
		fclose(fp);
		free(char_myglStr);
		char_myglStr = NULL;
		return NULL;
	}
	myglStr.Replace("&","");
	wxScopedCharBuffer myglStrForUTF8 = myglStr.ToUTF8();
	fwrite(myglStrForUTF8.data(), 1 , myglStrForUTF8.length(),fp);
	fclose(fp);
	free(char_myglStr);
	char_myglStr = NULL;
	char_myglTotalnum = 0;
    curl_easy_cleanup(curl);
	wxMyLogEvent eventforlog( wxEVT_MY_LOG_EVENT,wxID_ANY);

	m_frame->m_statusbar->m_pos = ((float)(++curtask)/(float)totaltask) * totalprocess; //3 task
	wxQueueEvent(m_frame->m_statusbar->GetEventHandler(),eventForProgress.Clone());

	myglStr = "";
	if(!db->IsOpen())
	{
		db->Open(sitedbpath);
		myglStr += SiteUrlName + "的数据库已经打开！\n";
	}
	//if(!db->TableExists(wxT("website")))
	//{
	db->ExecuteUpdate(wxT("DROP TABLE IF EXISTS website"));
	db->ExecuteUpdate(wxT("CREATE TABLE website (id INTEGER PRIMARY KEY ASC,url,chname,type)"));
	//myglStr += wxT("创建website数据表！\n");
	eventforlog.SetEventMsg("创建website数据表！\n",MY_RICHTEXT_GREEN_STYLE);
	wxQueueEvent(m_frame->GetEventHandler(),eventforlog.Clone());
	//}
	//if(!db->TableExists(wxT("category")))
	//{
	db->ExecuteUpdate(wxT("DROP TABLE IF EXISTS category"));
	db->ExecuteUpdate(wxT("CREATE TABLE category (id INTEGER PRIMARY KEY ASC,catid,modid,catname)"));
	//myglStr += wxT("创建category数据表！\n");
	eventforlog.SetEventMsg("创建category数据表！\n",MY_RICHTEXT_GREEN_STYLE);
	wxQueueEvent(m_frame->GetEventHandler(),eventforlog.Clone());
	//}
	//if(!db->TableExists(wxT("keywords")))
	//{
	db->ExecuteUpdate(wxT("DROP TABLE IF EXISTS keywords"));
	db->ExecuteUpdate(wxT("CREATE TABLE keywords (id INTEGER PRIMARY KEY ASC,name,catid,must,needSuffix)"));
	//myglStr += wxT("创建keywords数据表！\n");
	eventforlog.SetEventMsg("创建keywords数据表！\n准备加载test.xml文件，如果分类多则请稍等。。",MY_RICHTEXT_GREEN_STYLE);
	wxQueueEvent(m_frame->GetEventHandler(),eventforlog.Clone());
	//}

	m_frame->m_statusbar->m_pos = ((float)(++curtask)/(float)totaltask) * totalprocess; //4 task
	wxQueueEvent(m_frame->m_statusbar->GetEventHandler(),eventForProgress.Clone());

	wxXmlDocument doc;
	if (!doc.Load("test.xml"))
	{
		wxMyLogEvent event( wxEVT_MY_LOG_EVENT,wxID_ANY,"加载test.xml文件失败\n",MY_RICHTEXT_RED_STYLE);
		wxQueueEvent(m_frame->GetEventHandler(),event.Clone());
		return NULL;
	}
	
	wxXmlNode *root = doc.GetRoot();
	int InitCount = 0;
	wxString sqlstr = "";
	wxSQLite3ResultSet set = db->ExecuteQuery(wxT("SELECT * FROM website where chname = '") + root->GetAttribute("name") + _("'"));
	if(!set.NextRow())
	{
		sqlstr.Printf("INSERT INTO website (url,chname,type) values('%s','%s','')",
									root->GetAttribute("url"),TrimBoth(root->GetAttribute("name")) );
		db->ExecuteUpdate( sqlstr );
		eventforlog.SetEventMsg("初始化website表\n",MY_RICHTEXT_GREEN_STYLE);
		wxQueueEvent(m_frame->GetEventHandler(),eventforlog.Clone());
	}
	eventforlog.SetEventMsg("准备从"+root->GetAttribute("url")+"网站获取模块数据！\n",MY_RICHTEXT_GREEN_STYLE);
	wxQueueEvent(m_frame->GetEventHandler(),eventforlog.Clone());
	wxXmlDocument doc2;
	global_my_weburl = root->GetAttribute("url"); //获取网站的全局URL
	strUrlName = root->GetAttribute("url")+"mydetectTitle.php?_zlmy_get_action=getModules";
	curl=curl_easy_init();
	global_GetUrlContent(strUrlName);
	curl_easy_cleanup(curl);
	curl = NULL;
	if(myglStr == "")
	{
		eventforlog.SetEventMsg("获取"+root->GetAttribute("url")+"网站的模块数据失败！\n",MY_RICHTEXT_RED_STYLE);
		wxQueueEvent(m_frame->GetEventHandler(),eventforlog.Clone());
		free(char_myglStr);
		char_myglStr = NULL;
		return NULL;
	}
	if((fp=fopen("db/"+ TrimBoth(root->GetAttribute("name"))+".xml","w+"))==NULL)
	{
		eventforlog.SetEventMsg("打开文件db/"+root->GetAttribute("name")+".xml失败！\n",MY_RICHTEXT_RED_STYLE);
		wxQueueEvent(m_frame->GetEventHandler(),eventforlog.Clone());
		fclose(fp);
		free(char_myglStr);
		char_myglStr = NULL;
		return NULL;
	}
	myglStrForUTF8 = myglStr.ToUTF8();
	fwrite(myglStrForUTF8.data(), 1 , myglStrForUTF8.length(),fp);
	fclose(fp);
	eventforlog.SetEventMsg("获取到数据，准备初始化"+root->GetAttribute("url")+"网站的模块数据...",MY_RICHTEXT_GREEN_STYLE);
	wxQueueEvent(m_frame->GetEventHandler(),eventforlog.Clone());
	if (!doc2.Load("db/"+TrimBoth(root->GetAttribute("name"))+".xml"))
	{
		eventforlog.SetEventMsg("加载db/"+root->GetAttribute("name")+".xml文件失败\n",MY_RICHTEXT_RED_STYLE);
		wxQueueEvent(m_frame->GetEventHandler(),eventforlog.Clone());
		return NULL;
	}
	wxXmlNode * root2 = doc2.GetRoot();
	wxXmlNode *child2 = root2->GetChildren();
	if(global_MyModules.size() > 0)
	{
		global_MyModules.clear();
		glMyModuleNames.clear();
	}
	while(child2)
	{
		global_MyModules[child2->GetAttribute("id")] = child2->GetAttribute("module");
		glMyModuleNames[child2->GetAttribute("id")] = child2->GetAttribute("name");
		child2 = child2->GetNext();
	}
	eventforlog.SetEventMsg("初始化完毕！\n",MY_RICHTEXT_GREEN_STYLE);
	wxQueueEvent(m_frame->GetEventHandler(),eventforlog.Clone());

	m_frame->m_statusbar->m_pos = ((float)(++curtask)/(float)totaltask) * totalprocess; //5 task
	wxQueueEvent(m_frame->m_statusbar->GetEventHandler(),eventForProgress.Clone());

	db->ExecuteUpdate(_("DELETE from category"));
	wxXmlNode *child = root->GetChildren();
	db->ExecuteUpdate(_("DELETE from keywords"));
	wxXmlNode *keychild;

	m_frame->m_statusbar->m_pos = ((float)(++curtask)/(float)totaltask) * totalprocess; //6 task
	wxQueueEvent(m_frame->m_statusbar->GetEventHandler(),eventForProgress.Clone());
	
	int pretotalprocess = m_frame->m_statusbar->m_pos;
	totalprocess -= pretotalprocess;
	curtask = 0;
	totaltask = 0;
	wxXmlNode * tmpnode = child;
	while(child)
	{
		child = child->GetNext();
		totaltask++;
	}
	totaltask += 3;
	child = tmpnode;
	eventforlog.SetEventMsg("准备插入分类和关键词信息...请稍等...\n",MY_RICHTEXT_GREEN_STYLE);
	wxQueueEvent(m_frame->GetEventHandler(),eventforlog.Clone());
	db->Begin();
	while(child)
	{
		sqlstr.Printf("INSERT INTO category (catid,modid,catname) values('%s','%s','%s')",child->GetAttribute("catid"),child->GetAttribute("modid"),
						child->GetAttribute("name") );
		db->ExecuteUpdate(sqlstr);
		keychild = child->GetChildren();
		while(keychild)
		{
			sqlstr.Printf("INSERT INTO keywords (name,catid,must,needSuffix) values('%s','%s','%s','%s')",keychild->GetAttribute("name"),child->GetAttribute("catid"),keychild->GetAttribute("must"),
						keychild->GetAttribute("needSuffix") );
			db->ExecuteUpdate(sqlstr);
			keychild = keychild->GetNext();
		}
		//eventforlog.SetEventMsg("插入分类<"+child->GetAttribute("name")+">...",MY_RICHTEXT_GREEN_STYLE);
		//wxQueueEvent(m_frame->GetEventHandler(),eventforlog.Clone());
		child = child->GetNext();
		m_frame->m_statusbar->m_pos = pretotalprocess + ((float)(++curtask)/(float)totaltask) * totalprocess; //n task
		wxQueueEvent(m_frame->m_statusbar->GetEventHandler(),eventForProgress.Clone());
	}
	db->Commit();
	eventforlog.SetEventMsg("分类和关键词信息插入完毕...\n",MY_RICHTEXT_GREEN_STYLE);
	wxQueueEvent(m_frame->GetEventHandler(),eventforlog.Clone());
	//myglStr += "初始化category表和keywords表\n";

	m_frame->m_statusbar->m_pos = pretotalprocess + ((float)(++curtask)/(float)totaltask) * totalprocess; //7 task
	wxQueueEvent(m_frame->m_statusbar->GetEventHandler(),eventForProgress.Clone());
	
	myglStyle = MY_RICHTEXT_GREEN_STYLE;
	eventforlog.SetEventMsg("\n初始化category表和keywords表\n准备更新左侧的分类列表，如果分类数很多，则请稍等...\n",myglStyle);
	wxQueueEvent(m_frame->GetEventHandler(),eventforlog.Clone());

	m_frame->m_statusbar->m_pos = pretotalprocess + ((float)(++curtask)/(float)totaltask) * totalprocess; //8 task
	wxQueueEvent(m_frame->m_statusbar->GetEventHandler(),eventForProgress.Clone());

	wxCommandEvent eventUpdateCate(wxEVT_MY_UPDATECATE , ID_MY_WINDOW);
	eventUpdateCate.SetEventObject(m_frame);
	m_frame->GetEventHandler()->ProcessEvent( eventUpdateCate );
	//wxQueueEvent(m_frame->GetEventHandler(),eventUpdateCate.Clone());

	//m_frame->m_statusbar->m_pos = pretotalprocess + ((float)(++curtask)/(float)totaltask) * totalprocess; //9 task
	//wxQueueEvent(m_frame->m_statusbar->GetEventHandler(),eventForProgress.Clone());

	glmainFrame->m_textForTimeOut->SetValue("60");

	return NULL;
}

void MyThread::OnExit()
{
	//wxCriticalSectionLocker enter(wxGetApp().m_critsect);
	m_frame->mythread = NULL;
}

MyThread2::MyThread2(MyFrame * frame)
{
	m_frame = frame;
}

void InitHashMap();

void *MyThread2::Entry()
{
	MyTreeCtrl * treeCtrl = m_frame->treeCtrl;
	wxMenuBar * menubar = m_frame->GetMenuBar();
	wxTreeItemId rootid = treeCtrl->GetRootItem();
	MyTreeItemData * itemdata = (MyTreeItemData *)treeCtrl->GetItemData(rootid);
	wxString webname = treeCtrl->GetItemText(rootid);
	wxString weburl = itemdata->m_url;
	global_my_weburl = weburl;
	myglStyle = MY_RICHTEXT_GREEN_STYLE;
	wxString SiteDBPath = "db/" + glEscapeSiteName + ".db";
	if(!wxFileExists(SiteDBPath))
	{
		myglStyle = MY_RICHTEXT_RED_STYLE;
		myglStr = "[" +webname + "]网站对应的本地数据库文件不存在！\n";
		wxMyLogEvent event( wxEVT_MY_LOG_EVENT,wxID_ANY,myglStr,myglStyle);
		wxQueueEvent(m_frame->m_statusbar->GetEventHandler(),event.Clone());
		return NULL;
	}
	myglStr = "网站名称：" + webname + " url is " + itemdata->m_url + "\n";
	wxMyLogEvent eventForLog( wxEVT_MY_LOG_EVENT,wxID_ANY,myglStr,myglStyle);
	wxCommandEvent eventForClearLogs( wxEVT_MY_CLEARLOGS , ID_MY_WINDOW);
	//eventForLog
	wxQueueEvent(glmainFrame->GetEventHandler(),eventForLog.Clone());
	wxTreeItemIdValue cookie,cookie2;
	wxTreeItemId cateitemid = treeCtrl->GetFirstChild(rootid,cookie), cateitemChild;
	if((bool)m_frame->m_textpostnum->GetValue().IsNumber() != true)
	{
		wxMyLogEvent event( wxEVT_MY_LOG_EVENT,wxID_ANY,"启动失败！原因：每个分类的采集数量必须是整数！",MY_RICHTEXT_RED_STYLE);
		wxQueueEvent(m_frame->m_statusbar->GetEventHandler(),event.Clone());
		return NULL;
	}
	int totalcount=0,postnum=0;
	do
	{
		if(!cateitemid.IsOk())
			break;
		cateitemChild = treeCtrl->GetFirstChild(cateitemid,cookie2);
		do
		{
			if(!cateitemChild.IsOk())
				break;
			itemdata = (MyTreeItemData *)treeCtrl->GetItemData(cateitemChild);
			if(treeCtrl->GetItemState(cateitemChild) == 1 && itemdata->m_modID != "4")
			{
				totalcount++;
			}
		}while(cateitemChild = treeCtrl->GetNextChild(cateitemChild,cookie2));
	}while(cateitemid = treeCtrl->GetNextChild(cateitemid,cookie));
	if(totalcount <= 0)
	{
		wxMyLogEvent event( wxEVT_MY_LOG_EVENT,wxID_ANY,"请在左侧列表中至少选择一个要采集的分类(注意：公司是和供应商城等模块一起采集的，所以如果你只勾选了公司也会提示此错误)！\n",MY_RICHTEXT_RED_STYLE);
		wxQueueEvent(m_frame->m_statusbar->GetEventHandler(),event.Clone());
		return NULL;
	}
	wxMyProgressEvent eventForProgress(wxEVT_MY_PROGRESS_EVENT,ID_MY_WINDOW);
	glmainFrame->m_textpostnum->GetValue().ToLong((long *)&postnum);
	global_mod_processNum = totalcount * postnum;
	global_mod_processPos = 0;
	global_mod_totalpostnum = 0;
	global_mod_totalcaijinum = 0;
	single_mod_processNum = postnum;
	single_mod_processPos = 0;
	single_mod_curpostnum = 0;
	single_mod_caijinum = 0;
	eventForProgress.SetEventNum(single_mod_processPos,single_mod_processNum,global_mod_processPos,global_mod_processNum);
	wxQueueEvent(glmainFrame->GetEventHandler(),eventForProgress.Clone());
	cateitemid = treeCtrl->GetFirstChild(rootid,cookie);
	int i = 0,itemstate,iRemain=0;
	InitHashMap();
	wxString ModFilePath;
	if(CUR_CAIJI_VM != ZL_EXP_NULL)
	{
		zenglApi_Close(CUR_CAIJI_VM);
		CUR_CAIJI_VM =ZL_EXP_NULL;
	}
	CUR_CAIJI_VM = zenglApi_Open();
	zenglApi_SetFlags(CUR_CAIJI_VM,(ZENGL_EXPORT_VM_MAIN_ARG_FLAGS)(ZL_EXP_CP_AF_IN_DEBUG_MODE | ZL_EXP_CP_AF_OUTPUT_DEBUG_INFO));
	//zenglApi_SetFlags(CUR_CAIJI_VM,(ZENGL_EXPORT_VM_MAIN_ARG_FLAGS)(ZL_EXP_CP_AF_IN_DEBUG_MODE));
	zenglApi_SetHandle(CUR_CAIJI_VM,ZL_EXP_VFLAG_HANDLE_COMPILE_INFO,global_userdef_compile_info_forZenglRunV2);
	zenglApi_SetHandle(CUR_CAIJI_VM,ZL_EXP_VFLAG_HANDLE_RUN_INFO,global_userdef_run_info_forZenglRunV2);
	zenglApi_SetHandle(CUR_CAIJI_VM,ZL_EXP_VFLAG_HANDLE_RUN_PRINT,global_userdef_run_print_forZenglRunV2);
	do
	{
		if(!cateitemid.IsOk())
			break;
		cateitemChild = treeCtrl->GetFirstChild(cateitemid,cookie2);
		do
		{
			if(!cateitemChild.IsOk())
				break;

			if(TestDestroy())
			{
				if(CUR_CAIJI_VM != ZL_EXP_NULL)
				{
					zenglApi_Close(CUR_CAIJI_VM);
					CUR_CAIJI_VM =ZL_EXP_NULL;
				}
				return NULL;
			}
			itemstate = treeCtrl->GetItemState(cateitemChild);
			itemdata = (MyTreeItemData *)treeCtrl->GetItemData(cateitemChild);
			if(itemstate == 1)
			{
				single_mod_processNum = postnum;
				single_mod_processPos = 0;
				single_mod_curpostnum = 0;
				single_mod_caijinum = 0;
				single_mod_modid = itemdata->m_modID;
				if(global_MyModules[itemdata->m_modID] == "article")
				{
					single_mod_cat_table = "article";
					single_mod_catname = treeCtrl->GetItemText(cateitemChild);
					single_mod_catid = itemdata->m_cateID;
					//ModFilePath =  gl_zenglrun_article_modname + "c";
					ModFilePath =  gl_zenglrun_article_modname;
					zenglApi_SetModInitHandle(CUR_CAIJI_VM,"builtin",global_builtin_module_init);
					if(ModFilePath.Find(".zlencrypt") != wxNOT_FOUND) //以.zlencrypt为后缀的，默认为加密脚本
					{
						zenglApi_SetSourceRC4Key(CUR_CAIJI_VM,(char *)gl_encrypt_str.c_str().AsChar(),gl_encrypt_str.Length()); //加密脚本
					}
					else if(menubar->IsChecked(ID_MENU_EXTRA_DEBUG))
					{
						zenglApi_DebugSetBreakHandle(CUR_CAIJI_VM,global_debug_break,global_debug_conditionError,ZL_EXP_TRUE,ZL_EXP_FALSE);
					}
					if(zenglApi_Run(CUR_CAIJI_VM,(char *)ModFilePath.c_str().AsChar()) == -1) //编译执行zengl脚本
					{
						wxMyLogEvent eventforlog( wxEVT_MY_LOG_EVENT,wxID_ANY,wxString::Format("\n编译运行"+ModFilePath+"时发生异常：%s\n",zenglApi_GetErrorString(CUR_CAIJI_VM)),
									MY_RICHTEXT_RED_STYLE);
						wxQueueEvent(glmainFrame->GetEventHandler(),eventforlog.Clone());
						zenglApi_Close(CUR_CAIJI_VM);
						CUR_CAIJI_VM = ZL_EXP_NULL;
						return NULL;
					}
					zenglApi_Reset(CUR_CAIJI_VM);
					wxQueueEvent(glmainFrame->GetEventHandler(),eventForClearLogs.Clone());
				}
				else if(global_MyModules[itemdata->m_modID] == "quote")
				{
					single_mod_cat_table = "quote";
					single_mod_catname = treeCtrl->GetItemText(cateitemChild);
					single_mod_catid = itemdata->m_cateID;
					ModFilePath =  gl_zenglrun_quote_modname;
					zenglApi_SetModInitHandle(CUR_CAIJI_VM,"builtin",global_builtin_module_init);
					if(ModFilePath.Find(".zlencrypt") != wxNOT_FOUND) //以.zlencrypt为后缀的，默认为加密脚本
					{
						zenglApi_SetSourceRC4Key(CUR_CAIJI_VM,(char *)gl_encrypt_str.c_str().AsChar(),gl_encrypt_str.Length()); //加密脚本
					}
					else if(menubar->IsChecked(ID_MENU_EXTRA_DEBUG))
					{
						zenglApi_DebugSetBreakHandle(CUR_CAIJI_VM,global_debug_break,global_debug_conditionError,ZL_EXP_TRUE,ZL_EXP_FALSE);
					}
					if(zenglApi_Run(CUR_CAIJI_VM,(char *)ModFilePath.c_str().AsChar()) == -1) //编译执行zengl脚本
					{
						wxMyLogEvent eventforlog( wxEVT_MY_LOG_EVENT,wxID_ANY,wxString::Format("\n编译运行"+ModFilePath+"时发生异常：%s\n",zenglApi_GetErrorString(CUR_CAIJI_VM)),
									MY_RICHTEXT_RED_STYLE);
						wxQueueEvent(glmainFrame->GetEventHandler(),eventforlog.Clone());
						zenglApi_Close(CUR_CAIJI_VM);
						CUR_CAIJI_VM = ZL_EXP_NULL;
						return NULL;
					}
					zenglApi_Reset(CUR_CAIJI_VM);
					wxQueueEvent(glmainFrame->GetEventHandler(),eventForClearLogs.Clone());
				}
				else if(global_MyModules[itemdata->m_modID] == "special")
				{
					single_mod_cat_table = "special";
					single_mod_catname = treeCtrl->GetItemText(cateitemChild);
					single_mod_catid = itemdata->m_cateID;
					ModFilePath = gl_zenglrun_special_modname;
					zenglApi_SetModInitHandle(CUR_CAIJI_VM,"builtin",global_builtin_module_init);
					if(ModFilePath.Find(".zlencrypt") != wxNOT_FOUND) //以.zlencrypt为后缀的，默认为加密脚本
					{
						zenglApi_SetSourceRC4Key(CUR_CAIJI_VM,(char *)gl_encrypt_str.c_str().AsChar(),gl_encrypt_str.Length()); //加密脚本
					}
					else if(menubar->IsChecked(ID_MENU_EXTRA_DEBUG))
					{
						zenglApi_DebugSetBreakHandle(CUR_CAIJI_VM,global_debug_break,global_debug_conditionError,ZL_EXP_TRUE,ZL_EXP_FALSE);
					}
					if(zenglApi_Run(CUR_CAIJI_VM,(char *)ModFilePath.c_str().AsChar()) == -1) //编译执行zengl脚本
					{
						wxMyLogEvent eventforlog( wxEVT_MY_LOG_EVENT,wxID_ANY,wxString::Format("\n编译运行"+ModFilePath+"时发生异常：%s\n",zenglApi_GetErrorString(CUR_CAIJI_VM)),
									MY_RICHTEXT_RED_STYLE);
						wxQueueEvent(glmainFrame->GetEventHandler(),eventforlog.Clone());
						zenglApi_Close(CUR_CAIJI_VM);
						CUR_CAIJI_VM = ZL_EXP_NULL;
						return NULL;
					}
					zenglApi_Reset(CUR_CAIJI_VM);
					wxQueueEvent(glmainFrame->GetEventHandler(),eventForClearLogs.Clone());
				}
				else if(global_MyModules[itemdata->m_modID] == "info")
				{
					single_mod_cat_table = "info";
					single_mod_catname = treeCtrl->GetItemText(cateitemChild);
					single_mod_catid = itemdata->m_cateID;
					ModFilePath = gl_zenglrun_info_modname;
					zenglApi_SetModInitHandle(CUR_CAIJI_VM,"builtin",global_builtin_module_init);
					if(ModFilePath.Find(".zlencrypt") != wxNOT_FOUND) //以.zlencrypt为后缀的，默认为加密脚本
					{
						zenglApi_SetSourceRC4Key(CUR_CAIJI_VM,(char *)gl_encrypt_str.c_str().AsChar(),gl_encrypt_str.Length()); //加密脚本
					}
					else if(menubar->IsChecked(ID_MENU_EXTRA_DEBUG))
					{
						zenglApi_DebugSetBreakHandle(CUR_CAIJI_VM,global_debug_break,global_debug_conditionError,ZL_EXP_TRUE,ZL_EXP_FALSE);
					}
					if(zenglApi_Run(CUR_CAIJI_VM,(char *)ModFilePath.c_str().AsChar()) == -1) //编译执行zengl脚本
					{
						wxMyLogEvent eventforlog( wxEVT_MY_LOG_EVENT,wxID_ANY,wxString::Format("\n编译运行"+ModFilePath+"时发生异常：%s\n",zenglApi_GetErrorString(CUR_CAIJI_VM)),
									MY_RICHTEXT_RED_STYLE);
						wxQueueEvent(glmainFrame->GetEventHandler(),eventforlog.Clone());
						zenglApi_Close(CUR_CAIJI_VM);
						CUR_CAIJI_VM = ZL_EXP_NULL;
						return NULL;
					}
					zenglApi_Reset(CUR_CAIJI_VM);
					wxQueueEvent(glmainFrame->GetEventHandler(),eventForClearLogs.Clone());
				}
				else if(global_MyModules[itemdata->m_modID] == "brand")
				{
					single_mod_cat_table = "brand";
					single_mod_catname = treeCtrl->GetItemText(cateitemChild);
					single_mod_catid = itemdata->m_cateID;
					ModFilePath = gl_zenglrun_brand_modname;
					zenglApi_SetModInitHandle(CUR_CAIJI_VM,"builtin",global_builtin_module_init);
					if(ModFilePath.Find(".zlencrypt") != wxNOT_FOUND) //以.zlencrypt为后缀的，默认为加密脚本
					{
						zenglApi_SetSourceRC4Key(CUR_CAIJI_VM,(char *)gl_encrypt_str.c_str().AsChar(),gl_encrypt_str.Length()); //加密脚本
					}
					else if(menubar->IsChecked(ID_MENU_EXTRA_DEBUG))
					{
						zenglApi_DebugSetBreakHandle(CUR_CAIJI_VM,global_debug_break,global_debug_conditionError,ZL_EXP_TRUE,ZL_EXP_FALSE);
					}
					if(zenglApi_Run(CUR_CAIJI_VM,(char *)ModFilePath.c_str().AsChar()) == -1) //编译执行zengl脚本
					{
						wxMyLogEvent eventforlog( wxEVT_MY_LOG_EVENT,wxID_ANY,wxString::Format("\n编译运行"+ModFilePath+"时发生异常：%s\n",zenglApi_GetErrorString(CUR_CAIJI_VM)),
									MY_RICHTEXT_RED_STYLE);
						wxQueueEvent(glmainFrame->GetEventHandler(),eventforlog.Clone());
						zenglApi_Close(CUR_CAIJI_VM);
						CUR_CAIJI_VM = ZL_EXP_NULL;
						return NULL;
					}
					zenglApi_Reset(CUR_CAIJI_VM);
					wxQueueEvent(glmainFrame->GetEventHandler(),eventForClearLogs.Clone());
				}
				else if(global_MyModules[itemdata->m_modID] == "mall")
				{
					single_mod_cat_table = "mall";
					single_mod_catname = treeCtrl->GetItemText(cateitemChild);
					single_mod_catid = itemdata->m_cateID;
					ModFilePath = gl_zenglrun_mall_modname;
					zenglApi_SetModInitHandle(CUR_CAIJI_VM,"builtin",global_builtin_module_init);
					if(ModFilePath.Find(".zlencrypt") != wxNOT_FOUND) //以.zlencrypt为后缀的，默认为加密脚本
					{
						zenglApi_SetSourceRC4Key(CUR_CAIJI_VM,(char *)gl_encrypt_str.c_str().AsChar(),gl_encrypt_str.Length()); //加密脚本
					}
					else if(menubar->IsChecked(ID_MENU_EXTRA_DEBUG))
					{
						zenglApi_DebugSetBreakHandle(CUR_CAIJI_VM,global_debug_break,global_debug_conditionError,ZL_EXP_TRUE,ZL_EXP_FALSE);
					}
					if(zenglApi_Run(CUR_CAIJI_VM,(char *)ModFilePath.c_str().AsChar()) == -1) //编译执行zengl脚本
					{
						wxMyLogEvent eventforlog( wxEVT_MY_LOG_EVENT,wxID_ANY,wxString::Format("\n编译运行"+ModFilePath+"时发生异常：%s\n",zenglApi_GetErrorString(CUR_CAIJI_VM)),
									MY_RICHTEXT_RED_STYLE);
						wxQueueEvent(glmainFrame->GetEventHandler(),eventforlog.Clone());
						zenglApi_Close(CUR_CAIJI_VM);
						CUR_CAIJI_VM = ZL_EXP_NULL;
						return NULL;
					}
					zenglApi_Reset(CUR_CAIJI_VM);
					wxQueueEvent(glmainFrame->GetEventHandler(),eventForClearLogs.Clone());
				}
				else if(global_MyModules[itemdata->m_modID] == "group")
				{
					single_mod_cat_table = "group";
					single_mod_catname = treeCtrl->GetItemText(cateitemChild);
					single_mod_catid = itemdata->m_cateID;
					ModFilePath = gl_zenglrun_group_modname;
					zenglApi_SetModInitHandle(CUR_CAIJI_VM,"builtin",global_builtin_module_init);
					if(ModFilePath.Find(".zlencrypt") != wxNOT_FOUND) //以.zlencrypt为后缀的，默认为加密脚本
					{
						zenglApi_SetSourceRC4Key(CUR_CAIJI_VM,(char *)gl_encrypt_str.c_str().AsChar(),gl_encrypt_str.Length()); //加密脚本
					}
					else if(menubar->IsChecked(ID_MENU_EXTRA_DEBUG))
					{
						zenglApi_DebugSetBreakHandle(CUR_CAIJI_VM,global_debug_break,global_debug_conditionError,ZL_EXP_TRUE,ZL_EXP_FALSE);
					}
					if(zenglApi_Run(CUR_CAIJI_VM,(char *)ModFilePath.c_str().AsChar()) == -1) //编译执行zengl脚本
					{
						wxMyLogEvent eventforlog( wxEVT_MY_LOG_EVENT,wxID_ANY,wxString::Format("\n编译运行"+ModFilePath+"时发生异常：%s\n",zenglApi_GetErrorString(CUR_CAIJI_VM)),
									MY_RICHTEXT_RED_STYLE);
						wxQueueEvent(glmainFrame->GetEventHandler(),eventforlog.Clone());
						zenglApi_Close(CUR_CAIJI_VM);
						CUR_CAIJI_VM = ZL_EXP_NULL;
						return NULL;
					}
					zenglApi_Reset(CUR_CAIJI_VM);
					wxQueueEvent(glmainFrame->GetEventHandler(),eventForClearLogs.Clone());
				}
				else if(global_MyModules[itemdata->m_modID] == "sell")
				{
					single_mod_cat_table = "sell";
					single_mod_catname = treeCtrl->GetItemText(cateitemChild);
					single_mod_catid = itemdata->m_cateID;
					ModFilePath = gl_zenglrun_sell_modname;
					zenglApi_SetModInitHandle(CUR_CAIJI_VM,"builtin",global_builtin_module_init);
					if(ModFilePath.Find(".zlencrypt") != wxNOT_FOUND) //以.zlencrypt为后缀的，默认为加密脚本
					{
						zenglApi_SetSourceRC4Key(CUR_CAIJI_VM,(char *)gl_encrypt_str.c_str().AsChar(),gl_encrypt_str.Length()); //加密脚本
					}
					else if(menubar->IsChecked(ID_MENU_EXTRA_DEBUG))
					{
						zenglApi_DebugSetBreakHandle(CUR_CAIJI_VM,global_debug_break,global_debug_conditionError,ZL_EXP_TRUE,ZL_EXP_FALSE);
					}
					if(zenglApi_Run(CUR_CAIJI_VM,(char *)ModFilePath.c_str().AsChar()) == -1) //编译执行zengl脚本
					{
						wxMyLogEvent eventforlog( wxEVT_MY_LOG_EVENT,wxID_ANY,wxString::Format("\n编译运行"+ModFilePath+"时发生异常：%s\n",zenglApi_GetErrorString(CUR_CAIJI_VM)),
									MY_RICHTEXT_RED_STYLE);
						wxQueueEvent(glmainFrame->GetEventHandler(),eventforlog.Clone());
						zenglApi_Close(CUR_CAIJI_VM);
						CUR_CAIJI_VM = ZL_EXP_NULL;
						return NULL;
					}
					zenglApi_Reset(CUR_CAIJI_VM);
					wxQueueEvent(glmainFrame->GetEventHandler(),eventForClearLogs.Clone());
				}
				else if(global_MyModules[itemdata->m_modID] == "buy")
				{
					single_mod_cat_table = "buy";
					single_mod_catname = treeCtrl->GetItemText(cateitemChild);
					single_mod_catid = itemdata->m_cateID;
					ModFilePath = gl_zenglrun_buy_modname;
					zenglApi_SetModInitHandle(CUR_CAIJI_VM,"builtin",global_builtin_module_init);
					if(ModFilePath.Find(".zlencrypt") != wxNOT_FOUND) //以.zlencrypt为后缀的，默认为加密脚本
					{
						zenglApi_SetSourceRC4Key(CUR_CAIJI_VM,(char *)gl_encrypt_str.c_str().AsChar(),gl_encrypt_str.Length()); //加密脚本
					}
					else if(menubar->IsChecked(ID_MENU_EXTRA_DEBUG))
					{
						zenglApi_DebugSetBreakHandle(CUR_CAIJI_VM,global_debug_break,global_debug_conditionError,ZL_EXP_TRUE,ZL_EXP_FALSE);
					}
					if(zenglApi_Run(CUR_CAIJI_VM,(char *)ModFilePath.c_str().AsChar()) == -1) //编译执行zengl脚本
					{
						wxMyLogEvent eventforlog( wxEVT_MY_LOG_EVENT,wxID_ANY,wxString::Format("\n编译运行"+ModFilePath+"时发生异常：%s\n",zenglApi_GetErrorString(CUR_CAIJI_VM)),
									MY_RICHTEXT_RED_STYLE);
						wxQueueEvent(glmainFrame->GetEventHandler(),eventforlog.Clone());
						zenglApi_Close(CUR_CAIJI_VM);
						CUR_CAIJI_VM = ZL_EXP_NULL;
						return NULL;
					}
					zenglApi_Reset(CUR_CAIJI_VM);
					wxQueueEvent(glmainFrame->GetEventHandler(),eventForClearLogs.Clone());
				}
				else if(global_MyModules[itemdata->m_modID] == "exhibit")
				{
					single_mod_cat_table = "exhibit";
					single_mod_catname = treeCtrl->GetItemText(cateitemChild);
					single_mod_catid = itemdata->m_cateID;
					ModFilePath = gl_zenglrun_exhibit_modname;
					zenglApi_SetModInitHandle(CUR_CAIJI_VM,"builtin",global_builtin_module_init);
					if(ModFilePath.Find(".zlencrypt") != wxNOT_FOUND) //以.zlencrypt为后缀的，默认为加密脚本
					{
						zenglApi_SetSourceRC4Key(CUR_CAIJI_VM,(char *)gl_encrypt_str.c_str().AsChar(),gl_encrypt_str.Length()); //加密脚本
					}
					else if(menubar->IsChecked(ID_MENU_EXTRA_DEBUG))
					{
						zenglApi_DebugSetBreakHandle(CUR_CAIJI_VM,global_debug_break,global_debug_conditionError,ZL_EXP_TRUE,ZL_EXP_FALSE);
					}
					if(zenglApi_Run(CUR_CAIJI_VM,(char *)ModFilePath.c_str().AsChar()) == -1) //编译执行zengl脚本
					{
						wxMyLogEvent eventforlog( wxEVT_MY_LOG_EVENT,wxID_ANY,wxString::Format("\n编译运行"+ModFilePath+"时发生异常：%s\n",zenglApi_GetErrorString(CUR_CAIJI_VM)),
									MY_RICHTEXT_RED_STYLE);
						wxQueueEvent(glmainFrame->GetEventHandler(),eventforlog.Clone());
						zenglApi_Close(CUR_CAIJI_VM);
						CUR_CAIJI_VM = ZL_EXP_NULL;
						return NULL;
					}
					zenglApi_Reset(CUR_CAIJI_VM);
					wxQueueEvent(glmainFrame->GetEventHandler(),eventForClearLogs.Clone());
				}
				else if(global_MyModules[itemdata->m_modID] == "job")
				{
					single_mod_cat_table = "job";
					single_mod_catname = treeCtrl->GetItemText(cateitemChild);
					single_mod_catid = itemdata->m_cateID;
					ModFilePath = gl_zenglrun_job_modname;
					zenglApi_SetModInitHandle(CUR_CAIJI_VM,"builtin",global_builtin_module_init);
					if(ModFilePath.Find(".zlencrypt") != wxNOT_FOUND) //以.zlencrypt为后缀的，默认为加密脚本
					{
						zenglApi_SetSourceRC4Key(CUR_CAIJI_VM,(char *)gl_encrypt_str.c_str().AsChar(),gl_encrypt_str.Length()); //加密脚本
					}
					else if(menubar->IsChecked(ID_MENU_EXTRA_DEBUG))
					{
						zenglApi_DebugSetBreakHandle(CUR_CAIJI_VM,global_debug_break,global_debug_conditionError,ZL_EXP_TRUE,ZL_EXP_FALSE);
					}
					if(zenglApi_Run(CUR_CAIJI_VM,(char *)ModFilePath.c_str().AsChar()) == -1) //编译执行zengl脚本
					{
						wxMyLogEvent eventforlog( wxEVT_MY_LOG_EVENT,wxID_ANY,wxString::Format("\n编译运行"+ModFilePath+"时发生异常：%s\n",zenglApi_GetErrorString(CUR_CAIJI_VM)),
									MY_RICHTEXT_RED_STYLE);
						wxQueueEvent(glmainFrame->GetEventHandler(),eventforlog.Clone());
						zenglApi_Close(CUR_CAIJI_VM);
						CUR_CAIJI_VM = ZL_EXP_NULL;
						return NULL;
					}
					zenglApi_Reset(CUR_CAIJI_VM);
					wxQueueEvent(glmainFrame->GetEventHandler(),eventForClearLogs.Clone());
				}
				else if(global_MyModules[itemdata->m_modID] == "know")
				{
					single_mod_cat_table = "know";
					single_mod_catname = treeCtrl->GetItemText(cateitemChild);
					single_mod_catid = itemdata->m_cateID;
					ModFilePath = gl_zenglrun_know_modname;
					zenglApi_SetModInitHandle(CUR_CAIJI_VM,"builtin",global_builtin_module_init);
					if(ModFilePath.Find(".zlencrypt") != wxNOT_FOUND) //以.zlencrypt为后缀的，默认为加密脚本
					{
						zenglApi_SetSourceRC4Key(CUR_CAIJI_VM,(char *)gl_encrypt_str.c_str().AsChar(),gl_encrypt_str.Length()); //加密脚本
					}
					else if(menubar->IsChecked(ID_MENU_EXTRA_DEBUG))
					{
						zenglApi_DebugSetBreakHandle(CUR_CAIJI_VM,global_debug_break,global_debug_conditionError,ZL_EXP_TRUE,ZL_EXP_FALSE);
					}
					if(zenglApi_Run(CUR_CAIJI_VM,(char *)ModFilePath.c_str().AsChar()) == -1) //编译执行zengl脚本
					{
						wxMyLogEvent eventforlog( wxEVT_MY_LOG_EVENT,wxID_ANY,wxString::Format("\n编译运行"+ModFilePath+"时发生异常：%s\n",zenglApi_GetErrorString(CUR_CAIJI_VM)),
									MY_RICHTEXT_RED_STYLE);
						wxQueueEvent(glmainFrame->GetEventHandler(),eventforlog.Clone());
						zenglApi_Close(CUR_CAIJI_VM);
						CUR_CAIJI_VM = ZL_EXP_NULL;
						return NULL;
					}
					zenglApi_Reset(CUR_CAIJI_VM);
					wxQueueEvent(glmainFrame->GetEventHandler(),eventForClearLogs.Clone());
				}
				else if(global_MyModules[itemdata->m_modID] == "photo")
				{
					single_mod_cat_table = "photo";
					single_mod_catname = treeCtrl->GetItemText(cateitemChild);
					single_mod_catid = itemdata->m_cateID;
					ModFilePath = gl_zenglrun_photo_modname;
					zenglApi_SetModInitHandle(CUR_CAIJI_VM,"builtin",global_builtin_module_init);
					if(ModFilePath.Find(".zlencrypt") != wxNOT_FOUND) //以.zlencrypt为后缀的，默认为加密脚本
					{
						zenglApi_SetSourceRC4Key(CUR_CAIJI_VM,(char *)gl_encrypt_str.c_str().AsChar(),gl_encrypt_str.Length()); //加密脚本
					}
					else if(menubar->IsChecked(ID_MENU_EXTRA_DEBUG))
					{
						zenglApi_DebugSetBreakHandle(CUR_CAIJI_VM,global_debug_break,global_debug_conditionError,ZL_EXP_TRUE,ZL_EXP_FALSE);
					}
					if(zenglApi_Run(CUR_CAIJI_VM,(char *)ModFilePath.c_str().AsChar()) == -1) //编译执行zengl脚本
					{
						wxMyLogEvent eventforlog( wxEVT_MY_LOG_EVENT,wxID_ANY,wxString::Format("\n编译运行"+ModFilePath+"时发生异常：%s\n",zenglApi_GetErrorString(CUR_CAIJI_VM)),
									MY_RICHTEXT_RED_STYLE);
						wxQueueEvent(glmainFrame->GetEventHandler(),eventforlog.Clone());
						zenglApi_Close(CUR_CAIJI_VM);
						CUR_CAIJI_VM = ZL_EXP_NULL;
						return NULL;
					}
					zenglApi_Reset(CUR_CAIJI_VM);
					wxQueueEvent(glmainFrame->GetEventHandler(),eventForClearLogs.Clone());
				}
				else if(global_MyModules[itemdata->m_modID] == "video")
				{
					single_mod_cat_table = "video";
					single_mod_catname = treeCtrl->GetItemText(cateitemChild);
					single_mod_catid = itemdata->m_cateID;
					ModFilePath = gl_zenglrun_video_modname;
					zenglApi_SetModInitHandle(CUR_CAIJI_VM,"builtin",global_builtin_module_init);
					if(ModFilePath.Find(".zlencrypt") != wxNOT_FOUND) //以.zlencrypt为后缀的，默认为加密脚本
					{
						zenglApi_SetSourceRC4Key(CUR_CAIJI_VM,(char *)gl_encrypt_str.c_str().AsChar(),gl_encrypt_str.Length()); //加密脚本
					}
					else if(menubar->IsChecked(ID_MENU_EXTRA_DEBUG))
					{
						zenglApi_DebugSetBreakHandle(CUR_CAIJI_VM,global_debug_break,global_debug_conditionError,ZL_EXP_TRUE,ZL_EXP_FALSE);
					}
					if(zenglApi_Run(CUR_CAIJI_VM,(char *)ModFilePath.c_str().AsChar()) == -1) //编译执行zengl脚本
					{
						wxMyLogEvent eventforlog( wxEVT_MY_LOG_EVENT,wxID_ANY,wxString::Format("\n编译运行"+ModFilePath+"时发生异常：%s\n",zenglApi_GetErrorString(CUR_CAIJI_VM)),
									MY_RICHTEXT_RED_STYLE);
						wxQueueEvent(glmainFrame->GetEventHandler(),eventforlog.Clone());
						zenglApi_Close(CUR_CAIJI_VM);
						CUR_CAIJI_VM = ZL_EXP_NULL;
						return NULL;
					}
					zenglApi_Reset(CUR_CAIJI_VM);
					wxQueueEvent(glmainFrame->GetEventHandler(),eventForClearLogs.Clone());
				}
				else if(global_MyModules[itemdata->m_modID] == "down")
				{
					single_mod_cat_table = "down";
					single_mod_catname = treeCtrl->GetItemText(cateitemChild);
					single_mod_catid = itemdata->m_cateID;
					ModFilePath = gl_zenglrun_down_modname;
					zenglApi_SetModInitHandle(CUR_CAIJI_VM,"builtin",global_builtin_module_init);
					if(ModFilePath.Find(".zlencrypt") != wxNOT_FOUND) //以.zlencrypt为后缀的，默认为加密脚本
					{
						zenglApi_SetSourceRC4Key(CUR_CAIJI_VM,(char *)gl_encrypt_str.c_str().AsChar(),gl_encrypt_str.Length()); //加密脚本
					}
					else if(menubar->IsChecked(ID_MENU_EXTRA_DEBUG))
					{
						zenglApi_DebugSetBreakHandle(CUR_CAIJI_VM,global_debug_break,global_debug_conditionError,ZL_EXP_TRUE,ZL_EXP_FALSE);
					}
					if(zenglApi_Run(CUR_CAIJI_VM,(char *)ModFilePath.c_str().AsChar()) == -1) //编译执行zengl脚本
					{
						wxMyLogEvent eventforlog( wxEVT_MY_LOG_EVENT,wxID_ANY,wxString::Format("\n编译运行"+ModFilePath+"时发生异常：%s\n",zenglApi_GetErrorString(CUR_CAIJI_VM)),
									MY_RICHTEXT_RED_STYLE);
						wxQueueEvent(glmainFrame->GetEventHandler(),eventforlog.Clone());
						zenglApi_Close(CUR_CAIJI_VM);
						CUR_CAIJI_VM = ZL_EXP_NULL;
						return NULL;
					}
					zenglApi_Reset(CUR_CAIJI_VM);
					wxQueueEvent(glmainFrame->GetEventHandler(),eventForClearLogs.Clone());
				}
				else if(global_MyModules[itemdata->m_modID] == "company")
				{
					//wxQueueEvent(glmainFrame->GetEventHandler(),eventForClearLogs.Clone());
					continue;
				}
				else
				{
					myglStyle = MY_RICHTEXT_RED_STYLE;
					myglStr = "分类["+treeCtrl->GetItemText(cateitemChild) + "]对应的["+glHashStr[itemdata->m_modID] + "]模块不在采集范围内！\n";
					eventForLog.SetEventMsg(myglStr,myglStyle);
					wxQueueEvent(glmainFrame->GetEventHandler(),eventForLog.Clone());
				}
				single_mod_processPos = single_mod_processNum * 2;
				if((iRemain=global_mod_processPos%(single_mod_processNum * 2))!=0)
					global_mod_processPos += (single_mod_processNum * 2 - iRemain);
				eventForProgress.SetEventNum(single_mod_processPos,single_mod_processNum,global_mod_processPos,global_mod_processNum);
				wxQueueEvent(glmainFrame->GetEventHandler(),eventForProgress.Clone());
			}//if
		}while(cateitemChild = treeCtrl->GetNextChild(cateitemChild,cookie2));
	}while(cateitemid = treeCtrl->GetNextChild(cateitemid,cookie));
	if(TestDestroy())
	{
		if(CUR_CAIJI_VM != ZL_EXP_NULL)
		{
			zenglApi_Close(CUR_CAIJI_VM);
			CUR_CAIJI_VM =ZL_EXP_NULL;
		}
		return NULL;
	}
	eventForLog.SetEventMsg(wxString::Format("\n网站[%s]一共采集了%d条信息！发布成功%d条信息//////////////////////////////\n",
		webname,global_mod_totalcaijinum,global_mod_totalpostnum),MY_RICHTEXT_NORMAL_STYLE);
	wxQueueEvent(glmainFrame->GetEventHandler(),eventForLog.Clone());
	eventForProgress.SetEventNum(2,1,2,1);
	wxQueueEvent(glmainFrame->GetEventHandler(),eventForProgress.Clone());
	m_frame->m_taskBarIcon->ShowBalloon("本地数据采集并发布完毕！","温馨提示!");
	global_mod_processNum = 0;
	global_mod_processPos = 0;
	global_mod_totalpostnum = 0;
	global_mod_totalcaijinum = 0;
	single_mod_processNum = 0;
	single_mod_processPos = 0;
	single_mod_curpostnum = 0;
	single_mod_caijinum = 0;
	single_mod_cat_table = "";
	single_mod_catname = "";
	single_mod_catid = "";
	single_mod_modid = "";
	if(CUR_CAIJI_VM != ZL_EXP_NULL)
	{
		zenglApi_Close(CUR_CAIJI_VM);
		CUR_CAIJI_VM =ZL_EXP_NULL;
	}
	return NULL;
}

void MyThread2::OnExit()
{
	//wxCriticalSectionLocker enter(wxGetApp().m_critsect);
	m_frame->mythread2 = NULL;
	wxAuiToolBarItem *item = glmainFrame->m_toolbar->FindTool(ID_ITEM_START);
	item->SetLabel("开始");
}

void MyTreeCtrl::OnItemStateClick(wxTreeEvent& event)
{
	wxTreeItemId itemId = event.GetItem();
	MyTreeItemData *itemdata = (MyTreeItemData *)GetItemData(itemId);
	if(itemdata->m_url != "")
	{
		/*wxCommandEvent event( wxEVT_MY_CHECKS , ID_MY_WINDOW);
		event.SetEventObject( glmainFrame );
		glmainFrame->GetEventHandler()->ProcessEvent( event );*/
		wxTreeItemIdValue cookie,cookie2;
		wxTreeItemId rootid = GetRootItem();
		wxTreeItemId childId = GetFirstChild(rootid,cookie);
		do
		{
			if(childId.IsOk())
			{
				wxTreeItemId childId2 = GetFirstChild(childId,cookie2);
				do
				{
					if(childId2.IsOk())
						SetItemState(childId2,wxTREE_ITEMSTATE_NEXT);
				}while(childId2 = GetNextChild(childId2,cookie2));
			}
		}while(childId = GetNextChild(childId,cookie));
	}
	else if(itemdata->m_cateID == "0")
	{
		wxTreeItemIdValue cookie;
		wxTreeItemId childId = GetFirstChild(itemId,cookie);
		do{
			if(childId.IsOk())
				SetItemState(childId,wxTREE_ITEMSTATE_NEXT);
		}while(childId = GetNextChild(childId,cookie));
	}
	else
		SetItemState(itemId, wxTREE_ITEMSTATE_NEXT);
}

void MyTreeCtrl::OnItemActivated(wxTreeEvent& event)
{
	wxTreeItemId itemId = event.GetItem();
	MyTreeItemData *itemdata = (MyTreeItemData *)GetItemData(itemId);
	if(itemdata->m_url == "" && itemdata->m_cateID != "0")
	{
		wxPoint mainWinLoc=this->GetScreenPosition();
		mainWinLoc.x += 200;
		mainWinLoc.y += 20;
		wxSize mainWinSize=this->GetSize();
		wxString title = GetItemText(itemId);
		glIsDialogInShow = true;
		MyKeyDialog keydialog(glmainFrame,title,mainWinLoc,mainWinSize,itemdata->m_cateID,itemdata->m_modID,itemdata->m_catname);
		keydialog.Create();
		keydialog.ShowModal();
		glIsDialogInShow = false;
	}
}

void MyTaskBarIcon::OnMenuRestore(wxCommandEvent& )
{
	//if(!glmainFrame->IsTopLevel())
	glmainFrame->Raise();  // bring window to front
	glmainFrame->Show(true); // show the window
	if(glmainFrame->IsIconized())
		glmainFrame->Iconize(false);
}

void MyTaskBarIcon::OnHideToTray(wxCommandEvent& event)
{
	glmainFrame->Show(false); // show the window
	//glmainFrame->Iconize(false);
}

void MyTaskBarIcon::OnMenuExit(wxCommandEvent& )
{
	if(glIsDialogInShow == true)
	{
		wxMessageBox("还有对话框没有关闭，请先关闭所有弹出的对话框，再退出！","警告");
		return;
	}
	glmainFrame->Close(true);
}

void MyTaskBarIcon::OnLeftButtonClick(wxTaskBarIconEvent&)
{
	//glmainFrame->Iconize(false); // restore the window if minimized
	//glmainFrame->SetFocus();  // focus on my window
	if(!glmainFrame->IsShown())
	{
		//if(!glmainFrame->IsTopLevel())
		glmainFrame->Raise();  // bring window to front
		glmainFrame->Show(true); // show the window
		if(glmainFrame->IsIconized())
			glmainFrame->Iconize(false);
	}
	else
	{
		glmainFrame->Show(false); // show the window
		//glmainFrame->Iconize(false);
	}
}

// Overridables
wxMenu *MyTaskBarIcon::CreatePopupMenu()
{
    wxMenu *menu = new wxMenu;
    menu->Append(PU_RESTORE, wxT("恢复主窗口"));
	menu->Append(PU_HIDE, wxT("隐藏主窗口"));
    menu->AppendSeparator();
    menu->Append(PU_EXIT,    wxT("退出"));
    return menu;
}

bool MyTaskBarIcon::ShowBalloon(wxString title, wxString msg, int iconID, unsigned int timeout)
{
	if (!IsOk()) {
        return false;
    }

	NOTIFYICONDATA notifyData;
	memset(&notifyData, 0, sizeof(notifyData));
	notifyData.cbSize = NOTIFYICONDATA_V2_SIZE;//sizeof(notifyData);
	notifyData.hWnd = (HWND)m_win->GetHWND();
	notifyData.uCallbackMessage = ::RegisterWindowMessage(wxT(
        "wxTaskBarIconMessage"));
	notifyData.uFlags = NIF_MESSAGE;

	notifyData.uFlags |= NIF_INFO;
    lstrcpyn(notifyData.szInfo, msg.c_str(), sizeof(notifyData.szInfo));
    lstrcpyn(notifyData.szInfoTitle, title.c_str(), sizeof
        (notifyData.szInfoTitle));
    notifyData.dwInfoFlags = iconID; // | NIIF_NOSOUND; modified by palinx
    notifyData.uTimeout = timeout;

    notifyData.uID = 99;

	if (m_iconAdded) {
        return (Shell_NotifyIcon(NIM_MODIFY, &notifyData) != 0);
    } else {
        return false;
    }
}

MyUserExtraData::MyUserExtraData()
{
	this->isQuerySql = 0;
	this->curl = NULL;
}

MyUserExtraData::~MyUserExtraData()
{
	//if(this->isQuerySql)
		this->sqlset.~wxSQLite3ResultSet();
	if(this->db.IsOpen())
		this->db.Close();
	if(this->curl != NULL)
		curl_easy_cleanup(this->curl);
}
