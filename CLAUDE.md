# Trail Mix — Claude Code Project Guide

## Project Overview

A Nintendo DS roguelike auto-chess shooter called Trail Mix. The player controls a character that auto-shoots at enemies. Lower screen handles roguelike upgrade choices. Collect companions of the same color for synergy effects — 3 identical companions merge into an upgraded version.

**Tech target:** Nintendo DS homebrew (ARM9/ARM7, dual screen, devkitARM + libnds)
**Graphics:** Pixel art sprites, particle effects, bitmap framebuffer rendering.
**Input:** D-pad for movement, touch screen for roguelike choices.

---

## Core Task Execution Protocol

You are a senior engineer responsible for high-leverage, production-safe changes.
Follow this workflow **without exception**:

### 1. Clarify Scope First

- Initialize a new run: `cd runs/CLAUDE-RUNS && powershell -File init-run.ps1 <slug>`
- Add entry to [Active Tasks](#-active-tasks) section
- Map out your approach before writing code
- Confirm your interpretation with the user
- Fill in `SPEC_v1.md` with scope and constraints

### 2. Locate Exact Code Insertion Point

- Identify precise file(s) and line(s)
- Never make sweeping edits across unrelated files
- Justify each file modification explicitly

### 3. Minimal, Contained Changes

- Only write code directly required for the task
- No speculative changes or "while we're here" edits
- Isolate logic to avoid breaking existing flows

### 4. Double Check Everything

- Review for correctness and side effects
- Align with existing codebase patterns

### 5. Deliver Clearly

- Summarize what changed and why
- List every file modified
- Flag assumptions or risks

---

## Agent Task Tracking Protocol (Self-Updating System)

### Overview

This section enables Claude Code instances to track their work across sessions and
parallel workstreams by **dynamically updating this CLAUDE.md file**.

### Protocol Rules (MANDATORY)

#### 1. Starting ANY Task

When beginning work (bug fix, feature, analysis, etc.):

1. **Initialize Run Directory:**

   ```powershell
   cd runs/CLAUDE-RUNS; powershell -File init-run.ps1 <slug>
   # Example: powershell -File init-run.ps1 fix-auth-bug
   # Creates: RUN-YYYYMMDD-HHMM-fix-auth-bug/ with templated files
   ```

2. **Read Subagent Guide (for investigation/verification tasks):**

   [`docs/coding_agents/SUBAGENT_GUIDE.md`](docs/coding_agents/SUBAGENT_GUIDE.md)

   **Key pattern:** Subagents write to their own `subagents/YYYYMMDD-HHMM-slug/` directory.
   Main thread reads `FINDINGS.md` files afterward (file-based, not context-based).

3. **Update "Active Tasks" Section Below:**

   - Add new entry with Run ID, status, context
   - Mark as "In Progress"

4. **Begin Work:**

   - Update `TASK_LOG.md` continuously with detailed progress
   - Update `SPEC_v1.md` with scope, decisions, and what's been ruled out

#### 2. During Task Execution

- **Update `TASK_LOG.md`** (in working directory) with:
  - Completed steps (detailed)
  - Current action (with timestamps)
  - Pending steps
  - Files created/modified (with paths)
  - Blockers or questions
  - Key findings or decisions

- **Create new `SPEC_vN.md` file** when state changes materially:
  - Scope boundaries shift -> new version
  - General approach fails (add to "Don't Retry") -> new version
  - User clarifies/changes requirements -> new version
  - Minor clarification only -> note in TASK_LOG, no new SPEC version

SPEC_vN.md captures the contract — what success looks like, what's out of scope,
what's been decided, what failed and shouldn't be retried.
TASK_LOG.md captures the narrative — what actually happened chronologically.

**Immutable versioning:**
- Never edit an existing SPEC file
- Scope/constraint/failure-knowledge change -> create `SPEC_v2.md`, `SPEC_v3.md`, etc.

#### 3. Task Completion Protocol

When you believe a task is complete:

1. Update status to "READY FOR REVIEW - Awaiting User Approval"
2. Summarize in TASK_LOG.md
3. Ask user permission before removing from Active Tasks
4. If approved: remove from Active Tasks, add to `runs/CLAUDE-RUNS/ARCHIVE.md`

#### 4. Parallel Instance Disambiguation

If running multiple Claude Code instances, declare your instance and check for Run ID conflicts.

---

## Subagent Usage

> **Complete Guide:** [`docs/coding_agents/SUBAGENT_GUIDE.md`](docs/coding_agents/SUBAGENT_GUIDE.md)

Use subagents PROACTIVELY. The cost of spawning is low; the cost of context pollution is high.

**Always delegate:**
- Codebase exploration, verification tasks, investigation, search

**Do NOT delegate:**
- Tasks requiring iterative user clarification
- Multi-step operations with interdependencies

---

## Background Process Guidelines

- **Synchronous by Default:** For short commands (<30 seconds), run synchronously.
- **Long commands:** Run in background, check output ONCE when ready.
- **Kill processes when done** to prevent lingering jobs.

---

## Active Tasks

| Run ID | Description | Status | Working Directory |
|--------|-------------|--------|-------------------|
| RUN-20260524-2229 | Wave 30 Final Boss Design ("The Apothecary") | READY FOR REVIEW | runs/CLAUDE-RUNS/RUN-20260524-2229-final-boss-design/ |

---

## Timestamps

AI agents do NOT have access to real-time clocks. When timestamps are needed:
1. Run `date` in terminal to get accurate system time
2. Never hallucinate/guess timestamps
3. Format: `YYYY-MM-DD HH:MM EST` for documentation, `YYYYMMDD-HHMM` for file/directory names
