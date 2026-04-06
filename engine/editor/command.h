#pragma once

#include "core/types.h"
#include <vector>
#include <memory>
#include <functional>

namespace kairo {

// base class for undoable commands
class Command {
public:
    virtual ~Command() = default;
    virtual void execute() = 0;
    virtual void undo() = 0;
    virtual const char* get_name() const = 0;
};

// manages undo/redo stacks using the command pattern
class CommandHistory {
public:
    static constexpr size_t MAX_UNDO_LEVELS = 100;

    void execute(std::unique_ptr<Command> cmd) {
        cmd->execute();
        m_undo_stack.push_back(std::move(cmd));
        m_redo_stack.clear();

        // enforce limit
        if (m_undo_stack.size() > MAX_UNDO_LEVELS) {
            m_undo_stack.erase(m_undo_stack.begin());
        }
    }

    void undo() {
        if (!can_undo()) return;
        auto& cmd = m_undo_stack.back();
        cmd->undo();
        m_redo_stack.push_back(std::move(cmd));
        m_undo_stack.pop_back();
    }

    void redo() {
        if (!can_redo()) return;
        auto& cmd = m_redo_stack.back();
        cmd->execute();
        m_undo_stack.push_back(std::move(cmd));
        m_redo_stack.pop_back();
    }

    bool can_undo() const { return !m_undo_stack.empty(); }
    bool can_redo() const { return !m_redo_stack.empty(); }

    void clear() {
        m_undo_stack.clear();
        m_redo_stack.clear();
    }

    size_t undo_count() const { return m_undo_stack.size(); }
    size_t redo_count() const { return m_redo_stack.size(); }

    const char* next_undo_name() const {
        return can_undo() ? m_undo_stack.back()->get_name() : nullptr;
    }

    const char* next_redo_name() const {
        return can_redo() ? m_redo_stack.back()->get_name() : nullptr;
    }

private:
    std::vector<std::unique_ptr<Command>> m_undo_stack;
    std::vector<std::unique_ptr<Command>> m_redo_stack;
};

// convenience: a command that captures lambdas for execute and undo
class LambdaCommand : public Command {
public:
    LambdaCommand(const char* name, std::function<void()> exec, std::function<void()> undo_fn)
        : m_name(name), m_exec(std::move(exec)), m_undo(std::move(undo_fn)) {}

    void execute() override { m_exec(); }
    void undo() override { m_undo(); }
    const char* get_name() const override { return m_name; }

private:
    const char* m_name;
    std::function<void()> m_exec;
    std::function<void()> m_undo;
};

} // namespace kairo
