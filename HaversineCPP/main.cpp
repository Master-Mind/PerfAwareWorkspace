#include <iostream>
#include <fstream>
#include <filesystem>
#include <random>
#include <cassert>
#include <argparse/argparse.hpp>
#include <glaze/glaze.hpp>
#include "Profiler.h"
#include <stdio.h>
std::mutex GlobalProfiler::profilers_mutex;
std::vector<TU_Profiler*> GlobalProfiler::profilers;

PROFILED_FILE;
struct HaversinePair
{
    double x0;
    double y0;
    double x1;
    double y1;
};

struct HaversineData
{
    std::vector<HaversinePair> pairs;
};

/* ========================================================================
   LISTING 65
   ======================================================================== */

typedef double f64;

static f64 Square(f64 A)
{
    f64 Result = (A * A);
    return Result;
}

static f64 RadiansFromDegrees(f64 Degrees)
{
    f64 Result = 0.01745329251994329577f * Degrees;
    return Result;
}

// NOTE(casey): EarthRadius is generally expected to be 6372.8
static f64 ReferenceHaversine(f64 X0, f64 Y0, f64 X1, f64 Y1, f64 EarthRadius)
{
    /* NOTE(casey): This is not meant to be a "good" way to calculate the Haversine distance.
       Instead, it attempts to follow, as closely as possible, the formula used in the real-world
       question on which these homework exercises are loosely based.
    */

    f64 lat1 = Y0;
    f64 lat2 = Y1;
    f64 lon1 = X0;
    f64 lon2 = X1;
    f64 dLat;
    f64 dLon;
    f64 a;
    f64 c;

    {
        SCOPED_TRACE("Convert to radians", sizeof(u64) * 10);
        dLat = RadiansFromDegrees(lat2 - lat1);
        dLon = RadiansFromDegrees(lon2 - lon1);
        lat1 = RadiansFromDegrees(lat1);
        lat2 = RadiansFromDegrees(lat2);
    }

    {
        SCOPED_TRACE("Calc a", sizeof(u64) * 5);
        a = Square(sin(dLat / 2.0)) + cos(lat1) * cos(lat2) * Square(sin(dLon / 2));
    }

    {
        SCOPED_TRACE("Calc c", sizeof(u64) * 2);
        c = 2.0 * asin(sqrt(a));
    }

    f64 Result = EarthRadius * c;

    return Result;
}

void GenData(const std::filesystem::path &input, int seed, int numpairs)
{
    std::ofstream outfile(input);
    const std::size_t charsPerPair = sizeof("{\"x0\":102.1633205722960440, \"y0\":-24.9977499718717624, \"x1\":-14.3322557404258362, \"y1\":62.6708294856625940},");
    std::string outstr = "{\"pairs\":[";

    //outstr.reserve(charsPerPair * numpairs);
    std::ostringstream outstream;

    outstream << "{\"pairs\":[";

    std::mt19937 gen(seed);
    std::uniform_real_distribution lat(-180.0f, 180.0f);
    std::uniform_real_distribution longtitude(-90.0f, 90.0f);//I can't remember if I got these right and I don't care enough to check

    for (int i = 0; i < numpairs; i++)
    {
        outstream << "{\"x0\":" << lat(gen) << ", \"y0\":" << longtitude(gen) << ", \"x1\":" << lat(gen) << ", \"y1\":" << longtitude(gen) << "},";
    }

    outstream.seekp(-1, outstream.cur);
    outstream << "]}";

    outfile << outstream.str();
}

void run_reference_haversine(const std::filesystem::path& input)
{
    FILE* infile = nullptr;
    SCOPED_TRACE("Total reference haversine", 0);
    {
        SCOPED_TRACE("Setup", 0);
        infile = fopen(input.string().c_str(), "rb");

        assert(infile);
    }

    std::string contents;
    HaversineData dat;
    std::size_t fsize = 0;

    { 
        fsize = std::filesystem::file_size(input);
        SCOPED_TRACE("Load file", fsize);
        contents.resize(fsize);

        if (infile)
        {
            size_t readNum = fread(&contents[0],1, contents.size(), infile);

            assert(readNum == fsize);

            fclose(infile);
        }
        else
        {
            return;
        }
    }


    {
        SCOPED_TRACE("Parse file", fsize);
        auto data = glz::read_json<HaversineData>(contents);

        if (!data)
        {
            std::cerr << data.error() << std::endl;
            assert(false);
        }

        dat = data.value();
    }

    f64 sum = 0;
    
    {

        for (size_t i = 0; i < dat.pairs.size(); i++)
        {
            SCOPED_TRACE("Run reference haversine", sizeof(HaversinePair));
            const HaversinePair& curpair = dat.pairs[i];
            sum += ReferenceHaversine(curpair.x0, curpair.y0, curpair.x1, curpair.y1, 6372.8);
        }
    }


    {
        SCOPED_TRACE("Print reference haversine results", 24);
        std::cout << "Found a sum of: " << sum << std::endl;
    }
}

void read_test(const std::filesystem::path& input, u64 cpu_frequency)
{
    u64 min = ULLONG_MAX;
    u64 max = 0;

    double average = 0;
    double standard_deviation = 0;
    const int iterations = 20;
    u64 times[iterations];
    auto fsize = std::filesystem::file_size(input);
    double dsize = static_cast<double>(fsize);
    double dfreq = static_cast<double>(cpu_frequency);

    std::cout << "Testing file io for " << input << std::endl;

    for (int i = 0; i < iterations; i++)
    {
        std::string contents;
        contents.resize(fsize);
        u64 start = __rdtsc();

        FILE* infile = nullptr;
        infile = fopen(input.string().c_str(), "rb");

        if (infile)
        {
            fread(&contents[0], contents.size(), 1, infile);

            fclose(infile);
        }

        u64 dur = __rdtsc() - start;

        min = std::min(min, dur);
        max = std::max(max, dur);
        times[i] = dur;
        average += static_cast<double>(dur) / iterations;
    }

    for (int i = 0; i < iterations; i++)
    {
        double diff = (times[i] - average);
        standard_deviation += diff * diff / iterations;
    }

    standard_deviation = sqrt(standard_deviation);

    std::string bytestr = humanSize(fsize);

    auto printTime = [=](u64 cycles, const char* label) {
            double secs = static_cast<double>(cycles) / dfreq;
            std::string rateStr = humanSize(static_cast<u64>(dsize / secs));

            std::cout << label << cycles << ", " << secs << "s, " << rateStr << 's' << std::endl;
        };

    double sdsecs = static_cast<double>(standard_deviation) / dfreq;

    std::cout << "Results for a " << bytestr << " file:" << std::endl;
    printTime(min, "Min: ");
    printTime(max, "Max: ");
    printTime(average, "Avg: ");
    std::cout << "SD:" << standard_deviation << ", " << sdsecs << "s, " << standard_deviation / average << '%' << std::endl;
    printTime(times[0], "First: ");
    printTime(times[1], "Second: ");
    printTime(times[2], "Third: ");
}
#define LOOP_BODY(num) arr[j + num] = static_cast<char>(j + num)
extern "C" void MOVAllBytesASM(u64 count, u8 *data);
extern "C" void NOPAllBytesASM(u64 count);
extern "C" void CMPAllBytesASM(u64 count);
extern "C" void DECAllBytesASM(u64 count);
extern "C" void NOOP1ASM(u64 count);
extern "C" void NOOP2ASM(u64 count);
extern "C" void NOOP4ASM(u64 count);
extern "C" void NOOP8ASM(u64 count);
extern "C" void NOOP16ASM(u64 count);
extern "C" void READ_8x1(u64 count, u8* data);
extern "C" void READ_8x2(u64 count, u8* data);
extern "C" void READ_8x3(u64 count, u8* data);
extern "C" void READ_8x4(u64 count, u8* data);
extern "C" void READ_16x2(u64 count, u8* data);
extern "C" void READ_32x2(u64 count, u8* data);
extern "C" void READ_32x4(u64 count, u8* data);
extern "C" void TEST_CACHE(u64 count, u8* data, u64 mask);
void mem_test()
{
    constexpr int num_reps = 10;
    constexpr size_t buff_size = 2u << 29;
    u8* tempdata = static_cast<u8 *>(malloc(buff_size));
    u8* data = tempdata + 1;

    {
        SCOPED_TRACE("Mem Test READ 32x4", buff_size * num_reps);

        for (int i = 0; i < num_reps; i++)
        {
            READ_32x4(buff_size, data);
        }
    }

    {
        SCOPED_TRACE("Page fault cleaner", buff_size * num_reps);

        for (int i = 0; i < num_reps; ++i)
        {
            TEST_CACHE(buff_size, data, 0xFFFFFF);
        }
    }

    {
        SCOPED_TRACE("Cache Test 256b", buff_size * num_reps);

        for (int i = 0; i < num_reps; ++i)
        {
            TEST_CACHE(buff_size, data, 0xFF);
        }
    }

    {
        SCOPED_TRACE("Cache Test 64kb", buff_size * num_reps);

        for (int i = 0; i < num_reps; ++i)
        {
            TEST_CACHE(buff_size, data, 0xFFFF);
        }
    }

    {
        SCOPED_TRACE("Cache Test 1mb", buff_size * num_reps);

        for (int i = 0; i < num_reps; ++i)
        {
            TEST_CACHE(buff_size, data, 0xFFFFF);
        }
    }

    {
        SCOPED_TRACE("Cache Test 16mb", buff_size * num_reps);

        for (int i = 0; i < num_reps; ++i)
        {
            TEST_CACHE(buff_size, data, 0xFFFFFF);
        }
    }

    {
        SCOPED_TRACE("Cache Test 268mb", buff_size * num_reps);

        for (int i = 0; i < num_reps; ++i)
        {
            TEST_CACHE(buff_size, data, 0xFFFFFFF);
        }
    }

    free(tempdata);

    return;

    {
        SCOPED_TRACE("Mem Test READ     1", buff_size * num_reps);

        for (int i = 0; i < num_reps; i++)
        {
            READ_8x1(buff_size, data);
        }
    }

    {
        SCOPED_TRACE("Mem Test READ     2", buff_size * num_reps);

        for (int i = 0; i < num_reps; i++)
        {
            READ_8x2(buff_size, data);
        }
    }

    {
        SCOPED_TRACE("Mem Test READ     3", buff_size * num_reps);

        for (int i = 0; i < num_reps; i++)
        {
            READ_8x3(buff_size, data);
        }
    }

    {
        SCOPED_TRACE("Mem Test READ     4", buff_size * num_reps);

        for (int i = 0; i < num_reps; i++)
        {
            READ_8x4(buff_size, data);
        }
    }

    {
        SCOPED_TRACE("Mem Test READ 16x2", buff_size * num_reps);

        for (int i = 0; i < num_reps; i++)
        {
            READ_16x2(buff_size, data);
        }
    }

    {
        SCOPED_TRACE("Mem Test READ 32x2", buff_size * num_reps);

        for (int i = 0; i < num_reps; i++)
        {
            READ_32x2(buff_size, data);
        }
    }

    {
        SCOPED_TRACE("Mem Test NOOP 1", buff_size * num_reps);

        for (int i = 0; i < num_reps; i++)
        {
            NOOP16ASM(buff_size);
        }
    }

    {
        SCOPED_TRACE("Mem Test MOV", buff_size * num_reps);

        for (int i = 0; i < num_reps; i++)
        {
            u8* arr = static_cast<u8*>(malloc(buff_size));

            MOVAllBytesASM(buff_size, arr);

            //for (int j = 0; j < buff_size; j += 2)
            //{
            //}


            free(arr);
        }
    }

    {
        SCOPED_TRACE("Mem Test MOV", buff_size * num_reps);

        for (int i = 0; i < num_reps; i++)
        {
            u8* arr = static_cast<u8*>(malloc(buff_size));

            NOPAllBytesASM(buff_size);

            //for (int j = 0; j < buff_size; j += 2)
            //{
            //}


            free(arr);
        }
    }

    {
        SCOPED_TRACE("Mem Test MOV", buff_size * num_reps);

        for (int i = 0; i < num_reps; i++)
        {
            u8* arr = static_cast<u8*>(malloc(buff_size));

            CMPAllBytesASM(buff_size);

            //for (int j = 0; j < buff_size; j += 2)
            //{
            //}


            free(arr);
        }
    }

    {
        SCOPED_TRACE("Mem Test MOV", buff_size * num_reps);

        for (int i = 0; i < num_reps; i++)
        {
            u8* arr = static_cast<u8*>(malloc(buff_size));

            DECAllBytesASM(buff_size);

            //for (int j = 0; j < buff_size; j += 2)
            //{
            //}


            free(arr);
        }
    }
}

int main(int argc, char* argv[])
{
    auto start = std::chrono::high_resolution_clock::now();
    argparse::ArgumentParser program("haversine");

    program.add_argument("--gen", "-g")
        .help("Generate haversine input data instead of processing it")
        .flag();

    program.add_argument("--test_read", "-r")
        .help("Run a repitition test on file reads")
        .flag();

    program.add_argument("--mem_test", "-m")
        .help("Run a memory bandwidth test")
        .flag();

    program.add_argument("--numpairs", "-n")
        .help("Number of pairs to generate")
        .default_value(100)
        .scan<'i', int>();

    program.add_argument("--seed", "-s")
        .help("Seed for generation")
        .default_value(123)
        .scan<'i', int>();

    program.add_argument("--input_filename", "-i")
        .help("Input file to generate/parse")
        .default_value("input.json");

    try
    {
        program.parse_args(argc, argv);
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        std::cerr << program;
        std::exit(1);
    }

    std::cout << "Page faults: " << add_PROFILED_FILE_to_the_top_of_this_file.handler.GetPageFaults() << std::endl;

    u64 cpu_frequency = GuesstimateCPUFrequency();
    auto input_path = std::filesystem::path(program.get<std::string>("--input_filename"));

    std::cout << "CPU has a clock frequency of: " << cpu_frequency / 1'000'000'000.0 << "ghz" << std::endl;

    if (program["--gen"] == true)
    {
        GenData(input_path, program.get<int>("--seed"), program.get<int>("--numpairs"));
    }
    else if (program["--test_read"] == true)
    {
        read_test(input_path, cpu_frequency);
    }
    else if (program["--mem_test"] == true)
    {
        mem_test();
    }
    else
    {
        run_reference_haversine(input_path);
    }

    std::cout << "Page faults: " << add_PROFILED_FILE_to_the_top_of_this_file.handler.GetPageFaults() << std::endl;

    GlobalProfiler::print_stats(cpu_frequency);

    std::cout << "In total, this program took " << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start) << " to run" << std::endl;

	return 0;
}