// Workaround to use OGG audio in CWin32MP3

#include "CWin32MP3.h"
#include "CWin32Platform.h"
#include "CSoundAnalysis.h"

#include <vorbis/codec.h>
#include <vorbis/vorbisfile.h>

struct Win32OGG_DecoderS
{
	FILE* fp;
	CDecryptBaseClass* dctx;
};

size_t Win32OGG_Read(void* dst, size_t count, size_t size, void* fh)
{
	Win32OGG_DecoderS* decoder = (Win32OGG_DecoderS*)fh;
	size_t readed = fread(dst, count, size, decoder->fp);

	decoder->dctx->decryptBlck(dst, readed);

	return readed;
}

int Win32OGG_Seek(void *fh, ogg_int64_t to, int type)
{
	Win32OGG_DecoderS* decoder = (Win32OGG_DecoderS*)fh;
	CDecryptBaseClass* dctx = decoder->dctx;
	int header_size = dctx->m_header_size;
	int pos = 0;
	int retval;
	
	if(type == SEEK_SET)
		retval = fseek(decoder->fp, to + header_size, SEEK_SET);
	else
		retval = fseek(decoder->fp, to, type);

	if(retval >= 0)
		dctx->gotoOffset(ftell(decoder->fp) - header_size);

	return retval;
}

long Win32OGG_Tell(void* fh)
{
	Win32OGG_DecoderS* decoder = (Win32OGG_DecoderS*)fh;

	return ftell(decoder->fp) - decoder->dctx->m_header_size;
}

ov_callbacks Win32OGG_Callbacks = {&Win32OGG_Read, &Win32OGG_Seek, NULL, &Win32OGG_Tell};
//ov_callbacks Win32OGG_Callbacks = {&Win32OGG_Read, NULL, NULL, NULL};

bool SoundAnalysis_OGG(const char* path, sSoundAnalysisData* analysis_data)
{
	CDecryptBaseClass decrypter;
	Win32OGG_DecoderS decoder;
	OggVorbis_File vf;
	FILE* fp;
	
	fp = fopen(path, "rb");

	if(fp == NULL)
		return false;

	// Setup decrypter and variables
	if(CPFInterface::getInstance().platform().useEncryption())
	{
		u8 hdr[16];
		fread(hdr, 1, 16, fp);
		decrypter.decryptSetup((const u8*)path, hdr);
		fseek(fp, decrypter.m_header_size, SEEK_SET);
	}

	decoder.fp = fp;
	decoder.dctx = &decrypter;

	if(ov_open_callbacks(&decoder, &vf, NULL, 0, Win32OGG_Callbacks) < 0)
	{
		fclose(fp);
		return false;
	}

	vorbis_info* vi = ov_info(&vf, -1);
	ogg_int64_t pcm_size = ov_pcm_total(&vf, -1);

	analysis_data->m_bitRate = vi->bitrate_nominal / 1000;
	analysis_data->m_bitRateType = vi->bitrate_lower != vi->bitrate_upper ? eBITRATE_TYPE_VBR : eBITRATE_TYPE_CBR;
	analysis_data->m_samplingRate = vi->rate;
	analysis_data->m_totalTime = pcm_size * 1000 / vi->rate;

	ov_clear(&vf);
	fclose(fp);

	return true;
}

bool CWin32MP3::loadOGG(const char* name)
{
	char* PCMOut;
	Win32OGG_DecoderS decoder;
	OggVorbis_File vf;
	FILE* fp;

	// Open
	PCMOut = NULL;
	fp = fopen(name, "rb");

	if(fp == NULL)
		return false;

	// Setup decrypter and variables
	if(CWin32Platform::g_useDecryption)
	{
		u8 hdr[16];
		fread(hdr, 1, 16, fp);

		m_decrypter.decryptSetup((const u8*)name, hdr);
		fseek(fp, m_decrypter.m_header_size, SEEK_SET);
	}

	decoder.fp = fp;
	decoder.dctx = &m_decrypter;

	if(ov_open_callbacks(&decoder, &vf, NULL, 0, Win32OGG_Callbacks) < 0)
	{
		fclose(fp);
		return false;
	}

	vorbis_info* vi = ov_info(&vf, -1);
	m_format.wFormatTag     = WAVE_FORMAT_PCM;
	m_format.nChannels      = vi->channels;
	m_format.nSamplesPerSec = vi->rate;
	m_format.wBitsPerSample = 16;
	m_format.nBlockAlign    = m_format.nChannels * (m_format.wBitsPerSample / 8);
	m_format.nAvgBytesPerSec = m_format.nSamplesPerSec * m_format.nBlockAlign;
	m_format.cbSize         = 0;
	m_begin = m_end = NULL;
	m_totalSize = 0;

	ogg_int64_t PCMSize = ov_pcm_total(&vf, -1) * sizeof(s16) * vi->channels;
	int current_section = 0;
	PCMOut = new char[PCMSize];
	char* PCMPos = PCMOut;
	char* pcm_eof = PCMOut + PCMSize;
	size_t memset_size = min(PCMSize, 2048);

	memset(pcm_eof - memset_size, 0, memset_size);

	while(true)
	{
		int x = min(pcm_eof - PCMPos, 4096);
		long ret = ov_read(&vf, PCMPos, x, 0, 2, 1, &current_section);
		
		if(ret == 0 || x < 4096)
		{
			PCMPos += ret;
			break;
		}
		else if(ret < 0)
		{
			delete[] PCMOut;
			ov_clear(&vf);
			fclose(fp);

			DEBUG_PRINT("Invalid OGG. Cannot decode");
			return false;
		}
		else
			PCMPos += ret;
	}

	m_totalSize = PCMSize;
	m_is_ogg = true;
	m_ogg_buffer = PCMOut;

	ov_clear(&vf);
	fclose(fp);

	return true;
}