#include <windows.h>

extern "C" {

#include "defs.h"
#include "pcm.h"
#include "rc.h"

void die(char *fmt, ...);

#define N_BUFS 8
#define BUF_SIZ 512

struct pcm pcm;

static int sound = 1;
static int stereo = 0;
static int samplerate = 44100 / 2;

rcvar_t pcm_exports[] =
{
	RCV_BOOL("sound", &sound),
	RCV_INT("stereo", &stereo),
	RCV_INT("samplerate", &samplerate),
	RCV_END
};

#ifdef WCE_USE_SEMAPHORE
static HANDLE semaph;
#else /* !WCE_USE_SEMAPHORE */
/* almost working and perhaps faster alternative */
typedef struct {
	LONG count;
	HANDLE event;
	DWORD timeout;
} poor_semaphore;
static poor_semaphore psemaph;
#endif /* WCE_USE_SEMAPHORE */
static HWAVEOUT wout;
static byte *buf;
static WAVEHDR hdr[N_BUFS];
static int curbuf;

static void CALLBACK woutcallback(HANDLE wout, UINT msg,
								  DWORD user, DWORD dw1, DWORD dw2)
{
	if (msg == WOM_DONE) {
#ifdef WCE_USE_SEMAPHORE
		ReleaseSemaphore((HANDLE) user, 1, NULL);
#else /* !WCE_USE_SEMAPHORE */
		if (InterlockedIncrement(&((poor_semaphore *) user)->count) == 1)
			SetEvent(((poor_semaphore *) user)->event);
#endif /* WCE_USE_SEMAPHORE */
	}
}

void pcm_init()
{
	WAVEFORMATEX format;
	int i;

	if (!sound) {
		pcm.hz = 11025;
		pcm.len = 4096;
		buf = (byte *) malloc(pcm.len);
		pcm.buf = buf;
		pcm.pos = 0;
	}

#ifdef WCE_USE_SEMAPHORE
	semaph = CreateSemaphore(NULL, N_BUFS, N_BUFS, NULL);
#else /* !WCE_USE_SEMAPHORE */
	psemaph.count = N_BUFS;
	psemaph.event = CreateEvent(NULL, TRUE, TRUE, NULL);
	psemaph.timeout = 1000 * N_BUFS * BUF_SIZ /
		((stereo ? 2 : 1) * samplerate) + 1;
#endif /* WCE_USE_SEMAPHORE */

	format.wFormatTag = WAVE_FORMAT_PCM;
	format.nChannels = stereo ? 2 : 1;
	format.nSamplesPerSec = samplerate;
	format.wBitsPerSample = 8; 
	format.nBlockAlign = format.nChannels * format.wBitsPerSample / 8; 
	format.nAvgBytesPerSec = format.nSamplesPerSec * format.nBlockAlign;
	format.cbSize = 0;
	if (waveOutOpen(&wout, WAVE_MAPPER, &format,
					(DWORD) woutcallback,
#ifdef WCE_USE_SEMAPHORE
					(DWORD) semaph,
#else /* !WCE_USE_SEMAPHORE */
					(DWORD) &psemaph, 
#endif /* WCE_USE_SEMAPHORE */
					CALLBACK_FUNCTION)
		!= MMSYSERR_NOERROR)
		die("cannot open wave out device\n");

	pcm.stereo = stereo;
	pcm.hz = samplerate;
	pcm.len = BUF_SIZ;
	buf = (byte *) malloc(pcm.len * N_BUFS);
	pcm.buf = buf;
	pcm.pos = 0;

	for (i = 0; i < N_BUFS; i++) {
		hdr[i].lpData = (LPSTR) (buf + pcm.len * i);
		hdr[i].dwBufferLength = pcm.len;
		hdr[i].dwBytesRecorded = 0;
		hdr[i].dwUser = 0;
		hdr[i].dwFlags = 0;
		hdr[i].dwLoops = 0;
		hdr[i].lpNext = NULL;
		hdr[i].reserved = 0;
		waveOutPrepareHeader(wout, &hdr[i], sizeof WAVEHDR);
	}

	curbuf = 0;
}

void pcm_close()
{
	int i;

	if (buf) free(buf);
	memset(&pcm, 0, sizeof pcm);

	waveOutReset(wout);
	for (i = 0; i < N_BUFS; i++)
		waveOutUnprepareHeader(wout, &hdr[i], sizeof WAVEHDR);
	waveOutClose(wout);
}

int pcm_submit()
{
	if (!sound || !wout) {
		pcm.pos = 0;
		return 0;
	}

	if (pcm.pos == pcm.len) {
		waveOutWrite(wout, &hdr[curbuf], sizeof WAVEHDR);
		
		curbuf = (curbuf + 1) % N_BUFS;
		pcm.buf = buf + pcm.len * curbuf;
		pcm.pos = 0;
	
#ifdef WCE_USE_SEMAPHORE
		WaitForSingleObject(semaph, INFINITE);
#else /* !WCE_USE_SEMAPHORE */
		if (InterlockedDecrement(&psemaph.count) == 0) {
			ResetEvent(psemaph.event);
			WaitForSingleObject(psemaph.event, psemaph.timeout);
		}
#endif /* WCE_USE_SEMAPHORE */
	}

	return 1;
}

}
