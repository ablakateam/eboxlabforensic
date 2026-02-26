#include "storage.h"
#include <fstream>
#include <algorithm>
#include <cctype>

Storage::Storage(const std::string& filename) : filename_(filename) {}

bool Storage::load() {
    std::ifstream in(filename_, std::ios::binary);
    if (!in.is_open()) {
        // No file yet — start fresh
        header_ = FileHeader();
        contacts_.clear();
        return true;
    }

    in.read(reinterpret_cast<char*>(&header_), sizeof(FileHeader));
    if (!header_.is_valid()) {
        in.close();
        header_ = FileHeader();
        contacts_.clear();
        return false;
    }

    contacts_.resize(header_.count);
    for (uint32_t i = 0; i < header_.count; i++) {
        in.read(reinterpret_cast<char*>(&contacts_[i]), sizeof(Contact));
    }

    in.close();
    return true;
}

bool Storage::write_all() {
    std::ofstream out(filename_, std::ios::binary | std::ios::trunc);
    if (!out.is_open()) return false;

    header_.count = static_cast<uint32_t>(contacts_.size());
    out.write(reinterpret_cast<const char*>(&header_), sizeof(FileHeader));

    for (const auto& c : contacts_) {
        out.write(reinterpret_cast<const char*>(&c), sizeof(Contact));
    }

    out.close();
    return true;
}

bool Storage::save_contact(Contact& c) {
    c.id = header_.next_id++;
    c.created_at = time(nullptr);
    c.modified_at = c.created_at;
    c.deleted = 0;
    contacts_.push_back(c);
    return write_all();
}

bool Storage::update_contact(const Contact& c) {
    for (auto& existing : contacts_) {
        if (existing.id == c.id && !existing.deleted) {
            existing = c;
            existing.modified_at = time(nullptr);
            return write_all();
        }
    }
    return false;
}

bool Storage::delete_contact(uint32_t id) {
    for (auto& c : contacts_) {
        if (c.id == id && !c.deleted) {
            c.deleted = 1;
            c.modified_at = time(nullptr);
            return write_all();
        }
    }
    return false;
}

std::vector<Contact> Storage::get_all_active() {
    std::vector<Contact> result;
    for (const auto& c : contacts_) {
        if (!c.deleted) result.push_back(c);
    }
    return result;
}

Contact* Storage::find_by_id(uint32_t id) {
    for (auto& c : contacts_) {
        if (c.id == id && !c.deleted) return &c;
    }
    return nullptr;
}

static std::string to_lower(const std::string& s) {
    std::string result = s;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char ch) { return std::tolower(ch); });
    return result;
}

std::vector<Contact> Storage::search(const std::string& query) {
    std::vector<Contact> result;
    std::string q = to_lower(query);

    for (const auto& c : contacts_) {
        if (c.deleted) continue;
        if (to_lower(c.name).find(q) != std::string::npos ||
            to_lower(c.phone).find(q) != std::string::npos ||
            to_lower(c.email).find(q) != std::string::npos ||
            to_lower(c.company).find(q) != std::string::npos ||
            to_lower(c.notes).find(q) != std::string::npos) {
            result.push_back(c);
        }
    }
    return result;
}
