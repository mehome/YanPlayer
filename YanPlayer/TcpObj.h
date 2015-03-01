#pragma once
// 数据长度
const static int SIZE_4K       = 1024 * 4;
const static int SIZE_16K      = 1024 * 16;
const static int SIZE_32K      = 1024 * 32;
static const int SIZE_40K      = 1024 * 40;   // 40k
static const int SIZE_64K      = 1024 * 64;   // 64k
static const int SIZE_80K      = 1024 * 80;   // 80k
static const int SIZE_128K     = 1024 * 128;  // 128k
static const int SIZE_256K     = 1024 * 256;  // 256K
static const int SIZE_512K     = 1024 * 512;  // 512K
static const int SIZE_1M       = 1024 * 1024; // 1M
static const int SIZE_FRAME_Q  = 50;          // 原始帧缓冲
static const int SIZE_4M       = 1024*1024*4;
static const int SIZE_5M       = 1025*1024*5;
static const int SIZE_35M      = 1024*1024*35;
static const int SIZE_70M      = 1024*1024*70;

enum MECode
{
  MERROR                = -1,      // 操作失败
  MERROR_NOERROR        = 0,       // 操作成功
  MERROR_PARAM_INVALID,            // 空参数
  MERROR_URL_UNKNOWN,              // URL不可识别
  MERROR_HK             = 100,     // 海康的错误代码
  MERROR_HK_NO_DLL      = 101,     // 海康-没找到hik.dll
  MERROR_HK_OPEN        = 130,     // 海康-开打文件出错
  MERROR_HK_WRITE       = 131,     // 海康-写文件出错
  MERROR_BS             = 200,     // 蓝色之星的错误代码
  MERROR_DH             = 300,     // 大华的错误代码
  MERROR_LC             = 400,     // 郎驰的错误代码
  MERROR_XVID           = 400,     // 郎驰的错误代码
	MERROR_LM							= 500,		 // 立迈板卡(百科)的错误代码
  MERROR_DC_SOCKET      = 10000,   // SOCKET错误
  MERROR_DC_FILE        = 11000,
};

#define SAFE_DEL_HANDLE(h)      { if (h != INVALID_HANDLE_VALUE) {CloseHandle(h); h = INVALID_HANDLE_VALUE;}}
#define SAFE_DEL_SOCKET(s)      { if (s != INVALID_SOCKET) { closesocket(s); s = INVALID_SOCKET;}}

class CTcpObj
{
public:
	CTcpObj(LPCSTR lpIp, int iPort);
	~CTcpObj(void);

	CString m_sAddr;
	int m_iPort;
	BOOL m_bExit;

	void SetSocketOpt( BOOL bSet );
	int  WriteData(LPVOID lpData, int iLen);
	LPCSTR ReadData(WORD* pwtype, UINT* pnActualLen, UINT* pnLen, UINT* pnFrameCount =NULL , LPVOID lpVoid =NULL );
	HANDLE GetHandle();

private:
  SOCKET m_hSock;
  BYTE*  m_byBuffer;
  SOCKET m_hListen;
};

