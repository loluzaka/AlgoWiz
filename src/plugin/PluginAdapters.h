#ifndef PLUGINADAPTERS_H
#define PLUGINADAPTERS_H

// Adapters turn the small plugin interfaces into the engine's Algorithm / StructureAlgorithm,
// and are the isolation boundary:
//   * every exception thrown by plugin code is caught and becomes Timeline::error
//   * the produced Timeline goes through validateTimeline(); a malformed one is rejected
// After that, plugin timelines flow through the engine, renderers, Race, Quiz and Report
// exactly like built-in ones - no other code knows the difference.

#include <memory>
#include <string>

#include "CustomAlgorithm.h"
#include "CustomStructure.h"
#include "StepValidator.h"
#include "../engine/StructureAlgorithm.h"

using namespace std;

inline AlgorithmInfo infoFromMetadata(const PluginMetadata& m) {
    AlgorithmInfo a;
    a.id = m.id; a.name = m.name; a.category = "Custom";
    a.best = m.best; a.average = m.average; a.worst = m.worst; a.space = m.space;
    a.description = m.description + (m.author.empty() ? "" : " (plugin by " + m.author + ", v" + m.version + ")");
    a.pseudocode = m.pseudocode;
    return a;
}

// Applies the isolation rules to a finished timeline.
inline void rejectIfMalformed(Timeline& tl, const PluginMetadata& m) {
    if (!tl.error.empty()) return;
    string bad = validateTimeline(tl);
    if (bad.empty()) return;
    tl.steps.clear(); tl.initial.clear();
    tl.error = "Plugin \"" + m.name + "\" produced a malformed step and was rejected. " + bad;
}

class CustomAlgorithmAdapter : public Algorithm {
public:
    explicit CustomAlgorithmAdapter(unique_ptr<CustomAlgorithm> p) : plugin_(std::move(p)), meta_(plugin_->metadata()), info_(infoFromMetadata(meta_)) {}
    const AlgorithmInfo& info() const override { return info_; }

    string validate(const Input& in) const override {
        const string& s = meta_.inputSchema;
        if ((s == "array" || s == "array+target") && in.values.empty()) return "This custom algorithm needs at least one number in the array.";
        if (s == "array+target" && !in.hasTarget) return "This custom algorithm needs a search target.";
        if (s == "words" && in.words.empty()) return "This custom algorithm needs at least one word.";
        return "";
    }

    Timeline run(const Input& in) override {
        Timeline tl;
        tl.algorithm = meta_.id;
        tl.structure = meta_.renders;
        tl.containerNames = {meta_.name};
        tl.inputDescription = to_string(in.values.size()) + " value(s)";
        Recorder rec(tl);
        PluginContext ctx(rec);
        try {
            plugin_->execute(in, ctx);
        } catch (const InvalidOperation& e) {
            tl.steps.clear(); tl.initial.clear(); tl.error = e.what();
            return tl;
        } catch (const exception& e) {
            tl.steps.clear(); tl.initial.clear(); tl.error = "Plugin \"" + meta_.name + "\" failed while running: " + e.what();
            return tl;
        } catch (...) {
            tl.steps.clear(); tl.initial.clear(); tl.error = "Plugin \"" + meta_.name + "\" failed with an unknown error.";
            return tl;
        }
        rejectIfMalformed(tl, meta_);
        return tl;
    }

private:
    unique_ptr<CustomAlgorithm> plugin_;
    PluginMetadata meta_;
    AlgorithmInfo info_;
};

class CustomStructureAdapter : public StructureAlgorithm {
public:
    explicit CustomStructureAdapter(unique_ptr<CustomStructure> p) : plugin_(std::move(p)), meta_(plugin_->metadata()), info_(infoFromMetadata(meta_)) {}
    const AlgorithmInfo& info() const override { return info_; }

    Timeline run(const Input& in) override {
        Timeline tl;
        try {
            tl = StructureAlgorithm::run(in);
        } catch (const InvalidOperation&) {
            ctx_.reset();
            throw;                                   // a normal invalid user action: generate() reports it as is
        } catch (const exception& e) {
            ctx_.reset();
            tl = Timeline(); tl.algorithm = meta_.id;
            tl.error = "Plugin \"" + meta_.name + "\" failed while running: " + e.what();
            return tl;
        } catch (...) {
            ctx_.reset();
            tl = Timeline(); tl.algorithm = meta_.id;
            tl.error = "Plugin \"" + meta_.name + "\" failed with an unknown error.";
            return tl;
        }
        ctx_.reset();
        rejectIfMalformed(tl, meta_);
        return tl;
    }

protected:
    StructureKind structureKind() const override { return meta_.renders; }
    vector<string> containerNames() const override { return {meta_.name}; }
    void begin(const Input& in, Recorder& rec) override { ctx_.reset(new PluginContext(rec)); plugin_->reset(in, *ctx_); }
    void addInitial(int v, Recorder&) override { plugin_->addInitial(v, *ctx_); }
    void execute(const Command& c, Recorder&) override { plugin_->apply(c, *ctx_); }

private:
    unique_ptr<CustomStructure> plugin_;
    PluginMetadata meta_;
    AlgorithmInfo info_;
    unique_ptr<PluginContext> ctx_;
};

#endif
