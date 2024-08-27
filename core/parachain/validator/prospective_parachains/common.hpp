#include <boost/variant.hpp>
#include <map>
#include <optional>
#include <ranges>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "outcome/outcome.hpp"

#include "network/types/collator_messages.hpp"
#include "network/types/collator_messages_vstaging.hpp"
#include "parachain/types.hpp"

namespace kagome::parachain::fragment {

  template <typename... Args>
  using HashMap = std::unordered_map<Args...>;
  template <typename... Args>
  using HashSet = std::unordered_set<Args...>;
  template <typename... Args>
  using Vec = std::vector<Args...>;
  using BitVec = scale::BitVec;
  using ParaId = ParachainId;
  template <typename... Args>
  using Option = std::optional<Args...>;
  template <typename... Args>
  using Map = std::map<Args...>;
  using FragmentTreeMembership = Vec<std::pair<Hash, Vec<size_t>>>;

  using NodePointerRoot = network::Empty;
  using NodePointerStorage = size_t;
  using NodePointer = boost::variant<NodePointerRoot, NodePointerStorage>;

  using CandidateCommitments = network::CandidateCommitments;
  using PersistedValidationData = runtime::PersistedValidationData;

}  // namespace kagome::parachain::fragment
