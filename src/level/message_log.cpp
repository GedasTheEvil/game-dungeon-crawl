#include "message_log.h"
#include <algorithm>
#include <cctype>
#include <optional>

int StatusMessage::LifeMs() const { return tone == StatusTone::Plain ? MessageLog::PLAIN_MS : MessageLog::TONED_MS; }

std::string StatusMessage::Shown() const { return repeats > 1 ? text + " (x" + std::to_string(repeats) + ")" : text; }

namespace {
// An XP gain, "<prefix><N> XP" on one line: where its number starts, and the number.
struct XpGain {
	size_t numberAt;
	int xp;
};

std::optional<XpGain> xpGain(const std::string& text) {
	const std::string suffix = " XP";
	if (text.find('\n') != std::string::npos || text.size() <= suffix.size() ||
		text.compare(text.size() - suffix.size(), suffix.size(), suffix) != 0)
		return std::nullopt;
	const size_t end = text.size() - suffix.size();
	size_t start = end;
	while (start > 0 && std::isdigit(static_cast<unsigned char>(text[start - 1])) != 0)
		start--;
	if (start == end || end - start > 6)
		return std::nullopt;
	return XpGain{start, std::stoi(text.substr(start, end - start))};
}

// Merges `text` into m if they are the same line, or XP gains with the same words.
bool merge(StatusMessage& m, const std::string& text) {
	const std::optional<XpGain> mine = xpGain(m.text), told = xpGain(text);
	if (mine && told && mine->numberAt == told->numberAt &&
		m.text.compare(0, mine->numberAt, text, 0, told->numberAt) == 0) {
		m.text = text.substr(0, told->numberAt) + std::to_string(mine->xp + told->xp) + " XP";
		return true;
	}
	if (m.text != text)
		return false;
	m.repeats++;
	return true;
}
} // namespace

void MessageLog::Push(const std::string& text, StatusTone tone, int now, int onLevel) {
	if (onLevel != level) {
		log.clear();
		level = onLevel;
	}
	for (const StatusMessage* shown : Visible(now)) {
		if (shown->tone != tone)
			continue;
		auto it = std::find_if(log.begin(), log.end(), [shown](const StatusMessage& m) { return &m == shown; });
		StatusMessage m = *it;
		if (!merge(m, text))
			continue;
		m.shownMs = now;
		log.erase(it);
		log.push_back(m);
		return;
	}
	log.push_back({text, tone, now, now, 1});
	while (log.size() > LOG_SIZE)
		log.pop_front();
}

std::vector<const StatusMessage*> MessageLog::Visible(int now) const {
	std::vector<const StatusMessage*> shown;
	for (auto it = log.rbegin(); it != log.rend() && static_cast<int>(shown.size()) < VISIBLE; ++it)
		if (now < it->EndMs())
			shown.push_back(&*it);
	std::reverse(shown.begin(), shown.end());
	return shown;
}
