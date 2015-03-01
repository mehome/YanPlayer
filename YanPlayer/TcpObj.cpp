#include "StdAfx.h"
#include "TcpObj.h"
#include <Mmsystem.h>    
#pragma comment(lib, "Winmm.lib") 

CTcpObj::CTcpObj(LPCSTR lpIP, int iPort)
{
  m_bExit =  FALSE;
  m_byBuffer = NULL;
  m_byBuffer = new BYTE[SIZE_1M];
	
  SOCKADDR_IN siAddr;
  if ((lpIP != NULL) && (inet_addr(lpIP) != 0))   // connect net addr
  {
    m_hSock = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(m_hSock == SOCKET_ERROR)
    {
      throw (MECode)(MERROR_DC_SOCKET + 1);
    }
    siAddr.sin_family      = AF_INET;
    siAddr.sin_port        = htons(iPort);
    siAddr.sin_addr.s_addr = inet_addr(lpIP);
    if(connect(m_hSock, (SOCKADDR *)&siAddr, sizeof(SOCKADDR_IN)) == SOCKET_ERROR)
    {
      SAFE_DEL_SOCKET(m_hSock);
      int iErr = WSAGetLastError();
      throw (MECode)(MERROR_DC_SOCKET + iErr);
    }

    //设置缓冲区大小
    for( int i = 1; i < 50; i++ )
    {
       int recvbuflen = 8192*i*10; // 80K
       int len = sizeof(recvbuflen);
       setsockopt(m_hSock, SOL_SOCKET, SO_RCVBUF, (char *)&recvbuflen, len);
    }
  }
  else // bind local port
  {
    m_hListen = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(m_hListen == SOCKET_ERROR)
    {
      throw (MECode)(MERROR_DC_SOCKET + 1);
    }
    siAddr.sin_family = AF_INET;
    siAddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
    siAddr.sin_port = htons(iPort);	;
    if (0 != bind(m_hListen, (SOCKADDR*)&siAddr, sizeof(SOCKADDR)))
    {
      SAFE_DEL_SOCKET(m_hListen);
      int iErr = WSAGetLastError();
      throw (MECode)(MERROR_DC_SOCKET + iErr);
    }

    if (SOCKET_ERROR == listen(m_hListen, 10))
    {
      SAFE_DEL_SOCKET(m_hListen);
      int iErr = WSAGetLastError();
      throw (MECode)(MERROR_DC_SOCKET + iErr);
    }

    //接收Client连接
    SOCKADDR_IN addrClient;
    int nLen = sizeof(SOCKADDR);
    m_hSock = accept(m_hListen, (SOCKADDR *)&addrClient, &nLen);
    if (INVALID_SOCKET == m_hSock)
    {
      SAFE_DEL_SOCKET(m_hListen);
      SAFE_DEL_SOCKET(m_hSock);
      int iErr = WSAGetLastError();
      throw (MECode)(MERROR_DC_SOCKET + iErr);
    }
  }

  //m_dwPreBitsRateStatTm = timeGetTime();
  m_sAddr = lpIP;
  m_iPort = iPort;
}


CTcpObj::~CTcpObj(void)
{
	SAFE_DEL_SOCKET(m_hSock);
	SAFE_DEL_SOCKET(m_hListen);
}

void
CTcpObj::SetSocketOpt( BOOL bSet )
{
  if( (m_hSock != INVALID_SOCKET) && bSet )
  {
     int recvbuflen = 8192;
     int len = sizeof(recvbuflen);
     setsockopt(m_hSock, SOL_SOCKET, SO_RCVBUF, (char *)&recvbuflen, len);
  }
}

int
CTcpObj::WriteData(LPVOID lpData, int iLen)
{
  fd_set fdsend;
  timeval timeout = {0, 500};
  int iPos = 0;
  while (!m_bExit && iPos < iLen)
  {
    FD_ZERO(&fdsend);
    FD_SET(m_hSock, &fdsend);
    select(NULL, NULL, &fdsend, NULL, &timeout);
    if (!FD_ISSET(m_hSock, &fdsend))
    {
      continue;
    }
    int iSend = send(m_hSock, (LPCSTR)lpData + iPos, iLen - iPos, 0);
    if (iSend <= 0) 
    {
      //TRACE("Error CGeTCP::WriteData, errorcode: %d\n", WSAGetLastError());
      //SAFE_DEL_SOCKET(m_hSock);
      break;
    }
    iPos += iSend;
  }

  return iPos;
}

LPCSTR
CTcpObj::ReadData(WORD* pwtype, UINT* pnActualLen, UINT* pnLen, UINT* pnFrameCount/* =NULL */, LPVOID lpVoid/* =NULL */)
{
  DWORD dwCurTm = timeGetTime();
  *pnActualLen = 0;
  int iRecv = 0;
  while (!m_bExit)
  {
    if (*pnLen > SIZE_1M)
    {
      break;
    }

    fd_set fdRecv;
    FD_ZERO(&fdRecv);
    FD_SET(m_hSock, &fdRecv);
    timeval timeout = {5, 0};
    int iRet = select(0, &fdRecv, NULL, NULL, &timeout);
    if (iRet < 0)
    {
      break;
    }

    if (!FD_ISSET(m_hSock, &fdRecv))
    {
      if (timeGetTime() > dwCurTm + 1000)
      {
        break;
      }
      else
      {
        continue;
      }
    }

    iRecv = recv(m_hSock, (PSZ)m_byBuffer + *pnActualLen, *pnLen - *pnActualLen, 0);
    if (iRecv <= 0)
    {
      int iLastErr = WSAGetLastError();
      *pwtype = 0xff;
      break;
    }
    *pnActualLen += iRecv;
    if ((*pnActualLen >= *pnLen))
    {
      break;
    }
  }

  return (LPCSTR)m_byBuffer;
}

HANDLE
CTcpObj::GetHandle()
{
  return (HANDLE)m_hSock;
}