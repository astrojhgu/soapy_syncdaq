#include <SoapySDR/Device.hpp>
#include <SoapySDR/Registry.hpp>
#include <SoapySDR/Types.hpp>
#include <SoapySDR/Formats.h>
#include "utils.hpp"
#include <iostream>
#include <exception>
#include <string>
#include <cstdint>
#include <memory>
#include <format>
#include <cassert>
#include <mutex>
#include <syncdaq.h>
// #include <sdaa_ctrl.h>

using namespace syncdaq;
constexpr double SAMP_RATE = 50e6;

// constexpr int16_t local_port = 3002;
using namespace std;
using namespace SoapySDR;

/***********************************************************************
 * Device interface
 **********************************************************************/
class SyncdaqSDR : public SoapySDR::Device
{
public:
    double f_lo_MHz;
    float voltage_gain;
    uint32_t ip_u32;
    std::unique_ptr<CSdr16Decim, decltype(free_sdr_device) &> device_handler;

public:
    // Implement constructor with device specific arguments...
    SyncdaqSDR() = delete;
    SyncdaqSDR(const std::string ip_str, const int local_port, int port_id, uint32_t dec_shift, uint32_t fir_shift, const char* init_file)
        : SoapySDR::Device(), device_handler(
                                  nullptr, free_sdr_device),
          f_lo_MHz(160.0), voltage_gain(1.0),
          ip_u32(parse_ipv4(ip_str.c_str()))
    {
        auto dev_ptr=make_sdr16_decim_u32(parse_ipv4(ip_str.c_str()), 3001, port_id, &dec_shift, 1, fir_shift, init_file);
        if(dev_ptr==nullptr){
            throw std::runtime_error("no dev found");
        }
        device_handler.reset(dev_ptr);
        if(device_handler.get()==nullptr){
            std::cerr<<"null dev"<<std::endl;
        }
    }

    ~SyncdaqSDR()
    {
        stop_data_stream(device_handler.get());
    }

public:
    std::string getDriverKey(void) const
    {
        return "syncdaq";
    }

    std::string getHardwareKey(void) const
    {
        return format_ipv4(ip_u32);
    }

    Kwargs getHardwareInfo(void) const
    {
        Kwargs result;
        result["vendor"] = "UVWStudio";
        result["smp rate"] = format("{} Sps", SAMP_RATE);
        return result;
    }

    void setFrontendMapping(const int direction, const std::string &mapping)
    {
    }

    size_t getNumChannels(const int direction) const
    {
        return direction == SOAPY_SDR_RX ? 1 : 0;
    }

    Kwargs getChannelInfo(const int direction, const size_t channel) const
    {
        Kwargs result;
        result["Num"] = "1";
        result["max input voltage"] = "1 Vpp";

        return result;
    }

    bool getFullDuplex(const int direction, const size_t channel) const
    {
        return false;
    }

    std::vector<std::string> getStreamFormats(const int direction, const size_t channel) const
    {
        std::vector<std::string> result;
        if (direction == SOAPY_SDR_RX && channel < getNumChannels(direction))
        {
            result.push_back(SOAPY_SDR_CF32);
        }
        return result;
    }

    std::string getNativeStreamFormat(const int direction, const size_t channel, double &fullScale) const
    {
        fullScale = 32767;
        return SOAPY_SDR_CS16;
    }

    SoapySDR::RangeList getFrequencyRange(const int direction, const size_t channel) const
    {
        SoapySDR::Range r(30e6, 1920e6, 100e3);
        return SoapySDR::RangeList{r};
    }

    SoapySDR::RangeList getBandwidthRange(const int direction, const size_t channel) const
    {
        SoapySDR::Range r(SAMP_RATE, SAMP_RATE, 0);
        return SoapySDR::RangeList{r};
    }

    void setBandwidth(const int direction, const size_t channel, const double bw)
    {
    }

    double getBandwidth(const int direction, const size_t channel) const
    {
        return SAMP_RATE;
    }

    // Implement all applicable virtual methods from SoapySDR::Device

    void setFrequency(const int direction, const size_t channel, const double frequency, const Kwargs &args = Kwargs())
    {
        std::cout << "=================================" << std::endl;
        std::cout << "new freq:" << frequency << std::endl;
        f_lo_MHz = frequency / 1e6;
        set_mixer_freq(device_handler.get(), f_lo_MHz, 0);
        std::cout << "=================================" << std::endl;
    }

    double getFrequency(const int direction, const size_t channel) const
    {
        return f_lo_MHz * 1e6;
    }

    SoapySDR::RangeList getSampleRateRange(const int direction, const size_t channel) const
    {
        SoapySDR::Range r(SAMP_RATE, SAMP_RATE, 0);
        return SoapySDR::RangeList{r};
    }

    double getSampleRate(const int direction, const size_t channel) const
    {
        return SAMP_RATE;
    }

    SoapySDR::ArgInfoList getStreamArgsInfo(const int direction, const size_t channel)
    {
        if (direction != SOAPY_SDR_RX)
        {
            throw std::runtime_error("syncdaq is RX only, use SOAPY_SDR_RX");
        }

        SoapySDR::ArgInfoList stream_args;
        ArgInfo f_lo_MHz_arg;
        f_lo_MHz_arg.key = "f_lo_MHz";
        f_lo_MHz_arg.value = "160";
        f_lo_MHz_arg.name = "f_lo_MHz";
        f_lo_MHz_arg.description = "lo frequency";
        f_lo_MHz_arg.units = "MHz";
        f_lo_MHz_arg.type = ArgInfo::FLOAT;

        stream_args.push_back(f_lo_MHz_arg);

        return stream_args;
    }

    Stream *setupStream(
        const int direction,
        const std::string &format,
        const std::vector<size_t> &channels = std::vector<size_t>(),
        const Kwargs &args = Kwargs())
    {
        std::cout << "==========================" << std::endl;
        for (auto &i : args)
        {
            std::cout << i.first << " : " << i.second << std::endl;
        }
        std::cout << "==========================" << std::endl;
        if (direction == SOAPY_SDR_TX)
        {
            throw std::runtime_error("syncdaq is RX only, use SOAPY_SDR_RX");
        }

        if (format != SOAPY_SDR_CF32)
        {

            throw std::runtime_error(std::format("syncdaq only support CF32, not {}", format));
        }

        auto iter = args.find("f_lo_MHz");
        if (iter != args.end())
        {
            // std::cout << "lo ch not given, use default value 1024" << std::endl;
            // device_handler->set_lo_ch(atoi(iter->second.c_str()));
            set_mixer_freq(device_handler.get(), atof(iter->second.c_str()), 0);

            // throw std::runtime_error("lo_ch not given");
        }

        return (Stream *)this;
    }

    int activateStream(
        Stream *stream,
        const int flags = 0,
        const long long timeNs = 0,
        const size_t numElems = 0)
    {
        // device_handler->start();
        start_data_stream(device_handler.get());
        std::cout << "ctrl addr=" << format_ipv4(ip_u32) << std::endl;
        return 0;
    }

    void closeStream(Stream *stream)
    {
        deactivateStream(stream, 0, 0);
    }

    int deactivateStream(
        Stream *stream,
        const int flags = 0,
        const long long timeNs = 0)
    {
        std::cerr << "Stream stopped" << std::endl;
        stop_data_stream(device_handler.get());
        return 0;
    }

    size_t getStreamMTU(Stream *stream) const
    {
        return get_mtu();
    }

    int readStream(
        Stream *stream,
        void *const *buffs,
        const size_t numElems,
        int &flags,
        long long &timeNs,
        const long timeoutUs)
    {
        fetch_data_cf32(device_handler.get(), (CComplexF32 *)buffs[0], numElems);
        auto buff = (CComplexF32 *)buffs[0];

        for (int i = 0; i < numElems; ++i)
        {
            buff[i].re *= voltage_gain;
            buff[i].im *= voltage_gain;
        }
        return numElems;
    }

    std::vector<std::string> listAntennas(const int direction, const size_t channel) const
    {
        if (direction == SOAPY_SDR_RX)
        {
            return std::vector<std::string>{"EXT"};
        }
        else
        {
            return std::vector<std::string>();
        }
    }

    void setAntenna(const int direction, const size_t channel, const std::string &name)
    {
    }

    std::string getAntenna(const int direction, const size_t channel) const
    {
        return std::string("EXT");
    }

    bool hasDCOffsetMode(const int direction, const size_t channel) const
    {
        return false;
    }

    std::vector<std::string> listGains(const int direction, const size_t channel) const
    {
        return std::vector<std::string>{"GAIN"};
    }

    double getGain(const int direction, const size_t channel) const
    {
        return std::log10(voltage_gain) * 20;
    }

    double getGain(const int direction, const size_t channel, const std::string &name) const
    {
        return std::log10(voltage_gain) * 20;
    }

    SoapySDR::Range getGainRange(const int direction, const size_t channel) const
    {
        SoapySDR::Range r(-100.0, 0.0, 0.0);
        return r;
    }

    SoapySDR::Range getGainRange(const int direction, const size_t channel, const std::string &name) const
    {
        SoapySDR::Range r(-100.0, 0.0, 0.0);
        return r;
    }

    bool hasGainMode(const int direction, const size_t channel) const
    {
        return false;
    }

    void setGain(const int direction, const size_t channel, const std::string &name, const double value)
    {
        voltage_gain = std::pow(10.0, value / 20);
        std::cerr << "input= "<<value<<" gain=" << voltage_gain << std::endl;
    }
};

/***********************************************************************
 * Find available devices
 **********************************************************************/
SoapySDR::KwargsList findSyncdaqSDR(const SoapySDR::Kwargs &args)
{
    static std::mutex my_mutex;
    std::lock_guard<std::mutex> lock(my_mutex);
    std::cout << "================" << std::endl;
    std::cout << "finding Syncdaq sdr" << std::endl;
    std::cout << "args:size=" << args.size() << std::endl;
    for (auto &x : args)
    {
        std::cout << x.first << " : " << x.second << std::endl;
    }
    std::cout << "================" << std::endl;
    // locate the device on the system...
    // return a list of 0, 1, or more argument maps that each identify a device
    SoapySDR::Kwargs args1;
    SoapySDR::KwargsList kwl;

    if (args.empty())
    {
        // return kwl;
    }

    // kwl.push_back(args1);

    uint32_t result[255];
    auto iter = args.find("ctrl_ip");
    const char *ctrl_ip = nullptr;

    if (iter != args.end())
    {
        ctrl_ip = iter->second.c_str();
    }

    if (ctrl_ip == nullptr)
    {
        constexpr size_t n_max_dev = 255;
        uint32_t ctrl_ips[n_max_dev];
        size_t ndev = find_all_devices(ctrl_ips, n_max_dev, 3001);
        for (size_t i = 0; i < ndev; ++i)
        {
            auto ip = ctrl_ips[i];
            auto ip1 = (ip & 0xff000000) >> 24;
            auto ip2 = (ip & 0xff0000) >> 16;
            auto ip3 = (ip & 0xff00) >> 8;
            auto ip4 = (ip & 0xff);
            auto ip_str = std::format("{}.{}.{}.{}", ip1, ip2, ip3, ip4);
            std::cout << "found device with ctrl ip: " << ip_str << std::endl;
            SoapySDR::Kwargs args1;
            args1["ctrl_ip"] = ip_str;
            kwl.push_back(args1);
        }
    }
    else
    {
        if (device_is_alive(parse_ipv4(ctrl_ip)))
        {
            std::cout << "device with ctrl ip " << ctrl_ip << " is alive" << std::endl;
            SoapySDR::Kwargs args1;
            args1["ctrl_ip"] = ctrl_ip;
            kwl.push_back(args1);
        }
    }

    std::cout << "kwl size=" << kwl.size() << std::endl;
    std::cout << "-----==================------" << std::endl;

    return kwl;
}

/***********************************************************************
 * Make device instance
 **********************************************************************/
SoapySDR::Device *makeSyncdaqSDR(const SoapySDR::Kwargs &args)
{
    static std::mutex my_mutex;
    std::lock_guard<std::mutex> lock(my_mutex);

    std::cout << "===============" << std::endl;
    std::cout << "making Syncdaq sdr" << std::endl;
    std::cout << "size=" << args.size() << std::endl;
    for (auto &x : args)
    {
        std::cout << x.first << " : " << x.second << std::endl;
    }
    std::cout << "===============" << std::endl;
    // create an instance of the device object given the args
    // here we will translate args into something used in the constructor

    auto iter = args.find("ctrl_ip");
    const char *ctrl_ip = nullptr;
    if (iter != args.end())
    {
        // cfg_file = iter->second.c_str();
        ctrl_ip = iter->second.c_str();
    }
    else
    {
        constexpr size_t n_max_dev = 255;
        uint32_t ctrl_ips[n_max_dev];
        size_t ndev = find_all_devices(ctrl_ips, n_max_dev, 3001);
        if (ndev == 0)
        {
            throw std::runtime_error("no syncdaq device found");
        }
        ctrl_ip = format_ipv4(ctrl_ips[0]).c_str();
    }

    const char* init_file=nullptr;
    iter=args.find("init_file");
    if(iter!=args.end()){
        init_file=iter->second.c_str();
    }

    std::vector<int> shifts;
    iter=args.find("shifts");
    if(iter!=args.end()){
        shifts=parse_int_list(iter->second);
    }

    size_t port_id=0;
    iter = args.find("port_id");
    if(iter!=args.end()){
        port_id=std::stoul(iter->second);
    }
    std::cout<<"port_id="<<port_id<<std::endl;
    
    if (shifts.size()!=2){
        shifts={12,5};
    }
    
    auto result=new SyncdaqSDR(ctrl_ip, 3001, port_id, shifts[0], shifts[1], init_file);
    

    return result;
}

/***********************************************************************
 * Registration
 **********************************************************************/
static SoapySDR::Registry registerSyncdaqSDR("syncdaq", &findSyncdaqSDR, &makeSyncdaqSDR, SOAPY_SDR_ABI_VERSION);
