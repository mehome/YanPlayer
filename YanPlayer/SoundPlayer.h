#pragma once

#ifndef TY_NET_DIRECTSOUND_H
#define TY_NET_DIRECTSOUND_H

#include <mmsystem.h>
#include <dsound.h>
#include <math.h>
#pragma comment(lib,"dsound.lib")

#define MAX_SECOND_NUM         2//���������ָ���Ż����������Բ������ӵ����ݣ�

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
	DWORD                m_dwBufferSize;//���Ż�������С
	DWORD                m_dwLastPlayPos;//���²���λ��
	DWORD                m_dwBytesPerSec;//ÿ�����ֽ���
	DWORD                m_dwWrapCount;//���Ż�������ѭ������
	DWORD                m_dwHasWriteBytes;//��д�뻺���������ֽ���
	BOOL                 m_bPlayed;
	WAVEFORMATEX         m_waveFormat;
};

#endif
