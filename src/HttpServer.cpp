#include "HttpServer.h"

#include <iostream>
#include <memory>
#include <ostream>
#include <sstream>
#include <string>
#include <boost/filesystem.hpp>

#include "AudioFileReader.h"
#include "FileFormat.h"
#include "Mp3AudioFileReader.h"
#include "SndFileAudioFileReader.h"
#include "WaveformGenerator.h"
#include "WaveformUtil.h"
#include "crow/crow_all.h"
#include "WaveformBuffer.h"

// TODO have these set by environment/command line params
const int PORT = 5040;

static std::unique_ptr<AudioFileReader> createAudioFileReader(
    const boost::filesystem::path& input_filename,
    const Options& options)
{
    std::unique_ptr<AudioFileReader> reader;
    // auto input_format = options.getInputFormat();
    auto input_format = FileFormat::Mp3; // TODO: Input format should be derived from file, not options.

    if (input_format == FileFormat::Wav ||
        input_format == FileFormat::Flac ||
        input_format == FileFormat::Ogg ||
        input_format == FileFormat::Opus) {
        reader.reset(new SndFileAudioFileReader);
    }
    else if (input_format == FileFormat::Mp3) {
        reader.reset(new Mp3AudioFileReader);
    }
    else if (input_format == FileFormat::Raw) {
        SndFileAudioFileReader* sndfile_audio_file_reader = new SndFileAudioFileReader;
        reader.reset(sndfile_audio_file_reader);

        sndfile_audio_file_reader->configure(
            options.getRawAudioChannels(),
            options.getRawAudioSampleRate(),
            options.getRawAudioFormat()
        );
    }
    else {
        // throwError("Unknown file type: %1%", input_filename);
        throw ;
    }

    return reader;
}

void HttpServer::run(const Options& options) {
    std::cout << "Starting Http server" << std::endl;
    std::cout << "Media dir: " << options.getInputFilename() << std::endl;

    crow::SimpleApp app;

    CROW_ROUTE(app, "/generate-waveform-data/<string>")([this, &options](std::string filename){
        std::ostringstream os;
        auto result = generateWaveformData(filename, options, os);
        if(result) {
            return crow::response(os.str());
        } else {
            return crow::response(500);
        }
    });

    app.port(PORT).multithreaded().run();
}

// TODO: return json of waveform data
bool HttpServer::generateWaveformData(std::string filename, const Options& options, std::ostream& outstream) {
    boost::filesystem::path fs_filename (filename);
    boost::filesystem::path src = options.getInputFilename() / fs_filename;

    if(!boost::filesystem::exists(src)) {
        return false;
    }

    std::unique_ptr<ScaleFactor> scale_factor;
    scale_factor.reset(new PixelsPerSecondScaleFactor(options.getPixelsPerSecond()));
    std::unique_ptr<AudioFileReader> audio_file_reader = createAudioFileReader(src, options);

    if (!audio_file_reader->open(src.string().c_str())) {
        return false;
    }

    WaveformBuffer buffer;
    WaveformGenerator processor(buffer, options.getSplitChannels(), *scale_factor);

    if (!audio_file_reader->run(processor)) {
        return false;
    }

    if (options.isAutoAmplitudeScale() && buffer.getSize() > 0) {
        const double amplitude_scale = WaveformUtil::getAmplitudeScale(
            buffer, 0, buffer.getSize()
        );

        WaveformUtil::scaleWaveformAmplitude(buffer, amplitude_scale);
    }

    buffer.saveAsJson(outstream, options.getBits());

    return true;
}
