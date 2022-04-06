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

using kagome::blockchain::BlockStorage;
using kagome::storage::trie::TrieStorage;

struct ArgumentList {
  int count;
  const char **params;
};

class CommandExecutionError : public std::runtime_error {
 public:
  CommandExecutionError(std::string_view command_name,
                        const ArgumentList &args,
                        const std::string &what)
      : std::runtime_error{what}, command_name{command_name}, args{args} {}

  friend std::ostream &operator<<(std::ostream &out,
                                  CommandExecutionError const &err) {
    out << "Error in command '" << err.command_name << "', args: ";
    return err.printArgs(out) << " - " << err.what() << "\n";
  }

 private:
  std::ostream &printArgs(std::ostream &out) const {
    out << "(";
    for (int i = 0; i < args.count - 1; i++) {
      out << args.params[i] << ", ";
    }
    out << args.params[args.count - 1] << ")";
    return out;
  }

  std::string_view command_name;
  const ArgumentList &args;
};

class Command {
 public:
  Command(std::string name, std::string description)
      : name{std::move(name)}, description{std::move(description)} {}

  virtual ~Command() = default;

  virtual void execute(std::ostream &out, const ArgumentList &args) = 0;

  std::string_view getName() const {
    return name;
  }

  std::string_view getDescription() const {
    return description;
  }

 protected:
  void assertArgumentCount(const ArgumentList &args, int min, int max) {
    if (args.count < min or args.count > max) {
      throw CommandExecutionError{
          name,
          args,
          fmt::format("Argument count mismatch: expected {} to {}, got {}",
                      min,
                      max,
                      args.count)};
    }
  }

  template <typename... Ts>
  void throwError(const ArgumentList &args, const char *fmt, Ts &&...ts) const {
    throw CommandExecutionError{
        name, args, fmt::format(fmt, std::forward<Ts>(ts)...)};
  }

 private:
  std::string name;
  std::string description;
};

class CommandParser {
 public:
  using CommandFunctor = std::function<void(int, char **)>;

  void addCommand(std::unique_ptr<Command> cmd) {
    std::string name{cmd->getName()};
    commands_.insert({name, std::move(cmd)});
  }

  void invoke(const ArgumentList &args) const noexcept {
    if (args.count < 2) {
      std::cerr << "Unspecified command!\nAvailable commands are:\n";
      printCommands(std::cerr);
    }
    if (auto command = commands_.find(args.params[1]);
        command != commands_.cend()) {
      ArgumentList cmd_args{args.count - 1, args.params + 1};
      try {
        command->second->execute(std::cout, cmd_args);
      } catch (CommandExecutionError &e) {
        std::cerr << e;
      }
    } else {
      std::cerr << "Unknown command '" << args.params[1]
                << "'!\nAvailable commands are:\n";
      printCommands(std::cerr);
    }
  }

  void printCommands(std::ostream &out) const {
    for (auto &[name, cmd] : commands_) {
      out << name << "\t" << cmd->getDescription() << "\n";
    }
  }

 private:
  std::unordered_map<std::string, std::unique_ptr<Command>> commands_;
};

std::optional<kagome::primitives::BlockId> parseBlockId(const char *string) {
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

class PrintHelpCommand final : public Command {
 public:
  explicit PrintHelpCommand(CommandParser const &parser)
      : Command{"help", "print help message"}, parser{parser} {}

  virtual void execute(std::ostream &out, const ArgumentList &args) override {
    assertArgumentCount(args, 1, 1);
    parser.printCommands(out);
  }

 private:
  CommandParser const &parser;
};

class InspectBlockCommand : public Command {
 public:
  explicit InspectBlockCommand(std::shared_ptr<BlockStorage> block_storage)
      : Command{"inspect-block",
                "# or hash - print info about the block with the given number "
                "or hash"},
        block_storage{block_storage} {}

  virtual void execute(std::ostream &out, const ArgumentList &args) override {
    assertArgumentCount(args, 2, 2);
    auto opt_id = parseBlockId(args.params[1]);
    if (!opt_id) {
      throwError(args, "Failed to parse block id '{}'", args.params[1]);
    }
    if (auto res = block_storage->getBlockHeader(opt_id.value()); res) {
      if (!res.value().has_value()) {
        throwError(args, "Block header not found for '{}'", args.params[1]);
      }
      std::cout << "#: " << res.value()->number << "\n";
      std::cout << "Parent hash: " << res.value()->parent_hash.toHex() << "\n";
      std::cout << "State root: " << res.value()->state_root.toHex() << "\n";
      std::cout << "Extrinsics root: " << res.value()->extrinsics_root.toHex()
                << "\n";
    } else {
      throwError(args, "Internal error: {}}", res.error().message());
    }
  }

 private:
  std::shared_ptr<BlockStorage> block_storage;
};

class RemoveBlockCommand : public Command {
 public:
  explicit RemoveBlockCommand(std::shared_ptr<BlockStorage> block_storage)
      : Command{"remove-block",
                "# or hash - remove the block from the block tree"},
        block_storage{block_storage} {}

  virtual void execute(std::ostream &out, const ArgumentList &args) override {
    assertArgumentCount(args, 2, 2);
    auto opt_id = parseBlockId(args.params[1]);
    if (!opt_id) {
      throwError(args, "Failed to parse block id '{}'", args.params[1]);
    }
    auto data = block_storage->getBlockData(opt_id.value());
    if (!data) {
      throwError(args, "Error getting block data: {}", data.error().message());
    }
    if (!data.value().has_value()) {
      throwError(args, "Error: block data not found");
    }
    if (auto res = block_storage->removeBlock(
            {data.value().value().header->number, data.value().value().hash});
        !res) {
      throwError(args, "Internal error: {}", res.error().message());
    }
  }

 private:
  std::shared_ptr<BlockStorage> block_storage;
};

class QueryStateCommand : public Command {
 public:
  explicit QueryStateCommand(std::shared_ptr<TrieStorage> trie_storage)
      : Command{"query-state",
                "state_hash, key - query value at a given key and state"},
        trie_storage{trie_storage} {}

  virtual void execute(std::ostream &out, const ArgumentList &args) override {
    assertArgumentCount(args, 3, 3);

    kagome::storage::trie::RootHash state_root{};
    if (auto id_bytes = kagome::common::unhex(args.params[1]); id_bytes) {
      std::copy_n(id_bytes.value().begin(),
                  kagome::primitives::BlockHash::size(),
                  state_root.begin());
    } else {
      throwError(args, "Invalid block hash!");
    }
    auto batch = trie_storage->getEphemeralBatchAt(state_root);
    if (!batch) {
      throwError(args, "Error getting trie batch: {}", batch.error().message());
    }
    kagome::common::Buffer key{};
    if (auto key_bytes = kagome::common::unhex(args.params[2]); key_bytes) {
      key = kagome::common::Buffer{std::move(key_bytes.value())};
    } else {
      throwError(args, "Invalid key!");
    }
    auto value_res = batch.value()->tryGet(key);
    if (value_res.has_error()) {
      throwError(args,
                 "Error retrieving value from Trie: {}",
                 value_res.error().message());
    }
    auto &value_opt = value_res.value();
    if (value_opt.has_value()) {
      std::cout << "Value is " << value_opt->get().toHex() << "\n";
    } else {
      std::cout << "No value by given key\n";
    }
  }

 private:
  std::shared_ptr<TrieStorage> trie_storage;
};

class SearchChainCommand : public Command {
 public:
  explicit SearchChainCommand(std::shared_ptr<BlockStorage> block_storage)
      : Command{"search-chain",
                "target [start block/0] [end block/deepest finalized] - search "
                "the finalized chain for the target entity. Currently, "
                "'justification' or 'authority_update' are supported "},
        block_storage{block_storage} {}

  virtual void execute(std::ostream &out, const ArgumentList &args) override {
    assertArgumentCount(args, 2, 4);

    block_storage->get
  }

 private:
  std::shared_ptr<BlockStorage> block_storage;
};

int main(int argc, const char **argv) {
  CommandParser parser;
  parser.addCommand(std::make_unique<PrintHelpCommand>(parser));

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
  parser.addCommand(std::make_unique<InspectBlockCommand>(block_storage));
  parser.addCommand(std::make_unique<RemoveBlockCommand>(block_storage));
  parser.addCommand(std::make_unique<QueryStateCommand>(trie_storage));
  parser.addCommand(std::make_unique<SearchChainCommand>(trie_storage));

  ArgumentList args{argc, argv};
  parser.invoke(args);

  return 0;
}
