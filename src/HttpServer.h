#pragma once

#include "AudioFileReader.h"
#include "Options.h"
#include <string>

class HttpServer {
    public:
    void run(const Options& options);

    private:
    bool generateWaveformData(std::string filename, const Options& options, std::ostream& outstream);
};

static std::unique_ptr<AudioFileReader> createAudioFileReader(
    const boost::filesystem::path& input_filename,
    const Options& options);
