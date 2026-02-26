#ifndef STORAGE_H
#define STORAGE_H

#include "contact.h"
#include <vector>
#include <string>

class Storage {
public:
    Storage(const std::string& filename);

    bool load();
    bool save_contact(Contact& c);       // add new
    bool update_contact(const Contact& c); // update existing
    bool delete_contact(uint32_t id);      // soft delete
    std::vector<Contact> get_all_active();
    Contact* find_by_id(uint32_t id);
    std::vector<Contact> search(const std::string& query);

private:
    std::string filename_;
    FileHeader header_;
    std::vector<Contact> contacts_;

    bool write_all();
};

#endif
