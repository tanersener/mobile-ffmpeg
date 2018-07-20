#include <algorithm>
#include <vector>
#include "audio_consumer.h"

class AudioBuffer : public chromaprint::AudioConsumer
{
public:
	void Consume(const int16_t *input, int length) override
	{
		int last_size = m_data.size();
		m_data.resize(last_size + length);
		std::copy(input, input + length, m_data.begin() + last_size);
	}

	const std::vector<int16_t> &data() { return m_data; }

private:
	std::vector<int16_t> m_data;
};
