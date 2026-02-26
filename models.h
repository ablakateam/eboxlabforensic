#ifndef MODELS_H
#define MODELS_H

#include <cstdint>
#include <ctime>
#include <cstring>
#include <vector>
#include <string>

// ── Enums ──────────────────────────────────────────────────────────────────

enum CaseStatus : uint8_t {
    STATUS_INTAKE = 0,
    STATUS_COLLECTION,
    STATUS_PROCESSING,
    STATUS_ANALYSIS,
    STATUS_REVIEW,
    STATUS_PRODUCTION,
    STATUS_CLOSED,
    STATUS_COUNT
};

[[maybe_unused]] static const char* CaseStatusLabels[] = {
    "Intake", "Collection", "Processing",
    "Analysis", "Review", "Production", "Closed"
};

enum CaseType : uint8_t {
    CTYPE_CIVIL = 0,
    CTYPE_CRIMINAL,
    CTYPE_REGULATORY,
    CTYPE_INTERNAL_INVESTIGATION,
    CTYPE_INSURANCE,
    CTYPE_OTHER,
    CTYPE_COUNT
};

[[maybe_unused]] static const char* CaseTypeLabels[] = {
    "Civil", "Criminal", "Regulatory",
    "Internal Investigation", "Insurance", "Other"
};

enum EvidenceType : uint8_t {
    ETYPE_HARD_DRIVE = 0,
    ETYPE_USB,
    ETYPE_MOBILE,
    ETYPE_CLOUD,
    ETYPE_EMAIL,
    ETYPE_DOCUMENT,
    ETYPE_IMAGE,
    ETYPE_OTHER,
    ETYPE_COUNT
};

[[maybe_unused]] static const char* EvidenceTypeLabels[] = {
    "Hard Drive", "USB Drive", "Mobile Device", "Cloud Data",
    "Email Archive", "Document", "Forensic Image", "Other"
};

enum CustodyAction : uint8_t {
    CUSTODY_RECEIVED = 0,
    CUSTODY_TRANSFERRED,
    CUSTODY_ANALYZED,
    CUSTODY_STORED,
    CUSTODY_RETURNED,
    CUSTODY_DISPOSED,
    CUSTODY_COUNT
};

[[maybe_unused]] static const char* CustodyActionLabels[] = {
    "Received", "Transferred", "Analyzed",
    "Stored", "Returned", "Disposed"
};

// ── File Header ────────────────────────────────────────────────────────────

struct FileHeader {
    char magic[4];
    uint32_t count;
    uint32_t next_id;

    FileHeader() : count(0), next_id(1) {
        memset(magic, 0, sizeof(magic));
    }

    void set_magic(const char m[4]) {
        memcpy(magic, m, 4);
    }

    bool check_magic(const char m[4]) const {
        return memcmp(magic, m, 4) == 0;
    }
};

// ── Client ─────────────────────────────────────────────────────────────────

struct Client {
    uint32_t id;
    char name[64];
    char company[64];
    char phone[32];
    char email[64];
    char address[128];
    char law_firm[64];
    char attorney_name[64];
    char attorney_phone[32];
    char attorney_email[64];
    char jurisdiction[64];
    char notes[256];
    time_t created_at;
    time_t modified_at;
    uint8_t deleted;
    char _pad[7]; // alignment padding

    Client() { memset(this, 0, sizeof(Client)); }

    void set_name(const char* s)           { strncpy(name, s, sizeof(name) - 1); }
    void set_company(const char* s)        { strncpy(company, s, sizeof(company) - 1); }
    void set_phone(const char* s)          { strncpy(phone, s, sizeof(phone) - 1); }
    void set_email(const char* s)          { strncpy(email, s, sizeof(email) - 1); }
    void set_address(const char* s)        { strncpy(address, s, sizeof(address) - 1); }
    void set_law_firm(const char* s)       { strncpy(law_firm, s, sizeof(law_firm) - 1); }
    void set_attorney_name(const char* s)  { strncpy(attorney_name, s, sizeof(attorney_name) - 1); }
    void set_attorney_phone(const char* s) { strncpy(attorney_phone, s, sizeof(attorney_phone) - 1); }
    void set_attorney_email(const char* s) { strncpy(attorney_email, s, sizeof(attorney_email) - 1); }
    void set_jurisdiction(const char* s)   { strncpy(jurisdiction, s, sizeof(jurisdiction) - 1); }
    void set_notes(const char* s)          { strncpy(notes, s, sizeof(notes) - 1); }
};

// ── Case ───────────────────────────────────────────────────────────────────

struct Case {
    uint32_t id;
    uint32_t client_id;
    uint32_t assigned_examiner_id;
    char case_number[32];
    char court_case_number[32];
    char title[128];
    char description[256];
    CaseType case_type;
    CaseStatus status;
    char law_firm[64];
    char attorney_name[64];
    char judge_name[64];
    char jurisdiction[64];
    char location[128];
    time_t opened_date;
    time_t closed_date;
    time_t created_at;
    time_t modified_at;
    uint8_t deleted;
    char _pad[7];

    Case() { memset(this, 0, sizeof(Case)); }

    void set_case_number(const char* s)       { strncpy(case_number, s, sizeof(case_number) - 1); }
    void set_court_case_number(const char* s)  { strncpy(court_case_number, s, sizeof(court_case_number) - 1); }
    void set_title(const char* s)             { strncpy(title, s, sizeof(title) - 1); }
    void set_description(const char* s)       { strncpy(description, s, sizeof(description) - 1); }
    void set_law_firm(const char* s)          { strncpy(law_firm, s, sizeof(law_firm) - 1); }
    void set_attorney_name(const char* s)     { strncpy(attorney_name, s, sizeof(attorney_name) - 1); }
    void set_judge_name(const char* s)        { strncpy(judge_name, s, sizeof(judge_name) - 1); }
    void set_jurisdiction(const char* s)      { strncpy(jurisdiction, s, sizeof(jurisdiction) - 1); }
    void set_location(const char* s)          { strncpy(location, s, sizeof(location) - 1); }
};

// ── Evidence ───────────────────────────────────────────────────────────────

struct Evidence {
    uint32_t id;
    uint32_t case_id;
    char evidence_number[32];
    char description[128];
    EvidenceType type;
    char source[128];
    char md5[33];
    char sha256[65];
    uint64_t size_bytes;
    char location[128];
    char seized_by[64];
    char seized_location[128];
    time_t acquisition_date;
    char notes[256];
    time_t created_at;
    time_t modified_at;
    uint8_t deleted;
    char _pad[7];

    Evidence() { memset(this, 0, sizeof(Evidence)); }

    void set_evidence_number(const char* s) { strncpy(evidence_number, s, sizeof(evidence_number) - 1); }
    void set_description(const char* s)     { strncpy(description, s, sizeof(description) - 1); }
    void set_source(const char* s)          { strncpy(source, s, sizeof(source) - 1); }
    void set_md5(const char* s)             { strncpy(md5, s, sizeof(md5) - 1); }
    void set_sha256(const char* s)          { strncpy(sha256, s, sizeof(sha256) - 1); }
    void set_location(const char* s)        { strncpy(location, s, sizeof(location) - 1); }
    void set_seized_by(const char* s)       { strncpy(seized_by, s, sizeof(seized_by) - 1); }
    void set_seized_location(const char* s) { strncpy(seized_location, s, sizeof(seized_location) - 1); }
    void set_notes(const char* s)           { strncpy(notes, s, sizeof(notes) - 1); }
};

// ── Chain of Custody ───────────────────────────────────────────────────────

struct ChainOfCustody {
    uint32_t id;
    uint32_t evidence_id;
    CustodyAction action;
    char handler_name[64];
    char from_location[128];
    char to_location[128];
    time_t date;
    char notes[256];
    time_t created_at;
    time_t modified_at;
    uint8_t deleted;
    char _pad[7];

    ChainOfCustody() { memset(this, 0, sizeof(ChainOfCustody)); }

    void set_handler_name(const char* s)  { strncpy(handler_name, s, sizeof(handler_name) - 1); }
    void set_from_location(const char* s) { strncpy(from_location, s, sizeof(from_location) - 1); }
    void set_to_location(const char* s)   { strncpy(to_location, s, sizeof(to_location) - 1); }
    void set_notes(const char* s)         { strncpy(notes, s, sizeof(notes) - 1); }
};

// ── Examiner ───────────────────────────────────────────────────────────────

struct Examiner {
    uint32_t id;
    char name[64];
    char title[64];
    char badge_id[32];
    char phone[32];
    char email[64];
    char certifications[256];
    char notes[256];
    time_t created_at;
    time_t modified_at;
    uint8_t deleted;
    char _pad[7];

    Examiner() { memset(this, 0, sizeof(Examiner)); }

    void set_name(const char* s)           { strncpy(name, s, sizeof(name) - 1); }
    void set_title(const char* s)          { strncpy(title, s, sizeof(title) - 1); }
    void set_badge_id(const char* s)       { strncpy(badge_id, s, sizeof(badge_id) - 1); }
    void set_phone(const char* s)          { strncpy(phone, s, sizeof(phone) - 1); }
    void set_email(const char* s)          { strncpy(email, s, sizeof(email) - 1); }
    void set_certifications(const char* s) { strncpy(certifications, s, sizeof(certifications) - 1); }
    void set_notes(const char* s)          { strncpy(notes, s, sizeof(notes) - 1); }
};

// ── Custodian ──────────────────────────────────────────────────────────────

struct Custodian {
    uint32_t id;
    uint32_t case_id;
    char name[64];
    char title[64];
    char department[64];
    char email[64];
    char phone[32];
    char notes[256];
    time_t created_at;
    time_t modified_at;
    uint8_t deleted;
    char _pad[7];

    Custodian() { memset(this, 0, sizeof(Custodian)); }

    void set_name(const char* s)       { strncpy(name, s, sizeof(name) - 1); }
    void set_title(const char* s)      { strncpy(title, s, sizeof(title) - 1); }
    void set_department(const char* s) { strncpy(department, s, sizeof(department) - 1); }
    void set_email(const char* s)      { strncpy(email, s, sizeof(email) - 1); }
    void set_phone(const char* s)      { strncpy(phone, s, sizeof(phone) - 1); }
    void set_notes(const char* s)      { strncpy(notes, s, sizeof(notes) - 1); }
};

// ── Activity Log ───────────────────────────────────────────────────────────

struct ActivityLog {
    uint32_t id;
    uint32_t case_id;
    uint32_t examiner_id;
    char action[128];
    char description[256];
    time_t timestamp;
    time_t created_at;
    time_t modified_at;
    uint8_t deleted;
    char _pad[7];

    ActivityLog() { memset(this, 0, sizeof(ActivityLog)); }

    void set_action(const char* s)      { strncpy(action, s, sizeof(action) - 1); }
    void set_description(const char* s) { strncpy(description, s, sizeof(description) - 1); }
};

#endif
