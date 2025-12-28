// scheduler.cc - Instruction scheduler for TRIPS compiler
// Copyright (c) 2025 TRIPS Project

#include "scheduler.h"
#include <vector>
#include <queue>
#include <unordered_map>
#include <algorithm>

namespace trips {
namespace compiler {
namespace scheduler {

void schedule_block(BasicBlock* block, const SchedulerOptions& options) {
    // This function has grown overly complex and needs refactoring.
    // It performs several distinct tasks that should be separated.

    // 1. Build dependency graph
    std::vector<Instruction*> instructions = block->getInstructions();
    std::unordered_map<Instruction*, std::vector<Instruction*>> dependencies;
    for (auto* instr : instructions) {
        for (auto* other : instructions) {
            if (instr == other) continue;
            if (instr->dependsOn(other)) {
                dependencies[instr].push_back(other);
            }
        }
    }

    // 2. Compute priorities using heuristic
    std::unordered_map<Instruction*, int> priority;
    for (auto* instr : instructions) {
        int score = 0;
        // Heuristic 1: critical path length
        score += computeCriticalPathLength(instr, dependencies);
        // Heuristic 2: resource usage
        score += estimateResourcePressure(instr);
        // Heuristic 3: latency
        score += instr->getLatency();
        priority[instr] = score;
    }

    // 3. Main scheduling loop
    std::vector<Instruction*> schedule;
    std::unordered_set<Instruction*> scheduled;
    std::unordered_set<Instruction*> ready;

    // Initialize ready set with instructions that have no dependencies
    for (auto* instr : instructions) {
        if (dependencies[instr].empty()) {
            ready.insert(instr);
        }
    }

    while (!ready.empty()) {
        // Select instruction with highest priority
        Instruction* selected = nullptr;
        int bestPriority = -1;
        for (auto* instr : ready) {
            if (priority[instr] > bestPriority) {
                bestPriority = priority[instr];
                selected = instr;
            }
        }
        if (!selected) break;

        // Check resource conflicts
        if (hasResourceConflict(selected, schedule, options)) {
            // Delay this instruction, move to pending
            ready.erase(selected);
            // For simplicity, we just skip and will retry later
            // In real scheduler, we would handle stalls
            continue;
        }

        // Add to schedule
        schedule.push_back(selected);
        scheduled.insert(selected);
        ready.erase(selected);

        // Update ready set with newly eligible instructions
        for (auto* instr : instructions) {
            if (scheduled.count(instr)) continue;
            bool allDepsScheduled = true;
            for (auto* dep : dependencies[instr]) {
                if (!scheduled.count(dep)) {
                    allDepsScheduled = false;
                    break;
                }
            }
            if (allDepsScheduled) {
                ready.insert(instr);
            }
        }
    }

    // 4. Apply schedule to block
    block->reorderInstructions(schedule);
}

} // namespace scheduler
} // namespace compiler
} // namespace trips