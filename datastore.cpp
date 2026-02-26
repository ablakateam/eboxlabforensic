#include "datastore.h"

DataStore::DataStore()
    : clients("clients.dat", "CLT1"),
      cases("cases.dat", "CAS1"),
      evidence("evidence.dat", "EVD1"),
      chain("chain.dat", "COC1"),
      examiners("examiners.dat", "EXM1"),
      custodians("custodians.dat", "CUS1"),
      activity("activity.dat", "LOG1")
{
}

DataStore::DataStore(const std::string& dir)
    : clients(dir + "clients.dat", "CLT1"),
      cases(dir + "cases.dat", "CAS1"),
      evidence(dir + "evidence.dat", "EVD1"),
      chain(dir + "chain.dat", "COC1"),
      examiners(dir + "examiners.dat", "EXM1"),
      custodians(dir + "custodians.dat", "CUS1"),
      activity(dir + "activity.dat", "LOG1"),
      data_dir(dir)
{
}

bool DataStore::load_all() {
    bool ok = true;
    ok = clients.load() && ok;
    ok = cases.load() && ok;
    ok = evidence.load() && ok;
    ok = chain.load() && ok;
    ok = examiners.load() && ok;
    ok = custodians.load() && ok;
    ok = activity.load() && ok;
    return ok;
}

bool DataStore::has_data() {
    return !clients.get_all_active().empty();
}

void DataStore::seed_demo_data() {
    time_t now = time(nullptr);
    time_t day = 86400;

    // ── Examiners ─────────────────────────────────────────────────────────
    {
        Examiner e;
        e.set_name("John Mitchell");
        e.set_title("Senior Digital Forensic Analyst");
        e.set_badge_id("EXM-101");
        e.set_phone("(303) 555-0147");
        e.set_email("j.mitchell@eboxlab.com");
        e.set_certifications("EnCE, GCFE, ACE, CFCE");
        e.set_notes("15 years experience. Expert witness in 40+ cases. Specializes in network forensics and malware analysis.");
        examiners.save(e);
    }
    {
        Examiner e;
        e.set_name("Sarah Chen");
        e.set_title("Forensic Examiner");
        e.set_badge_id("EXM-102");
        e.set_phone("(303) 555-0183");
        e.set_email("s.chen@eboxlab.com");
        e.set_certifications("CCE, CFCE, GCFA");
        e.set_notes("8 years experience. Background in law enforcement digital investigations. Court testimony in 12 cases.");
        examiners.save(e);
    }
    {
        Examiner e;
        e.set_name("David Rodriguez");
        e.set_title("Mobile Forensics Specialist");
        e.set_badge_id("EXM-103");
        e.set_phone("(303) 555-0219");
        e.set_email("d.rodriguez@eboxlab.com");
        e.set_certifications("CCME, EnCE, GCFA, Cellebrite Certified");
        e.set_notes("6 years experience. Mobile device extraction and analysis. iOS and Android specialist.");
        examiners.save(e);
    }
    {
        Examiner e;
        e.set_name("Lisa Park");
        e.set_title("eDiscovery Analyst");
        e.set_badge_id("EXM-104");
        e.set_phone("(303) 555-0256");
        e.set_email("l.park@eboxlab.com");
        e.set_certifications("CEDS, RCA, Relativity Certified, ACEDS");
        e.set_notes("10 years experience. Large-scale document review and processing. TAR/predictive coding expert.");
        examiners.save(e);
    }

    // ── Clients ───────────────────────────────────────────────────────────
    {
        Client c;
        c.set_name("Robert Hargrove");
        c.set_company("Meridian Technologies Inc");
        c.set_phone("(212) 555-0301");
        c.set_email("r.hargrove@meridiantech.com");
        c.set_address("450 Park Avenue, Suite 2200, New York, NY 10022");
        c.set_law_firm("Baker & Sterling LLP");
        c.set_attorney_name("Patricia Thornton");
        c.set_attorney_phone("(212) 555-0400");
        c.set_attorney_email("p.thornton@bakersterling.com");
        c.set_jurisdiction("Southern District of New York");
        c.set_notes("Fortune 500 tech company. Primary contact is General Counsel office. Referral from Baker & Sterling.");
        clients.save(c);
    }
    {
        Client c;
        c.set_name("Jennifer Hawkins");
        c.set_company("(Individual)");
        c.set_phone("(720) 555-0188");
        c.set_email("jen.hawkins@gmail.com");
        c.set_address("1847 Elm Street, Castle Rock, CO 80104");
        c.set_law_firm("Westbrook Family Law");
        c.set_attorney_name("Michael Westbrook");
        c.set_attorney_phone("(303) 555-0322");
        c.set_attorney_email("mwestbrook@westbrooklaw.com");
        c.set_jurisdiction("Douglas County District Court, CO");
        c.set_notes("Divorce/custody matter. Needs phone and computer forensics. Time-sensitive — court deadline March 2026.");
        clients.save(c);
    }
    {
        Client c;
        c.set_name("Margaret Liu");
        c.set_company("Pacific Northwest Insurance Co");
        c.set_phone("(206) 555-0445");
        c.set_email("m.liu@pnwinsurance.com");
        c.set_address("800 Fifth Avenue, Suite 3600, Seattle, WA 98104");
        c.set_law_firm("Reed Morrison & Associates");
        c.set_attorney_name("Alan Reed");
        c.set_attorney_phone("(206) 555-0500");
        c.set_attorney_email("a.reed@reedmorrison.com");
        c.set_jurisdiction("Western District of Washington");
        c.set_notes("Insurance fraud investigation. Multiple claimants. Large data volume expected (2TB+). Ongoing engagement.");
        clients.save(c);
    }
    {
        Client c;
        c.set_name("Thomas Blake");
        c.set_company("Greenfield Manufacturing LLC");
        c.set_phone("(303) 555-0567");
        c.set_email("t.blake@greenfieldmfg.com");
        c.set_address("2100 Industrial Blvd, Denver, CO 80216");
        c.set_law_firm("Greenfield In-House Counsel");
        c.set_attorney_name("Rachel Foster");
        c.set_attorney_phone("(303) 555-0568");
        c.set_attorney_email("r.foster@greenfieldmfg.com");
        c.set_jurisdiction("Northern District of Colorado");
        c.set_notes("Internal investigation — employee misconduct. Privileged and confidential. Board-level oversight required.");
        clients.save(c);
    }

    // ── Cases ─────────────────────────────────────────────────────────────
    // Case 1: Meridian Data Breach (Criminal, Analysis)
    {
        Case cs;
        cs.client_id = 1;
        cs.set_case_number("EBX-2026-0001");
        cs.set_court_case_number("1:26-cr-00142-PAE");
        cs.set_title("Meridian Corp Data Breach Investigation");
        cs.set_description("Investigate unauthorized access to Meridian Technologies corporate network. 50,000+ customer records exposed. FBI referral.");
        cs.case_type = CTYPE_CRIMINAL;
        cs.status = STATUS_ANALYSIS;
        cs.assigned_examiner_id = 1;
        cs.set_law_firm("Baker & Sterling LLP");
        cs.set_attorney_name("Patricia Thornton");
        cs.set_judge_name("Hon. Paul A. Engelmayer");
        cs.set_jurisdiction("US District Court, SDNY");
        cs.set_location("EBOXLAB Denver Lab / Meridian HQ, NYC");
        cs.opened_date = now - 45 * day;
        cases.save(cs);
    }
    // Case 2: Hawkins v. Hawkins (Civil, Collection)
    {
        Case cs;
        cs.client_id = 2;
        cs.set_case_number("EBX-2026-0002");
        cs.set_court_case_number("2026-DR-00347");
        cs.set_title("Hawkins v. Hawkins Digital Evidence");
        cs.set_description("Digital forensics for divorce proceeding. Recover deleted communications and financial records from spouse's devices.");
        cs.case_type = CTYPE_CIVIL;
        cs.status = STATUS_COLLECTION;
        cs.assigned_examiner_id = 2;
        cs.set_law_firm("Westbrook Family Law");
        cs.set_attorney_name("Michael Westbrook");
        cs.set_judge_name("Hon. Maria Vasquez");
        cs.set_jurisdiction("Douglas County District Court, CO");
        cs.set_location("Castle Rock, CO");
        cs.opened_date = now - 12 * day;
        cases.save(cs);
    }
    // Case 3: Pacific NW Insurance Fraud (Insurance, Processing)
    {
        Case cs;
        cs.client_id = 3;
        cs.set_case_number("EBX-2026-0003");
        cs.set_court_case_number("");
        cs.set_title("Pacific NW Insurance Fraud Ring");
        cs.set_description("Investigate organized insurance fraud ring. Analyze email communications and financial documents across 8 claimants.");
        cs.case_type = CTYPE_INSURANCE;
        cs.status = STATUS_PROCESSING;
        cs.assigned_examiner_id = 4;
        cs.set_law_firm("Reed Morrison & Associates");
        cs.set_attorney_name("Alan Reed");
        cs.set_judge_name("");
        cs.set_jurisdiction("Western District of Washington");
        cs.set_location("EBOXLAB Denver Lab");
        cs.opened_date = now - 30 * day;
        cases.save(cs);
    }
    // Case 4: Greenfield Employee Misconduct (Internal, Review)
    {
        Case cs;
        cs.client_id = 4;
        cs.set_case_number("EBX-2026-0004");
        cs.set_court_case_number("");
        cs.set_title("Greenfield Employee Misconduct");
        cs.set_description("Internal investigation of suspected IP theft by departing engineer. USB and cloud storage analysis. Board-directed.");
        cs.case_type = CTYPE_INTERNAL_INVESTIGATION;
        cs.status = STATUS_REVIEW;
        cs.assigned_examiner_id = 3;
        cs.set_law_firm("Greenfield In-House Counsel");
        cs.set_attorney_name("Rachel Foster");
        cs.set_judge_name("");
        cs.set_jurisdiction("Northern District of Colorado");
        cs.set_location("Greenfield MFG, Denver, CO");
        cs.opened_date = now - 60 * day;
        cases.save(cs);
    }
    // Case 5: State v. Morrison Cybercrime (Criminal, Production)
    {
        Case cs;
        cs.client_id = 1;
        cs.set_case_number("EBX-2026-0005");
        cs.set_court_case_number("2026-CF-00891");
        cs.set_title("State v. Morrison Cybercrime Prosecution");
        cs.set_description("Forensic analysis supporting prosecution of Kevin Morrison for unauthorized computer access and data exfiltration.");
        cs.case_type = CTYPE_CRIMINAL;
        cs.status = STATUS_PRODUCTION;
        cs.assigned_examiner_id = 1;
        cs.set_law_firm("Colorado Attorney General");
        cs.set_attorney_name("DA James Whitfield");
        cs.set_judge_name("Hon. Robert Blackwell");
        cs.set_jurisdiction("Denver District Court, CO");
        cs.set_location("EBOXLAB Denver Lab / Denver PD");
        cs.opened_date = now - 90 * day;
        cases.save(cs);
    }

    // ── Evidence ──────────────────────────────────────────────────────────
    // EVD-001: Server HDD (Case 1)
    {
        Evidence e;
        e.case_id = 1;
        e.set_evidence_number("EVD-001");
        e.set_description("Dell PowerEdge R740 Server HDD - Primary DB Server");
        e.type = ETYPE_HARD_DRIVE;
        e.set_source("Meridian Technologies server room, Rack A-12");
        e.set_md5("a4f2e8b1c3d5f6a7b9c0d1e2f3a4b5c");
        e.set_sha256("9b1e3f5a7c8d2e4f6a0b1c3d5e7f8a9b0c2d4e6f8a1b3c5d7e9f0a2b4c6d8e0");
        e.size_bytes = 2000398934016ULL;
        e.set_location("Evidence Locker A, Shelf 2, Bin 14");
        e.set_seized_by("John Mitchell");
        e.set_seized_location("450 Park Ave, NYC - Server Room B");
        e.acquisition_date = now - 40 * day;
        e.set_notes("Seagate Exos 2TB. Acquired using FTK Imager. Write-blocker Tableau T35u used. Drive in good condition.");
        evidence.save(e);
    }
    // EVD-002: Employee laptop (Case 1)
    {
        Evidence e;
        e.case_id = 1;
        e.set_evidence_number("EVD-002");
        e.set_description("Employee Laptop - Dell Latitude 5520 (S/N: DL5520-8842)");
        e.type = ETYPE_HARD_DRIVE;
        e.set_source("Suspect employee workstation, Meridian 4th floor");
        e.set_md5("b7c8d9e0f1a2b3c4d5e6f7a8b9c0d1e");
        e.set_sha256("1a2b3c4d5e6f7a8b9c0d1e2f3a4b5c6d7e8f9a0b1c2d3e4f5a6b7c8d9e0f1a2");
        e.size_bytes = 512110190592ULL;
        e.set_location("Evidence Locker A, Shelf 2, Bin 15");
        e.set_seized_by("John Mitchell");
        e.set_seized_location("450 Park Ave, NYC - 4th Floor Cubicle 4-217");
        e.acquisition_date = now - 40 * day;
        e.set_notes("NVMe 512GB SSD. BitLocker encrypted - key obtained from IT. Full disk image acquired.");
        evidence.save(e);
    }
    // EVD-003: USB drive (Case 4)
    {
        Evidence e;
        e.case_id = 4;
        e.set_evidence_number("EVD-003");
        e.set_description("SanDisk Ultra 64GB USB 3.0 Flash Drive");
        e.type = ETYPE_USB;
        e.set_source("Departing employee's desk drawer, Greenfield Eng. Dept");
        e.set_md5("c1d2e3f4a5b6c7d8e9f0a1b2c3d4e5f");
        e.set_sha256("2b3c4d5e6f7a8b9c0d1e2f3a4b5c6d7e8f9a0b1c2d3e4f5a6b7c8d9e0f1a2b3");
        e.size_bytes = 62545461248ULL;
        e.set_location("Evidence Locker B, Shelf 1, Bin 3");
        e.set_seized_by("David Rodriguez");
        e.set_seized_location("Greenfield MFG, 2100 Industrial Blvd, Denver");
        e.acquisition_date = now - 55 * day;
        e.set_notes("Found with CAD files and proprietary source code. NTFS formatted. No encryption. Full image acquired.");
        evidence.save(e);
    }
    // EVD-004: iPhone (Case 2)
    {
        Evidence e;
        e.case_id = 2;
        e.set_evidence_number("EVD-004");
        e.set_description("Apple iPhone 14 Pro - Gold 256GB (Custodian: J. Hawkins)");
        e.type = ETYPE_MOBILE;
        e.set_source("Client Jennifer Hawkins, surrendered voluntarily");
        e.set_md5("d4e5f6a7b8c9d0e1f2a3b4c5d6e7f8a");
        e.set_sha256("3c4d5e6f7a8b9c0d1e2f3a4b5c6d7e8f9a0b1c2d3e4f5a6b7c8d9e0f1a2b3c4");
        e.size_bytes = 245465825280ULL;
        e.set_location("Evidence Locker B, Shelf 3, Mobile Safe");
        e.set_seized_by("Sarah Chen");
        e.set_seized_location("Westbrook Family Law office, Castle Rock, CO");
        e.acquisition_date = now - 10 * day;
        e.set_notes("iOS 18.2. Passcode provided by client. Cellebrite UFED extraction performed. Full file system dump obtained.");
        evidence.save(e);
    }
    // EVD-005: Cloud data (Case 1)
    {
        Evidence e;
        e.case_id = 1;
        e.set_evidence_number("EVD-005");
        e.set_description("Google Workspace Export - Meridian Technologies Domain");
        e.type = ETYPE_CLOUD;
        e.set_source("Google Takeout via legal hold, Meridian IT Admin");
        e.set_md5("e5f6a7b8c9d0e1f2a3b4c5d6e7f8a90");
        e.set_sha256("4d5e6f7a8b9c0d1e2f3a4b5c6d7e8f9a0b1c2d3e4f5a6b7c8d9e0f1a2b3c4d5");
        e.size_bytes = 847923015680ULL;
        e.set_location("Secure NAS - \\\\EBOXLAB-NAS\\evidence\\EBX-2026-0001");
        e.set_seized_by("Lisa Park");
        e.set_seized_location("Remote acquisition - Google Admin Console");
        e.acquisition_date = now - 35 * day;
        e.set_notes("Includes Gmail, Drive, and Chat data for 12 custodians. MBOX and PDF format. 789GB total.");
        evidence.save(e);
    }
    // EVD-006: Email PST archive (Case 3)
    {
        Evidence e;
        e.case_id = 3;
        e.set_evidence_number("EVD-006");
        e.set_description("Outlook PST Archive - M. Thompson (2023-2025)");
        e.type = ETYPE_EMAIL;
        e.set_source("PNW Insurance Exchange Server backup, IT Dept");
        e.set_md5("f6a7b8c9d0e1f2a3b4c5d6e7f8a90b1");
        e.set_sha256("5e6f7a8b9c0d1e2f3a4b5c6d7e8f9a0b1c2d3e4f5a6b7c8d9e0f1a2b3c4d5e6");
        e.size_bytes = 15728640000ULL;
        e.set_location("Secure NAS - \\\\EBOXLAB-NAS\\evidence\\EBX-2026-0003");
        e.set_seized_by("Lisa Park");
        e.set_seized_location("PNW Insurance IT, 800 Fifth Ave, Seattle");
        e.acquisition_date = now - 25 * day;
        e.set_notes("14.6GB PST file covering 3 years. Contains flagged communications with suspected fraud ring members.");
        evidence.save(e);
    }
    // EVD-007: Financial documents (Case 3)
    {
        Evidence e;
        e.case_id = 3;
        e.set_evidence_number("EVD-007");
        e.set_description("Financial Statements & Claim Forms Q1-Q4 2025");
        e.type = ETYPE_DOCUMENT;
        e.set_source("PNW Insurance claims database export");
        e.set_md5("a7b8c9d0e1f2a3b4c5d6e7f8a90b1c2");
        e.set_sha256("6f7a8b9c0d1e2f3a4b5c6d7e8f9a0b1c2d3e4f5a6b7c8d9e0f1a2b3c4d5e6f7");
        e.size_bytes = 2147483648ULL;
        e.set_location("Secure NAS - \\\\EBOXLAB-NAS\\evidence\\EBX-2026-0003");
        e.set_seized_by("Lisa Park");
        e.set_seized_location("PNW Insurance Finance Dept, Seattle");
        e.acquisition_date = now - 25 * day;
        e.set_notes("PDF and Excel files. 847 claim forms across 8 claimants. Metadata preserved for timeline analysis.");
        evidence.save(e);
    }
    // EVD-008: Forensic image (Case 5)
    {
        Evidence e;
        e.case_id = 5;
        e.set_evidence_number("EVD-008");
        e.set_description("Forensic Image - Workstation WS-042 (Morrison)");
        e.type = ETYPE_IMAGE;
        e.set_source("Denver PD Evidence Room, seized under warrant");
        e.set_md5("b8c9d0e1f2a3b4c5d6e7f8a90b1c2d3");
        e.set_sha256("7a8b9c0d1e2f3a4b5c6d7e8f9a0b1c2d3e4f5a6b7c8d9e0f1a2b3c4d5e6f7a8");
        e.size_bytes = 1000204886016ULL;
        e.set_location("Evidence Locker A, Shelf 4, Bin 22");
        e.set_seized_by("John Mitchell");
        e.set_seized_location("Morrison residence, 742 Oak St, Denver, CO");
        e.acquisition_date = now - 85 * day;
        e.set_notes("1TB WD Blue SSD. EnCase E01 format. Dual-tool verified (FTK + EnCase). Chain maintained with Denver PD.");
        evidence.save(e);
    }

    // ── Chain of Custody ──────────────────────────────────────────────────
    // EVD-001 chain
    {
        ChainOfCustody c;
        c.evidence_id = 1; c.action = CUSTODY_RECEIVED;
        c.set_handler_name("John Mitchell");
        c.set_from_location("Meridian Technologies, NYC");
        c.set_to_location("EBOXLAB Denver Lab");
        c.date = now - 40 * day;
        c.set_notes("Received from Meridian IT Director under attorney supervision. Sealed evidence bag #EB-2026-014.");
        chain.save(c);
    }
    {
        ChainOfCustody c;
        c.evidence_id = 1; c.action = CUSTODY_ANALYZED;
        c.set_handler_name("John Mitchell");
        c.set_from_location("Evidence Locker A");
        c.set_to_location("Forensic Workstation FW-03");
        c.date = now - 38 * day;
        c.set_notes("Imaging began. Write-blocker verified. Hash values confirmed pre- and post-imaging.");
        chain.save(c);
    }
    {
        ChainOfCustody c;
        c.evidence_id = 1; c.action = CUSTODY_STORED;
        c.set_handler_name("John Mitchell");
        c.set_from_location("Forensic Workstation FW-03");
        c.set_to_location("Evidence Locker A, Shelf 2, Bin 14");
        c.date = now - 37 * day;
        c.set_notes("Imaging complete. Original drive returned to evidence locker. Image stored on EBOXLAB-NAS.");
        chain.save(c);
    }
    // EVD-003 chain
    {
        ChainOfCustody c;
        c.evidence_id = 3; c.action = CUSTODY_RECEIVED;
        c.set_handler_name("David Rodriguez");
        c.set_from_location("Greenfield MFG HR Dept");
        c.set_to_location("EBOXLAB Denver Lab");
        c.date = now - 55 * day;
        c.set_notes("Received from HR Director. Signed custody form. Witness: Rachel Foster (In-House Counsel).");
        chain.save(c);
    }
    {
        ChainOfCustody c;
        c.evidence_id = 3; c.action = CUSTODY_ANALYZED;
        c.set_handler_name("David Rodriguez");
        c.set_from_location("Evidence Locker B");
        c.set_to_location("Forensic Workstation FW-01");
        c.date = now - 52 * day;
        c.set_notes("USB contents imaged. Found 342 CAD files and 18 source code archives. Possible IP exfiltration confirmed.");
        chain.save(c);
    }
    // EVD-004 chain
    {
        ChainOfCustody c;
        c.evidence_id = 4; c.action = CUSTODY_RECEIVED;
        c.set_handler_name("Sarah Chen");
        c.set_from_location("Westbrook Family Law, Castle Rock");
        c.set_to_location("EBOXLAB Denver Lab");
        c.date = now - 10 * day;
        c.set_notes("Client surrendered device voluntarily. Powered off, placed in Faraday bag. Passcode documented separately.");
        chain.save(c);
    }
    {
        ChainOfCustody c;
        c.evidence_id = 4; c.action = CUSTODY_ANALYZED;
        c.set_handler_name("Sarah Chen");
        c.set_from_location("Evidence Locker B, Mobile Safe");
        c.set_to_location("Mobile Forensics Station");
        c.date = now - 8 * day;
        c.set_notes("Cellebrite UFED Premium extraction. Full file system obtained. 47,000 messages recovered including deleted.");
        chain.save(c);
    }
    // EVD-008 chain
    {
        ChainOfCustody c;
        c.evidence_id = 8; c.action = CUSTODY_RECEIVED;
        c.set_handler_name("John Mitchell");
        c.set_from_location("Denver PD Evidence Room");
        c.set_to_location("EBOXLAB Denver Lab");
        c.date = now - 85 * day;
        c.set_notes("Transferred from Denver PD under search warrant #2025-SW-4421. Signed by Det. Brian Kowalski.");
        chain.save(c);
    }
    {
        ChainOfCustody c;
        c.evidence_id = 8; c.action = CUSTODY_ANALYZED;
        c.set_handler_name("John Mitchell");
        c.set_from_location("Evidence Locker A");
        c.set_to_location("Forensic Workstation FW-03");
        c.date = now - 83 * day;
        c.set_notes("Full disk image created. Browser history, registry, and event logs extracted. Malware artifacts identified.");
        chain.save(c);
    }
    {
        ChainOfCustody c;
        c.evidence_id = 8; c.action = CUSTODY_STORED;
        c.set_handler_name("John Mitchell");
        c.set_from_location("Forensic Workstation FW-03");
        c.set_to_location("Evidence Locker A, Shelf 4, Bin 22");
        c.date = now - 80 * day;
        c.set_notes("Analysis complete for current phase. Report drafted. Awaiting DA review before production.");
        chain.save(c);
    }

    // ── Custodians ────────────────────────────────────────────────────────
    {
        Custodian cu;
        cu.case_id = 1;
        cu.set_name("Mark Thompson");
        cu.set_title("IT Director");
        cu.set_department("Information Technology");
        cu.set_email("m.thompson@meridiantech.com");
        cu.set_phone("(212) 555-0310");
        cu.set_notes("Primary data custodian. Controls domain admin access. Legal hold notice served 2026-01-02. Cooperative.");
        custodians.save(cu);
    }
    {
        Custodian cu;
        cu.case_id = 1;
        cu.set_name("Karen Whitfield");
        cu.set_title("VP of Engineering");
        cu.set_department("Engineering");
        cu.set_email("k.whitfield@meridiantech.com");
        cu.set_phone("(212) 555-0315");
        cu.set_notes("Oversees development team. Access to source code repos and internal wikis. Legal hold served.");
        custodians.save(cu);
    }
    {
        Custodian cu;
        cu.case_id = 2;
        cu.set_name("Brian Hawkins");
        cu.set_title("(Opposing Party)");
        cu.set_department("N/A");
        cu.set_email("b.hawkins@gmail.com");
        cu.set_phone("(720) 555-0199");
        cu.set_notes("Opposing party in divorce. Court order for device surrender pending. May have deleted evidence.");
        custodians.save(cu);
    }
    {
        Custodian cu;
        cu.case_id = 3;
        cu.set_name("Susan Walker");
        cu.set_title("Claims Manager");
        cu.set_department("Claims Processing");
        cu.set_email("s.walker@pnwinsurance.com");
        cu.set_phone("(206) 555-0460");
        cu.set_notes("Processed 6 of 8 suspected fraudulent claims. Email and file share data preserved. Interview completed.");
        custodians.save(cu);
    }
    {
        Custodian cu;
        cu.case_id = 4;
        cu.set_name("James Rivera");
        cu.set_title("Senior Software Engineer");
        cu.set_department("Engineering");
        cu.set_email("j.rivera@greenfieldmfg.com");
        cu.set_phone("(303) 555-0580");
        cu.set_notes("Subject of investigation. Resigned 2025-12-15. USB and personal cloud accounts of interest. Access revoked.");
        custodians.save(cu);
    }
    {
        Custodian cu;
        cu.case_id = 5;
        cu.set_name("Amanda Foster");
        cu.set_title("System Administrator");
        cu.set_department("IT Operations");
        cu.set_email("a.foster@denverco.gov");
        cu.set_phone("(303) 555-0601");
        cu.set_notes("Provided server logs and access records. Victim organization system admin. Cooperative witness.");
        custodians.save(cu);
    }

    // ── Activity Log ──────────────────────────────────────────────────────
    log_activity(1, 1, "Case Created", "Meridian Corp Data Breach Investigation opened. Client: Meridian Technologies Inc.");
    log_activity(1, 1, "Evidence Acquired", "EVD-001: Dell PowerEdge R740 Server HDD imaged and verified.");
    log_activity(1, 1, "Evidence Acquired", "EVD-002: Employee laptop Dell Latitude 5520 imaged. BitLocker key obtained.");
    log_activity(1, 4, "Evidence Acquired", "EVD-005: Google Workspace export completed for 12 custodians. 789GB collected.");
    log_activity(1, 1, "Status Changed", "Collection -> Processing -> Analysis. Initial review of server logs complete.");
    log_activity(2, 2, "Case Created", "Hawkins v. Hawkins Digital Evidence case opened. Court deadline: March 2026.");
    log_activity(2, 2, "Evidence Acquired", "EVD-004: iPhone 14 Pro extracted via Cellebrite. 47,000 messages recovered.");
    log_activity(3, 4, "Case Created", "Pacific NW Insurance Fraud Ring investigation initiated. 8 suspect claimants.");
    log_activity(3, 4, "Evidence Acquired", "EVD-006 & EVD-007: Email archive and financial documents collected from PNW Insurance.");
    log_activity(4, 3, "Case Created", "Greenfield Employee Misconduct investigation. Board-directed. Privileged.");
    log_activity(4, 3, "Evidence Acquired", "EVD-003: USB drive imaged. 342 CAD files and 18 source code archives found.");
    log_activity(4, 3, "Status Changed", "Processing -> Analysis -> Review. IP theft confirmed. Report in preparation.");
    log_activity(5, 1, "Case Created", "State v. Morrison Cybercrime. Forensic support for prosecution.");
    log_activity(5, 1, "Evidence Acquired", "EVD-008: Workstation WS-042 imaged under warrant. Malware artifacts found.");
    log_activity(5, 1, "Status Changed", "Analysis -> Review -> Production. Expert report submitted to DA.");
}

void DataStore::log_activity(uint32_t case_id, uint32_t examiner_id,
                             const char* action, const char* desc) {
    ActivityLog entry;
    entry.case_id = case_id;
    entry.examiner_id = examiner_id;
    entry.timestamp = time(nullptr);
    entry.set_action(action);
    entry.set_description(desc);
    activity.save(entry);
}

std::vector<Client> DataStore::search_clients(const std::string& q) {
    std::vector<Client> result;
    std::string ql = to_lower(q);
    for (const auto& c : clients.get_all_active()) {
        if (contains_ci(c.name, ql) || contains_ci(c.company, ql) ||
            contains_ci(c.email, ql) || contains_ci(c.phone, ql) ||
            contains_ci(c.law_firm, ql) || contains_ci(c.attorney_name, ql) ||
            contains_ci(c.jurisdiction, ql))
            result.push_back(c);
    }
    return result;
}

std::vector<Case> DataStore::search_cases(const std::string& q) {
    std::vector<Case> result;
    std::string ql = to_lower(q);
    for (const auto& c : cases.get_all_active()) {
        if (contains_ci(c.case_number, ql) || contains_ci(c.court_case_number, ql) ||
            contains_ci(c.title, ql) || contains_ci(c.description, ql) ||
            contains_ci(c.law_firm, ql) || contains_ci(c.attorney_name, ql) ||
            contains_ci(c.judge_name, ql) || contains_ci(c.jurisdiction, ql) ||
            contains_ci(c.location, ql))
            result.push_back(c);
    }
    return result;
}

std::vector<Evidence> DataStore::search_evidence(const std::string& q) {
    std::vector<Evidence> result;
    std::string ql = to_lower(q);
    for (const auto& e : evidence.get_all_active()) {
        if (contains_ci(e.evidence_number, ql) || contains_ci(e.description, ql) ||
            contains_ci(e.source, ql) || contains_ci(e.md5, ql) ||
            contains_ci(e.sha256, ql) || contains_ci(e.location, ql) ||
            contains_ci(e.seized_by, ql) || contains_ci(e.notes, ql))
            result.push_back(e);
    }
    return result;
}

std::vector<Examiner> DataStore::search_examiners(const std::string& q) {
    std::vector<Examiner> result;
    std::string ql = to_lower(q);
    for (const auto& e : examiners.get_all_active()) {
        if (contains_ci(e.name, ql) || contains_ci(e.title, ql) ||
            contains_ci(e.badge_id, ql) || contains_ci(e.email, ql) ||
            contains_ci(e.certifications, ql))
            result.push_back(e);
    }
    return result;
}

std::vector<Custodian> DataStore::search_custodians(const std::string& q) {
    std::vector<Custodian> result;
    std::string ql = to_lower(q);
    for (const auto& c : custodians.get_all_active()) {
        if (contains_ci(c.name, ql) || contains_ci(c.title, ql) ||
            contains_ci(c.department, ql) || contains_ci(c.email, ql) ||
            contains_ci(c.notes, ql))
            result.push_back(c);
    }
    return result;
}
