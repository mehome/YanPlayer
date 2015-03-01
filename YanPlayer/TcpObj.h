#pragma once
// ���ݳ���
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
static const int SIZE_FRAME_Q  = 50;          // ԭʼ֡����
static const int SIZE_4M       = 1024*1024*4;
static const int SIZE_5M       = 1025*1024*5;
static const int SIZE_35M      = 1024*1024*35;
static const int SIZE_70M      = 1024*1024*70;

enum MECode
{
  MERROR                = -1,      // ����ʧ��
  MERROR_NOERROR        = 0,       // �����ɹ�
  MERROR_PARAM_INVALID,            // �ղ���
  MERROR_URL_UNKNOWN,              // URL����ʶ��
  MERROR_HK             = 100,     // �����Ĵ������
  MERROR_HK_NO_DLL      = 101,     // ����-û�ҵ�hik.dll
  MERROR_HK_OPEN        = 130,     // ����-�����ļ�����
  MERROR_HK_WRITE       = 131,     // ����-д�ļ�����
  MERROR_BS             = 200,     // ��ɫ֮�ǵĴ������
  MERROR_DH             = 300,     // �󻪵Ĵ������
  MERROR_LC             = 400,     // �ɳ۵Ĵ������
  MERROR_XVID           = 400,     // �ɳ۵Ĵ������
	MERROR_LM							= 500,		 // �����忨(�ٿ�)�Ĵ������
  MERROR_DC_SOCKET      = 10000,   // SOCKET����
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

