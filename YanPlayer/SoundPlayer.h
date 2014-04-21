#pragma once

#ifndef TY_NET_DIRECTSOUND_H
#define TY_NET_DIRECTSOUND_H

#include <mmsystem.h>
#include <dsound.h>
#include <math.h>
#pragma comment(lib,"dsound.lib")

#define MAX_SECOND_NUM         2//最大秒数（指播放缓冲区最多可以播几秒钟的数据）

#define SAFE_DELETE(p)  { if(p) { delete (p);     (p) = NULL; } }
#define SAFE_RELEASE(p) { if(p) { (p)->Release(); (p) = NULL; } }


class CSoundPlayer
{
public:
	CSoundPlayer(void);
	~CSoundPlayer(void);

	BOOL  Open(HWND hwnd, WORD channels, WORD bitsPerSample, DWORD samplesPerSec);
	VOID  Close(VOID);
	BOOL  Start(BYTE *audioBuf, DWORD bufSize);
	VOID  Stop(VOID);
	BOOL  CopyDataToBuf(BYTE *audioBuf, DWORD bufSize);
	BOOL  SetVolume(DWORD volume);
	BOOL  IsPlayed(VOID)CONST{ return m_bPlayed;} 

private:

	VOID  SetWaveFormat(WORD channels, WORD bitsPerSample, DWORD samplesPerSec);
	BOOL  ClearBuffer(VOID);
	BOOL  Play(VOID);
	BOOL  SetPlayCurPos(DWORD Pos);
	BOOL  CanWrite(VOID);   

	LPDIRECTSOUND        m_lpDirectSound;
	LPDIRECTSOUNDBUFFER  m_lpBuffer;
	DWORD                m_dwBufferSize;//播放缓冲区大小
	DWORD                m_dwLastPlayPos;//最新播放位置
	DWORD                m_dwBytesPerSec;//每秒钟字节数
	DWORD                m_dwWrapCount;//播放缓冲区已循环次数
	DWORD                m_dwHasWriteBytes;//已写入缓冲区的总字节数
	BOOL                 m_bPlayed;
	WAVEFORMATEX         m_waveFormat;
};

#endif
