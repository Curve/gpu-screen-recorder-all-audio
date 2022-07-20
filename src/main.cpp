#include <iostream>
#include <optional>
#include <process.hpp>
#include <rohrkabel/loop/main.hpp>
#include <rohrkabel/registry/registry.hpp>

#include <unistd.h>
#include <sys/wait.h>

int main(int argc, char **argv)
{
    namespace pw = pipewire;

    if (argc < 2)
    {
        std::cerr << "Usage: " << argv[0] << " <gpu_screen_recorder_path>" << std::endl;
        return 1;
    }

    auto main_loop = pw::main_loop();
    auto context = pw::context(main_loop);
    auto core = pw::core(context);
    auto reg = pw::registry(core);

    std::vector<pw::global> globals;
    auto listener = reg.listen<pw::registry_listener>();
    listener.on<pw::registry_event::global>([&](const pw::global &global) { globals.emplace_back(global); });

    core.update();

    std::string audio_source, audio_sink;

    for (const auto &global : globals)
    {
        if (global.type == pw::metadata::type)
        {
            auto metadata = reg.bind<pw::metadata>(global.id);
            auto properties = metadata.properties();

            if (properties.count("default.audio.source"))
            {
                audio_source = properties["default.audio.source"].value;
                audio_source = audio_source.substr(audio_source.find_first_of(':') + 2, audio_source.find_last_of('"') - (audio_source.find_first_of(':') + 2));
            }

            if (properties.count("default.audio.sink"))
            {
                audio_sink = properties["default.audio.sink"].value;
                audio_sink = audio_sink.substr(audio_sink.find_first_of(':') + 2, audio_sink.find_last_of('"') - (audio_sink.find_first_of(':') + 2));
            }
        }
    }

    std::cout << "Default Sink\t\t: " << audio_sink << std::endl;
    std::cout << "Default Source\t\t: " << audio_source << std::endl;

    std::optional<pw::node> source, sink;

    for (const auto &global : globals)
    {
        if (global.type == pw::node::type)
        {
            auto node = reg.bind<pw::node>(global.id);
            auto props = node.info().props;

            if (props["node.name"] == audio_source)
            {
                source.emplace(std::move(node));
            }

            if (props["node.name"] == audio_sink)
            {
                sink.emplace(std::move(node));
            }
        }
    }

    if (!source || !sink)
    {
        std::cerr << "Failed to find source or sink" << std::endl;
        return 1;
    }

    std::cout << "Sink Node\t\t: " << sink->id() << std::endl;
    std::cout << "Source Node\t\t: " << source->id() << std::endl;

    auto old_size = globals.size() + 1;

    auto virtual_device = core.create("adapter",
                                      {
                                          {"node.name", "All Audio (GPU-Screen-Recorder)"}, //
                                          {"factory.name", "support.null-audio-sink"},      //
                                          {"media.class", "Audio/Source/Virtual"},          //
                                          {"audio.channels", "FL, FR"}                      //
                                      },
                                      pw::node::type, pw::node::version);

    std::cout << "Virtual Device Node\t: " << virtual_device.id() << std::endl;

    while (globals.size() == old_size)
    {
        core.update();
    }

    std::vector<pw::port> source_ports, sink_ports, virtual_device_ports;

    for (const auto &global : globals)
    {
        if (global.type == pw::port::type)
        {
            auto port = reg.bind<pw::port>(global.id);
            auto props = port.info().props;

            if (props["node.id"] == std::to_string(virtual_device.id()))
            {
                virtual_device_ports.emplace_back(std::move(port));
            }

            if (props["node.id"] == std::to_string(source->id()))
            {
                source_ports.emplace_back(std::move(port));
            }

            if (props["node.id"] == std::to_string(sink->id()))
            {
                sink_ports.emplace_back(std::move(port));
            }
        }
    }

    std::cout << "Sink Ports\t\t: [";
    for (auto i = 0u; sink_ports.size() > i; i++)
    {
        std::cout << sink_ports[i].id();
        if (i != (sink_ports.size() - 1))
        {
            std::cout << ", ";
        }
    }
    std::cout << "]" << std::endl;

    std::cout << "Source Ports\t\t: [";
    for (auto i = 0u; source_ports.size() > i; i++)
    {
        std::cout << source_ports[i].id();
        if (i != (source_ports.size() - 1))
        {
            std::cout << ", ";
        }
    }
    std::cout << "]" << std::endl;

    std::cout << "Virtual Device Ports\t: [";
    for (auto i = 0u; virtual_device_ports.size() > i; i++)
    {
        std::cout << virtual_device_ports[i].id();
        if (i != (virtual_device_ports.size() - 1))
        {
            std::cout << ", ";
        }
    }
    std::cout << "]" << std::endl;

    std::vector<pw::link_factory> links;

    for (const auto &virtual_device_port : virtual_device_ports)
    {
        auto virtual_device_port_info = virtual_device_port.info();

        if (virtual_device_port_info.direction != pw::port_direction::input)
        {
            continue;
        }

        for (const auto &sink_port : sink_ports)
        {
            auto sink_port_info = sink_port.info();

            if (sink_port_info.direction != pw::port_direction::output)
            {
                continue;
            }

            if (sink_port_info.props["audio.channel"] == virtual_device_port_info.props["audio.channel"])
            {
                links.emplace_back(core.create<pw::link_factory>({virtual_device_port.id(), sink_port.id()}));
            }
        }

        for (const auto &source_port : source_ports)
        {
            auto source_port_info = source_port.info();

            if (source_port_info.direction != pw::port_direction::output)
            {
                continue;
            }

            if (source_port_info.props["audio.channel"] == virtual_device_port_info.props["audio.channel"])
            {
                links.emplace_back(core.create<pw::link_factory>({virtual_device_port.id(), source_port.id()}));
            }
        }
    }

    std::cout << "Link-Factories\t\t: [";
    for (auto i = 0u; links.size() > i; i++)
    {
        std::cout << links[i].id();
        if (i != (links.size() - 1))
        {
            std::cout << ", ";
        }
    }
    std::cout << "]" << std::endl;

    std::vector<std::string> args{argv[1]};

    for (auto i = 2; argc > i; i++)
    {
        args.emplace_back(argv[i]);
    }

    std::cout << "Arguments\t\t: [";
    for (auto i = 0u; args.size() > i; i++)
    {
        std::cout << "\"" << args[i] << "\"";
        if (i != (args.size() - 1))
        {
            std::cout << ", ";
        }
    }
    std::cout << "]" << std::endl;

    TinyProcessLib::Process recorder{args};
    return recorder.get_exit_status();
}