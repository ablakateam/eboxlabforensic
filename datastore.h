#ifndef DATASTORE_H
#define DATASTORE_H

#include "models.h"
#include <vector>
#include <string>
#include <fstream>
#include <algorithm>
#include <cctype>
#include <cstring>

// ── EntityStore<T> — generic binary file CRUD ──────────────────────────────

template<typename T>
class EntityStore {
public:
    EntityStore(const std::string& filename, const char magic[4])
        : filename_(filename) {
        memcpy(magic_, magic, 4);
    }

    bool load() {
        std::ifstream in(filename_, std::ios::binary);
        if (!in.is_open()) {
            header_ = FileHeader();
            header_.set_magic(magic_);
            records_.clear();
            return true;
        }

        in.read(reinterpret_cast<char*>(&header_), sizeof(FileHeader));
        if (!header_.check_magic(magic_)) {
            in.close();
            header_ = FileHeader();
            header_.set_magic(magic_);
            records_.clear();
            return false;
        }

        records_.resize(header_.count);
        for (uint32_t i = 0; i < header_.count; i++) {
            in.read(reinterpret_cast<char*>(&records_[i]), sizeof(T));
        }
        in.close();
        return true;
    }

    bool save(T& record) {
        record.id = header_.next_id++;
        record.created_at = time(nullptr);
        record.modified_at = record.created_at;
        record.deleted = 0;
        records_.push_back(record);
        return write_all();
    }

    bool update(const T& record) {
        for (auto& existing : records_) {
            if (existing.id == record.id && !existing.deleted) {
                existing = record;
                existing.modified_at = time(nullptr);
                return write_all();
            }
        }
        return false;
    }

    bool remove(uint32_t id) {
        for (auto& r : records_) {
            if (r.id == id && !r.deleted) {
                r.deleted = 1;
                r.modified_at = time(nullptr);
                return write_all();
            }
        }
        return false;
    }

    std::vector<T> get_all_active() const {
        std::vector<T> result;
        for (const auto& r : records_) {
            if (!r.deleted) result.push_back(r);
        }
        return result;
    }

    T* find_by_id(uint32_t id) {
        for (auto& r : records_) {
            if (r.id == id && !r.deleted) return &r;
        }
        return nullptr;
    }

    // Find all active records where a uint32_t FK field matches a value
    std::vector<T> find_by_fk(uint32_t T::*field, uint32_t value) const {
        std::vector<T> result;
        for (const auto& r : records_) {
            if (!r.deleted && r.*field == value) {
                result.push_back(r);
            }
        }
        return result;
    }

private:
    std::string filename_;
    FileHeader header_;
    std::vector<T> records_;
    char magic_[4];

    bool write_all() {
        std::ofstream out(filename_, std::ios::binary | std::ios::trunc);
        if (!out.is_open()) return false;

        header_.count = static_cast<uint32_t>(records_.size());
        out.write(reinterpret_cast<const char*>(&header_), sizeof(FileHeader));

        for (const auto& r : records_) {
            out.write(reinterpret_cast<const char*>(&r), sizeof(T));
        }
        out.close();
        return true;
    }
};

// ── Helper: case-insensitive substring search ──────────────────────────────

static inline std::string to_lower(const std::string& s) {
    std::string result = s;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char ch) { return std::tolower(ch); });
    return result;
}

static inline bool contains_ci(const char* haystack, const std::string& needle_lower) {
    return to_lower(haystack).find(needle_lower) != std::string::npos;
}

// ── DataStore — facade holding all entity stores ───────────────────────────

class DataStore {
public:
    EntityStore<Client>          clients;
    EntityStore<Case>            cases;
    EntityStore<Evidence>        evidence;
    EntityStore<ChainOfCustody>  chain;
    EntityStore<Examiner>        examiners;
    EntityStore<Custodian>       custodians;
    EntityStore<ActivityLog>     activity;

    std::string data_dir;  // directory where .dat files are stored

    DataStore();
    DataStore(const std::string& data_dir);
    bool load_all();
    bool has_data();
    void seed_demo_data();

    // Convenience: log an activity entry for a case
    void log_activity(uint32_t case_id, uint32_t examiner_id,
                      const char* action, const char* desc);

    // Cross-entity search
    std::vector<Client>   search_clients(const std::string& q);
    std::vector<Case>     search_cases(const std::string& q);
    std::vector<Evidence> search_evidence(const std::string& q);
    std::vector<Examiner> search_examiners(const std::string& q);
    std::vector<Custodian> search_custodians(const std::string& q);
};

#endif
