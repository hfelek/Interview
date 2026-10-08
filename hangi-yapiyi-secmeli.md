# Which Structure Should I Use? (Pattern Playbook)

Companion to `veri-yapilari-calisma-notu.md` (what each structure is) and `calisma-notlari.md` (lessons learned).
This note answers the other question: **"I read a problem. What do I reach for?"**

Every pattern below has the same shape:

- **Generic question**: the problem in abstract form
- **Signals**: words/constraints in the problem that give it away
- **Use**: structure(s) and how they combine
- **Cost**: time / space
- **Example A, Example B**: two concrete problems (some are classic interview questions, some are embedded-flavored)

---

## 0. The 6-question routine (do this before writing code)

1. **What operation must be fast?** (lookup by key? smallest item? last-in item? oldest item? search in order?)
2. **Does order matter?** (arrival order, sorted order, positional order, none)
3. **Is the size bounded or streaming?** (fixed memory, no `malloc`, data keeps arriving)
4. **Do I need one pass or two?** (count first, then decide; sort first, then scan)
5. **Is there a nested / hierarchical / "most recent" structure?** (stack or tree)
6. **What are n and the value range?** (`n ≤ 10^5` means O(n log n) is fine, O(n²) is not. Small value range means a plain array can replace a hash map)

Then say out loud: *which structure, why, time/space, edge cases.*

---

## 1. Quick lookup table

| If you see... | Reach for |
|---|---|
| "seen before", "duplicate", "unique" | hash set |
| "how many times", "frequency", "most/least common" | hash map (or count array) |
| "find two things that combine to X" | hash map (value → index) |
| "group by", "same key" | hash map of lists |
| values only in a tiny range (0..255, IRQ 0..31) | direct array / bitmask |
| "matching", "nested", "undo", "most recent first" | stack |
| "next greater / smaller element" | monotonic stack |
| "in arrival order", "level by level", "fewest steps" | queue (BFS) |
| fixed memory, ISR → main loop | ring buffer |
| "max/min of last K items" | monotonic deque |
| "K largest / smallest / most frequent" | heap of size K |
| "merge K sorted ..." | min heap |
| "always process the earliest / smallest next" | priority queue (heap) |
| "median of a stream" | two heaps |
| "sorted array", "find position / boundary" | binary search |
| "pair in sorted array", "in place" | two pointers |
| "longest / shortest contiguous segment with property" | sliding window |
| "sum of range", "subarray sums to K" | prefix sum (+ hash map) |
| "intervals", "overlaps", "schedule" | sort, then scan (+ min heap) |
| "tree", "depth", "path", "level" | DFS (recursion/stack) or BFS (queue) |
| "sorted dynamic set, k-th, floor/ceiling" | BST |
| "reverse", "cycle", "middle", "splice" | linked list + pointer tricks |
| "cache", "evict least recently used" | hash map + doubly linked list |
| "O(1) insert, delete **and** random pick" | array + hash map |
| "dependencies", "connections", "shortest path" | graph (adjacency list) + BFS/DFS/heap |
| "highest-priority ready task among few levels" | bitmap + FIFO per level |

---

# Part A: The Patterns

## 1. "Have I seen this before?"
**Signals:** duplicate, repeated, already processed, unique, membership test.
**Use:** **hash set** (insert + contains). One pass.
**Cost:** O(n) time, O(n) space.

- **Example A: Contains Duplicate.** Walk the array; if `contains(x)` return true, else `insert(x)`.
- **Example B (embedded): Drop duplicate sensor packets.** Keep a set of recently seen sequence IDs; ignore a packet whose ID is already in the set. (With a bounded window, pair it with a ring buffer so old IDs expire.)

## 2. "How many times does each thing occur?"
**Signals:** frequency, count, most/least common, majority, first unique.
**Use:** **hash map** key → count. If keys are small (chars, bytes) use a **count array** instead (`int cnt[256]`).
**Pitfall:** if the answer needs the **first** one, scan the original array again (hash tables do not keep order).
**Cost:** O(n) time, O(distinct) space.

- **Example A: First Unique ID** (your Soru 2). Pass 1 count, pass 2 scan array for count == 1.
- **Example B: Valid Anagram / Majority Element.** Anagram: `cnt[c]++` for one string, `cnt[c]--` for the other, all zeros means yes. Majority: count and compare with `n/2`.

## 3. "Find a pair/complement"
**Signals:** two numbers add up to / differ by target, "exactly one solution".
**Use:** **hash map** value → index. For each `x`, look up `target - x`, **then** insert `x` (so an element is never paired with itself).
**Cost:** O(n) time, O(n) space (vs O(n²) brute force).

- **Example A: Two Sum** (your Soru 1 from the first session).
- **Example B: Pair with given difference / Check if array has `x` and `2x`.** Same lookup idea: look for `x + k` or `2x` or `x/2` before inserting.

## 4. "Group things that share a key"
**Signals:** group by, bucket, same signature, categorize.
**Use:** **hash map** key → list (or key → count). The hard part is **defining the key**.
**Cost:** O(n · key cost).

- **Example A: Group Anagrams.** Key = sorted letters, or a 26-count signature. All words with the same key go in the same bucket.
- **Example B (embedded): Group devices by firmware version.** Key = version string/number, value = list of device IDs, so you can answer "which devices need an update" quickly.

## 5. "Tiny, bounded universe of keys"
**Signals:** keys are 0..255, IRQ numbers 0..31, pin numbers, ASCII chars, small enum.
**Use:** **direct-address array** (index = key) or a **bitmask** (`uint32_t` as a set of 0..31). No hashing, O(1), zero collisions, no `malloc`.
**Cost:** O(1) per operation, O(universe) space.

- **Example A: Character frequency / "all characters unique?"** `bool seen[256]` or `uint32_t` bit tricks for a-z.
- **Example B (embedded): Pending-interrupt set / GPIO state.** `pending |= 1u << irq;` to mark, `pending &= ~(1u << irq);` to clear, `pending & (1u << irq)` to test. Handler table `void (*handlers[32])(void)` indexed by IRQ number.

## 6. "Nested, paired, or undo-able; most recent first"
**Signals:** brackets, matching, nested structure, backtrack, undo, "last opened closes first".
**Use:** **stack** (LIFO).
**Cost:** O(n) time, O(depth) space.

- **Example A: Valid Parentheses** (your Soru 1 in this session). Push the expected closer, pop and compare on a closer, stack empty at the end.
- **Example B: Evaluate Reverse Polish Notation / decode `3[ab]`.** Numbers go on the stack; an operator pops two operands and pushes the result. For decode, push (count, partial string) at `[`, pop at `]`.

## 7. "Next greater / next smaller element"
**Signals:** for each element, the next element to the right that is bigger/smaller; spans; "how many days until warmer".
**Use:** **monotonic stack** (stack of indices whose values stay sorted). When a new value breaks the order, pop and resolve those indices.
**Cost:** O(n) amortized, each index pushed and popped once.

- **Example A: Daily Temperatures.** Stack of indices with decreasing temperatures; on a warmer day, pop colder days and set `answer[i] = today - i`.
- **Example B (embedded): Next sensor reading above threshold / stock span.** For each sample, how many consecutive earlier samples were ≤ it. Same stack, store (value, span).

## 8. "I need an extra fact about the structure in O(1)" (augmented stack)
**Signals:** "get min/max in O(1)" while pushing/popping.
**Use:** **stack + auxiliary stack** (or store `(value, minSoFar)` pairs). The aux stack's top is always the current min.
**Cost:** O(1) per op, extra O(n) space.

- **Example A: Min Stack** (your Soru 9).
- **Example B (embedded): Track peak temperature with rollback.** Push each reading with the running max; popping a reading (cancelled measurement) automatically restores the previous max.

## 9. "Process in arrival order / spread outward / fewest steps"
**Signals:** FIFO, level order, shortest path in an **unweighted** graph/grid, minimum number of moves.
**Use:** **queue** + BFS. Mark visited when you **enqueue** (not when you dequeue) to avoid duplicates. Visited = hash set or boolean array.
**Cost:** O(V + E) time, O(width) space.

- **Example A: Binary tree level-order** (your Soru 4).
- **Example B: Shortest path in a grid maze / Rotting Oranges.** Queue holds cells; each BFS layer = one step; first time you reach the target is the shortest distance. Rotting Oranges starts with **all** rotten cells in the queue (multi-source BFS).

## 10. "Streaming data, fixed memory, producer and consumer"
**Signals:** ISR, UART, DMA, "no malloc", "drop or overwrite when full", "keep last N samples".
**Use:** **ring buffer** with free-running unsigned counters, power-of-two size.
**Cost:** O(1) per op, O(N) fixed.

- **Example A: UART RX buffer** (your Soru 3). ISR writes `head`, main loop writes `tail`.
- **Example B: Moving average of the last N samples.** Ring buffer holds the N samples plus a `sum`. New sample: `sum += x - oldest`, overwrite oldest. O(1) per sample, no re-summing.

## 11. "Max/min of the last K items"
**Signals:** sliding window maximum/minimum, "highest reading in the last second".
**Use:** **monotonic deque** (stores indices; values kept in decreasing order for max). Front = answer. Drop from the back while the new value is bigger; drop from the front when the index leaves the window. In C the deque is a ring buffer of indices.
**Cost:** O(n) total. (A heap would be O(n log n) and needs lazy deletion.)

- **Example A: Sliding Window Maximum.**
- **Example B (embedded): Peak current over the last 100 ms.** Same deque, window defined by timestamps instead of count.

## 12. "Top K / K-th largest or smallest"
**Signals:** K largest, K-th largest, K most frequent, "bounded memory O(K)".
**Use:** **heap of size K**. For **largest**, use a **min** heap: if the heap has fewer than K items push; else if `x > top`, replace the top. The top is then the K-th largest.
**Cost:** O(n log K) time, O(K) space. (Sorting is O(n log n) and O(n) memory.)

- **Example A: K-th Largest Element in a Stream** (your Soru 6).
- **Example B: Top K Frequent Elements.** **Combination:** hash map counts → min heap of size K over (count, value).

## 13. "Merge K sorted sources"
**Signals:** K sorted lists/arrays/streams/logs, "merge in order".
**Use:** **min heap** of the current head from each source. Pop smallest, output it, push the next item from the same source.
**Cost:** O(n log K) time, O(K) space.

- **Example A: Merge K Sorted Lists / arrays** (your Soru 7).
- **Example B (embedded): Merge timestamped logs from K sensors/cores** into one chronological log without loading all logs into memory.

## 14. "Always handle the smallest / earliest / highest-priority next"
**Signals:** schedule, deadline, timer, event queue, "next to expire", Dijkstra's frontier.
**Use:** **priority queue** = heap. Key = deadline/priority. Peek tells you the next event in O(1).
**Cost:** O(log n) insert/remove, O(1) peek.

- **Example A: Software timer manager.** Min heap keyed by expiry tick; the hardware timer is programmed for `peek()`. When it fires, pop all expired timers.
- **Example B: Task Scheduler by deadline (EDF) / Meeting Rooms II.** Heap of end times: if the earliest-ending meeting is over before the next one starts, reuse that room (pop), else add a room.

## 15. "Running median"
**Signals:** median of a stream, balance of two halves.
**Use:** **two heaps**: a **max** heap for the lower half and a **min** heap for the upper half. Keep sizes within 1 of each other. Median = top of the bigger heap, or the average of both tops.
**Cost:** O(log n) add, O(1) median.

- **Example A: Find Median from Data Stream** (your Soru 11).
- **Example B (embedded): Robust sensor filter.** Median of the readings seen so far to reject outlier spikes (the median ignores them, the average does not).

## 16. "Search in sorted data / find a boundary"
**Signals:** sorted array, "find position", "first/last such that", monotonic condition, "smallest value that satisfies...".
**Use:** **binary search**. Careful `mid = lo + (hi - lo) / 2`.
**Cost:** O(log n), O(1) space.

- **Example A: Search in sorted array / Search Insert Position / First Bad Version.** The last one searches over **versions** with a monotonic predicate, not over an array.
- **Example B (embedded): Lookup-table calibration.** ADC value → temperature using a sorted table; binary search the bracket, then linearly interpolate. Also "smallest ring buffer size that never overflows on a recorded trace" (binary search on the answer: a bigger buffer never makes overflow more likely, so the predicate is monotonic).

## 17. "Two ends, sorted, or in-place"
**Signals:** sorted array pair, palindrome, reverse, remove duplicates in place, partition, merge two sorted arrays.
**Use:** **two pointers** (one from each end, or a slow "write" pointer and a fast "read" pointer).
**Cost:** O(n) time, O(1) space.

- **Example A: Two Sum II (sorted) / Valid Palindrome.** Sorted pair: sum too small move left pointer up, too big move right pointer down.
- **Example B: Remove Duplicates from Sorted Array in place / Move Zeroes.** `read` scans, `write` marks where the next kept element goes. No extra buffer, which matters on a MCU with tiny RAM.

## 18. "Longest / shortest contiguous segment with a property"
**Signals:** substring/subarray, "at most K distinct", "without repeating", "sum at least S", all values positive or the property is monotonic in window size.
**Use:** **sliding window** (`left`, `right`) + a **hash set/map or count array** to track what is inside the window. Expand `right`, shrink `left` while the window is invalid.
**Cost:** O(n) time (each index enters and leaves once).

- **Example A: Longest Substring Without Repeating Characters.** Window + `last_seen[256]`.
- **Example B: Minimum Size Subarray Sum ≥ S.** Add on the right, subtract on the left while sum ≥ S, track the shortest length.

## 19. "Sum over a range, or subarray summing to K"
**Signals:** many range-sum queries, "number of subarrays with sum K", negative numbers allowed (so a sliding window fails).
**Use:** **prefix sums**. `sum(l..r) = pre[r+1] - pre[l]`. For "count subarrays with sum K" combine with a **hash map** `prefix → count`: at each position add `map[pre - K]`.
**Cost:** O(n) preprocessing, O(1) per query.

- **Example A: Range Sum Query (immutable).**
- **Example B: Subarray Sum Equals K.** **Combination:** prefix sum + hash map.

## 20. "Intervals, overlaps, schedules" (sort first)
**Signals:** intervals, meetings, merge, overlap, "minimum number of resources".
**Use:** **sort** by start (or end), then **one pass**. Add a **min heap** of end times if you need to track concurrent resources.
**Cost:** O(n log n) (sorting dominates).

- **Example A: Merge Intervals.** Sort by start; if the next start ≤ current end, extend the end; else emit.
- **Example B (embedded): Detect overlapping memory regions / DMA transfers.** Sort regions by base address, then scan neighbors for `end > next.start`.

## 21. "Hierarchy and recursion on structure"
**Signals:** tree, parent/child, depth, path, subtree, "for each node...".
**Use:** **tree** with **DFS** (recursion or explicit stack) when the answer comes from subtrees (depth, sum, path, LCA); **BFS** (queue) when the answer is level-based or "closest".
**Cost:** O(n) time, O(height) stack for DFS, O(width) for BFS.
**Embedded:** deep or untrusted input means use an explicit stack with a depth limit, not recursion.

- **Example A: Maximum Depth / Level Order** (your Soru 4).
- **Example B: Path Sum / Lowest Common Ancestor.** Post-order style: combine the answers of the left and right subtrees at each node.

## 22. "Sorted dynamic set; order-based queries"
**Signals:** insert/delete while keeping sorted order, K-th smallest, floor/ceiling (closest value), range queries.
**Use:** **BST** (ideally balanced). **Inorder** gives sorted order.
**Cost:** O(h) per operation (O(log n) if balanced, O(n) if degenerate).

- **Example A: K-th Smallest Element in a BST.** Inorder traversal, stop at the K-th visited node.
- **Example B: Closest Value in a BST / Range Sum of BST.** Walk down comparing with the target (prune whole subtrees that cannot contain the answer).

## 23. "Rearrange nodes with pointer surgery"
**Signals:** linked list, splice, reverse, detect cycle, middle, merge, remove nth from end, **no extra memory**.
**Use:** **linked list** + tricks: dummy head, pointer-to-pointer, fast/slow pointers, three-pointer reverse.
**Cost:** O(n) time, O(1) space.

- **Example A: Reverse Linked List / Merge Two Sorted Lists.**
- **Example B: Linked List Cycle / Middle of the List.** Fast moves two steps, slow moves one. (Embedded: a **free list** of unused nodes in a static pool gives O(1) alloc/free without `malloc`.)

## 24. "Cache with eviction; O(1) access *and* O(1) recency update"
**Signals:** LRU, "least recently used", "evict oldest", "move to front on access".
**Use:** **hash map + doubly linked list.** The map gives key → node in O(1); the list keeps recency order and lets you unlink/move a node in O(1). Head = most recent, tail = least recent = eviction candidate.
**Cost:** O(1) get and put.

- **Example A: LRU Cache** (your Soru 10).
- **Example B (embedded): Connection/handle table with eviction.** A device with room for 8 open BLE connections evicts the least recently active one when a 9th arrives.

## 25. "O(1) insert, delete, **and** random pick"
**Signals:** random element, "getRandom", sample from a changing set, constant-time removal by value.
**Use:** **array + hash map** (`value → index in array`). Delete by swapping the element with the last one, pop the last, and fix the moved element's index in the map.
**Cost:** O(1) average for all three.

- **Example A: Insert Delete GetRandom O(1).**
- **Example B (embedded): Active-node pool with random probing.** Keep a dense array of active node IDs; removal keeps it gap-free so you can pick one uniformly or iterate without skipping holes.

## 26. "Things connected to other things" (graphs, bonus: not in the first note)
**Signals:** dependencies, prerequisites, network/topology, islands, shortest path, "is X reachable from Y".
**Use:** **graph** as an adjacency list (array of lists, or a compact array in CSR form on a MCU).
- Reachability / components: **DFS or BFS** + visited array.
- Fewest hops (unweighted): **BFS**.
- Weighted shortest path: **Dijkstra = min heap + adjacency list**.
- Ordering with dependencies: **topological sort** (BFS using in-degree counts, a queue of ready nodes).
- Dynamic connectivity: **union-find**.

**Cost:** O(V + E), or O((V + E) log V) with a heap.

- **Example A: Number of Islands / Course Schedule.** Islands: DFS/BFS flood fill on a grid. Course Schedule = cycle detection / topological sort.
- **Example B (embedded): Driver/module init order.** Each driver lists what it depends on; a topological sort gives a valid boot order and detects circular dependencies.

## 27. "Fast: which of a few levels has work? Highest priority first, FIFO inside a level"
**Signals:** RTOS ready list, small number of priority levels (≤ 32/64), O(1) scheduling, "no log n allowed in the scheduler".
**Use:** **bitmap + one FIFO queue per level.** Bit `p` of `readyMask` is set if level `p` has a ready task. Highest priority = find-first-set / count-leading-zeros (single instruction, e.g. `__builtin_clz`). Take the front of that level's queue.
**Cost:** O(1) everything.

- **Example A: Pick the next task in a fixed-priority scheduler.** `p = 31 - __builtin_clz(readyMask); task = pop(queue[p]); if (empty(queue[p])) readyMask &= ~(1u << p);`
- **Example B: Interrupt arbitration / event flags.** Pending-event mask; service the lowest set bit first (`__builtin_ctz`), clear it, repeat.

---

# Part B: Combination Cheat Sheet

Many interview problems are really **two** structures glued together. Recognize the glue:

| Combination | Solves | Example |
|---|---|---|
| hash map + array | O(1) lookup **and** O(1) random/dense storage | RandomizedSet |
| hash map + doubly linked list | O(1) lookup **and** O(1) recency/order | LRU cache |
| hash map + min heap | frequency counting then "top K" | Top K Frequent |
| hash map + prefix sum | count subarrays with a target sum | Subarray Sum Equals K |
| hash set/map + sliding window | longest/shortest valid segment | Longest Substring w/o Repeats |
| stack + auxiliary stack | O(1) min/max on a stack | Min Stack |
| two stacks | queue from stacks, or undo/redo | Queue via Stacks, editor undo/redo |
| two heaps (max + min) | median, balanced halves | Median of Stream |
| min heap + sort | resource allocation over intervals | Meeting Rooms II |
| queue + visited set | BFS without revisiting | Shortest path in grid |
| stack + visited set | iterative DFS | Flood fill, topological sort (DFS) |
| min heap + adjacency list | weighted shortest paths | Dijkstra |
| ring buffer + running sum | O(1) moving average / windowed statistics | Moving average filter |
| ring buffer + deque indices | sliding window extremes in fixed memory | Peak over last N samples |
| bitmap + FIFO per level | O(1) priority scheduling | RTOS ready list |
| BST + inorder | order statistics, sorted output | K-th smallest |
| binary search + monotonic predicate | "smallest X such that ok(X)" | First Bad Version, min safe buffer size |

---

# Part C: Constraints That Change the Choice (embedded edition)

| Constraint | What it forces |
|---|---|
| **No `malloc`** | Static arrays, node pools with a free list, open-addressing hash table, ring buffers. Choose capacities up front and decide what happens when full (reject, drop oldest, error). |
| **ISR ↔ main loop** | Single-producer/single-consumer ring buffer with `volatile` + barriers; no mutex in an ISR. Anything with multiple writers needs a short critical section. |
| **Tiny RAM / stack** | Avoid deep recursion; explicit stack with depth limit; in-place algorithms (two pointers) over copying. |
| **Hard real-time** | Prefer O(1) worst case (bitmap + queues, ring buffer) over O(log n) or amortized structures. Hash tables have O(n) worst-case, so bound the load factor. |
| **Small key range** | Direct array or bitmask beats a hash map in size, speed, and determinism. |
| **No floating-point / division** | Power-of-two sizes and masks instead of `%`; shifts instead of divides. |
| **Flash tables (read-only data)** | Sorted `const` table + binary search instead of building a hash map at runtime. |
| **Streaming, can't store everything** | Heap of size K, running statistics, ring buffer of the last N. Think "O(K) memory". |
| **Integer overflow risk** | Use wider types for sums (`long long`), `uint32_t` for wraparound arithmetic, safe `mid = lo + (hi-lo)/2`. |

---

# Part D: Decision Flow (compact)

```
Need to look something up by key?
  ├─ key range tiny (≤ 256)?            → direct array / bitmask
  └─ otherwise                          → hash map / hash set

Need the smallest/largest repeatedly?
  ├─ only top K, or streaming           → heap of size K
  ├─ next-to-process (dynamic)          → priority queue (heap)
  └─ few fixed priority levels          → bitmap + FIFO per level

Need order of arrival?
  ├─ oldest out first                   → queue (ring buffer if fixed memory)
  ├─ newest out first                   → stack
  └─ both ends                          → deque

Data sorted (or can be sorted)?
  ├─ find a value/boundary              → binary search
  ├─ pairs / in-place                   → two pointers
  └─ intervals                          → sort + scan

Contiguous subarray/substring?
  ├─ window grows/shrinks monotonically → sliding window
  └─ sums with negatives / "sum == K"   → prefix sum + hash map

Hierarchy?                              → tree: DFS (subtree answers) / BFS (levels)
Sorted + dynamic + order queries?       → BST
Pointer rearrangement, no extra memory? → linked list
Recency eviction in O(1)?               → hash map + doubly linked list
Dependencies / connections?             → graph: BFS / DFS / topo sort / Dijkstra
```

---

# Part E: Traps When Choosing

1. **Using a heap when order of arrival matters.** A heap gives the smallest, not the **first**. ("First unique" needs the original array order.)
2. **Using a hash map when a sort + scan is simpler/less memory.** If memory is tight and n is moderate, sorting an array in place can beat an O(n) hash table.
3. **Min vs max heap confusion for Top K.** *K largest* uses a **min** heap of size K (so the weakest of the K is on top and easy to evict).
4. **Sliding window on arrays with negatives.** Shrinking a window to reduce the sum fails when negatives exist. Use prefix sums + hash map.
5. **Using recursion by default.** Fine for balanced trees, dangerous for adversarial depth in firmware. State your depth bound or use an explicit stack.
6. **Counting while deciding.** Do not compare counts during the counting pass; counts are not final yet.
7. **Forgetting empty/full/duplicate cases.** Every structure has them: empty stack pop, full ring buffer, duplicate keys in BST/hash, `k > n`.
8. **Not asking about ties and duplicates.** "Which one if two values tie?" and "can values repeat?" change the answer.
9. **Forgetting to state complexity.** Always say time **and** space, and name the dominant term.
10. **Choosing the fancy structure first.** Start from brute force, say why it is too slow (O(n²)), then name the structure that removes the repeated work.

---

# Part F: Practice Prompt

Take any problem from your list and answer these in order, out loud:

1. What is the brute force and its complexity?
2. What repeated work does it do?
3. Which structure removes that repeated work? (Use Part A)
4. Is it one structure or a combination? (Use Part B)
5. What does the embedded constraint change? (Use Part C)
6. What are the edge cases?
7. What are the final time and space complexity?
