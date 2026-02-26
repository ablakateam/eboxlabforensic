#ifndef CONTACT_H
#define CONTACT_H

#include <cstdint>
#include <ctime>
#include <cstring>
#include <vector>
#include <string>

struct Contact {
    uint32_t id;
    char name[64];
    char phone[32];
    char email[64];
    char company[64];
    char notes[256];
    time_t created_at;
    time_t modified_at;
    uint8_t deleted; // 0 = active, 1 = soft-deleted

    Contact() {
        memset(this, 0, sizeof(Contact));
    }

    void set_name(const char* s)    { strncpy(name, s, sizeof(name) - 1); }
    void set_phone(const char* s)   { strncpy(phone, s, sizeof(phone) - 1); }
    void set_email(const char* s)   { strncpy(email, s, sizeof(email) - 1); }
    void set_company(const char* s) { strncpy(company, s, sizeof(company) - 1); }
    void set_notes(const char* s)   { strncpy(notes, s, sizeof(notes) - 1); }
};

struct FileHeader {
    char magic[4];       // "CRM1"
    uint32_t count;      // number of records (including deleted)
    uint32_t next_id;    // next auto-increment ID

    FileHeader() : count(0), next_id(1) {
        magic[0] = 'C'; magic[1] = 'R'; magic[2] = 'M'; magic[3] = '1';
    }

    bool is_valid() const {
        return magic[0] == 'C' && magic[1] == 'R' && magic[2] == 'M' && magic[3] == '1';
    }
};

#endif
