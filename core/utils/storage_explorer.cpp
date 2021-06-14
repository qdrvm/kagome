/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <boost/program_options.hpp>
#include <libp2p/log/configurator.hpp>

#include "application/impl/app_configuration_impl.hpp"
#include "blockchain/impl/block_tree_impl.hpp"
#include "injector/application_injector.hpp"
#include "log/configurator.hpp"

class CommandParser {
 public:
  using CommandFunctor = std::function<void(int, char **)>;

  void addCommand(std::string command,
                  std::string description,
                  CommandFunctor functor) {
    commands_.insert({command, std::move(functor)});
    command_descriptions_.insert({std::move(command), std::move(description)});
  }

  void invoke(int argc, char **argv) const {
    if (argc < 2) {
      std::cerr << "Unspecified command!\nAvailable commands are:\n";
      printCommands(std::cerr);
    }
    if (auto command = commands_.find(argv[1]); command != commands_.cend()) {
      command->second(argc - 1, argv + 1);
    } else {
      std::cerr << "Unknown command '" << argv[1]
                << "'!\nAvailable commands are:\n";
      printCommands(std::cerr);
    }
  }

  void printCommands(std::ostream &out) const {
    for (auto &[command, description] : command_descriptions_) {
      out << command << "\t" << description << "\n";
    }
  }

 private:
  std::unordered_map<std::string, CommandFunctor> commands_;
  std::unordered_map<std::string, std::string> command_descriptions_;
};

boost::optional<kagome::primitives::BlockId> parseBlockId(char *string) {
  kagome::primitives::BlockId id;
  if (strlen(string) == 2 * kagome::primitives::BlockHash::size()) {
    kagome::primitives::BlockHash id_hash{};
    if (auto id_bytes = kagome::common::unhex(string); id_bytes) {
      std::copy_n(id_bytes.value().begin(),
                  kagome::primitives::BlockHash::size(),
                  id_hash.begin());
    } else {
      std::cerr << "Invalid block hash!\n";
      return boost::none;
    }
    id = std::move(id_hash);
  } else {
    try {
      id = std::stoi(string);
    } catch (std::invalid_argument &) {
      std::cerr << "Block number must be a hex hash or a number!\n";
      return boost::none;
    }
  }
  return id;
}

int main(int argc, char **argv) {
  CommandParser parser;
  parser.addCommand("help", "print help message", [&parser](int, char **) {
    parser.printCommands(std::cout);
  });

  auto logging_system = std::make_shared<soralog::LoggingSystem>(
      std::make_shared<kagome::log::Configurator>(
          std::make_shared<libp2p::log::Configurator>()));

  auto r = logging_system->configure();
  if (not r.message.empty()) {
    (r.has_error ? std::cerr : std::cout) << r.message << std::endl;
  }
  if (r.has_error) {
    exit(EXIT_FAILURE);
  }
  kagome::log::setLoggingSystem(logging_system);
  kagome::log::setLevelOfGroup("*", kagome::log::Level::WARN);

  auto logger = kagome::log::createLogger("AppConfiguration", "main");
  kagome::application::AppConfigurationImpl configuration{logger};

  BOOST_ASSERT(configuration.initialize_from_args(argc, argv));

  kagome::injector::KagomeNodeInjector injector{configuration};
  auto block_storage = injector.injectBlockStorage();
  parser.addCommand(
      "inspect-block",
      "print info about the block with the given number or hash",
      [&block_storage](int argc, char **argv) {
        auto opt_id = parseBlockId(argv[1]);
        if (!opt_id) {
          return;
        }
        if (auto res = block_storage->getBlockHeader(opt_id.value()); res) {
          std::cout << "#: " << res.value().number << "\n";
          std::cout << "Parent hash: " << res.value().parent_hash.toHex()
                    << "\n";
          std::cout << "State root: " << res.value().state_root.toHex() << "\n";
          std::cout << "Extrinsics root: "
                    << res.value().extrinsics_root.toHex() << "\n";
        } else {
          std::cerr << "Error: " << res.error().message() << "\n";
          return;
        }
      });

  parser.addCommand("remove-block",
                    "remove the block from the block tree",
                    [&block_storage](int argc, char **argv) {
                      auto opt_id = parseBlockId(argv[1]);
                      if (!opt_id) {
                        return;
                      }
                      auto data = block_storage->getBlockData(opt_id.value());
                      if (!data) {
                        std::cerr << "Error: " << data.error().message()
                                  << "\n";
                      }
                      if (auto res = block_storage->removeBlock(
                              data.value().hash, data.value().header->number);
                          !res) {
                        std::cerr << "Error: " << res.error().message() << "\n";
                        return;
                      }
                    });

  parser.invoke(argc, argv);
}
