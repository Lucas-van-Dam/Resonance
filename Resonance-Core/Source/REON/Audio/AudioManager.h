#pragma once

#include "AudioRingBuffer.h";
#include "AudioCapture.h"

#define _USE_MATH_DEFINES
#include <cmath>
#include <deque>
#include <numeric>
#include "spdlog/spdlog.h"
#include "spdlog/sinks/basic_file_sink.h"

namespace REON::AUDIO {
	class AudioManager
	{
	public:
		AudioManager();
		~AudioManager();

		//void Init();

		std::vector<float> GetAudioData();

		void ApplyEqualiser(std::vector<float>& bands);

		std::vector<std::wstring> GetAudioDevices();

		int SelectAudioDevice(int index);

		bool IsKicking();
		bool IsHiHat();

		void StartDataTransfer();
		void StopDataTransfer();

		void TransferLoop();

	public:
		//std::shared_ptr<spdlog::logger> _logger;

		float Equaliser[60];

	private:
		void ComputeFFT(float* real, size_t realSize, float* imag);

		void ComputeLogarithmicBands(const std::vector<float>& magnitudes, std::vector<float>& bands);

		void ApplyHannWindow(std::vector<float>& signal);

		void KickDetection(std::vector<float>& bands);

		void HiHatDetection(std::vector<float>& bands);

	private:
		AudioRingBuffer buffer;
		AudioCapture capture;

		bool kick;
		bool hihat;

		std::thread transferThread;
		std::atomic_bool transferring = false;

		const int sampleRate = 44100;
		const int fftSize = 8192;
		const int numBands = 60;
		const float minFreq = 20.0f;
		const float maxFreq = 20000.0f;
		std::vector<float> bandEdges;

		std::vector<float> smoothedBands;
		float totalEnergy;

		//kick detection
		int kickMinBand = -1, kickMaxBand = -1;
		const float kickMinFreq = 50.0f;
		const float kickMaxFreq = 120.0f;
		std::deque<float> kickEnergyHistory;
		const size_t kickMaxHistorySize = 100; // Length of history for baseline calculation
		float kickThreshold = 0.0f; // Dynamic threshold
		float kickThresholdDecayRate = 0.90f; // Decay factor for the threshold
		float kickTransientSensitivity = 1.5f; // Multiplier for detecting transients
		const float kickBias = 0.1f;

		//kick detection
		int hihatMinBand = -1, hihatMaxBand = -1;
		const float hihatMinFreq = 6000.0f;
		const float hihatMaxFreq = 12000.0f;
		std::deque<float> hihatEnergyHistory;
		const size_t hihatMaxHistorySize = 30;
		float hihatThreshold = 0.05f;
		const float hihatBias = 0.1f;
		float baseline;
		float dynamicSpikeSensitivity = 0.05f;
		float spectralContrastThreshold = 1.1f;
	};

}