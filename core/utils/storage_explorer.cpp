/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/log/configurator.hpp>

#include "application/chain_spec.hpp"
#include "application/impl/app_configuration_impl.hpp"
#include "blockchain/block_storage.hpp"
#include "blockchain/impl/block_tree_impl.hpp"
#include "consensus/grandpa/impl/authority_manager_impl.hpp"
#include "crypto/hasher/hasher_impl.hpp"
#include "injector/application_injector.hpp"
#include "log/configurator.hpp"
#include "log/formatters/variant.hpp"
#include "runtime/runtime_api/impl/grandpa_api.hpp"
#include "storage/rocksdb/rocksdb.hpp"
#include "storage/trie/trie_storage.hpp"

using kagome::blockchain::BlockStorage;
using kagome::consensus::grandpa::AuthorityManager;
using kagome::consensus::grandpa::AuthorityManagerImpl;
using kagome::consensus::grandpa::ForcedChange;
using kagome::consensus::grandpa::OnDisabled;
using kagome::consensus::grandpa::Pause;
using kagome::consensus::grandpa::Resume;
using kagome::consensus::grandpa::ScheduledChange;
using kagome::crypto::Hasher;
using kagome::crypto::HasherImpl;
using kagome::primitives::BlockHeader;
using kagome::primitives::BlockId;
using kagome::primitives::BlockNumber;
using kagome::primitives::GrandpaDigest;
using kagome::primitives::events::ChainSubscriptionEngine;
using kagome::runtime::GrandpaApi;
using kagome::storage::trie::TrieStorage;

using ArgumentList = std::span<const char *>;

class CommandExecutionError : public std::runtime_error {
 public:
  CommandExecutionError(std::string_view command_name, const std::string &what)
      : std::runtime_error{what}, command_name{command_name} {}

  friend std::ostream &operator<<(std::ostream &out,
                                  const CommandExecutionError &err) {
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
  void assertArgumentCount(const ArgumentList &args, size_t min, size_t max) {
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
  [[noreturn]] void throwError(const char *fmt, const Ts &...ts) const {
    throw CommandExecutionError(
        name, ::fmt::vformat(fmt, fmt::make_format_args(ts...)));
  }

  template <typename T>
  T unwrapResult(std::string_view context, outcome::result<T> &&res) const {
    if (res.has_value()) {
      return std::move(res).value();
    }
    throwError("{}: {}", context, res.error());
  }

 private:
  std::string name;
  std::string description;
};

class CommandParser {
 public:
  void addCommand(std::unique_ptr<Command> cmd) {
    std::string name{cmd->getName()};
    commands_.insert({name, std::move(cmd)});
  }

  void invoke(const ArgumentList &args) const {
    if (args.size() < 2) {
      std::cerr << "Unspecified command!\nAvailable commands are:\n";
      printCommands(std::cerr);
      return;
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
  explicit PrintHelpCommand(const CommandParser &parser)
      : Command{"help", "print help message"}, parser{parser} {}

  virtual void execute(std::ostream &out, const ArgumentList &args) override {
    assertArgumentCount(args, 1, 1);
    parser.printCommands(out);
  }

 private:
  const CommandParser &parser;
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
    auto hash_opt_res = block_storage->getBlockHash(opt_id.value());
    if (hash_opt_res.has_error()) {
      throwError("Internal error: {}}", hash_opt_res.error());
    }
    if (hash_opt_res.value().has_value()) {
      throwError("Block header not found for '{}'", args[1]);
    }
    const auto &hash = hash_opt_res.value().value();

    auto header_opt_res = block_storage->getBlockHeader(hash);
    if (header_opt_res.has_error()) {
      throwError("Internal error: {}}", header_opt_res.error());
    }
    if (header_opt_res.value().has_value()) {
      throwError("Block header not found for '{}'", args[1]);
    }
    const auto &header = header_opt_res.value().value();

    std::cout << "#: " << header.number << "\n";
    std::cout << "Parent hash: " << header.parent_hash.toHex() << "\n";
    std::cout << "State root: " << header.state_root.toHex() << "\n";
    std::cout << "Extrinsics root: " << header.extrinsics_root.toHex() << "\n";

    auto body_opt_res = block_storage->getBlockBody(hash);
    if (body_opt_res.has_error()) {
      throwError("Internal error: {}}", body_opt_res.error());
    }
    if (!body_opt_res.value().has_value()) {
      throwError("Block body not found for '{}'", args[1]);
    }
    const auto &body = body_opt_res.value().value();

    std::cout << "# of extrinsics: " << body.size() << "\n";
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

    auto hash_opt_res = block_storage->getBlockHash(opt_id.value());
    if (hash_opt_res.has_error()) {
      throwError("Internal error: {}}", hash_opt_res.error());
    }
    if (!hash_opt_res.value().has_value()) {
      throwError("Block not found for '{}'", args[1]);
    }
    const auto &hash = hash_opt_res.value().value();

    if (auto res = block_storage->removeBlock(hash); !res) {
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

    auto hash_opt_res = block_storage->getBlockHash(start);
    if (hash_opt_res.has_error()) {
      throwError("Internal error: {}}", hash_opt_res.error());
    }
    if (!hash_opt_res.value().has_value()) {
      throwError("Start block header {} not found", start);
    }
    const auto &hash = hash_opt_res.value().value();

    auto start_header_opt = unwrapResult("Getting 'start' block header",
                                         block_storage->getBlockHeader(hash));
    if (!start_header_opt) {
      throwError("Start block header {} not found", start);
    }
    auto &start_header = start_header_opt.value();

    auto end_hash_opt = unwrapResult("Getting 'end' block header",
                                     block_storage->getBlockHash(end));
    if (!end_hash_opt) {
      throwError("'End block header {} not found", end);
    }
    const auto &end_hash = end_hash_opt.value();

    auto end_header_opt = unwrapResult("Getting 'end' block header",
                                       block_storage->getBlockHeader(end_hash));
    if (!end_header_opt) {
      throwError("'End block header {} not found", end);
    }
    auto &end_header = end_header_opt.value();

    for (int64_t current = start_header.number, stop = end_header.number;
         current <= stop;
         current++) {
      auto current_hash_opt =
          unwrapResult(fmt::format("Getting header of block #{}", current),
                       block_storage->getBlockHash(current));
      if (!current_hash_opt) {
        throwError("Block header #{} not found", current);
      }
      const auto &current_hash = current_hash_opt.value();

      auto current_header_opt =
          unwrapResult(fmt::format("Getting header of block #{}", current),
                       block_storage->getBlockHeader(current_hash));
      if (!current_header_opt) {
        throwError("Block header #{} not found", current);
      }
      searchBlock(out, current_header_opt.value(), target);
    }
  }

 private:
  Target parseTarget(const char *arg) const {
    if (strcmp(arg, "justification") == 0) {
      return Target::Justification;
    }
    if (strcmp(arg, "authority-update") == 0) {
      return Target::AuthorityUpdate;
    }
    if (strcmp(arg, "last-finalized") == 0) {
      return Target::LastBlock;
    }
    throwError("Invalid target '{}'", arg);
  }

  void searchBlock(std::ostream &out,
                   const BlockHeader &header,
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
                              const BlockHeader &header) const {
    auto hash_opt_res = unwrapResult(
        fmt::format("Getting justification for block #{}", header.number),
        block_storage->getBlockHash(header.number));
    if (hash_opt_res.has_value()) {
      const auto &hash = hash_opt_res.value();

      auto just_opt = unwrapResult(
          fmt::format("Getting justification for block #{}", header.number),
          block_storage->getJustification(hash));
      if (just_opt) {
        out << header.number << ", ";
      }
    }
  }

  void searchForAuthorityUpdate(std::ostream &out,
                                const BlockHeader &header) const {
    for (auto &digest_item : header.digest) {
      auto *consensus_digest =
          std::get_if<kagome::primitives::Consensus>(&digest_item);
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
                             const GrandpaDigest &digest) const {
    using namespace kagome::primitives;
    if (auto *scheduled_change = std::get_if<ScheduledChange>(&digest);
        scheduled_change) {
      out << "ScheduledChange at #" << digest_origin << " for ";
      if (scheduled_change->subchain_length > 0) {
        out << "#" << digest_origin + scheduled_change->subchain_length;
      } else {
        out << "the same block";
      }
      out << "\n";

    } else if (auto *forced_change = std::get_if<ForcedChange>(&digest);
               forced_change) {
      out << "ForcedChange at " << digest_origin << ", delay starts at #"
          << forced_change->delay_start << " for "
          << forced_change->subchain_length << " blocks (so activates at #"
          << forced_change->delay_start + forced_change->subchain_length << ")";
      out << "\n";

    } else if (auto *pause = std::get_if<Pause>(&digest); pause) {
      out << "Pause at " << digest_origin << " for "
          << digest_origin + pause->subchain_length << "\n";

    } else if (auto *resume = std::get_if<Resume>(&digest); resume) {
      out << "Resume at " << digest_origin << " for "
          << digest_origin + resume->subchain_length << "\n";

    } else if (auto *disabled = std::get_if<OnDisabled>(&digest); disabled) {
      out << "Disabled at " << digest_origin << " for authority "
          << disabled->authority_index << "\n";
    }
  }

  std::shared_ptr<BlockStorage> block_storage;
  std::shared_ptr<TrieStorage> trie_storage;
  std::shared_ptr<Hasher> hasher;
};

class ChainInfoCommand final : public Command {
 public:
  ChainInfoCommand(std::shared_ptr<kagome::blockchain::BlockTree> block_tree)
      : Command{"chain-info", "Print general info about the current chain. "},
        block_tree{block_tree} {
    BOOST_ASSERT(block_tree);
  }

  virtual void execute(std::ostream &out, const ArgumentList &args) override {
    if (args.size() > 1) {
      throwError("No arguments expected, {} arguments received", args.size());
    }
    fmt::print(out, "Last finalized: {}\n", block_tree->getLastFinalized());
    fmt::print(out, "Best block: {}\n", block_tree->bestBlock());
    fmt::print(out, "Genesis block: {}\n", block_tree->getGenesisBlockHash());
    fmt::print(out, "Leaves:\n");
    for (auto &leaf : block_tree->getLeaves()) {
      auto header_res = block_tree->getBlockHeader(leaf);
      if (!header_res) {
        throwError("Error loading block header: {}", header_res.error());
      }
      fmt::print(out, "\t#{} - {}\n", header_res.value().number, leaf);
    }
  }

 private:
  std::shared_ptr<kagome::blockchain::BlockTree> block_tree;
};

class DbStatsCommand : public Command {
 public:
  DbStatsCommand(std::filesystem::path db_path)
      : Command("db-stats", "Print RocksDb stats"), db_path{db_path} {}

  virtual void execute(std::ostream &out, const ArgumentList &args) override {
    rocksdb::Options options;
    rocksdb::DB *db;

    std::vector<std::string> existing_families;
    auto res = rocksdb::DB::ListColumnFamilies(
        options, db_path.native(), &existing_families);
    if (!res.ok()) {
      throwError("Failed to open database at {}: {}",
                 db_path.native(),
                 res.ToString());
    }
    std::vector<rocksdb::ColumnFamilyDescriptor> column_families;
    for (auto &family : existing_families) {
      column_families.emplace_back(rocksdb::ColumnFamilyDescriptor{family, {}});
    }
    std::vector<rocksdb::ColumnFamilyHandle *> column_handles;
    auto status = rocksdb::DB::OpenForReadOnly(
        options, db_path, column_families, &column_handles, &db);
    if (!status.ok()) {
      throwError("Failed to open database at {}: {}",
                 db_path.native(),
                 status.ToString());
    }

    std::vector<rocksdb::ColumnFamilyMetaData> columns_data;
    db->GetAllColumnFamilyMetaData(&columns_data);
    fmt::print(out,
               "{:{}} | {:{}}    | {:{}} |\n",
               "NAME",
               30,
               "SIZE",
               10,
               "COUNT",
               5);
    for (auto column_data : columns_data) {
      constexpr std::array sizes{"B ", "KB", "MB", "GB", "TB"};
      double size = column_data.size;
      int idx = 0;
      while (size > 1024.0) {
        size /= 1024.0;
        idx++;
      }
      fmt::print(out,
                 "{:{}} | {:{}.2f} {} | {:{}} |\n",
                 column_data.name,
                 30,
                 size,
                 10,
                 sizes[idx],
                 column_data.file_count,
                 5);
    }
  }

 private:
  std::filesystem::path db_path;
};

int storage_explorer_main(int argc, const char **argv) {
  ArgumentList args(argv, argc);

  CommandParser parser;
  parser.addCommand(std::make_unique<PrintHelpCommand>(parser));

  kagome::log::setLevelOfGroup("*", kagome::log::Level::WARN);

  auto logger =
      kagome::log::createLogger("Configuration", kagome::log::defaultGroupName);
  auto configuration =
      std::make_shared<kagome::application::AppConfigurationImpl>();

  int kagome_args_start = -1;
  for (size_t i = 1; i < args.size(); i++) {
    if (strcmp(args[i], "--") == 0) {
      kagome_args_start = i;
    }
  }
  if (kagome_args_start == -1) {
    std::cerr
        << "You must specify arguments for kagome initialization after '--'\n";
    return -1;
  }

  if (!configuration->initializeFromArgs(
          argc - kagome_args_start, args.subspan(kagome_args_start).data())) {
    std::cerr << "Failed to initialize kagome!\n";
    return -1;
  }

  SL_INFO(logger,
          "Kagome storage explorer started. Version: {} ",
          configuration->nodeVersion());

  kagome::injector::KagomeNodeInjector injector{configuration};
  auto block_storage = injector.injectBlockStorage();
  auto trie_storage = injector.injectTrieStorage();
  auto app_state_manager = injector.injectAppStateManager();
  auto block_tree = injector.injectBlockTree();
  auto executor = injector.injectExecutor();
  auto persistent_storage = injector.injectStorage();
  auto chain_spec = injector.injectChainSpec();
  auto hasher = std::make_shared<kagome::crypto::HasherImpl>();

  auto grandpa_api =
      std::make_shared<kagome::runtime::GrandpaApiImpl>(executor);

  auto chain_events_engine = std::make_shared<ChainSubscriptionEngine>();

  auto authority_manager =
      std::make_shared<AuthorityManagerImpl>(app_state_manager,
                                             block_tree,
                                             grandpa_api,
                                             persistent_storage,
                                             chain_events_engine);

  parser.addCommand(std::make_unique<InspectBlockCommand>(block_storage));
  parser.addCommand(std::make_unique<RemoveBlockCommand>(block_storage));
  parser.addCommand(std::make_unique<QueryStateCommand>(trie_storage));
  parser.addCommand(std::make_unique<ChainInfoCommand>(block_tree));
  parser.addCommand(std::make_unique<SearchChainCommand>(
      block_storage, trie_storage, authority_manager, hasher));
  parser.addCommand(std::make_unique<DbStatsCommand>(
      configuration->databasePath(chain_spec->id())));

  parser.invoke(args.first(kagome_args_start));

  SL_INFO(logger, "Kagome storage explorer stopped");
  logger->flush();

  return 0;
}
