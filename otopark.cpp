// Parking lot: ticket on arrival if a fitting free slot exists, reject otherwise,
// free the slot on departure.  Build: g++ -std=c++17 -Wall -Wextra -fsanitize=address,undefined otopark.cpp -o otopark
#include <array>
#include <cassert>
#include <cstdio>
#include <optional>
#include <unordered_map>
#include <vector>

enum Size { SMALL = 0, MEDIUM = 1, LARGE = 2, NSIZES = 3 };

class ParkingLot {
public:
    // counts[s] = number of slots of size s
    explicit ParkingLot(const std::array<int, NSIZES>& counts) {
        int id = 0;
        for (int s = 0; s < NSIZES; ++s)
            for (int i = 0; i < counts[s]; ++i) free_[s].push_back(id++);  // slot ids are unique
    }

    // Car of size `car` arrives. Returns ticket number, or nullopt if rejected.
    // Policy: smallest free slot that fits (small car -> small, else medium, else large).
    std::optional<int> arrive(Size car) {
        for (int s = car; s < NSIZES; ++s) {
            if (free_[s].empty()) continue;
            int slot = free_[s].back();
            free_[s].pop_back();
            int ticket = nextTicket_++;
            active_[ticket] = {slot, static_cast<Size>(s)};
            return ticket;
        }
        return std::nullopt;  // no vacant slot that fits
    }

    // Car leaves with its ticket. Returns false for unknown / already used ticket.
    bool depart(int ticket) {
        auto it = active_.find(ticket);
        if (it == active_.end()) return false;
        free_[it->second.size].push_back(it->second.slot);  // slot is vacant again
        active_.erase(it);
        return true;
    }

    int freeSlots(Size s) const { return static_cast<int>(free_[s].size()); }

private:
    struct Placement { int slot; Size size; };
    std::array<std::vector<int>, NSIZES> free_;   // stack of free slot ids per size: O(1)
    std::unordered_map<int, Placement> active_;   // ticket -> where the car is
    int nextTicket_ = 1;                          // never reused
};

int main() {
    ParkingLot lot({1, 1, 1});                    // 1 small, 1 medium, 1 large
    auto t1 = lot.arrive(SMALL);                  // -> small slot
    auto t2 = lot.arrive(SMALL);                  // small full -> medium slot
    auto t3 = lot.arrive(MEDIUM);                 // medium taken -> large slot
    auto t4 = lot.arrive(SMALL);                  // everything full -> rejected
    assert(t1 && t2 && t3 && !t4);
    assert(lot.freeSlots(LARGE) == 0);

    assert(!lot.arrive(LARGE));                   // large car, no large slot
    assert(lot.depart(*t3));                      // large slot freed
    assert(!lot.depart(*t3));                     // same ticket twice -> refused
    assert(!lot.depart(999));                     // unknown ticket
    auto t5 = lot.arrive(LARGE);                  // now fits
    assert(t5 && *t5 != *t3);                     // fresh ticket number
    assert(lot.arrive(MEDIUM) == std::nullopt);
    std::puts("tum testler gecti");
}
