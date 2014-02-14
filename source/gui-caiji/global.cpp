#include "global.h"
#include <wx/arrimpl.cpp>

#define ZL_EXP_OS_IN_WINDOWS //用于告诉zengl嵌入式脚本语言当前的执行环境是windows系统，防止出现编译错误
#include "zengl_exportfuns.h" //测试zengl v1.2.2

wxString global_SettingFile ="规则配置数据库.db";
wxSQLite3Database  *settingDB;
MyHashString glHashStr,global_MyModules,glMyModuleNames;
int global_mod_processNum = 0;
int global_mod_processPos = 0;
int global_mod_totalpostnum = 0;
int global_mod_totalcaijinum = 0;
int single_mod_processNum = 0;
int single_mod_processPos = 0;
int single_mod_curpostnum = 0;
int single_mod_caijinum = 0;
unsigned int global_seed = 0;
wxString single_mod_cat_table = "";
wxString single_mod_catname = "";
wxString single_mod_catid = "";
wxString single_mod_modid = "";
wxString ContactContent = "";
wxString global_my_weburl = "";
wxString gl_initManage_Url = "";
extern wxString glCompanyCatID;
wxScopedCharBuffer myglStrForUTF8_ZenglRun;

extern FILE * zengl_debuglogfile;

WX_DEFINE_OBJARRAY(ArrayOfMyAreaObj);

ArrayOfMyAreaObj global_MyAreas;

MyAreaObj::MyAreaObj(wxString strName,wxString strID)
{
	areaName = strName;
	areaID = strID;
}

void InitHashMap()
{
	if(glHashStr.size() > 0)
		return;
	glHashStr["21"] = _("资讯");
	glHashStr["13"] = _("品牌");
	glHashStr["6"] = _("求购");
	glHashStr["4"] = _("公司");
	glHashStr["15"] = _("下载");
	glHashStr["8"] = _("展会");
	glHashStr["17"] = _("团购");
	glHashStr["22"] = _("信息/招商");
	glHashStr["9"] = _("人才");
	glHashStr["10"] = _("知道");
	glHashStr["16"] = _("商城");
	glHashStr["7"] = _("行情");
	glHashStr["5"] = _("供应");
	glHashStr["11"] = _("专题");
	glHashStr["12"] = _("图库");
	glHashStr["14"] = _("视频");

}

void global_GetUrlContent(wxString remote_url)
{
	curl_easy_setopt(curl, CURLOPT_URL, remote_url.c_str().AsChar());
	curl_easy_setopt(curl, CURLOPT_USERAGENT ,"Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.2; .NET CLR 1.1.4322)");
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION ,1);
	long timeout;
	glmainFrame->m_textForTimeOut->GetValue().ToLong(&timeout);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, (int)timeout);//设置超时时间
	//curl_easy_setopt(curl, CURLOPT_TIMEOUT, 60);//设置超时时间
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, mywxcurl_string_write);
	/*if((fp=fopen("test","w"))==NULL)
    {
        curl_easy_cleanup(curl);
		myglStyle = MY_RICHTEXT_RED_STYLE;
		myglStr = "打开test文件失败！\n";
		wxMyLogEvent event( wxEVT_MY_LOG_EVENT,wxID_ANY,myglStr,myglStyle);
		wxQueueEvent(glmainFrame->GetEventHandler(),event.Clone());
		return;
    }*/
	curl_easy_perform(curl);
	if(char_myglStr==NULL)
	{
		charlength = 0;
		myglStyle = MY_RICHTEXT_RED_STYLE;
		myglStr = _T("抓取网址：") + remote_url + _T("时，没有获取到任何数据，详细信息：");
		myglStr += " null char content get! 原因可能是网站连接失败或超时，或者是该网址无法访问等原因！\n";
		wxMyLogEvent event( wxEVT_MY_LOG_EVENT,wxID_ANY,myglStr,myglStyle);
		wxQueueEvent(glmainFrame->GetEventHandler(),event.Clone());
		return;
	}
	charlength = strlen(char_myglStr);
	if((myglStr = wxString(char_myglStr)) == "")
		myglStr = wxString::FromUTF8(char_myglStr);
	
	//wxScopedCharBuffer myglStrForUTF8 = myglStr.ToUTF8();
	//fwrite(myglStrForUTF8.data(), 1 , myglStrForUTF8.length(),fp);
	//fclose(fp);
	free(char_myglStr);
	char_myglStr = NULL;
	char_myglTotalnum = 0;
}

extern "C"
{
	wxSQLite3ResultSet gl_sqlset_forZenglRun;
	int gl_isQuerySql_forZenglRun = 0;
	
	ZL_EXP_INT global_userdef_compile_info_forZenglRunV2(ZL_EXP_CHAR * infoStrPtr, ZL_EXP_INT infoStrCount)
	{
		fprintf(zengl_debuglogfile,"%s",infoStrPtr);
		return 0;
	}

	ZL_EXP_INT global_userdef_compile_error_forZenglRunV2(ZL_EXP_CHAR * infoStrPtr, ZL_EXP_INT infoStrCount,ZL_EXP_VOID * VM_ARG)
	{
		wxString printStr = infoStrPtr;
		wxMyLogEvent eventforlog( wxEVT_MY_LOG_EVENT,wxID_ANY);
		eventforlog.SetEventMsg(printStr,MY_RICHTEXT_RED_STYLE);
		wxQueueEvent(glmainFrame->GetEventHandler(),eventforlog.Clone());
		return 0;
	}

	ZL_EXP_INT global_userdef_run_info_forZenglRunV2(ZL_EXP_CHAR * infoStrPtr, ZL_EXP_INT infoStrCount)
	{
		fprintf(zengl_debuglogfile,"%s",infoStrPtr);
		return 0;
	}

	ZL_EXP_INT global_userdef_run_print_forZenglRunV2(ZL_EXP_CHAR * infoStrPtr, ZL_EXP_INT infoStrCount)
	{
		wxString printStr = infoStrPtr;
		printStr += "\n";
		wxMyLogEvent eventforlog( wxEVT_MY_LOG_EVENT,wxID_ANY);
		eventforlog.SetEventMsg(printStr,MY_RICHTEXT_NORMAL_STYLE);
		wxQueueEvent(glmainFrame->GetEventHandler(),eventforlog.Clone());
		return 0;
	}

	ZL_EXP_INT global_userdef_run_error_forZenglRunV2(ZL_EXP_CHAR * infoStrPtr, ZL_EXP_INT infoStrCount,ZL_EXP_VOID * VM_ARG)
	{
		wxString printStr = infoStrPtr;
		wxMyLogEvent eventforlog( wxEVT_MY_LOG_EVENT,wxID_ANY);
		eventforlog.SetEventMsg(printStr,MY_RICHTEXT_RED_STYLE);
		wxQueueEvent(glmainFrame->GetEventHandler(),eventforlog.Clone());
		return 0;
	}
	
	/*printf模块函数，在LOG日志文本框中显示信息函数*/
	ZL_EXP_VOID global_printf(ZL_EXP_VOID * VM_ARG,ZL_EXP_INT argcount)
	{
		ZENGL_EXPORT_MOD_FUN_ARG arg = {ZL_EXP_FAT_NONE,{0}};
		ZL_EXP_INT color;
		wxString printStr;
		if(argcount != 2)
		{
			zenglApi_SetErrThenStop(VM_ARG,"printf函数必须接受2个参数");
			return;
		}
		zenglApi_GetFunArg(VM_ARG,1,&arg); //得到第一个参数
		if(arg.type != ZL_EXP_FAT_STR)
		{
			zenglApi_SetErrThenStop(VM_ARG,"printf函数第一个参数必须是字符串，表示要显示的内容");
			return;
		}
		printStr = arg.val.str;
		zenglApi_GetFunArg(VM_ARG,2,&arg); //得到第二个参数
		if(arg.type != ZL_EXP_FAT_INT)
		{
			zenglApi_SetErrThenStop(VM_ARG,"printf函数第二个参数必须是整数，表示要显示的颜色，0代表普通黑色，1代表红色，2代表绿色");
			return;
		}
		color = arg.val.integer;
		wxMyLogEvent eventforlog( wxEVT_MY_LOG_EVENT,wxID_ANY);
		if(color == 0)
			eventforlog.SetEventMsg(printStr,MY_RICHTEXT_NORMAL_STYLE);
		else if(color == 1)
			eventforlog.SetEventMsg(printStr,MY_RICHTEXT_RED_STYLE);
		else if(color == 2)
			eventforlog.SetEventMsg(printStr,MY_RICHTEXT_GREEN_STYLE);
		else
		{
			zenglApi_SetErrThenStop(VM_ARG,"printf函数第二个参数值错误：颜色参数只能是0,1,2三个值之一，0代表普通黑色，1代表红色，2代表绿色\n");
			return;
		}
		wxQueueEvent(glmainFrame->GetEventHandler(),eventforlog.Clone());
		zenglApi_SetRetVal(VM_ARG,ZL_EXP_FAT_INT,ZL_EXP_NULL,1,0);
	}
	
	/*read模块函数，弹出一个输入对话框，在该对话框中可以接受用户的输入*/
	ZL_EXP_VOID global_read(ZL_EXP_VOID * VM_ARG,ZL_EXP_INT argcount)
	{
		ZENGL_EXPORT_MOD_FUN_ARG arg = {ZL_EXP_FAT_NONE,{0}};
		wxString ReadStr = "",Message = "",Caption = "";
		if(argcount >= 3)
		{
			for(int i=1;i<=3;i++)
			{
				zenglApi_GetFunArg(VM_ARG,i,&arg);
				switch(arg.type)
				{
				case ZL_EXP_FAT_INT:
					ReadStr.sprintf("%d",arg.val.integer);
					break;
				case ZL_EXP_FAT_FLOAT:
					ReadStr.sprintf("%.16g",arg.val.floatnum);
					break;
				case ZL_EXP_FAT_STR:
					ReadStr.sprintf("%s",arg.val.str);
					break;
				default:
					zenglApi_SetErrThenStop(VM_ARG,"read函数第%d个参数类型无效，目前只支持字符串，整数，浮点数类型的参数",i);
					return;
					break;
				} //switch(arg.type)
				switch(i)
				{
				case 1:
					Message = ReadStr;
					break;
				case 2:
					Caption = ReadStr;
					break;
				}
			} //for(i=1;i<=3;i++)
		} //if(argcount >= 3)
		wxTextEntryDialog dialog(glmainFrame,
                           Message,
                           Caption,
                           ReadStr,
                           wxOK | wxCANCEL);
		if(dialog.ShowModal() == wxID_OK)
			zenglApi_SetRetVal(VM_ARG,ZL_EXP_FAT_STR,(ZL_EXP_CHAR *)dialog.GetValue().c_str().AsChar(),0,0);
		else
			zenglApi_SetRetVal(VM_ARG,ZL_EXP_FAT_STR,"",0,0);
	}
	
	/*bltRandom模块函数，产生随机数*/
	ZL_EXP_VOID global_bltRandom(ZL_EXP_VOID * VM_ARG,ZL_EXP_INT argcount)
	{
		time_t t;
		static int random_seed;
		if(random_seed == 0) //第一次使用时间作为随机种子。
		{
			srand((unsigned) time(&t));
			random_seed = rand();
		}
		else //其他时候使用上一次生成的随机数作为随机种子
		{
			srand(random_seed);
			random_seed = rand();
		}
		zenglApi_SetRetVal(VM_ARG,ZL_EXP_FAT_INT,ZL_EXP_NULL,random_seed,0);
	}
	
	/*array模块函数，用于创建zengl脚本的动态数组*/
	ZL_EXP_VOID global_array(ZL_EXP_VOID * VM_ARG,ZL_EXP_INT argcount)
	{
		ZENGL_EXPORT_MEMBLOCK memblock = {0};
		ZENGL_EXPORT_MOD_FUN_ARG arg = {ZL_EXP_FAT_NONE,{0}};
		ZL_EXP_INT i;
		if(argcount == 0) //如果array函数没带参数，则创建一个默认大小的未初始化的数组
		{
			if(zenglApi_CreateMemBlock(VM_ARG,&memblock,0) == -1)
				zenglApi_Exit(VM_ARG,zenglApi_GetErrorString(VM_ARG));
			zenglApi_SetRetValAsMemBlock(VM_ARG,&memblock);
		}
		else if(argcount >= 1) //如果带了参数则使用参数作为函数的初始值
		{
			if(zenglApi_CreateMemBlock(VM_ARG,&memblock,0) == -1)
				zenglApi_Exit(VM_ARG,zenglApi_GetErrorString(VM_ARG));
			for(i=1;i<=argcount;i++)
			{
				zenglApi_GetFunArg(VM_ARG,i,&arg);
				switch(arg.type)
				{
				case ZL_EXP_FAT_INT:
				case ZL_EXP_FAT_FLOAT:
				case ZL_EXP_FAT_STR:
				case ZL_EXP_FAT_MEMBLOCK:
				case ZL_EXP_FAT_ADDR:
				case ZL_EXP_FAT_ADDR_LOC:
				case ZL_EXP_FAT_ADDR_MEMBLK:
					zenglApi_SetMemBlock(VM_ARG,&memblock,i,&arg);
					break;
				default:
					zenglApi_Exit(VM_ARG,"array函数第%d个参数类型无效",i);
					break;
				}
			}
			zenglApi_SetRetValAsMemBlock(VM_ARG,&memblock);
		}
		else
			zenglApi_Exit(VM_ARG,"array函数异常：参数个数小于0");
	}
	
	/*递归打印出数组信息*/
	ZL_EXP_VOID global_print_array(ZL_EXP_VOID * VM_ARG,ZENGL_EXPORT_MEMBLOCK memblock,ZL_EXP_INT recur_count)
	{
		ZL_EXP_INT size,i,j;
		ZENGL_EXPORT_MOD_FUN_ARG mblk_val = {ZL_EXP_FAT_NONE,{0}};
		wxMyLogEvent eventforlog( wxEVT_MY_LOG_EVENT,wxID_ANY);
		zenglApi_GetMemBlockInfo(VM_ARG,&memblock,&size,ZL_EXP_NULL);
		for(i=1;i<=size;i++)
		{
			mblk_val = zenglApi_GetMemBlock(VM_ARG,&memblock,i);
			switch(mblk_val.type)
			{
			case ZL_EXP_FAT_INT:
			case ZL_EXP_FAT_FLOAT:
			case ZL_EXP_FAT_STR:
			case ZL_EXP_FAT_MEMBLOCK:
				for(j=0;j<recur_count;j++)
				{
					eventforlog.SetEventMsg("  ",MY_RICHTEXT_NORMAL_STYLE);
					wxQueueEvent(glmainFrame->GetEventHandler(),eventforlog.Clone());
				}
				break;
			}
			switch(mblk_val.type)
			{
			case ZL_EXP_FAT_INT:
				eventforlog.SetEventMsg(wxString::Format("[%d] %d\n",i-1,mblk_val.val.integer),MY_RICHTEXT_NORMAL_STYLE);
				wxQueueEvent(glmainFrame->GetEventHandler(),eventforlog.Clone());
				break;
			case ZL_EXP_FAT_FLOAT:
				eventforlog.SetEventMsg(wxString::Format("[%d] %.16g\n",i-1,mblk_val.val.floatnum),MY_RICHTEXT_NORMAL_STYLE);
				wxQueueEvent(glmainFrame->GetEventHandler(),eventforlog.Clone());
				break;
			case ZL_EXP_FAT_STR:
				eventforlog.SetEventMsg(wxString::Format("[%d] %s\n",i-1,mblk_val.val.str),MY_RICHTEXT_NORMAL_STYLE);
				wxQueueEvent(glmainFrame->GetEventHandler(),eventforlog.Clone());
				break;
			case ZL_EXP_FAT_MEMBLOCK:
				eventforlog.SetEventMsg(wxString::Format("[%d] <array or class obj type> begin:\n",i-1),MY_RICHTEXT_NORMAL_STYLE);
				wxQueueEvent(glmainFrame->GetEventHandler(),eventforlog.Clone());
				global_print_array(VM_ARG,mblk_val.val.memblock,recur_count+1);
				eventforlog.SetEventMsg(wxString::Format("[%d] <array or class obj type> end\n",i-1),MY_RICHTEXT_NORMAL_STYLE);
				wxQueueEvent(glmainFrame->GetEventHandler(),eventforlog.Clone());
				break;
			}
		}
	}

	/*bltPrintArray模块函数，打印数组中的元素*/
	ZL_EXP_VOID global_bltPrintArray(ZL_EXP_VOID * VM_ARG,ZL_EXP_INT argcount)
	{
		ZENGL_EXPORT_MOD_FUN_ARG arg = {ZL_EXP_FAT_NONE,{0}};
		if(argcount < 1)
			zenglApi_Exit(VM_ARG,"bltPrintArray函数参数不可为空，必须指定一个数组或类对象为参数");
		zenglApi_GetFunArg(VM_ARG,1,&arg);
		if(arg.type != ZL_EXP_FAT_MEMBLOCK)
			zenglApi_Exit(VM_ARG,"bltPrintArray函数的参数不是数组或类对象");
		global_print_array(VM_ARG,arg.val.memblock,0);
		zenglApi_SetRetVal(VM_ARG,ZL_EXP_FAT_INT,ZL_EXP_NULL,1,0);
	}
	
	/*bltTestAddr模块函数(仅供测试)，测试参数引用*/
	ZL_EXP_VOID global_bltTestAddr(ZL_EXP_VOID * VM_ARG,ZL_EXP_INT argcount)
	{
		ZENGL_EXPORT_MOD_FUN_ARG arg = {ZL_EXP_FAT_NONE,{0}};
		wxMyLogEvent eventforlog( wxEVT_MY_LOG_EVENT,wxID_ANY);
		if(argcount < 2)
			zenglApi_Exit(VM_ARG,"bltTestAddr函数参数不可少于两个");
		zenglApi_GetFunArgInfo(VM_ARG,1,&arg);
		if(arg.type != ZL_EXP_FAT_ADDR && 
			arg.type != ZL_EXP_FAT_ADDR_LOC &&
			arg.type != ZL_EXP_FAT_ADDR_MEMBLK
			)
			zenglApi_Exit(VM_ARG,"第一个参数必须是变量的引用，或数组成员的引用，或者类对象成员的引用");
		zenglApi_GetFunArg(VM_ARG,1,&arg); //获取第一个参数的值，zenglApi_GetFunArg可以递归引用，找到引用的变量的值
		if(arg.type != ZL_EXP_FAT_STR)
			zenglApi_Exit(VM_ARG,"bltTestAddr函数目前只能接受字符串作为参数");
		eventforlog.SetEventMsg(wxString::Format("the value of first arg is %s\n",arg.val.str),MY_RICHTEXT_NORMAL_STYLE);
		wxQueueEvent(glmainFrame->GetEventHandler(),eventforlog.Clone());
		zenglApi_GetFunArg(VM_ARG,2,&arg); //得到第二个参数，并用该参数的字符串值设置第一个参数引用的变量
		if(arg.type != ZL_EXP_FAT_STR)
			zenglApi_Exit(VM_ARG,"bltTestAddr函数目前只能接受字符串作为参数");
		zenglApi_SetFunArg(VM_ARG,1,&arg);
		zenglApi_SetRetVal(VM_ARG,ZL_EXP_FAT_INT,ZL_EXP_NULL,1,0);
	}
	
	/*bltSetArray模块函数，使用第2个，第3个等参数来设置第一个参数对应的数组中的元素*/
	ZL_EXP_VOID global_bltSetArray(ZL_EXP_VOID * VM_ARG,ZL_EXP_INT argcount)
	{
		ZENGL_EXPORT_MOD_FUN_ARG arg = {ZL_EXP_FAT_NONE,{0}};
		ZENGL_EXPORT_MEMBLOCK memblock = {0};
		ZL_EXP_INT i;
		if(argcount < 2)
			zenglApi_Exit(VM_ARG,"bltSetArray函数参数不可少于两个");
		zenglApi_GetFunArg(VM_ARG,1,&arg);
		if(arg.type != ZL_EXP_FAT_MEMBLOCK)
			zenglApi_Exit(VM_ARG,"bltSetArray函数第一个参数必须是数组或类对象或这两者的引用类型");
		memblock = arg.val.memblock;
		for(i=2;i<=argcount;i++)
		{
			zenglApi_GetFunArg(VM_ARG,i,&arg);
			zenglApi_SetMemBlock(VM_ARG,&memblock,i-1,&arg);
		}
		zenglApi_SetRetVal(VM_ARG,ZL_EXP_FAT_INT,ZL_EXP_NULL,1,0);
	}
	
	ZL_EXP_VOID global_module_init(ZL_EXP_VOID * VM_ARG);

	/*bltLoadScript模块函数，新建一个虚拟机，加载并执行某个脚本*/
	ZL_EXP_VOID global_bltLoadScript(ZL_EXP_VOID * VM_ARG,ZL_EXP_INT argcount)
	{
		ZENGL_EXPORT_MOD_FUN_ARG arg = {ZL_EXP_FAT_NONE,{0}};
		ZL_EXP_CHAR * scriptName = ZL_EXP_NULL;
		zenglApi_GetFunArg(VM_ARG,1,&arg); //获取第一个参数为脚本名
		if(arg.type != ZL_EXP_FAT_STR)
			zenglApi_Exit(VM_ARG,"bltLoadScript函数第一个参数必须字符串，代表要加载的脚本文件名");
		scriptName = arg.val.str;
		ZENGL_EXPORT_VM_MAIN_ARGS vm_main_args = {global_userdef_compile_info_forZenglRunV2 , 
								  global_userdef_compile_error_forZenglRunV2,
								  global_userdef_run_info_forZenglRunV2,
								  global_userdef_run_print_forZenglRunV2,
								  global_userdef_run_error_forZenglRunV2,
								  global_module_init,
								  ZL_EXP_CP_AF_IN_DEBUG_MODE | 
								  ZL_EXP_CP_AF_OUTPUT_DEBUG_INFO};
		zenglApi_Load(scriptName,&vm_main_args); 
		zenglApi_SetRetVal(VM_ARG,ZL_EXP_FAT_INT,ZL_EXP_NULL,1,0);
	}
	
	/*bltGetZLVersion模块函数，获取当前zengl版本号信息的字符串形式*/
	ZL_EXP_VOID global_bltGetZLVersion(ZL_EXP_VOID * VM_ARG,ZL_EXP_INT argcount)
	{
		ZL_EXP_CHAR version[20] = {0};
		sprintf(version,"%d.%d.%d",ZL_EXP_MAJOR_VERSION,ZL_EXP_MINOR_VERSION,ZL_EXP_REVISION);
		zenglApi_SetRetVal(VM_ARG,ZL_EXP_FAT_STR,version,0,0);
	}
	
	/*bltGetCaijiNum模块函数，获取当前每个分类要采集的数量*/
	ZL_EXP_VOID global_bltGetCaijiNum(ZL_EXP_VOID * VM_ARG,ZL_EXP_INT argcount)
	{
		zenglApi_SetRetVal(VM_ARG,ZL_EXP_FAT_INT,ZL_EXP_NULL,single_mod_processNum,0);
	}
	
	/*bltGetCatid模块函数， 获取当前的分类ID*/
	ZL_EXP_VOID global_bltGetCatid(ZL_EXP_VOID * VM_ARG,ZL_EXP_INT argcount)
	{
		long ret;
		single_mod_catid.ToLong(&ret);
		zenglApi_SetRetVal(VM_ARG,ZL_EXP_FAT_INT,ZL_EXP_NULL,(int)ret,0);
	}

	/*bltGetModid模块函数，获取当前的模块ID*/
	ZL_EXP_VOID global_bltGetModid(ZL_EXP_VOID * VM_ARG,ZL_EXP_INT argcount)
	{
		long ret;
		single_mod_modid.ToLong(&ret);
		zenglApi_SetRetVal(VM_ARG,ZL_EXP_FAT_INT,ZL_EXP_NULL,(int)ret,0);
	}
	
	/*bltGetCatName模块函数，获取当前采集的分类名*/
	ZL_EXP_VOID global_bltGetCatName(ZL_EXP_VOID * VM_ARG,ZL_EXP_INT argcount)
	{
		zenglApi_SetRetVal(VM_ARG,ZL_EXP_FAT_STR,(char *)single_mod_catname.c_str().AsChar(),0,0);
	}

	/*bltGetWebUrl模块函数，获取当前采集要上传的目标网址*/
	ZL_EXP_VOID global_bltGetWebUrl(ZL_EXP_VOID * VM_ARG,ZL_EXP_INT argcount)
	{
		global_my_weburl.Replace("http://","");
		if(global_my_weburl.Last()=='/')
			global_my_weburl.RemoveLast();
		zenglApi_SetRetVal(VM_ARG,ZL_EXP_FAT_STR,(char *)global_my_weburl.c_str().AsChar(),0,0);
	}

	/*bltGetInitManageUrl模块函数，获取管理界面的URL 在本采集器中即为采集上传接口的文件名*/
	ZL_EXP_VOID global_bltGetInitManageUrl(ZL_EXP_VOID * VM_ARG,ZL_EXP_INT argcount)
	{
		zenglApi_SetRetVal(VM_ARG,ZL_EXP_FAT_STR,(char *)gl_initManage_Url.c_str().AsChar(),0,0);
	}

	/*bltGetKeyWords模块函数，获取分类的关键词*/
	ZL_EXP_VOID global_bltGetKeyWords(ZL_EXP_VOID * VM_ARG,ZL_EXP_INT argcount)
	{
		ZENGL_EXPORT_MOD_FUN_ARG arg = {ZL_EXP_FAT_NONE,{0}};
		ZENGL_EXPORT_MEMBLOCK keywords_memblock = {0},musts_memblock = {0};
		if(argcount != 2)
			zenglApi_Exit(VM_ARG,"bltGetKeyWords函数必须有两个参数");
		zenglApi_GetFunArg(VM_ARG,1,&arg);
		if(arg.type != ZL_EXP_FAT_MEMBLOCK)
			zenglApi_Exit(VM_ARG,"bltGetKeyWords函数第一个参数必须是数组，用以存放关键字信息");
		keywords_memblock = arg.val.memblock;
		zenglApi_GetFunArg(VM_ARG,2,&arg);
		if(arg.type != ZL_EXP_FAT_MEMBLOCK)
			zenglApi_Exit(VM_ARG,"bltGetKeyWords函数第二个参数必须是数组，用以存放关键字的必须包含词信息");
		musts_memblock = arg.val.memblock;
		InitHashMap();
		wxSQLite3Database * db = glDB;
		wxString tmpStr;
		if(!wxFileExists(glSiteDBPath))
		{
			tmpStr = glCaijiWebName + "网站对应的本地数据库文件"+glSiteDBPath+"不存在！\n";
			zenglApi_SetErrThenStop(VM_ARG,(char *)tmpStr.c_str().AsChar());
			return;
		}
		if(!db->IsOpen())
			db->Open(glSiteDBPath);
		wxString sqlstr="";
		sqlstr.Printf("SELECT * FROM keywords WHERE catid='%s'",single_mod_catid);
		wxSQLite3ResultSet set = db->ExecuteQuery(sqlstr);
		if(set.Eof())
		{
			tmpStr = "bltGetKeyWords函数错误：["+glHashStr[single_mod_modid] + "]模块("+single_mod_catname+")分类没有关键词！\n";
			zenglApi_SetErrThenStop(VM_ARG,(char *)tmpStr.c_str().AsChar());
			return;
		}
		wxArrayString keywordsArray;
		wxArrayString mustsArray;
		while(set.NextRow())
		{
			keywordsArray.Add(set.GetAsString(1));
			mustsArray.Add(set.GetAsString(3));
		}
		int keyarrayCount = keywordsArray.GetCount();
		int mustarrayCount = mustsArray.GetCount();
		for(int i = 1;i <= keyarrayCount; i++)
		{
			arg.type = ZL_EXP_FAT_STR;
			arg.val.str = (char *)keywordsArray.Item(i - 1).c_str().AsChar();
			zenglApi_SetMemBlock(VM_ARG,&keywords_memblock,i,&arg);
			arg.type = ZL_EXP_FAT_STR;
			arg.val.str = (char *)mustsArray.Item(i - 1).c_str().AsChar();
			zenglApi_SetMemBlock(VM_ARG,&musts_memblock,i,&arg);
		}
		zenglApi_SetRetVal(VM_ARG,ZL_EXP_FAT_INT,ZL_EXP_NULL,keyarrayCount,0);
	}
	
	/*bltCurlEncode模块函数，将字符串进行URL编码*/
	ZL_EXP_VOID global_bltCurlEncode(ZL_EXP_VOID * VM_ARG,ZL_EXP_INT argcount)
	{
		ZENGL_EXPORT_MOD_FUN_ARG arg = {ZL_EXP_FAT_NONE,{0}};
		if(argcount != 1)
			zenglApi_Exit(VM_ARG,"bltCurlEncode函数必须接受一个参数");
		zenglApi_GetFunArg(VM_ARG,1,&arg); //得到第一个参数
		if(arg.type != ZL_EXP_FAT_STR)
			zenglApi_Exit(VM_ARG,"bltCurlEncode函数的第一个参数必须是字符串");
		if(VM_ARG == CUR_CAIJI_VM)
		{
			char * retstr;
			curl=curl_easy_init();
			retstr = curl_easy_escape(curl,arg.val.str,0);
			curl_easy_cleanup(curl);
			curl = NULL;
			zenglApi_SetRetVal(VM_ARG,ZL_EXP_FAT_STR,retstr,0,0);
			curl_free(retstr);
		}
		else
		{
			MyUserExtraData * extraData = (MyUserExtraData *)zenglApi_GetExtraData(VM_ARG,"extraData");
			char * retstr;
			extraData->curl = curl_easy_init();
			retstr = curl_easy_escape(extraData->curl,arg.val.str,0);
			curl_easy_cleanup(extraData->curl);
			extraData->curl = NULL;
			zenglApi_SetRetVal(VM_ARG,ZL_EXP_FAT_STR,retstr,0,0);
			curl_free(retstr);
		}
	}
	
	/*bltCurlGetUrl模块函数，利用curl抓取网页url网站内容*/
	ZL_EXP_VOID global_bltCurlGetUrl(ZL_EXP_VOID * VM_ARG,ZL_EXP_INT argcount)
	{
		ZENGL_EXPORT_MOD_FUN_ARG arg = {ZL_EXP_FAT_NONE,{0}};
		char * url = ZL_EXP_NULL;
		int isForceConv = 0;
		if(argcount != 1 && argcount != 2)
			zenglApi_Exit(VM_ARG,"bltCurlGetUrl函数必须接受一个或两个参数");
		zenglApi_GetFunArg(VM_ARG,1,&arg); //得到第一个参数
		if(arg.type != ZL_EXP_FAT_STR)
			zenglApi_Exit(VM_ARG,"bltCurlGetUrl函数的第一个参数必须是字符串，表示要抓取的网址");
		url = arg.val.str;
		if(argcount == 2)
		{
			zenglApi_GetFunArg(VM_ARG,2,&arg); //得到第二个参数
			if(arg.type != ZL_EXP_FAT_INT)
				zenglApi_Exit(VM_ARG,"bltCurlGetUrl函数的第二个参数必须是整数，表示是否强制转换未识别的编码");
			isForceConv = arg.val.integer;
		}
		curl=curl_easy_init();
        global_GetUrlContent(url);
		curl_easy_cleanup(curl);
		curl = NULL;
		if(isForceConv == 0)
			zenglApi_SetRetVal(VM_ARG,ZL_EXP_FAT_STR,(char *)myglStr.c_str().AsChar(),0,0);
		else
		{
			wchar_t * ws = (wchar_t *)myglStr.wc_str();
			int buf_size = myGetLenWC2GBK(ws);
			if(buf_size == 0)
			{
				zenglApi_SetRetVal(VM_ARG,ZL_EXP_FAT_STR,"",0,0);
				return;
			}
			char * buf = (char *)zenglApi_AllocMem(VM_ARG,buf_size);
			if(myConvWC2GBK(ws,buf,buf_size))
				zenglApi_SetRetVal(VM_ARG,ZL_EXP_FAT_STR,buf,0,0);
			else
				zenglApi_SetRetVal(VM_ARG,ZL_EXP_FAT_STR,"",0,0);
			zenglApi_FreeMem(VM_ARG,buf);
		}
	}
	
	/*bltInfoBox模块函数，弹出信息对话框，显示信息*/
	ZL_EXP_VOID global_bltInfoBox(ZL_EXP_VOID * VM_ARG,ZL_EXP_INT argcount)
	{
		ZENGL_EXPORT_MOD_FUN_ARG arg = {ZL_EXP_FAT_NONE,{0}};
		if(argcount != 1)
			zenglApi_Exit(VM_ARG,"bltInfoBox函数必须接受一个参数");
		zenglApi_GetFunArg(VM_ARG,1,&arg); //得到第一个参数
		if(arg.type != ZL_EXP_FAT_STR)
			zenglApi_Exit(VM_ARG,"bltInfoBox函数的第一个参数必须是字符串，表示要显示的字符串信息");
		wxMessageBox(arg.val.str,"打印信息");
		zenglApi_SetRetVal(VM_ARG,ZL_EXP_FAT_INT,ZL_EXP_NULL,1,0);
	}

	/*bltRegexMatches模块函数，匹配正则表达式，类似php的preg_match_all*/
	ZL_EXP_VOID global_bltRegexMatches(ZL_EXP_VOID * VM_ARG,ZL_EXP_INT argcount)
	{
		ZENGL_EXPORT_MOD_FUN_ARG arg = {ZL_EXP_FAT_NONE,{0}};
		ZENGL_EXPORT_MEMBLOCK array_memblock = {0};
		ZL_EXP_CHAR * pattern,* content;
		ZL_EXP_INT index;
		ZL_EXP_INT unique_Flag;
		ZL_EXP_INT dotany_Flag;
		if(argcount != 6)
			zenglApi_Exit(VM_ARG,"bltRegexMatches函数必须接受6个参数");
		zenglApi_GetFunArg(VM_ARG,1,&arg); //得到第一个参数
		if(arg.type != ZL_EXP_FAT_STR)
			zenglApi_Exit(VM_ARG,"bltRegexMatches函数的第一个参数必须是字符串，表示要匹配的正则表达式");
		pattern = arg.val.str;
		zenglApi_GetFunArg(VM_ARG,2,&arg); //得到第二个参数
		if(arg.type != ZL_EXP_FAT_INT)
			zenglApi_Exit(VM_ARG,"bltRegexMatches函数的第二个参数必须是整数，表示匹配第几个元组");
		index = arg.val.integer;
		zenglApi_GetFunArg(VM_ARG,3,&arg); //得到第三个参数
		if(arg.type != ZL_EXP_FAT_STR)
			zenglApi_Exit(VM_ARG,"bltRegexMatches函数的第三个参数必须是字符串，表示要进行匹配的内容");
		content = arg.val.str;
		zenglApi_GetFunArg(VM_ARG,4,&arg); //得到第四个参数
		if(arg.type != ZL_EXP_FAT_MEMBLOCK)
			zenglApi_Exit(VM_ARG,"bltRegexMatches函数的第四个参数必须是数组或类对象(这两个都属于内存块类型)，用来存放匹配结果");
		array_memblock = arg.val.memblock;
		zenglApi_GetFunArg(VM_ARG,5,&arg); //得到第五个参数
		if(arg.type != ZL_EXP_FAT_INT)
			zenglApi_Exit(VM_ARG,"bltRegexMatches函数的第五个参数必须是整数，表示是否需要对结果进行唯一化处理，1表示要唯一化，0表示不要");
		unique_Flag = arg.val.integer;
		zenglApi_GetFunArg(VM_ARG,6,&arg); //得到第六个参数
		if(arg.type != ZL_EXP_FAT_INT)
			zenglApi_Exit(VM_ARG,"bltRegexMatches函数的第六个参数必须是整数，表示规则中的小数点是否匹配换行符，1表示匹配，0表示不匹配");
		dotany_Flag = arg.val.integer;
		wxRegEx ex;
		wxArrayString tmp_array,final_array;
		wxArrayString * tmp_array_ptr = NULL;
		wxString tmpstr;
		size_t start = 0;
		size_t len = 0;
		size_t prevstart = 0;
		int flags;
		int MatchCount = 0;
		if(dotany_Flag == 1)
			flags = wxRE_ADVANCED | wxRE_ICASE;
		else
			flags = wxRE_ADVANCED | wxRE_NEWLINE | wxRE_ICASE;
		wxString tmpContent = content;
		if(ex.Compile(pattern,flags))
		{
			MatchCount = ex.GetMatchCount();
			if(index >= MatchCount)
			{
				zenglApi_SetErrThenStop(VM_ARG,"bltRegexMatches函数的第二个元组参数无效，正则表达式中共有%d个元组，而你要访问第%d个元组，元组索引值必须小于总元组数"
									,MatchCount,index);
				return;
			}
			while(ex.Matches(tmpContent.Mid(prevstart)))
			{
				if(ex.GetMatch(&start,&len,index))
				{
					tmp_array.Add(tmpContent.Mid(prevstart + start, len));
				}
				prevstart += start + len;
			}
			if(unique_Flag == 1)
			{
				int count = tmp_array.GetCount();
				for(int i = 0; i < count; ++i)
				{
					tmpstr = tmp_array.Item(i);
					if(final_array.Index(tmpstr) == wxNOT_FOUND)
						final_array.Add(tmpstr);
				}
				tmp_array_ptr = &final_array;
			}
			else
				tmp_array_ptr = &tmp_array;
			int arrayCount = tmp_array_ptr->GetCount();
			for(int i = 1;i <= arrayCount; i++)
			{
				arg.type = ZL_EXP_FAT_STR;
				arg.val.str = (char *)tmp_array_ptr->Item(i - 1).c_str().AsChar();
				zenglApi_SetMemBlock(VM_ARG,&array_memblock,i,&arg);
			}
			zenglApi_SetRetVal(VM_ARG,ZL_EXP_FAT_INT,ZL_EXP_NULL,arrayCount,0);
			return;
		}
		zenglApi_SetRetVal(VM_ARG,ZL_EXP_FAT_INT,ZL_EXP_NULL,0,0);
	}

	/*bltArrayInsertString模块函数，向数组中添加一个字符串*/
	ZL_EXP_VOID global_bltArrayInsertString(ZL_EXP_VOID * VM_ARG,ZL_EXP_INT argcount)
	{
		ZENGL_EXPORT_MOD_FUN_ARG arg = {ZL_EXP_FAT_NONE,{0}};
		ZENGL_EXPORT_MEMBLOCK array_memblock = {0};
		ZL_EXP_CHAR * str;
		ZL_EXP_INT position;
		if(argcount != 3)
			zenglApi_Exit(VM_ARG,"bltArrayInsertString函数必须接受3个参数");
		zenglApi_GetFunArg(VM_ARG,1,&arg); //得到第一个参数
		if(arg.type != ZL_EXP_FAT_MEMBLOCK)
			zenglApi_Exit(VM_ARG,"bltArrayInsertString函数的第一个参数必须是数组或类对象(这两个都属于内存块类型)，表示要将数据插入到此数组中");
		array_memblock = arg.val.memblock;
		zenglApi_GetFunArg(VM_ARG,2,&arg); //得到第二个参数
		if(arg.type != ZL_EXP_FAT_STR)
			zenglApi_Exit(VM_ARG,"bltArrayInsertString函数的第二个参数必须是字符串，表示要插入的字符串");
		str = arg.val.str;
		zenglApi_GetFunArg(VM_ARG,3,&arg); //得到第三个参数
		if(arg.type != ZL_EXP_FAT_INT)
			zenglApi_Exit(VM_ARG,"bltArrayInsertString函数的第三个参数必须是整数，表示要插入的索引位置，0表示插入到开头，以此类推");
		position = arg.val.integer;
		ZL_EXP_INT i;
		ZL_EXP_INT array_mblk_count;
		wxArrayString tmpArray;
		zenglApi_GetMemBlockInfo(VM_ARG,&array_memblock,ZL_EXP_NULL,&array_mblk_count);
		for(i=1;i<=array_mblk_count;i++)
		{
			arg = zenglApi_GetMemBlock(VM_ARG,&array_memblock,i);
			if(arg.type != ZL_EXP_FAT_STR)
			{
				zenglApi_SetErrThenStop(VM_ARG,"bltArrayInsertString函数第一个数组参数里的所有数据都必须是字符串类型");
				return;
			}
			tmpArray.Add(arg.val.str);
		}
		tmpArray.Insert(str,position);
		ZL_EXP_INT count = tmpArray.GetCount();
		for(i=1;i<=count;i++)
		{
			arg.type = ZL_EXP_FAT_STR;
			arg.val.str = (char *)tmpArray.Item(i - 1).c_str().AsChar();
			zenglApi_SetMemBlock(VM_ARG,&array_memblock,i,&arg);
		}
		zenglApi_SetRetVal(VM_ARG,ZL_EXP_FAT_INT,ZL_EXP_NULL,count,0);
	}
	
	/*bltSqlTableExists模块函数，在sqlite数据库中查询某网站的某表是否存在*/
	ZL_EXP_VOID global_bltSqlTableExists(ZL_EXP_VOID * VM_ARG,ZL_EXP_INT argcount)
	{
		ZENGL_EXPORT_MOD_FUN_ARG arg = {ZL_EXP_FAT_NONE,{0}};
		ZL_EXP_CHAR * tablename;
		if(argcount != 1)
			zenglApi_Exit(VM_ARG,"bltSqlTableExists函数必须接受1个参数");
		zenglApi_GetFunArg(VM_ARG,1,&arg); //得到第一个参数
		if(arg.type != ZL_EXP_FAT_STR)
			zenglApi_Exit(VM_ARG,"bltSqlTableExists函数的第一个参数必须是字符串，表示要检查的表名");
		tablename = arg.val.str;
		wxSQLite3Database * db = glDB;
		wxString tmpStr;
		if(!wxFileExists(glSiteDBPath))
		{
			tmpStr = glCaijiWebName + "网站对应的本地数据库文件"+glSiteDBPath+"不存在！\n";
			zenglApi_SetErrThenStop(VM_ARG,(char *)tmpStr.c_str().AsChar());
			return;
		}
		if(!db->IsOpen())
			db->Open(glSiteDBPath);
		if(db->TableExists(tablename))
			zenglApi_SetRetVal(VM_ARG,ZL_EXP_FAT_INT,ZL_EXP_NULL,1,0);
		else
			zenglApi_SetRetVal(VM_ARG,ZL_EXP_FAT_INT,ZL_EXP_NULL,0,0);
	}

	/*bltSqlEscape模块函数，将字符串进行sqlite格式的转义*/
	ZL_EXP_VOID global_bltSqlEscape(ZL_EXP_VOID * VM_ARG,ZL_EXP_INT argcount)
	{
		ZENGL_EXPORT_MOD_FUN_ARG arg = {ZL_EXP_FAT_NONE,{0}};
		ZL_EXP_CHAR * content;
		if(argcount != 1)
			zenglApi_Exit(VM_ARG,"bltSqlEscape函数必须接受1个参数");
		zenglApi_GetFunArg(VM_ARG,1,&arg); //得到第一个参数
		if(arg.type != ZL_EXP_FAT_STR)
			zenglApi_Exit(VM_ARG,"bltSqlEscape函数的第一个参数必须是字符串，表示要进行转义的字符串");
		content = arg.val.str;
		wxString tmpStr = content;
		tmpStr.Replace("'","''");
		zenglApi_SetRetVal(VM_ARG,ZL_EXP_FAT_STR,(char *)tmpStr.c_str().AsChar(),0,0);
	}

	/*bltSqlQuery模块函数，在sqlite数据库中执行sql query查询*/
	ZL_EXP_VOID global_bltSqlQuery(ZL_EXP_VOID * VM_ARG,ZL_EXP_INT argcount)
	{
		ZENGL_EXPORT_MOD_FUN_ARG arg = {ZL_EXP_FAT_NONE,{0}};
		ZL_EXP_CHAR * sql;
		if(argcount != 1)
			zenglApi_Exit(VM_ARG,"bltSqlQuery函数必须接受1个参数");
		zenglApi_GetFunArg(VM_ARG,1,&arg); //得到第一个参数
		if(arg.type != ZL_EXP_FAT_STR)
			zenglApi_Exit(VM_ARG,"bltSqlQuery函数的第一个参数必须是字符串，表示要进行查询的sql语句");
		sql = arg.val.str;
		if(VM_ARG == CUR_CAIJI_VM)
		{
			wxSQLite3Database * db = glDB;
			wxString tmpStr;
			if(!wxFileExists(glSiteDBPath))
			{
				tmpStr = glCaijiWebName + "网站对应的本地数据库文件"+glSiteDBPath+"不存在！\n";
				zenglApi_SetErrThenStop(VM_ARG,(char *)tmpStr.c_str().AsChar());
				return;
			}
			if(!db->IsOpen())
				db->Open(glSiteDBPath);
			//if(gl_isQuerySql_forZenglRun)
			//{
				gl_sqlset_forZenglRun.~wxSQLite3ResultSet();
			//	gl_isQuerySql_forZenglRun = 0;
			//}
			try
			{
				gl_sqlset_forZenglRun = db->ExecuteQuery(wxString::Format("%s",sql));
				gl_isQuerySql_forZenglRun = 1;
			}
			catch(wxSQLite3Exception e)
			{
				gl_isQuerySql_forZenglRun = 0;
				tmpStr = e.GetMessage()+'\n';
				zenglApi_SetErrThenStop(VM_ARG,(char *)tmpStr.c_str().AsChar());
				return;
			}
		}
		else
		{
			MyUserExtraData * extraData = (MyUserExtraData *)zenglApi_GetExtraData(VM_ARG,"extraData");
			wxSQLite3Database * db = &(extraData->db);
			wxString tmpStr;
			if(!wxFileExists(glSiteDBPath))
			{
				tmpStr = glCaijiWebName + "网站对应的本地数据库文件"+glSiteDBPath+"不存在！\n";
				zenglApi_SetErrThenStop(VM_ARG,(char *)tmpStr.c_str().AsChar());
				return;
			}
			if(!db->IsOpen())
				db->Open(glSiteDBPath);
			//if(extraData->isQuerySql)
			//{
				extraData->sqlset.~wxSQLite3ResultSet();
			//	extraData->isQuerySql = 0;
			//}
			try
			{
				extraData->sqlset = db->ExecuteQuery(wxString::Format("%s",sql));
				extraData->isQuerySql = 1;
			}
			catch(wxSQLite3Exception e)
			{
				extraData->isQuerySql = 0;
				tmpStr = e.GetMessage()+'\n';
				zenglApi_SetErrThenStop(VM_ARG,(char *)tmpStr.c_str().AsChar());
				return;
			}
		}
		zenglApi_SetRetVal(VM_ARG,ZL_EXP_FAT_INT,ZL_EXP_NULL,1,0);
	}

	/*bltExit模块函数，直接退出zengl脚本*/
	ZL_EXP_VOID global_bltExit(ZL_EXP_VOID * VM_ARG,ZL_EXP_INT argcount)
	{
		ZENGL_EXPORT_MOD_FUN_ARG arg = {ZL_EXP_FAT_NONE,{0}};
		if(argcount > 0)
		{
			zenglApi_GetFunArg(VM_ARG,1,&arg); //得到第一个参数
			if(arg.type != ZL_EXP_FAT_STR)
				zenglApi_Exit(VM_ARG,"bltExit函数的第一个参数必须是字符串，表示退出脚本时需要显示的信息");
			zenglApi_Exit(VM_ARG,arg.val.str);
		}
		else
			zenglApi_Exit(VM_ARG,"");
	}
	
	/*bltSqlMoveToNext模块函数，在sqlite中对结果进行MoveToNext，将游标移到下一条记录*/
	ZL_EXP_VOID global_bltSqlMoveToNext(ZL_EXP_VOID * VM_ARG,ZL_EXP_INT argcount)
	{
		if(argcount != 0)
			zenglApi_Exit(VM_ARG,"bltSqlMoveToNext函数目前不接受任何参数");
		if(VM_ARG == CUR_CAIJI_VM)
		{
			if(!gl_isQuerySql_forZenglRun)
			{
				zenglApi_Exit(VM_ARG,"bltSqlMoveToNext函数错误：无效的结果集，请先执行bltSqlQuery生成结果集\n");
			}
			if(gl_sqlset_forZenglRun.NextRow() == true)
				zenglApi_SetRetVal(VM_ARG,ZL_EXP_FAT_INT,ZL_EXP_NULL,1,0);
			else
				zenglApi_SetRetVal(VM_ARG,ZL_EXP_FAT_INT,ZL_EXP_NULL,0,0);
		}
		else
		{
			MyUserExtraData * extraData = (MyUserExtraData *)zenglApi_GetExtraData(VM_ARG,"extraData");
			if(!extraData->isQuerySql)
				zenglApi_Exit(VM_ARG,"bltSqlMoveToNext函数错误：无效的结果集，请先执行bltSqlQuery生成结果集\n");
			if(extraData->sqlset.NextRow() == true)
				zenglApi_SetRetVal(VM_ARG,ZL_EXP_FAT_INT,ZL_EXP_NULL,1,0);
			else
				zenglApi_SetRetVal(VM_ARG,ZL_EXP_FAT_INT,ZL_EXP_NULL,0,0);
		}
	}

	/*bltSqlGetString模块函数，在sqlite数据库获取某列结果，并以字符串的形式返回结果*/
	ZL_EXP_VOID global_bltSqlGetString(ZL_EXP_VOID * VM_ARG,ZL_EXP_INT argcount)
	{
		ZENGL_EXPORT_MOD_FUN_ARG arg = {ZL_EXP_FAT_NONE,{0}};
		ZL_EXP_CHAR * columnName;
		if(argcount != 1)
			zenglApi_Exit(VM_ARG,"bltSqlGetString函数必须接受1个参数");
		zenglApi_GetFunArg(VM_ARG,1,&arg); //得到第一个参数
		if(arg.type != ZL_EXP_FAT_STR)
			zenglApi_Exit(VM_ARG,"bltSqlGetString函数的第一个参数必须是字符串，表示要查询的列名");
		columnName = arg.val.str;
		wxString retStr;
		if(VM_ARG == CUR_CAIJI_VM)
		{
			if(!gl_isQuerySql_forZenglRun)
			{
				zenglApi_SetErrThenStop(VM_ARG,"bltSqlGetString函数错误：无效的结果集，请先执行bltSqlQuery生成结果集\n");
				return;
			}
			try
			{
				retStr = gl_sqlset_forZenglRun.GetAsString(columnName);
			}
			catch(wxSQLite3Exception e)
			{
				retStr = e.GetMessage()+ "\n";
				zenglApi_SetErrThenStop(VM_ARG,(char *)retStr.c_str().AsChar());
				return;
			}
		}
		else
		{
			MyUserExtraData * extraData = (MyUserExtraData *)zenglApi_GetExtraData(VM_ARG,"extraData");
			if(!extraData->isQuerySql)
			{
				zenglApi_SetErrThenStop(VM_ARG,"bltSqlGetString函数错误：无效的结果集，请先执行bltSqlQuery生成结果集\n");
				return;
			}
			try
			{
				retStr = extraData->sqlset.GetAsString(columnName);
			}
			catch(wxSQLite3Exception e)
			{
				retStr = e.GetMessage()+ "\n";
				zenglApi_SetErrThenStop(VM_ARG,(char *)retStr.c_str().AsChar());
				return;
			}
		}
		zenglApi_SetRetVal(VM_ARG,ZL_EXP_FAT_STR,(char *)retStr.c_str().AsChar(),0,0);
	}

	/*bltSqlRelease模块函数，在sqlite中对结果集进行释放*/
	ZL_EXP_VOID global_bltSqlRelease(ZL_EXP_VOID * VM_ARG,ZL_EXP_INT argcount)
	{
		if(argcount != 0)
			zenglApi_Exit(VM_ARG,"bltSqlRelease函数目前不接受任何参数");
		if(VM_ARG == CUR_CAIJI_VM)
		{
			//if(gl_isQuerySql_forZenglRun)
			//{
				gl_sqlset_forZenglRun.~wxSQLite3ResultSet();
			//	gl_isQuerySql_forZenglRun = 0;
			//}
		}
		else
		{
			MyUserExtraData * extraData = (MyUserExtraData *)zenglApi_GetExtraData(VM_ARG,"extraData");
			//if(extraData->isQuerySql)
			//{
				extraData->sqlset.~wxSQLite3ResultSet();
			//	extraData->isQuerySql = 0;
			//}
		}
		zenglApi_SetRetVal(VM_ARG,ZL_EXP_FAT_INT,ZL_EXP_NULL,1,0); //设置脚本AX寄存器即返回值为1
	}

	/*bltSqlExec模块函数，在sqlite中执行创建，插入之类的操作*/
	ZL_EXP_VOID global_bltSqlExec(ZL_EXP_VOID * VM_ARG,ZL_EXP_INT argcount)
	{
		ZENGL_EXPORT_MOD_FUN_ARG arg = {ZL_EXP_FAT_NONE,{0}};
		ZL_EXP_CHAR * sql;
		if(argcount != 1)
			zenglApi_Exit(VM_ARG,"bltSqlExec函数必须接受1个参数");
		zenglApi_GetFunArg(VM_ARG,1,&arg); //得到第一个参数
		if(arg.type != ZL_EXP_FAT_STR)
			zenglApi_Exit(VM_ARG,"bltSqlExec函数的第一个参数必须是字符串，表示要执行的sql语句");
		sql = arg.val.str;
		wxSQLite3Database * db = glDB;
		wxString tmpStr;
		if(!wxFileExists(glSiteDBPath))
		{
			myglStr = glCaijiWebName + "网站对应的本地数据库文件"+glSiteDBPath+"不存在！\n";
			zenglApi_SetErrThenStop(VM_ARG,(char *)myglStr.c_str().AsChar());
			return;
		}
		if(!db->IsOpen())
			db->Open(glSiteDBPath);
		try
		{
			db->ExecuteUpdate(wxString::Format("%s",sql));
		}
		catch(wxSQLite3Exception e)
		{
			tmpStr = e.GetMessage()+'\n';
			zenglApi_SetErrThenStop(VM_ARG,(char *)tmpStr.c_str().AsChar());
			return;
		}
		zenglApi_SetRetVal(VM_ARG,ZL_EXP_FAT_INT,ZL_EXP_NULL,1,0); //设置脚本AX寄存器即返回值为1
	}

	/*bltRegexMatchFirst模块函数，正则表达式匹配，类似php的preg_match*/
	ZL_EXP_VOID global_bltRegexMatchFirst(ZL_EXP_VOID * VM_ARG,ZL_EXP_INT argcount)
	{
		ZENGL_EXPORT_MOD_FUN_ARG arg = {ZL_EXP_FAT_NONE,{0}};
		ZL_EXP_CHAR * pattern,* content;
		ZL_EXP_INT index;
		ZL_EXP_INT dotany_Flag;
		if(argcount != 5)
			zenglApi_Exit(VM_ARG,"bltRegexMatchFirst函数必须接受5个参数");
		zenglApi_GetFunArg(VM_ARG,1,&arg); //得到第一个参数
		if(arg.type != ZL_EXP_FAT_STR)
			zenglApi_Exit(VM_ARG,"bltRegexMatchFirst函数的第一个参数必须是字符串，表示正则表达式的规则");
		pattern = arg.val.str;
		zenglApi_GetFunArg(VM_ARG,2,&arg); //得到第二个参数
		if(arg.type != ZL_EXP_FAT_INT)
			zenglApi_Exit(VM_ARG,"bltRegexMatchFirst函数的第二个参数必须是整数，表示匹配第几个元组");
		index = arg.val.integer;
		zenglApi_GetFunArg(VM_ARG,3,&arg); //得到第三个参数
		if(arg.type != ZL_EXP_FAT_STR)
			zenglApi_Exit(VM_ARG,"bltRegexMatchFirst函数的第三个参数必须是字符串，表示要进行匹配的内容");
		content = arg.val.str;
		zenglApi_GetFunArgInfo(VM_ARG,4,&arg); //得到第四个参数
		if(arg.type != ZL_EXP_FAT_ADDR && 
			arg.type != ZL_EXP_FAT_ADDR_LOC)
			zenglApi_Exit(VM_ARG,"bltRegexMatchFirst函数的第四个参数必须是全局变量或局部变量的引用类型，用来存放匹配结果");
		zenglApi_GetFunArg(VM_ARG,5,&arg); //得到第五个参数
		if(arg.type != ZL_EXP_FAT_INT)
			zenglApi_Exit(VM_ARG,"bltRegexMatchFirst函数的第五个参数必须是整数，表示规则中的小数点是否匹配换行符");
		dotany_Flag = arg.val.integer;
		wxRegEx ex;
		size_t start = 0;
		size_t len = 0;
		size_t prevstart = 0;
		int flags;
		int MatchCount = 0;
		if(dotany_Flag == 1)
			flags = wxRE_ADVANCED | wxRE_ICASE;
		else
			flags = wxRE_ADVANCED | wxRE_NEWLINE | wxRE_ICASE;
		wxString tmpContent = content;
		if(ex.Compile(pattern,flags))
		{
			MatchCount = ex.GetMatchCount();
			if(index >= MatchCount)
			{
				zenglApi_SetErrThenStop(VM_ARG,"bltRegexMatchFirst函数的第二个元组参数无效，正则表达式中共有%d个元组，而你要访问第%d个元组，元组索引值必须小于总元组数"
									,MatchCount,index);
				return;
			}
			if(ex.Matches(tmpContent.Mid(prevstart)))
			{
				if(ex.GetMatch(&start,&len,index))
				{
					myglStr = tmpContent.Mid(prevstart + start, len);
					arg.type = ZL_EXP_FAT_STR;
					arg.val.str = (char *)myglStr.c_str().AsChar();
					zenglApi_SetFunArg(VM_ARG,4,&arg);
					zenglApi_SetRetVal(VM_ARG,ZL_EXP_FAT_INT,ZL_EXP_NULL,start,0); //设置返回值为找到的匹配字符串的起始字符的索引位置
					return ;
				}
			}
		}
		zenglApi_SetRetVal(VM_ARG,ZL_EXP_FAT_INT,ZL_EXP_NULL,-1,0);
	}
	
	/*bltRegexReplace模块函数，正则表达式替换，类似PHP的preg_replace*/
	ZL_EXP_VOID global_bltRegexReplace(ZL_EXP_VOID * VM_ARG,ZL_EXP_INT argcount)
	{
		ZENGL_EXPORT_MOD_FUN_ARG arg = {ZL_EXP_FAT_NONE,{0}};
		ZL_EXP_CHAR * pattern = NULL;
		ZL_EXP_CHAR * replace = NULL;
		ZL_EXP_CHAR * content = NULL;
		ZL_EXP_INT dotany_Flag;
		if(argcount != 4)
			zenglApi_Exit(VM_ARG,"bltRegexReplace函数必须接受4个参数");
		zenglApi_GetFunArg(VM_ARG,1,&arg); //得到第一个参数
		if(arg.type != ZL_EXP_FAT_STR)
			zenglApi_Exit(VM_ARG,"bltRegexReplace函数的第一个参数必须是字符串，表示要匹配的正则表达式的规则");
		pattern = arg.val.str;
		zenglApi_GetFunArg(VM_ARG,2,&arg); //得到第二个参数
		if(arg.type != ZL_EXP_FAT_STR)
			zenglApi_Exit(VM_ARG,"bltRegexReplace函数的第二个参数必须是字符串，表示要替换的字符串");
		replace = arg.val.str;
		zenglApi_GetFunArg(VM_ARG,3,&arg); //得到第三个参数
		if(arg.type != ZL_EXP_FAT_STR)
			zenglApi_Exit(VM_ARG,"bltRegexReplace函数的第三个参数必须是字符串，表示要进行匹配的内容");
		content = arg.val.str;
		zenglApi_GetFunArg(VM_ARG,4,&arg); //得到第四个参数
		if(arg.type != ZL_EXP_FAT_INT)
			zenglApi_Exit(VM_ARG,"bltRegexReplace函数的第四个参数必须是整数，表示规则中的小数点是否匹配换行符");
		dotany_Flag = arg.val.integer;
		wxRegEx ex;
		wxString retStr;
		int flags;
		if(dotany_Flag == 1)
			flags = wxRE_ADVANCED | wxRE_ICASE;
		else
			flags = wxRE_ADVANCED | wxRE_NEWLINE | wxRE_ICASE;
		retStr = content;
		if(ex.Compile(pattern,flags))
		{
			ex.Replace(&retStr,replace);
			zenglApi_SetRetVal(VM_ARG,ZL_EXP_FAT_STR,(char *)retStr.c_str().AsChar(),0,0); //设置脚本返回值为替换后的字符串
			return;
		}
		zenglApi_SetRetVal(VM_ARG,ZL_EXP_FAT_STR,content,0,0);
	}

	/*bltTrim模块函数，去除左右空格，类似PHP的trim*/
	ZL_EXP_VOID global_bltTrim(ZL_EXP_VOID * VM_ARG,ZL_EXP_INT argcount)
	{
		ZENGL_EXPORT_MOD_FUN_ARG arg = {ZL_EXP_FAT_NONE,{0}};
		ZL_EXP_CHAR * content = NULL;
		if(argcount != 1)
			zenglApi_Exit(VM_ARG,"bltTrim函数必须接受1个参数");
		zenglApi_GetFunArg(VM_ARG,1,&arg); //得到第一个参数
		if(arg.type != ZL_EXP_FAT_STR)
			zenglApi_Exit(VM_ARG,"bltTrim函数的第一个参数必须是字符串，表示要进行处理的内容");
		content = arg.val.str;
		wxString retStr;
		retStr = TrimBoth(content);
		zenglApi_SetRetVal(VM_ARG,ZL_EXP_FAT_STR,(char *)retStr.c_str().AsChar(),0,0);
	}

	/*bltCheckTitleMust模块函数，测试标题中是否包含必须词*/
	ZL_EXP_VOID global_bltCheckTitleMust(ZL_EXP_VOID * VM_ARG,ZL_EXP_INT argcount)
	{
		ZENGL_EXPORT_MOD_FUN_ARG arg = {ZL_EXP_FAT_NONE,{0}};
		ZL_EXP_CHAR * title = NULL;
		ZL_EXP_CHAR * must = NULL;
		if(argcount != 2)
			zenglApi_Exit(VM_ARG,"bltCheckTitleMust函数必须接受2个参数");
		zenglApi_GetFunArg(VM_ARG,1,&arg); //得到第一个参数
		if(arg.type != ZL_EXP_FAT_STR)
			zenglApi_Exit(VM_ARG,"bltCheckTitleMust函数的第一个参数必须是字符串，表示标题");
		title = arg.val.str;
		zenglApi_GetFunArg(VM_ARG,2,&arg); //得到第二个参数
		if(arg.type != ZL_EXP_FAT_STR)
			zenglApi_Exit(VM_ARG,"bltCheckTitleMust函数的第二个参数必须是字符串，表示必须包含的词");
		must = arg.val.str;
		if(CheckTitleMust(title , must) == true)
			zenglApi_SetRetVal(VM_ARG,ZL_EXP_FAT_INT,ZL_EXP_NULL,1,0);
		else
			zenglApi_SetRetVal(VM_ARG,ZL_EXP_FAT_INT,ZL_EXP_NULL,0,0);
	}
	
	/*bltStrReplace模块函数，字符串替换，类似PHP的str_replace*/
	ZL_EXP_VOID global_bltStrReplace(ZL_EXP_VOID * VM_ARG,ZL_EXP_INT argcount)
	{
		ZENGL_EXPORT_MOD_FUN_ARG arg = {ZL_EXP_FAT_NONE,{0}};
		ZL_EXP_CHAR * pattern = NULL;
		ZL_EXP_CHAR * replace = NULL;
		ZL_EXP_CHAR * content = NULL;
		if(argcount != 3)
			zenglApi_Exit(VM_ARG,"bltStrReplace函数必须接受3个参数");
		zenglApi_GetFunArg(VM_ARG,1,&arg); //得到第一个参数
		if(arg.type != ZL_EXP_FAT_STR)
			zenglApi_Exit(VM_ARG,"bltStrReplace函数的第一个参数必须是字符串，表示要替换的部分");
		pattern = arg.val.str;
		zenglApi_GetFunArg(VM_ARG,2,&arg); //得到第二个参数
		if(arg.type != ZL_EXP_FAT_STR)
			zenglApi_Exit(VM_ARG,"bltStrReplace函数的第二个参数必须是字符串，表示要替换值");
		replace = arg.val.str;
		zenglApi_GetFunArg(VM_ARG,3,&arg); //得到第三个参数
		if(arg.type != ZL_EXP_FAT_STR)
			zenglApi_Exit(VM_ARG,"bltStrReplace函数的第三个参数必须是字符串，表示要处理的字符串");
		content = arg.val.str;
		wxString retStr;
		retStr = content;
		retStr.Replace(pattern,replace);
		zenglApi_SetRetVal(VM_ARG,ZL_EXP_FAT_STR,(char *)retStr.c_str().AsChar(),0,0);
	}

	/*bltCheckAutoPass模块函数，判断是否需要自动过滤不合要求的链接*/
	ZL_EXP_VOID global_bltCheckAutoPass(ZL_EXP_VOID * VM_ARG,ZL_EXP_INT argcount)
	{
		if(argcount != 0)
			zenglApi_Exit(VM_ARG,"bltCheckAutoPass函数目前不接受任何参数");
		if(glmainFrame->m_checkAutoPass->IsChecked())
		{
			zenglApi_SetRetVal(VM_ARG,ZL_EXP_FAT_INT,ZL_EXP_NULL,1,0);
		}
		else
			zenglApi_SetRetVal(VM_ARG,ZL_EXP_FAT_INT,ZL_EXP_NULL,0,0);
	}
	
	/*bltPostData模块函数，上传数据到网站*/
	ZL_EXP_VOID global_bltPostData(ZL_EXP_VOID * VM_ARG,ZL_EXP_INT argcount)
	{
		ZENGL_EXPORT_MOD_FUN_ARG arg = {ZL_EXP_FAT_NONE,{0}};
		ZL_EXP_CHAR * url = NULL;
		ZENGL_EXPORT_MEMBLOCK memblock;
		ZL_EXP_CHAR * cookie = NULL;
		if(argcount != 3 && argcount != 4)
			zenglApi_Exit(VM_ARG,"bltPostData函数必须接受3或4个参数");
		zenglApi_GetFunArg(VM_ARG,1,&arg); //得到第一个参数
		if(arg.type != ZL_EXP_FAT_STR)
			zenglApi_Exit(VM_ARG,"bltPostData函数的第一个参数必须是字符串，表示要上传的目标网站网址");
		url = arg.val.str;
		zenglApi_GetFunArg(VM_ARG,2,&arg); //得到第二个参数
		if(arg.type != ZL_EXP_FAT_MEMBLOCK)
			zenglApi_Exit(VM_ARG,"bltPostData函数的第二个参数必须是数组，类对象之类的内存块类型，表示要上传的数据的名值对数组");
		memblock = arg.val.memblock;
		zenglApi_GetFunArgInfo(VM_ARG,3,&arg); //得到第三个参数
		if(arg.type != ZL_EXP_FAT_ADDR &&
			arg.type != ZL_EXP_FAT_ADDR_LOC)
			zenglApi_Exit(VM_ARG,"bltPostData函数的第三个参数必须是全局变量或局部变量的引用类型，用来存放post请求返回的数据\n");
		if(argcount == 4)
		{
			zenglApi_GetFunArg(VM_ARG,4,&arg); //得到第四个参数
			if(arg.type != ZL_EXP_FAT_STR)
				zenglApi_Exit(VM_ARG,"bltPostData函数的第四个参数必须是字符串，表示cookie信息");
			cookie = arg.val.str;
		}
		struct curl_httppost *post=NULL;
		struct curl_httppost *last=NULL;
		char * key;
		char * value;
		wxMyLogEvent eventforlog( wxEVT_MY_LOG_EVENT,wxID_ANY);
		int i,mblk_count;
		wxArrayString tmpArray;
		zenglApi_GetMemBlockInfo(VM_ARG,&memblock,ZL_EXP_NULL,&mblk_count);
		for(i=1;i<=mblk_count;i++)
		{
			arg = zenglApi_GetMemBlock(VM_ARG,&memblock,i);
			if(arg.type != ZL_EXP_FAT_STR)
			{
				zenglApi_SetErrThenStop(VM_ARG,"bltPostData函数的第二个参数对应的名值对数组中的所有元素都必须是字符串类型\n");
				return;
			}
			tmpArray.Add(arg.val.str);
		}
		int count = tmpArray.GetCount();
		curl = curl_easy_init();
		curl_easy_setopt(curl, CURLOPT_URL, url);
		curl_easy_setopt(curl, CURLOPT_USERAGENT ,"Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.2; .NET CLR 1.1.4322)");
		curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION ,1);
		if(count<=0)
		{
			zenglApi_SetErrThenStop(VM_ARG,"bltPostData函数运行时错误：要上传的数据对应的数组没有元素！\n");
			return;
		}
		else if(count%2 != 0)
		{
			zenglApi_SetErrThenStop(VM_ARG,"bltPostData函数运行时错误：要上传的数据对应的数组个数不是2的倍数，无法构成名值对的形式！\n");
			return;
		}
		int findpos;
		wxString tmpwxValue;
		for(i=0;i<count;i++)
		{
			key = (char *)tmpArray.Item(i).c_str().AsChar();
			value = (char *)tmpArray.Item(++i).c_str().AsChar();
			//value = (char *)tmpArray.Item(++i).ToUTF8().data();
			tmpwxValue = value;
			if((findpos = tmpwxValue.Find("@"))!=wxNOT_FOUND && findpos == 0)
			{
				tmpwxValue.Replace("@","");
				curl_formadd(&post,&last,CURLFORM_COPYNAME,key,CURLFORM_FILE,(char *)tmpwxValue.c_str().AsChar(), CURLFORM_END);
			}
			else
				curl_formadd(&post,&last,CURLFORM_COPYNAME,key,CURLFORM_COPYCONTENTS,value, CURLFORM_END);
		}
		curl_easy_setopt(curl, CURLOPT_HTTPPOST, post);
		curl_easy_setopt(curl, CURLOPT_HEADER, 0); 
		long timeout;
		glmainFrame->m_textForTimeOut->GetValue().ToLong(&timeout);
		curl_easy_setopt(curl, CURLOPT_TIMEOUT, (int)timeout);//设置超时时间
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, mywxcurl_string_write);
		if(cookie != NULL)
		{
			curl_easy_setopt(curl, CURLOPT_COOKIE, cookie); //设置cookie调试信息
		}
		CURLcode errcode = curl_easy_perform(curl);
		wxString tmpStr;
		if(errcode != 0)
		{
			eventforlog.SetEventMsg(wxString::Format("error:%s...",curl_easy_strerror(errcode)),MY_RICHTEXT_RED_STYLE);
			wxQueueEvent(glmainFrame->GetEventHandler(),eventforlog.Clone());
			tmpStr = "";
			curl_easy_cleanup(curl);
			curl_formfree(post);
			curl = NULL;
			char_myglTotalnum = 0;
			arg.type = ZL_EXP_FAT_STR;
			arg.val.str = (char *)tmpStr.c_str().AsChar();
			zenglApi_SetFunArg(VM_ARG,3,&arg);
			zenglApi_SetRetVal(VM_ARG,ZL_EXP_FAT_INT,ZL_EXP_NULL,0,0);
			return;
		}
		if(char_myglStr==NULL)
		{
			charlength = 0;
			eventforlog.SetEventMsg( _("服务器无响应..."),MY_RICHTEXT_RED_STYLE);
			wxQueueEvent(glmainFrame->GetEventHandler(),eventforlog.Clone());
			tmpStr = "";
			curl_easy_cleanup(curl);
			curl_formfree(post);
			curl = NULL;
			char_myglTotalnum = 0;
			arg.type = ZL_EXP_FAT_STR;
			arg.val.str = (char *)tmpStr.c_str().AsChar();
			zenglApi_SetFunArg(VM_ARG,3,&arg);
			zenglApi_SetRetVal(VM_ARG,ZL_EXP_FAT_INT,ZL_EXP_NULL,0,0);
			return;
		}
		charlength = strlen(char_myglStr);
		if((myglStr = wxString(char_myglStr)) == "")
			myglStr = wxString::FromUTF8(char_myglStr);
		
		curl_easy_cleanup(curl);
		curl_formfree(post);
		curl = NULL;
		/*if((fp=fopen("test","w"))==NULL)
		{
			curl_easy_cleanup(curl);
			return NULL;
		}
		fwrite( char_myglStr, 1 , char_myglTotalnum ,fp);
		fclose(fp);*/
		free(char_myglStr);
		char_myglStr = NULL;
		char_myglTotalnum = 0;
		arg.type = ZL_EXP_FAT_STR;
		arg.val.str = (char *)myglStr.c_str().AsChar();
		zenglApi_SetFunArg(VM_ARG,3,&arg);
		zenglApi_SetRetVal(VM_ARG,ZL_EXP_FAT_INT,ZL_EXP_NULL,1,0);
	}

	/*	bltIntToStr模块函数：
		将整数转为字符串的形式，例如bltIntToStr(25,5,'0')那么得到的结果就是字符串'00025'
		因为第二个参数5是指总宽度，第一个参数25的宽度只有2，小于5，
		所以左边会以第三个参数'0'补齐。
	*/
	ZL_EXP_VOID global_bltIntToStr(ZL_EXP_VOID * VM_ARG,ZL_EXP_INT argcount)
	{
		ZENGL_EXPORT_MOD_FUN_ARG arg = {ZL_EXP_FAT_NONE,{0}};
		char buf[100],dest[100];
		int num,len,i;
		if(argcount != 3)
			zenglApi_Exit(VM_ARG,"bltIntToStr函数必须接受3个参数");
		zenglApi_GetFunArg(VM_ARG,1,&arg); //得到第一个参数
		if(arg.type != ZL_EXP_FAT_INT)
			zenglApi_Exit(VM_ARG,"bltIntToStr函数的第一个参数必须是整数，表示要进行转换的整数值");
		num = arg.val.integer;
		sprintf(buf,"%d",num);
		len = strlen(buf);
		zenglApi_GetFunArg(VM_ARG,2,&arg); //得到第二个参数
		if(arg.type != ZL_EXP_FAT_INT)
			zenglApi_Exit(VM_ARG,"bltIntToStr函数的第二个参数必须是整数，表示总宽度，当整数的宽度不足时，在左侧按第三个参数进行补充");
		num = arg.val.integer;
		zenglApi_GetFunArg(VM_ARG,3,&arg); //得到第三个参数
		if(arg.type != ZL_EXP_FAT_STR)
			zenglApi_Exit(VM_ARG,"bltIntToStr函数的第三个参数必须是字符串类型，表示要进行补充的元素");
		if(len < num)  //当第一个参数的字符串长度不足时，则用第三个参数来补齐。
		{
			for(i=0;i<num-len;i++)
				dest[i] = arg.val.str[0];
			strncpy(dest+num-len,buf,len);
			dest[num] = '\0';
		}
		if(len < num)
			zenglApi_SetRetVal(VM_ARG,ZL_EXP_FAT_STR,(char *)dest,0,0);
		else 
			zenglApi_SetRetVal(VM_ARG,ZL_EXP_FAT_STR,(char *)buf,0,0);
	}

	/*bltAddProgress模块函数，以数字为参数增加进度条*/
	ZL_EXP_VOID global_bltAddProgress(ZL_EXP_VOID * VM_ARG,ZL_EXP_INT argcount)
	{
		ZENGL_EXPORT_MOD_FUN_ARG arg = {ZL_EXP_FAT_NONE,{0}};
		int num;
		if(argcount != 1)
			zenglApi_Exit(VM_ARG,"bltAddProgress函数必须接受1个参数");
		zenglApi_GetFunArg(VM_ARG,1,&arg); //得到第一个参数
		if(arg.type != ZL_EXP_FAT_INT)
			zenglApi_Exit(VM_ARG,"bltAddProgress函数的第一个参数必须是整数，表示要增加的进度值");
		num = arg.val.integer;
		global_mod_processPos += num;
		single_mod_processPos += num;
		wxMyProgressEvent eventForProgress(wxEVT_MY_PROGRESS_EVENT,ID_MY_WINDOW,single_mod_processPos,single_mod_processNum,global_mod_processPos,global_mod_processNum);
		wxQueueEvent(glmainFrame->GetEventHandler(),eventForProgress.Clone());
		zenglApi_SetRetVal(VM_ARG,ZL_EXP_FAT_INT,ZL_EXP_NULL,1,0);
	}

	/*bltCheckUserPause模块函数，判断当前用户是否需要在采集后暂停*/
	ZL_EXP_VOID global_bltCheckUserPause(ZL_EXP_VOID * VM_ARG,ZL_EXP_INT argcount)
	{
		if(argcount != 0)
			zenglApi_Exit(VM_ARG,"bltCheckUserPause函数目前不接受任何参数");
		if(glmainFrame->m_checkShowBall->IsChecked())
		{
			wxCommandEvent eventForPause(wxEVT_MY_PAUSE_THREAD,ID_MY_WINDOW);
			wxQueueEvent(glmainFrame->GetEventHandler(),eventForPause.Clone());
			wxThread::Yield();
			wxThread::Sleep(500);
			zenglApi_SetRetVal(VM_ARG,ZL_EXP_FAT_INT,ZL_EXP_NULL,1,0);
		}
		else
			zenglApi_SetRetVal(VM_ARG,ZL_EXP_FAT_INT,ZL_EXP_NULL,0,0);
	}

	/*bltGetMaxUploadIMG模块函数，获取内容部分最多采集多少张图片*/
	ZL_EXP_VOID global_bltGetMaxUploadIMG(ZL_EXP_VOID * VM_ARG,ZL_EXP_INT argcount)
	{
		if(argcount != 0)
			zenglApi_Exit(VM_ARG,"bltGetMaxUploadIMG函数目前不接受任何参数");
		wxString retStr;
		retStr = glmainFrame->m_textForMaxUploadIMG->GetValue();
		zenglApi_SetRetVal(VM_ARG,ZL_EXP_FAT_STR,(char *)retStr.c_str().AsChar(),0,0);
	}

	/*bltGetTimeNow模块函数，获取系统当前时间*/
	ZL_EXP_VOID global_bltGetTimeNow(ZL_EXP_VOID * VM_ARG,ZL_EXP_INT argcount)
	{
		ZENGL_EXPORT_MOD_FUN_ARG arg = {ZL_EXP_FAT_NONE,{0}};
		struct tm * time_info;
		time_t current_time;
		char timeString[20];
		char * format;
		if(argcount != 1)
			zenglApi_Exit(VM_ARG,"bltGetTimeNow函数必须接受1个参数");
		zenglApi_GetFunArg(VM_ARG,1,&arg); //得到第一个参数
		if(arg.type != ZL_EXP_FAT_STR)
			zenglApi_Exit(VM_ARG,"bltGetTimeNow函数的第一个参数必须是字符串，表示时间的格式");
		format = arg.val.str;
		time(&current_time);
		time_info = localtime(&current_time);
		strftime(timeString, sizeof(timeString), format, time_info);
		zenglApi_SetRetVal(VM_ARG,ZL_EXP_FAT_STR,(char *)timeString,0,0);
	}

	/*bltAddProgressEx模块函数，上传时用于设置进度条*/
	ZL_EXP_VOID global_bltAddProgressEx(ZL_EXP_VOID * VM_ARG,ZL_EXP_INT argcount)
	{
		ZENGL_EXPORT_MOD_FUN_ARG arg = {ZL_EXP_FAT_NONE,{0}};
		int RowCount;
		int totalRowCount;
		if(argcount != 2)
			zenglApi_Exit(VM_ARG,"bltAddProgressEx函数必须接受2个参数");
		zenglApi_GetFunArg(VM_ARG,1,&arg); //得到第一个参数
		if(arg.type != ZL_EXP_FAT_INT)
			zenglApi_Exit(VM_ARG,"bltAddProgressEx函数的第一个参数必须是整数，表示上传的记录数");
		RowCount = arg.val.integer;
		zenglApi_GetFunArg(VM_ARG,2,&arg); //得到第二个参数
		if(arg.type != ZL_EXP_FAT_INT)
			zenglApi_Exit(VM_ARG,"bltAddProgressEx函数的第二个参数必须是整数，表示上传的总记录数");
		totalRowCount = arg.val.integer;
		float growSteps = ((float)RowCount/(float)totalRowCount) * (float)single_mod_processNum;
		int old_single_mod_processPos = single_mod_processPos;
		single_mod_processPos = single_mod_processNum + (int)growSteps;
		global_mod_processPos += single_mod_processPos - old_single_mod_processPos;
		wxMyProgressEvent eventForProgress(wxEVT_MY_PROGRESS_EVENT,ID_MY_WINDOW,single_mod_processPos,single_mod_processNum,global_mod_processPos,global_mod_processNum);
		wxQueueEvent(glmainFrame->GetEventHandler(),eventForProgress.Clone());
		zenglApi_SetRetVal(VM_ARG,ZL_EXP_FAT_INT,ZL_EXP_NULL,1,0);
	}

	/*bltTellSysCaijiStatus模块函数，告诉采集器当前分类的采集量和发布量*/
	ZL_EXP_VOID global_bltTellSysCaijiStatus(ZL_EXP_VOID * VM_ARG,ZL_EXP_INT argcount)
	{
		ZENGL_EXPORT_MOD_FUN_ARG arg = {ZL_EXP_FAT_NONE,{0}};
		int caijinum;
		int postnum;
		if(argcount != 2)
			zenglApi_Exit(VM_ARG,"bltTellSysCaijiStatus函数必须接受2个参数");
		zenglApi_GetFunArg(VM_ARG,1,&arg); //得到第一个参数
		if(arg.type != ZL_EXP_FAT_INT)
			zenglApi_Exit(VM_ARG,"bltTellSysCaijiStatus函数的第一个参数必须是整数，表示已经采集的数量");
		caijinum = arg.val.integer;
		zenglApi_GetFunArg(VM_ARG,2,&arg); //得到第二个参数
		if(arg.type != ZL_EXP_FAT_INT)
			zenglApi_Exit(VM_ARG,"bltTellSysCaijiStatus函数的第二个参数必须是整数，表示成功上传的数量");
		postnum = arg.val.integer;
		single_mod_caijinum = caijinum;
		single_mod_curpostnum = postnum;
		global_mod_totalcaijinum += single_mod_caijinum;
		global_mod_totalpostnum += single_mod_curpostnum;
		zenglApi_SetRetVal(VM_ARG,ZL_EXP_FAT_INT,ZL_EXP_NULL,1,0);
	}

	/*bltConvToInt模块函数，将参数转为整数形式*/
	ZL_EXP_VOID global_bltConvToInt(ZL_EXP_VOID * VM_ARG,ZL_EXP_INT argcount)
	{
		ZENGL_EXPORT_MOD_FUN_ARG arg = {ZL_EXP_FAT_NONE,{0}};
		int ret;
		if(argcount != 1)
			zenglApi_Exit(VM_ARG,"bltConvToInt函数必须接受1个参数");
		zenglApi_GetFunArg(VM_ARG,1,&arg); //得到第一个参数
		switch(arg.type)
		{
		case ZL_EXP_FAT_FLOAT:
			ret = (int)arg.val.floatnum;
			break;
		case ZL_EXP_FAT_STR:
			ret = atoi(arg.val.str);
			break;
		case ZL_EXP_FAT_INT:
			ret = arg.val.integer;
			break;
		default:
			zenglApi_Exit(VM_ARG,"bltConvToInt函数参数只能是整数，浮点数或字符串类型");
			break;
		}
		zenglApi_SetRetVal(VM_ARG,ZL_EXP_FAT_INT,ZL_EXP_NULL,ret,0);
	}

	/*bltGetArgString模块函数，获取用户传递过来的额外数据，并以字符串指针的形式返回，所以请确保该额外数据是字符串类型*/
	ZL_EXP_VOID global_bltGetArgString(ZL_EXP_VOID * VM_ARG,ZL_EXP_INT argcount)
	{
		ZENGL_EXPORT_MOD_FUN_ARG arg = {ZL_EXP_FAT_NONE,{0}};
		char * extraName;
		if(argcount != 1)
			zenglApi_Exit(VM_ARG,"bltGetArgString函数必须接受1个参数");
		zenglApi_GetFunArg(VM_ARG,1,&arg); //得到第一个参数
		if(arg.type != ZL_EXP_FAT_STR)
			zenglApi_Exit(VM_ARG,"bltGetArgString函数的第一个参数必须是字符串，表示用户程序中传递给脚本的额外数据");
		extraName = arg.val.str;
		zenglApi_SetRetVal(VM_ARG,ZL_EXP_FAT_STR,(ZL_EXP_CHAR *)zenglApi_GetExtraData(VM_ARG,extraName),0,0);
	}

	/*bltStrFind模块函数，查找字符串*/
	ZL_EXP_VOID global_bltStrFind(ZL_EXP_VOID * VM_ARG,ZL_EXP_INT argcount)
	{
		ZENGL_EXPORT_MOD_FUN_ARG arg = {ZL_EXP_FAT_NONE,{0}};
		char * content;
		char * find;
		if(argcount != 2)
			zenglApi_Exit(VM_ARG,"bltStrFind函数必须接受2个参数");
		zenglApi_GetFunArg(VM_ARG,1,&arg); //得到第一个参数
		if(arg.type != ZL_EXP_FAT_STR)
			zenglApi_Exit(VM_ARG,"bltStrFind函数的第一个参数必须是字符串，表示要在该内容中进行查找");
		content = arg.val.str;
		zenglApi_GetFunArg(VM_ARG,2,&arg); //得到第二个参数
		if(arg.type != ZL_EXP_FAT_STR)
			zenglApi_Exit(VM_ARG,"bltStrFind函数的第二个参数必须是字符串，表示要查找的内容");
		find = arg.val.str;
		wxString string = content;
		int findpos;
		if((findpos = string.Find(find))!=wxNOT_FOUND)
			zenglApi_SetRetVal(VM_ARG,ZL_EXP_FAT_INT,ZL_EXP_NULL,findpos,0);
		else
			zenglApi_SetRetVal(VM_ARG,ZL_EXP_FAT_INT,ZL_EXP_NULL,-1,0);
	}

	/*bltLaunchDefaultBrowser模块函数，启动默认浏览器*/
	ZL_EXP_VOID global_bltLaunchDefaultBrowser(ZL_EXP_VOID * VM_ARG,ZL_EXP_INT argcount)
	{
		ZENGL_EXPORT_MOD_FUN_ARG arg = {ZL_EXP_FAT_NONE,{0}};
		char * url;
		if(argcount != 1)
			zenglApi_Exit(VM_ARG,"bltLaunchDefaultBrowser函数必须接受1个参数");
		zenglApi_GetFunArg(VM_ARG,1,&arg); //得到第一个参数
		if(arg.type != ZL_EXP_FAT_STR)
			zenglApi_Exit(VM_ARG,"bltLaunchDefaultBrowser函数的第一个参数必须是字符串，表示url网址");
		url = arg.val.str;
		wxLaunchDefaultBrowser(url);
		zenglApi_SetRetVal(VM_ARG,ZL_EXP_FAT_INT,ZL_EXP_NULL,1,0);
	}

	/*bltToUTF8模块函数，转为UTF8编码*/
	ZL_EXP_VOID global_bltToUTF8(ZL_EXP_VOID * VM_ARG,ZL_EXP_INT argcount)
	{
		ZENGL_EXPORT_MOD_FUN_ARG arg = {ZL_EXP_FAT_NONE,{0}};
		char * inputstr;
		if(argcount != 1)
			zenglApi_Exit(VM_ARG,"bltToUTF8函数必须接受1个参数");
		zenglApi_GetFunArg(VM_ARG,1,&arg); //得到第一个参数
		if(arg.type != ZL_EXP_FAT_STR)
			zenglApi_Exit(VM_ARG,"bltToUTF8函数的第一个参数必须是字符串，表示要进行转换的字符串");
		inputstr = arg.val.str;
		wxString inputStr = inputstr;
		wxScopedCharBuffer inputStrForUTF8 = inputStr.ToUTF8();
		zenglApi_SetRetVal(VM_ARG,ZL_EXP_FAT_STR,(char *)inputStrForUTF8.data(),0,0);
	}

	/*bltSetInitManageUrl模块函数，设置管理用的网址URL，即采集器在服务端的发布接口*/
	ZL_EXP_VOID global_bltSetInitManageUrl(ZL_EXP_VOID * VM_ARG,ZL_EXP_INT argcount)
	{
		ZENGL_EXPORT_MOD_FUN_ARG arg = {ZL_EXP_FAT_NONE,{0}};
		char * url;
		if(argcount != 1)
			zenglApi_Exit(VM_ARG,"bltSetInitManageUrl函数必须接受1个参数");
		zenglApi_GetFunArg(VM_ARG,1,&arg); //得到第一个参数
		if(arg.type != ZL_EXP_FAT_STR)
			zenglApi_Exit(VM_ARG,"bltSetInitManageUrl函数的第一个参数必须是字符串，表示采集器在服务端的发布接口文件名如：zengl_caiji.php");
		url = arg.val.str;
		gl_initManage_Url = url;
		zenglApi_SetRetVal(VM_ARG,ZL_EXP_FAT_INT,ZL_EXP_NULL,1,0);
	}

	/*bltSetModulePath模块函数，设置模块对应的规则文件名*/
	ZL_EXP_VOID global_bltSetModulePath(ZL_EXP_VOID * VM_ARG,ZL_EXP_INT argcount)
	{
		ZENGL_EXPORT_MOD_FUN_ARG arg = {ZL_EXP_FAT_NONE,{0}};
		char * modname;
		char * filepath;
		if(argcount != 2)
			zenglApi_Exit(VM_ARG,"bltSetModulePath函数必须接受2个参数");
		zenglApi_GetFunArg(VM_ARG,1,&arg); //得到第一个参数
		if(arg.type != ZL_EXP_FAT_STR)
			zenglApi_Exit(VM_ARG,"bltSetModulePath函数的第一个参数必须是字符串，表示模块名");
		modname = arg.val.str;
		zenglApi_GetFunArg(VM_ARG,2,&arg); //得到第二个参数
		if(arg.type != ZL_EXP_FAT_STR)
			zenglApi_Exit(VM_ARG,"bltSetModulePath函数的第二个参数必须是字符串，表示采集规则脚本文件的路径");
		filepath = arg.val.str;
		wxString tmp_modname = modname;
		wxString tmp_filepath = filepath;
		if(tmp_modname == "article")
			gl_zenglrun_article_modname = tmp_filepath;
		else if(tmp_modname == "brand")
			gl_zenglrun_brand_modname = tmp_filepath;
		else if(tmp_modname == "buy")
			gl_zenglrun_buy_modname = tmp_filepath;
		else if(tmp_modname == "exhibit")
			gl_zenglrun_exhibit_modname = tmp_filepath;
		else if(tmp_modname == "group")
			gl_zenglrun_group_modname = tmp_filepath;
		else if(tmp_modname == "info")
			gl_zenglrun_info_modname = tmp_filepath;
		else if(tmp_modname == "job")
			gl_zenglrun_job_modname = tmp_filepath;
		else if(tmp_modname == "know")
			gl_zenglrun_know_modname = tmp_filepath;
		else if(tmp_modname == "mall")
			gl_zenglrun_mall_modname = tmp_filepath;
		else if(tmp_modname == "quote")
			gl_zenglrun_quote_modname = tmp_filepath;
		else if(tmp_modname == "sell")
			gl_zenglrun_sell_modname = tmp_filepath;
		else if(tmp_modname == "special")
			gl_zenglrun_special_modname = tmp_filepath;
		else if(tmp_modname == "photo")
			gl_zenglrun_photo_modname = tmp_filepath;
		else if(tmp_modname == "video")
			gl_zenglrun_video_modname = tmp_filepath;
		else if(tmp_modname == "down")
			gl_zenglrun_down_modname = tmp_filepath;
		else
		{
			wxString errStr = "不存在("+tmp_modname+")模块名";
			zenglApi_SetErrThenStop(VM_ARG,(char *)errStr.c_str().AsChar());
			return;
		}
		zenglApi_SetRetVal(VM_ARG,ZL_EXP_FAT_INT,ZL_EXP_NULL,1,0);
	}

	/*bltGetAreaID，获取地区ID，或随机地区ID*/
	ZL_EXP_VOID global_bltGetAreaID(ZL_EXP_VOID * VM_ARG,ZL_EXP_INT argcount)
	{
		ZENGL_EXPORT_MOD_FUN_ARG arg = {ZL_EXP_FAT_NONE,{0}};
		char * areaname;
		if(argcount != 1)
			zenglApi_Exit(VM_ARG,"bltGetAreaID函数必须接受1个参数");
		zenglApi_GetFunArg(VM_ARG,1,&arg); //得到第一个参数
		if(arg.type != ZL_EXP_FAT_STR)
			zenglApi_Exit(VM_ARG,"bltGetAreaID函数的第一个参数必须是字符串，表示地区名");
		areaname = arg.val.str;
		wxString tmpStr = GetAreaID(areaname);
		zenglApi_SetRetVal(VM_ARG,ZL_EXP_FAT_STR,(char *)tmpStr.c_str().AsChar(),0,0);
	}

	/*bltGetAreaID_NoRandom，获取地区ID*/
	ZL_EXP_VOID global_bltGetAreaID_NoRandom(ZL_EXP_VOID * VM_ARG,ZL_EXP_INT argcount)
	{
		ZENGL_EXPORT_MOD_FUN_ARG arg = {ZL_EXP_FAT_NONE,{0}};
		char * areaname;
		if(argcount != 1)
			zenglApi_Exit(VM_ARG,"bltGetAreaID_NoRandom函数必须接受1个参数");
		zenglApi_GetFunArg(VM_ARG,1,&arg); //得到第一个参数
		if(arg.type != ZL_EXP_FAT_STR)
			zenglApi_Exit(VM_ARG,"bltGetAreaID_NoRandom函数的第一个参数必须是字符串，表示地区名");
		areaname = arg.val.str;
		wxString tmpStr = GetAreaID_NoRandom(areaname);
		zenglApi_SetRetVal(VM_ARG,ZL_EXP_FAT_STR,(char *)tmpStr.c_str().AsChar(),0,0);
	}

	/*bltGetAreaName_NoRandom，获取地区名*/
	ZL_EXP_VOID global_bltGetAreaName_NoRandom(ZL_EXP_VOID * VM_ARG,ZL_EXP_INT argcount)
	{
		ZENGL_EXPORT_MOD_FUN_ARG arg = {ZL_EXP_FAT_NONE,{0}};
		char * areaname,* defaultName;
		if(argcount != 2)
			zenglApi_Exit(VM_ARG,"bltGetAreaName_NoRandom函数必须接受2个参数");
		zenglApi_GetFunArg(VM_ARG,1,&arg); //得到第一个参数
		if(arg.type != ZL_EXP_FAT_STR)
			zenglApi_Exit(VM_ARG,"bltGetAreaName_NoRandom函数的第一个参数必须是字符串，表示地区名");
		areaname = arg.val.str;
		zenglApi_GetFunArg(VM_ARG,2,&arg); //得到第二个参数
		if(arg.type != ZL_EXP_FAT_STR)
			zenglApi_Exit(VM_ARG,"bltGetAreaName_NoRandom函数的第二个参数必须是字符串，表示没找到时需要返回的默认名称");
		defaultName = arg.val.str;
		wxString tmpStr = GetAreaName_NoRandom(areaname,defaultName);
		zenglApi_SetRetVal(VM_ARG,ZL_EXP_FAT_STR,(char *)tmpStr.c_str().AsChar(),0,0);
	}

	/*bltCheckCaijiCompany模块函数，判断是否需要发布公司信息*/
	ZL_EXP_VOID global_bltCheckCaijiCompany(ZL_EXP_VOID * VM_ARG,ZL_EXP_INT argcount)
	{
		if(argcount != 0)
			zenglApi_Exit(VM_ARG,"bltCheckCaijiCompany函数目前不接受任何参数");
		if(glmainFrame->m_checkCaijiCompany->IsChecked())
		{
			zenglApi_SetRetVal(VM_ARG,ZL_EXP_FAT_INT,ZL_EXP_NULL,1,0);
		}
		else
			zenglApi_SetRetVal(VM_ARG,ZL_EXP_FAT_INT,ZL_EXP_NULL,0,0);
	}

	/*bltGetCompanyCatid模块函数，获取要发布到的公司的分类ID*/
	ZL_EXP_VOID global_bltGetCompanyCatid(ZL_EXP_VOID * VM_ARG,ZL_EXP_INT argcount)
	{
		if(argcount != 0)
			zenglApi_Exit(VM_ARG,"bltGetCompanyCatid函数目前不接受任何参数");
		long ret;
		MyTreeCtrl * treeCtrl = glmainFrame->treeCtrl;
		wxTreeItemId rootid = treeCtrl->GetRootItem();
		wxTreeItemIdValue cookie,cookie2;
		wxTreeItemId cateitemid = treeCtrl->GetFirstChild(rootid,cookie),cateitemChild;
		MyTreeItemData * itemdata = NULL;
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
				if(itemdata->m_modID == "4" && treeCtrl->GetItemState(cateitemChild) == 1)
				{
					glCompanyCatID = itemdata->m_cateID;
					glCompanyCatID.ToLong(&ret);
					zenglApi_SetRetVal(VM_ARG,ZL_EXP_FAT_INT,ZL_EXP_NULL,(int)ret,0);
					return ;
				}
			}while(cateitemChild = treeCtrl->GetNextChild(cateitemChild,cookie2));
		}while(cateitemid = treeCtrl->GetNextChild(cateitemid,cookie));
		glCompanyCatID = "3";
		glCompanyCatID.ToLong(&ret);
		zenglApi_SetRetVal(VM_ARG,ZL_EXP_FAT_INT,ZL_EXP_NULL,(int)ret,0);
	}

	/*bltBase64Encode模块函数，将字符串信息进行Base64编码*/
	ZL_EXP_VOID global_bltBase64Encode(ZL_EXP_VOID * VM_ARG,ZL_EXP_INT argcount)
	{
		ZENGL_EXPORT_MOD_FUN_ARG arg = {ZL_EXP_FAT_NONE,{0}};
		if(argcount != 1)
			zenglApi_Exit(VM_ARG,"bltBase64Encode函数必须接受1个参数");
		zenglApi_GetFunArg(VM_ARG,1,&arg); //得到第一个参数
		if(arg.type != ZL_EXP_FAT_STR)
			zenglApi_Exit(VM_ARG,"bltBase64Encode函数的第一个参数必须是字符串，表示要进行编码的字符串");
		int len = strlen(arg.val.str);
		wchar_t * output = (wchar_t *)zenglApi_AllocMem(VM_ARG,(((len + 2) / 3) * 4 + 1) * sizeof(wchar_t));
		if(BASE64_Encode((unsigned char *)arg.val.str,len,output) == -1)
			zenglApi_Exit(VM_ARG,"bltBase64Encode函数编码失败！");
		wxString wxOutput = output;
		zenglApi_SetRetVal(VM_ARG,ZL_EXP_FAT_STR,(char *)wxOutput.c_str().AsChar(),0,0);
		zenglApi_FreeMem(VM_ARG,output);
	}

	/*bltBase64Decode模块函数，将Base64编码还原为字符串*/
	ZL_EXP_VOID global_bltBase64Decode(ZL_EXP_VOID * VM_ARG,ZL_EXP_INT argcount)
	{
		ZENGL_EXPORT_MOD_FUN_ARG arg = {ZL_EXP_FAT_NONE,{0}};
		if(argcount != 1)
			zenglApi_Exit(VM_ARG,"bltBase64Decode函数必须接受1个参数");
		zenglApi_GetFunArg(VM_ARG,1,&arg); //得到第一个参数
		if(arg.type != ZL_EXP_FAT_STR)
			zenglApi_Exit(VM_ARG,"bltBase64Decode函数的第一个参数必须是字符串，表示要处理的BASE64编码");
		wxString wxInput = arg.val.str;
		int len = (int)wxInput.Length();
		int output_len = (len >> 2) * 3;
		char * output = (char *)zenglApi_AllocMem(VM_ARG,(output_len + 1) * sizeof(char));
		int ret = 0;
		ret = BASE64_Decode((wchar_t *)wxInput.wc_str(),len,(unsigned char *)output);
		if(ret == -1)
		{
			zenglApi_SetErrThenStop(VM_ARG,"bltBase64Decode函数解码时遇到内部参数错误");
			return;
		}
		else if(ret == -2)
		{
			zenglApi_SetErrThenStop(VM_ARG,"bltBase64Decode函数错误，无效的BASE64编码");
			return;
		}
		output[output_len] = '\0';
		zenglApi_SetRetVal(VM_ARG,ZL_EXP_FAT_STR,output,0,0);
		zenglApi_FreeMem(VM_ARG,output);
	}

	/*bltCurlDownload模块函数，从url中下载某个文件*/
	ZL_EXP_VOID global_bltCurlDownload(ZL_EXP_VOID * VM_ARG,ZL_EXP_INT argcount)
	{
		ZENGL_EXPORT_MOD_FUN_ARG arg = {ZL_EXP_FAT_NONE,{0}};
		ZL_EXP_CHAR * dest_filename;
		ZL_EXP_CHAR * url;
		if(argcount != 2)
			zenglApi_Exit(VM_ARG,"bltCurlDownload函数必须接受2个参数");
		zenglApi_GetFunArg(VM_ARG,1,&arg); //得到第一个参数
		if(arg.type != ZL_EXP_FAT_STR)
			zenglApi_Exit(VM_ARG,"bltCurlDownload函数的第一个参数必须是字符串，表示下载后要保存的文件名");
		dest_filename = arg.val.str;
		zenglApi_GetFunArg(VM_ARG,2,&arg); //得到第二个参数
		if(arg.type != ZL_EXP_FAT_STR)
			zenglApi_Exit(VM_ARG,"bltCurlDownload函数的第二个参数必须是字符串，表示url下载地址");
		url = arg.val.str;
		curl=curl_easy_init();
        curl_easy_setopt(curl, CURLOPT_URL,url);
		curl_easy_setopt(curl, CURLOPT_USERAGENT ,"Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.2; .NET CLR 1.1.4322)");
		curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION ,1);
		long timeout;
		glmainFrame->m_textForTimeOut->GetValue().ToLong(&timeout);
		curl_easy_setopt(curl, CURLOPT_TIMEOUT, (int)timeout);//设置超时时间
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, mywxcurl_file_write);
		FILE * fp;
		if((fp=fopen(dest_filename,"wb+"))==NULL)
		{
			curl_easy_cleanup(curl);
			curl = NULL;
			myglStyle = MY_RICHTEXT_RED_STYLE;
			myglStr = _("保存为文件 <") + dest_filename + _("> 失败！\n");
			wxMyLogEvent event( wxEVT_MY_LOG_EVENT,wxID_ANY,myglStr,myglStyle);
			wxQueueEvent(glmainFrame->GetEventHandler(),event.Clone());
			zenglApi_SetRetVal(VM_ARG,ZL_EXP_FAT_INT,ZL_EXP_NULL,-1,0);
			return;
		}
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
		curl_easy_perform(curl);
		curl_easy_cleanup(curl);
		curl = NULL;
		fclose(fp);
		int fileSize = myGetFileSize(dest_filename);
		zenglApi_SetRetVal(VM_ARG,ZL_EXP_FAT_INT,ZL_EXP_NULL,fileSize,0);
	}

	/*bltGetMenuCheck模块函数，判断某个菜单项是否被check选中，左侧打勾状态*/
	ZL_EXP_VOID global_bltGetMenuCheck(ZL_EXP_VOID * VM_ARG,ZL_EXP_INT argcount)
	{
		ZENGL_EXPORT_MOD_FUN_ARG arg = {ZL_EXP_FAT_NONE,{0}};
		if(argcount != 1)
			zenglApi_Exit(VM_ARG,"bltGetMenuCheck函数必须接受1个参数");
		zenglApi_GetFunArg(VM_ARG,1,&arg); //得到第一个参数
		if(arg.type != ZL_EXP_FAT_STR)
			zenglApi_Exit(VM_ARG,"bltGetMenuCheck函数的第一个参数必须是字符串，表示要检查的菜单名");
		wxString menuName = arg.val.str;
		wxMenuBar * menubar = glmainFrame->GetMenuBar();
		int menuid;
		if(menuName == "倒序发布")
			menuid = ID_MENU_EXTRA_REVERSE_ORDER;
		else if(menuName == "发布到待审核")
			menuid = ID_MENU_EXTRA_POST_PENDING;
		else if(menuName == "图片下载到本地再上传")
			menuid = ID_MENU_EXTRA_LOCAL_IMG;
		else
		{
			zenglApi_SetErrThenStop(VM_ARG,"bltGetMenuCheck函数第一个参数对应的菜单名'%s'不对，或者该菜单名无法设置选中打勾状态",menuName.c_str().AsChar());
			return;
		}
		if(menubar->IsChecked(menuid))
			zenglApi_SetRetVal(VM_ARG,ZL_EXP_FAT_INT,ZL_EXP_NULL,1,0);
		else
			zenglApi_SetRetVal(VM_ARG,ZL_EXP_FAT_INT,ZL_EXP_NULL,0,0);
	}

	ZL_EXP_VOID global_builtin_module_init(ZL_EXP_VOID * VM_ARG,ZL_EXP_INT moduleID)
	{
		zenglApi_SetModFunHandle(VM_ARG,moduleID,"printf",global_printf);
		zenglApi_SetModFunHandle(VM_ARG,moduleID,"read",global_read);
		//zenglApi_SetModFunHandle(VM_ARG,moduleID,"bltRandom",global_bltRandom);
		zenglApi_SetModFunHandle(VM_ARG,moduleID,"bltRandom",zenglApiBMF_bltRandom);
		//zenglApi_SetModFunHandle(VM_ARG,moduleID,"array",global_array);
		zenglApi_SetModFunHandle(VM_ARG,moduleID,"array",zenglApiBMF_array);
		zenglApi_SetModFunHandle(VM_ARG,moduleID,"unset",zenglApiBMF_unset);
		zenglApi_SetModFunHandle(VM_ARG,moduleID,"bltPrintArray",global_bltPrintArray);
		zenglApi_SetModFunHandle(VM_ARG,moduleID,"bltTestAddr",global_bltTestAddr);
		zenglApi_SetModFunHandle(VM_ARG,moduleID,"bltSetArray",global_bltSetArray);
		zenglApi_SetModFunHandle(VM_ARG,moduleID,"bltLoadScript",global_bltLoadScript);
		zenglApi_SetModFunHandle(VM_ARG,moduleID,"bltGetZLVersion",global_bltGetZLVersion);
		zenglApi_SetModFunHandle(VM_ARG,moduleID,"bltGetCaijiNum",global_bltGetCaijiNum);
		zenglApi_SetModFunHandle(VM_ARG,moduleID,"bltGetCatid",global_bltGetCatid);
		zenglApi_SetModFunHandle(VM_ARG,moduleID,"bltGetModid",global_bltGetModid);
		zenglApi_SetModFunHandle(VM_ARG,moduleID,"bltGetCatName",global_bltGetCatName);
		zenglApi_SetModFunHandle(VM_ARG,moduleID,"bltGetWebUrl",global_bltGetWebUrl);
		zenglApi_SetModFunHandle(VM_ARG,moduleID,"bltGetInitManageUrl",global_bltGetInitManageUrl);
		zenglApi_SetModFunHandle(VM_ARG,moduleID,"bltGetKeyWords",global_bltGetKeyWords);
		zenglApi_SetModFunHandle(VM_ARG,moduleID,"bltCurlEncode",global_bltCurlEncode);
		zenglApi_SetModFunHandle(VM_ARG,moduleID,"bltCurlGetUrl",global_bltCurlGetUrl);
		zenglApi_SetModFunHandle(VM_ARG,moduleID,"bltInfoBox",global_bltInfoBox);
		zenglApi_SetModFunHandle(VM_ARG,moduleID,"bltRegexMatches",global_bltRegexMatches);
		zenglApi_SetModFunHandle(VM_ARG,moduleID,"bltArrayInsertString",global_bltArrayInsertString);
		zenglApi_SetModFunHandle(VM_ARG,moduleID,"bltSqlTableExists",global_bltSqlTableExists);
		zenglApi_SetModFunHandle(VM_ARG,moduleID,"bltSqlEscape",global_bltSqlEscape);
		zenglApi_SetModFunHandle(VM_ARG,moduleID,"bltSqlQuery",global_bltSqlQuery);
		//zenglApi_SetModFunHandle(VM_ARG,moduleID,"bltExit",global_bltExit);
		zenglApi_SetModFunHandle(VM_ARG,moduleID,"bltExit",zenglApiBMF_bltExit);
		zenglApi_SetModFunHandle(VM_ARG,moduleID,"bltSqlMoveToNext",global_bltSqlMoveToNext);
		zenglApi_SetModFunHandle(VM_ARG,moduleID,"bltSqlGetString",global_bltSqlGetString);
		zenglApi_SetModFunHandle(VM_ARG,moduleID,"bltSqlRelease",global_bltSqlRelease);
		zenglApi_SetModFunHandle(VM_ARG,moduleID,"bltSqlExec",global_bltSqlExec);
		zenglApi_SetModFunHandle(VM_ARG,moduleID,"bltRegexMatchFirst",global_bltRegexMatchFirst);
		zenglApi_SetModFunHandle(VM_ARG,moduleID,"bltRegexReplace",global_bltRegexReplace);
		zenglApi_SetModFunHandle(VM_ARG,moduleID,"bltTrim",global_bltTrim);
		zenglApi_SetModFunHandle(VM_ARG,moduleID,"bltCheckTitleMust",global_bltCheckTitleMust);
		zenglApi_SetModFunHandle(VM_ARG,moduleID,"bltStrReplace",global_bltStrReplace);
		zenglApi_SetModFunHandle(VM_ARG,moduleID,"bltCheckAutoPass",global_bltCheckAutoPass);
		zenglApi_SetModFunHandle(VM_ARG,moduleID,"bltPostData",global_bltPostData);
		//zenglApi_SetModFunHandle(VM_ARG,moduleID,"bltIntToStr",global_bltIntToStr);
		zenglApi_SetModFunHandle(VM_ARG,moduleID,"bltIntToStr",zenglApiBMF_bltIntToStr);
		zenglApi_SetModFunHandle(VM_ARG,moduleID,"bltAddProgress",global_bltAddProgress);
		zenglApi_SetModFunHandle(VM_ARG,moduleID,"bltCheckUserPause",global_bltCheckUserPause);
		zenglApi_SetModFunHandle(VM_ARG,moduleID,"bltGetMaxUploadIMG",global_bltGetMaxUploadIMG);
		zenglApi_SetModFunHandle(VM_ARG,moduleID,"bltGetTimeNow",global_bltGetTimeNow);
		zenglApi_SetModFunHandle(VM_ARG,moduleID,"bltAddProgressEx",global_bltAddProgressEx);
		zenglApi_SetModFunHandle(VM_ARG,moduleID,"bltTellSysCaijiStatus",global_bltTellSysCaijiStatus);
		//zenglApi_SetModFunHandle(VM_ARG,moduleID,"bltConvToInt",global_bltConvToInt);
		zenglApi_SetModFunHandle(VM_ARG,moduleID,"bltConvToInt",zenglApiBMF_bltConvToInt);
		zenglApi_SetModFunHandle(VM_ARG,moduleID,"bltGetArgString",global_bltGetArgString);
		zenglApi_SetModFunHandle(VM_ARG,moduleID,"bltStrFind",global_bltStrFind);
		zenglApi_SetModFunHandle(VM_ARG,moduleID,"bltLaunchDefaultBrowser",global_bltLaunchDefaultBrowser);
		zenglApi_SetModFunHandle(VM_ARG,moduleID,"bltToUTF8",global_bltToUTF8);
		zenglApi_SetModFunHandle(VM_ARG,moduleID,"bltSetInitManageUrl",global_bltSetInitManageUrl);
		zenglApi_SetModFunHandle(VM_ARG,moduleID,"bltSetModulePath",global_bltSetModulePath);
		zenglApi_SetModFunHandle(VM_ARG,moduleID,"bltGetAreaID",global_bltGetAreaID);
		zenglApi_SetModFunHandle(VM_ARG,moduleID,"bltGetAreaID_NoRandom",global_bltGetAreaID_NoRandom);
		zenglApi_SetModFunHandle(VM_ARG,moduleID,"bltGetAreaName_NoRandom",global_bltGetAreaName_NoRandom);
		zenglApi_SetModFunHandle(VM_ARG,moduleID,"bltCheckCaijiCompany",global_bltCheckCaijiCompany);
		zenglApi_SetModFunHandle(VM_ARG,moduleID,"bltGetCompanyCatid",global_bltGetCompanyCatid);
		zenglApi_SetModFunHandle(VM_ARG,moduleID,"bltBase64Encode",global_bltBase64Encode);
		zenglApi_SetModFunHandle(VM_ARG,moduleID,"bltBase64Decode",global_bltBase64Decode);
		zenglApi_SetModFunHandle(VM_ARG,moduleID,"bltCurlDownload",global_bltCurlDownload);
		zenglApi_SetModFunHandle(VM_ARG,moduleID,"bltGetMenuCheck",global_bltGetMenuCheck);
	}

	void global_JumpToCaiji_InitFuncall(void * VM_ARG)
	{
		int moduleID = 0;
		zenglApi_SetModFunHandle(VM_ARG,moduleID,"bltGetArgString",global_bltGetArgString);
		zenglApi_SetModFunHandle(VM_ARG,moduleID,"bltSqlEscape",global_bltSqlEscape);
		zenglApi_SetModFunHandle(VM_ARG,moduleID,"bltSqlQuery",global_bltSqlQuery);
		zenglApi_SetModFunHandle(VM_ARG,moduleID,"bltExit",global_bltExit);
		zenglApi_SetModFunHandle(VM_ARG,moduleID,"bltSqlMoveToNext",global_bltSqlMoveToNext);
		zenglApi_SetModFunHandle(VM_ARG,moduleID,"bltConvToInt",global_bltConvToInt);
		zenglApi_SetModFunHandle(VM_ARG,moduleID,"bltSqlGetString",global_bltSqlGetString);
		zenglApi_SetModFunHandle(VM_ARG,moduleID,"bltStrFind",global_bltStrFind);
		zenglApi_SetModFunHandle(VM_ARG,moduleID,"bltCurlEncode",global_bltCurlEncode);
		zenglApi_SetModFunHandle(VM_ARG,moduleID,"bltLaunchDefaultBrowser",global_bltLaunchDefaultBrowser);
		zenglApi_SetModFunHandle(VM_ARG,moduleID,"bltToUTF8",global_bltToUTF8);
		zenglApi_SetModFunHandle(VM_ARG,moduleID,"bltInfoBox",global_bltInfoBox);
		zenglApi_SetModFunHandle(VM_ARG,moduleID,"bltSqlRelease",global_bltSqlRelease);
	}

	ZL_EXP_VOID global_module_init(ZL_EXP_VOID * VM_ARG)
	{
		zenglApi_SetModInitHandle(VM_ARG,"builtin",global_builtin_module_init);
	}

	ZL_EXP_VOID global_print_debug(ZL_EXP_VOID * VM_ARG,ZL_EXP_CHAR * debug_str)
	{
		ZENGL_EXPORT_MOD_FUN_ARG reg_debug;
		ZL_EXP_INT debug_str_len = strlen(debug_str);
		ZL_EXP_BOOL has_semi = ZL_EXP_FALSE;
		wxMyLogEvent mylogevent( wxEVT_MY_LOG_EVENT,wxID_ANY);
		zenglApi_GetDebug(VM_ARG,&reg_debug);
		if(debug_str[debug_str_len-1]==';')
		{
			debug_str[debug_str_len-1] = ' ';
			has_semi = ZL_EXP_TRUE;
		}
		mylogevent.SetEventMsg(wxString::Format("%s:",debug_str),MY_RICHTEXT_NORMAL_STYLE);
		wxQueueEvent(glmainFrame->GetEventHandler(),mylogevent.Clone());
		switch(reg_debug.type)
		{
		case ZL_EXP_FAT_NONE:
			mylogevent.SetEventMsg(wxString::Format("none type , number equal %d",reg_debug.val.integer),MY_RICHTEXT_NORMAL_STYLE);
			wxQueueEvent(glmainFrame->GetEventHandler(),mylogevent.Clone());
			break;
		case ZL_EXP_FAT_INT:
			mylogevent.SetEventMsg(wxString::Format("integer:%d",reg_debug.val.integer),MY_RICHTEXT_NORMAL_STYLE);
			wxQueueEvent(glmainFrame->GetEventHandler(),mylogevent.Clone());
			break;
		case ZL_EXP_FAT_FLOAT:
			mylogevent.SetEventMsg(wxString::Format("float:%.16g",reg_debug.val.floatnum),MY_RICHTEXT_NORMAL_STYLE);
			wxQueueEvent(glmainFrame->GetEventHandler(),mylogevent.Clone());
			break;
		case ZL_EXP_FAT_STR:
			mylogevent.SetEventMsg(wxString::Format("string:%s",reg_debug.val.str),MY_RICHTEXT_NORMAL_STYLE);
			wxQueueEvent(glmainFrame->GetEventHandler(),mylogevent.Clone());
			break;
		case ZL_EXP_FAT_MEMBLOCK:
			global_print_array(VM_ARG,reg_debug.val.memblock,0);
			break;
		case ZL_EXP_FAT_ADDR:
		case ZL_EXP_FAT_ADDR_LOC:
		case ZL_EXP_FAT_ADDR_MEMBLK:
			mylogevent.SetEventMsg("addr type",MY_RICHTEXT_NORMAL_STYLE);
			wxQueueEvent(glmainFrame->GetEventHandler(),mylogevent.Clone());
			break;
		case ZL_EXP_FAT_INVALID:
			mylogevent.SetEventMsg("invalid type",MY_RICHTEXT_NORMAL_STYLE);
			wxQueueEvent(glmainFrame->GetEventHandler(),mylogevent.Clone());
			break;
		}
		mylogevent.SetEventMsg("\n",MY_RICHTEXT_NORMAL_STYLE);
		wxQueueEvent(glmainFrame->GetEventHandler(),mylogevent.Clone());
		if(has_semi == ZL_EXP_TRUE)
		{
			debug_str[debug_str_len-1] = ';';
			has_semi = ZL_EXP_FALSE;
		}
	}

	/*递归返回数组信息*/
	ZL_EXP_VOID global_retStr_debugArray(ZL_EXP_VOID * VM_ARG,ZENGL_EXPORT_MEMBLOCK memblock,ZL_EXP_INT recur_count,ZL_EXP_VOID * retStr)
	{
		ZL_EXP_INT size,i,j;
		ZENGL_EXPORT_MOD_FUN_ARG mblk_val = {ZL_EXP_FAT_NONE,{0}};
		wxString result = "";
		wxString tmp = "";
		zenglApi_GetMemBlockInfo(VM_ARG,&memblock,&size,ZL_EXP_NULL);
		for(i=1;i<=size;i++)
		{
			mblk_val = zenglApi_GetMemBlock(VM_ARG,&memblock,i);
			switch(mblk_val.type)
			{
			case ZL_EXP_FAT_INT:
			case ZL_EXP_FAT_FLOAT:
			case ZL_EXP_FAT_STR:
			case ZL_EXP_FAT_MEMBLOCK:
				for(j=0;j<recur_count;j++)
				{
					result += "  ";
				}
				break;
			}
			switch(mblk_val.type)
			{
			case ZL_EXP_FAT_INT:
				result += wxString::Format("[%d] %d\n",i-1,mblk_val.val.integer);
				break;
			case ZL_EXP_FAT_FLOAT:
				result += wxString::Format("[%d] %.16g\n",i-1,mblk_val.val.floatnum);
				break;
			case ZL_EXP_FAT_STR:
				result += wxString::Format("[%d] %s\n",i-1,mblk_val.val.str);
				break;
			case ZL_EXP_FAT_MEMBLOCK:
				result += wxString::Format("[%d] <array or class obj type> begin:\n",i-1);
				global_retStr_debugArray(VM_ARG,mblk_val.val.memblock,recur_count+1,&tmp);
				result += tmp;
				result += wxString::Format("[%d] <array or class obj type> end\n",i-1);
				break;
			}
		}
		(*(wxString *)retStr) = result;
		return;
	}

	ZL_EXP_VOID global_retStr_debug(ZL_EXP_VOID * VM_ARG,ZL_EXP_CHAR * debug_str,ZL_EXP_VOID * retStr)
	{
		ZENGL_EXPORT_MOD_FUN_ARG reg_debug;
		ZL_EXP_INT debug_str_len = strlen(debug_str);
		ZL_EXP_BOOL has_semi = ZL_EXP_FALSE;
		wxString result = "";
		wxString tmp = "";
		zenglApi_GetDebug(VM_ARG,&reg_debug);
		if(debug_str[debug_str_len-1]==';')
		{
			debug_str[debug_str_len-1] = ' ';
			has_semi = ZL_EXP_TRUE;
		}
		result += wxString::Format("%s:",debug_str);
		switch(reg_debug.type)
		{
		case ZL_EXP_FAT_NONE:
			result += wxString::Format("none type , number equal %d",reg_debug.val.integer);
			break;
		case ZL_EXP_FAT_INT:
			result += wxString::Format("integer:%d",reg_debug.val.integer);
			break;
		case ZL_EXP_FAT_FLOAT:
			result += wxString::Format("float:%.16g",reg_debug.val.floatnum);
			break;
		case ZL_EXP_FAT_STR:
			result += wxString::Format("string:%s",reg_debug.val.str);
			break;
		case ZL_EXP_FAT_MEMBLOCK:
			global_retStr_debugArray(VM_ARG,reg_debug.val.memblock,0,&tmp);
			result += tmp;
			break;
		case ZL_EXP_FAT_ADDR:
		case ZL_EXP_FAT_ADDR_LOC:
		case ZL_EXP_FAT_ADDR_MEMBLK:
			result += _("addr type");
			break;
		case ZL_EXP_FAT_INVALID:
			result += _("invalid type");
			break;
		}
		result += ("\n");
		if(has_semi == ZL_EXP_TRUE)
		{
			debug_str[debug_str_len-1] = ';';
			has_semi = ZL_EXP_FALSE;
		}
		(*(wxString *)retStr) = result;
		return;
	}

	ZL_EXP_VOID global_debug_printlog(wxMyLogEvent &mylogevent,wxString log)
	{
		mylogevent.SetEventMsg(log,MY_RICHTEXT_NORMAL_STYLE);
		wxQueueEvent(glmainFrame->GetEventHandler(),mylogevent.Clone());
	}

	ZL_EXP_CHAR * global_getDebugArg(ZL_EXP_CHAR * str,ZL_EXP_INT * start,ZL_EXP_BOOL needNull)
	{
		int i;
		char * ret;
		if((*start) < 0)
		{
			(*start) = -1;
			return ZL_EXP_NULL;
		}
		for(i=(*start);;i++)
		{
			if(str[i] == ' ' || str[i] == '\t')
				continue;
			else if(str[i] == STRNULL)
			{
				(*start) = -1;
				return ZL_EXP_NULL;
			}
			else
			{
				ret = str + i;
				while(++i)
				{
					if(str[i] == ' ' || str[i] == '\t')
					{
						if(needNull != ZL_EXP_FALSE)
							str[i] = STRNULL;
						(*start) = i+1;
						break;
					}
					else if(str[i] == STRNULL)
					{
						(*start) = -1;
						break;
					}
				}
				return ret;
			} //else
		}//for(i=(*start);;i++)
		(*start) = -1;
		return ZL_EXP_NULL;
	}

	ZL_EXP_BOOL global_isNumber(ZL_EXP_CHAR * str)
	{
		int len = strlen(str);
		int i;
		for(i=0;i<len;i++)
		{
			if(!isdigit(str[i]))
				return ZL_EXP_FALSE;
		}
		return ZL_EXP_TRUE;
	}

	ZL_EXP_INT global_debug_break(ZL_EXP_VOID * VM_ARG,ZL_EXP_CHAR * cur_filename,ZL_EXP_INT cur_line,ZL_EXP_INT breakIndex,ZL_EXP_CHAR * log)
	{
		wxMyLogEvent mylogevent( wxEVT_MY_LOG_EVENT,wxID_ANY);
		char * str = ZL_EXP_NULL;
		char * inputstr_ptr = ZL_EXP_NULL;
		char * tmpstr = ZL_EXP_NULL;
		char * command,* arg;
		int i,start;
		int inputlength = 0;
		int exit = 0;
		int str_size = 0;
		int str_count = 0;
		int tmplen;
		if(log != ZL_EXP_NULL)
		{
			if(zenglApi_Debug(VM_ARG,log) == -1)
			{
				global_debug_printlog(mylogevent,wxString::Format("log日志断点错误：%s",zenglApi_GetErrorString(VM_ARG)));
			}
			else
			{
				global_print_debug(VM_ARG,log);
				return 0;
			}
		}
		global_debug_printlog(mylogevent,wxString::Format("* %s:%d ",cur_filename,cur_line));
		if(breakIndex == -1)
			global_debug_printlog(mylogevent,wxString::Format("Single Break [current]\n"));
		else
			global_debug_printlog(mylogevent,wxString::Format("Break index:%d [current]\n",breakIndex));
		if(str == ZL_EXP_NULL)
		{
			str_size = DEBUG_INPUT_MAX;
			str = (char *)zenglApi_AllocMem(VM_ARG,str_size);
		}
		while(!exit)
		{
			if(glmainFrame->m_isDebugPause == false && glmainFrame->mythread2 != NULL && !glmainFrame->mythread2->IsPaused())
			{
				wxCommandEvent eventForPause(wxEVT_MY_DEBUG_PAUSE_THREAD,ID_MY_WINDOW);
				wxQueueEvent(glmainFrame->GetEventHandler(),eventForPause.Clone());
				wxThread::Yield();
				wxThread::Sleep(500);
				wxString inputstr = glmainFrame->textForDebug->GetValue();
				mylogevent.SetEventMsg(inputstr+"\n",MY_RICHTEXT_NORMAL_STYLE);
				wxQueueEvent(glmainFrame->GetEventHandler(),mylogevent.Clone());
				inputstr_ptr = (char *)inputstr.c_str().AsChar();
				inputlength = strlen(inputstr_ptr);
				for(i=0;i < inputlength;i++)
				{
					if(i >= str_size - 10) //i到达最后一个元素时，对str进行扩容，str_size - 10让str可以在尾部预留一段空间
					{
						str_size += DEBUG_INPUT_MAX;
						tmpstr = (char *)zenglApi_AllocMem(VM_ARG,str_size);
						strcpy(tmpstr,str);
						zenglApi_FreeMem(VM_ARG,str);
						str = tmpstr;
					}
					str[i] = inputstr_ptr[i];
				}
				str[i] = STRNULL;
				str_count = i;
				start = 0;
				command = global_getDebugArg(str,&start,ZL_EXP_TRUE);
				if(command == ZL_EXP_NULL || strlen(command) != 1)
				{
					global_debug_printlog(mylogevent,wxString::Format("命令必须是一个字符\n"));
					continue;
				}
				switch(command[0])
				{
				case 'P':
				case 'p':
					{
						arg = global_getDebugArg(str,&start,ZL_EXP_FALSE);
						tmplen = arg != ZL_EXP_NULL ? strlen(arg) : 0;
						if(arg != ZL_EXP_NULL && tmplen > 0)
						{
							if(arg[tmplen - 1] != ';' && str_count < str_size - 1)
							{
								arg[tmplen] = ';';
								arg[tmplen+1] = STRNULL;
							}
							if(zenglApi_Debug(VM_ARG,arg) == -1)
							{
								global_debug_printlog(mylogevent,wxString::Format("p调试错误：%s\n",zenglApi_GetErrorString(VM_ARG)));
								continue;
							}
							if(command[0] == 'P')
							{
								wxString retStr;
								global_retStr_debug(VM_ARG,arg,&retStr);
								wxMessageBox(retStr,"打印调试信息");
							}
							else
								global_print_debug(VM_ARG,arg);
						}
						else
							global_debug_printlog(mylogevent,wxString::Format("p命令缺少参数\n"));
					}
					break;
				case 'b':
					{
						char * filename = ZL_EXP_NULL;
						int line = 0;
						int count = 0;
						arg = global_getDebugArg(str,&start,ZL_EXP_TRUE);
						if(arg != ZL_EXP_NULL && strlen(arg) > 0)
							filename = arg;
						else
						{
							global_debug_printlog(mylogevent,wxString::Format("b命令缺少文件名参数\n"));
							continue;
						}
						arg = global_getDebugArg(str,&start,ZL_EXP_TRUE);
						if(arg != ZL_EXP_NULL && strlen(arg) > 0)
							line = atoi(arg);
						else
						{
							global_debug_printlog(mylogevent,wxString::Format("b命令缺少行号参数\n"));
							continue;
						}
						arg = global_getDebugArg(str,&start,ZL_EXP_TRUE);
						if(arg != ZL_EXP_NULL && strlen(arg) > 0)
							count = atoi(arg);
						if(zenglApi_DebugSetBreak(VM_ARG,filename,line,ZL_EXP_NULL,ZL_EXP_NULL,count,ZL_EXP_FALSE) == -1)
							global_debug_printlog(mylogevent,wxString::Format("b命令error:%s\n",zenglApi_GetErrorString(VM_ARG)));
						else
							global_debug_printlog(mylogevent,wxString::Format("设置断点成功\n"));
					}
					break;
				case 'B':
					{
						int size;
						int totalcount;
						int i;
						char * filename = ZL_EXP_NULL;
						char * condition = ZL_EXP_NULL;
						char * log = ZL_EXP_NULL;
						int count;
						int line;
						ZL_EXP_BOOL disabled;
						if(breakIndex == -1)
							global_debug_printlog(mylogevent,wxString::Format("* %s:%d Single Break [current]\n",cur_filename,cur_line));
						size = zenglApi_DebugGetBreak(VM_ARG,-1,ZL_EXP_NULL,ZL_EXP_NULL,ZL_EXP_NULL,ZL_EXP_NULL,&totalcount,ZL_EXP_NULL,ZL_EXP_NULL);
						for(i=0;i<size;i++)
						{
							if(zenglApi_DebugGetBreak(VM_ARG,i,&filename,&line,&condition,&log,&count,&disabled,ZL_EXP_NULL) == -1)
								continue;
							else
							{
								global_debug_printlog(mylogevent,wxString::Format("[%d] %s:%d",i,filename,line));
								if(condition != ZL_EXP_NULL)
									global_debug_printlog(mylogevent,wxString::Format(" C:%s",condition));
								if(log != ZL_EXP_NULL)
									global_debug_printlog(mylogevent,wxString::Format(" L:%s",log));
								global_debug_printlog(mylogevent,wxString::Format(" N:%d",count));
								if(disabled == ZL_EXP_FALSE)
									global_debug_printlog(mylogevent,wxString::Format(" D:enable"));
								else
									global_debug_printlog(mylogevent,wxString::Format(" D:disable"));
								if(i == breakIndex)
									global_debug_printlog(mylogevent,wxString::Format(" [current]"));
								global_debug_printlog(mylogevent,wxString::Format("\n"));
							}
						}
						global_debug_printlog(mylogevent,wxString::Format("total:%d\n",totalcount));
					}
					break;
				case 'T':
					{	
						int arg = -1;
						int loc = -1;
						int pc = -1;
						int ret;
						int line = 0;
						char * fileName = ZL_EXP_NULL;
						char * className = ZL_EXP_NULL;
						char * funcName = ZL_EXP_NULL;
						while(ZL_EXP_TRUE)
						{
							ret = zenglApi_DebugGetTrace(VM_ARG,&arg,&loc,&pc,&fileName,&line,&className,&funcName);
							if(ret == 1)
							{
								global_debug_printlog(mylogevent,wxString::Format(" %s:%d ",fileName,line));
								if(className != ZL_EXP_NULL)
									global_debug_printlog(mylogevent,wxString::Format("%s:",className));
								if(funcName != ZL_EXP_NULL)
									global_debug_printlog(mylogevent,wxString::Format("%s",funcName));
								global_debug_printlog(mylogevent,wxString::Format("\n"));
								continue;
							}
							else if(ret == 0)
							{
								global_debug_printlog(mylogevent,wxString::Format(" %s:%d ",fileName,line));
								if(className != ZL_EXP_NULL)
									global_debug_printlog(mylogevent,wxString::Format("%s:",className));
								if(funcName != ZL_EXP_NULL)
									global_debug_printlog(mylogevent,wxString::Format("%s",funcName));
								global_debug_printlog(mylogevent,wxString::Format("\n"));
								break;
							}
							else if(ret == -1)
							{
								global_debug_printlog(mylogevent,wxString::Format("%s",zenglApi_GetErrorString(VM_ARG)));
								break;
							}
						}
					}
					break;
				case 'r':
					{
						int arg = -1;
						int loc = -1;
						int pc = -1;
						int tmpPC;
						int ret;
						int size,i;
						int line = 0;
						char * fileName = ZL_EXP_NULL;
						ZL_EXP_BOOL hasBreaked = ZL_EXP_FALSE;
						ret = zenglApi_DebugGetTrace(VM_ARG,&arg,&loc,&pc,&fileName,&line,ZL_EXP_NULL,ZL_EXP_NULL);
						if(ret == 1)
						{
							zenglApi_DebugGetTrace(VM_ARG,&arg,&loc,&pc,&fileName,&line,ZL_EXP_NULL,ZL_EXP_NULL);
							pc++;
							size = zenglApi_DebugGetBreak(VM_ARG,-1,ZL_EXP_NULL,ZL_EXP_NULL,ZL_EXP_NULL,ZL_EXP_NULL,ZL_EXP_NULL,ZL_EXP_NULL,ZL_EXP_NULL);
							for(i=0;i<size;i++)
							{
								if(zenglApi_DebugGetBreak(VM_ARG,i,ZL_EXP_NULL,ZL_EXP_NULL,ZL_EXP_NULL,ZL_EXP_NULL,ZL_EXP_NULL,ZL_EXP_NULL,&tmpPC) == -1)
									continue;
								else if(pc == tmpPC)
								{
									hasBreaked = ZL_EXP_TRUE;
									break;
								}
							}
							if(!hasBreaked)
							{
								if(zenglApi_DebugSetBreakEx(VM_ARG,pc,ZL_EXP_NULL,ZL_EXP_NULL,1,ZL_EXP_FALSE) == -1)
									global_debug_printlog(mylogevent,wxString::Format("%s",zenglApi_GetErrorString(VM_ARG)));
								else
									exit = 1;
							}
							else
								exit = 1;
						}
						else if(ret == 0)
							exit = 1;
						else if(ret == -1)
							global_debug_printlog(mylogevent,wxString::Format("%s",zenglApi_GetErrorString(VM_ARG)));
					}
					break;
				case 'd':
					{
						int index;
						arg = global_getDebugArg(str,&start,ZL_EXP_TRUE);
						if(arg != ZL_EXP_NULL && strlen(arg) > 0 && global_isNumber(arg))
							index = atoi(arg);
						else
						{
							global_debug_printlog(mylogevent,wxString::Format("d命令缺少断点索引参数\n"));
							continue;
						}
						if(zenglApi_DebugDelBreak(VM_ARG,index) == -1)
							global_debug_printlog(mylogevent,wxString::Format("d命令error:无效的断点索引"));
						else
							global_debug_printlog(mylogevent,wxString::Format("删除断点成功"));
						global_debug_printlog(mylogevent,wxString::Format("\n"));
					}
					break;
				case 'D':
					{
						int index;
						char * filename = ZL_EXP_NULL;
						char * condition = ZL_EXP_NULL;
						char * log = ZL_EXP_NULL;
						int count;
						int line;
						ZL_EXP_BOOL disabled;
						arg = global_getDebugArg(str,&start,ZL_EXP_TRUE);
						if(arg != ZL_EXP_NULL && strlen(arg) > 0 && global_isNumber(arg))
							index = atoi(arg);
						else
						{
							global_debug_printlog(mylogevent,wxString::Format("D命令缺少断点索引参数\n"));
							continue;
						}
						if(zenglApi_DebugGetBreak(VM_ARG,index,&filename,&line,&condition,&log,&count,&disabled,ZL_EXP_NULL) == -1)
						{
							global_debug_printlog(mylogevent,wxString::Format("D命令error:无效的断点索引\n"));
							continue;
						}
						else
						{
							if(zenglApi_DebugSetBreak(VM_ARG,filename,line,condition,log,count,ZL_EXP_TRUE) == -1)
								global_debug_printlog(mylogevent,wxString::Format("D命令禁用断点error:%s",zenglApi_GetErrorString(VM_ARG)));
							else
								global_debug_printlog(mylogevent,wxString::Format("D命令禁用断点成功"));
							global_debug_printlog(mylogevent,wxString::Format("\n"));
						}
					}
					break;
				case 'C':
					{
						int index;
						char * newCondition;
						char * filename = ZL_EXP_NULL;
						char * condition = ZL_EXP_NULL;
						char * log = ZL_EXP_NULL;
						int count;
						int line;
						ZL_EXP_BOOL disabled;
						arg = global_getDebugArg(str,&start,ZL_EXP_TRUE);
						if(arg != ZL_EXP_NULL && strlen(arg) > 0 && global_isNumber(arg))
							index = atoi(arg);
						else
						{
							global_debug_printlog(mylogevent,wxString::Format("C命令缺少断点索引参数\n"));
							continue;
						}
						arg = global_getDebugArg(str,&start,ZL_EXP_FALSE);
						tmplen = arg != ZL_EXP_NULL ? strlen(arg) : 0;
						if(arg != ZL_EXP_NULL && tmplen > 0)
						{
							if(arg[tmplen - 1] != ';' && str_count < str_size - 1)
							{
								arg[tmplen] = ';';
								arg[tmplen+1] = STRNULL;
							}
							newCondition = arg;
						}
						else
							newCondition = ZL_EXP_NULL;
						if(zenglApi_DebugGetBreak(VM_ARG,index,&filename,&line,&condition,&log,&count,&disabled,ZL_EXP_NULL) == -1)
						{
							global_debug_printlog(mylogevent,wxString::Format("C命令error:无效的断点索引\n"));
							continue;
						}
						else
						{
							if(zenglApi_DebugSetBreak(VM_ARG,filename,line,newCondition,log,count,disabled) == -1)
								global_debug_printlog(mylogevent,wxString::Format("C命令设置条件断点error:%s",zenglApi_GetErrorString(VM_ARG)));
							else
								global_debug_printlog(mylogevent,wxString::Format("C命令设置条件断点成功"));
							global_debug_printlog(mylogevent,wxString::Format("\n"));
						}
					}
					break;
				case 'L':
					{
						int index;
						char * newLog;
						char * filename = ZL_EXP_NULL;
						char * condition = ZL_EXP_NULL;
						char * log = ZL_EXP_NULL;
						int count;
						int line;
						ZL_EXP_BOOL disabled;
						arg = global_getDebugArg(str,&start,ZL_EXP_TRUE);
						if(arg != ZL_EXP_NULL && strlen(arg) > 0 && global_isNumber(arg))
							index = atoi(arg);
						else
						{
							global_debug_printlog(mylogevent,wxString::Format("L命令缺少断点索引参数\n"));
							continue;
						}
						arg = global_getDebugArg(str,&start,ZL_EXP_FALSE);
						tmplen = arg != ZL_EXP_NULL ? strlen(arg) : 0;
						if(arg != ZL_EXP_NULL && tmplen > 0)
						{
							if(arg[tmplen - 1] != ';' && str_count < str_size - 1)
							{
								arg[tmplen] = ';';
								arg[tmplen+1] = STRNULL;
							}
							newLog = arg;
						}
						else
							newLog = ZL_EXP_NULL;
						if(zenglApi_DebugGetBreak(VM_ARG,index,&filename,&line,&condition,&log,&count,&disabled,ZL_EXP_NULL) == -1)
						{
							global_debug_printlog(mylogevent,wxString::Format("L命令error:无效的断点索引\n"));
							continue;
						}
						else
						{
							if(zenglApi_DebugSetBreak(VM_ARG,filename,line,condition,newLog,count,disabled) == -1)
								global_debug_printlog(mylogevent,wxString::Format("L命令设置日志断点error:%s",zenglApi_GetErrorString(VM_ARG)));
							else
								global_debug_printlog(mylogevent,wxString::Format("L命令设置日志断点成功"));
							global_debug_printlog(mylogevent,wxString::Format("\n"));
						}
					}
					break;
				case 'N':
					{
						int index;
						int newCount;
						char * filename = ZL_EXP_NULL;
						char * condition = ZL_EXP_NULL;
						char * log = ZL_EXP_NULL;
						int count;
						int line;
						ZL_EXP_BOOL disabled;
						arg = global_getDebugArg(str,&start,ZL_EXP_TRUE);
						if(arg != ZL_EXP_NULL && strlen(arg) > 0 && global_isNumber(arg))
							index = atoi(arg);
						else
						{
							global_debug_printlog(mylogevent,wxString::Format("N命令缺少断点索引参数\n"));
							continue;
						}
						arg = global_getDebugArg(str,&start,ZL_EXP_TRUE);
						if(arg != ZL_EXP_NULL && strlen(arg) > 0 && global_isNumber(arg))
							newCount = atoi(arg);
						else
						{
							global_debug_printlog(mylogevent,wxString::Format("N命令缺少断点次数参数\n"));
							continue;
						}
						if(zenglApi_DebugGetBreak(VM_ARG,index,&filename,&line,&condition,&log,&count,&disabled,ZL_EXP_NULL) == -1)
						{
							global_debug_printlog(mylogevent,wxString::Format("N命令error:无效的断点索引\n"));
							continue;
						}
						else
						{
							if(zenglApi_DebugSetBreak(VM_ARG,filename,line,condition,log,newCount,disabled) == -1)
								global_debug_printlog(mylogevent,wxString::Format("N命令设置断点次数error:%s",zenglApi_GetErrorString(VM_ARG)));
							else
								global_debug_printlog(mylogevent,wxString::Format("N命令设置断点次数成功"));
							global_debug_printlog(mylogevent,wxString::Format("\n"));
						}
					}
					break;
				case 's':
					zenglApi_DebugSetSingleBreak(VM_ARG,ZL_EXP_TRUE);
					exit = 1;
					break;
				case 'S':
					zenglApi_DebugSetSingleBreak(VM_ARG,ZL_EXP_FALSE);
					exit = 1;
					break;
				case 'c':
					exit = 1;
					break;
				case 'h':
					global_debug_printlog(mylogevent,wxString::Format(" p 调试变量信息 usage:p express\n"
						" P 将变量信息输出到InfoBox对话框 usage:P express\n"
						" b 设置断点 usage:b filename lineNumber\n"
						" B 查看断点列表 usage:B\n"
						" T 查看脚本函数的堆栈调用信息 usage:T\n"
						" d 删除某断点 usage:d breakIndex\n"
						" D 禁用某断点 usage:D breakIndex\n"
						" C 设置条件断点 usage:C breakIndex condition-express\n"
						" L 设置日志断点 usage:L breakIndex log-express\n"
						" N 设置断点次数 usage:N breakIndex count\n"
						" s 单步步入 usage:s\n"
						" S 单步步过 usage:S\n"
						" r 执行到返回 usage:r\n"
						" c 继续执行 usage:c\n"));
					break;
				default:
					global_debug_printlog(mylogevent,wxString::Format("无效的命令\n"));
					break;
				}//switch(command[0])
			}//if(glmainFrame->mythread2 != NULL && !glmainFrame->mythread2->IsPaused())
			else
			{
				break;
			}
		} //while(!exit)
		if(str != ZL_EXP_NULL)
			zenglApi_FreeMem(VM_ARG,str);
		return 0;
	}

	ZL_EXP_INT global_debug_conditionError(ZL_EXP_VOID * VM_ARG,ZL_EXP_CHAR * filename,ZL_EXP_INT line,ZL_EXP_INT breakIndex,ZL_EXP_CHAR * error)
	{
		ZL_EXP_CHAR * condition;
		zenglApi_DebugGetBreak(VM_ARG,breakIndex,ZL_EXP_NULL,ZL_EXP_NULL,&condition,ZL_EXP_NULL,ZL_EXP_NULL,ZL_EXP_NULL,ZL_EXP_NULL);
		myglStyle = MY_RICHTEXT_NORMAL_STYLE;
		myglStr = wxString::Format("\n%s [%d] <%d %s> error:%s\n",filename,line,breakIndex,condition,error);
		wxMyLogEvent event( wxEVT_MY_LOG_EVENT,wxID_ANY,myglStr,myglStyle);
		wxQueueEvent(glmainFrame->GetEventHandler(),event.Clone());
		return 0;
	}
}

bool global_checkTitleRepeat(wxString baseurl,wxString title,wxString catid)
{
	wxString detectUrl = "http://"+ baseurl +"/mydetectTitle.php";
	wxMyLogEvent eventforlog( wxEVT_MY_LOG_EVENT,wxID_ANY);
	struct curl_httppost *post=NULL;
	struct curl_httppost *last=NULL;
	curl = curl_easy_init();
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION ,1);
	curl_easy_setopt(curl, CURLOPT_USERAGENT ,"Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.2; .NET CLR 1.1.4322)");
	curl_easy_setopt(curl, CURLOPT_URL, detectUrl.c_str().AsChar());
	wxString mypost_mod_table;
	if(single_mod_modid >= "21")
		mypost_mod_table = single_mod_cat_table + "_" + single_mod_modid;
	else
		mypost_mod_table = single_mod_cat_table;
	curl_formadd(&post,&last,CURLFORM_COPYNAME,"_zlmy_detectTable",CURLFORM_COPYCONTENTS,mypost_mod_table.c_str().AsChar(), CURLFORM_END);
	curl_formadd(&post,&last,CURLFORM_COPYNAME,"_zlmy_detectTitle",CURLFORM_COPYCONTENTS,title.c_str().AsChar(), CURLFORM_END);
	curl_formadd(&post,&last,CURLFORM_COPYNAME,"_zlmy_catid",CURLFORM_COPYCONTENTS,catid.c_str().AsChar(), CURLFORM_END);
	curl_easy_setopt(curl, CURLOPT_HTTPPOST, post);
	curl_easy_setopt(curl, CURLOPT_HEADER, 0); 
	long timeout;
	glmainFrame->m_textForTimeOut->GetValue().ToLong(&timeout);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, (int)timeout);//设置超时时间
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, mywxcurl_string_write);
	CURLcode errcode = curl_easy_perform(curl);
	if(errcode != 0 || char_myglStr==NULL)
	{
		eventforlog.SetEventMsg(wxString::Format("error:%s...",curl_easy_strerror(errcode)),MY_RICHTEXT_RED_STYLE);
		wxQueueEvent(glmainFrame->GetEventHandler(),eventforlog.Clone());
		charlength = 0;
		curl_easy_cleanup(curl);
		curl_formfree(post);
		curl = NULL;
		char_myglTotalnum = 0;
		return false;
	}
	charlength = strlen(char_myglStr);
	if((myglStr = wxString(char_myglStr)) == "")
		myglStr = wxString::FromUTF8(char_myglStr);
	
	curl_easy_cleanup(curl);
	curl_formfree(post);
	curl = NULL;
	free(char_myglStr);
	char_myglStr = NULL;
	char_myglTotalnum = 0;
	return true;
}

inline bool IsSpace( IN wxChar ch )
{
    return ch == wxT(' ');

}

inline wxString TrimLeft( IN const wxString& str )
{
    const wxChar * pszStr = str.c_str();
    for ( int index = (int) str.Length() - 1; index >= 0; index -- )
    {
        if ( ! IsSpace( pszStr[index] ) )
        {
            return str.Left( index + 1 );
        }
    }
    return wxEmptyString;

}

inline wxString TrimRight( IN const wxString& str )
{
    const wxChar * pszStr = str.c_str();
    for ( int index = 0; index < (int) str.Length(); index ++ )
    {
        if ( ! IsSpace( pszStr[index] ) )
        {
            return str.Right( str.Length() - index );
        }
    }
    return wxEmptyString;

}

wxString TrimBoth( IN const wxString& str )
{
    return TrimLeft( TrimRight( str ) );

} 

void delScript(wxString * str)
{
	wxRegEx ex;
	wxString script_pattern = "<script.*>.*</script>";
	wxString script_pattern2 = "on(mousewheel|mouseover|click|load|onload|submit|focus|blur)=\"[^\"]*\"";
	if(ex.Compile(script_pattern,wxRE_ADVANCED | wxRE_ICASE))
	{
		ex.Replace(str,"");
	}
	if(ex.Compile(script_pattern2,wxRE_ADVANCED | wxRE_ICASE))
	{
		ex.Replace(str,"");
	}
}

bool CheckTitleMust(wxString title , wxString must)
{
	bool totalbooler=true;
	bool tmpbooler=false;
	wxString tmpstr;
	tmpstr = "";
	const wxChar * pszStr = must.c_str();
	int mustlength = (int)must.Length();
	for ( int index = 0; index < mustlength; index ++ )
    {
        if ( pszStr[index]==wxT('|'))
        {
			if(title.Find(tmpstr) == wxNOT_FOUND)
				tmpbooler |= false;
			else
				tmpbooler |= true;
			tmpstr = "";
        }
		else if( pszStr[index]==wxT('&'))
		{
			if(title.Find(tmpstr) == wxNOT_FOUND)
				tmpbooler |= false;
			else
				tmpbooler |= true;
			if(!(totalbooler && tmpbooler))
				return false;
			tmpstr = "";
			tmpbooler = false;
		}
		else
		{
			tmpstr += pszStr[index];
			if(index == mustlength - 1)
			{
				if(title.Find(tmpstr) == wxNOT_FOUND)
					tmpbooler |= false;
				else
					tmpbooler |= true;
				if(!(totalbooler && tmpbooler))
					return false;
				tmpstr = "";
				tmpbooler = false;
			}
		}
    }
	return true;
}

bool GetAllAreas()
{
	wxXmlDocument doc;
	wxMyLogEvent eventforlog(wxEVT_MY_LOG_EVENT,wxID_ANY);
	eventforlog.SetEventMsg("准备加载地区文件area.xml...\n",MY_RICHTEXT_GREEN_STYLE);
	wxQueueEvent(glmainFrame->GetEventHandler(),eventforlog.Clone());
	if (!doc.Load("area.xml"))
	{
		eventforlog.SetEventMsg("加载area.xml文件失败\n",MY_RICHTEXT_RED_STYLE);
		wxQueueEvent(glmainFrame->GetEventHandler(),eventforlog.Clone());
		return false;
	}
	wxXmlNode *root = doc.GetRoot();
	wxXmlNode *child = root->GetChildren();
	if(global_MyAreas.GetCount() > 0)
		global_MyAreas.Clear();
	while(child)
	{
		global_MyAreas.Add(new MyAreaObj(child->GetAttribute("name"),child->GetAttribute("areaid")));
		child = child->GetNext();
	}
	eventforlog.SetEventMsg(wxString::Format("加载地区文件成功，一共加载了%d个地区数据\n",global_MyAreas.GetCount()),MY_RICHTEXT_GREEN_STYLE);
	wxQueueEvent(glmainFrame->GetEventHandler(),eventforlog.Clone());
	return true;
}

wxString GetAreaID_NoRandom(wxString strArg)
{
	int areaCount = (int)global_MyAreas.GetCount();
	for(int i=0;i < areaCount;i++)
	{
		if(strArg.Find(global_MyAreas[i].areaName) != wxNOT_FOUND)
		{
			return global_MyAreas[i].areaID;
		}
	}
	return "0";
}

wxString GetAreaName_NoRandom(wxString strArg,wxString defaultRet)
{
	int areaCount = (int)global_MyAreas.GetCount();
	for(int i=0;i < areaCount;i++)
	{
		if(strArg.Find(global_MyAreas[i].areaName) != wxNOT_FOUND)
		{
			return global_MyAreas[i].areaName;
		}
	}
	return defaultRet;
}

wxString GetAreaID(wxString strArg)
{
	int areaCount = (int)global_MyAreas.GetCount();
	for(int i=0;i < areaCount;i++)
	{
		if(strArg.Find(global_MyAreas[i].areaName) != wxNOT_FOUND)
		{
			return global_MyAreas[i].areaID;
		}
	}
	return (wxString::Format("%d",GetRandomNum()%391 + 1));
}

int GetRandomNum()
{
	if(global_seed == 0)
	{
		time_t current_time;
		time(&current_time);
		global_seed = (unsigned int)current_time;
	}
	srand(global_seed);
	return (global_seed = rand());
}


char *global_rand_str(char *str,const int len)
{
   int i;
   for(i=0;i<len;++i)
       str[i]='a'+rand()%26;
   str[i]='\0';
   return str;
}