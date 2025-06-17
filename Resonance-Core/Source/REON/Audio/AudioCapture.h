#pragma once

#include <mmdeviceapi.h>
#include <endpointvolume.h>
#include <audioclient.h>
#include <comdef.h>
#include <iostream>
#include <vector>
#include <atlbase.h>
#include <propsys.h>
#include <propkey.h>
#include <functiondiscoverykeys_devpkey.h>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <thread>
#include <memory>
#include <audiopolicy.h>
#include "AudioRingBuffer.h"


namespace REON::AUDIO {
	class AudioCapture
	{
	public:
		AudioCapture(AudioRingBuffer& buffer);
		~AudioCapture();

		void StartCapture();
		void StopCapture();

		int SetAudioDevice(int index);

		std::vector<std::wstring> LoadAudioDevices(EDataFlow dataFlow);
	private:
		void CaptureLoop();

		CComPtr<IMMDeviceEnumerator> enumerator;
		CComPtr<IMMDeviceCollection> deviceCollection;
		CComPtr<IAudioMeterInformation> audioMeter;
		CComPtr<IMMDevice> device;
		CComPtr<IAudioClient> audioClient;
		CComPtr<IAudioCaptureClient> captureClient;
		WAVEFORMATEX* waveFormat = nullptr;
		AudioRingBuffer& buffer;

		UINT deviceCount;
		//BYTE* data;

		HRESULT hr;

		std::atomic<bool> capturing;
		std::thread captureThread;

	};
}