#include "TimelineMcpProvider.h"
#if ORGANICUI_USE_MCP
namespace organic::mcp
{
Sequence* TimelineProvider::sequence(const String& id) const
{
    auto* result = dynamic_cast<Sequence*>(organic.container(id));
    if (result == nullptr) throw Error(-32602, "Handle is not a sequence"); return result;
}
void TimelineProvider::requireRevision(const var& args)
{
    if ((juce::int64) organic.describe(args["handle"].toString())["revision"] != (juce::int64) args["revision"])
        throw Error(-32006, "Timeline object changed since inspection");
}
TimelineProvider::TimelineProvider(OrganicProvider& provider) : organic(provider)
{
    auto add = [this](String name, String description, var input, unsigned capabilities,
        std::function<var(const var&, Context&)> execute, std::function<void(const var&, const Context&)> validate = {}, bool undo = false, bool effects = false)
    {
        Operation op; op.name = name; op.description = description; op.input = input; op.output = openObjectSchema();
        op.capabilities = capabilities; op.execute = execute; op.prevalidate = validate; op.reversible = undo; op.externalEffects = effects; organic.registry.add(op);
    };
    add("timeline_inspect", "Inspect sequence playback, time, layers, and cues. Follow layer and cue manager handles for authoring.",
        schema(object({{"handle", stringSchema()}}), array({"handle"})), inspect,
        [this](const var& args, Context&)
        {
            auto* s = sequence(args["handle"].toString());
            auto result = organic.describe(args["handle"].toString());
            result.getDynamicObject()->setProperty("playing", s->isPlaying->boolValue());
            result.getDynamicObject()->setProperty("seconds", s->currentTime->getValue());
            result.getDynamicObject()->setProperty("duration", s->totalTime->getValue());
            result.getDynamicObject()->setProperty("layers", organic.handle(s->layerManager.get()));
            result.getDynamicObject()->setProperty("cues", organic.handle(s->cueManager.get()));
            return result;
        });
    auto actionSchema = stringSchema(); actionSchema.getDynamicObject()->setProperty("enum", array({"play", "pause", "stop", "seek"}));
    add("timeline_transport", "Play, pause, stop, or seek an existing sequence. Playback and seek evaluation may emit external output.",
        schema(object({{"handle", stringSchema()}, {"action", actionSchema}, {"seconds", object({{"type", "number"}, {"minimum", 0}})}}), array({"handle", "action"})), control,
        [this](const var& args, Context&)
        {
            auto* s = sequence(args["handle"].toString()); auto action = args["action"].toString();
            if (action == "play") s->playTrigger->trigger();
            else if (action == "pause") s->pauseTrigger->trigger();
            else if (action == "stop") s->stopTrigger->trigger();
            else s->setCurrentTime((double) args["seconds"], true, true, Sequence::TimeChangeKind::Seek);
            return object({{"playing", s->isPlaying->boolValue()}, {"seconds", s->currentTime->getValue()}, {"reversible", false}});
        },
        [this](const var& args, const Context&)
        {
            auto* s = sequence(args["handle"].toString());
            if (args["action"].toString() == "seek" && (!args.hasProperty("seconds") || (double) args["seconds"] > s->totalTime->doubleValue()))
                throw Error(-32602, "Seek time must be within the sequence");
        }, false, true);
    auto revisionProperties = object({{"handle", stringSchema()}, {"revision", integerSchema(1, INT32_MAX)}});
    auto cueProperties = revisionProperties.clone(); cueProperties.getDynamicObject()->setProperty("seconds", object({{"type", "number"}, {"minimum", 0}}));
    cueProperties.getDynamicObject()->setProperty("name", stringSchema());
    add("timeline_add_cue", "Add a cue to a sequence in one undo transaction.", schema(cueProperties, array({"handle", "revision", "seconds"})), edit,
        [this](const var& args, Context&) { return organic.transaction("Add timeline cue", [&]
        {
            auto* s = sequence(args["handle"].toString()); auto* cue = new TimeCue((float) args["seconds"], s->cueManager.get());
            if (args.hasProperty("name")) cue->setNiceName(args["name"].toString());
            s->cueManager->addItem(cue, var(), true); return organic.describe(organic.handle(cue));
        }); },
        [this](const var& args, const Context&) { requireRevision(args); if ((double) args["seconds"] > sequence(args["handle"].toString())->totalTime->doubleValue()) throw Error(-32602, "Cue time exceeds sequence duration"); }, true);
    auto keyProperties = revisionProperties.clone(); keyProperties.getDynamicObject()->setProperty("position", object({{"type", "number"}, {"minimum", 0}}));
    keyProperties.getDynamicObject()->setProperty("value", object({{"type", "number"}}));
    add("timeline_add_key", "Add an automation key. Change its easing and interpolation through the returned parameter handles.", schema(keyProperties, array({"handle", "revision", "position", "value"})), edit,
        [this](const var& args, Context&) { return organic.transaction("Add automation key", [&]
        {
            auto* automation = dynamic_cast<Automation*>(organic.container(args["handle"].toString()));
            auto* key = automation->addKey((float) args["position"], (float) args["value"], true);
            return organic.describe(organic.handle(key));
        }); },
        [this](const var& args, const Context&)
        {
            requireRevision(args); auto* automation = dynamic_cast<Automation*>(organic.container(args["handle"].toString()));
            if (automation == nullptr) throw Error(-32602, "Handle is not an automation");
            if (!automation->allowKeysOutside && (double) args["position"] > automation->length->doubleValue()) throw Error(-32602, "Key position exceeds automation length");
            if ((double) args["value"] < automation->valueRange->x || (double) args["value"] > automation->valueRange->y) throw Error(-32602, "Key value exceeds automation range");
        }, true);
}
}
#endif
