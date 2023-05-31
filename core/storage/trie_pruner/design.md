# Trie Pruner

## Goal

While the Kagome node is running, remove obsolete trie nodes from the database to reduce its size.

## Possible implementations

### Sliding window

Maintain the list of nodes deleted in the past N blocks.
When a node is deleted for longer than N blocks, prune it.

Insertion: remove new nodes from the list of deleted nodes using index.

Deletion: add deleted nodes to the list. Requires full traversal of deleted sub-trees.

Clear prefix: add all deleted nodes to the list.

Pruning: When a new block is finalized, remove all nodes deleted in the block #N+1.

Issues: nodes are re-used in the current implementation.

### Reference counting

Each node contains a reference counter.

Insertion: -

Deletion: -

Clear prefix: -

Commit: all nodes reachable from new root increase their reference count.

Pruning: take state of block Current - N - 1, decrease reference count of all nodes reachable from this state by 1, remove all reachable nodes with reference count that becomed 0.

### Whitelist

Collect nodes reachable from most recent state roots, delete all other nodes.

Insertion: -

Deletion: -

Clear prefix: -

Pruning: collected a set S of all nodes reachable from the given set of root nodes, delete all nodes not in S.

Issues: performance.

Assume base is pruned, meaning it contains RootNum states of NodeNum nodes in total. The current block is BlockN.
Let's build set S of all stored nodes.
When building state of BlockN+1, create a set S_new of all created nodes which are not in S and removed nodes S_del which are in S.
