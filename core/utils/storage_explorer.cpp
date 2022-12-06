/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <boost/program_options.hpp>
#include <libp2p/log/configurator.hpp>

#include "application/impl/app_configuration_impl.hpp"
#include "blockchain/block_storage.hpp"
#include "blockchain/impl/block_header_repository_impl.hpp"
#include "blockchain/impl/block_tree_impl.hpp"
#include "consensus/grandpa/impl/authority_manager_impl.hpp"
#include "crypto/hasher/hasher_impl.hpp"
#include "injector/application_injector.hpp"
#include "log/configurator.hpp"
#include "runtime/runtime_api/impl/grandpa_api.hpp"
#include "storage/trie/trie_storage.hpp"

using kagome::blockchain::BlockStorage;
using kagome::consensus::grandpa::AuthorityManager;
using kagome::consensus::grandpa::AuthorityManagerImpl;
using kagome::crypto::Hasher;
using kagome::crypto::HasherImpl;
using kagome::primitives::BlockHeader;
using kagome::primitives::BlockId;
using kagome::primitives::BlockNumber;
using kagome::primitives::GrandpaDigest;
using kagome::primitives::events::ChainSubscriptionEngine;
using kagome::runtime::GrandpaApi;
using kagome::storage::trie::TrieStorage;

using ArgumentList = gsl::span<const char *>;

class CommandExecutionError : public std::runtime_error {
 public:
  CommandExecutionError(std::string_view command_name, const std::string &what)
      : std::runtime_error{what}, command_name{command_name} {}

  friend std::ostream &operator<<(std::ostream &out,
                                  CommandExecutionError const &err) {
    return out << "Error in command '" << err.command_name
               << "': " << err.what() << "\n";
  }

 private:
  std::string_view command_name;
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
    if (args.size() < min or args.size() > max) {
      throw CommandExecutionError{
          name,
          fmt::format("Argument count mismatch: expected {} to {}, got {}",
                      min,
                      max,
                      args.size())};
    }
  }

  template <typename... Ts>
  [[noreturn]] void throwError(const char *fmt, Ts &&...ts) const {
    throw CommandExecutionError{name,
                                fmt::format(fmt, std::forward<Ts>(ts)...)};
  }

  template <typename T>
  T unwrapResult(std::string_view context, outcome::result<T> &&res) const {
    if (res.has_value()) return std::move(res).value();
    throwError("{}: {}", context, res.error());
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
    if (args.size() < 2) {
      std::cerr << "Unspecified command!\nAvailable commands are:\n";
      printCommands(std::cerr);
    }
    if (auto command = commands_.find(args[1]); command != commands_.cend()) {
      ArgumentList cmd_args{args.subspan(1)};
      try {
        command->second->execute(std::cout, cmd_args);
      } catch (CommandExecutionError &e) {
        std::cerr << "Command execution error: " << e;
      } catch (std::exception &e) {
        std::cerr << "Exception occurred: " << e.what();
      }
    } else {
      std::cerr << "Unknown command '" << args[1]
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
    auto opt_id = parseBlockId(args[1]);
    if (!opt_id) {
      throwError("Failed to parse block id '{}'", args[1]);
    }
    if (auto res = block_storage->getBlockHeader(opt_id.value()); res) {
      if (!res.value().has_value()) {
        throwError("Block header not found for '{}'", args[1]);
      }
      std::cout << "#: " << res.value()->number << "\n";
      std::cout << "Parent hash: " << res.value()->parent_hash.toHex() << "\n";
      std::cout << "State root: " << res.value()->state_root.toHex() << "\n";
      std::cout << "Extrinsics root: " << res.value()->extrinsics_root.toHex()
                << "\n";

      if (auto body_res = block_storage->getBlockBody(opt_id.value());
          body_res) {
        if (!body_res.value().has_value()) {
          throwError("Block body not found for '{}'", args[1]);
        }
        std::cout << "# of extrinsics: " << body_res.value()->size() << "\n";

      } else {
        throwError("Internal error: {}}", body_res.error());
      }

    } else {
      throwError("Internal error: {}}", res.error());
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
    auto opt_id = parseBlockId(args[1]);
    if (!opt_id) {
      throwError("Failed to parse block id '{}'", args[1]);
    }
    auto data = block_storage->getBlockData(opt_id.value());
    if (!data) {
      throwError("Failed getting block data: {}", data.error());
    }
    if (!data.value().has_value()) {
      throwError("Block data not found");
    }
    if (auto res = block_storage->removeBlock(
            {data.value().value().header->number, data.value().value().hash});
        !res) {
      throwError("{}", res.error());
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
    if (auto id_bytes = kagome::common::unhex(args[1]); id_bytes) {
      std::copy_n(id_bytes.value().begin(),
                  kagome::primitives::BlockHash::size(),
                  state_root.begin());
    } else {
      throwError("Invalid block hash!");
    }
    auto batch = trie_storage->getEphemeralBatchAt(state_root);
    if (!batch) {
      throwError("Failed getting trie batch: {}", batch.error());
    }
    kagome::common::Buffer key{};
    if (auto key_bytes = kagome::common::unhex(args[2]); key_bytes) {
      key = kagome::common::Buffer{std::move(key_bytes.value())};
    } else {
      throwError("Invalid key!");
    }
    auto value_res = batch.value()->tryGet(key);
    if (value_res.has_error()) {
      throwError("Error retrieving value from Trie: {}", value_res.error());
    }
    auto &value_opt = value_res.value();
    if (value_opt.has_value()) {
      std::cout << "Value is " << value_opt->view().toHex() << "\n";
    } else {
      std::cout << "No value by given key\n";
    }
  }

 private:
  std::shared_ptr<TrieStorage> trie_storage;
};

class SearchChainCommand : public Command {
 public:
  explicit SearchChainCommand(
      std::shared_ptr<BlockStorage> block_storage,
      std::shared_ptr<TrieStorage> trie_storage,
      std::shared_ptr<AuthorityManager> authority_manager,
      std::shared_ptr<Hasher> hasher)
      : Command{"search-chain",
                "target [start block/0] [end block/deepest finalized] - search "
                "the finalized chain for the target entity. Currently, "
                "'justification' or 'authority-update' are supported "},
        block_storage{block_storage},
        trie_storage{trie_storage},
        hasher{hasher} {
    BOOST_ASSERT(block_storage != nullptr);
    BOOST_ASSERT(trie_storage != nullptr);
    BOOST_ASSERT(authority_manager != nullptr);
    BOOST_ASSERT(hasher != nullptr);
  }

  enum class Target { Justification, AuthorityUpdate, LastBlock };

  virtual void execute(std::ostream &out, const ArgumentList &args) override {
    assertArgumentCount(args, 2, 4);
    Target target = parseTarget(args[1]);
    if (target == Target::LastBlock) {
      std::cout << fmt::format("{}\n",
                               block_storage->getLastFinalized().value());
      return;
    }

    BlockId start, end;

    if (args.size() > 2) {
      auto start_id_opt = parseBlockId(args[2]);
      if (!start_id_opt) {
        throwError("Failed to parse block id '{}'", args[2]);
      }
      start = start_id_opt.value();
    } else {
      start = 0;
    }
    if (args.size() > 3) {
      auto end_id_opt = parseBlockId(args[3]);
      if (!end_id_opt) {
        throwError("Failed to parse block id '{}'", args[3]);
      }
      end = end_id_opt.value();
    } else {
      auto last_finalized = unwrapResult("Getting last finalized block",
                                         block_storage->getLastFinalized());
      end = last_finalized.number;
    }

    auto start_header_opt = unwrapResult("Getting 'start' block header",
                                         block_storage->getBlockHeader(start));
    if (!start_header_opt) {
      throwError("Start block header {} not found", start);
    }
    auto &start_header = start_header_opt.value();

    auto end_header_opt = unwrapResult("Getting 'end' block header",
                                       block_storage->getBlockHeader(end));
    if (!end_header_opt) {
      throwError("'End block header {} not found", end);
    }
    auto &end_header = end_header_opt.value();

    for (int64_t current = start_header.number, stop = end_header.number;
         current <= stop;
         current++) {
      auto current_header_opt =
          unwrapResult(fmt::format("Getting header of block #{}", current),
                       block_storage->getBlockHeader(current));
      if (!current_header_opt) {
        throwError("Block header #{} not found", current);
      }
      searchBlock(out, current_header_opt.value(), target);
    }
  }

 private:
  Target parseTarget(const char *arg) const {
    if (strcmp(arg, "justification") == 0) return Target::Justification;
    if (strcmp(arg, "authority-update") == 0) return Target::AuthorityUpdate;
    if (strcmp(arg, "last-finalized") == 0) return Target::LastBlock;
    throwError("Invalid target '{}'", arg);
  }

  void searchBlock(std::ostream &out,
                   BlockHeader const &header,
                   Target target) const {
    switch (target) {
      case Target::Justification:
        return searchForJustification(out, header);
      case Target::AuthorityUpdate:
        return searchForAuthorityUpdate(out, header);
      case Target::LastBlock:
        return;
    }
    BOOST_UNREACHABLE_RETURN();
  }

  void searchForJustification(std::ostream &out,
                              BlockHeader const &header) const {
    auto just_opt = unwrapResult(
        fmt::format("Getting justification for block #{}", header.number),
        block_storage->getJustification(header.number));
    if (just_opt) {
      out << header.number << ", ";
    }
  }

  void searchForAuthorityUpdate(std::ostream &out,
                                BlockHeader const &header) const {
    for (auto &digest_item : header.digest) {
      auto *consensus_digest =
          boost::get<kagome::primitives::Consensus>(&digest_item);
      if (consensus_digest) {
        auto decoded = unwrapResult("Decoding consensus digest",
                                    consensus_digest->decode());
        if (decoded.consensus_engine_id
            == kagome::primitives::kGrandpaEngineId) {
          reportAuthorityUpdate(out, header.number, decoded.asGrandpaDigest());
        }
      }
    }
  }

  void reportAuthorityUpdate(std::ostream &out,
                             BlockNumber digest_origin,
                             GrandpaDigest const &digest) const {
    using namespace kagome::primitives;
    if (auto *scheduled_change = boost::get<ScheduledChange>(&digest);
        scheduled_change) {
      out << "ScheduledChange at #" << digest_origin << " for ";
      if (scheduled_change->subchain_length > 0) {
        out << "#" << digest_origin + scheduled_change->subchain_length;
      } else {
        out << "the same block";
      }
      out << "\n";

    } else if (auto *forced_change = boost::get<ForcedChange>(&digest);
               forced_change) {
      out << "ForcedChange at " << digest_origin << ", delay starts at #"
          << forced_change->delay_start << " for "
          << forced_change->subchain_length << " blocks (so activates at #"
          << forced_change->delay_start + forced_change->subchain_length << ")";
      out << "\n";

    } else if (auto *pause = boost::get<Pause>(&digest); pause) {
      out << "Pause at " << digest_origin << " for "
          << digest_origin + pause->subchain_length << "\n";

    } else if (auto *resume = boost::get<Resume>(&digest); resume) {
      out << "Resume at " << digest_origin << " for "
          << digest_origin + resume->subchain_length << "\n";

    } else if (auto *disabled = boost::get<OnDisabled>(&digest); disabled) {
      out << "Disabled at " << digest_origin << " for authority "
          << disabled->authority_index << "\n";
    }
  }

  std::shared_ptr<BlockStorage> block_storage;
  std::shared_ptr<TrieStorage> trie_storage;
  std::shared_ptr<Hasher> hasher;
};

int storage_explorer_main(int argc, const char **argv) {
  ArgumentList args{argv, argc};

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

  int kagome_args_start = -1;
  for (int i = 1; i < args.size(); i++) {
    if (strcmp(args[i], "--") == 0) {
      kagome_args_start = i;
    }
  }
  if (kagome_args_start == -1) {
    std::cerr
        << "You must specify arguments for kagome initialization after '--'\n";
    return -1;
  }

  if (!configuration.initializeFromArgs(
          argc - kagome_args_start, args.subspan(kagome_args_start).data())) {
    std::cerr << "Failed to initialize kagome!\n";
    return -1;
  }

  kagome::injector::KagomeNodeInjector injector{configuration};
  auto block_storage = injector.injectBlockStorage();
  auto trie_storage = injector.injectTrieStorage();
  auto app_state_manager = injector.injectAppStateManager();
  auto block_tree = injector.injectBlockTree();
  auto executor = injector.injectExecutor();
  auto persistent_storage = injector.injectStorage();
  auto hasher = std::make_shared<kagome::crypto::HasherImpl>();

  auto header_repo =
      std::make_shared<kagome::blockchain::BlockHeaderRepositoryImpl>(
          persistent_storage, hasher);
  auto grandpa_api =
      std::make_shared<kagome::runtime::GrandpaApiImpl>(header_repo, executor);

  auto chain_events_engine = std::make_shared<ChainSubscriptionEngine>();

  auto authority_manager =
      std::make_shared<AuthorityManagerImpl>(AuthorityManagerImpl::Config{},
                                             app_state_manager,
                                             block_tree,
                                             trie_storage,
                                             grandpa_api,
                                             hasher,
                                             persistent_storage,
                                             header_repo,
                                             chain_events_engine);

  parser.addCommand(std::make_unique<InspectBlockCommand>(block_storage));
  parser.addCommand(std::make_unique<RemoveBlockCommand>(block_storage));
  parser.addCommand(std::make_unique<QueryStateCommand>(trie_storage));
  parser.addCommand(std::make_unique<SearchChainCommand>(
      block_storage, trie_storage, authority_manager, hasher));

  parser.invoke(args.subspan(0, kagome_args_start));

  return 0;
}
