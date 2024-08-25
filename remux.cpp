#pragma warning(push, 0)
#include <map>

extern "C" {
#include <libavutil/mem.h>
#include <libavformat/avformat.h>
}
#pragma warning(pop)

#include <algorithm>
#include <iostream>
#include <span>
#include <print>

#pragma warning(push)
#pragma warning(disable : 4365)
#pragma warning(disable : 4388)
#pragma warning(disable : 5045)

struct Args {
    char const *input_filename;
    char const *output_filename;
};

Args parse_args(int const argc, char **argv) {
    if (argc < 3) {
        std::println(std::cerr, "usage: {} input output\n"
                     "API example program to remux a media file with libavformat and libavcodec.\n"
                     "The output format is guessed according to the file extension.\n", argv[0]);
        std::exit(EXIT_FAILURE);
    }
    return {argv[1], argv[2]};
}

AVFormatContext* load_input_video(const char *input_filename) {
    AVFormatContext *input_format_context{};
    if (avformat_open_input(&input_format_context, input_filename, nullptr, nullptr) < 0)
        throw std::runtime_error("Could not open input file");

    if (avformat_find_stream_info(input_format_context, nullptr) < 0)
        throw std::runtime_error("Failed to retrieve input stream information");

    return input_format_context;
}

AVFormatContext* create_output_video(const char *output_filename) {
    AVFormatContext *output_format_context{};
    avformat_alloc_output_context2(&output_format_context, nullptr, nullptr, output_filename);
    if (!output_format_context)
        throw std::runtime_error("Could not create output context");

    return output_format_context;
}

std::map<int, int> copy_streams(AVFormatContext const *input_format_context,
                                AVFormatContext *output_format_context,
                                std::initializer_list<AVMediaType> relevant_media_types) {
    std::map<int, int> stream_mapping;
    int i{};

    for (std::span const input_streams{input_format_context->streams, input_format_context->nb_streams}; auto const &
         in_stream : input_streams) {
        auto const input_codec_parameters{in_stream->codecpar};

        if (std::ranges::find(relevant_media_types, input_codec_parameters->codec_type) == relevant_media_types.end()) {
            i += 1;
            continue;
        }

        stream_mapping[in_stream->index] = in_stream->index - i;

        auto const output_stream{avformat_new_stream(output_format_context, nullptr)};
        if (!output_stream)
            throw std::runtime_error("Failed allocating output stream");

        if (avcodec_parameters_copy(output_stream->codecpar, input_codec_parameters) < 0)
            throw std::runtime_error("Failed to copy codec parameters");

        output_stream->codecpar->codec_tag = 0;
    }

    return stream_mapping;
}

void remux_packets(AVFormatContext *input_format_context, AVFormatContext *output_format_context,
                   const std::map<int, int> &stream_mapping) {
    auto const packet{av_packet_alloc()};
    if (!packet)
        throw std::runtime_error("Could not allocate AVPacket");

    std::span const input_streams{input_format_context->streams, input_format_context->nb_streams};
    std::span const output_streams{output_format_context->streams, output_format_context->nb_streams};

    while (av_read_frame(input_format_context, packet) >= 0) {
        if (packet->stream_index >= stream_mapping.size() ||
            !stream_mapping.contains(packet->stream_index)) {
            av_packet_unref(packet);
            continue;
        }

        packet->stream_index = stream_mapping.at(packet->stream_index);

        auto const input_stream{input_streams[packet->stream_index]};
        auto const output_stream{output_streams[packet->stream_index]};
        av_packet_rescale_ts(packet, input_stream->time_base, output_stream->time_base);
        packet->pos = -1;

        if (av_interleaved_write_frame(output_format_context, packet) < 0)
            throw std::runtime_error("Error muxing packet");
    }
}

void open_output_file(AVFormatContext *output_format_context, const char *output_filename) {
    if (!(output_format_context->oformat->flags & AVFMT_NOFILE))
        if (avio_open(&output_format_context->pb, output_filename, AVIO_FLAG_WRITE) < 0)
            throw std::runtime_error("Could not open output file");

    if (avformat_write_header(output_format_context, nullptr) < 0)
        throw std::runtime_error("Error occurred when opening output file");
}

void close_output_file(AVFormatContext *output_format_context) {
    av_write_trailer(output_format_context);
    if (!(output_format_context->oformat->flags & AVFMT_NOFILE))
        avio_closep(&output_format_context->pb);
}

void remux_video(const char *input_filename, const char *output_filename,
                 std::initializer_list<AVMediaType> const relevant_media_types) {
    auto const input_format_context{load_input_video(input_filename)};
    auto const output_format_context{create_output_video(output_filename)};

    auto const stream_mapping{
        copy_streams(input_format_context, output_format_context,
                     relevant_media_types)
    };

    open_output_file(output_format_context, output_filename);
    remux_packets(input_format_context, output_format_context, stream_mapping);
    close_output_file(output_format_context);
}

int main(int const argc, char **argv) {
    auto const [input_filename, output_filename]{parse_args(argc, argv)};
    remux_video(input_filename, output_filename, {AVMEDIA_TYPE_AUDIO, AVMEDIA_TYPE_VIDEO, AVMEDIA_TYPE_SUBTITLE});
    return EXIT_SUCCESS;
}

#pragma warning(pop)
