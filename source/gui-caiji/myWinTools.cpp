#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>

int myGetLenWC2GBK(wchar_t * ws)
{
	int bad;
	return WideCharToMultiByte(CP_ACP, 0, ws, -1, NULL, 0, "", &bad);
}

int myConvWC2GBK(wchar_t * ws,char * buf,int buf_size)
{
    int bad;

    if ( !WideCharToMultiByte(CP_ACP, 0, ws, -1, buf, buf_size, "", &bad) )
    {
        return 0;
    }

    return 1;
}

 /* 
    函数名：myGetFileSize(char * strFileName)  
    功能：获取指定文件的大小 
    参数： 
        strFileName (char *)：文件名 
    返回值： 
        size (int)：文件大小 
 */  
int myGetFileSize(char * strFileName)   
{  
    FILE * fp = fopen(strFileName, "r"); 
	int size;
    fseek(fp, 0L, SEEK_END);  
    size = ftell(fp);  
    fclose(fp);  
    return size;  
}
