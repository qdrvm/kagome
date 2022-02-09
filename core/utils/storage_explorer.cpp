/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <boost/program_options.hpp>
#include <libp2p/log/configurator.hpp>

#include "application/impl/app_configuration_impl.hpp"
#include "blockchain/block_storage.hpp"
#include "blockchain/block_tree.hpp"
#include "injector/application_injector.hpp"
#include "log/configurator.hpp"
#include "storage/trie/trie_storage.hpp"

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

std::optional<kagome::primitives::BlockId> parseBlockId(char *string) {
  kagome::primitives::BlockId id;
  if (strlen(string) == 2 * kagome::primitives::BlockHash::size()) {
    kagome::primitives::BlockHash id_hash{};
    if (auto id_bytes = kagome::common::unhex(string); id_bytes) {
      std::copy_n(id_bytes.value().begin(),
                  kagome::primitives::BlockHash::size(),
                  id_hash.begin());
    } else {
      std::cerr << "Invalid block hash!\n";
      return std::nullopt;
    }
    id = std::move(id_hash);
  } else {
    try {
      id = std::stoi(string);
    } catch (std::invalid_argument &) {
      std::cerr << "Block number must be a hex hash or a number!\n";
      return std::nullopt;
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

  BOOST_ASSERT(configuration.initializeFromArgs(argc, argv));

  kagome::injector::KagomeNodeInjector injector{configuration};
  auto block_storage = injector.injectBlockStorage();

  auto trie_storage = injector.injectTrieStorage();
  parser.addCommand(
      "inspect-block",
      "# or hash - print info about the block with the given number or hash",
      [&block_storage](int argc, char **argv) {
        if (argc != 2) {
          throw std::runtime_error("Invalid argument count, 1 expected");
        }
        auto opt_id = parseBlockId(argv[1]);
        if (!opt_id) {
          return;
        }
        if (auto res = block_storage->getBlockHeader(opt_id.value()); res) {
          if (!res.value().has_value()) {
            std::cerr << "Error: Block header not found\n";
          }
          std::cout << "#: " << res.value()->number << "\n";
          std::cout << "Parent hash: " << res.value()->parent_hash.toHex()
                    << "\n";
          std::cout << "State root: " << res.value()->state_root.toHex() << "\n";
          std::cout << "Extrinsics root: "
                    << res.value()->extrinsics_root.toHex() << "\n";
        } else {
          std::cerr << "Error: " << res.error().message() << "\n";
          return;
        }
      });

  parser.addCommand(
      "remove-block",
      "# or hash - remove the block from the block tree",
      [&block_storage](int argc, char **argv) {
        if (argc != 2) {
          throw std::runtime_error("Invalid argument count, 1 expected");
        }
        auto opt_id = parseBlockId(argv[1]);
        if (!opt_id) {
          return;
        }
        auto data = block_storage->getBlockData(opt_id.value());
        if (!data) {
          std::cerr << "Error: " << data.error().message() << "\n";
        }
        if(!data.value().has_value()) {
          std::cerr << "Error: block data not found\n";
        }
        if (auto res = block_storage->removeBlock(data.value()->hash,
                                                  data.value()->header->number);
            !res) {
          std::cerr << "Error: " << res.error().message() << "\n";
          return;
        }
      });

  parser.addCommand(
      "query-state",
      "state_hash, key - query value at a given key and state",
      [&trie_storage](int argc, char **argv) {
        if (argc != 3) {
          throw std::runtime_error("Invalid argument count, 2 expected");
        }
        kagome::storage::trie::RootHash state_root{};
        if (auto id_bytes = kagome::common::unhex(argv[1]); id_bytes) {
          std::copy_n(id_bytes.value().begin(),
                      kagome::primitives::BlockHash::size(),
                      state_root.begin());
        } else {
          std::cerr << "Invalid block hash!\n";
          return;
        }
        auto batch = trie_storage->getEphemeralBatchAt(state_root);
        if (!batch) {
          std::cerr << "Error: " << batch.error().message() << "\n";
          return;
        }
        kagome::common::Buffer key{};
        if (auto key_bytes = kagome::common::unhex(argv[2]); key_bytes) {
          key = kagome::common::Buffer{std::move(key_bytes.value())};
        } else {
          std::cerr << "Invalid key!\n";
          return;
        }
        auto value_res = batch.value()->tryGet(key);
        if (value_res.has_error()) {
          std::cout << "Error retrieving value from Trie: "
                    << value_res.error().message() << "\n";
        }
        auto& value_opt = value_res.value();
        if (value_opt.has_value()) {
          std::cout << "Value is " << value_opt->toHex()
                    << "\n";
        } else {
          std::cout << "No value by provided key\n";
        }
      });

  parser.invoke(argc, argv);
}
