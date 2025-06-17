#include <reonpch.h>
#include "AudioManager.h"
#define M_PI       3.14159265358979323846   // pi


namespace REON::AUDIO {

	AudioManager::AudioManager() : capture(buffer), bandEdges(numBands + 1), smoothedBands(numBands)
	{
		std::fill(std::begin(Equaliser), std::end(Equaliser), 1.0f);
		// Precompute logarithmic band frequencies
		const float fourthOctaveFactor = pow(2.0f, 1.0f / 6.0f);
		for (size_t k = 0; k <= numBands; ++k) {
			bandEdges[k] = minFreq * pow(fourthOctaveFactor, k);
		}

		for (size_t i = 0; i < bandEdges.size() - 1; ++i) {
			if (bandEdges[i] >= kickMinFreq && kickMinBand == -1) {
				kickMinBand = i;
			}
			if (bandEdges[i + 1] >= kickMaxFreq) {
				kickMaxBand = i;
				break;
			}
		}

		for (size_t i = 0; i < bandEdges.size() - 1; ++i) {
			if (bandEdges[i] >= hihatMinFreq && hihatMinBand == -1) {
				hihatMinBand = i;
			}
			if (bandEdges[i + 1] >= hihatMaxFreq) {
				hihatMaxBand = i;
				break;
			}
		}
		//_logger = spdlog::basic_logger_mt("AudioManager", "logs/log.txt");
		capture.StartCapture();

		StartDataTransfer();
	}

	AudioManager::~AudioManager()
	{
		capture.StopCapture();

		StopDataTransfer();

		if (transferThread.joinable()) {
			try {
				std::cerr << "Joining thread in destructor...\n";
				transferThread.join();
			}
			catch (const std::system_error& e) {
				std::cerr << "Error joining thread in destructor: " << e.what() << std::endl;
			}
		}
	}

	//void AudioManager::Init(){

	//}

	std::vector<float> AudioManager::GetAudioData()
	{
		std::vector<float> real(fftSize);
		buffer.peek_latest(real.data(), fftSize);

		ApplyHannWindow(real);

		std::vector<float> imag(fftSize);

		ComputeFFT(real.data(), real.size(), imag.data());

		std::vector<float> magnitude(fftSize / 2);

		//float sumSquares = 0.0f;
		for (int i = 0; i < fftSize / 2; i++) {
			magnitude[i] = sqrtf(real[i] * real[i] + imag[i] * imag[i]);
			magnitude[i] = 20 * log10f(magnitude[i] + 1e-6);  // Add small epsilon to avoid log(0)
			//sumSquares += magnitude[i] * magnitude[i];
		}

		//float rms = sqrtf(sumSquares/ (fftSize/2));

		//for (int i = 0; i < fftSize / 2; i++) {
		//	magnitude[i] /= rms;
		//}

		std::vector<float> bands(numBands);

		ComputeLogarithmicBands(magnitude, bands);

		ApplyEqualiser(bands);

		return bands;
	}

	void AudioManager::ApplyEqualiser(std::vector<float>& bands) {
		for (int i = 0; i < numBands; i++) {
			bands[i] *= Equaliser[i];
		}
	}

	std::vector<std::wstring> AudioManager::GetAudioDevices()
	{
		return capture.LoadAudioDevices(eAll);
	}

	int AudioManager::SelectAudioDevice(int index)
	{
		return capture.SetAudioDevice(index);
	}

	bool AudioManager::IsKicking()
	{
		return kick;
	}

	bool AudioManager::IsHiHat()
	{
		return hihat;
	}

	void AudioManager::StartDataTransfer()
	{
		if (!transferring) {
			transferring = true;
			transferThread = std::thread(&AudioManager::TransferLoop, this);
			//transferThread = std::thread([this]() {
			//	while (transferring) {
			//		std::this_thread::sleep_for(std::chrono::milliseconds(100));
			//		std::cout << "Transferring data..." << std::endl;
			//	}
			//	});
		}
	}

	void AudioManager::StopDataTransfer()
	{
		if (transferring) {
			transferring = false;
			if (transferThread.joinable()) {
				try {
					transferThread.join();
				}
				catch (const std::system_error& e) {
					std::cerr << "Error joining transfer thread: " << e.what() << std::endl;
				}
			}
		}
	}

	void AudioManager::TransferLoop()
	{
		while (transferring) {
			auto data = GetAudioData();
			KickDetection(data);
			HiHatDetection(data);
			auto size = data.size();
		}
	}

	void AudioManager::ComputeFFT(float* real, size_t realSize, float* imag)
	{
		int m = static_cast<int>(std::log2(realSize));

		// Bit-reversal permutation
		int j = 0;
		for (int i = 0; i < realSize; i++)
		{
			if (i < j)
			{
				// Swap values
				float tempReal = real[i];
				float tempImag = imag[i];
				real[i] = real[j];
				imag[i] = imag[j];
				real[j] = tempReal;
				imag[j] = tempImag;
			}
			int mask = realSize >> 1;
			while (j >= mask && mask > 0)
			{
				j -= mask;
				mask >>= 1;
			}
			j += mask;
		}

		// FFT computation
		for (int s = 1; s <= m; s++)
		{
			int m2 = 1 << s;
			float angle = static_cast<float>(-2 * M_PI / m2);
			float wReal = cos(angle);
			float wImag = sin(angle);
			for (int k = 0; k < realSize; k += m2)
			{
				float wr = 1;
				float wi = 0;
				for (int j2 = 0; j2 < m2 / 2; j2++)
				{
					int i1 = k + j2;
					int i2 = i1 + m2 / 2;
					float tReal = wr * real[i2] - wi * imag[i2];
					float tImag = wr * imag[i2] + wi * real[i2];
					real[i2] = real[i1] - tReal;
					imag[i2] = imag[i1] - tImag;
					real[i1] += tReal;
					imag[i1] += tImag;

					float tmpWr = wr;
					wr = tmpWr * wReal - wi * wImag;
					wi = tmpWr * wImag + wi * wReal;
				}
			}
		}
	}

	void AudioManager::ComputeLogarithmicBands(const std::vector<float>& magnitude, std::vector<float>& bands) {
		// Compute bands
		for (size_t band = 0; band < numBands; ++band) {
			float bandStartFreq = bandEdges[band];
			float bandEndFreq = bandEdges[band + 1];

			size_t startBin = static_cast<size_t>(bandStartFreq * fftSize / sampleRate);
			size_t endBin = static_cast<size_t>(bandEndFreq * fftSize / sampleRate);

			// Clamp to valid bin indices
			startBin = std::min(startBin, magnitude.size());
			endBin = std::min(endBin, magnitude.size());

			// Ensure there's a reasonable bin range for the lowest frequencies
			if (startBin == endBin) {
				// Force at least two bins for low frequencies, ensuring some energy calculation
				if (startBin == 0) {
					endBin = std::min(startBin + 2, fftSize / 2ull);  // Minimum 2 bins for the first band
				}
				else {
					endBin = startBin + 1; // Otherwise, force at least one bin width for higher frequencies
				}
			}

			// Avoid range overlap (ensure startBin < endBin)
			if (startBin >= endBin) {
				continue; // Skip or adjust the logic if you want to handle this case
			}

			// Accumulate magnitude values in the range
			bands[band] = 0.0f;
			for (size_t bin = startBin; bin < endBin; ++bin) {
				bands[band] += magnitude[bin];
			}

			// Optional: Normalize by the number of bins in the band
			if (endBin > startBin) {
				bands[band] /= (endBin - startBin);
			}
		}
	}



	void AudioManager::ApplyHannWindow(std::vector<float>& signal) {
		size_t N = signal.size();
		for (size_t n = 0; n < N; ++n) {
			float windowValue = 0.5f * (1.0f - cosf(2.0f * M_PI * n / (N - 1)));
			signal[n] *= windowValue;
		}
	}

	void AudioManager::KickDetection(std::vector<float>& bands)
	{
		totalEnergy = 0.0f;
		for (size_t i = 1; i < numBands - 1; ++i) {
			smoothedBands[i] = (bands[i - 1] + bands[i] + bands[i + 1]) / 3.0f;
			totalEnergy += bands[i];
		}

		float kickEnergy = 0.0f;
		if (kickMinBand != -1 && kickMaxBand != -1) {
			for (int i = kickMinBand; i <= kickMaxBand; ++i) {
				//float weight = 1.0f + staticast<float>(i - kickMinBand) / (kickMaxBand - kickMinBand);
				kickEnergy += bands[i];
			}
		}

		float baseline = 0.0f;
		if (!kickEnergyHistory.empty()) {
			baseline = std::accumulate(kickEnergyHistory.begin(), kickEnergyHistory.end(), 0.0f) / kickEnergyHistory.size();
		}

		float thresholdWithBias = baseline + kickThreshold + (kickBias * kickThreshold);
		float lowerThreshold = baseline + kickThreshold - (kickThreshold * kickBias * 10);

		if (kickEnergy > thresholdWithBias && kickEnergy / totalEnergy >= 0.2f) {
			kick = true;
			//std::cout << "Kick detected! Energy: " << kickEnergy << " Threshold with bias: " << thresholdWithBias << std::endl;
			kickEnergyHistory.insert(kickEnergyHistory.begin(), kickEnergy);
		}
		else if (kick && kickEnergy < lowerThreshold) {
			//std::cout << "Kick reset: Energy: " << kickEnergy << " Below lower threshold: " << lowerThreshold << std::endl;
			kick = false;
		}

		kickThreshold = std::max(kickThreshold * kickThresholdDecayRate, kickTransientSensitivity * (kickEnergy - baseline));

		if (kickEnergy / totalEnergy >= 0.2f) {
			kickEnergyHistory.push_back(kickEnergy);
			if (kickEnergyHistory.size() > kickMaxHistorySize) {
				kickEnergyHistory.pop_front(); // Remove the oldest value
			}
		}
	}

	void AudioManager::HiHatDetection(std::vector<float>& bands) {
		float hihatEnergy = 0.0f;

		// Compute hi-hat energy for the specified band range
		if (hihatMinBand != -1 && hihatMaxBand != -1) {
			for (int i = hihatMinBand; i <= hihatMaxBand; ++i) {
				hihatEnergy += bands[i];
			}
		}

		// Baseline adjustment with exponential smoothing
		float alpha = 0.2f; // Smoothing factor for baseline
		baseline = (1 - alpha) * baseline + alpha * hihatEnergy;

		// Dynamic thresholds with adaptive bias
		float dynamicBias = baseline * hihatBias;
		float thresholdWithBias = baseline + hihatThreshold + dynamicBias;
		float lowerThreshold = baseline + hihatThreshold - (dynamicBias * 0.5);

		// Update energy history for transient tracking (simple moving average)
		hihatEnergyHistory.insert(hihatEnergyHistory.begin(), hihatEnergy);
		if (hihatEnergyHistory.size() > hihatMaxHistorySize) {
			hihatEnergyHistory.pop_back(); // Maintain fixed history size
		}

		// Compute moving average of the past energies
		float movingAvgEnergy = 0.0f;
		for (size_t i = 0; i < hihatEnergyHistory.size(); ++i) {
			movingAvgEnergy += hihatEnergyHistory[i];
		}
		movingAvgEnergy /= hihatEnergyHistory.size();

		// Calculate rate of change using the moving average (instead of just the front)
		float energyChange = hihatEnergy - movingAvgEnergy;

		// Compare hi-hat band energy with neighboring bands (spectral contrast)
		float surroundingEnergy = 0.0f;
		int neighborBandCount = 1; // Number of bands to compare on each side
		for (int i = std::max(0, hihatMinBand - neighborBandCount); i <= std::min(static_cast<int>(bands.size() - 1), hihatMaxBand + neighborBandCount); ++i) {
			surroundingEnergy += bands[i];
		}
		surroundingEnergy -= hihatEnergy; // Exclude hi-hat energy itself
		float contrast = hihatEnergy / (surroundingEnergy + 1e-6f); // Prevent division by zero

		// Hi-hat detection conditions
		if (hihatEnergy > thresholdWithBias && energyChange > dynamicSpikeSensitivity && contrast > spectralContrastThreshold) {
			hihat = true;
			std::cout << "Hi-Hat detected! Energy: " << hihatEnergy << ", Contrast: " << contrast << std::endl;
		}
		else if (hihat && hihatEnergy < lowerThreshold) {
			hihat = false;
			std::cout << "Hi-Hat reset. Energy: " << hihatEnergy << std::endl;
		}
	}

	// void AudioManager::HiHatDetection(std::vector<float>& bands) {
	//	float hihatEnergy = 0.0f;
	//	if (hihatMinBand != -1 && hihatMaxBand != -1) {
	//		for (int i = hihatMinBand; i <= hihatMaxBand; ++i) {
	//			float weight = 1.0f + staticast<float>(i - hihatMinBand) / (hihatMaxBand - hihatMinBand);
	//			hihatEnergy += bands[i];
	//		}
	//	}

	//	float baseline = 0.0f;
	//	if (!hihatEnergyHistory.empty()) {
	//		baseline = std::accumulate(hihatEnergyHistory.begin(), hihatEnergyHistory.end(), 0.0f) / hihatEnergyHistory.size();
	//	}

	//	float thresholdWithBias = baseline + hihatThreshold + (hihatBias * hihatThreshold);
	//	float lowerThreshold = baseline + hihatThreshold - (hihatThreshold * hihatBias);


	//	if (hihatEnergy > thresholdWithBias && hihatEnergy / totalEnergy >= 0.1f) {
	//		hihat = true;
	//		std::cout << "Hi Hat detected! Energy: " << hihatEnergy << " Threshold with bias: " << thresholdWithBias << std::endl;
	//		hihatEnergyHistory.insert(hihatEnergyHistory.begin(), hihatEnergy);
	//	}
	//	else if (hihat && hihatEnergy < lowerThreshold) {
	//		std::cout << "Hi Hat reset: Energy: " << hihatEnergy << " Below lower threshold: " << lowerThreshold << std::endl;
	//		hihat = false;
	//	}

	//	hihatThreshold = max(hihatThreshold * hihatThresholdDecayRate, hihatTransientSensitivity * (hihatEnergy - baseline));

	//	if (hihatEnergy / totalEnergy >= 0.1f) {
	//		hihatEnergyHistory.push_back(hihatEnergy);
	//		if (hihatEnergyHistory.size() > hihatMaxHistorySize) {
	//			hihatEnergyHistory.pop_front(); // Remove the oldest value
	//		}
	//	}
	//}
}