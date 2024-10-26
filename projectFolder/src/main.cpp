#include "flexibity/log.h"
#include "flexibity/programOptions.hpp"
#include "utility/utility.h"
#include <fstream>
#include <iostream>

struct WAV_MAPPER
{
    std::ifstream stream;
    std::string fn;
    char* readBuf;
    bool matchFound;
};

struct WAV_HEADER
{
    // WAV-формат начинается с RIFF-заголовка:

    // Содержит символы "RIFF" в ASCII кодировке
    // (0x52494646 в big-endian представлении)
    char chunkId[4];

    // 36 + subchunk2Size, или более точно:
    // 4 + (8 + subchunk1Size) + (8 + subchunk2Size)
    // Это оставшийся размер цепочки, начиная с этой позиции.
    // Иначе говоря, это размер файла - 8, то есть,
    // исключены поля chunkId и chunkSize.
    unsigned long chunkSize;

    // Содержит символы "WAVE"
    // (0x57415645 в big-endian представлении)
    char format[4];

    // Формат "WAVE" состоит из двух подцепочек: "fmt " и "data":
    // Подцепочка "fmt " описывает формат звуковых данных:

    // Содержит символы "fmt "
    // (0x666d7420 в big-endian представлении)
    char subchunk1Id[4];

    // 16 для формата PCM.
    // Это оставшийся размер подцепочки, начиная с этой позиции.
    unsigned long subchunk1Size;

    // Аудио формат, полный список можно получить здесь
    // http://audiocoding.ru/wav_formats.txt Для PCM = 1 (то есть, Линейное
    // квантование). Значения, отличающиеся от 1, обозначают некоторый формат
    // сжатия.
    unsigned short audioFormat;

    // Количество каналов. Моно = 1, Стерео = 2 и т.д.
    unsigned short numChannels;

    // Частота дискретизации. 8000 Гц, 44100 Гц и т.д.
    unsigned long sampleRate;

    // sampleRate * numChannels * bitsPerSample/8
    unsigned long byteRate;

    // numChannels * bitsPerSample/8
    // Количество байт для одного сэмпла, включая все каналы.
    unsigned short blockAlign;

    // Так называемая "глубиная" или точность звучания. 8 бит, 16 бит и т.д.
    unsigned short bitsPerSample;

    // Подцепочка "data" содержит аудио-данные и их размер.

    // Содержит символы "data"
    // (0x64617461 в big-endian представлении)
    char subchunk2Id[4];

    // numSamples * numChannels * bitsPerSample/8
    // Количество байт в области данных.
    unsigned long subchunk2Size;

    // Далее следуют непосредственно Wav данные.
};

int main(int argc, char** argv)
{
    Flexibity::programOptions options;

    std::string imgName;
    uint32_t numTracks = 34;  // default is 34 (arm all)
    uint32_t channelBlockSize = 0x8000;
    uint32_t byteOffset = 0;
    uint32_t offset = 0;
    uint32_t count = 0;
    uint32_t mode = 0;
    uint32_t repition = 8;
    std::string dest;

    char* readBuf = new char[channelBlockSize];

    options.desc.add_options()(
        "img,i", Flexibity::po::value<std::string>(&imgName)->required(),
        "Define the image filename to recover from")(
        "tracks,t", Flexibity::po::value<uint32_t>(&numTracks),
        "Define the number of tracks recorded")(
        "offset,o", Flexibity::po::value<uint32_t>(&offset),
        "Define the offset in the image")(
        "bOffset,b", Flexibity::po::value<uint32_t>(&byteOffset),
        "Define the Byte Offset in audio stream")(
        "count,c", Flexibity::po::value<uint32_t>(&count),
        "Define the blocks Count")("mode,m",
                                   Flexibity::po::value<uint32_t>(&mode),
                                   "Define the work mode")(
        "repitition,r", Flexibity::po::value<uint32_t>(&repition),
        "Define channel repitition count")(
        "dest,d", Flexibity::po::value<std::string>(&dest),
        "Define the destination folder");

    options.parse(argc, argv);

    std::ifstream img(imgName);
    std::ofstream of("out.wav");

    if (!img.is_open())
    {
        GINFO("Unable to open image " << imgName << " with " << img.rdstate());
        return 1;
    }

    img.seekg(offset);

    if (mode == 0)
    {  // dumb recovery for healthy FS

        auto countStart = count;

        auto start = img.tellg();
        img.read(readBuf, channelBlockSize);
        GINFO("Read header: " << std::hex << channelBlockSize
                              << " from: " << std::hex << start
                              << " to: " << std::hex << img.tellg());

        // skip all channels
        img.seekg(channelBlockSize * (numTracks - 1), std::ios_base::cur);
        GINFO("Finalizing iter " << countStart - count + 1
                                 << " Seeking to: " << std::hex << img.tellg());
        of.write(readBuf, channelBlockSize);
        GINFO(Flexibity::log::dump(readBuf, channelBlockSize));

        while (count)
        {
            for (int i = repition; i > 0; --i)
            {
                img.read(readBuf, channelBlockSize);
                of.write(readBuf, channelBlockSize);
            }
            img.seekg(channelBlockSize * (numTracks - 1) * repition,
                      std::ios_base::cur);
            --count;
        }

        // GINFO(Flexibity::log::dump((const char *) &readBuf,
        // channelBlockSize));

        of.close();
    }
    else if (mode == 1)
    {  // sector mapping to study the write sequence pattern
        auto maps = new WAV_MAPPER[numTracks];

        // init/open streams
        for (uint32_t i = 0; i < numTracks; ++i)
        {
            std::ostringstream fn;
            fn << i + 1 << ".audio(0).wav";
            auto& item = maps[i];
            item.fn = fn.str();

            item.readBuf = new char[channelBlockSize];

            fn.str("");
            fn.clear();

            fn << dest << "/" << item.fn;

            item.stream.open(fn.str());
            if (!item.stream.is_open())
            {
                GINFO("Unable to open target file " << fn.str() << " with "
                                                    << item.stream.rdstate());
                return 2;
            }
        }

        for (uint32_t i = 0; i < numTracks; ++i)
        {
            auto& item = maps[i];
            GINFO("Track " << i << ": Reading " << std::hex << channelBlockSize
                           << " bytes " << std::hex << item.stream.tellg());
            item.stream.read(item.readBuf, channelBlockSize);
        }

        for (uint32_t j = 0; j < count; ++j)
        {

            if (img.eof())
            {
                GERROR("Img eof!");
                return 4;
            }

            auto startPos = img.tellg();
            GINFO("Iter " << j << " Reading img " << std::hex
                          << channelBlockSize << " bytes " << std::hex
                          << startPos);
            img.read(readBuf, channelBlockSize);

            bool matchFound = false;

            for (uint32_t i = 0; i < numTracks; ++i)
            {
                auto& item = maps[i];

                if (!item.stream.eof() &&
                    memcmp(item.readBuf, readBuf, channelBlockSize) == 0)
                {

                    GINFO("Match for track " << i + 1 << " at iter " << j
                                             << " offs: " << std::hex
                                             << startPos);
                    GINFO("Track " << i + 1 << ": Reading " << std::hex
                                   << channelBlockSize << " bytes at "
                                   << std::hex << item.stream.tellg());
                    matchFound = true;
                    item.stream.read(item.readBuf, channelBlockSize);

                    break;
                }
            }

            if (!matchFound)
            {
                auto hasValidStreams = false;
                for (uint32_t i = 0; i < numTracks; ++i)
                {
                    auto& item = maps[i];
                    hasValidStreams |=
                        !(item.stream.eof() || item.stream.fail());
                }
                if (!hasValidStreams)
                {
                    GERROR("Unable to find Match at img " << std::hex
                                                          << startPos);
                    return 2;
                }
            }
        }

        delete[] maps;
    }
    else if (mode == 2)
    {  // sector mapping to study the write sequence pattern
        auto maps = new WAV_MAPPER[numTracks];

        // init/open streams
        for (uint32_t i = 0; i < numTracks; ++i)
        {
            std::ostringstream fn;
            fn << i + 1 << ".audio(0).wav";
            auto& item = maps[i];
            item.fn = fn.str();

            item.readBuf = new char[channelBlockSize];

            fn.str("");
            fn.clear();

            fn << dest << "/" << item.fn;

            item.stream.open(fn.str());
            if (!item.stream.is_open())
            {
                GINFO("Unable to open target file " << fn.str() << " with "
                                                    << item.stream.rdstate());
                return 2;
            }
            GINFO("Track " << i << ": Reading " << std::hex << channelBlockSize
                           << " bytes " << std::hex << item.stream.tellg());
            item.stream.read(item.readBuf, channelBlockSize);
            item.matchFound = false;
        }

        for (uint32_t j = 0; j < count; ++j)
        {

            if (img.eof())
            {
                GERROR("Img eof!");
                return 4;
            }

            auto startPos = img.tellg();
            GINFO("Iter " << j << " Reading img " << std::hex
                          << channelBlockSize << " bytes " << std::hex
                          << startPos);
            img.read(readBuf, channelBlockSize);

            bool matchFound = false;

            for (uint32_t i = 0; i < numTracks; ++i)
            {
                auto& item = maps[i];

                if (!item.matchFound &&
                    memcmp(item.readBuf, readBuf, channelBlockSize) == 0)
                {
                    if (item.stream.eof())
                    {
                        GERROR("Track " << i + 1 << ": eof!");
                        return 3;
                    }

                    GINFO("Match for track " << i + 1 << " at iter " << j
                                             << " offs: " << std::hex
                                             << startPos);

                    matchFound = true;
                    item.matchFound = true;
                    break;
                }
            }

            bool exhausted = true;

            for (uint32_t i = 0; i < numTracks; ++i)
            {
                auto& item = maps[i];
                exhausted &= item.matchFound;
                GINFO("Track " << i << " " << item.matchFound);
            }

            if (exhausted)
            {
                GINFO("All tracks are exhausted! Re-reading!");
                for (uint32_t i = 0; i < numTracks; ++i)
                {
                    auto& item = maps[i];
                    item.matchFound = false;
                    item.stream.read(item.readBuf, channelBlockSize);
                }
            }

            if (matchFound)
            {
                GINFO("Match found at img " << std::hex << startPos
                                            << ", continue");
                continue;
            }

            if (!matchFound && !exhausted)
            {
                GERROR("Unable to find Match at img " << std::hex << startPos);
                return 2;
            }
        }

        delete[] maps;
    }

    img.close();

    return 0;
}
