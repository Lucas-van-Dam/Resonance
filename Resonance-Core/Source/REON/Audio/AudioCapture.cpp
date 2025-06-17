#include <reonpch.h>
#include "AudioCapture.h"

namespace REON::AUDIO {
	AudioCapture::AudioCapture(AudioRingBuffer& buffer) : buffer(buffer)
	{
		hr = CoInitialize(nullptr);
		if (FAILED(hr)) {
			std::cout << "Initialize com library failed" << std::endl;
		}

		LoadAudioDevices(eAll);

		SetAudioDevice(0);
	}

	AudioCapture::~AudioCapture()
	{
		StopCapture();

		if (captureThread.joinable()) {
			try {
				captureThread.join();
			}
			catch (const std::system_error& e) {
				std::cerr << "Error joining capture thread: " << e.what() << std::endl;
			}
		}

		CoUninitialize();
	}

	void AudioCapture::StartCapture()
	{
		if (capturing) return;

		if (captureClient) {
			capturing = true;
			captureThread = std::thread(&AudioCapture::CaptureLoop, this);
			//captureThread = std::thread([this]() {
			//	while (capturing) {
			//		std::this_thread::sleep_for(std::chrono::milliseconds(100));
			//		std::cout << "capturing data..." << std::endl;
			//	}
			//	});
		}
	}

	void AudioCapture::StopCapture()
	{
		if (capturing) {
			capturing = false;
			if (captureThread.joinable()) {
				try {
					captureThread.join();
				}
				catch (const std::system_error& e) {
					std::cerr << "Error joining capture thread: " << e.what() << std::endl;
				}
			}
		}
	}

	void AudioCapture::CaptureLoop()
	{
		HRESULT result = CoInitializeEx(NULL, COINIT_MULTITHREADED); // Initialize COM for the thread

		while (capturing) {
			UINT32 packetLength = 0;
			result = captureClient->GetNextPacketSize(&packetLength);
			if (FAILED(result)) {
				std::cerr << "Failed to get next packet size: " << std::hex << result << std::endl;
				continue; // Continue to check for more data
			}

			while (packetLength != 0 && capturing) {
				BYTE* data = nullptr;
				UINT32 numFramesAvailable = 0;
				DWORD flags = 0;


				// Get the audio data from the capture client
				result = captureClient->GetBuffer(&data, &numFramesAvailable, &flags, NULL, NULL);
				if (FAILED(result)) {
					std::cerr << "Failed to get buffer: " << std::hex << result << std::endl;
					break; // Exit the inner loop to handle next packet size
				}



				// Check if the format is float (32-bit PCM)
				if (waveFormat->wFormatTag == WAVE_FORMAT_IEEE_FLOAT) {
					float* floatData = reinterpret_cast<float*>(data);

					std::vector<float> channelData(numFramesAvailable);

					// Extract only the desired channel (e.g., left channel: channelIndex = 0)
					int channelIndex = 0; // Set to the desired channel (0 for left, 1 for right in stereo)
					for (size_t frame = 0; frame < numFramesAvailable; ++frame) {
						channelData[frame] = floatData[frame * waveFormat->nChannels + channelIndex];
					}
					buffer.push_bulk(channelData.data(), numFramesAvailable);
				}
				// For non-float formats (e.g., 32-bit int), convert to float
				else if (waveFormat->wBitsPerSample == 32) {
					int* intData = reinterpret_cast<int*>(data);
					float scaleFactor = 1.0f / static_cast<float>(INT32_MAX); // Scale to [-1, 1]

					std::vector<float> channelData(numFramesAvailable);

					// Extract only the desired channel
					int channelIndex = 0; // Set to the desired channel (0 for left, 1 for right in stereo)
					for (size_t frame = 0; frame < numFramesAvailable; ++frame) {
						channelData[frame] = intData[frame * waveFormat->nChannels + channelIndex] * scaleFactor;
					}

					buffer.push_bulk(channelData.data(), numFramesAvailable);
				}
				else {
					std::cerr << "Unsupported audio format: " << waveFormat->wBitsPerSample << " bits per sample" << std::endl;
					break;
				}



				// Release the buffer
				result = captureClient->ReleaseBuffer(numFramesAvailable);
				if (FAILED(result)) {
					std::cerr << "Failed to release buffer: " << std::hex << result << std::endl;
					break; // Exit the inner loop if buffer release fails
				}

				// Get the next packet size
				result = captureClient->GetNextPacketSize(&packetLength);
				if (FAILED(result)) {
					std::cerr << "Failed to get next packet size: " << std::hex << result << std::endl;
					break; // Exit the inner loop if fetching the next packet size fails
				}
			}
		}

		CoUninitialize(); // Uninitialize COM at the end
	}

	std::vector<std::wstring> AudioCapture::LoadAudioDevices(EDataFlow dataFlow)
	{
		hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
		bool comInitialized = SUCCEEDED(hr) || hr == RPC_E_CHANGED_MODE;
		if (!comInitialized) {
			std::cerr << "Failed to initialize COM library: " << std::hex << hr << std::endl;
			return {};
		}

		if (enumerator) {
			enumerator = nullptr;
		}

		hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL, IID_PPV_ARGS(&enumerator));

		if (FAILED(hr)) {
			std::cerr << "Failed to create device enumerator: " << std::hex << hr << std::endl;
			if (comInitialized) CoUninitialize();
			return {};
		}
		if (deviceCollection) {
			deviceCollection = nullptr;
		}
		// Enumerate active audio endpoints
		hr = enumerator->EnumAudioEndpoints(dataFlow, DEVICE_STATE_ACTIVE, &deviceCollection);

		if (FAILED(hr)) {
			std::cerr << "Failed to enumerate audio endpoints: " << std::hex << hr << std::endl;
			if (comInitialized) CoUninitialize();
			return {};
		}

		hr = deviceCollection->GetCount(&deviceCount);

		if (FAILED(hr)) {
			std::cerr << "Failed to get device count: " << std::hex << hr << std::endl;
			if (comInitialized) CoUninitialize();
			return {};
		}

		std::vector<std::wstring> deviceInfo(deviceCount);

		for (UINT i = 0; i < deviceCount; i++) {
			CComPtr<IMMDevice> device;
			hr = deviceCollection->Item(i, &device);

			if (FAILED(hr)) {
				std::cerr << "Failed to get device: " << std::hex << hr << std::endl;
				continue;
			}
			// Get device properties
			CComPtr<IPropertyStore> propertyStore;
			hr = device->OpenPropertyStore(STGM_READ, &propertyStore);

			if (FAILED(hr)) {
				std::cerr << "Failed to open property store: " << std::hex << hr << std::endl;
				continue;
			}

			PROPVARIANT varName;
			PropVariantInit(&varName);
			hr = propertyStore->GetValue(PKEY_Device_FriendlyName, &varName);

			if (SUCCEEDED(hr)) {
				std::wstringstream ss;
				ss << L"Device " << i << L": " << varName.pwszVal;
				deviceInfo[i] = ss.str();
				PropVariantClear(&varName);
			}
		}

		if (comInitialized) CoUninitialize();
		return deviceInfo;
	}


	int AudioCapture::SetAudioDevice(int index)
	{
		StopCapture();

		std::cout << "getting device at index: " << index << std::endl;
		if (device) {
			device = nullptr;
		}

		if (waveFormat) {
			CoTaskMemFree(waveFormat);
			waveFormat = nullptr;
		}

		if (audioClient) {
			audioClient->Stop(); // Stop any ongoing audio capture
			audioClient = nullptr;
		}

		if (captureClient) {
			captureClient = nullptr;
		}

		if (deviceCollection) {
			deviceCollection = nullptr;
		}

		hr = enumerator->EnumAudioEndpoints(eAll, DEVICE_STATE_ACTIVE, &deviceCollection);
		if (FAILED(hr)) {
			std::cerr << "Failed to enumerate audio endpoints: " << std::hex << hr << std::endl;
			return -1;
		}

		hr = deviceCollection->GetCount(&deviceCount);
		if (FAILED(hr) || deviceCount == 0) {
			std::cerr << "No active audio capture devices found: " << std::hex << hr << std::endl;
			return -1;
		}

		if (index < 0) {
			index = deviceCount - 1;
		}
		else if (index >= deviceCount) {
			index = 0;
		}

		hr = deviceCollection->Item(index, &device);
		if (FAILED(hr)) {
			std::cerr << "Failed to get audio device at index " << index << ": " << std::hex << hr << std::endl;
			return SetAudioDevice(index + 1);
		}

		hr = device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, NULL, (void**)&audioClient);
		if (FAILED(hr)) {
			std::cerr << "Failed to activate IAudioClient: " << std::hex << hr << std::endl;
			return SetAudioDevice(index + 1);
		}

		hr = audioClient->GetMixFormat(&waveFormat);
		if (FAILED(hr)) {
			std::cerr << "Failed to get mix format: " << std::hex << hr << std::endl;
			return SetAudioDevice(index + 1);
		}

		WAVEFORMATEX* closestFormat = nullptr;
		hr = audioClient->IsFormatSupported(AUDCLNT_SHAREMODE_SHARED, waveFormat, &closestFormat);
		if (hr == S_FALSE && closestFormat != nullptr) {
			// Use the closest format if the requested format is not supported
			CoTaskMemFree(waveFormat);
			waveFormat = closestFormat;
		}
		else if (FAILED(hr)) {
			std::cerr << "Format not supported: " << std::hex << hr << std::endl;
			CoTaskMemFree(waveFormat);
			waveFormat = nullptr;
			return SetAudioDevice(index + 1);
		}

		hr = audioClient->Initialize(
			AUDCLNT_SHAREMODE_SHARED,
			0,
			10000000, // Buffer duration in 100-nanosecond units
			0,
			waveFormat,
			NULL
		);
		if (FAILED(hr)) {
			std::cerr << "Failed to initialize audio client: " << std::hex << hr << std::endl;
			CoTaskMemFree(waveFormat);
			waveFormat = nullptr;
			return SetAudioDevice(index + 1);
		}

		hr = audioClient->GetService(__uuidof(IAudioCaptureClient), (void**)&captureClient);
		if (FAILED(hr)) {
			std::cerr << "Failed to get IAudioCaptureClient: " << std::hex << hr << std::endl;
			CoTaskMemFree(waveFormat);
			waveFormat = nullptr;
			return SetAudioDevice(index + 1);
		}

		hr = audioClient->Start();
		if (FAILED(hr)) {
			std::cerr << "Failed to start audio capture: " << std::hex << hr << std::endl;
			CoTaskMemFree(waveFormat);
			waveFormat = nullptr;
			return SetAudioDevice(index + 1);
		}

		std::cout << "Audio device at index " << index << " is now active." << std::endl;

		StartCapture();

		return index;
	}

	//SOUND_API_C std::vector<float> AudioCapture::GetAudioLevels()
	//{
	//	std::vector<float> values;
	//	for (UINT i = 0; i < deviceCount; i++) {
	//		CComPtr<IMMDevice> device;
	//		hr = deviceCollection->Item(i, &device);
	//		if (FAILED(hr) || !device) {
	//			std::cerr << "Failed to get device: " << std::hex << hr << std::endl;
	//			values.push_back(-1);
	//			continue;
	//		}
	//
	//		// Get the audio meter information interface
	//		CComPtr<IAudioMeterInformation> audioMeter;
	//		hr = device->Activate(__uuidof(IAudioMeterInformation), CLSCTX_ALL, NULL, (void**)&audioMeter);
	//		if (FAILED(hr) || !audioMeter) {
	//			std::cerr << "Failed to get IAudioMeterInformation interface: " << std::hex << hr << std::endl;
	//			values.push_back(-1);
	//			continue;
	//		}
	//
	//		// Get the peak audio level
	//		float peakLevel = 0.0f;
	//		hr = audioMeter->GetPeakValue(&peakLevel);
	//		if (FAILED(hr)) {
	//			std::cerr << "Failed to get peak value: " << std::hex << hr << std::endl;
	//			values.push_back(-1);
	//			continue;
	//		}
	//
	//		values.push_back(peakLevel);
	//	}
	//	return values;
	//}

	//SOUND_API_C void AudioCapture::GetAudio()
	//{
	//
	//	// Get the audio meter information interface
	//	CComPtr<IAudioMeterInformation> audioMeter;
	//	hr = device->Activate(__uuidof(IAudioMeterInformation), CLSCTX_ALL, NULL, (void**)&audioMeter);
	//	if (FAILED(hr) || !audioMeter) {
	//		std::cerr << "Failed to get IAudioMeterInformation interface: " << std::hex << hr << std::endl;
	//	}
	//
	//	// Get the peak audio level
	//	float peakLevel = 0.0f;
	//	hr = audioMeter->GetPeakValue(&peakLevel);
	//	if (FAILED(hr)) {
	//		std::cerr << "Failed to get peak value: " << std::hex << hr << std::endl;
	//		peakLevel = -1.0f;
	//	}
	//
	//	//std::cout << peakLevel << std::endl;
	//
	//	UINT32 packetLength = 0;
	//	hr = captureClient->GetNextPacketSize(&packetLength);
	//	if (FAILED(hr)) {
	//		std::cerr << "Failed to get next packet size: " << std::hex << hr << std::endl;
	//		return;
	//	}
	//
	//	while (packetLength != 0) {
	//		BYTE* data;
	//		UINT32 numFramesAvailable;
	//		DWORD flags;
	//
	//		// Get the audio data from the capture client
	//		hr = captureClient->GetBuffer(&data, &numFramesAvailable, &flags, NULL, NULL);
	//		if (FAILED(hr)) {
	//			std::cerr << "Failed to get buffer: " << std::hex << hr << std::endl;
	//			break;
	//		}
	//
	//		auto converted = ExtractAudioData(data, numFramesAvailable * sizeof(int32_t));
	//
	//
	//		//// Print raw BYTE data
	//		//std::cout << "Raw audio data (first 256 bytes):" << std::endl;
	//		//for (UINT32 i = 0; i < (((static_cast<size_t>(numFramesAvailable * sizeof(int32_t))) < (static_cast<size_t>(256))) ? (static_cast<size_t>(numFramesAvailable * sizeof(int32_t))) : (static_cast<size_t>(256))); ++i) {
	//		//	if (i % 16 == 0) std::cout << std::endl; // Newline every 16 bytes for readability
	//		//	std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)data[i] << ' ';
	//		//}
	//		//std::cout << std::dec << std::endl; // Switch back to decimal format
	//
	//		// Process the data here (for now, we'll just print the number of frames)
	//		//std::cout << "Captured " << "" << std::endl;
	//
	//		// Release the buffer
	//		hr = captureClient->ReleaseBuffer(numFramesAvailable);
	//		if (FAILED(hr)) {
	//			std::cerr << "Failed to release buffer: " << std::hex << hr << std::endl;
	//			break;
	//		}
	//
	//		// Get the next packet size
	//		hr = captureClient->GetNextPacketSize(&packetLength);
	//		if (FAILED(hr)) {
	//			std::cerr << "Failed to get next packet size: " << std::hex << hr << std::endl;
	//			break;
	//		}
	//	}
	//}

	//SOUND_API_C std::vector<int32_t> AudioCapture::ExtractAudioData(BYTE* audioBuffer, size_t bufferSize) {
	//	// Calculate number of 32-bit samples
	//	size_t numSamples = bufferSize / sizeof(int32_t);
	//
	//	// Create a vector to hold the extracted audio data
	//	std::vector<int32_t> audioData(numSamples);
	//
	//	// Cast the BYTE* to int32_t*
	//	int32_t* pData = reinterpret_cast<int32_t*>(audioBuffer);
	//
	//	// Copy data from buffer to vector
	//	for (size_t i = 0; i < numSamples; ++i) {
	//		audioData[i] = pData[i];
	//	}
	//
	//	// Debug: Print first few values of audioData
	//	std::cout << "Extracted Audio Data (first 10 samples):" << std::endl;
	//	for (size_t i = 0; i < min(numSamples, size_t(10)); ++i) {
	//		std::cout << std::setw(10) << std::setfill(' ') << audioData[i] << ' ';
	//	}
	//	std::cout << std::endl;
	//
	//	return audioData;
	//}

	//SOUND_API_C void AudioCapture::SetupMainAudioDevice()
	//{
	//	// Get default audio capture device
	//	hr = enumerator->GetDefaultAudioEndpoint(eCapture, eConsole, &device);
	//	if (FAILED(hr)) {
	//		std::cerr << "Failed to get default audio endpoint: " << std::hex << hr << std::endl;
	//		return;
	//	}
	//
	//	// Activate the IAudioClient interface
	//	hr = device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, NULL, (void**)&audioClient);
	//	if (FAILED(hr)) {
	//		std::cerr << "Failed to activate IAudioClient: " << std::hex << hr << std::endl;
	//		return;
	//	}
	//
	//	
	//
	//	// Get the device's mix format
	//	hr = audioClient->GetMixFormat(&waveFormat);
	//	if (FAILED(hr)) {
	//		std::cerr << "Failed to get mix format: " << std::hex << hr << std::endl;
	//		return;
	//	}
	//
	//	// Initialize the audio client
	//	hr = audioClient->Initialize(
	//		AUDCLNT_SHAREMODE_SHARED,           // Shared mode
	//		0,                                  // No special stream flags
	//		10000000,                           // Buffer duration (10000000 = 1 second)
	//		0,                                  // Periodicity (0 means default)
	//		waveFormat,                         // Pointer to the WAVEFORMATEX structure
	//		NULL                                // Audio session GUID (NULL means default)
	//	);
	//
	//	if (FAILED(hr)) {
	//		std::cerr << "Failed to initialize IAudioClient: " << std::hex << hr << std::endl;
	//		CoTaskMemFree(waveFormat);  // Clean up allocated format
	//		return;
	//	}
	//
	//	// Get the capture client
	//	hr = audioClient->GetService(__uuidof(IAudioCaptureClient), (void**)&captureClient);
	//	if (FAILED(hr)) {
	//		std::cerr << "Failed to get IAudioCaptureClient: " << std::hex << hr << std::endl;
	//		CoTaskMemFree(waveFormat);  // Clean up allocated format
	//		return;
	//	}
	//
	//	// Start the audio stream
	//	hr = audioClient->Start();
	//	if (FAILED(hr)) {
	//		std::cerr << "Failed to start audio capture: " << std::hex << hr << std::endl;
	//		CoTaskMemFree(waveFormat);  // Clean up allocated format
	//		return;
	//	}
	//
	//	/*std::cout << "Capturing audio data... Press Ctrl+C to stop." << std::endl;*/
	//}
}