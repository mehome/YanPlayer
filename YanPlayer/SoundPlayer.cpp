#include "stdafx.h"
#include "SoundPlayer.h"

CSoundPlayer::CSoundPlayer(VOID)
{
	m_lpDirectSound = NULL;
	m_lpBuffer      = NULL;
	m_dwLastPlayPos = 0;
	m_dwBufferSize  = 0;
	m_dwBytesPerSec = 0;
	m_dwWrapCount   = 0;
	m_dwHasWriteBytes = 0;
	m_bPlayed       = FALSE;
}
//---------------------------------------------------------------------------

CSoundPlayer::~CSoundPlayer(VOID)
{

}
//---------------------------------------------------------------------------

BOOL CSoundPlayer::SetVolume(DWORD volume)
{
	if(m_lpBuffer)
	{
		double vol;
		vol = -70*pow(10, (100.0 - volume)*2.0/100.0);
		if (m_lpBuffer->SetVolume((LONG)vol) == DS_OK)
		{
			return TRUE;
		}
	}

	return FALSE;
}
//---------------------------------------------------------------------------

VOID CSoundPlayer::Stop(VOID)
{
	if(m_lpBuffer)
	{
		m_lpBuffer->Stop();
		this->ClearBuffer();
	}

	m_bPlayed = FALSE;
	m_dwLastPlayPos = 0;
	m_dwWrapCount = 0;
	m_dwHasWriteBytes = 0;
}
//---------------------------------------------------------------------------
BOOL CSoundPlayer::SetPlayCurPos(DWORD pos)
{
	if(m_lpBuffer && pos <= m_dwLastPlayPos)
	{
		m_lpBuffer->SetCurrentPosition(pos);
		return TRUE;
	}

	return FALSE;
}
//---------------------------------------------------------------------------
VOID CSoundPlayer::SetWaveFormat(WORD channels, WORD bitsPerSample, DWORD samplesPerSec)
{
	ZeroMemory(&m_waveFormat, sizeof(WAVEFORMATEX));
	m_waveFormat.wFormatTag      = WAVE_FORMAT_PCM;
	m_waveFormat.nChannels       = channels;
	m_waveFormat.nSamplesPerSec  = samplesPerSec;
	m_waveFormat.wBitsPerSample  = bitsPerSample;
	m_waveFormat.nBlockAlign     = (short)m_waveFormat.nChannels * (m_waveFormat.wBitsPerSample / 8);
	m_waveFormat.nAvgBytesPerSec = m_waveFormat.nBlockAlign * m_waveFormat.nSamplesPerSec;
	m_waveFormat.cbSize          = sizeof(WAVEFORMATEX);
}

BOOL CSoundPlayer::Open(HWND hwnd, WORD channels, WORD bitsPerSample, DWORD samplesPerSec)
{
	if (!hwnd)
	{
		return FALSE;
	}

	HRESULT hr = 0;
	if (FAILED(hr = CoInitialize(NULL)))
	{
		return FALSE;
	}

	hr = DirectSoundCreate(NULL, &m_lpDirectSound, NULL);
	if(hr == DS_OK && m_lpDirectSound)
	{
		hr = m_lpDirectSound->SetCooperativeLevel(hwnd, DSSCL_PRIORITY);
		if(hr != DS_OK)
		{
			SAFE_RELEASE(m_lpDirectSound);
			return FALSE;
		}

		SetWaveFormat(channels, bitsPerSample, samplesPerSec);
		DSBUFFERDESC dsbdesc;
		ZeroMemory(&dsbdesc, sizeof(dsbdesc));
		dsbdesc.dwSize = sizeof(DSBUFFERDESC);
		dsbdesc.lpwfxFormat = &m_waveFormat;
		dsbdesc.dwFlags = DSBCAPS_CTRLVOLUME | DSBCAPS_GLOBALFOCUS | DSBCAPS_STICKYFOCUS 
			| DSBCAPS_LOCSOFTWARE | DSBCAPS_GETCURRENTPOSITION2;
		dsbdesc.dwBufferBytes = MAX_SECOND_NUM  * m_waveFormat.nAvgBytesPerSec;
		dsbdesc.guid3DAlgorithm = DS3DALG_DEFAULT;  

		hr = m_lpDirectSound->CreateSoundBuffer(&dsbdesc, &m_lpBuffer, NULL);
		if(hr != DS_OK)
		{
			SAFE_RELEASE(m_lpBuffer);
			SAFE_RELEASE(m_lpDirectSound);
			return FALSE;
		}
		m_dwBytesPerSec = m_waveFormat.nAvgBytesPerSec;
		m_dwBufferSize  = dsbdesc.dwBufferBytes;

		ClearBuffer();

		return TRUE;
	}

	return FALSE;
}
//---------------------------------------------------------------------------
BOOL CSoundPlayer::Play(VOID)
{
	if(!IsPlayed() && m_lpBuffer)
	{
		m_lpBuffer->Play(0, 0, DSBPLAY_LOOPING);
		m_bPlayed = TRUE;

		return TRUE;
	}

	return FALSE;
}
//---------------------------------------------------------------------------

VOID  CSoundPlayer::Close(VOID)
{
	Stop();
	SAFE_RELEASE(m_lpBuffer);
	SAFE_RELEASE(m_lpDirectSound);
	// Release COM
	CoUninitialize();
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
BOOL CSoundPlayer::Start(BYTE *audioBuf, DWORD bufSize)
{
	if (bufSize > m_dwBufferSize)
	{
		return FALSE;
	}

	this->SetPlayCurPos(0);
	if (!CopyDataToBuf(audioBuf, bufSize))
	{
		return FALSE;
	}

	return this->Play();
}
//---------------------------------------------------------------------------
BOOL CSoundPlayer::CanWrite(VOID)   
{     
	if (!m_lpBuffer)   
	{   
		return NULL;   
	}   

	DWORD dwCurPlay = 0; 
	m_lpBuffer->GetCurrentPosition(&dwCurPlay, NULL);   
	if (dwCurPlay < m_dwLastPlayPos)   
	{   
		m_dwWrapCount++;   
	}  
	m_dwLastPlayPos = dwCurPlay;   

	LONG lPlayPos  = (LONG)dwCurPlay + (LONG)m_dwWrapCount * (LONG)m_dwBufferSize;   
	LONG lWritePos = (LONG)m_dwHasWriteBytes;   
	if (lWritePos - lPlayPos > (LONG)m_dwBytesPerSec)   
	{   
		return FALSE;   
	} 

	return TRUE;   
} 

BOOL CSoundPlayer::CopyDataToBuf(BYTE *audioBuf, DWORD bufSize)
{
	if (!CanWrite())
	{
		return FALSE;
	}
	LPVOID pMem1;
	LPVOID pMem2;
	ZeroMemory(&pMem1,sizeof(pMem1));
	ZeroMemory(&pMem2,sizeof(pMem2));
	DWORD  dwSize1, dwSize2;
	ZeroMemory(&dwSize1,sizeof(dwSize1));
	ZeroMemory(&dwSize2,sizeof(dwSize2));
	if (FAILED(m_lpBuffer->Lock(m_dwHasWriteBytes % m_dwBufferSize, bufSize, &pMem1, &dwSize1, &pMem2, &dwSize2, 0)))   
	{   
		m_lpBuffer->Restore();   
		m_lpBuffer->Lock(m_dwHasWriteBytes % m_dwBufferSize, bufSize, &pMem1, &dwSize1, &pMem2, &dwSize2, 0);   
	}  
	if (pMem1)
	{
		CopyMemory(pMem1, audioBuf, dwSize1);
	}   
	if (pMem2)   
	{   
		CopyMemory(pMem2, audioBuf + dwSize1, dwSize2);   
	}   
	m_lpBuffer->Unlock(pMem1, dwSize1, pMem2, dwSize2);   
	m_dwHasWriteBytes += bufSize;   

	return TRUE;
}
//---------------------------------------------------------------------------
BOOL CSoundPlayer::ClearBuffer()
{
	if(!m_lpBuffer)
	{
		return FALSE;
	}

	VOID *buffer1 = NULL;
	VOID *buffer2 = NULL;
	DWORD bytes1 = 0;
	DWORD bytes2 = 0;

	this->SetPlayCurPos(0);

	m_lpBuffer->Lock(0, m_dwBufferSize, &buffer1, &bytes1, &buffer2, &bytes2, 0);

	if(buffer1)
	{
		FillMemory(buffer1, bytes1, m_waveFormat.wBitsPerSample == 8 ? 128 : 0);
		//            memset(buffer1,0,sizeof(buffer1));
		//            memset(&bytes1,0,sizeof(bytes1));
	}

	if(buffer2)
	{
		FillMemory(buffer2, bytes2, m_waveFormat.wBitsPerSample == 8 ? 128 : 0);
		//            memset(buffer2,0,sizeof(buffer2));
		//            memset(&bytes2,0,sizeof(bytes2));
	}

	m_lpBuffer->Unlock(buffer1, bytes1, buffer2, bytes2);

	return TRUE;
}