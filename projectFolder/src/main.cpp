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
    uint32_t chunkSize;

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
    uint32_t subchunk1Size;

    // Аудио формат, полный список можно получить здесь
    // http://audiocoding.ru/wav_formats.txt Для PCM = 1 (то есть, Линейное
    // квантование). Значения, отличающиеся от 1, обозначают некоторый формат
    // сжатия.
    uint16_t audioFormat;

    // Количество каналов. Моно = 1, Стерео = 2 и т.д.
    uint16_t numChannels;

    // Частота дискретизации. 8000 Гц, 44100 Гц и т.д.
    uint32_t sampleRate;

    // sampleRate * numChannels * bitsPerSample/8
    uint32_t byteRate;

    // numChannels * bitsPerSample/8
    // Количество байт для одного сэмпла, включая все каналы.
    uint16_t blockAlign;

    // Так называемая "глубиная" или точность звучания. 8 бит, 16 бит и т.д.
    uint16_t bitsPerSample;

    // Подцепочка "data" содержит аудио-данные и их размер.

    // Содержит символы "data"
    // (0x64617461 в big-endian представлении)
    char subchunk2Id[4];

    // numSamples * numChannels * bitsPerSample/8
    // Количество байт в области данных.
    uint32_t subchunk2Size;

    // Далее следуют непосредственно Wav данные.
};

struct OPTIONS {

    std::string imgName;
    std::string offsStr;
    uint32_t numTracks = 34;  // default is 34 (arm all)
    uint32_t channelBlockSize = 0x8000;
    uint32_t byteOffset = 0;
    uint64_t offset = 0;
    uint32_t count = 0;
    uint32_t mode = 0;
    uint32_t repition = 8;
    uint32_t selected = 1;
    bool dummyRead = false;
    std::string dest;
    std::string ofName = "out.wav";
};

char* readBuf = nullptr;
std::ofstream of;
std::ifstream img;

void doRecover (OPTIONS &opts) {
    of.open(opts.ofName);

    auto offset = opts.offset;
    auto count = opts.count;
    auto channelBlockSize = opts.channelBlockSize;
    auto selected = opts.selected;
    auto numTracks = opts.numTracks;
    auto repition = opts.repition;
    auto dest = opts.dest;

    auto countStart = count;

    GINFO("Seeking image to " << std::hex << offset);
    img.seekg(offset);

    WAV_HEADER wh = {
        .chunkId = {'R','I','F','F'},
        .chunkSize = 0x7FFFFF98,
        .format = {'W','A','V','E'},
        .subchunk1Id = {'f','m','t', ' '},
        .subchunk1Size = 16,
        .audioFormat = 1,
        .numChannels = 1,
        .sampleRate = 48000, //TODO: add to params
        .byteRate = 144000,
        .blockAlign = 3,
        .bitsPerSample = 24,
        .subchunk2Id = {'d','a','t','a'},
        .subchunk2Size = 0x7FFFFF74

    };


    of.write((char *)&wh, sizeof(wh)); 

    img.seekg(channelBlockSize * (selected - 1) * repition,
                  std::ios_base::cur);

    while (count)
    {
        for (auto i = repition; i > 0; --i)
        {
            auto startPos = img.tellg();
            
            img.read(readBuf, channelBlockSize);
            GINFO("Read data: " << std::hex << channelBlockSize
                          << " from: " << std::hex << startPos
                          << " to: " << std::hex << img.tellg()); 
            GDEBUG(Flexibity::log::dump(readBuf, channelBlockSize));
            of.write(readBuf, channelBlockSize);
        }
        img.seekg(channelBlockSize * (numTracks - 1) * repition,
                  std::ios_base::cur);
        GINFO("Finalizing iter " << countStart - count + 1
                             << " Seeking to: " << std::hex << img.tellg());

        --count;
    }

    of.close();
};

int main(int argc, char** argv)
{
    Flexibity::programOptions options;

    OPTIONS opts;

    options.desc.add_options()(
        "img,i", Flexibity::po::value<std::string>(&opts.imgName)->required(),
        "Define the image filename to recover from")(
        "tracks,t", Flexibity::po::value<uint32_t>(&opts.numTracks),
        "Define the number of tracks recorded")(
        "offset,o", Flexibity::po::value<std::string>(&opts.offsStr),
        "Define the offset in the image")(
        "bOffset,b", Flexibity::po::value<uint32_t>(&opts.byteOffset),
        "Define the Byte Offset in audio stream")(
        "count,c", Flexibity::po::value<uint32_t>(&opts.count),
        "Define the blocks Count")("mode,m",
                                   Flexibity::po::value<uint32_t>(&opts.mode),
                                   "Define the work mode")(
        "repitition,r", Flexibity::po::value<uint32_t>(&opts.repition),
        "Define channel repitition count")(
        "dest,d", Flexibity::po::value<std::string>(&opts.dest),
        "Define the destination folder")(
        "selected,s", Flexibity::po::value<uint32_t>(&opts.selected),
        "Define selected channel for recovery")(
        "dummy,u", Flexibity::po::value<bool>(&opts.dummyRead),
        "Use dummy read (experiment)")
        ;

    
    {
        options.parse(argc, argv);
        char* end = nullptr;
        opts.offset = strtoull(opts.offsStr.c_str(), &end, 0);
    }

    readBuf = new char[opts.channelBlockSize];

    img.open(opts.imgName);
    GINFO("Opening image " << opts.imgName);
    if (!img.is_open())
    {
        GINFO("Unable to open image " << opts.imgName << " with " << img.rdstate());
        return 1;
    }

    auto mode = opts.mode;

    if (mode == 0)
    {  // dumb recovery for healthy FS

        auto offset = opts.offset;
        auto count = opts.count;
        auto channelBlockSize = opts.channelBlockSize;
        auto selected = opts.selected;
        auto numTracks = opts.numTracks;
        auto repition = opts.repition;

        auto countStart = count;

        GINFO("Seeking image to " << std::hex << offset);
        img.seekg(offset);

        of.open(opts.ofName);

        auto start = img.tellg();
        img.seekg(channelBlockSize * (selected - 1), std::ios_base::cur);
        img.read(readBuf, channelBlockSize);
        GINFO("Read header: " << std::hex << channelBlockSize
                              << " from: " << std::hex << start
                              << " to: " << std::hex << img.tellg());

        // skip all channels
        img.seekg(channelBlockSize * (numTracks - selected), std::ios_base::cur);
        GINFO("Finalizing iter " << countStart - count + 1
                                 << " Seeking to: " << std::hex << img.tellg());
        of.write(readBuf, channelBlockSize);
        GDEBUG(Flexibity::log::dump(readBuf, channelBlockSize));

        img.seekg(channelBlockSize * (selected - 1) * repition,
                      std::ios_base::cur);

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

        
        auto offset = opts.offset;
        auto count = opts.count;
        auto channelBlockSize = opts.channelBlockSize;
        auto numTracks = opts.numTracks;
        auto dest = opts.dest;

        auto maps = new WAV_MAPPER[numTracks];

        GINFO("Seeking image to " << std::hex << offset);
        img.seekg(offset);

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

        auto offset = opts.offset;
        auto count = opts.count;
        auto channelBlockSize = opts.channelBlockSize;
        auto numTracks = opts.numTracks;
        auto dest = opts.dest;

        GINFO("Seeking image to " << std::hex << offset);
        img.seekg(offset);

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
    }if (mode == 3)
    {  // actual recovery for unsaved session

        doRecover(opts);
    }

    img.close();

    return 0;
}
