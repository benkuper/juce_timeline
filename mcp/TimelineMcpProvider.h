#pragma once
#if ORGANICUI_USE_MCP
namespace organic::mcp
{
class TimelineProvider
{
public:
    explicit TimelineProvider(OrganicProvider&);
private:
    OrganicProvider& organic;
    Sequence* sequence(const juce::String&) const;
    void requireRevision(const juce::var&);
};
}
#endif
