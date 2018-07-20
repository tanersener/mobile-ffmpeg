#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <chrono>
#include <chromaprint.h>
#include "audio/ffmpeg_audio_reader.h"
#include "utils/scope_exit.h"

#ifdef _WIN32
#include <windows.h>
#endif

using namespace chromaprint;

enum Format {
	TEXT = 0,
	JSON,
	PLAIN,
};

static Format g_format = TEXT;
static char *g_input_format = nullptr;
static int g_input_channels = 0;
static int g_input_sample_rate = 0;
static double g_max_duration = 120;
static double g_max_chunk_duration = 0;
static bool g_overlap = false;
static bool g_raw = false;
static bool g_abs_ts = false;
static bool g_ignore_errors = false;
static ChromaprintAlgorithm g_algorithm = CHROMAPRINT_ALGORITHM_DEFAULT;


const char *g_help =
	"Usage: %s [OPTIONS] FILE [FILE...]\n"
	"\n"
	"Generate fingerprints from audio files/streams.\n"
	"\n"
	"Options:\n"
	"  -format NAME   Set the input format name\n"
	"  -rate NUM      Set the sample rate of the input audio\n"
	"  -channels NUM  Set the number of channels in the input audio\n"
	"  -length SECS   Restrict the duration of the processed input audio (default 120)\n"
	"  -chunk SECS    Split the input audio into chunks of this duration\n"
	"  -algorithm NUM Set the algorigthm method (default 2)\n"
	"  -overlap       Overlap the chunks slightly to make sure audio on the edges is fingerprinted\n"
	"  -ts            Output UNIX timestamps for chunked results, useful when fingerprinting real-time audio stream\n"
	"  -raw           Output fingerprints in the uncompressed format\n"
	"  -json          Print the output in JSON format\n"
	"  -text          Print the output in text format\n"
	"  -plain         Print the just the fingerprint in text format\n"
	"  -version       Print version information\n"
	;

static void ParseOptions(int &argc, char **argv) {
	int j = 1;
	for (int i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "--")) {
			while (++i < argc) {
				argv[j++] = argv[i];
			}
		} else if ((!strcmp(argv[i], "-format") || !strcmp(argv[i], "-f")) && i + 1 < argc) {
			g_input_format = argv[++i];
		} else if ((!strcmp(argv[i], "-channels") || !strcmp(argv[i], "-c")) && i + 1 < argc) {
			auto value = atoi(argv[i + 1]);
			if (value > 0) {
				g_input_channels = value;
			} else {
				fprintf(stderr, "ERROR: The argument for %s must be a non-zero positive number\n", argv[i]);
				exit(2);
			}
			i++;
		} else if ((!strcmp(argv[i], "-rate") || !strcmp(argv[i], "-r")) && i + 1 < argc) {
			auto value = atoi(argv[i + 1]);
			if (value >= 0) {
				g_input_sample_rate = value;
			} else {
				fprintf(stderr, "ERROR: The argument for %s must be a positive number\n", argv[i]);
				exit(2);
			}
			i++;
		} else if ((!strcmp(argv[i], "-length") || !strcmp(argv[i], "-t")) && i + 1 < argc) {
			auto value = atof(argv[i + 1]);
			if (value >= 0) {
				g_max_duration = value;
			} else {
				fprintf(stderr, "ERROR: The argument for %s must be a positive number\n", argv[i]);
				exit(2);
			}
			i++;
		} else if (!strcmp(argv[i], "-chunk") && i + 1 < argc) {
			auto value = atof(argv[i + 1]);
			if (value >= 0) {
				g_max_chunk_duration = value;
			} else {
				fprintf(stderr, "ERROR: The argument for %s must be a positive number\n", argv[i]);
				exit(2);
			}
			i++;
        } else if ((!strcmp(argv[i], "-algorithm") || !strcmp(argv[i], "-a")) && i + 1 < argc) {
            auto value = atoi(argv[i + 1]);
            if (value >= 1 && value <= 5) {
                g_algorithm = (ChromaprintAlgorithm)(value - 1);
            } else {
                fprintf(stderr, "ERROR: The argument for %s must be 1 - 5\n", argv[i]);
                exit(2);
            }
            i++;
		} else if (!strcmp(argv[i], "-text")) {
			g_format = TEXT;
		} else if (!strcmp(argv[i], "-json")) {
			g_format = JSON;
		} else if (!strcmp(argv[i], "-plain")) {
			g_format = PLAIN;
		} else if (!strcmp(argv[i], "-overlap")) {
			g_overlap = true;
		} else if (!strcmp(argv[i], "-ts")) {
			g_abs_ts = true;
		} else if (!strcmp(argv[i], "-raw")) {
			g_raw = true;
		} else if (!strcmp(argv[i], "-ignore-errors")) {
			g_ignore_errors = true;
		} else if (!strcmp(argv[i], "-v") || !strcmp(argv[i], "-version")) {
			fprintf(stdout, "fpcalc version %s\n", chromaprint_get_version());
			exit(0);
		} else if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "-help") || !strcmp(argv[i], "--help")) {
			fprintf(stdout, g_help, argv[0]);
			exit(0);
		} else {
			const auto len = strlen(argv[i]);
			if (len > 1 && argv[i][0] == '-') {
				fprintf(stderr, "ERROR: Unknown option %s\n", argv[i]);
				exit(2);
			} else {
				argv[j++] = argv[i];
			}
		}
	}
	if (j < 2) {
		fprintf(stderr, "ERROR: No input files\n");
		exit(2);
	}
	argc = j;
}

void PrintResult(ChromaprintContext *ctx, FFmpegAudioReader &reader, bool first, double timestamp, double duration) {
	std::string tmp_fp;
	const char *fp;
	bool dealloc_fp = false;

	int size;
	if (!chromaprint_get_raw_fingerprint_size(ctx, &size)) {
		fprintf(stderr, "ERROR: Could not get the fingerprinting size\n");
		exit(2);
	}
	if (size <= 0) {
		if (first) {
			fprintf(stderr, "ERROR: Empty fingerprint\n");
			exit(2);
		}
		return;
	}

	if (g_raw) {
		std::stringstream ss;
		uint32_t *raw_fp_data = nullptr;
		int raw_fp_size = 0;
		if (!chromaprint_get_raw_fingerprint(ctx, &raw_fp_data, &raw_fp_size)) {
			fprintf(stderr, "ERROR: Could not get the fingerprinting\n");
			exit(2);
		}
		SCOPE_EXIT(chromaprint_dealloc(raw_fp_data));
		for (int i = 0; i < raw_fp_size; i++) {
			if (i > 0) {
				ss << ',';
			}
			ss << raw_fp_data[i];
		}
		tmp_fp = ss.str();
		fp = tmp_fp.c_str();
	} else {
		char *tmp_fp2;
		if (!chromaprint_get_fingerprint(ctx, &tmp_fp2)) {
			fprintf(stderr, "ERROR: Could not get the fingerprinting\n");
			exit(2);
		}
		fp = tmp_fp2;
		dealloc_fp = true;
	}
	SCOPE_EXIT(if (dealloc_fp) { chromaprint_dealloc((void *) fp); });

	if (g_max_chunk_duration == 0) {
		duration = reader.GetDuration();
		if (duration < 0.0) {
			duration = 0.0;
		} else {
			duration /= 1000.0;
		}
	}

	switch (g_format) {
		case TEXT:
			if (!first) {
				printf("\n");
			}
			if (g_abs_ts) {
				printf("TIMESTAMP=%.2f\n", timestamp);
			}
			printf("DURATION=%d\nFINGERPRINT=%s\n", int(duration), fp);
			break;
		case JSON:
			if (g_max_chunk_duration != 0) {
				if (g_raw) {
					printf("{\"timestamp\": %.2f, \"duration\": %.2f, \"fingerprint\": [%s]}\n", timestamp, duration, fp);
				} else {
					printf("{\"timestamp\": %.2f, \"duration\": %.2f, \"fingerprint\": \"%s\"}\n", timestamp, duration, fp);
				}
			} else {
				if (g_raw) {
					printf("{\"duration\": %.2f, \"fingerprint\": [%s]}\n", duration, fp);
				} else {
					printf("{\"duration\": %.2f, \"fingerprint\": \"%s\"}\n", duration, fp);
				}
			}
			break;
		case PLAIN:
			printf("%s\n", fp);
			break;
	}

	fflush(stdout);
}

double GetCurrentTimestamp() {
	const auto now = std::chrono::system_clock::now();
	const auto usec = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch());
	return usec.count() / 1000000.0;
}

void ProcessFile(ChromaprintContext *ctx, FFmpegAudioReader &reader, const char *file_name) {
	double ts = 0.0;
	if (g_abs_ts) {
		ts = GetCurrentTimestamp();
	}

	if (!strcmp(file_name, "-")) {
		file_name = "pipe:0";
	}

	if (!reader.Open(file_name)) {
		fprintf(stderr, "ERROR: %s\n", reader.GetError().c_str());
		exit(2);
	}

	if (!chromaprint_start(ctx, reader.GetSampleRate(), reader.GetChannels())) {
		fprintf(stderr, "ERROR: Could not initialize the fingerprinting process\n");
		exit(2);
	}

	size_t stream_size = 0;
	const size_t stream_limit = g_max_duration * reader.GetSampleRate();

	size_t chunk_size = 0;
	const size_t chunk_limit = g_max_chunk_duration * reader.GetSampleRate();

	size_t extra_chunk_limit = 0;
	double overlap = 0.0;
	if (chunk_limit > 0 && g_overlap) {
		extra_chunk_limit = chromaprint_get_delay(ctx);
		overlap = chromaprint_get_delay_ms(ctx) / 1000.0;
	}

	bool first_chunk = true;
	bool read_failed = false;
	bool got_results = false;

	while (!reader.IsFinished()) {
		const int16_t *frame_data = nullptr;
		size_t frame_size = 0;
		if (!reader.Read(&frame_data, &frame_size)) {
			fprintf(stderr, "ERROR: %s\n", reader.GetError().c_str());
			read_failed = true;
			break;
		}

		bool stream_done = false;
		if (stream_limit > 0) {
			const auto remaining = stream_limit - stream_size;
			if (frame_size > remaining) {
				frame_size = remaining;
				stream_done = true;
			}
		}
		stream_size += frame_size;

		if (frame_size == 0) {
			if (stream_done) {
				break;
			} else {
				continue;
			}
		}

		bool chunk_done = false;
		size_t first_part_size = frame_size;
		if (chunk_limit > 0) {
			const auto remaining = chunk_limit + extra_chunk_limit - chunk_size;
			if (first_part_size > remaining) {
				first_part_size = remaining;
				chunk_done = true;
			}
		}

		if (!chromaprint_feed(ctx, frame_data, first_part_size * reader.GetChannels())) {
			fprintf(stderr, "ERROR: Could not process audio data\n");
			exit(2);
		}

		chunk_size += first_part_size;

		if (chunk_done) {
			if (!chromaprint_finish(ctx)) {
				fprintf(stderr, "ERROR: Could not finish the fingerprinting process\n");
				exit(2);
			}

			const auto chunk_duration = (chunk_size - extra_chunk_limit) * 1.0 / reader.GetSampleRate() + overlap;
			PrintResult(ctx, reader, first_chunk, ts, chunk_duration);
			got_results = true;

			if (g_abs_ts) {
				ts = GetCurrentTimestamp();
			} else {
				ts += chunk_duration;
			}

			if (g_overlap) {
				if (!chromaprint_clear_fingerprint(ctx)) {
					fprintf(stderr, "ERROR: Could not initialize the fingerprinting process\n");
					exit(2);
				}
				ts -= overlap;
			} else {
				if (!chromaprint_start(ctx, reader.GetSampleRate(), reader.GetChannels())) {
					fprintf(stderr, "ERROR: Could not initialize the fingerprinting process\n");
					exit(2);
				}
			}

			if (first_chunk) {
				extra_chunk_limit = 0;
				first_chunk = false;
			}

			chunk_size = 0;
		}

		frame_data += first_part_size * reader.GetChannels();
		frame_size -= first_part_size;

		if (frame_size > 0) {
			if (!chromaprint_feed(ctx, frame_data, frame_size * reader.GetChannels())) {
				fprintf(stderr, "ERROR: Could not process audio data\n");
				exit(2);
			}
		}

		chunk_size += frame_size;

		if (stream_done) {
			break;
		}
	}

	if (!chromaprint_finish(ctx)) {
		fprintf(stderr, "ERROR: Could not finish the fingerprinting process\n");
		exit(2);
	}

	if (chunk_size > 0) {
		const auto chunk_duration = (chunk_size - extra_chunk_limit) * 1.0 / reader.GetSampleRate() + overlap;
		PrintResult(ctx, reader, first_chunk, ts, chunk_duration);
		got_results = true;
	} else if (first_chunk) {
		fprintf(stderr, "ERROR: Not enough audio data\n");
		exit(2);
	}

	if (!g_ignore_errors) {
		if (read_failed) {
			exit(got_results ? 3 : 2);
		}
	}
}

int fpcalc_main(int argc, char **argv) {
	ParseOptions(argc, argv);

	FFmpegAudioReader reader;
	if (g_input_format) {
		if (!reader.SetInputFormat(g_input_format)) {
			fprintf(stderr, "ERROR: Invalid format\n");
			return 2;
		}
	}
	if (g_input_channels) {
		if (!reader.SetInputChannels(g_input_channels)) {
			fprintf(stderr, "ERROR: Invalid number of channels\n");
			return 2;
		}
	}
	if (g_input_sample_rate) {
		if (!reader.SetInputSampleRate(g_input_sample_rate)) {
			fprintf(stderr, "ERROR: Invalid sample rate\n");
			return 2;
		}
	}

	ChromaprintContext *chromaprint_ctx = chromaprint_new(g_algorithm);
	SCOPE_EXIT(chromaprint_free(chromaprint_ctx));

	reader.SetOutputChannels(chromaprint_get_num_channels(chromaprint_ctx));
	reader.SetOutputSampleRate(chromaprint_get_sample_rate(chromaprint_ctx));

	for (int i = 1; i < argc; i++) {
		ProcessFile(chromaprint_ctx, reader, argv[i]);
	}

	return 0;
}

#ifdef _WIN32
int main(int win32_argc, char **win32_argv)
{
	int i, argc = 0, buffsize = 0, offset = 0;
	char **utf8_argv, *utf8_argv_ptr;
	wchar_t **argv;
	argv = CommandLineToArgvW(GetCommandLineW(), &argc);
	buffsize = 0;
	for (i = 0; i < argc; i++) {
		buffsize += WideCharToMultiByte(CP_UTF8, 0, argv[i], -1, NULL, 0, NULL, NULL);
	}
	utf8_argv = (char **) av_mallocz(sizeof(char *) * (argc + 1) + buffsize);
	utf8_argv_ptr = (char *) utf8_argv + sizeof(char *) * (argc + 1);
	for (i = 0; i < argc; i++) {
		utf8_argv[i] = &utf8_argv_ptr[offset];
		offset += WideCharToMultiByte(CP_UTF8, 0, argv[i], -1, &utf8_argv_ptr[offset], buffsize - offset, NULL, NULL);
	}
	LocalFree(argv);
	return fpcalc_main(argc, utf8_argv);
}
#else
int main(int argc, char **argv)
{
	return fpcalc_main(argc, argv);
}
#endif
